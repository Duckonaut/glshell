#include <wayland-egl-core.h>
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <GL/glew.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "args.h"
#include "glshell.h"

void init_gl(const char* fragment_shader);
void shutdown_gl(void);
void draw_frame(void);

// signal handler
static void signal_cleanup(int sig) {
    printf("[glshell] received signal %d\n", sig);

    if (sig == SIGINT || sig == SIGTERM) {
        printf("[glshell] stopping\n");
        glshell_stop();
    }
}

int main(int argc, char* argv[]) {
    args_t args = args_parse(argc, argv);
    glshell_params_t params = {
        .width = args.width,
        .height = args.height,
        .margin = args.margin,
        .anchor = args.anchor,
        .reserve = args.reserve,
        .layer = args.layer,
        .output_name = args.output_name,
    };

    glshell_init(&params);

    // register signal handler
    if (signal(SIGINT, signal_cleanup) == SIG_ERR) {
        printf("[glshell] error: unable to register signal handler\n");
        exit(1);
    }

    if (signal(SIGTERM, signal_cleanup) == SIG_ERR) {
        printf("[glshell] error: unable to register signal handler\n");
        exit(1);
    }

    // load fragment shader
    FILE* fragment_shader_file = fopen(args.fragment_shader, "r");
    if (fragment_shader_file == NULL) {
        printf("[glshell] error: unable to open fragment shader file\n");
        exit(1);
    }
    fseek(fragment_shader_file, 0, SEEK_END);
    size_t fragment_shader_size = ftell(fragment_shader_file);
    fseek(fragment_shader_file, 0, SEEK_SET);
    char* fragment_shader = malloc(fragment_shader_size + 1);
    fread(fragment_shader, 1, fragment_shader_size, fragment_shader_file);
    fclose(fragment_shader_file);
    fragment_shader[fragment_shader_size] = '\0';

    // initialize glew
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK) {
        printf("[glshell] error: unable to initialize GLEW\n");
        exit(1);
    }

    // set up OpenGL
    init_gl(fragment_shader);

    bool running = true;
    while (running) {
        running = glshell_poll_events();

        draw_frame();

        glshell_swap_buffers();
    }

    glshell_cleanup();

    return 0;
}

const char* c_vertex_shader =
    "#version 330 core\n"
    "\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec2 uv;\n"
    "\n"
    "out vec2 texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = vec4(pos, 1.0);\n"
    "    texcoord = uv;\n"
    "}\n";

struct vertex {
    float pos[3];
    float uv[2];
} __attribute__((packed));

static const struct vertex g_quad_vertices[] = {
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
    { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
    { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
};

static const uint16_t g_quad_indices[] = { 0, 1, 2, 2, 1, 3 };

struct gl_context {
    GLuint program;
    GLuint vao;
    GLuint vbo;
} g_gl_context;

void init_gl(const char* fragment_shader) {
    printf("[glshell] initializing OpenGL\n");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // set up multisampling
    glEnable(GL_MULTISAMPLE);

    // create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create vertex buffer object
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // upload vertex data
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertices), g_quad_vertices, GL_STATIC_DRAW);

    // set up vertex attributes
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(struct vertex),
        (void*)offsetof(struct vertex, pos)
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(struct vertex),
        (void*)offsetof(struct vertex, uv)
    );
    glEnableVertexAttribArray(1);

    // create shader program
    GLuint program = glCreateProgram();
    GLint status;

    // create vertex shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &c_vertex_shader, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint log_length;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &log_length);
        printf("[glshell] error: unable to compile vertex shader\n");
        char* log = malloc(log_length);
        glGetShaderInfoLog(vs, log_length, NULL, log);
        printf("[glshell] error: %s\n", log);
        free(log);
        exit(1);
    }
    glAttachShader(program, vs);

    // create fragment shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint log_length;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &log_length);
        printf("[glshell] error: unable to compile fragment shader\n");
        char* log = malloc(log_length);
        glGetShaderInfoLog(fs, log_length, NULL, log);
        printf("[glshell] error: %s\n", log);
        free(log);
        exit(1);
    }
    glAttachShader(program, fs);

    // link program
    glLinkProgram(program);
    if (glGetError() != GL_NO_ERROR) {
        printf("[glshell] error: unable to link program\n");
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        printf("[glshell] error: %s\n", log);
        exit(1);
    }

    // delete shaders
    glDeleteShader(vs);
    glDeleteShader(fs);

    // set up global context
    g_gl_context.program = program;
    g_gl_context.vao = vao;
    g_gl_context.vbo = vbo;
}

void shutdown_gl(void) {
    glDeleteProgram(g_gl_context.program);
    glDeleteVertexArrays(1, &g_gl_context.vao);
    glDeleteBuffers(1, &g_gl_context.vbo);
}

void draw_frame(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(g_gl_context.vao);
    glUseProgram(g_gl_context.program);
    glUniform1f(glGetUniformLocation(g_gl_context.program, "u_time"), glshell_get_time());
    float resolution[2] = { glshell_get_width(), glshell_get_height() };
    glUniform2fv(glGetUniformLocation(g_gl_context.program, "u_resolution"), 1, resolution);

    // set up model matrix

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, g_quad_indices);
}

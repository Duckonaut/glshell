#include "glshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-util.h>

#include "stb_ds.h"

struct glshell_output_descriptor {
    char* name;
    uint32_t width;
    uint32_t height;
    struct wl_output* wl_output;
};

/* Wayland code */
struct glshell_state {
    /* Globals */
    struct wl_display* wl_display;
    struct wl_registry* wl_registry;
    struct wl_compositor* wl_compositor;
    struct zwlr_layer_shell_v1* zwlr_layer_shell_v1;
    /* Objects */
    struct wl_surface* wl_surface;
    struct wl_egl_window* wl_egl_surface;
    struct zwlr_layer_surface_v1* zwlr_layer_surface_v1;

    // EGL
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;

    // outputs
    struct glshell_output_descriptor* outputs;

    // data
    char* output_name;
    uint32_t output_width;
    uint32_t output_height;

    // time
    struct timespec start_time;
    struct timespec last_time;
    struct timespec current_time;

    // stop
    bool stop;
};

static struct glshell_state* g_state;

static void zwlr_layer_surface_configure(
    void* data,
    struct zwlr_layer_surface_v1* zwlr_layer_surface_v1,
    uint32_t serial,
    uint32_t width,
    uint32_t height
) {
    struct glshell_state* state = data;
    zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
    zwlr_layer_surface_v1_set_size(zwlr_layer_surface_v1, width, height);

    // struct wl_buffer* buffer = draw_frame(state);
    // wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = zwlr_layer_surface_configure,
};

static void wl_output_name(void* data, struct wl_output* wl_output, const char* name) {
    struct glshell_state* state = data;
    for (size_t i = 0; i < arrlenu(state->outputs); i++) {
        struct glshell_output_descriptor* output_descriptor = &state->outputs[i];
        if (output_descriptor->wl_output == wl_output) {
            output_descriptor->name = strdup(name);
            return;
        }
    }
    struct glshell_output_descriptor output_descriptor = {
        .name = strdup(name),
        .width = 0,
        .height = 0,
        .wl_output = wl_output,
    };
    arrput(state->outputs, output_descriptor);
}

// we need to fill in all the fields, but we only care about the name and width/height

static void
wl_output_description(void* data, struct wl_output* wl_output, const char* description) {
    (void)data;
    (void)wl_output;
    (void)description;
}

static void wl_output_done(void* data, struct wl_output* wl_output) {
    (void)data;
    (void)wl_output;
}

static void wl_output_scale(void* data, struct wl_output* wl_output, int32_t scale) {
    (void)data;
    (void)wl_output;
    (void)scale;
}

static void wl_output_geometry(
    void* data,
    struct wl_output* wl_output,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char* make,
    const char* model,
    int32_t transform
) {
    (void)data;
    (void)wl_output;
    (void)x;
    (void)y;
    (void)physical_width;
    (void)physical_height;
    (void)subpixel;
    (void)make;
    (void)model;
    (void)transform;
}

static void wl_output_mode(
    void* data,
    struct wl_output* wl_output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh
) {
    (void)data;
    (void)wl_output;
    (void)flags;
    (void)refresh;

    struct glshell_state* state = data;
    for (size_t i = 0; i < arrlenu(state->outputs); i++) {
        struct glshell_output_descriptor* output_descriptor = &state->outputs[i];
        if (output_descriptor->wl_output == wl_output) {
            output_descriptor->width = width;
            output_descriptor->height = height;
            return;
        }
    }

    struct glshell_output_descriptor output_descriptor = {
        .name = NULL,
        .width = width,
        .height = height,
        .wl_output = wl_output,
    };

    arrput(state->outputs, output_descriptor);
}

static const struct wl_output_listener wl_output_listener = {
    .name = wl_output_name,
    .description = wl_output_description,
    .done = wl_output_done,
    .scale = wl_output_scale,
    .geometry = wl_output_geometry,
    .mode = wl_output_mode,
};

static void registry_global(
    void* data,
    struct wl_registry* wl_registry,
    uint32_t name,
    const char* interface,
    uint32_t version
) {
    (void)version;
    struct glshell_state* state = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor =
            wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->zwlr_layer_shell_v1 =
            wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, version);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct wl_output* wl_output =
            wl_registry_bind(wl_registry, name, &wl_output_interface, version);
        wl_output_add_listener(wl_output, &wl_output_listener, state);
    }
}

static void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name) {
    (void)data;
    (void)wl_registry;
    (void)name;
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

void glshell_init(glshell_params_t* params) {
    struct glshell_state* state = malloc(sizeof(struct glshell_state));
    g_state = state;

    if (params->output_name != NULL) {
        state->output_name = params->output_name;
    }

    state->wl_display = wl_display_connect(NULL);
    state->wl_registry = wl_display_get_registry(state->wl_display);
    wl_registry_add_listener(state->wl_registry, &wl_registry_listener, state);
    wl_display_roundtrip(state->wl_display);

    state->wl_surface = wl_compositor_create_surface(state->wl_compositor);
    struct wl_region* region = wl_compositor_create_region(state->wl_compositor);
    wl_surface_set_input_region(state->wl_surface, region);
    wl_region_destroy(region);

    struct wl_output* output = NULL;

    wl_display_dispatch(state->wl_display);

    for (size_t i = 0; i < arrlenu(state->outputs); i++) {
        struct glshell_output_descriptor* output_descriptor = &state->outputs[i];
        if (output_descriptor->name == NULL) {
            printf("[glshell] error: output %zu has no name\n", i);
            exit(1);
        }
        if (output_descriptor->width == 0 || output_descriptor->height == 0) {
            printf(
                "[glshell] error: output %s has invalid dimensions\n",
                output_descriptor->name
            );
            exit(1);
        }

        printf(
            "[glshell] output %s: %dx%d\n",
            output_descriptor->name,
            output_descriptor->width,
            output_descriptor->height
        );
    }

    if (params->output_name != NULL) {
        for (size_t i = 0; i < arrlenu(state->outputs); i++) {
            struct glshell_output_descriptor* output_descriptor = &state->outputs[i];
            if (strcmp(output_descriptor->name, params->output_name) == 0) {
                output = output_descriptor->wl_output;
                state->output_width = output_descriptor->width;
                state->output_height = output_descriptor->height;

                params->width = state->output_width;
                params->height = state->output_height;
                break;
            }
        }

        if (output == NULL) {
            printf("[glshell] error: output %s not found\n", params->output_name);
            exit(1);
        }
        if (state->output_width == 0 || state->output_height == 0) {
            printf("[glshell] error: output %s has invalid dimensions\n", params->output_name);
            exit(1);
        }
    } else {
        printf("[glshell] output not specified\n");
        printf("[glshell] using default output\n");
        struct glshell_output_descriptor* output_descriptor = &state->outputs[0];
        state->output_width = output_descriptor->width;
        state->output_height = output_descriptor->height;
        printf(
            "[glshell] default output chosen: %s (%dx%d)\n",
            output_descriptor->name,
            output_descriptor->width,
            output_descriptor->height
        );
        if (params->width == 0) {
            params->width = state->output_width;
            printf("[glshell] using default width: %d\n", params->width);
        }
        if (params->height == 0) {
            params->height = state->output_height;
            printf("[glshell] using default height: %d\n", params->height);
        }
    }

    uint32_t surface_width = params->width - 2 * params->margin;
    uint32_t surface_height = params->height - 2 * params->margin;

    state->wl_egl_surface =
        wl_egl_window_create(state->wl_surface, surface_width, surface_height);

    state->zwlr_layer_surface_v1 = zwlr_layer_shell_v1_get_layer_surface(
        state->zwlr_layer_shell_v1,
        state->wl_surface,
        output,
        params->layer,
        PROJECT_NAME
    );
    zwlr_layer_surface_v1_set_size(state->zwlr_layer_surface_v1, surface_width, surface_height);
    zwlr_layer_surface_v1_set_anchor(state->zwlr_layer_surface_v1, params->anchor);
    zwlr_layer_surface_v1_set_margin(
        state->zwlr_layer_surface_v1,
        params->margin,
        params->margin,
        params->margin,
        params->margin
    );
    if (params->reserve) {
        zwlr_layer_surface_v1_set_exclusive_zone(state->zwlr_layer_surface_v1, -1);
    } else {
        zwlr_layer_surface_v1_set_exclusive_zone(state->zwlr_layer_surface_v1, 0);
    }
    zwlr_layer_surface_v1_set_keyboard_interactivity(state->zwlr_layer_surface_v1, 0);
    zwlr_layer_surface_v1_add_listener(
        state->zwlr_layer_surface_v1,
        &layer_surface_listener,
        state
    );

    wl_surface_commit(state->wl_surface);

    if (!eglBindAPI(EGL_OPENGL_API)) {
        printf("[glshell] error: failed to bind OpenGL API\n");
        exit(1);
    }

    state->egl_display = eglGetDisplay(state->wl_display);
    if (state->egl_display == EGL_NO_DISPLAY) {
        printf("[glshell] error: failed to get EGL display\n");
        exit(1);
    }

    EGLint major, minor;

    if (!eglInitialize(state->egl_display, &major, &minor)) {
        printf("[glshell] error: failed to initialize EGL\n");
        exit(1);
    }

    EGLint total_configs;
    EGLConfig egl_config;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_BIT,
        EGL_NONE,
    };

    if (!eglChooseConfig(state->egl_display, config_attribs, &egl_config, 1, &total_configs)) {
        printf("[glshell] error: failed to choose EGL config\n");
        exit(1);
    }

    // setup for GLES instead of GL
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
    };

    state->egl_context =
        eglCreateContext(state->egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);

    if (state->egl_context == EGL_NO_CONTEXT) {
        printf("[glshell] error: failed to create EGL context\n");
        exit(1);
    }

    // print context info
    printf("[glshell] EGL context client APIs: %s\n", eglQueryString(state->egl_display, EGL_CLIENT_APIS));

    EGLint width, height;
    eglQuerySurface(state->egl_display, state->wl_surface, EGL_WIDTH, &width);
    eglQuerySurface(state->egl_display, state->wl_surface, EGL_HEIGHT, &height);

    state->egl_surface = eglCreateWindowSurface(
        state->egl_display,
        egl_config,
        (EGLNativeWindowType)state->wl_egl_surface,
        0
    );

    if (state->egl_surface == EGL_NO_SURFACE) {
        printf("[glshell] error: failed to create EGL surface\n");
        EGLint egl_error = eglGetError();
        char* egl_error_str = NULL;
        switch (egl_error) {
            case EGL_BAD_DISPLAY:
                egl_error_str = "EGL_BAD_DISPLAY";
                break;
            case EGL_NOT_INITIALIZED:
                egl_error_str = "EGL_NOT_INITIALIZED";
                break;
            case EGL_BAD_CONFIG:
                egl_error_str = "EGL_BAD_CONFIG";
                break;
            case EGL_BAD_NATIVE_WINDOW:
                egl_error_str = "EGL_BAD_NATIVE_WINDOW";
                break;
            case EGL_BAD_ATTRIBUTE:
                egl_error_str = "EGL_BAD_ATTRIBUTE";
                break;
            case EGL_BAD_ALLOC:
                egl_error_str = "EGL_BAD_ALLOC";
                break;
            case EGL_BAD_MATCH:
                egl_error_str = "EGL_BAD_MATCH";
                break;
            case EGL_BAD_SURFACE:
                egl_error_str = "EGL_BAD_SURFACE";
                break;
            case EGL_BAD_CURRENT_SURFACE:
                egl_error_str = "EGL_BAD_CURRENT_SURFACE";
                break;
            case EGL_BAD_CONTEXT:
                egl_error_str = "EGL_BAD_CONTEXT";
                break;
            case EGL_BAD_NATIVE_PIXMAP:
                egl_error_str = "EGL_BAD_NATIVE_PIXMAP";
                break;
            case EGL_CONTEXT_LOST:
                egl_error_str = "EGL_CONTEXT_LOST";
                break;
            default:
                egl_error_str = "unknown";
                break;
        }
        printf("[glshell] EGL error: %s\n", egl_error_str);
        exit(1);
    }

    if (!eglMakeCurrent(
            state->egl_display,
            state->egl_surface,
            state->egl_surface,
            state->egl_context
        )) {
        printf("[glshell] error: failed to make EGL context current\n");
        exit(1);
    }

    printf("[glshell] EGL vendor: %s\n", eglQueryString(state->egl_display, EGL_VENDOR));
    printf("[glshell] EGL version: %s\n", eglQueryString(state->egl_display, EGL_VERSION));

    eglSwapInterval(state->egl_display, 1);
}

void glshell_cleanup(void) {
    struct glshell_state* state = g_state;

    for (size_t i = 0; i < arrlenu(state->outputs); i++) {
        struct glshell_output_descriptor* output_descriptor = &state->outputs[i];
        free(output_descriptor->name);
    }

    eglDestroySurface(state->egl_display, state->egl_surface);
    eglDestroyContext(state->egl_display, state->egl_context);
    eglTerminate(state->egl_display);
    eglReleaseThread();

    zwlr_layer_surface_v1_destroy(state->zwlr_layer_surface_v1);
    wl_surface_destroy(state->wl_surface);
    zwlr_layer_shell_v1_destroy(state->zwlr_layer_shell_v1);
    wl_compositor_destroy(state->wl_compositor);
    wl_registry_destroy(state->wl_registry);
    wl_display_disconnect(state->wl_display);

    free(state);
}

void glshell_swap_buffers(void) {
    struct glshell_state* state = g_state;
    eglSwapBuffers(state->egl_display, state->egl_surface);
}

bool glshell_poll_events(void) {
    struct glshell_state* state = g_state;
    return wl_display_dispatch(state->wl_display) != -1 && !state->stop;
}

float glshell_get_delta_time(void) {
    struct glshell_state* state = g_state;
    clock_gettime(CLOCK_MONOTONIC, &state->current_time);
    float delta_time = (state->current_time.tv_sec - state->last_time.tv_sec) +
                       (state->current_time.tv_nsec - state->last_time.tv_nsec) / 1000000000.0f;
    state->last_time = state->current_time;
    return delta_time;
}

float glshell_get_time(void) {
    struct glshell_state* state = g_state;
    clock_gettime(CLOCK_MONOTONIC, &state->current_time);
    float time = (state->current_time.tv_sec - state->start_time.tv_sec) +
                 (state->current_time.tv_nsec - state->start_time.tv_nsec) / 1000000000.0f;
    return time;
}

float glshell_get_width(void) {
    struct glshell_state* state = g_state;
    return state->output_width;
}

float glshell_get_height(void) {
    struct glshell_state* state = g_state;
    return state->output_height;
}

void glshell_stop(void) {
    struct glshell_state* state = g_state;
    state->stop = true;
}

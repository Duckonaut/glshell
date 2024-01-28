#version 330 core

in vec2 texcoord;

out vec4 color;

uniform vec2 u_resolution;
uniform float u_time;

// fragment shader for a mandelbrot set

vec4 mandelbrot(vec2 pos) {
    vec2 z = vec2(0.0, 0.0);
    vec2 c = pos;
    float iter = 0.0;
    float max_iter = 100.0;
    float max_radius = 2.0;
    float radius = 0.0;

    for (int i = 0; i < 100; i++) {
        float x = z.x * z.x - z.y * z.y + c.x;
        float y = 2.0 * z.x * z.y + c.y;
        z = vec2(x, y);
        radius = z.x * z.x + z.y * z.y;
        if (radius > max_radius) {
            break;
        }
        iter += 1.0;
    }

    float color = iter / max_iter;
    return vec4(color, color, color, 1.0);
}

void main() {
    vec2 center = u_resolution / 2.0;
    vec2 diff = texcoord * u_resolution - center;
    vec2 scaled_diff = diff / u_resolution;
    vec2 math_coords = vec2(scaled_diff.x * (u_resolution.x / u_resolution.y), -scaled_diff.y);

    color = mandelbrot(math_coords * 3.0);
}

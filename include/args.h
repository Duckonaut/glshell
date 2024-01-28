#pragma once

#include <stdbool.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>

typedef struct args {
    // glshell_params_t
    int width;
    int height;
    int margin;
    enum zwlr_layer_surface_v1_anchor anchor;
    bool reserve;
    enum zwlr_layer_shell_v1_layer layer;
    char* output_name;

    // specific to this example
    char* fragment_shader;
} args_t;

args_t args_parse(int argc, char* argv[]);

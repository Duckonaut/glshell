#pragma once

#include <stdbool.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>

typedef struct glshell_params {
    int width;
    int height;
    int margin;
    enum zwlr_layer_surface_v1_anchor anchor;
    bool reserve;
    enum zwlr_layer_shell_v1_layer layer;
    char* output_name;
} glshell_params_t;

void glshell_init(glshell_params_t*);
void glshell_swap_buffers(void);
bool glshell_poll_events(void);
void glshell_cleanup(void);
void glshell_stop(void);

float glshell_get_delta_time(void);
float glshell_get_time(void);
float glshell_get_width(void);
float glshell_get_height(void);

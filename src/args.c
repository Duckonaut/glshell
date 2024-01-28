#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>

void usage(char* argv[]) {
    printf(
        "%s " PROJECT_VERSION "\n"
        "get an overlay of your choice on your wayland compositor\n"
        "\n"
        "Usage: %s FRAGMENT [OPTIONS]\n"
        "\n"
        "Arguments:\n"
        "  FRAGMENT                         path to the fragment shader\n"
        "\n"
        "Options:\n"
        "  -w, --width <width>              set the width of the overlay\n"
        "                                   default: image width\n"
        "  -h, --height <height>            set the height of the overlay\n"
        "                                   default: image height\n"
        "  -m, --margin <margin>            set the margin of the overlay\n"
        "                                   default: 0\n"
        "  -a, --anchor <anchor>:<anchor>   set the anchors of the overlay\n"
        "                                   (top|middle|bottom):(left|middle|right)\n"
        "                                   default: top:left\n"
        "  -r, --reserve                    reserve space for the overlay\n"
        "                                   default: false\n"
        "  -l, --layer <layer>              set the layer of the overlay\n"
        "                                   (background|bottom|top|overlay)\n"
        "                                   default: overlay\n"
        "  -o, --output <output>            set the output of the overlay\n"
        "                                   default: NULL\n"
        "\n"
        "Example:\n"
        "  %s example/mandelbrot.frag -l background\n"
        "  %s example/mandelbrot.frag -h 300 -m 10 -a top:middle -r -l bottom\n",
        argv[0],
        argv[0],
        argv[0],
        argv[0]
    );
}

args_t args_parse(int argc, char* argv[]) {
    args_t args = {
        .fragment_shader = NULL,
        .width = 0,
        .height = 0,
        .anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
        .margin = 0,
        .reserve = true,
        .layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        .output_name = NULL,
    };

    if (argc < 2) {
        usage(argv);
        exit(1);
    }

    args.fragment_shader = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            args.width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0) {
            args.height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--margin") == 0) {
            args.margin = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            char* output = argv[++i];
            args.output_name = output;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reserve") == 0) {
            args.reserve = true;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--layer") == 0) {
            char* layer = argv[++i];
            if (strcmp(layer, "background") == 0) {
                args.layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
            } else if (strcmp(layer, "bottom") == 0) {
                args.layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
            } else if (strcmp(layer, "top") == 0) {
                args.layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
            } else if (strcmp(layer, "overlay") == 0) {
                args.layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
            } else {
                usage(argv);
                exit(1);
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--anchor") == 0) {
            char* anchor = argv[++i];
            if (strcmp(anchor, "top:left") == 0) {
                args.anchor =
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
            } else if (strcmp(anchor, "top:middle") == 0) {
                args.anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else if (strcmp(anchor, "top:right") == 0) {
                args.anchor =
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else if (strcmp(anchor, "middle:left") == 0) {
                args.anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
            } else if (strcmp(anchor, "middle:middle") == 0) {
                args.anchor =
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else if (strcmp(anchor, "middle:right") == 0) {
                args.anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else if (strcmp(anchor, "bottom:left") == 0) {
                args.anchor =
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
            } else if (strcmp(anchor, "bottom:middle") == 0) {
                args.anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else if (strcmp(anchor, "bottom:right") == 0) {
                args.anchor =
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            } else {
                usage(argv);
                exit(1);
            }
        } else {
            usage(argv);
            exit(1);
        }
    }
    return args;
}

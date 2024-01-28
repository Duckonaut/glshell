# glshell

Creates a layer surface from a given GLSL fragment shader.

Can be used to create a simple overlay for a wayland compositor, a status bar, or a wallpaper.

## Requirements:
- A wayland compositor supporting `wlr-layer-shell-unstable-v1`

## Usage
`glshell FRAGMENT [OPTIONS]`

### Arguments:
```
  FRAGMENT                         path to the fragment shader
  -w, --width <width>              set the width of the overlay
                                   default: image width
  -h, --height <height>            set the height of the overlay
                                   default: image height
  -m, --margin <margin>            set the margin of the overlay
                                   default: 0
  -a, --anchor <anchor>:<anchor>   set the anchors of the overlay
                                   (top|middle|bottom):(left|middle|right)
                                   default: none (full screen)
  -r, --reserve                    reserve space for the overlay
                                   default: false
  -l, --layer <layer>              set the layer of the overlay
                                   (background|bottom|top|overlay)
                                   default: overlay
  -o, --output <output>            set the output of the overlay
                                   default: NULL
```

### Examples:
```
glshell example/mandelbrot.frag -l background
glshell example/mandelbrot.frag -h 300 -m 10 -a top:middle -r -l bottom
```

## Shader API
The shader is provided with the following uniforms:
```glsl
uniform vec2 u_resolution; // the resolution of the overlay
uniform vec2 u_time;       // the time since the overlay was created in seconds
```

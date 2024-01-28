setup:
    meson setup build

build:
    meson compile -C build

run: build
    ./build/glshell example/example.fs -l background

# Atom orbital visualizer

This desktop app keeps the original quantum equations and particle sampling in
`atom.cpp`. `reference_visual.hpp` draws the result in a dark, reference-style
canvas with a control panel. The unmodified starting source is saved as
`atom_original.cpp.bak`.

Build and run on Linux with GLFW, GLEW, GLM, OpenGL, and GLU installed:

```sh
g++ -std=c++17 -O2 atom.cpp -o atom $(pkg-config --cflags --libs glfw3 glew glu) -lGL -pthread
./atom
```

Drag in the canvas to rotate; scroll to zoom. Drag the sliders to change the
quantum numbers, color intensity, or X/Y/Z clipping planes. A clipping slider
at zero disables that plane. The original keyboard controls are also available:
W/S for `n`, E/D for `l`, R/F for `m`, T/G for particle count, and Esc to close.

For a screenshot of only the app window:

```sh
./atom --screenshot /tmp/atom-preview.ppm
```

# Caelum

A small experimental sandbox for solving the problem of effective dynamic 2D lighting applied to a 3D space. This was to develop a way to make efficient a method of having nice lighting from many light sources, without incurring a huge GPU cost by baking as much data as possible using 2D tile-based visibility algorithms.

Live lighting recalculation has been disabled for the moment due to a bug.

## Dependencies

* An OpenGL 3.3 compatible graphics adapter and driver
* SDL2
* SDL2\_image
* GLEW
* CGLM (provided with source)

## Compiling

#### Linux
Run `make` in the project's root directory to compile the application.
The resulting executable is `bin/caelum`.

#### Windows
Run `win-compile.sh` (Linux) or `win-compile.bat` (Windows) to compile the application for Windows.
The resulting executable is `win32/caelum.exe`

**NOTE:** Compiling for Windows requires MinGW with GCC along with associated dependencies.

## Controls
- W, A, S and D to move
- Left-click to remove wall/tile
- Right-click to add wall-tile

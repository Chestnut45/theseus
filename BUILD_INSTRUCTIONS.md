# Setup Instructions

## Windows

### Setting up Dev Environment

1) Install MinGW-w64 and GCC *OR* Visual Studio Community Edition
2) Install Visual Studio Code
3) Install CMake (At least version 3.12)
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options:\
    A) If you want to use MSVC: e.g. "Visual Studio Community 2022 Release - amd64"\
    B) If you want to use GCC/Mingw: e.g. "GCC 13.2.0 x86_64-w64-mingw32 (mingw64)"
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well (if not, make sure your compiler / kit from step 3 supports C++20)
6) You may need to choose your build target (theseus) from the dropdown the first time

### Known Issues
1) Debugging is currently only configured for use with GDB, if you want to use MSVC with debugging you'll need to setup vsdbg.exe from the C/C++ VS Code extension, or use the visual studio builtin debugger
2) If build succeeds but the app immediately closes before displaying the window, you may need to copy glew32d.dll from /build/bin to C:/Windows/System32
3) On Windows, VSCode may try to attach gdb even to release builds, causing terrible lag spikes. Launch the app manually after build if this happens

## Linux

### Setting up Dev Environment

1) Install gcc or clang
2) Install Visual Studio Code
3) Install CMake (At least version 3.12)
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options (e.g. gcc)
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well
6) You may need to choose your build target (theseus) from the dropdown the first time
7) If you get include errors for "GL/glu.h", you may also need libglu1 development libraries (apt-get install libglu1-mesa libglu1-mesa-dev)
# Setup Instructions

## Windows

### Setting up Dev Environment

1) Install Visual Studio Community Edition 
2) Install Visual Studio Code
3) Install CMake
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options (e.g. gcc)
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well (if not, make sure your compiler / kit from step 3 supports C++20)
6) You may need to choose your build target (theseus) from the dropdown the first time

## Linux

### Setting up Dev Environment

If dev environment not set up yet from other samples or TechLab 2050, follow these steps:

1) Install gcc or clang
2) Install Visual Studio Code
3) Install CMake
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
7) If you see errors about including "GL/glu.h", you may also need libglu1 development libraries (apt-get install libglu1-mesa libglu1-mesa-dev)

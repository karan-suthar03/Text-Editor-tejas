
## A WORK IN PROGRESS...

<hr>

## About the project

This project is an attempt to create a simple text editor that supports most 
of the modern features in a text editor while keeping the application 
reletively light weight.

The editor in its current stage provides following features.
- Basic Text Editing.
- Cursor Movement.
- File Saving.

<video src="./docs/demo.mp4" width="25%" height="25%"> </video>

The Future plans for this editor includes.
- Text manipulations in different editing mode (like vim).
- In-house syntax highlighting for popular languages.
- Multi Cursor editing.

## Build Instructions

### Requirements
- **Windows 10/11**
- **Visual Studio 2022** (MSVC) or `cl.exe` build tools
- **CMake** (optional)

### Build (Cmake)

```cmd
mkdir build
cd build

cmake ..
```

### Build (Command Line)
Run from the project root
```cmd
build.bat
```

### Run
Run from the project root
```cmd
run.bat
```
or if you want to build with Cmake
Run from the build dir created with cmake.
```cmd
cmake --build .
```

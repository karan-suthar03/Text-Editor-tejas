cmake_minimum_required(VERSION 3.16)
project(TextEditor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Add the executable
add_executable(editor src/main.cpp
        src/buffer.cpp
        include/buffer.h
        src/editor.cpp
        include/editor.h
)

# Link Windows libraries (User32, Gdi32)
if (WIN32)
    target_link_libraries(editor PRIVATE user32 gdi32)
endif()

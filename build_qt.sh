#!/bin/bash
echo "Building CHIP-8 Qt Emulator via CMake..."
mkdir -p build
cmake -B build -S .
cmake --build build --config Release

if [ $? -eq 0 ]; then
    echo "======================================================="
    echo "BUILD SUCCESSFUL! Executable: ./build/chip8_qt"
    echo "======================================================="
else
    echo "BUILD FAILED"
fi

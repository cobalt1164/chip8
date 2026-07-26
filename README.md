<div align="center">

<img width="572" height="331" alt="CHIP-8 Emulator Screenshot" src="https://github.com/user-attachments/assets/959c0dc9-4444-4f9a-96d7-e571053d4201" />

# CHIP-8 Emulator (Qt)

**A cross-platform CHIP-8 interpreter written in C++ and QT guis**

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)
![UI Framework](https://img.shields.io/badge/UI-Qt6%2FQt5-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-orange.svg)

</div>

---

## Overview

This is a cross-platform emulator/interpreter for the CHIP-8 VM. It implements the full 35-opcode instruction set, sound and delay timers, XOR sprite rendering, hex keypad input, and customizable color themes.

---

## Controls & Keypad Mapping

CHIP-8 uses a 16-key hexadecimal keypad (`0x0` through `0xF`). These are mapped to standard QWERTY keyboard keys:

```
 CHIP-8 Keypad             PC Keyboard
+---+---+---+---+        +---+---+---+---+
| 1 | 2 | 3 | C |  --->  | 1 | 2 | 3 | 4 |
+---+---+---+---+        +---+---+---+---+
| 4 | 5 | 6 | D |  --->  | Q | W | E | R |
+---+---+---+---+        +---+---+---+---+
| 7 | 8 | 9 | E |  --->  | A | S | D | F |
+---+---+---+---+        +---+---+---+---+
| A | 0 | B | F |  --->  | Z | X | C | V |
+---+---+---+---+        +---+---+---+---+
```

---

##  Building & Running

### Prerequisites
- **C++17 Compiler** (MSVC 2019+, GCC 9+, or Clang 10+)
- **CMake** (>= 3.16)
- **Qt** (Qt 5.15+ or Qt 6.x `Widgets` module)

---

### Windows

#### Using Command Prompt / PowerShell
```cmd
.\build_qt.bat
```

#### Using CMake directly
```cmd
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```
*Executable destination: `bin/chip8_qt.exe`*

---

### macOS

Install dependencies via [Homebrew](https://brew.sh):
```bash
brew install cmake qt
```

Build with CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
*Executable destination: `bin/chip8_qt` or `bin/chip8_qt.app`*

---

### Linux (Ubuntu / Debian / Fedora / Arch)

#### Ubuntu / Debian
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

#### Fedora
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

#### Arch Linux
```bash
sudo pacman -S base-devel cmake qt6-base
```

#### Build with CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
*Executable destination: `bin/chip8_qt`*

---

## License

This project is open-source under the [MIT License](LICENSE).

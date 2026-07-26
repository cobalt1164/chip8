<div align="center">

<img width="572" height="331" alt="CHIP-8 Emulator Screenshot" src="https://github.com/user-attachments/assets/959c0dc9-4444-4f9a-96d7-e571053d4201" />

# CHIP-8 Emulator

**A CHIP-8 interpreter written in C++ with Qt GUIs**

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Cross--Platform-lightgrey.svg)
![UI Frameworks](https://img.shields.io/badge/UI-Win32%20GDI%20%7C%20Qt6%2FQt5-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-orange.svg)

</div>

---

## Overview

This is a complete emulator/interpreter for the classic CHIP-8 VM. It supports the full 35-opcode CHIP-8 instruction set, graphics rendering, timer subsystem, sound feedback, and keyboard input handling. 

---

## Controls & Keypad Mapping

CHIP-8 used a 16-key hexadecimal keypad (`0x0` through `0xF`). These are mapped to the standard QWERTY keyboard layout:

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

## ️Building & Running

### Prerequisites
* **Windows**: Visual Studio 2019/2022 with MSVC C++ Workload.
* **Qt**: Qt 5.15+ or Qt 6.x and CMake 3.16+.
---
```cmd
.\build_qt.bat
```

#### Using CMake directly
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The executable will be located in `bin/chip8_qt`.

---

## License

This project is open-source under the [MIT License](LICENSE).

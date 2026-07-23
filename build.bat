@echo off
echo Setting up Visual Studio MSVC 64-bit environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist bin mkdir bin

echo Compiling CHIP-8 Win32 GUI Emulator...
cl /nologo /EHsc /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I src src\chip8.cpp src\main_win32.cpp /Fe:bin\chip8_win32.exe user32.lib gdi32.lib comdlg32.lib shell32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo BUILD SUCCESSFUL! Executable created in bin\chip8_win32.exe
    echo =======================================================
) else (
    echo.
    echo BUILD FAILED with error code %ERRORLEVEL%
)

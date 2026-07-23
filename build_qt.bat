@echo off
echo Setting up Visual Studio MSVC 64-bit environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

set "PATH=C:\Program Files\CMake\bin;C:\Qt\6.5.3\msvc2019_64\bin;%PATH%"
set "CMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64"

if not exist bin mkdir bin

echo Building CHIP-8 Qt Emulator via CMake...
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CMake generation failed.
    exit /b %ERRORLEVEL%
)

cmake --build build --config Release
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Copying Qt DLL dependencies to bin...
    windeployqt --release bin\chip8_qt.exe
    echo.
    echo =======================================================
    echo BUILD SUCCESSFUL! Executable in bin\chip8_qt.exe
    echo =======================================================
) else (
    echo.
    echo BUILD FAILED with error code %ERRORLEVEL%
)

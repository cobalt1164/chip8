cmake_minimum_required(VERSION 3.16)
project(chip8_emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Binary Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

# Enable Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Enable MSVC PDB Symbol Generation for Release Crash Dump Line Numbers
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# Find Qt6 or fallback to Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
    set(QT_LIBRARIES Qt6::Widgets)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    message(STATUS "Found Qt5: ${Qt5_VERSION}")
    set(QT_LIBRARIES Qt5::Widgets)
endif()

include_directories(src)

add_executable(chip8_qt
    src/chip8.cpp
    src/chip8.h
    src/main_qt.cpp
)

target_link_libraries(chip8_qt PRIVATE ${QT_LIBRARIES})

# Windows Subsystem GUI & Visual Studio Debugger Paths
if (WIN32)
    set_target_properties(chip8_qt PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
        VS_DEBUGGER_ENVIRONMENT "PATH=C:\\Qt\\6.5.3\\msvc2019_64\\bin;$ENV{PATH}"
    )
endif()

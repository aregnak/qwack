# qwack

### Description

A simple League of Legends companion app capable of displaying in game statistics using DX11, SDL3, and ImGui.

### Features

- Dynamic position overlays:
    - CS/min display
    - Current season player ranks
    - Item gold diff per lane
- Menu to toggle any overlay and display some debug information
- More to come soon!

## Installation & Usage

### External Dependencies

Recommended to use vcpkg for:

- SDL3
- OpenSSL

You will also need MSVC to build this project, installed through the Visual Studio Installer.

(All other libraries are included in the libs folder.)

### How To Build

#### CMAKE

You need the OpenSSL library, if you got them using vcpkg, build using:
(Make sure to change the path to vcpkg before running)

```bash
# For a regular build:
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
# OPTIONAL: Debug build
cmake --build build --config Debug

# For a portable build, also see note* below:
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DPORTABLE=ON

cmake --build build --config Release
```

\*Portable build: If you are using vcpkg, you will need to install static libraries:

```
vcpkg install openssl:x64-windows-static
vcpkg install sdl3:x64-windows-static
```

You can also build using Visual Studio after generating solution files using cmake (the first cmake command).
Select the config you want in the editor and build. This might also work for the portable build if the solution
files are generated using the cmake portable command. I will need to test and update this later.

## Enjoy!

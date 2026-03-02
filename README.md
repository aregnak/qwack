# qwack

### Description

A simple League of Legends companion app capable of displaying in game statistics using DX11, SDL3, and ImGui.

### Features

- CS/min display
- Current season player ranks
- Item gold diff per lane
- Settings menu to toggle any overlay
- More to come soon!

### External Dependencies

- SDL3
- ImGui
- OpenSSL & Crypto

You will also need MSVC to build this project.

### How To Build

#### CMAKE

You need the OpenSSL library, if you got them using vcpkg, build using:
(Make sure to change the path to vcpkg before running)

```bash
# For a regular build:
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Debug

# For a portable build, also see note* below:
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DPORTABLE=ON

cmake --build build --config Release
```

*Portable build: If you are using vcpkg like me, you will need to install OpenSSL static libraries:
`vcpkg install openssl:x64-windows-static`

You can also build using Visual Studio, it will use the cmake config.
However, you're on your own for this for now, I will update this section soon.

## Enjoy!

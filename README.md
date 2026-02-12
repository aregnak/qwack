# qwack

### Description
A simple League of Legends companion app capable of displaying in game statistics using DX11, SDL3, and ImGui.

### Features
- CS/min display
- Player ranks
- Item gold diff (soon)
- Yeah that's all...

### External Dependencies
- SDL3
- ImGui
- OpenSSL

### How To Compile
#### CMAKE
You need the OpenSSL library, if you got them using vcpkg, build using:
(Make sure to change the path to vcpkg before running)
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cd build
cmake --build . --config release
```

You can also build using Visual Studio, it will use the cmake config.
However, you're on your own for this for now, I will update this section soon.

## Enjoy!

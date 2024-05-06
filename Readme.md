# NOURTON

Simple file sharing application over network

## Prequisite

- CMake (at least v3.27)
- vcpkg
- C++ compiler with c++23 support (Clang 18/MSVC 17.9)

## Build

if you doesn't have vcpkg installed consider to get the vcpkg first by:

```cmd
> git clone https://github.com/microsoft/vcpkg
> .\vcpkg\bootstrap-vcpkg.bat
```

or go to [vcpkg repository](https://github.com/microsoft/vcpkg) for more detailed explanation.

build project using cmake (change `VCPKG_ROOT` with vcpkg path installation):

```cmd
> cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE={VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
> cmake --build build
```

if you just cloned the vcpkg, then it will become like this:

```cmd
> cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=.\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
> cmake --build build
```

## Usage

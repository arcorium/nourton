# NOURTON

Simple file sharing application over network. Transmitted data is encrypted using Dual Modulus RSA (DM-RSA)
and Camellia.

## Screenshot

<img src="./dashboard.png" alt="drawing" width="700" height="400"/>
<img src="./send_file.png" alt="drawing" width="700" height="400"/>

## Prerequisite

- CMake (at least v3.15)
- vcpkg
- C++ compiler with c++23 support (tested on gcc 14, clang 18, and msvc 17.9)

## Dependencies

All dependencies are defined on `vcpkg.json` file and automatically installed when building the project. Dependencies
that aren't defined on vcpkg.json are located on `/thirdparty` folder, they are:

- [AES](https://github.com/SergeyBel/AES) [MIT]
- [glad](https://github.com/Dav1dde/glad) [MIT]
- [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) [Zlib]
- [Dear ImGui](https://github.com/ocornut/imgui) [MIT]
- [primesieve](https://github.com/kimwalisch/primesieve) [BSD-2]
- [stb](https://github.com/nothings/stb) [Public]
- [tinyfiledialog](https://sourceforge.net/projects/tinyfiledialogs/) [Zlib]

## Build

If you doesn't have vcpkg installed consider to get the vcpkg first by:

```cmd
> git clone https://github.com/microsoft/vcpkg
Windows:
> ./vcpkg/bootstrap-vcpkg.bat
Linux:
> ./vcpkg/bootstrap-vcpkg.sh 
```

Or go to [vcpkg repository](https://github.com/microsoft/vcpkg) for more detailed explanation.

Build project using cmake (change `VCPKG_ROOT` with vcpkg path installation):

```cmd
> cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE={VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
> cmake --build build --target install
```

If you just cloned the vcpkg on this directory, then it will become like this:

```cmd
> cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
> cmake --build build --target install
```

You can clang or gcc as compiler with setting the `-DCMAKE_C_COMPILER=[gcc|clang]`
and `-DCMAKE_CXX_COMPILER=[g++|clang++]`:

```cmd
> cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
> cmake --build build --target install
```

For multi-configuration generators, add `--config Release` when building. Like this:

```cmd
> cmake --build build --target install --config Release
```

**NOTE:** If you want to use global vcpkg you can delete `vcpkg.json` file, so it will not try to build all the
dependencies again.

Application binaries can be found at `./build/nourton` directory, unless you set the directory installation by yourself.

## Usage

Run the server with:

```cmd
> ./nourton-server [--ip xx.xx.xx.xx] [--port xxxx]
```

If the ip or port argument is not defined, it will use the default value. you can see the default value by:

```cmd
> ./nourton-server --help
```

Run the client:

```cmd
> ./nourton-client [--ip xx.xx.xx.xx] [--port xxxx] [--dir ./files]
```

If one of argument is not defined, it will use the default value like on the server. you can see the default value by:

```cmd
> ./nourton-client --help
```

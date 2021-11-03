#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
SOURCE_DIR="$PWD/example"

export PDFium_DIR="$PWD/staging"
[ "$OS" == "windows" ] && export CMAKE_GENERATOR="Visual Studio 16 2019"

case "$OS-$CPU" in
  windows-x86)    
    export CMAKE_GENERATOR_PLATFORM="Win32"
    ;;
  windows-x64)
    export CMAKE_GENERATOR_PLATFORM="x64"
    ;;
  mac-arm64)
    export CMAKE_OSX_ARCHITECTURES="arm64"
    ;;
  linux-arm)
    export CC="arm-linux-gnueabihf-gcc" CXX="arm-linux-gnueabihf-g++"
    ;;
  linux-arm64)
    export CC="aarch64-linux-gnu-gcc" CXX="aarch64-linux-gnu-g++"
    ;;
esac

mkdir -p build
cd build
cmake "$SOURCE_DIR"
cmake --build .

if [ "$OS" == "windows" ]; then
  file Debug/example.exe
else
  file example
fi

if [ "$CPU" == "x86" ] || [ "$CPU" == "x64" ]; then
  ctest --output-on-failure .
fi
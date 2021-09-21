#!/usr/bin/env bash

set -ex

OS=$(uname)
case $OS in
MINGW*)
  OS="windows"
  ;;
*)
  OS=$(echo $OS | tr '[:upper:]' '[:lower:]')
  ;;
esac

export SOURCE_DIR="$PWD/example"
export PDFium_DIR="$PWD/staging"

if [ "$OS" == "windows" ]; then
  export CMAKE_GENERATOR="Visual Studio 16 2019"
  [ "$TARGET_CPU" == "x86" ] && export CMAKE_GENERATOR_PLATFORM="Win32"
  [ "$TARGET_CPU" == "x64" ] && export CMAKE_GENERATOR_PLATFORM="x64"
fi

mkdir build
cd build
cmake "$SOURCE_DIR"
cmake --build .
ctest --output-on-failure .

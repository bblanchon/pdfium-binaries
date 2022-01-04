#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
SOURCE_DIR="$PWD/example"
ANDROID_TOOLCHAIN="${PDFium_SOURCE_DIR:?}/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/"
CMAKE_ARGS=()

export PDFium_DIR="$PWD/staging"

case "$OS" in
  android)
    export PATH="$ANDROID_TOOLCHAIN:$PATH"
    case "$CPU" in
      arm)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="armv7a-linux-androideabi16-clang"
          -D CMAKE_CXX_COMPILER="armv7a-linux-androideabi16-clang++"
        )
        ;;
      arm64)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="aarch64-linux-android21-clang"
          -D CMAKE_CXX_COMPILER="aarch64-linux-android21-clang++"
        )
        ;;
      x64)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="x86_64-linux-android21-clang"
          -D CMAKE_CXX_COMPILER="x86_64-linux-android21-clang++"
        )
        ;;
      x86)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="i686-linux-android16-clang"
          -D CMAKE_CXX_COMPILER="i686-linux-android16-clang++"
        )
        ;;
    esac
    ;;

  ios)
    CMAKE_ARGS+=(
      -D CMAKE_SYSTEM_NAME="iOS"
      # https://discourse.cmake.org/t/find-package-stops-working-when-cmake-system-name-ios/4609/7
      -D CMAKE_FIND_ROOT_PATH_MODE_PACKAGE="BOTH"
      -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE="BOTH"
      -D CMAKE_FIND_ROOT_PATH_MODE_LIBRARY="BOTH"
    )
    case "$CPU" in
      arm64)
        CMAKE_ARGS+=(
          -D CMAKE_OSX_SYSROOT="iphoneos"
          -D CMAKE_OSX_ARCHITECTURES="arm64"
        )
        ;;
      x64)
        CMAKE_ARGS+=(
          -D CMAKE_OSX_SYSROOT="iphonesimulator"
          -D CMAKE_OSX_ARCHITECTURES="x86_64"
        )
        ;;
    esac
    ;;

  linux)
    case "$CPU" in
      arm)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="arm-linux-gnueabihf-gcc"
          -D CMAKE_CXX_COMPILER="arm-linux-gnueabihf-g++"
        )
        ;;
      arm64)
        CMAKE_ARGS+=(
          -D CMAKE_C_COMPILER="aarch64-linux-gnu-gcc"
          -D CMAKE_CXX_COMPILER="aarch64-linux-gnu-g++"
        )
        ;;
    esac
    ;;

  mac)
    case "$CPU" in
      arm64)
        CMAKE_ARGS+=(
          -D CMAKE_OSX_ARCHITECTURES="arm64"
        )
      ;;
    esac
    ;;

  win)
    CMAKE_ARGS+=(
      -G "Visual Studio 16 2019"
    )
    case "$CPU" in
      arm64)
        CMAKE_ARGS+=(
          -A ARM64
        )
        ;;
      x86)
        CMAKE_ARGS+=(
          -A Win32
        )
        ;;
      x64)
        CMAKE_ARGS+=(
          -A x64
        )
        ;;
    esac
    ;;
esac

CMAKE_ARGS+=("$SOURCE_DIR")

mkdir -p build
cd build
cmake "${CMAKE_ARGS[@]}"
cmake --build .

if [ "$OS" == "win" ]; then
  file Debug/example.exe
else
  file example
fi

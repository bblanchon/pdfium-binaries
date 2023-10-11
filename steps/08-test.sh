#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
TARGET_LIBC="${PDFium_TARGET_LIBC:-default}"
SOURCE_DIR="$PWD/example"
CMAKE_ARGS=()
CAN_RUN_ON_HOST=false

export PDFium_DIR="$PWD/staging"

case "$OS" in
  android)
    case "$CPU" in
      arm)
        PREFIX="armv7a-linux-androideabi19-"
        ;;
      arm64)
        PREFIX="aarch64-linux-android21-"
        ;;
      x64)
        PREFIX="x86_64-linux-android21-"
        ;;
      x86)
        PREFIX="i686-linux-android19-"
        ;;
    esac
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="${PREFIX:-}clang"
      -D CMAKE_CXX_COMPILER="${PREFIX:-}clang++"
    )
    ;;

  ios)
    case "$CPU" in
      arm64)
        ARCH="arm64"
        SDK="iphoneos"
        ;;
      x64)
        ARCH="x86_64"
        SDK="iphonesimulator"
        ;;
    esac
    CMAKE_ARGS+=(
      -D CMAKE_SYSTEM_NAME="iOS"
      -D CMAKE_OSX_SYSROOT="$SDK"
      -D CMAKE_OSX_ARCHITECTURES="$ARCH"
      # https://discourse.cmake.org/t/find-package-stops-working-when-cmake-system-name-ios/4609/7
      -D CMAKE_FIND_ROOT_PATH_MODE_PACKAGE="BOTH"
      -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE="BOTH"
      -D CMAKE_FIND_ROOT_PATH_MODE_LIBRARY="BOTH"
    )
    ;;

  linux)
    case "$CPU" in
      arm)
        if [ "$TARGET_LIBC" == "musl" ]; then
          PREFIX="arm-linux-musleabihf-"
        else
          PREFIX="arm-linux-gnueabihf-"
          SUFFIX="-10"
        fi
        ;;
      arm64)
        if [ "$TARGET_LIBC" == "musl" ]; then
          PREFIX="aarch64-linux-musl-"
        else
          PREFIX="aarch64-linux-gnu-"
          SUFFIX="-10"
        fi
        ;;
      x86)
        if [ "$TARGET_LIBC" == "musl" ]; then
          PREFIX="i686-linux-musl-"
        else
          CAN_RUN_ON_HOST=true
        fi
        CMAKE_ARGS+=(
          -D CMAKE_CXX_FLAGS="-m32"
          -D CMAKE_C_FLAGS="-m32"
        )
        ;;
      x64)
        if [ "$TARGET_LIBC" == "musl" ]; then
          PREFIX="x86_64-linux-musl-"
        else
          CAN_RUN_ON_HOST=true
        fi
        ;;
    esac
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="${PREFIX:-}gcc${SUFFIX:-}"
      -D CMAKE_CXX_COMPILER="${PREFIX:-}g++${SUFFIX:-}"
    )
    ;;

  mac)
    case "$CPU" in
      arm64)
        ARCH="arm64"
        ;;
      x64)
        ARCH="x86_64"
        CAN_RUN_ON_HOST=true
        ;;
    esac
    CMAKE_ARGS+=(
      -D CMAKE_OSX_ARCHITECTURES="$ARCH"
    )
    ;;

  win)
    case "$CPU" in
      arm64)
        ARCH="ARM64"
        ;;
      x86)
        ARCH="Win32"
        CAN_RUN_ON_HOST=true
        ;;
      x64)
        ARCH="x64"
        CAN_RUN_ON_HOST=true
        ;;
    esac
    CMAKE_ARGS+=(
      -G "Visual Studio 17 2022"
      -A "$ARCH"
    )
    ;;

  wasm)
    # TODO: add test for Wasm
    exit
    ;;
esac

CMAKE_ARGS+=("$SOURCE_DIR")

mkdir -p build
pushd build

cmake "${CMAKE_ARGS[@]}"
cmake --build .

if [ "$OS" == "win" ]; then
  EXAMPLE="Debug/example.exe"
else
  EXAMPLE="./example"
fi

file $EXAMPLE

if [ $CAN_RUN_ON_HOST == "true" ]; then
  $EXAMPLE "${PDFium_SOURCE_DIR}/testing/resources/hello_world.pdf" hello_world.ppm
fi

popd

#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
TARGET_LIBC="${PDFium_TARGET_LIBC:-default}"
SOURCE_DIR="$PWD/example"
ANDROID_TOOLCHAIN="${PDFium_SOURCE_DIR:?}/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/"
CMAKE_ARGS=()

export PDFium_DIR="$PWD/staging"

case "$OS" in
  android)
    case "$CPU" in
      arm)
        PREFIX="armv7a-linux-androideabi16-"
        ;;
      arm64)
        PREFIX="aarch64-linux-android21-"
        ;;
      x64)
        PREFIX="x86_64-linux-android21-"
        ;;
      x86)
        PREFIX="i686-linux-android16-"
        ;;
    esac
    export PATH="$ANDROID_TOOLCHAIN:$PATH"
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
        PREFIX="arm-linux-gnueabihf-"
        SUFFIX="-9"
        ;;
      arm64)
        PREFIX="aarch64-linux-gnu-"
        SUFFIX="-9"
        ;;
      x86)
        [ "$TARGET_LIBC" == "musl" ] && PREFIX="i686-linux-musl-"
        CMAKE_ARGS+=(
          -D CMAKE_CXX_FLAGS="-m32"
          -D CMAKE_C_FLAGS="-m32"
        )
        ;;
      x64)
        [ "$TARGET_LIBC" == "musl" ] && PREFIX="x86_64-linux-musl-"
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
        ;;
      x64)
        ARCH="x64"
        ;;
    esac
    CMAKE_ARGS+=(
      -G "Visual Studio 16 2019"
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
  file Debug/example.exe
else
  file example
fi

popd

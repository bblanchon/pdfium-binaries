#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
SOURCE_DIR="$PWD/example"
ANDROID_TOOLCHAIN="${PDFium_SOURCE_DIR:?}/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/"
CMAKE_ARGS=()

export PDFium_DIR="$PWD/staging"

[ "$OS" == "android" ] && export PATH="$ANDROID_TOOLCHAIN:$PATH"

case "$OS-$CPU" in
  android-arm)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="armv7a-linux-androideabi16-clang"
      -D CMAKE_CXX_COMPILER="armv7a-linux-androideabi16-clang++"
    )
    ;;
  android-arm64)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="aarch64-linux-android21-clang"
      -D CMAKE_CXX_COMPILER="aarch64-linux-android21-clang++"
    )
    ;;
  android-x64)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="x86_64-linux-android21-clang"
      -D CMAKE_CXX_COMPILER="x86_64-linux-android21-clang++"
    )
    ;;
  android-x86)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="i686-linux-android16-clang"
      -D CMAKE_CXX_COMPILER="i686-linux-android16-clang++"
    )
    ;;
  ios-arm64)
    # WARNING: doesn't work, see following page
    # https://discourse.cmake.org/t/find-package-stops-working-when-cmake-system-name-ios/4609
    CMAKE_ARGS+=(
      -D CMAKE_SYSTEM_NAME="iOS"
      -D CMAKE_OSX_ARCHITECTURES="arm64"
    )
    ;;
  ios-x64)
    # WARNING: doesn't work, see following page
    # https://discourse.cmake.org/t/find-package-stops-working-when-cmake-system-name-ios/4609
    CMAKE_ARGS+=(
      -D CMAKE_SYSTEM_NAME="iOS"
      -D CMAKE_OSX_ARCHITECTURES="x86_64"
    )
    ;;
  linux-arm)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="arm-linux-gnueabihf-gcc"
      -D CMAKE_CXX_COMPILER="arm-linux-gnueabihf-g++"
    )
    ;;
  linux-arm64)
    CMAKE_ARGS+=(
      -D CMAKE_C_COMPILER="aarch64-linux-gnu-gcc"
      -D CMAKE_CXX_COMPILER="aarch64-linux-gnu-g++"
    )
    ;;
  mac-arm64)
    CMAKE_ARGS+=(
      -D CMAKE_OSX_ARCHITECTURES="arm64"
    )
    ;;
  win-x86)
    CMAKE_ARGS+=(
      -G "Visual Studio 16 2019"
      -A Win32
    )
    ;;
  win-x64)
    CMAKE_ARGS+=(
      -G "Visual Studio 16 2019"
      -A x64
    )
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

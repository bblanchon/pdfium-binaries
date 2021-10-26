#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
CPU="${PDFium_TARGET_CPU:?}"
PDFIUM="${PDFium_SOURCE_DIR:?}"
SOURCE_DIR="$PWD/example"

export PDFium_DIR="$PWD/staging"
[ "$OS" == "win" ] && export CMAKE_GENERATOR="Visual Studio 16 2019"

case "$OS-$CPU" in
  win-x86)    
    export CMAKE_GENERATOR_PLATFORM="Win32"
    ;;
  win-x64)
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
  android-arm)
    export PATH="$PDFIUM/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/:$PATH"
    export CC="armv7a-linux-androideabi16-clang" CXX="armv7a-linux-androideabi16-clang++"
    ;;
  android-arm64)
    export PATH="$PDFIUM/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/:$PATH"
    export CC="aarch64-linux-android21-clang" CXX="aarch64-linux-android21-clang++"
    ;;
  android-x86)
    export PATH="$PDFIUM/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/:$PATH"
    export CC="i686-linux-android16-clang" CXX="i686-linux-android16-clang++"
    ;;
  android-x64)
    export PATH="$PDFIUM/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/:$PATH"
    export CC="x86_64-linux-android21-clang" CXX="x86_64-linux-android21-clang++"
    ;;
esac

mkdir -p build
cd build
cmake "$SOURCE_DIR"
cmake --build .

if [ "$OS" == "win" ]; then
  file Debug/example.exe
else
  file example
fi

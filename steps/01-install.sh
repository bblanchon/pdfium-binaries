#!/bin/bash -eux

PATH_FILE=${GITHUB_PATH:-$PWD/.path}
TARGET_OS=${PDFium_TARGET_OS:?}
TARGET_ENVIRONMENT=${PDFium_TARGET_ENVIRONMENT:-}
TARGET_CPU=${PDFium_TARGET_CPU:?}
CURRENT_CPU=${PDFium_CURRENT_CPU:-x64}
MUSL_URL=${MUSL_URL:-https://musl.cc}
ENABLE_V8=${PDFium_ENABLE_V8:-false}

DepotTools_URL='https://chromium.googlesource.com/chromium/tools/depot_tools.git'
DepotTools_DIR="$PWD/depot_tools"
WindowsSDK_DIR="/c/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0"

# Download depot_tools if not exists in this location
if [ ! -d "$DepotTools_DIR" ]; then
  git clone "$DepotTools_URL" "$DepotTools_DIR"
fi

echo "$DepotTools_DIR" >> "$PATH_FILE"

case "$TARGET_OS" in
  android)
    sudo apt-get update
    sudo apt-get install -y unzip

    # pdfium installs its version of the NDK, but we need one for compiling the example
    ANDROID_NDK_VERSION="r25c"
    ANDROID_NDK_FOLDER="android-ndk-$ANDROID_NDK_VERSION"
    ANDROID_NDK_ZIP="android-ndk-$ANDROID_NDK_VERSION-linux.zip"
    if [ ! -d "$ANDROID_NDK_FOLDER" ];
    then
      [ -f "$ANDROID_NDK_ZIP" ] || curl -Os "https://dl.google.com/android/repository/$ANDROID_NDK_ZIP"
      unzip -o -q "$ANDROID_NDK_ZIP"
      rm -f "$ANDROID_NDK_ZIP"
    fi
    echo "$PWD/$ANDROID_NDK_FOLDER/toolchains/llvm/prebuilt/linux-x86_64/bin" >> "$PATH_FILE"
    ;;

  linux)
    sudo apt-get update
    sudo apt-get install -y cmake pkg-config

    if [ "$TARGET_ENVIRONMENT" == "musl" ]; then

      case "$TARGET_CPU" in
        x86)
          MUSL_VERSION="i686-linux-musl-cross"
          PACKAGES="g++ g++-multilib"
          ;;

        x64)
          MUSL_VERSION="x86_64-linux-musl-cross"
          PACKAGES="g++"
          ;;

        arm)
          MUSL_VERSION="arm-linux-musleabihf-cross"
          PACKAGES="g++"
          ;;

        arm64)
          MUSL_VERSION="aarch64-linux-musl-cross"
          PACKAGES="g++"
          ;;
      esac

      [ -d "$MUSL_VERSION" ] || curl -L "$MUSL_URL/$MUSL_VERSION.tgz" | tar xz
      echo "$PWD/$MUSL_VERSION/bin" >> "$PATH_FILE"

      sudo apt-get install -y $PACKAGES

    else

      case "$TARGET_CPU" in
        arm)
          sudo apt-get install -y libc6-i386 gcc-10-multilib g++-10-arm-linux-gnueabihf gcc-10-arm-linux-gnueabihf
          ;;

        arm64)
          sudo apt-get install -y libc6-i386 gcc-10-multilib g++-10-aarch64-linux-gnu gcc-10-aarch64-linux-gnu
          ;;

        x86)
          sudo apt-get install -y g++-multilib
          ;;

        x64)
          sudo apt-get install -y g++
          ;;
      esac

    fi
    ;;

  win)
    echo "$WindowsSDK_DIR/$CURRENT_CPU" >> "$PATH_FILE"
    ;;

  ios)
    # Xcode 15.4 produces the following error when targeting ARM64 with V8:
    # undefined symbol: be_memory_inline_jit_restrict_rwx_to_rx_with_witness_impl
    sudo xcode-select -s "/Applications/Xcode_15.0.1.app"
    ;;

  emscripten)
    if [ "$ENABLE_V8" == "true" ]; then
      sudo apt-get update
      # We need to install the snapshot toolchain for x86
      sudo apt-get install -y g++-multilib
    fi
esac

#!/bin/bash -eux

PATH_FILE=${GITHUB_PATH:-$PWD/.path}
TARGET_OS=${PDFium_TARGET_OS:?}
TARGET_LIBC=${PDFium_TARGET_LIBC:-default}
TARGET_CPU=${PDFium_TARGET_CPU:?}
CURRENT_CPU=${PDFium_CURRENT_CPU:-x64}
MUSL_URL=${MUSL_URL:-https://musl.cc}

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

    if [ "$TARGET_LIBC" == "musl" ]; then

      case "$TARGET_CPU" in
        x86)
          MUSL_VERSION="i686-linux-musl-cross"
          PACKAGES="g++-10 g++-10-multilib"
          ;;

        x64)
          MUSL_VERSION="x86_64-linux-musl-cross"
          PACKAGES="g++-10"
          ;;

        arm)
          MUSL_VERSION="arm-linux-musleabihf-cross"
          PACKAGES="g++-10"
          ;;

        arm64)
          MUSL_VERSION="aarch64-linux-musl-cross"
          PACKAGES="g++-10"
          ;;
      esac

      [ -d "$MUSL_VERSION" ] || curl -L "$MUSL_URL/$MUSL_VERSION.tgz" | tar xz
      echo "$PWD/$MUSL_VERSION/bin" >> "$PATH_FILE"

      sudo apt-get install -y $PACKAGES
      sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
      sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10

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

  wasm)
    if [ -e "emsdk" ]; then
      git -C "emsdk" pull
    else
      git clone https://github.com/emscripten-core/emsdk.git
    fi
    pushd emsdk
    ./emsdk install ${EMSDK_VERSION:-3.1.34}
    ./emsdk activate ${EMSDK_VERSION:-3.1.34}
    echo "$PWD/upstream/emscripten" >> "$PATH_FILE"
    echo "$PWD/upstream/bin" >> "$PATH_FILE"
    popd
    ;;

  win)
    echo "$WindowsSDK_DIR/$CURRENT_CPU" >> "$PATH_FILE"
    ;;

  ios)
    # Xcode 15.4 produces the following error when targeting ARM64 with V8:
    # undefined symbol: be_memory_inline_jit_restrict_rwx_to_rx_with_witness_impl
    sudo xcode-select -s "/Applications/Xcode_15.0.1.app"
    ;;
esac

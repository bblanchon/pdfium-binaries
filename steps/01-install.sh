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
  linux)
    sudo apt-get update
    sudo apt-get install -y cmake pkg-config

    if [ "$TARGET_LIBC" == "musl" ]; then

      case "$TARGET_CPU" in
        x86)
          MUSL_VERSION="i686-linux-musl-cross"
          ;;

        x64)
          MUSL_VERSION="x86_64-linux-musl-cross"
          ;;
      esac

      [ -d "$MUSL_VERSION" ] || curl -L "$MUSL_URL/$MUSL_VERSION.tgz" | tar xz
      echo "$PWD/$MUSL_VERSION/bin" >> "$PATH_FILE"

      sudo apt install -y g++-10
      sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
      sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10

    else

      case "$TARGET_CPU" in
        arm)
          sudo apt-get install -y libc6-i386 gcc-9-multilib g++-9-arm-linux-gnueabihf gcc-9-arm-linux-gnueabihf
          ;;

        arm64)
          sudo apt-get install -y libc6-i386 gcc-9-multilib g++-9-aarch64-linux-gnu gcc-9-aarch64-linux-gnu
          ;;

        x86)
          sudo apt-get install -y g++-multilib
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
    ./emsdk install 2.0.24
    ./emsdk activate 2.0.24
    echo "$PWD/upstream/emscripten" >> "$PATH_FILE"
    echo "$PWD/upstream/bin" >> "$PATH_FILE"
    popd
    ;;

  win)
    echo "$WindowsSDK_DIR/$CURRENT_CPU" >> "$PATH_FILE"
    ;;
esac
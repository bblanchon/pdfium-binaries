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

case "$TARGET_OS-$TARGET_LIBC-$TARGET_CPU" in
  win-*)
    echo "$WindowsSDK_DIR/$CURRENT_CPU" >> "$PATH_FILE"
    ;;

  linux-default-arm)
    sudo apt-get update
    sudo apt-get install -y g++-arm-linux-gnueabihf
    ;;

  linux-default-arm64)
    sudo apt-get update
    sudo apt-get install -y g++-aarch64-linux-gnu
    ;;

  linux-default-x86)
    sudo apt-get update
    sudo apt-get install -y g++-multilib
    ;;

  linux-musl-x86)
    curl -L "$MUSL_URL/i686-linux-musl-cross.tgz" | tar xz
    echo "$PWD/i686-linux-musl-cross/bin" >> "$PATH_FILE"
    ;;

  linux-musl-x64)
    curl -L "$MUSL_URL/x86_64-linux-musl-cross.tgz" | tar xz
    echo "$PWD/x86_64-linux-musl-cross/bin" >> "$PATH_FILE"
    ;;

  wasm-*)
    if [ -e "emsdk" ]; then
      git -C "emsdk" pull
    else
      git clone https://github.com/emscripten-core/emsdk.git
    fi
    pushd emsdk
    ./emsdk install 3.1.24
    ./emsdk activate 3.1.24
    echo "$PWD/upstream/emscripten" >> "$PATH_FILE"
    echo "$PWD/upstream/bin" >> "$PATH_FILE"
    popd
    ;;

esac

if [ "$TARGET_LIBC" == "musl" ]; then
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10
fi

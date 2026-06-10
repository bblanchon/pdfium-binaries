#!/bin/bash -eux

PATH_FILE=${GITHUB_PATH:-$PWD/.path}
ROOT=$PWD
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
TARGET_OS=${PDFium_TARGET_OS:?}
TARGET_ENVIRONMENT=${PDFium_TARGET_ENVIRONMENT:-}
TARGET_CPU=${PDFium_TARGET_CPU:?}
PATCHES="$PWD/patches"

pushd "$SOURCE"

case "$TARGET_OS" in
  linux)
    build/install-build-deps.sh
    gclient runhooks
    build/linux/sysroot_scripts/install-sysroot.py "--arch=$TARGET_CPU"

    if [ "$TARGET_ENVIRONMENT" == "musl" ]; then
      case "$TARGET_CPU" in
        x86)
          MUSL_VERSION="i686-linux-musl-cross"
          ;;
        x64)
          MUSL_VERSION="x86_64-linux-musl-cross"
          ;;
        arm)
          MUSL_VERSION="arm-linux-musleabihf-cross"
          ;;
        arm64)
          MUSL_VERSION="aarch64-linux-musl-cross"
          ;;
      esac
      ln -sf "$ROOT/$MUSL_VERSION" "third_party/$MUSL_VERSION"
    fi
    ;;

  android)
    build/install-build-deps.sh --android
    gclient runhooks
    ;;

  emscripten)
    pushd third_party
    if [ -e "emsdk" ]; then
      git -C "emsdk" pull
    else
      git clone https://github.com/emscripten-core/emsdk.git
    fi
    cd emsdk
    ./emsdk install ${EMSDK_VERSION:-5.0.5}
    ./emsdk activate ${EMSDK_VERSION:-5.0.5}
    echo "$PWD/upstream/emscripten" >> "$PATH_FILE"
    echo "$PWD/upstream/bin" >> "$PATH_FILE"

    if [ "$TARGET_CPU" == "wasm-standalone" ]; then
      pushd "$PWD/upstream/emscripten"
      patch -p1 --forward < "$PATCHES/wasm/emscripten.patch" || true
      rm -Rf cache
      popd
    fi

    popd
    ;;
esac

popd

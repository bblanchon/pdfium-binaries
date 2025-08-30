#!/bin/bash -eux

PATCHES="$PWD/patches"
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
TARGET_ENVIRONMENT="${PDFium_TARGET_ENVIRONMENT:-}"
ENABLE_V8=${PDFium_ENABLE_V8:-false}

apply_patch() {
  local FILE="$1"
  local DIR="${2:-.}"
  patch --verbose -p1 -d "$DIR" -i "$FILE"
}

pushd "${SOURCE}"

[ "$OS" != "emscripten" ] && apply_patch "$PATCHES/shared_library.patch"
apply_patch "$PATCHES/public_headers.patch"

[ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/v8/pdfium.patch"

case "$OS" in
  android)
    apply_patch "$PATCHES/android/build.patch" build
    ;;

  ios)
    apply_patch "$PATCHES/ios/pdfium.patch"
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/ios/v8.patch" v8
    ;;

  linux)
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/linux/v8.patch" v8
    ;;

  emscripten)
    apply_patch "$PATCHES/wasm/pdfium.patch"
    apply_patch "$PATCHES/wasm/build.patch" build
    if [ "$ENABLE_V8" == "true" ]; then
      apply_patch "$PATCHES/wasm/skia.patch"
      apply_patch "$PATCHES/wasm/v8.patch" v8
    fi
    mkdir -p "build/config/wasm"
    cp "$PATCHES/wasm/config.gn" "build/config/wasm/BUILD.gn"
    ;;

  win)
    apply_patch "$PATCHES/win/build.patch" build

    VERSION=${PDFium_VERSION:-0.0.0.0}
    YEAR=$(date +%Y)
    VERSION_CSV=${VERSION//./,}
    export YEAR VERSION VERSION_CSV
    envsubst < "$PATCHES/win/resources.rc" > "resources.rc"
    ;;
esac

case "$TARGET_ENVIRONMENT" in
  musl)
    apply_patch "$PATCHES/musl/pdfium.patch"
    apply_patch "$PATCHES/musl/build.patch" build
    mkdir -p "build/toolchain/linux/musl"
    cp "$PATCHES/musl/toolchain.gn" "build/toolchain/linux/musl/BUILD.gn"
    ;;
esac

popd

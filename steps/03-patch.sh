#!/bin/bash -eux

PATCHES="$PWD/patches"
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
TARGET_LIBC="${PDFium_TARGET_LIBC:-default}"

pushd "${SOURCE}"

[ "$OS" != "wasm" ] && git apply -v "$PATCHES/shared_library.patch"
git apply -v "$PATCHES/public_headers.patch"

[ "${PDFium_ENABLE_V8:-}" == "true" ] && git apply -v "$PATCHES/v8_init.patch"

case "$OS" in
  android)
    git -C build apply -v "$PATCHES/android/build.patch"
    ;;

  ios)
    git apply -v "$PATCHES/ios/pdfium.patch"
    git -C build apply -v "$PATCHES/ios/build.patch"
    git -C third_party/libjpeg_turbo apply -v "$PATCHES/ios/libjpeg_turbo.patch"
    ;;

  wasm)
    git apply -v "$PATCHES/wasm/pdfium.patch"
    git -C build apply -v "$PATCHES/wasm/build.patch"
    git -C base/allocator/partition_allocator apply -v "$PATCHES/wasm/partition_allocator.patch"
    mkdir -p "build/toolchain/wasm"
    cp "$PATCHES/wasm/toolchain.gn" "build/toolchain/wasm/BUILD.gn"
    mkdir -p "build/config/wasm"
    cp "$PATCHES/wasm/config.gn" "build/config/wasm/BUILD.gn"
    ;;

  win)
    git apply -v "$PATCHES/win/pdfium.patch"
    git -C build apply -v "$PATCHES/win/build.patch"

    VERSION=${PDFium_VERSION:-0.0.0.0}
    YEAR=$(date +%Y)
    VERSION_CSV=${VERSION//./,}
    export YEAR VERSION VERSION_CSV
    envsubst < "$PATCHES/win/resources.rc" > "resources.rc"
    ;;
esac

case "$TARGET_LIBC" in
  musl)
    git -C build apply -v "$PATCHES/musl/build.patch"
    mkdir -p "build/toolchain/linux/musl"
    cp "$PATCHES/musl/toolchain.gn" "build/toolchain/linux/musl/BUILD.gn"
    ;;
esac

"$PATCHES/aggregate_licenses.sh"

popd

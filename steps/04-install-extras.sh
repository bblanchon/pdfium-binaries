#!/bin/bash -eux

SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
CPU="${PDFium_TARGET_CPU:?}"

pushd "$SOURCE"

case "$OS" in
  linux)
    build/linux/sysroot_scripts/install-sysroot.py "--arch=$CPU"
    build/install-build-deps.sh
    gclient runhooks
    ;;

  android)
    build/install-build-deps.sh --android
    gclient runhooks
    ;;
esac

popd

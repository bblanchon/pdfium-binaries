#!/bin/bash -eux

SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
CPU="${PDFium_TARGET_CPU:?}"

pushd "$SOURCE"

case "$OS" in
  linux)
    build/linux/sysroot_scripts/install-sysroot.py "--arch=$CPU"
    ;;
    
  android)
    sudo apt-get update
    sudo apt-get remove -y php7.4-common
    build/install-build-deps-android.sh
    gclient runhooks
    ;;
esac

popd

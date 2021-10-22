#!/bin/bash -eux

SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
CPU="${PDFium_TARGET_CPU:?}"

pushd "$SOURCE"

if [ "$OS" == "linux" ]; then
  build/linux/sysroot_scripts/install-sysroot.py "--arch=$CPU"
fi

popd
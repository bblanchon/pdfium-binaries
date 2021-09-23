#!/bin/bash -eux

SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_OS:?}"
CPU="${PDFium_TARGET_CPU:?}"

pushd "$SOURCE"

# Install additional images if needed
if [ "$CPU" == "arm" ] && [ "$OS" == "linux" ]; then
  build/linux/sysroot_scripts/install-sysroot.py --arch=arm
fi

popd
#!/bin/bash -eux

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}
IS_DEBUG=${PDFium_IS_DEBUG:-false}

if [ "$TARGET_CPU" == "wasm" ]; then
  ninja -C "$BUILD_DIR" pdfium_wasm
else
  ninja -C "$BUILD_DIR" pdfium
fi

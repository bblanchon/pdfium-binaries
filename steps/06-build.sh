#!/bin/bash -eux

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}

ninja -C "$BUILD_DIR" pdfium

if [ "$TARGET_CPU" == "wasm" ]; then
  LIBPDFIUMA="$BUILD_DIR/obj/libpdfium.a"
  EXPORTED_FUNCTIONS=$(llvm-nm $LIBPDFIUMA --format=just-symbols | grep "^FPDF" | sed 's/^/_/' | paste -sd "," -)
  em++ \
    -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" \
    -s LLD_REPORT_UNDEFINED \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -o "$BUILD_DIR/pdfium.html" \
    "$LIBPDFIUMA" \
    --no-entry
fi
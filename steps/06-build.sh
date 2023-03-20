#!/bin/bash -eux

IS_DEBUG=${PDFium_IS_DEBUG:-false}
SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}

ninja -C "$BUILD_DIR" pdfium

if [ "$TARGET_CPU" == "wasm" ]; then
  LIBPDFIUMA="$BUILD_DIR/obj/libpdfium.a"
  EXPORTED_FUNCTIONS=$(llvm-nm $LIBPDFIUMA --format=just-symbols | grep "^FPDF\|^FSDK\|^FORM\|^IFSDK" | sed 's/^/_/' | paste -sd "," -)

  if [[ "$IS_DEBUG" == "true" ]]; then
    em++ \
      -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" \
      -s LLD_REPORT_UNDEFINED \
      -s WASM=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s STANDALONE_WASM=1 \
      --profile \
      -g \
      -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
      -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
      -o "$BUILD_DIR/pdfium.html" \
      "$LIBPDFIUMA" \
      --no-entry
  else
    # O3 does not work! Strips out too much!
    em++ \
      -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" \
      -s LLD_REPORT_UNDEFINED \
      -s WASM=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s STANDALONE_WASM=1 \
      -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
      -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
      -O2 \
      -o "$BUILD_DIR/pdfium.html" \
      "$LIBPDFIUMA" \
      --no-entry
  fi
fi

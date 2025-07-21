#!/bin/bash -eux

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}
IS_DEBUG=${PDFium_IS_DEBUG:-false}

ninja -C "$BUILD_DIR" pdfium

if [ "$TARGET_CPU" == "wasm" ]; then
  LIBPDFIUMA="$BUILD_DIR/obj/libpdfium.a"
  EXPORTED_FUNCTIONS=$(llvm-nm $LIBPDFIUMA --format=just-symbols | grep "^FPDF\|^FSDK\|^FORM\|^IFSDK" | sed 's/^/_/' | paste -sd "," -)
  EMCC_ARGS=(
    -s ALLOW_MEMORY_GROWTH=1
    -s ALLOW_TABLE_GROWTH=1
    -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS,_free,_malloc,_calloc,_realloc"
    -s EXPORTED_RUNTIME_METHODS="ccall,cwrap,addFunction,removeFunction"
    -s LLD_REPORT_UNDEFINED
    -s WASM=1
    -o "$BUILD_DIR/pdfium.html"
    "$LIBPDFIUMA"
    --no-entry
  )
  if [[ "$IS_DEBUG" == "true" ]]; then
    EMCC_ARGS+=(
      --profile
      -g
    )
  else
    # O3 does not work! Strips out too much!
    EMCC_ARGS+=(
      -O2
    )
  fi
  em++ "${EMCC_ARGS[@]}"
fi

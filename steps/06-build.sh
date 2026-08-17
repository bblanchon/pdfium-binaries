#!/bin/bash -eux

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}
IS_DEBUG=${PDFium_IS_DEBUG:-false}

ninja -C "$BUILD_DIR" pdfium

if [[ "$TARGET_CPU" == "wasm" || "$TARGET_CPU" == "wasm-standalone" ]]; then
  LIBPDFIUMA="$BUILD_DIR/obj/libpdfium.a"
  EXPORTED_FUNCTIONS=$("$SOURCE/third_party/emsdk/upstream/bin/llvm-nm" $LIBPDFIUMA --format=just-symbols | grep "^FPDF\|^FSDK\|^FORM\|^IFSDK" | sed 's/^/_/' | paste -sd "," -)
  EMCC_ARGS=(
    -s ALLOW_MEMORY_GROWTH=1
    -s ALLOW_TABLE_GROWTH=1
    -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS,_free,_malloc,_calloc,_realloc,_memset"
    -s EXPORTED_RUNTIME_METHODS="ccall,cwrap,addFunction,removeFunction"
    -s LLD_REPORT_UNDEFINED
    -s WASM=1
    -o "$BUILD_DIR/pdfium.html"
    "$LIBPDFIUMA"
    --no-entry
  )
  if [[ "$TARGET_CPU" == "wasm-standalone" ]]; then
    # Link a strong memset that uses the memory.fill instruction (libc's
    # memset is a weak alias, so this overrides it). Emscripten only ships
    # this in -Oz libc variants; see patches/wasm/memset_shim.c.
    EMCC="$SOURCE/third_party/emsdk/upstream/emscripten/emcc"
    BULKMEM_S="$SOURCE/third_party/emsdk/upstream/emscripten/system/lib/libc/emscripten_memset_bulkmem.S"
    "$EMCC" -O2 -mbulk-memory -fno-builtin-memset -c patches/wasm/memset_shim.c -o "$BUILD_DIR/memset_shim.o"
    "$EMCC" -mbulk-memory -c "$BULKMEM_S" -o "$BUILD_DIR/memset_bulkmem.o"
    EMCC_ARGS+=(
      -mbulk-memory
      -s ERROR_ON_UNDEFINED_SYMBOLS=0
      -s STANDALONE_WASM=1
      "$BUILD_DIR/memset_shim.o"
      "$BUILD_DIR/memset_bulkmem.o"
    )
  fi

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

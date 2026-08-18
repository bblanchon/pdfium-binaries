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
    -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS,_free,_malloc,_calloc,_realloc,_memset,_gopdfium_jpeg_encode,_gopdfium_jpeg_free"
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
    # JPEG encode shim: exposes libjpeg-turbo's compressor (otherwise
    # dead-stripped, since PDFium only decodes). Needs the longjmp lowering
    # at compile time for its setjmp error handler.
    "$EMCC" -O2 -mbulk-memory -msimd128 -sSUPPORT_LONGJMP=wasm -DMANGLE_JPEG_NAMES \
      -I "$SOURCE/third_party/libjpeg_turbo" -I "$SOURCE/third_party/libjpeg_turbo/src" \
      -c patches/wasm/jpeg_encode_shim.c -o "$BUILD_DIR/jpeg_encode_shim.o"
    "$EMCC" -mbulk-memory -c "$BULKMEM_S" -o "$BUILD_DIR/memset_bulkmem.o"
    EMCC_ARGS+=(
      -mbulk-memory
      -msimd128
      -sSUPPORT_LONGJMP=wasm
      -s ERROR_ON_UNDEFINED_SYMBOLS=0
      -s STANDALONE_WASM=1
      "$BUILD_DIR/memset_shim.o"
      "$BUILD_DIR/jpeg_encode_shim.o"
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
  # Keep the wasm name section (real function names) for profilers, e.g.
  # wazero's perfmap support (build tag "perfmap"). Adds size, so opt-in.
  if [[ "${PDFium_PROFILING_NAMES:-false}" == "true" ]]; then
    EMCC_ARGS+=(
      --profiling-funcs
    )
  fi
  em++ "${EMCC_ARGS[@]}"

  if [[ "$TARGET_CPU" == "wasm-standalone" ]]; then
    # Translate the LLVM legacy exception handling encoding to the
    # standardized exnref encoding (required by e.g. wazero). The --enable
    # flags only gate binaryen's validator (emscripten strips the
    # target_features section, so they must be passed explicitly); they do
    # not change the output. This is the minimal set the current module
    # needs; if a future emscripten emits more features, wasm-opt fails
    # loudly and the missing --enable flag can be added.
    WASM_OPT_ARGS=(--translate-to-exnref)
    if [[ "${PDFium_PROFILING_NAMES:-false}" == "true" ]]; then
      # Preserve the name section through the translation.
      WASM_OPT_ARGS+=(-g)
    fi
    "$SOURCE/third_party/emsdk/upstream/bin/wasm-opt" "${WASM_OPT_ARGS[@]}" \
      --enable-exception-handling --enable-reference-types \
      --enable-bulk-memory --enable-nontrapping-float-to-int --enable-simd \
      "$BUILD_DIR/pdfium.wasm" -o "$BUILD_DIR/pdfium.wasm"
  fi
fi

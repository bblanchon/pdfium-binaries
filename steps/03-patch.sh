#!/bin/bash -eux

PATCHES="$PWD/patches"
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
TARGET_CPU="${PDFium_TARGET_CPU:?}"
TARGET_ENVIRONMENT="${PDFium_TARGET_ENVIRONMENT:-}"
ENABLE_V8=${PDFium_ENABLE_V8:-false}
BUILD_TYPE=${PDFium_BUILD_TYPE:-shared}

apply_patch() {
  local FILE="$1"
  local DIR="${2:-.}"
  patch --verbose -p1 -d "$DIR" -i "$FILE"
}

pushd "${SOURCE}"

case "$BUILD_TYPE" in
  shared)
    [ "$OS" != "emscripten" ] && apply_patch "$PATCHES/shared_library.patch"
    ;;
  static)
    apply_patch "$PATCHES/static_library.patch"
    ;;
esac

apply_patch "$PATCHES/public_headers.patch"
# Under upstream review (CL 155510). Uses spans and fxcrt::Zip() rather than
# raw pointers, so it carries no UNSAFE_BUFFERS; drop it once the CL lands.
# CL 155530 (compositor) landed upstream, so its patch is gone.
apply_patch "$PATCHES/png_predictor_perf.patch"
# CL 155550's content landed upstream, so this patch now carries only the
# not-yet-uploaded run-planning follow-up (branch stretch-bilinear-rows:
# 1/2-tap column runs + tiny-work bail-out), re-derived on top of the typed
# destination spans that CL 155970 landed afterwards.
apply_patch "$PATCHES/stretch_engine_perf.patch"
[ "$OS" != "emscripten" ] && apply_patch "$PATCHES/alpha_unroll_native.patch"
# Decode 3-component JPEGs straight to BGR (branch jpeg-decode-bgr, not yet
# uploaded). Portable; helps native and wasm. The wasm jpeg_simd kernels
# already dispatch on JCS_EXT_BGR, so no wasm-side change is needed.
apply_patch "$PATCHES/jpeg_decode_bgr.patch"
apply_patch "$PATCHES/swap_translate_perf.patch"
apply_patch "$PATCHES/lcms_translate_memo.patch"
apply_patch "$PATCHES/t4_psfunc_memo.patch"
# Correctness fix, under upstream review (CL 156290, with its tests in
# CL 156270). Adobe TN #5014 requires later ToUnicode mappings to supersede
# earlier ones; PDFium kept the numerically lowest unicode, so subset fonts
# using a catch-all bfrange plus overrides extracted wrong characters. Drop
# this once the CL lands.
apply_patch "$PATCHES/tounicode_precedence.patch"
# Wasm-only. Both measure neutral on native (fillrect even regresses small
# cache-resident fills there, where libc memset switches to non-temporal
# stores), and only pay off under a runtime that neither vectorizes nor
# elides the bounds checks itself. See PERFORMANCE.md.
if [ "$OS" == "emscripten" ]; then
  apply_patch "$PATCHES/glyph_blend_perf.patch"
  apply_patch "$PATCHES/fillrect_memset_perf.patch"
  # Only take effect when compiling with -msimd128 (wasm-standalone); the
  # added code is guarded by __wasm_simd128__.
  apply_patch "$PATCHES/stretch_engine_wasm_simd.patch"
  # Swizzle kernels for the planned 1/2-tap runs (16B and 32B window
  # variants, per-run gated) and for CopyRowToOpaqueBgra (all four
  # BGR/BGRx -> BGRA/RGBA instantiations).
  apply_patch "$PATCHES/stretch_horz_wasm_simd.patch"
  apply_patch "$PATCHES/compositor_wasm_simd.patch"
apply_patch "$PATCHES/c3_compositor_wasm_simd.patch"
fi
apply_patch "$PATCHES/clang_rt.patch" build

[ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/v8/pdfium.patch"

case "$OS" in
  android)
    apply_patch "$PATCHES/android/build.patch" build
    ;;

  ios)
    apply_patch "$PATCHES/ios/pdfium.patch"
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/ios/v8.patch" v8
    ;;

  mac)
    apply_patch "$PATCHES/mac/build.patch" build
    ;;

  linux)
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/linux/v8.patch" v8
    ;;

  emscripten)
    apply_patch "$PATCHES/wasm/pdfium.patch"
    apply_patch "$PATCHES/wasm/build.patch" build
    apply_patch "$PATCHES/wasm/adler32_simd_wasm.patch"
    apply_patch "$PATCHES/wasm/inflate_chunk_wasm.patch"
    apply_patch "$PATCHES/wasm/jpeg_simd_wasm.patch"
    mkdir -p third_party/libjpeg_turbo/simd/wasm
    cp "$PATCHES/wasm/jpeg_simd/"*.c third_party/libjpeg_turbo/simd/wasm/
    if [ "$TARGET_CPU" == "wasm-standalone" ]; then
      apply_patch "$PATCHES/wasm/callbacks.patch"
    fi
    if [ "$ENABLE_V8" == "true" ]; then
      apply_patch "$PATCHES/wasm/v8.patch" v8
    fi
    mkdir -p "build/config/wasm"
    cp "$PATCHES/wasm/config.gn" "build/config/wasm/BUILD.gn"
    ;;

  win)
    apply_patch "$PATCHES/win/build.patch" build

    VERSION=${PDFium_VERSION:-0.0.0.0}
    YEAR=$(date +%Y)
    VERSION_CSV=${VERSION//./,}
    export YEAR VERSION VERSION_CSV
    envsubst < "$PATCHES/win/resources.rc" > "resources.rc"
    ;;
esac

case "$TARGET_ENVIRONMENT" in
  musl)
    apply_patch "$PATCHES/musl/pdfium.patch"
    apply_patch "$PATCHES/musl/build.patch" build
    mkdir -p "build/toolchain/linux/musl"
    cp "$PATCHES/musl/toolchain.gn" "build/toolchain/linux/musl/BUILD.gn"
    ;;
esac

case "$TARGET_CPU" in
  mipsel|mips64el)
    apply_patch "$PATCHES/mips64el/build.patch" build
    ;;
  ppc64)
    apply_patch "$PATCHES/ppc64/pdfium.patch"
    apply_patch "$PATCHES/ppc64/build.patch" build
    ;;
esac

popd

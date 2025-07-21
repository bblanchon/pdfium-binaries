#!/bin/bash -eux

SOURCE_DIR=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-pdfium/out}
TARGET_ENVIRONMENT=${PDFium_TARGET_ENVIRONMENT:-}
ENABLE_V8=${PDFium_ENABLE_V8:-false}
OUTPUT_DIR="$PWD/staging/licenses"

# Extract third-party library names and store in a variable
THIRD_PARTY_LIBRARIES=$(sed -n 's/.*third_party\/\([a-z0-9_-]*\).*/\1/p' "$BUILD_DIR/build.ninja" | sort -u)

mkdir -p "$OUTPUT_DIR"

sed 's|^//\s\?||' "$SOURCE_DIR/LICENSE" > "$OUTPUT_DIR/pdfium.txt"

while read -r LIBRARY; do
  case "$LIBRARY" in
    abseil|abseil-cpp)
      cp "$SOURCE_DIR/third_party/abseil-cpp/LICENSE" "$OUTPUT_DIR/abseil.txt"
      ;;
    agg23|fx_agg)
      sed '/^$/d;/#ifndef/,$d' "$SOURCE_DIR/third_party/agg23/agg_array.h" > "$OUTPUT_DIR/agg23.txt"
      ;;
    bigint)
      cp  "$SOURCE_DIR/third_party/bigint/LICENSE" "$OUTPUT_DIR/bigint.txt"
      ;;
    catapult)
      cp "$SOURCE_DIR/third_party/catapult/LICENSE" "$OUTPUT_DIR/catapult.txt"
      ;;
    cpu_features)
      cp "$SOURCE_DIR/third_party/cpu_features/src/LICENSE" "$OUTPUT_DIR/cpu_features.txt"
      ;;
    fast_float)
      cp "$SOURCE_DIR/third_party/fast_float/src/LICENSE-MIT" "$OUTPUT_DIR/fast_float.txt"
      ;;
    fp16)
      cp "$SOURCE_DIR/third_party/fp16/LICENSE" "$OUTPUT_DIR/fp16.txt"
      ;;
    freetype|fx_freetype)
      cp "$SOURCE_DIR/third_party/freetype/FTL.TXT" "$OUTPUT_DIR/freetype.txt"
      ;;
    highway)
      cp "$SOURCE_DIR/third_party/highway/LICENSE" "$OUTPUT_DIR/highway.txt"
      ;;
    icu)
      cp "$SOURCE_DIR/third_party/icu/LICENSE" "$OUTPUT_DIR/icu.txt"
      ;;
    inspector_protocol)
      cp "$SOURCE_DIR/v8/third_party/inspector_protocol/LICENSE" "$OUTPUT_DIR/inspector_protocol.txt"
      ;;
    lcms|lcms2|fx_lcms|fx_lcms2)
      sed '/^$/,$d' "$SOURCE_DIR/third_party/lcms/include/lcms2.h" > "$OUTPUT_DIR/lcms.txt"
      ;;
    libpng|png)
      cp "$SOURCE_DIR/third_party/libpng/LICENSE" "$OUTPUT_DIR/libpng.txt"
      ;;
    libjpeg_turbo|jpeg)
      cp "$SOURCE_DIR/third_party/libjpeg_turbo/LICENSE.md" "$OUTPUT_DIR/libjpeg_turbo.md"
      cp "$SOURCE_DIR/third_party/libjpeg_turbo/README.ijg" "$OUTPUT_DIR/libjpeg_turbo.ijg"
      ;;
    libopenjpeg|libopenjpeg2|fx_libopenjpeg)
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/libopenjpeg/openjpeg.c" > "$OUTPUT_DIR/libopenjpeg.txt"
      ;;
    libtiff|fx_tiff|tiff)
      sed '1d;s/^.\{0,3\}//;23,$d' "$SOURCE_DIR/third_party/libtiff/t4.h" > "$OUTPUT_DIR/libtiff.txt"
      ;;
    libunwind)
      cp "$SOURCE_DIR/third_party/libunwind/src/LICENSE.TXT" "$OUTPUT_DIR/libunwind.txt"
      ;;
    llvm-libc)
      cp "$SOURCE_DIR/third_party/llvm-libc/src/LICENSE.TXT" "$OUTPUT_DIR/llvm-libc.txt"
      ;;
    simdutf)
      cp "$SOURCE_DIR/third_party/simdutf/LICENSE" "$OUTPUT_DIR/simdutf.txt"
      ;;
    zlib)
      sed -n '/^\/\* zlib.h/,/\*\//p' "$SOURCE_DIR/third_party/zlib/zlib.h" > "$OUTPUT_DIR/zlib.txt"
      ;;
    nasm|googletest|test_fonts|libc|vinn)
      # IGNORE: we don't need to include these licenses
      ;;
    *)
      echo "WARNING: unknow library $LIBRARY" >&2
      ;;
  esac
done <<< "$THIRD_PARTY_LIBRARIES"

if [ "$ENABLE_V8" == "true" ]; then
  cp "$SOURCE_DIR/v8/LICENSE.v8" "$OUTPUT_DIR/v8.txt"
  cp "$SOURCE_DIR/v8/LICENSE.fdlibm" "$OUTPUT_DIR/fdlibm.txt"
  cp "$SOURCE_DIR/v8/LICENSE.strongtalk" "$OUTPUT_DIR/strongtalk.txt"
fi

if [ "$TARGET_ENVIRONMENT" == "musl" ]; then
  curl -s -o "$OUTPUT_DIR/musl.txt" https://git.musl-libc.org/cgit/musl/plain/COPYRIGHT
fi

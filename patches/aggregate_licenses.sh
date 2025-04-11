#!/usr/bin/env bash

SOURCE_DIR=${PDFium_SOURCE_DIR:-pdfium}
BUILD_DIR=${PDFium_BUILD_DIR:-pdfium/out}

set -eu

# Extract third-party library names and store in a variable
THIRD_PARTY_LIBRARIES=$(sed -n 's/.*third_party\/\([a-z0-9_-]*\).*/\1/p' "$BUILD_DIR/build.ninja" | sort -u)

printf '# BEGIN PDFium license\n\n'
sed 's|^//\s\?||' "$SOURCE_DIR/LICENSE"
printf '\n# END PDFium license\n\n'

while read -r LIBRARY; do
  case "$LIBRARY" in
    abseil|abseil-cpp)
      printf '\n# BEGIN Abseil license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/abseil-cpp/LICENSE"
      printf '\n# END Abseil license\n\n'
      ;;
    agg23|fx_agg)
      printf '\n# BEGIN agg23 (Anti-Grain Geometry 2.3) license note\n\n'
      sed '/^$/d;/#ifndef/,$d' "$SOURCE_DIR/third_party/agg23/agg_array.h"
      printf '\n# END agg23 (Anti-Grain Geometry 2.3) license note\n\n'
      ;;
    bigint)
      printf '\n# BEGIN bigint license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/bigint/LICENSE"
      printf '\n# END bigint license\n\n'
      ;;
    catapult)
      printf '\n# BEGIN catapult license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/catapult/LICENSE"
      printf '\n# END catapult license\n\n'
      ;;
    cpu_features)
      printf '\n# BEGIN cpu_features license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/cpu_features/src/LICENSE"
      printf '\n# END cpu_features license\n\n'
      ;;
    fast_float)
      printf '\n# BEGIN fast_float license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/fast_float/src/LICENSE-MIT"
      printf '\n# END fast_float license\n\n'
      ;;
    fp16)
      printf '\n# BEGIN fp16 license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/fp16/LICENSE"
      printf '\n# END fp16 license\n\n'
      ;;
    freetype|fx_freetype)
      printf '\n# BEGIN FreeType license\n\n'
      sed -n '1,166p' "$SOURCE_DIR/third_party/freetype/FTL.TXT"
      printf '\n# END FreeType license\n\n'
      ;;
    highway)
      printf '\n# BEGIN highway license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/highway/LICENSE"
      printf '\n# END highway license\n\n'
      ;;
    icu)
      printf '\n# BEGIN ICU (International Components for Unicode) license file\n\n'
      cat "$SOURCE_DIR/third_party/icu/LICENSE"
      printf '\n# END ICU (International Components for Unicode) license file\n'
      ;;
    inspector_protocol)
      printf '\n# BEGIN inspector_protocol license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/v8/third_party/inspector_protocol/LICENSE"
      printf '\n# END inspector_protocol license\n\n'
      ;;
    lcms|lcms2|fx_lcms|fx_lcms2)
      printf '\n# BEGIN lcms license note\n\n'
      sed '/^$/,$d' "$SOURCE_DIR/third_party/lcms/include/lcms2.h"
      printf '\n# END lcms license note\n\n'
      ;;
    libpng|png)
      printf '\n# BEGIN libpng license\n\n'
      sed -n '4,36p' "$SOURCE_DIR/third_party/libpng/LICENSE"
      printf '\n# END libpng license\n\n'
      ;;
    libjpeg_turbo|jpeg)
      printf '\n# BEGIN libjpeg-turbo license file\n\n'
      cat "$SOURCE_DIR/third_party/libjpeg_turbo/LICENSE.md"
      printf '\n# END libjpeg-turbo license file\n\n'
      printf '\n# BEGIN IJG (Independent JPEG Group) legal information\n\n'
      sed -n '115,159p' "$SOURCE_DIR/third_party/libjpeg_turbo/README.ijg"
      printf '\n# END IJG (Independent JPEG Group) legal information\n\n'
      ;;
    libopenjpeg|libopenjpeg2|fx_libopenjpeg)
      printf '\n# BEGIN openjpeg license note\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/libopenjpeg/openjpeg.c"
      printf '\n# END openjpeg license note\n\n'
      ;;
    libtiff|fx_tiff|tiff)
      printf '\n# BEGIN LibTIFF License\n\n'
      sed '1d;s/^.\{0,3\}//;23,$d' "$SOURCE_DIR/third_party/libtiff/t4.h"
      printf '\n# END LibTIFF License\n\n'
      ;;
    libunwind)
      printf '\n# BEGIN libunwind license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/libunwind/src/LICENSE.TXT"
      printf '\n# END libunwind license\n\n'
      ;;
    llvm-libc)
      printf '\n# BEGIN LLVM libc license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/llvm-libc/src/LICENSE.TXT"
      printf '\n# END LLVM libc license\n\n'
      ;;
    simdutf)
      printf '\n# BEGIN simdutf license\n\n'
      sed -n '1,/\*\//p' "$SOURCE_DIR/third_party/simdutf/LICENSE"
      printf '\n# END simdutf license\n\n'
      ;;
    zlib)
      printf '\n# BEGIN zlib license\n\n'
      sed -n '/^\/\* zlib.h/,/\*\//p' "$SOURCE_DIR/third_party/zlib/zlib.h"
      printf '\n# END zlib license\n\n'
      ;;
    nasm|googletest|test_fonts|libc|vinn)
      # IGNORE: we don't need to include these licenses
      ;;
    *)
      echo "WARNING: unknow library $LIBRARY" >&2
      ;;
  esac
done <<< "$THIRD_PARTY_LIBRARIES"

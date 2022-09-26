#!/usr/bin/env bash

set -eu

(
  printf '# BEGIN PDFium license\n\n'
  sed 's|^//\s\?||' LICENSE
  printf '\n# END PDFium license\n\n'

  printf '\n# BEGIN libpng license\n\n'
  sed -n '4,36p' third_party/libpng16/LICENSE
  printf '\n# END libpng license\n\n'

  printf '\n# BEGIN LibTIFF License\n\n'
  sed '1d;s/^.\{0,3\}//;23,$d' third_party/libtiff/t4.h
  printf '\n# END libpng LibTIFF License\n\n'

  printf '\n# BEGIN agg23 (Anti-Grain Geometry 2.3) license note\n\n'
  sed '/^$/d;/#ifndef/,$d' third_party/agg23/agg_array.h
  printf '\n# END agg23 (Anti-Grain Geometry 2.3) license note\n\n'

  printf '\n# BEGIN FreeType license\n\n'
  sed -n '1,166p' third_party/freetype/FTL.TXT
  printf '\n# END FreeType license\n\n'

  printf '\n# BEGIN lcms license note\n\n'
  sed '/^$/,$d' third_party/lcms/include/lcms2.h
  printf '\n# END lcms license note\n\n'

  printf '\n# BEGIN openjpeg license note\n\n'
  sed -n '1,/\*\//p' third_party/libopenjpeg/openjpeg.c
  printf '\n# END openjpeg license note\n\n'

  printf '\n# BEGIN zlib license\n\n'
  sed -n '/^\/\* zlib.h/,/\*\//p' third_party/zlib/zlib.h
  printf '\n# END zlib license\n\n'

  printf '\n# BEGIN libjpeg-turbo license file\n\n'
  cat third_party/libjpeg_turbo/LICENSE.md
  printf '\n# END libjpeg-turbo license file\n\n'

  printf '\n# BEGIN IJG (Independent JPEG Group) legal information\n\n'
  sed -n '115,159p' third_party/libjpeg_turbo/README.ijg
  printf '\n# END IJG (Independent JPEG Group) legal information\n\n'

  printf '\n# BEGIN ICU (International Components for Unicode) license file\n\n'
  cat third_party/icu/LICENSE
  printf '\n# END ICU (International Components for Unicode) license file\n'
) >LICENSES
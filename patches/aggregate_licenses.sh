#!/usr/bin/env bash

set -eu

cat >LICENSES <<END
# BEGIN PDFium license

$(sed 's|^//\s\?||' LICENSE)

# END PDFium license


# BEGIN libpng license

$(sed -n '4,36p' third_party/libpng16/LICENSE)

# END libpng license


# BEGIN LibTIFF License

$(sed '1d;s/^.\{0,3\}//;23,$d' third_party/libtiff/t4.h)

# END libpng LibTIFF License


# BEGIN agg23 (Anti-Grain Geometry 2.3) license note

$(sed '/^$/d;/#ifndef/,$d' third_party/agg23/agg_array.h)

# END agg23 (Anti-Grain Geometry 2.3) license note


# BEGIN FreeType license

$(head -n -2 third_party/freetype/FTL.TXT)

# END FreeType license


# BEGIN lcms license note

$(sed '/^$/,$d' third_party/lcms/include/lcms2.h)

# END lcms license note


# BEGIN openjpeg license note

$(sed -n '1,/\*\//p' third_party/libopenjpeg20/openjpeg.c)

# END openjpeg license note


# BEGIN zlib license

$(sed -n '/^\/\* zlib.h/,/\*\//p' third_party/zlib/zlib.h)

# END zlib license


# BEGIN libjpeg-turbo license file

$(cat 'third_party/libjpeg_turbo/LICENSE.md')

# END libjpeg-turbo license file


# BEGIN IJG (Independent JPEG Group) legal information

$(sed -n '115,159p' third_party/libjpeg_turbo/README.ijg)

# END IJG (Independent JPEG Group) legal information


# BEGIN ICU (International Components for Unicode) license file

$(cat third_party/icu/LICENSE)

# END ICU (International Components for Unicode) license file
END
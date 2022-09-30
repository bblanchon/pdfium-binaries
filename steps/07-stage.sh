#!/bin/bash -eux

IS_DEBUG=${PDFium_IS_DEBUG:-false}
ENABLE_V8=${PDFium_ENABLE_V8:-false}
OS=${PDFium_TARGET_OS:?}
VERSION=${PDFium_VERSION:-}
PATCHES="$PWD/patches"

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD=${PDFium_BUILD_DIR:-pdfium/out}

STAGING="$PWD/staging"
STAGING_BIN="$STAGING/bin"
STAGING_LIB="$STAGING/lib"
STAGING_RES="$STAGING/res"

mkdir -p "$STAGING"
mkdir -p "$STAGING_LIB"

sed "s/#VERSION#/${VERSION:-0.0.0.0}/" <"$PATCHES/PDFiumConfig.cmake" >"$STAGING/PDFiumConfig.cmake"

cp "$SOURCE/LICENSES" "$STAGING/LICENSE"
cp "$BUILD/args.gn" "$STAGING"
cp -R "$SOURCE/public" "$STAGING/include"
rm -f "$STAGING/include/DEPS"
rm -f "$STAGING/include/README"
rm -f "$STAGING/include/PRESUBMIT.py"

case "$OS" in
  android|linux)
    mv "$BUILD/libpdfium.so" "$STAGING_LIB"
    ;;

  mac|ios)
    mv "$BUILD/libpdfium.dylib" "$STAGING_LIB"
    ;;

  wasm)
    mv "$BUILD/pdfium.html" "$STAGING_LIB"
    mv "$BUILD/pdfium.js" "$STAGING_LIB"
    mv "$BUILD/pdfium.wasm" "$STAGING_LIB"
    rm -rf "$STAGING/include/cpp"
    rm "$STAGING/PDFiumConfig.cmake"
    ;;

  win)
    mv "$BUILD/pdfium.dll.lib" "$STAGING_LIB"
    mkdir -p "$STAGING_BIN"
    mv "$BUILD/pdfium.dll" "$STAGING_BIN"
    [ "$IS_DEBUG" == "true" ] && mv "$BUILD/pdfium.dll.pdb" "$STAGING_BIN"
    ;;
esac

if [ "$ENABLE_V8" == "true" ]; then
  mkdir -p "$STAGING_RES"
  mv "$BUILD/snapshot_blob.bin" "$STAGING_RES"

  case "$OS" in
    android)
      ICU_DATA_DIR="android"
      ;;
    ios)
      ICU_DATA_DIR="ios"
      ;;
    wasm)
      ICU_DATA_DIR="flutter"
      ;;
    *)
      ICU_DATA_DIR="common"
      ;;
  esac

  mv "$SOURCE/third_party/icu/$ICU_DATA_DIR/icudtl.dat" "$STAGING_RES"
fi

if [ -n "$VERSION" ]; then
  cat >"$STAGING/VERSION" <<END
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
BUILD=$(echo "$VERSION" | cut -d. -f3)
PATCH=$(echo "$VERSION" | cut -d. -f4)
END
fi
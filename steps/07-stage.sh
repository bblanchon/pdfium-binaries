#!/bin/bash -eux

IS_DEBUG=${PDFium_IS_DEBUG:-false}
OS=${PDFium_TARGET_OS:?}
CPU=${PDFium_TARGET_CPU:?}
LIBC=${PDFium_TARGET_LIBC:-default}
VERSION=${PDFium_VERSION:-}
PATCHES="$PWD/patches"

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD=${PDFium_BUILD_DIR:-pdfium/out}

STAGING="$PWD/staging"
STAGING_BIN="$STAGING/bin"
STAGING_LIB="$STAGING/lib"

mkdir -p "$STAGING"
mkdir -p "$STAGING_LIB"

sed "s/#VERSION#/${VERSION:-0.0.0.0}/" <"$PATCHES/PDFiumConfig.cmake" >"$STAGING/PDFiumConfig.cmake"

cp "$SOURCE/LICENSES" "$STAGING/LICENSE"
cp "$BUILD/args.gn" "$STAGING"
cp -R "$SOURCE/public" -T "$STAGING/include"
rm -f "$STAGING/include/DEPS"
rm -f "$STAGING/include/README"
rm -f "$STAGING/include/PRESUBMIT.py"

case "$OS" in
  android|linux)
    cp "$BUILD/libpdfium.so" "$STAGING_LIB"
    ;;

  mac|ios)
    cp "$BUILD/libpdfium.dylib" "$STAGING_LIB"
    ;;

  wasm)
    cp "$BUILD/pdfium.html" "$STAGING_LIB"
    cp "$BUILD/pdfium.js" "$STAGING_LIB"
    cp "$BUILD/pdfium.wasm" "$STAGING_LIB"
    rm -rf "$STAGING/include/cpp"
    rm "$STAGING/PDFiumConfig.cmake"
    ;;

  win)
    cp "$BUILD/pdfium.dll.lib" "$STAGING_LIB"
    mkdir -p "$STAGING_BIN"
    cp "$BUILD/pdfium.dll" "$STAGING_BIN"
    [ "$IS_DEBUG" == "true" ] && cp "$BUILD/pdfium.dll.pdb" "$STAGING_BIN"
    ;;
esac

V_BUILD=0
if [ -n "$VERSION" ]; then
  V_BUILD=$(echo "$VERSION" | cut -d. -f3)
  cat >"$STAGING/VERSION" <<END
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
BUILD=$V_BUILD
PATCH=$(echo "$VERSION" | cut -d. -f4)
END
fi

# begin conda packaging

case "$OS" in
  linux)
    CONDA_HOST="linux-64"
    ;;
  mac)
    CONDA_HOST="osx-64"
    ;;
  win)
    CONDA_HOST="win-64"
    ;;
  *)
    CONDA_HOST="none"
    ;;
esac

if [ "$CONDA_HOST" != "none" ] && [ "$LIBC" != "musl" ]; then
    
  mkdir -p conda/staging/
  # use only the build version for conda packages to simplify pinning
  VERSION=$V_BUILD conda build conda/ --output-folder conda/out/
  
  case "$OS-$CPU" in
    linux-x86)
      CONDA_TARGET="linux-32"
      ;;
    linux-arm64)
      CONDA_TARGET="linux-aarch64"
      ;;
    linux-arm)
      CONDA_TARGET="linux-armv7l"
      ;;
    mac-arm64)
      CONDA_TARGET="osx-arm64"
      ;;
    win-x86)
      CONDA_TARGET="win-32"
      ;;
    win-arm64)
      CONDA_TARGET="win-arm64"
      ;;
    *)
      CONDA_TARGET="none"
      ;;
  esac
  
  if [ "$CONDA_TARGET" == "none" ]; then
    mkdir -p conda/staging/$CONDA_HOST/
    cp conda/out/$CONDA_HOST/*.tar.bz2 conda/staging/$CONDA_HOST/
  else
    conda convert conda/out/$CONDA_HOST/*.tar.bz2 -p $CONDA_TARGET -o conda/out/
    mkdir -p conda/staging/$CONDA_TARGET/
    cp conda/out/$CONDA_TARGET/*.tar.bz2 conda/staging/$CONDA_TARGET/
  fi
  
fi

# end conda packaging

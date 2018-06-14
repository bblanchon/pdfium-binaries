#!/usr/bin/env bash
#
# Variables to provide:
# CONFIGURATION = Debug | Release
# PDFium_BRANCH = master | chromium/3211 | ...

set -ex

OS=$(uname)
case $OS in
MINGW*)
  OS="windows"
  ;;
*)
  OS=$(echo $OS | tr '[:upper:]' '[:lower:]')
  ;;
esac

# Input
export DepotTools_URL='https://chromium.googlesource.com/chromium/tools/depot_tools.git'
export DepotTools_DIR="$PWD/depot_tools"
export PDFium_URL='https://pdfium.googlesource.com/pdfium.git'
export PDFium_SOURCE_DIR="$PWD/pdfium"
export PDFium_BUILD_DIR="$PDFium_SOURCE_DIR/out"
export PDFium_PATCH_DIR="$PWD/patches"
export PDFium_CMAKE_CONFIG="$PWD/PDFiumConfig.cmake"
export PDFium_ARGS="$PWD/args/$OS.args.gn"

# Output
export PDFium_STAGING_DIR="$PWD/staging"
export PDFium_INCLUDE_DIR="$PDFium_STAGING_DIR/include"
export PDFium_LIB_DIR="$PDFium_STAGING_DIR/lib"
export PDFium_ARTIFACT="$PWD/pdfium-$OS.tgz"
[ "$CONFIGURATION" == "Debug" ] && export PDFium_ARTIFACT="$PWD/pdfium-$OS-debug.tgz"

# Prepare directories
mkdir -p "$PDFium_BUILD_DIR"
mkdir -p "$PDFium_STAGING_DIR"
mkdir -p "$PDFium_LIB_DIR"

# Download depot_tools
git clone "$DepotTools_URL" "$DepotTools_DIR"
export PATH="$DepotTools_DIR:$PATH"

# Clone
gclient config --unmanaged "$PDFium_URL"
gclient sync

# Checkout
cd "$PDFium_SOURCE_DIR"
git checkout "${PDFium_BRANCH:-master}"
gclient sync

# Patch
cd "$PDFium_SOURCE_DIR"
git apply -v "$PDFium_PATCH_DIR/shared_library.patch"
git apply -v "$PDFium_PATCH_DIR/relative_includes.patch"
#git apply -v "$PDFium_PATCH_DIR/static_libstdcxx.patch"

# Configure
cp "$PDFium_ARGS" "$PDFium_BUILD_DIR/args.gn"
[ "$CONFIGURATION" == "Release" ] && echo 'is_debug=false' >> "$PDFium_BUILD_DIR/args.gn"

# Generate Ninja files
gn gen "$PDFium_BUILD_DIR"

# Build
ninja -C "$PDFium_BUILD_DIR" pdfium
ls -l "$PDFium_BUILD_DIR"

# Install
cp "$PDFium_CMAKE_CONFIG" "$PDFium_STAGING_DIR"
cp "$PDFium_SOURCE_DIR/LICENSE" "$PDFium_STAGING_DIR"
cp -R "$PDFium_SOURCE_DIR/public" "$PDFium_INCLUDE_DIR"
rm -f "$PDFium_INCLUDE_DIR/DEPS"
rm -f "$PDFium_INCLUDE_DIR/README"
rm -f "$PDFium_INCLUDE_DIR/PRESUBMIT.py"
[ "$OS" == "linux" ] && mv "$PDFium_BUILD_DIR/libpdfium.so" "$PDFium_LIB_DIR"
[ "$OS" == "darwin" ] && mv "$PDFium_BUILD_DIR/libpdfium.dylib" "$PDFium_LIB_DIR"

# Pack
cd "$PDFium_STAGING_DIR"
tar cvf "$PDFium_ARTIFACT" -- *

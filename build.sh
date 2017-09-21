#!/usr/bin/env bash
#
# Variables to provide:
# CONFIGURATION = Debug | Release
# PDFium_BRANCH = master | chromium/3211 | ...

set -ex

# Input
DepotTools_URL='https://chromium.googlesource.com/chromium/tools/depot_tools.git'
DepotTools_DIR="$PWD/depot_tools"
PDFium_URL='https://pdfium.googlesource.com/pdfium.git'
PDFium_SOURCE_DIR="$PWD/pdfium"
PDFium_BUILD_DIR="$PDFium_SOURCE_DIR/out"
PDFium_PATCH="$PWD/shared_library.patch"
PDFium_CMAKE_CONFIG="$PWD/PDFiumConfig.cmake"
PDFium_ARGS="$PWD/args.gn"

# Output
PDFium_STAGING_DIR="$PWD/staging"
PDFium_INCLUDE_DIR="$PDFium_STAGING_DIR/include"
PDFium_LIB_DIR="$PDFium_STAGING_DIR/lib"
PDFium_ARTIFACT="$PWD/pdfium-linux.zip"
[ "$CONFIGURATION" == "Debug" ] && PDFium_ARTIFACT="$PWD/pdfium-linux-debug.zip"


# Prepare directories
mkdir -p "$PDFium_BUILD_DIR"
mkdir -p "$PDFium_STAGING_DIR"
mkdir -p "$PDFium_LIB_DIR"

# Download depot_tools
git clone "$DepotTools_URL" "$DepotTools_DIR"
PATH="$DepotTools_DIR:$PATH"

# Clone
gclient config --unmanaged "$PDFium_URL"
gclient sync

# Checkout
if [[ -v PDFium_BRANCH ]]; then
	cd "$PDFium_SOURCE_DIR"
	git checkout "$PDFium_BRANCH"
	gclient sync
fi

# Install build deps
sudo "$PDFium_SOURCE_DIR/build/install-build-deps.sh" --no-arm --no-chromeos-fonts --no-nacl --no-syms

# Patch
cd "$PDFium_SOURCE_DIR"
git apply -v "$PDFium_PATCH"

# Configure
[ "$CONFIGURATION" == "Release" ] && echo 'is_debug=false' >> "$PDFium_ARGS"
mv "$PDFium_ARGS" "$PDFium_BUILD_DIR/args.gn"

# Generate Ninja files
gn gen "$PDFium_BUILD_DIR"

# Build
ninja -C "$PDFium_BUILD_DIR" pdfium
ls -l "$PDFium_BUILD_DIR"

# Install
mv "$PDFium_CMAKE_CONFIG" "$PDFium_STAGING_DIR"
mv "$PDFium_SOURCE_DIR/LICENSE" "$PDFium_STAGING_DIR"
mv "$PDFium_SOURCE_DIR/public" "$PDFium_INCLUDE_DIR"
rm -f "$PDFium_INCLUDE_DIR/DEPS"
rm -f "$PDFium_INCLUDE_DIR/README"
mv "$PDFium_BUILD_DIR/pdfium.so" "$PDFium_LIB_DIR"

# Pack
cd %PDFium_STAGING_DIR%
tar cvf "$PDFium_ARTIFACT" ./*

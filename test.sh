#!/usr/bin/env bash

set -ex

export SOURCE_DIR="$PWD/example"
export PDFium_DIR="$PWD/staging"

# Travis overrides CC and CXX, so we had to use different names
[ -n "$_CC" ] && export CC="$_CC"
[ -n "$_CXX" ] && export CC="$_CXX"

[ "$TARGET_CPU" != "" ] && export CMAKE_OSX_ARCHITECTURES="$TARGET_CPU"

mkdir build
cd build
cmake "$SOURCE_DIR"
cmake --build .
if [ "$TARGET_CPU" == "" ]; then
    ctest --output-on-failure .
fi

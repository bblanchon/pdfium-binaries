#!/usr/bin/env bash

set -ex

export SOURCE_DIR="$PWD/example"
export PDFium_DIR="$PWD/staging"

[ "$TARGET_CPU" != "" ] && export CMAKE_OSX_ARCHITECTURES="$TARGET_CPU"

mkdir build
cd build
cmake "$SOURCE_DIR"
cmake --build .
if [ "$TARGET_CPU" == "" ]; then
    ctest --output-on-failure .
fi

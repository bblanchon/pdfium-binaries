#!/usr/bin/env bash

set -eux

export SOURCE_DIR="$PWD/example"
export PDFium_DIR="$PWD/staging"

mkdir build
cd build
cmake "$SOURCE_DIR"
cmake --build .
ctest --output-on-failure .
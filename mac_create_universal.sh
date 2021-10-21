#!/usr/bin/env bash
# Current directory has to contain pdfium-....tgz for x64 and arm

set -e

SRC_DIR="$PWD"
STAGE_DIR="$PWD/staging_univ"

create_univ () {
    if [ -e "pdfium-mac-x64$1.tgz" ] && [ -e "pdfium-mac-arm64$1.tgz" ]; then
        echo "Extracting x64..."
        mkdir -p $STAGE_DIR/x64
        cd $STAGE_DIR/x64
        tar xf $SRC_DIR/pdfium-mac-x64$1.tgz

        echo "Extracting arm64..."
        mkdir -p $STAGE_DIR/arm64
        cd $STAGE_DIR/arm64
        tar xf $SRC_DIR/pdfium-mac-arm64$1.tgz

        echo "Creating universal..."
        mkdir -p $STAGE_DIR/univ
        cp -r $STAGE_DIR/x64/* $STAGE_DIR/univ
        lipo -create \
            $STAGE_DIR/x64/lib/libpdfium.dylib \
            $STAGE_DIR/arm64/lib/libpdfium.dylib \
            -output $STAGE_DIR/univ/lib/libpdfium.dylib

        echo "Creating target..."
        cd $STAGE_DIR/univ
        tar cf "$SRC_DIR/pdfium-mac-universal$1.tgz" -- *

        cd $SRC_DIR
    fi
}

create_univ ""
create_univ "-v8"
create_univ "-v8-debug"

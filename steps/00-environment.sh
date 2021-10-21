#!/bin/bash -eux

ENV_FILE=${GITHUB_ENV:-.env}

# Input
cat >>"$ENV_FILE" <<END
PDFium_SOURCE_DIR=$PWD/pdfium
PDFium_BUILD_DIR=$PWD/pdfium/out
DEPOT_TOOLS_WIN_TOOLCHAIN=0
END

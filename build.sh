#!/usr/bin/env bash
#
# Variables to provide:
# PDFium_TARGET_CPU = x86 | x64 | arm | arm64 | wasm
# PDFium_TARGET_OS = mac | linux | win | wasm
# PDFium_TARGET_LIBC = musl | default
# PDFium_BRANCH = main | chromium/3211 | ...
# PDFium_ENABLE_V8 = true | false
# PDFium_IS_DEBUG = true | false

set -ex

ENV_FILE=${GITHUB_ENV:-.env}
PATH_FILE=${GITHUB_PATH:-.path}

# . steps/00-environment.sh
source "$ENV_FILE"

# . steps/01-install.sh
PATH="$(tr '\n' ':' < "$PATH_FILE")$PATH"
export PATH

# . steps/02-checkout.sh
# . steps/03-patch.sh
# . steps/04-install-extras.sh
# . steps/05-configure.sh
# . steps/06-build.sh
# . steps/07-pack.sh
. steps/08-test.sh

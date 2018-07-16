#!/usr/bin/env bash

set -eu

cd $(dirname "$0")

TAGNAME=$(git ls-remote --heads https://pdfium.googlesource.com/pdfium.git | grep -ohP 'chromium/\d+' | tail -n1)
git pull
git tag "$TAGNAME"
git push --tags

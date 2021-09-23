#!/bin/bash -eux

PATCHES="$PWD/patches"
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_OS:?}"

pushd "${SOURCE}"

git apply -v "$PATCHES/shared_library.patch"
git apply -v "$PATCHES/relative_includes.patch"
#git apply -v "$PATCHES/static_libstdcxx.patch"

[ "${PDFium_V8:-}" == "enabled" ] && git apply -v "$PATCHES/v8_init.patch"

if [ "$OS" == "windows" ]; then
  git apply -v "$PATCHES/widestring.patch"
  git -C build apply -v "$PATCHES/rc_compiler.patch"
  cp "$PATCHES/resources.rc" .
fi

popd
#!/bin/bash -eux

PDFium_URL='https://pdfium.googlesource.com/pdfium.git'
OS=${PDFium_TARGET_OS:?}

# Clone
gclient config --unmanaged "$PDFium_URL"
echo "target_os = [ '$OS' ]" >> .gclient
gclient sync

# Switch branch
if [ -n "${PDFium_BRANCH:-}" ]; then
  pushd pdfium
  git checkout "${PDFium_BRANCH}"
  gclient sync
  popd
fi
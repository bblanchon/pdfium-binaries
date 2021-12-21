#!/bin/bash -eux

PDFium_URL='https://pdfium.googlesource.com/pdfium.git'
OS=${PDFium_TARGET_OS:?}

# Clone
gclient config --unmanaged "$PDFium_URL"
echo "target_os = [ '$OS' ]" >> .gclient

gclient sync -r "origin/${PDFium_BRANCH:-main}" --no-history --shallow
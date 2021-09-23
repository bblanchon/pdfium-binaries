#!/bin/bash -eux

BUILD_DIR=${PDFium_BUILD_DIR?-pdfium/out}

ninja -C "$BUILD_DIR" pdfium
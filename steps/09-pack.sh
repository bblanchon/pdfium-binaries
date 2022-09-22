#!/bin/bash -eux

IS_DEBUG=${PDFium_IS_DEBUG:-false}
ENABLE_V8=${PDFium_ENABLE_V8:-false}
OS=${PDFium_TARGET_OS:?}
TARGET_LIBC=${PDFium_TARGET_LIBC:-default}
CPU=${PDFium_TARGET_CPU:?}
STAGING="$PWD/staging"

ARTIFACT_BASE="$PWD/pdfium-$OS"
[ "$TARGET_LIBC" != "default" ] && ARTIFACT_BASE="$ARTIFACT_BASE-$TARGET_LIBC"
[ "$OS" != "$CPU" ] && ARTIFACT_BASE="$ARTIFACT_BASE-$CPU"
[ "$ENABLE_V8" == "true" ] && ARTIFACT_BASE="$ARTIFACT_BASE-v8"
[ "$IS_DEBUG" == "true" ] && ARTIFACT_BASE="$ARTIFACT_BASE-debug"
ARTIFACT="$ARTIFACT_BASE.tgz"

pushd "$STAGING"
tar cvzf "$ARTIFACT" -- *
popd
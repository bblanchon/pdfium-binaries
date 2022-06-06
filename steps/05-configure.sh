#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD=${PDFium_BUILD_DIR:-$SOURCE/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}
TARGET_LIBC=${PDFium_TARGET_LIBC:-default}
ENABLE_V8=${PDFium_ENABLE_V8:-false}
IS_DEBUG=${PDFium_IS_DEBUG:-false}

mkdir -p "$BUILD"

(
  echo "is_debug = $IS_DEBUG"
  echo "pdf_is_standalone = true"
  echo "target_cpu = \"$TARGET_CPU\""
  echo "target_os = \"$OS\""
  echo "pdf_enable_v8 = $ENABLE_V8"
  echo "pdf_enable_xfa = $ENABLE_V8"
  echo "treat_warnings_as_errors = false"

  case "$OS" in
    ios)
      echo "ios_enable_code_signing = false"
      ;;
    mac)
      echo 'mac_deployment_target = "10.11.0"'
      ;;
    win)
      echo 'pdf_use_win32_gdi = true'
      ;;
    wasm):
      echo 'pdf_is_complete_lib = true'
      echo 'is_clang = false'
      ;;
  esac

  case "$TARGET_LIBC" in
    musl)
      echo 'is_musl = true'
      echo 'is_clang = false'
      echo 'use_custom_libcxx = false'
      ;;
  esac

) | sort > "$BUILD/args.gn"

# Generate Ninja files
pushd "$SOURCE"
gn gen "$BUILD"
popd
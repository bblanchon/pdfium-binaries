#!/bin/bash -eux

OS=${PDFium_TARGET_OS:?}
SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD=${PDFium_BUILD_DIR:-pdfium/out}
TARGET_CPU=${PDFium_TARGET_CPU:?}
if [ "${PDFium_V8:-}" == "enabled" ]; then
  ENABLE_V8="true"
else
  ENABLE_V8="false"
fi
if [ "${PDFium_CONFIGURATION:-}" == "Debug" ]; then
  IS_DEBUG="true"
else
  IS_DEBUG="false"
fi

mkdir -p "$BUILD"

(
  echo "is_debug = $IS_DEBUG"
  echo "pdf_is_standalone = true"
  echo "target_cpu = \"$TARGET_CPU\""
  echo "target_os = \"$OS\""
  echo "pdf_enable_v8 = $ENABLE_V8"
  echo "pdf_enable_xfa = $ENABLE_V8"

  case "$OS" in
    ios)
      echo "ios_enable_code_signing = false"
      ;;
    linux)
      echo 'use_custom_libcxx = true'
      ;;
    mac)
      echo 'mac_deployment_target = "10.11.0"'
      ;;
    win)
      echo 'pdf_use_win32_gdi = true'
      ;;
  esac

) | sort > "$BUILD/args.gn"

# Generate Ninja files
pushd "$SOURCE"
gn gen "$BUILD"
popd
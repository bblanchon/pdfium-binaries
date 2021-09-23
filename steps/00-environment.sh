#!/bin/bash -eux

ENV_FILE=${GITHUB_ENV:-.env}

OS=$(uname)
case $OS in
MINGW*)
  OS="windows"
  ;;
*)
  OS=$(echo $OS | tr '[:upper:]' '[:lower:]')
  ;;
esac

# Input
cat >>"$ENV_FILE" <<END
PDFium_OS=$OS
PDFium_SOURCE_DIR=$PWD/pdfium
PDFium_BUILD_DIR=$PWD/pdfium/out
END

# Extra config for Windows
if [ "$OS" == "windows" ]; then
  echo "DEPOT_TOOLS_WIN_TOOLCHAIN=0" >> "$ENV_FILE"
fi

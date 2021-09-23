#!/bin/bash -eux

PATH_FILE=${GITHUB_PATH:-.path}
OS=${PDFium_OS:?}
CPU="${PDFium_TARGET_CPU:?}"

DepotTools_URL='https://chromium.googlesource.com/chromium/tools/depot_tools.git'
DepotTools_DIR="$PWD/depot_tools"
WindowsSDK_DIR="/c/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0"

# Download depot_tools if not exists in this location or update utherwise
if [ ! -d "$DepotTools_DIR" ]; then
  git clone "$DepotTools_URL" "$DepotTools_DIR"
else 
  pushd "$DepotTools_DIR"
  git pull
  popd
fi

echo "$DepotTools_DIR" >> "$PATH_FILE"

if [ "$OS" == "windows" ]; then
  echo "$WindowsSDK_DIR/$CPU" >> "$PATH_FILE"
fi

if [ "$OS" == "linux" ] && [ "$CPU" == "arm" ]; then
  sudo apt-get update
  sudo apt-get install -y g++-arm-linux-gnueabihf
fi
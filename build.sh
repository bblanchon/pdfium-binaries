#!/bin/bash -eu

OS_NAMES="android|ios|linux|mac|wasm|win"
CPU_NAMES="arm|arm64|x64|x86|wasm"

if [[ $# == 0 ]]
then
  echo "PDFium build script.
https://github.com/bblanchon/pdfium-binaries

Usage $0 [options] os cpu

Arguments:
   os       = Target OS ($OS_NAMES)
   cpu      = Target CPU ($CPU_NAMES)

Options:
  -b branch = Chromium branch (default=main)
  -d        = debug build
  -j        = enable v8
  -m        = build fo musl"
  exit
fi

while getopts "b:djm" OPTION
do
  case $OPTION in
    b)
      export PDFium_BRANCH="$OPTARG"
      ;;

    d)
      export PDFium_IS_DEBUG=true
      ;;

    j)
      export PDFium_ENABLE_V8=true
      ;;

    m)
      export PDFium_TARGET_LIBC=musl
      ;;

    *)
      echo "Invalid flag -$OPTION"
      exit 1
  esac
done
shift $(($OPTIND -1))

if [[ $# -lt 2 ]]
then
  echo "You must specify target OS and CPU"
  exit 1
fi

if [[ $# -gt 2 ]]
then
  echo "Too many arguments"
  exit 1
fi

if [[ ! $1 =~ ^($OS_NAMES)$ ]]
then
  echo "Unknown OS: $1"
  exit 1
fi

if [[ ! $2 =~ ^($CPU_NAMES)$ ]]
then
  echo "Unknown CPU: $2"
  exit 1
fi

export PDFium_TARGET_OS=$1
export PDFium_TARGET_CPU=$2

set -x

ENV_FILE=${GITHUB_ENV:-.env}
PATH_FILE=${GITHUB_PATH:-.path}

. steps/00-environment.sh
source "$ENV_FILE"

. steps/01-install.sh
PATH="$(tr '\n' ':' < "$PATH_FILE")$PATH"
export PATH

. steps/02-checkout.sh
. steps/03-patch.sh
. steps/04-install-extras.sh
. steps/05-configure.sh
. steps/06-build.sh
. steps/07-stage.sh
. steps/08-test.sh
. steps/09-pack.sh

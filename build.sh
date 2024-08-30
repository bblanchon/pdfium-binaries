#!/bin/bash -eu

OS_NAMES="android|ios|linux|mac|wasm|win|maccatalyst"
CPU_NAMES="arm|arm64|x64|x86|wasm"
STEP_REGEX="[0-9]"

START_STEP=0

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
  -s 0-9    = Set start step (default=0)
  -d        = debug build
  -j        = enable v8
  -m        = build fo musl"
  exit
fi

while getopts "b:djms:" OPTION
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

    s)
      START_STEP="$OPTARG"
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

if [[ ! $START_STEP =~ ^($STEP_REGEX)$ ]]
then
  echo "Invalid step number: $START_STEP"
  exit 1
fi

export PDFium_TARGET_OS=$1
export PDFium_TARGET_CPU=$2

set -x

ENV_FILE=${GITHUB_ENV:-.env}
PATH_FILE=${GITHUB_PATH:-.path}

[ $START_STEP -le 0 ] && . steps/00-environment.sh
source "$ENV_FILE"

[ $START_STEP -le 1 ] && . steps/01-install.sh
PATH="$(tr '\n' ':' < "$PATH_FILE")$PATH"
export PATH

[ $START_STEP -le 2 ] && . steps/02-checkout.sh
[ $START_STEP -le 3 ] && . steps/03-patch.sh
[ $START_STEP -le 4 ] && . steps/04-install-extras.sh
[ $START_STEP -le 5 ] && . steps/05-configure.sh
[ $START_STEP -le 6 ] && . steps/06-build.sh
[ $START_STEP -le 7 ] && . steps/07-stage.sh
[ $START_STEP -le 8 ] && . steps/08-test.sh
[ $START_STEP -le 9 ] && . steps/09-pack.sh

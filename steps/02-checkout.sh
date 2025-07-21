#!/bin/bash -eux

PDFium_URL='https://pdfium.googlesource.com/pdfium.git'
OS=${PDFium_TARGET_OS:?}
ENABLE_V8=${PDFium_ENABLE_V8:-false}

CONFIG_ARGS=()
if [ "$ENABLE_V8" == "false" ]; then
  CONFIG_ARGS+=(
     --custom-var "checkout_configuration=minimal"
  )
fi

# Clone
gclient config --unmanaged "$PDFium_URL" "${CONFIG_ARGS[@]-}"
echo "target_os = [ '$OS' ]" >> .gclient


# Reset
for FOLDER in pdfium pdfium/build pdfium/v8 pdfium/third_party/libjpeg_turbo pdfium/base/allocator/partition_allocator; do
  if [ -e "$FOLDER" ]; then
    git -C $FOLDER reset --hard
    git -C $FOLDER clean -df
  fi
done

gclient sync -r "origin/${PDFium_BRANCH:-main}" --no-history --shallow

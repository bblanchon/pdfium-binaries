#!/bin/bash -eux

IS_DEBUG=${PDFium_IS_DEBUG:-false}
OS=${PDFium_TARGET_OS:?}
TARGET_ENVIRONMENT=${PDFium_TARGET_ENVIRONMENT:-}
VERSION=${PDFium_VERSION:-}
PATCHES="$PWD/patches"
BUILD_TYPE=${PDFium_BUILD_TYPE:-shared}

SOURCE=${PDFium_SOURCE_DIR:-pdfium}
BUILD=${PDFium_BUILD_DIR:-pdfium/out}

STAGING="$PWD/staging"
STAGING_BIN="$STAGING/bin"
STAGING_LIB="$STAGING/lib"

mkdir -p "$STAGING"
rm -rf "$STAGING"/*
mkdir -p "$STAGING_LIB"

case "$BUILD_TYPE" in
  shared)
    CMAKE_CONFIG_FILE="PDFiumConfig.cmake"
    ;;
  static)
    CMAKE_CONFIG_FILE="PDFiumStaticConfig.cmake"
    ;;
esac
sed "s/#VERSION#/${VERSION:-0.0.0.0}/" <"$PATCHES/$CMAKE_CONFIG_FILE" >"$STAGING/PDFiumConfig.cmake"

cp LICENSE "$STAGING"
cat >>"$STAGING/LICENSE" <<END

This package also includes third-party software. See the licenses/ directory for their respective licenses.
END

cp "$BUILD/args.gn" "$STAGING"
cp -R "$SOURCE/public" "$STAGING/include"
rm -f "$STAGING/include/DEPS"
rm -f "$STAGING/include/README"
rm -f "$STAGING/include/PRESUBMIT.py"

case "$OS-$BUILD_TYPE" in
  android-shared|linux-shared)
    mv "$BUILD/libpdfium.so" "$STAGING_LIB"
    ;;

  android-static|linux-static|mac-static|ios-static)
    mv "$BUILD/obj/libpdfium.a" "$STAGING_LIB"
    ;;

  mac-shared|ios-shared)
    mv "$BUILD/libpdfium.dylib" "$STAGING_LIB"
    ;;

  emscripten-*)
    mv "$BUILD/pdfium.html" "$STAGING_LIB"
    mv "$BUILD/pdfium.js" "$STAGING_LIB"
    mv "$BUILD/pdfium.wasm" "$STAGING_LIB"
    rm -rf "$STAGING/include/cpp"
    rm "$STAGING/PDFiumConfig.cmake"
    ;;

  win-shared)
    mv "$BUILD/pdfium.dll.lib" "$STAGING_LIB"
    mkdir -p "$STAGING_BIN"
    mv "$BUILD/pdfium.dll" "$STAGING_BIN"
    [ "$IS_DEBUG" == "true" ] && mv "$BUILD/pdfium.dll.pdb" "$STAGING_BIN"
    ;;

  win-shared)
    mv "$BUILD/obj/pdfium.lib" "$STAGING_LIB"
    ;;
esac

# The App Store rejects standalone dylibs in iOS apps, they must be embedded as frameworks
if [ "$OS-$BUILD_TYPE" == "ios-shared" ] && [ "$TARGET_ENVIRONMENT" != "catalyst" ]; then
  FRAMEWORK="$STAGING_LIB/pdfium.framework"
  mkdir -p "$FRAMEWORK"
  cp "$STAGING_LIB/libpdfium.dylib" "$FRAMEWORK/pdfium"
  install_name_tool -id @rpath/pdfium.framework/pdfium "$FRAMEWORK/pdfium"

  case "$TARGET_ENVIRONMENT" in
    simulator)
      PLATFORM="iPhoneSimulator"
      ;;
    *)
      PLATFORM="iPhoneOS"
      ;;
  esac

  # The App Store requires MinimumOSVersion to match the binary (ITMS-90208)
  MINIMUM_OS_VERSION=$(xcrun vtool -show-build "$FRAMEWORK/pdfium" | awk '$1 == "minos" { print $2; exit }')
  BUNDLE_VERSION=$(echo "${VERSION:-0.0.0}" | cut -d. -f1-3)

  cat >"$FRAMEWORK/Info.plist" <<END
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleExecutable</key>
  <string>pdfium</string>
  <key>CFBundleIdentifier</key>
  <string>io.github.bblanchon.pdfium</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>pdfium</string>
  <key>CFBundlePackageType</key>
  <string>FMWK</string>
  <key>CFBundleShortVersionString</key>
  <string>$BUNDLE_VERSION</string>
  <key>CFBundleVersion</key>
  <string>$BUNDLE_VERSION</string>
  <key>CFBundleSupportedPlatforms</key>
  <array>
    <string>$PLATFORM</string>
  </array>
  <key>MinimumOSVersion</key>
  <string>$MINIMUM_OS_VERSION</string>
</dict>
</plist>
END
fi

if [ -n "$VERSION" ]; then
  cat >"$STAGING/VERSION" <<END
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
BUILD=$(echo "$VERSION" | cut -d. -f3)
PATCH=$(echo "$VERSION" | cut -d. -f4)
END
fi

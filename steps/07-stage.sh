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
if [ "$OS-$BUILD_TYPE" == "ios-shared" ]; then
  FRAMEWORK="$STAGING_LIB/pdfium.framework"
  FRAMEWORK_CONTENTS="$FRAMEWORK"
  RESOURCES="$FRAMEWORK"
  case "$TARGET_ENVIRONMENT" in
    catalyst)
      PLATFORM="MacOSX"
      FRAMEWORK_CONTENTS="$FRAMEWORK/Versions/A"
      RESOURCES="$FRAMEWORK_CONTENTS/Resources"
      mkdir -p "$FRAMEWORK_CONTENTS"
      ln -s A "$FRAMEWORK/Versions/Current"
      ln -s Versions/Current/pdfium "$FRAMEWORK/pdfium"
      ln -s Versions/Current/Resources "$FRAMEWORK/Resources"
      ;;
    simulator)
      PLATFORM="iPhoneSimulator"
      ;;
    *)
      PLATFORM="iPhoneOS"
      ;;
  esac

  mkdir -p "$RESOURCES"
  cp "$STAGING_LIB/libpdfium.dylib" "$FRAMEWORK_CONTENTS/pdfium"
  install_name_tool -id @rpath/pdfium.framework/pdfium "$FRAMEWORK_CONTENTS/pdfium"

  # The App Store requires MinimumOSVersion to match the binary (ITMS-90208)
  MINIMUM_OS_VERSION=$(xcrun vtool -show-build "$FRAMEWORK/pdfium" | awk '$1 == "minos" { print $2; exit }')
  MINIMUM_OS_VERSION_KEY="MinimumOSVersion"
  if [ "$TARGET_ENVIRONMENT" == "catalyst" ]; then
    MINIMUM_OS_VERSION_KEY="LSMinimumSystemVersion"
    SDK_SETTINGS="$(xcrun --sdk macosx --show-sdk-path)/SDKSettings.plist"
    MINIMUM_OS_VERSION=$(/usr/libexec/PlistBuddy -c "Print :VersionMap:iOSMac_macOS:$MINIMUM_OS_VERSION" "$SDK_SETTINGS")
  fi
  BUNDLE_VERSION=$(echo "${VERSION:-0.0.0}" | cut -d. -f1-3)

  export PLATFORM MINIMUM_OS_VERSION MINIMUM_OS_VERSION_KEY BUNDLE_VERSION
  envsubst < "$PATCHES/ios/Info.plist" > "$RESOURCES/Info.plist"

  if [ "$TARGET_ENVIRONMENT" == "catalyst" ]; then
    # NuGet and GitHub artifacts do not preserve the symlinks in versioned frameworks.
    # The .NET SDK automatically extracts .framework.zip files before embedding them.
    (cd "$STAGING_LIB" && zip -qry --symlinks pdfium.framework.zip pdfium.framework)
    rm -rf "$FRAMEWORK"
  fi
fi

if [ -n "$VERSION" ]; then
  cat >"$STAGING/VERSION" <<END
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
BUILD=$(echo "$VERSION" | cut -d. -f3)
PATCH=$(echo "$VERSION" | cut -d. -f4)
END
fi

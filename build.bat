: Variables to provide:
: GYP_MSVS_VERSION = 2017 | 2015
: CONFIGURATION = Debug | Release
: PLATFORM = x86 | x64
: PDFium_BRANCH = master | chromium/3211 | ...

: Input
set DepotTools_URL=https://storage.googleapis.com/chrome-infra/depot_tools.zip
set DepotTools_DIR=%CD%/depot_tools
set PDFium_URL=https://pdfium.googlesource.com/pdfium.git
set PDFium_SOURCE_DIR=%CD%\pdfium
set PDFium_BUILD_DIR=%PDFium_SOURCE_DIR%\out
set PDFium_PATCH_DIR=%CD%\patches
set PDFium_CMAKE_CONFIG=%CD%\PDFiumConfig.cmake
set PDFium_ARGS=%CD%\args\windows.args.gn

: Output
set PDFium_STAGING_DIR=%CD%\staging
set PDFium_INCLUDE_DIR=%PDFium_STAGING_DIR%\include
set PDFium_BIN_DIR=%PDFium_STAGING_DIR%\%PLATFORM%\bin
set PDFium_LIB_DIR=%PDFium_STAGING_DIR%\%PLATFORM%\lib
set PDFium_ARTIFACT=%CD%\pdfium-windows-%PLATFORM%.zip
if "%CONFIGURATION%"=="Debug" set PDFium_ARTIFACT=%CD%\pdfium-windows-%PLATFORM%-debug.zip

echo on

: Prepare directories
mkdir %PDFium_BUILD_DIR%
mkdir %PDFium_STAGING_DIR%
mkdir %PDFium_BIN_DIR%
mkdir %PDFium_LIB_DIR%

: Download depot_tools
call curl -fsSL -o depot_tools.zip %DepotTools_URL% || exit /b
call 7z -bd x depot_tools.zip -o%DepotTools_DIR% || exit /b
set PATH=%DepotTools_DIR%;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

: Clone
call gclient config --unmanaged %PDFium_URL% || exit /b
call gclient sync || exit /b

: Checkout
echo on
cd %PDFium_SOURCE_DIR%
call git checkout %PDFium_BRANCH% || exit /b
call gclient sync || exit /b

: Patch
echo on
cd %PDFium_SOURCE_DIR%
copy "%PDFium_PATCH_DIR%\resources.rc" . || exit /b
call git apply -v "%PDFium_PATCH_DIR%\shared_library.patch" || exit /b
call git apply -v "%PDFium_PATCH_DIR%\relative_includes.patch" || exit /b

: Configure
copy %PDFium_ARGS% %PDFium_BUILD_DIR%\args.gn
if "%CONFIGURATION%"=="Release" echo is_debug=false >> %PDFium_BUILD_DIR%\args.gn
if "%PLATFORM%"=="x86" echo target_cpu="x86" >> %PDFium_BUILD_DIR%\args.gn

: Generate Ninja files
call gn gen %PDFium_BUILD_DIR% || exit /b

: Build
call ninja -C %PDFium_BUILD_DIR% pdfium || exit /b

: Install
copy %PDFium_CMAKE_CONFIG% %PDFium_STAGING_DIR%
copy %PDFium_SOURCE_DIR%\LICENSE %PDFium_STAGING_DIR%
xcopy /S /Y %PDFium_SOURCE_DIR%\public %PDFium_INCLUDE_DIR%\
del %PDFium_INCLUDE_DIR%\DEPS
del %PDFium_INCLUDE_DIR%\README
del %PDFium_INCLUDE_DIR%\PRESUBMIT.py
move %PDFium_BUILD_DIR%\pdfium.dll.lib %PDFium_LIB_DIR%
move %PDFium_BUILD_DIR%\pdfium.dll %PDFium_BIN_DIR%
if "%CONFIGURATION%"=="Debug" move %PDFium_BUILD_DIR%\pdfium.dll.pdb %PDFium_BIN_DIR%

: Pack
cd %PDFium_STAGING_DIR%
call 7z a %PDFium_ARTIFACT% *
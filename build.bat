: Variables to provide:
: GYP_MSVS_VERSION = 2017 | 2015
: CONFIGURATION = Debug | Release
: PLATFORM = x86 | x64

: Input
set DepotTools_URL=https://storage.googleapis.com/chrome-infra/depot_tools.zip
set DepotTools_DIR=%CD%/depot_tools
set PDFium_URL=https://pdfium.googlesource.com/pdfium.git
set PDFium_SOURCE_DIR=%CD%\pdfium
set PDFium_BUILD_DIR=%PDFium_SOURCE_DIR%\out
set PDFium_PATCH=%CD%\shared_library.patch
set PDFium_CMAKE_CONFIG=%CD%\PDFiumConfig.cmake
set PDFium_ARGS=%CD%\args.gn

: Output
set PDFium_NAME=pdfium
if "%CONFIGURATION%"=="Debug" set PDFium_NAME=pdfiumd
set PDFium_STAGING_DIR=%CD%\staging
set PDFium_INCLUDE_DIR=%PDFium_STAGING_DIR%\include
set PDFium_BIN_DIR=%PDFium_STAGING_DIR%\%PLATFORM%\bin
set PDFium_DLL_FILE=%PDFium_BIN_DIR%\%PDFium_NAME%.dll
set PDFium_PDB_FILE=%PDFium_BIN_DIR%\%PDFium_NAME%.pdb
set PDFium_LIB_DIR=%PDFium_STAGING_DIR%\%PLATFORM%\lib
set PDFium_LIB_FILE=%PDFium_LIB_DIR%\%PDFium_NAME%.lib
set PDFium_ARTIFACT=%CD%\pdfium-%PLATFORM%.zip
if "%CONFIGURATION%"=="Debug" set PDFium_ARTIFACT=%CD%\pdfium-%PLATFORM%-debug.zip

echo on

: Prepare directories
mkdir %PDFium_BUILD_DIR%
mkdir %PDFium_STAGING_DIR%
mkdir %PDFium_BIN_DIR%
mkdir %PDFium_LIB_DIR%

: Download depot_tools
call curl -fsSL -o depot_tools.zip %DepotTools_URL%
call 7z -bd x depot_tools.zip -o%DepotTools_DIR%
set PATH=%DepotTools_DIR%;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

: Checkout
call gclient config --unmanaged %PDFium_URL%
call gclient sync

: Patch
echo on
cd %PDFium_SOURCE_DIR%
call git apply -v %PDFium_PATCH%

: Configure
if "%CONFIGURATION%"=="Release" echo is_debug=false >> %PDFium_ARGS%
if "%PLATFORM%"=="x86" echo target_cpu="x86" >> %PDFium_ARGS%
move %PDFium_ARGS% %PDFium_BUILD_DIR%\args.gn

: Generate Ninja files
call gn gen %PDFium_BUILD_DIR%

: Build
call ninja -C %PDFium_BUILD_DIR% pdfium

: Install
move %PDFium_CMAKE_CONFIG% %PDFium_STAGING_DIR%
move %PDFium_SOURCE_DIR%\LICENSE %PDFium_STAGING_DIR%
move %PDFium_SOURCE_DIR%\public %PDFium_INCLUDE_DIR%
del %PDFium_INCLUDE_DIR%\DEPS
del %PDFium_INCLUDE_DIR%\README
move %PDFium_BUILD_DIR%\pdfium.dll.lib %PDFium_LIB_FILE%
move %PDFium_BUILD_DIR%\pdfium.dll %PDFium_DLL_FILE%
if "%CONFIGURATION%"=="Debug" move %PDFium_BUILD_DIR%\pdfium.dll.pdb %PDFium_PDB_FILE%
cd %PDFium_STAGING_DIR%

: Pack
call 7z a %PDFium_ARTIFACT% *
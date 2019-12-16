: Variables to provide:
: GYP_MSVS_VERSION = 2017 | 2015
: CONFIGURATION = Debug | Release
: PLATFORM = x86 | x64
: PDFium_BRANCH = master | chromium/3211 | ...
: PDFium_V8 = enabled

: Input
set WindowsSDK_DIR=C:\Program Files (x86)\Windows Kits\10\bin\%PLATFORM%
set DepotTools_URL=https://storage.googleapis.com/chrome-infra/depot_tools.zip
set DepotTools_DIR=%CD%\depot_tools
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
set PDFium_RES_DIR=%PDFium_STAGING_DIR%\%PLATFORM%\res
set PDFium_ARTIFACT_BASE=%CD%\pdfium-windows-%PLATFORM%
if "%PDFium_V8%"=="enabled" set PDFium_ARTIFACT_BASE=%PDFium_ARTIFACT_BASE%-v8
if "%CONFIGURATION%"=="Debug" set PDFium_ARTIFACT_BASE=%PDFium_ARTIFACT_BASE%-debug
set PDFium_ARTIFACT=%PDFium_ARTIFACT_BASE%.zip

echo on

: Prepare directories
mkdir %PDFium_BUILD_DIR%
mkdir %PDFium_STAGING_DIR%
mkdir %PDFium_BIN_DIR%
mkdir %PDFium_LIB_DIR%

: Download depot_tools
call curl -fsSL -o depot_tools.zip %DepotTools_URL% || exit /b
call 7z -bd -y x depot_tools.zip -o%DepotTools_DIR% || exit /b
set PATH=%DepotTools_DIR%;%WindowsSDK_DIR%;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

: check that rc.exe is in PATH
where rc.exe || exit /b

: Clone
call gclient config --unmanaged %PDFium_URL% || exit /b
call gclient sync || exit /b

: Checkout branch (or ignore if it doesn't exist)
echo on
cd %PDFium_SOURCE_DIR%
git.exe checkout %PDFium_BRANCH% && call gclient sync

: Install python packages
where python
call %DepotTools_DIR%\python.bat -m pip install pywin32 || exit /b

: Patch
cd %PDFium_SOURCE_DIR%
copy "%PDFium_PATCH_DIR%\resources.rc" . || exit /b
git.exe apply -v "%PDFium_PATCH_DIR%\shared_library.patch" || exit /b
git.exe apply -v "%PDFium_PATCH_DIR%\relative_includes.patch" || exit /b
if "%PDFium_V8%"=="enabled" git.exe apply -v "%PDFium_PATCH_DIR%\v8_init.patch" || exit /b
git.exe -C build apply -v "%PDFium_PATCH_DIR%\rc_compiler.patch" || exit /b

: Configure
copy %PDFium_ARGS% %PDFium_BUILD_DIR%\args.gn
if "%CONFIGURATION%"=="Release" echo is_debug=false >> %PDFium_BUILD_DIR%\args.gn
if "%PLATFORM%"=="x86" echo target_cpu="x86" >> %PDFium_BUILD_DIR%\args.gn
if "%PDFium_V8%"=="enabled" echo pdf_enable_v8=true >> %PDFium_BUILD_DIR%\args.gn
if "%PDFium_V8%"=="enabled" echo pdf_enable_xfa=true >> %PDFium_BUILD_DIR%\args.gn

: Generate Ninja files
call gn gen %PDFium_BUILD_DIR% || exit /b

: Build
call ninja -C %PDFium_BUILD_DIR% pdfium || exit /b

: Install
echo on
copy %PDFium_CMAKE_CONFIG% %PDFium_STAGING_DIR% || exit /b
copy %PDFium_SOURCE_DIR%\LICENSE %PDFium_STAGING_DIR% || exit /b
xcopy /S /Y %PDFium_SOURCE_DIR%\public %PDFium_INCLUDE_DIR%\ || exit /b
del %PDFium_INCLUDE_DIR%\DEPS
del %PDFium_INCLUDE_DIR%\README
del %PDFium_INCLUDE_DIR%\PRESUBMIT.py
move %PDFium_BUILD_DIR%\pdfium.dll.lib %PDFium_LIB_DIR% || exit /b
move %PDFium_BUILD_DIR%\pdfium.dll %PDFium_BIN_DIR% || exit /b
if "%CONFIGURATION%"=="Debug" move %PDFium_BUILD_DIR%\pdfium.dll.pdb %PDFium_BIN_DIR%
if "%PDFium_V8%"=="enabled" (
    mkdir %PDFium_RES_DIR%
    move %PDFium_BUILD_DIR%\icudtl.dat %PDFium_RES_DIR%
    move %PDFium_BUILD_DIR%\snapshot_blob.bin %PDFium_RES_DIR%
)

: Pack
cd %PDFium_STAGING_DIR%
call 7z a %PDFium_ARTIFACT% *
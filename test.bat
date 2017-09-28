set SOURCE_DIR=%CD%\example
set PDFium_DIR=%CD%\staging
set CMAKE_GENERATOR=Visual Studio 15 2017

if "%PLATFORM%"=="x64" set CMAKE_GENERATOR=%CMAKE_GENERATOR% Win64

@echo on

mkdir build
cd build
cmake -G "%CMAKE_GENERATOR%" "%SOURCE_DIR%"
cmake --build .
ctest --output-on-failure .
set SOURCE_DIR=%CD%\example
set PDFium_DIR=%CD%\staging
set CMAKE_GENERATOR=Visual Studio 16 2019

if "%PLATFORM%"=="x64" (set CMAKE_GENERATOR_PLATFORM=x64)
else (set CMAKE_GENERATOR_PLATFORM=Win32)

@echo on

mkdir build
cd build
cmake -G "%CMAKE_GENERATOR%" -A "%CMAKE_GENERATOR_PLATFORM%" "%SOURCE_DIR%"
cmake --build .
ctest --output-on-failure .
# PDFium Package Configuration for CMake
#
# To use PDFium in you CMake project:
#
#   1. set the environment variable PDFium_DIR to the folder containing this file.
#   2. in your CMakeLists.txt, add
#       find_package(PDFium)
#   3. then link you excecutable with PDFium
#       target_link_libraries(my_exe pdfium)

if(NOT MSVC)
  message(ERROR "Only Visual Studio is supported")
endif()

if(CMAKE_CL_64)
    set(PDFium_ARCH x64)
else()
    set(PDFium_ARCH x86)
endif()

if(NOT PDFium_FIND_QUIETLY)
  message(STATUS "PDFium ARCH: ${PDFium_ARCH}")
endif()

add_library(pdfium STATIC IMPORTED)
set_target_properties(pdfium
  PROPERTIES
  IMPORTED_LOCATION "C:\\Libraries\\pdfium\\${PDFium_ARCH}\\vs15\\lib\\pdfium.lib"
  IMPORTED_LOCATION_DEBUG "C:\\Libraries\\pdfium\\${PDFium_ARCH}\\vs15\\lib\\pdfiumd.lib"
  INTERFACE_INCLUDE_DIRECTORIES "C:\\Libraries\\pdfium\\include"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "-ignore:4099"
)

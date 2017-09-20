# Pre-compiled binaries of PDFium

This project hosts pre-compiled binaries of the [PDFium library](https://pdfium.googlesource.com/pdfium/).

See [Releases page](https://github.com/bblanchon/pdfium-binaries/releases) to download binaries.

### How too use in a CMake project

1. Unzip one of more variants in a folder (eg `C:\Libraries\pdfium`)
2. Set the environment variable `PDFium_DIR` to this folder (eg `C:\Libraries\pdfium`)
3. In your CMakeLists.txt, add

      find_package(PDFium)

4. Then link you excecutable with PDFium

      target_link_libraries(my_exe pdfium)
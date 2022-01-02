[![Total number of downloads](https://img.shields.io/github/downloads/bblanchon/pdfium-binaries/total.svg)](https://github.com/bblanchon/pdfium-binaries/releases)
[![Build](https://github.com/bblanchon/pdfium-binaries/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/bblanchon/pdfium-binaries/actions/workflows/build.yml)

# Pre-compiled binaries of PDFium

This project hosts pre-compiled binaries of the [PDFium library](https://pdfium.googlesource.com/pdfium/).

See [Releases page](https://github.com/bblanchon/pdfium-binaries/releases) to download binaries.

### Download links

Here are the download links for latest release:

<table>
  <tr>
    <th>OS</th>
    <th>CPU</th>
    <th>PDFium</th>
  </tr>

  <tr>
    <td rowspan="4">Android</td>
    <td>ARM</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-arm.tgz">pdfium-android-arm.tgz</a></td>
  </tr>
  <tr>
    <td>ARM64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-arm64.tgz">pdfium-android-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-x64.tgz">pdfium-android-x64.tgz</a></td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-x86.tgz">pdfium-android-x86.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="2">iOS</td>
    <td>ARM64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-arm64.tgz">pdfium-ios-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-x64.tgz">pdfium-ios-x64.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="3">Linux</td>
    <td>ARM</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-arm.tgz">pdfium-linux-arm.tgz</a></td>
  </tr>
  <tr>
    <td>ARM64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-arm64.tgz">pdfium-linux-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-x64.tgz">pdfium-linux-x64.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="2">macOS</td>
    <td>ARM64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-mac-arm64.tgz">pdfium-mac-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-mac-x64.tgz">pdfium-mac-x64.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="3">Windows</td>
    <td>ARM64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-arm64.tgz">pdfium-win-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x64.tgz">pdfium-win-x64.tgz</a></td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x86.tgz">pdfium-win-x86.tgz</a></td>
  </tr>
</table>

### How to use PDFium in a CMake project

1. Unzip one of more variants in a folder (eg `C:\Libraries\pdfium`)
2. Set the environment variable `PDFium_DIR` to this folder (eg `C:\Libraries\pdfium`)
3. In your `CMakeLists.txt`, add

        find_package(PDFium)

4. Then link you executable with PDFium:

        target_link_libraries(my_exe pdfium)

5. On Windows, make sure that `pdfium.dll` can be found by your executable.

### How to use JavaScript V8 enabled binaries

If you are using the V8 enabled binaries additional initialization is required.
In your code before using PDFium you have to call `FPDF_InitEmbeddedLibraries()`
from the additional header `fpdf_libs.h` which is only available in V8 enabled
binaries.

The archive will contain a `res` folder which you have to distribute with your
application. On macOS you should include this in your application bundle for other
platforms place it where your application binary will find it.

See the following example for usage:

        #include "fpdf_libs.h"

        ...

        // Determine the path to files in the res folder from the archive
        const char* resPath = "<path to the res folder>";

        // Initialize V8 and other embedded libraries
        FPDF_InitEmbeddedLibraries(resPath);

        // Make use of PDFium
        FPDF_InitLibrary();
        FPDF_DestroyLibrary();

### How to create macOS universal binary

To  create a universial macOS binary containing both Intel and ARM code download
both CPU versions and use the `mac_create_universal.sh` script to create a
universal archive.


---

This project isn't affilated with Google nor Foxit.
[![Total number of downloads](https://img.shields.io/github/downloads/bblanchon/pdfium-binaries/total.svg)](https://github.com/bblanchon/pdfium-binaries/releases)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/85rqdhpyxt589700?svg=true)](https://ci.appveyor.com/project/bblanchon/pdfium-binaries)
[![Linux/macOS Build Status](https://travis-ci.org/bblanchon/pdfium-binaries.svg?branch=master)](https://travis-ci.org/bblanchon/pdfium-binaries)

# Pre-compiled binaries of PDFium

This project hosts pre-compiled binaries of the [PDFium library](https://pdfium.googlesource.com/pdfium/).

See [Releases page](https://github.com/bblanchon/pdfium-binaries/releases) to download binaries.

### Download links

Here are the download links for latest release:

<table>
  <tr>
    <th>Platform</th>
    <th>PDFium</th>
    <th>PDFium with <a href="https://en.wikipedia.org/wiki/V8_(JavaScript_engine)">V8</a> and <a href="https://en.wikipedia.org/wiki/XFA">XFA</a></th>
  </tr>
  <tr>
    <td>Windows 32-bit</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-windows-x86.zip">pdfium-windows-x86.zip</a> (2 MB)</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-windows-x86-v8.zip">pdfium-windows-x86-v8.zip</a> (12 MB)</td>
  </tr>
  <tr>
    <td>Windows 64-bit</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-windows-x64.zip">pdfium-windows-x64.zip</a> (2 MB)</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-windows-x64-v8.zip">pdfium-windows-x64-v8.zip</a> (12 MB)</td>
  </tr>
  <tr>
    <td>Linux</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux.tgz">pdfium-linux.tgz</a> (6 MB)</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-v8.tgz">pdfium-linux-v8.tgz</a> (37 MB)</td>
  </tr>
  <tr>
    <td>macOS</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-darwin.tgz">pdfium-darwin.tgz</a> (6 MB)</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-darwin-v8.tgz">pdfium-darwin-v8.tgz</a> (42 MB)</td>
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



---

This project isn't affilated with Google nor Foxit.

We are very thankful to [AppVeyor](https://www.appveyor.com/) and [Travis CI](https://travis-ci.org/) for extending the build time limit, which allows us to build PDFium with V8.
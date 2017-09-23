[![Total number of downloads](https://img.shields.io/github/downloads/bblanchon/pdfium-binaries/total.svg)](https://github.com/bblanchon/pdfium-binaries/releases)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/85rqdhpyxt589700?svg=true)](https://ci.appveyor.com/project/bblanchon/pdfium-binaries)

# Pre-compiled binaries of PDFium

This project hosts pre-compiled binaries of the [PDFium library](https://pdfium.googlesource.com/pdfium/).

See [Releases page](https://github.com/bblanchon/pdfium-binaries/releases) to download binaries.

### Download links

Here are the download links for latest release:

<table>
  <tr>
    <th>Plateform</th>
    <th>Release build</th>
    <th>Debug build</th>
  </tr>
  <tr>
    <td>Windows 32-bit</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-windows-x86.zip</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-windows-x86-debug.zip</a></td>
  </tr>
  <tr>
    <td>Windows 64-bit</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-windows-x64.zip</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-windows-x64-debug.zip</a></td>
  </tr>
  <tr>
    <td>Linux</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-linux.zip</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest">pdfium-linux-debug.zip</a></td>
  </tr>
</table>

### How too use in a CMake project

1. Unzip one of more variants in a folder (eg `C:\Libraries\pdfium`)
2. Set the environment variable `PDFium_DIR` to this folder (eg `C:\Libraries\pdfium`)
3. In your CMakeLists.txt, add

        find_package(PDFium)

4. Then link you excecutable with PDFium

        target_link_libraries(my_exe pdfium)

5. On Windows, make sure that `pdfium.dll` can be found by your executable.

---

This project isn't affilated with Google nor Foxit.
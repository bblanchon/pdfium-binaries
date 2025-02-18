<p align="center">
  <img alt="PDFium binaries" src=".github/images/header.svg" />
</p>

# Pre-compiled binaries of PDFium

[![Patches](https://github.com/bblanchon/pdfium-binaries/actions/workflows/patch.yml/badge.svg?branch=master)](https://github.com/bblanchon/pdfium-binaries/actions/workflows/patch.yml)
[![Total downloads](https://img.shields.io/github/downloads/bblanchon/pdfium-binaries/total)](https://github.com/bblanchon/pdfium-binaries/releases/)

[![Latest release](https://img.shields.io/github/v/release/bblanchon/pdfium-binaries?display_name=release&label=github)](https://github.com/bblanchon/pdfium-binaries/releases/latest/)
[![Nuget](https://img.shields.io/nuget/v/bblanchon.PDFium)](https://www.nuget.org/packages/bblanchon.PDFium/)
[![Conda](https://img.shields.io/conda/v/bblanchon/pdfium-binaries?label=conda)](https://anaconda.org/bblanchon/pdfium-binaries)


This project hosts pre-compiled binaries of the [PDFium library](https://pdfium.googlesource.com/pdfium/), an open-source library for PDF manipulation and rendering.

Builds have been triggered automatically every Monday since 2017.

**Disclaimer**: This project isn't affiliated with Google or Foxit.

## Download

Here are the download links for latest release:

<table>
  <tr>
    <th>OS</th>
    <th>Env</th>
    <th>CPU</th>
    <th>PDFium</th>
    <th>PDFium V8</th>
  </tr>

  <tr>
    <td rowspan="4" colspan=2>Android</td>
    <td>arm</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-arm.tgz">pdfium-android-arm.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-android-arm.tgz">pdfium-v8-android-arm.tgz</a></td>
  </tr>
  <tr>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-arm64.tgz">pdfium-android-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-android-arm64.tgz">pdfium-v8-android-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-x64.tgz">pdfium-android-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-android-x64.tgz">pdfium-v8-android-x64.tgz</a></td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-android-x86.tgz">pdfium-android-x86.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-android-x86.tgz">pdfium-v8-android-x86.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="5">iOS</td>
    <td rowspan="2">catalyst</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-catalyst-arm64.tgz">pdfium-ios-catalyst-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-ios-catalyst-arm64.tgz">pdfium-v8-ios-catalyst-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-catalyst-x64.tgz">pdfium-ios-catalyst-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-ios-catalyst-x64.tgz">pdfium-v8-ios-catalyst-x64.tgz</a></td>
  </tr>

  <tr>
    <td>device</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-device-arm64.tgz">pdfium-ios-device-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-ios-device-arm64.tgz">pdfium-v8-ios-device-arm64.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="2">simulator</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-simulator-arm64.tgz">pdfium-ios-simulator-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-ios-simulator-arm64.tgz">pdfium-v8-ios-simulator-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-ios-simulator-x64.tgz">pdfium-ios-simulator-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-ios-simulator-x64.tgz">pdfium-v8-ios-simulator-x64.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="7">Linux</td>
    <td rowspan="4">glibc</td>
    <td>arm</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-arm.tgz">pdfium-linux-arm.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-linux-arm.tgz">pdfium-v8-linux-arm.tgz</a></td>
  </tr>
  <tr>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-arm64.tgz">pdfium-linux-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-linux-arm64.tgz">pdfium-v8-linux-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-x64.tgz">pdfium-linux-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-linux-x64.tgz">pdfium-v8-linux-x64.tgz</a></td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-x86.tgz">pdfium-linux-x86.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-linux-x86.tgz">pdfium-v8-linux-x86.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="3">musl</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-musl-arm64.tgz">pdfium-linux-musl-arm64.tgz</a></td>
    <td>failing (#192)</td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-musl-x64.tgz">pdfium-linux-musl-x64.tgz</a></td>
    <td>failing (#191)</td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-musl-x86.tgz">pdfium-linux-musl-x86.tgz</a></td>
    <td>failing (#193)</td>
  </tr>

  <tr>
    <td rowspan="3" colspan="2">macOS</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-mac-arm64.tgz">pdfium-mac-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-mac-arm64.tgz">pdfium-v8-mac-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-mac-x64.tgz">pdfium-mac-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-mac-x64.tgz">pdfium-v8-mac-x64.tgz</a></td>
  </tr>
  <tr>
    <td>univ</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-mac-univ.tgz">pdfium-mac-univ.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-mac-univ.tgz">pdfium-v8-mac-univ.tgz</a></td>
  </tr>

  <tr>
    <td rowspan="3" colspan="2">Windows</td>
    <td>arm64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-arm64.tgz">pdfium-win-arm64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-win-arm64.tgz">pdfium-v8-win-arm64.tgz</a></td>
  </tr>
  <tr>
    <td>x64</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x64.tgz">pdfium-win-x64.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-win-x64.tgz">pdfium-v8-win-x64.tgz</a></td>
  </tr>
  <tr>
    <td>x86</td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x86.tgz">pdfium-win-x86.tgz</a></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-v8-win-x86.tgz">pdfium-v8-win-x86.tgz</a></td>
  </tr>

  <tr>
    <td colspan="3">WebAssembly<sup>1</sup></td>
    <td><a href="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-wasm.tgz">pdfium-wasm.tgz</a></td>
    <td>not supported</td>
  </tr>
</table>

<small>1: WebAssembly build is experimental; please [provide feedback](https://github.com/bblanchon/pdfium-binaries/issues/28).</small>

See the [Releases page](https://github.com/bblanchon/pdfium-binaries/releases) to download older versions of PDFium.

### NuGet Packages

The following NuGet packages are available:

<table>
  <tr>
    <th>OS</th>
    <th>PDFium</th>
    <th>PDFium V8</th>
  </tr>

  <tr>
    <td>All (meta package)</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium/">bblanchon.PDFium</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8/">bblanchon.PDFiumV8</a></td>
  </tr>

  <tr>
    <td>Android</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.Android/">bblanchon.PDFium.Android</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8.Android/">bblanchon.PDFiumV8.Android</a></td>
  </tr>

  <tr>
    <td>iOS</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.iOS/">bblanchon.PDFium.iOS</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8.iOS/">bblanchon.PDFiumV8.iOS</a></td>
  </tr>

  <tr>
    <td>Linux</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.Linux/">bblanchon.PDFium.Linux</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8.Linux/">bblanchon.PDFiumV8.Linux</a></td>
  </tr>

  <tr>
    <td>macOS</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.macOS/">bblanchon.PDFium.macOS</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8.macOS/">bblanchon.PDFiumV8.macOS</a></td>
  </tr>

  <tr>
    <td>Windows</td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.Win32/">bblanchon.PDFium.Win32</a></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFiumV8.Win32/">bblanchon.PDFiumV8.Win32</a></td>
  </tr>

  <tr>
    <td>WebAssembly<sup>1</sup></td>
    <td><a href="https://www.nuget.org/packages/bblanchon.PDFium.WebAssembly/">bblanchon.PDFium.WebAssembly</a></td>
    <td>not supported</td>
  </tr>
</table>

<small>1: WebAssembly build is experimental; please [provide feedback](https://github.com/bblanchon/pdfium-binaries/issues/28).</small>

**HELP WANTED!**  
I can provide packages for your favorite package manager, but I need help from someone who knows the format. Contact me via [GitHub issues](https://github.com/bblanchon/pdfium-binaries/issues) if you want to help.

## Documentation

### PDFium API documentation

Please find the [documentation of the PDFium API on developers.foxit.com](https://developers.foxit.com/resources/pdf-sdk/c_api_reference_pdfium/index.html).

### How to use PDFium in a CMake project

1. Unzip the downloaded package in a folder (e.g., `C:\Libraries\pdfium`)
2. Set the environment variable `PDFium_DIR` to this folder (e.g., `C:\Libraries\pdfium`)
3. In your `CMakeLists.txt`, add

        find_package(PDFium)

4. Then link your executable with PDFium:

        target_link_libraries(my_exe pdfium)

5. On Windows, make sure that `pdfium.dll` can be found by your executable (copy it on the same folder, or put it on the `PATH`).


## Related projects

The following projects use (or recommend using) our PDFium builds:

| Name                     | Language | Description                                                                                                 |
|:-------------------------|:---------|:------------------------------------------------------------------------------------------------------------|
| [dart_pdf][dart_pdf]     | Dart     | PDF creation module for dart/flutter                                                                        |
| [DtronixPdf][dtronixpdf] | C#       | PDF viewer and editor toolset                                                                               |
| [go-pdfium][go-pdfium]   | Go       | Go wrapper around PDFium with helper functions for various methods like image rendering and text extraction |
| [libvips][libvips]       | C        | A performant image processing library                                                                       |
| [PDFium RS][pdfium_rs]   | Rust     | Rust wrapper around PDFium                                                                                  |
| [pdfium-vfp][pdfium-vfp] | VFP      | PDF Viewer component for Visual FoxPro                                                                      |
| [PDFiumCore][pdfiumcore] | C#       | .NET Standard P/Invoke bindings for PDFium                                                                  |
| [PdfiumLib][pdfiumlib]   | Pascal   | An interface to libpdfium for Delphi                                                                        |
| [PdfLibCore][pdflibcore] | C#       | A fast PDF editing and reading library for modern .NET Core applications                                    |
| [PDFtoImage][pdftoimage] | C#       | .NET library to render PDF content into images                                                              |
| [PDFtoZPL][pdftozpl]     | C#       | A .NET library to convert PDF files (and bitmaps) into Zebra Programming Language code                      |
| [PDFx][pdfx]             | Dart     | Flutter Render & show PDF documents on Web, MacOs 10.11+, Android 5.0+, iOS and Windows                     |
| [PyPDFium2][pypdfium2]   | Python   | Python bindings to PDFium                                                                                   |
| [Spacedrive][spacedrive] | Rust/TS  | Cross-platform file manager, powered by a virtual distributed filesystem                                    |
| [wxPDFView][wxpdfview]   | C++      | wxWidgets components to display PDF content                                                                 |

*Did we miss a project? Please open a PR!*  


## Contributors

<table>
  <thead>
    <tr>
      <th></th>
      <th>Username</th>
      <th>Contributions</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><img src="https://avatars.githubusercontent.com/u/5462433?v=4" width="48" height="48" alt="Benoit Blanchon"></td>
      <td><a href="https://github.com/bblanchon"><code>@bblanchon</code></a></td>
      <td>Main contributor</td>
    </tr>
    <tr>
      <td><img src="https://avatars.githubusercontent.com/u/248162?v=4" width="48" height="48" alt="Christoffer Green"></td>
      <td><a href="https://github.com/ChristofferGreen"><code>@ChristofferGreen</code></a></td>
      <td>Linux ARM build</td>
    </tr>
    <tr>
      <td rowspan="2"><img src="https://avatars.githubusercontent.com/u/1312921?v=4" width="48" height="48" alt="Jeroen Bobbeldijk"></td>
      <td rowspan="2"><a href="https://github.com/jerbob92"><code>@jerbob92</code></a></td>
      <td>Musl build</td>
    </tr>
    <tr>
      <td>WebAssembly build</td>
    </tr>
    <tr>
      <td rowspan="2"><img src="https://avatars.githubusercontent.com/u/65915611?v=4" width="48" height="48" alt="mara004"></td>
      <td rowspan="2"><a href="https://github.com/mara004"><code>@mara004</code></a></td>
      <td>Conda packages</td>
    </tr>
    <tr>
      <td>Frequent help with many aspects of the project</td>
    </tr>
    <tr>
      <td><img src="https://avatars.githubusercontent.com/u/12021771?v=4" width="48" height="48" alt="David Sungaila"></td>
      <td><a href="https://github.com/sungaila"><code>@sungaila</code></a></td>
      <td>NuGet packages</td>
    </tr>
    <tr>
      <td rowspan="2"><img src="https://avatars.githubusercontent.com/u/5075894?v=4" width="48" height="48" alt="Tobias Taschner"></td>
      <td rowspan="2"><a href="https://github.com/TcT2k"><code>@TcT2k</code></a></td>
      <td>macOS build</td>
    </tr>
    <tr>
      <td>V8 build</td>
    </tr>
  </tbody>
</table

[pdfium-vfp]:
https://github.com/dmitriychunikhin/pdfium-vfp
[dart_pdf]: https://github.com/DavBfr/dart_pdf
[pdfx]: https://github.com/scerio/packages.flutter/tree/main/packages/pdfx
[go-pdfium]: https://github.com/klippa-app/go-pdfium
[pdfium_rs]: https://github.com/asafigan/pdfium_rs
[pdfiumcore]: https://github.com/Dtronix/PDFiumCore
[pdftoimage]: https://github.com/sungaila/PDFtoImage
[pypdfium2]: https://github.com/pypdfium2-team/pypdfium2
[wxpdfview]: https://github.com/TcT2k/wxPDFView
[libvips]: https://github.com/libvips/libvips
[pdfiumlib]: https://github.com/ahausladen/PdfiumLib
[pdflibcore]: https://github.com/jbaarssen/PdfLibCore
[dtronixpdf]: https://github.com/Dtronix/DtronixPdf
[pdftozpl]: https://github.com/sungaila/PDFtoZPL
[spacedrive]: https://github.com/spacedriveapp/spacedrive

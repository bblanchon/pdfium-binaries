# Running PDFium's test suite under WebAssembly

Upstream PDFium only runs its test suite natively; the wasm-standalone build
normally links `libpdfium.a` into a module by hand (`steps/06-build.sh`).
`testsuite_emscripten.patch` makes the gtest binaries buildable with the GN
emscripten toolchain and runnable under node:

- `build/toolchain/wasm/BUILD.gn`: link flags for test executables
  (NODERAWFS for real-filesystem access, memory growth, SUPPORT_LONGJMP,
  ERROR_ON_UNDEFINED_SYMBOLS=0 for the `*_CB` host-callback trampolines,
  which tests never call).
- `testing/gtest/include/gtest/gtest.h`: maps EXPECT_DEATH/ASSERT_DEATH to
  the IF_SUPPORTED variants (emscripten has no fork(), so googletest
  disables death tests and does not define the plain macros).
- `testing/utils/path_service.cpp`: an emscripten branch (no
  `linux/limits.h`, no `/proc/self/exe`; the executable dir is taken from
  the working directory, which must be two levels below the source root).

## Usage

```sh
cd pdfium
git apply ../patches/wasm/testsuite_emscripten.patch
ninja -C out pdfium_unittests pdfium_embeddertests
mkdir -p out/run && ln -sfn ../test_fonts out/run/test_fonts
cd out/run
../../third_party/emsdk/node/24.19.0_64bit/bin/node ../pdfium_unittests.js
../../third_party/emsdk/node/24.19.0_64bit/bin/node ../pdfium_embeddertests.js
```

## Known wasm-environment failures (identical with and without our patches)

Verified 2026-08-19 against pdfium with the full wasm patch set (SIMD
kernels enabled) AND against a clean unpatched baseline: both configurations
fail the exact same tests, so none of the failures are caused by the
patches.

- `pdfium_unittests`: 1031/1033 pass. `WideString.FormatString` (musl
  vswprintf rejects non-ASCII wide chars in the C locale) and
  `RetainPtr.SetContains` (emsdk libc++ makes one extra std::set copy).
- `pdfium_embeddertests`: 761/852 pass. The 91 failures are text-metric and
  text-AA pixel comparisons against expectations generated on native Linux
  builds (glibc vs musl libm, libc++ differences shift glyph coordinates by
  ~0.1pt and anti-aliased pixels accordingly), plus tests derived from them
  (font subsetting, form text, saved-output hashes).

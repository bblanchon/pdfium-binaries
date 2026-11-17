/*
 * jdcolor-wasm.c - colorspace conversion (WebAssembly SIMD128)
 *
 * Driver for the YCbCr -> RGB kernel in jdcolext-wasm.c, instantiated once
 * per output pixel layout, following the structure of
 * simd/arm/jdcolor-neon.c. The kernels are bit-identical to libjpeg-turbo's
 * scalar C implementation (same fixed-point constants and rounding), unlike
 * the NEON/SSE2 kernels.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

/* Same fixed-point scheme as src/jdcolor.c. */
#define SCALEBITS_WASM  16
#define ONE_HALF_WASM  ((int32_t)1 << (SCALEBITS_WASM - 1))
#define FIX_WASM(x)  ((int32_t)((x) * (1L << SCALEBITS_WASM) + 0.5))

/* Plain JCS_RGB. */
#define RGB_RED  0
#define RGB_GREEN  1
#define RGB_BLUE  2
#define RGB_PIXELSIZE  3
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_rgb_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_RGB_RED
#define RGB_GREEN  EXT_RGB_GREEN
#define RGB_BLUE  EXT_RGB_BLUE
#define RGB_PIXELSIZE  EXT_RGB_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extrgb_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_RGBX_RED
#define RGB_GREEN  EXT_RGBX_GREEN
#define RGB_BLUE  EXT_RGBX_BLUE
#define RGB_ALPHA  3
#define RGB_PIXELSIZE  EXT_RGBX_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extrgbx_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_BGR_RED
#define RGB_GREEN  EXT_BGR_GREEN
#define RGB_BLUE  EXT_BGR_BLUE
#define RGB_PIXELSIZE  EXT_BGR_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extbgr_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_BGRX_RED
#define RGB_GREEN  EXT_BGRX_GREEN
#define RGB_BLUE  EXT_BGRX_BLUE
#define RGB_ALPHA  3
#define RGB_PIXELSIZE  EXT_BGRX_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extbgrx_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_XBGR_RED
#define RGB_GREEN  EXT_XBGR_GREEN
#define RGB_BLUE  EXT_XBGR_BLUE
#define RGB_ALPHA  0
#define RGB_PIXELSIZE  EXT_XBGR_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extxbgr_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

#define RGB_RED  EXT_XRGB_RED
#define RGB_GREEN  EXT_XRGB_GREEN
#define RGB_BLUE  EXT_XRGB_BLUE
#define RGB_ALPHA  0
#define RGB_PIXELSIZE  EXT_XRGB_PIXELSIZE
#define jsimd_ycc_rgb_convert_wasm_internal jsimd_ycc_extxrgb_convert_wasm
#include "jdcolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_ALPHA
#undef RGB_PIXELSIZE
#undef jsimd_ycc_rgb_convert_wasm_internal

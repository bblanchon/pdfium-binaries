/*
 * jccolor-wasm.c - colorspace conversion for compression (WebAssembly SIMD128)
 *
 * Driver for the RGB -> YCbCr kernel in jccolext-wasm.c, instantiated once
 * per input pixel layout, following the structure of simd/arm/jccolor-neon.c
 * and using the same 2^-16 fixed-point constants.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

/* RGB -> YCbCr conversion constants (identical to jccolor-neon.c) */
#define F_0_298  19595
#define F_0_587  38470
#define F_0_113  7471
#define F_0_168  11059
#define F_0_331  21709
#define F_0_500  32768
#define F_0_418  27439
#define F_0_081  5329

/* vld4q_u8: deinterleave 64 bytes into 4 planes of 16. */
static inline void wasm_load_deinterleave_4(const uint8_t *p, v128_t out[4])
{
  v128_t v0 = wasm_v128_load(p);
  v128_t v1 = wasm_v128_load(p + 16);
  v128_t v2 = wasm_v128_load(p + 32);
  v128_t v3 = wasm_v128_load(p + 48);
  /* Gather even/odd channel pairs from each 32-byte half... */
  v128_t p01_02 = wasm_i8x16_shuffle(v0, v1, 0, 4, 8, 12, 16, 20, 24, 28, 1,
                                     5, 9, 13, 17, 21, 25, 29);
  v128_t p01_13 = wasm_i8x16_shuffle(v0, v1, 2, 6, 10, 14, 18, 22, 26, 30, 3,
                                     7, 11, 15, 19, 23, 27, 31);
  v128_t p23_02 = wasm_i8x16_shuffle(v2, v3, 0, 4, 8, 12, 16, 20, 24, 28, 1,
                                     5, 9, 13, 17, 21, 25, 29);
  v128_t p23_13 = wasm_i8x16_shuffle(v2, v3, 2, 6, 10, 14, 18, 22, 26, 30, 3,
                                     7, 11, 15, 19, 23, 27, 31);
  /* ...then combine the halves per channel. */
  out[0] = wasm_i8x16_shuffle(p01_02, p23_02, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17,
                              18, 19, 20, 21, 22, 23);
  out[1] = wasm_i8x16_shuffle(p01_02, p23_02, 8, 9, 10, 11, 12, 13, 14, 15,
                              24, 25, 26, 27, 28, 29, 30, 31);
  out[2] = wasm_i8x16_shuffle(p01_13, p23_13, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17,
                              18, 19, 20, 21, 22, 23);
  out[3] = wasm_i8x16_shuffle(p01_13, p23_13, 8, 9, 10, 11, 12, 13, 14, 15,
                              24, 25, 26, 27, 28, 29, 30, 31);
}

/* vld3q_u8: deinterleave 48 bytes into 3 planes of 16. */
static inline void wasm_load_deinterleave_3(const uint8_t *p, v128_t out[3])
{
  v128_t v0 = wasm_v128_load(p);       /* bytes 0..15 */
  v128_t v1 = wasm_v128_load(p + 16);  /* bytes 16..31 */
  v128_t v2 = wasm_v128_load(p + 32);  /* bytes 32..47 */
  /* Channel 0: bytes 0,3,..,45 - 11 from v0/v1, 5 from v2. */
  v128_t c0a = wasm_i8x16_shuffle(v0, v1, 0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
                                  30, 0, 0, 0, 0, 0);
  out[0] = wasm_i8x16_shuffle(c0a, v2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17,
                              20, 23, 26, 29);
  /* Channel 1: bytes 1,4,..,46. */
  v128_t c1a = wasm_i8x16_shuffle(v0, v1, 1, 4, 7, 10, 13, 16, 19, 22, 25,
                                  28, 31, 0, 0, 0, 0, 0);
  out[1] = wasm_i8x16_shuffle(c1a, v2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 18,
                              21, 24, 27, 30);
  /* Channel 2: bytes 2,5,..,47 - 10 from v0/v1, 6 from v2. */
  v128_t c2a = wasm_i8x16_shuffle(v0, v1, 2, 5, 8, 11, 14, 17, 20, 23, 26,
                                  29, 0, 0, 0, 0, 0, 0);
  out[2] = wasm_i8x16_shuffle(c2a, v2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 19,
                              22, 25, 28, 31);
}

/* Plain JCS_RGB. */
#define RGB_RED  0
#define RGB_GREEN  1
#define RGB_BLUE  2
#define RGB_PIXELSIZE  3
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_rgb_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_RGB_RED
#define RGB_GREEN  EXT_RGB_GREEN
#define RGB_BLUE  EXT_RGB_BLUE
#define RGB_PIXELSIZE  EXT_RGB_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extrgb_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_RGBX_RED
#define RGB_GREEN  EXT_RGBX_GREEN
#define RGB_BLUE  EXT_RGBX_BLUE
#define RGB_PIXELSIZE  EXT_RGBX_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extrgbx_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_BGR_RED
#define RGB_GREEN  EXT_BGR_GREEN
#define RGB_BLUE  EXT_BGR_BLUE
#define RGB_PIXELSIZE  EXT_BGR_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extbgr_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_BGRX_RED
#define RGB_GREEN  EXT_BGRX_GREEN
#define RGB_BLUE  EXT_BGRX_BLUE
#define RGB_PIXELSIZE  EXT_BGRX_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extbgrx_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_XBGR_RED
#define RGB_GREEN  EXT_XBGR_GREEN
#define RGB_BLUE  EXT_XBGR_BLUE
#define RGB_PIXELSIZE  EXT_XBGR_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extxbgr_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

#define RGB_RED  EXT_XRGB_RED
#define RGB_GREEN  EXT_XRGB_GREEN
#define RGB_BLUE  EXT_XRGB_BLUE
#define RGB_PIXELSIZE  EXT_XRGB_PIXELSIZE
#define jsimd_rgb_ycc_convert_wasm_internal jsimd_extxrgb_ycc_convert_wasm
#include "jccolext-wasm.c"
#undef RGB_RED
#undef RGB_GREEN
#undef RGB_BLUE
#undef RGB_PIXELSIZE
#undef jsimd_rgb_ycc_convert_wasm_internal

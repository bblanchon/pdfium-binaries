/*
 * jcsample-wasm.c - downsampling (WebAssembly SIMD128)
 *
 * Transliteration of jsimd_h2v1_downsample_neon() and
 * jsimd_h2v2_downsample_neon() from simd/arm/jcsample-neon.c, producing the
 * same output as the scalar h2v1_downsample()/h2v2_downsample() in
 * jcsample.c: pairwise adds with the alternating bias pattern, then a
 * truncating shift. The last-block padding table is reused verbatim;
 * vqtbl1q_u8 maps directly onto wasm_i8x16_swizzle.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

static const uint8_t jsimd_h2_downsample_consts[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 0 */
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 1 */
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0E,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 2 */
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0D, 0x0D,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 3 */
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 4 */
  0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 5 */
  0x08, 0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 6 */
  0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 7 */
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   /* Pad 8 */
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06,   /* Pad 9 */
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x05,   /* Pad 10 */
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04, 0x04,   /* Pad 11 */
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x00, 0x01, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03,   /* Pad 12 */
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,   /* Pad 13 */
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,   /* Pad 14 */
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* Pad 15 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* vpadalq_u8: pairwise widening add of u8 pairs, accumulated into u16. */
#define PADAL_U8(acc, v) \
  wasm_i16x8_add(acc, wasm_u16x8_extadd_pairwise_u8x16(v))

/* vshrn_n_u16 + vst1_u8: shift, narrow to u8 (values fit), store 8 bytes. */
static inline void shrn_store8(JSAMPLE *out, v128_t samples_u16, int shift)
{
  v128_t narrowed = wasm_u16x8_shr(samples_u16, shift);
  narrowed = wasm_i8x16_shuffle(narrowed, narrowed, 0, 2, 4, 6, 8, 10, 12,
                                14, 0, 2, 4, 6, 8, 10, 12, 14);
  wasm_v128_store64_lane(out, narrowed, 0);
}

/* 2:1 horizontal, 1:1 vertical downsampling, without smoothing. */

void jsimd_h2v1_downsample_wasm(JDIMENSION image_width, int max_v_samp_factor,
                                JDIMENSION v_samp_factor,
                                JDIMENSION width_in_blocks,
                                JSAMPARRAY input_data, JSAMPARRAY output_data)
{
  JSAMPROW inptr, outptr;
  /* Expansion mask to pad remaining elements of the last DCT block. */
  const int mask_offset = 16 * ((width_in_blocks * 2 * DCTSIZE) - image_width);
  const v128_t expand_mask =
    wasm_v128_load(&jsimd_h2_downsample_consts[mask_offset]);
  /* Bias pattern, alternating every pixel: { 0, 1, 0, 1, 0, 1, 0, 1 } */
  const v128_t bias = wasm_i32x4_splat(0x00010000);
  unsigned i, outrow;

  for (outrow = 0; outrow < v_samp_factor; outrow++) {
    outptr = output_data[outrow];
    inptr = input_data[outrow];

    for (i = 0; i < width_in_blocks - 1; i++) {
      v128_t pixels = wasm_v128_load(inptr + i * 2 * DCTSIZE);
      shrn_store8(outptr + i * DCTSIZE, PADAL_U8(bias, pixels), 1);
    }

    /* Last DCT block: pad the empty elements with the last pixel's value. */
    v128_t pixels =
      wasm_v128_load(inptr + (width_in_blocks - 1) * 2 * DCTSIZE);
    pixels = wasm_i8x16_swizzle(pixels, expand_mask);
    shrn_store8(outptr + (width_in_blocks - 1) * DCTSIZE,
                PADAL_U8(bias, pixels), 1);
  }
}

/* 2:1 horizontal, 2:1 vertical downsampling, without smoothing. */

void jsimd_h2v2_downsample_wasm(JDIMENSION image_width, int max_v_samp_factor,
                                JDIMENSION v_samp_factor,
                                JDIMENSION width_in_blocks,
                                JSAMPARRAY input_data, JSAMPARRAY output_data)
{
  JSAMPROW inptr0, inptr1, outptr;
  /* Expansion mask to pad remaining elements of the last DCT block. */
  const int mask_offset = 16 * ((width_in_blocks * 2 * DCTSIZE) - image_width);
  const v128_t expand_mask =
    wasm_v128_load(&jsimd_h2_downsample_consts[mask_offset]);
  /* Bias pattern, alternating every pixel: { 1, 2, 1, 2, 1, 2, 1, 2 } */
  const v128_t bias = wasm_i32x4_splat(0x00020001);
  unsigned i, outrow;

  for (outrow = 0; outrow < v_samp_factor; outrow++) {
    outptr = output_data[outrow];
    inptr0 = input_data[outrow];
    inptr1 = input_data[outrow + 1];

    for (i = 0; i < width_in_blocks - 1; i++) {
      v128_t pixels_r0 = wasm_v128_load(inptr0 + i * 2 * DCTSIZE);
      v128_t pixels_r1 = wasm_v128_load(inptr1 + i * 2 * DCTSIZE);
      v128_t samples_u16 = PADAL_U8(PADAL_U8(bias, pixels_r0), pixels_r1);
      shrn_store8(outptr + i * DCTSIZE, samples_u16, 2);
    }

    /* Last DCT block: pad the empty elements with the last pixel's value. */
    v128_t pixels_r0 =
      wasm_v128_load(inptr0 + (width_in_blocks - 1) * 2 * DCTSIZE);
    v128_t pixels_r1 =
      wasm_v128_load(inptr1 + (width_in_blocks - 1) * 2 * DCTSIZE);
    pixels_r0 = wasm_i8x16_swizzle(pixels_r0, expand_mask);
    pixels_r1 = wasm_i8x16_swizzle(pixels_r1, expand_mask);
    v128_t samples_u16 = PADAL_U8(PADAL_U8(bias, pixels_r0), pixels_r1);
    shrn_store8(outptr + (width_in_blocks - 1) * DCTSIZE, samples_u16, 2);
  }
}

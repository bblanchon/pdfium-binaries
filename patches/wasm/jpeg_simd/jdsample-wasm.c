/*
 * jdsample-wasm.c - fancy chroma upsampling (WebAssembly SIMD128)
 *
 * Transliteration of jsimd_h2v2_fancy_upsample_neon() from
 * simd/arm/jdsample-neon.c, preserving its arithmetic exactly (which in turn
 * matches the scalar h2v2_fancy_upsample() in jdsample.c bit for bit: the
 * 3:1 blends with +7 truncating and +8 rounding shifts).
 *
 * Like the NEON version, the loops process 16 columns at a time with no
 * scalar tail: the sample buffers allocated by jmemmgr.c are padded to a
 * multiple of 32 bytes, so the final iteration may read and write slightly
 * past downsampled_width but stays within the buffers.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

/* widen(a_half) + widen(b_half) * 3, in u16 lanes (max 4 * 255, no wrap) */
#define BLEND3_LOW(a, b, three) \
  wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(a), \
                 wasm_i16x8_mul(wasm_u16x8_extend_low_u8x16(b), three))
#define BLEND3_HIGH(a, b, three) \
  wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(a), \
                 wasm_i16x8_mul(wasm_u16x8_extend_high_u8x16(b), three))

/* a + b * 3 in u16 lanes (max 16 * 255, no wrap) */
#define MLA3(a, b, three)  wasm_i16x8_add(a, wasm_i16x8_mul(b, three))

/* Truncating and rounding narrowing shifts by 4; values are <= 4088 >> 4 =
 * 255, so the saturating narrow is exact. */
#define SHRN4(lo, hi) \
  wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(lo, 4), wasm_u16x8_shr(hi, 4))
#define RSHRN4(lo, hi, eight) \
  wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(wasm_i16x8_add(lo, eight), 4), \
                          wasm_u16x8_shr(wasm_i16x8_add(hi, eight), 4))

/* vst2q_u8: interleave two 16-byte vectors into 32 output bytes. */
static inline void st2_u8(JSAMPLE *out, v128_t p1, v128_t p2)
{
  wasm_v128_store(out, wasm_i8x16_shuffle(p1, p2, 0, 16, 1, 17, 2, 18, 3, 19,
                                          4, 20, 5, 21, 6, 22, 7, 23));
  wasm_v128_store(out + 16,
                  wasm_i8x16_shuffle(p1, p2, 8, 24, 9, 25, 10, 26, 11, 27, 12,
                                     28, 13, 29, 14, 30, 15, 31));
}

void jsimd_h2v2_fancy_upsample_wasm(int max_v_samp_factor,
                                    JDIMENSION downsampled_width,
                                    JSAMPARRAY input_data,
                                    JSAMPARRAY *output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr0, inptr1, inptr2, outptr0, outptr1;
  int inrow, outrow;
  unsigned colctr;

  const v128_t three = wasm_i16x8_splat(3);
  const v128_t seven = wasm_i16x8_splat(7);
  const v128_t eight = wasm_i16x8_splat(8);

  inrow = outrow = 0;
  while (outrow < max_v_samp_factor) {
    inptr0 = input_data[inrow - 1];
    inptr1 = input_data[inrow];
    inptr2 = input_data[inrow + 1];
    outptr0 = output_data[outrow++];
    outptr1 = output_data[outrow++];

    /* First pixel component value in this row of the original image */
    int s0colsum0 = GETJSAMPLE(*inptr1) * 3 + GETJSAMPLE(*inptr0);
    *outptr0 = (JSAMPLE)((s0colsum0 * 4 + 8) >> 4);
    int s0colsum1 = GETJSAMPLE(*inptr1) * 3 + GETJSAMPLE(*inptr2);
    *outptr1 = (JSAMPLE)((s0colsum1 * 4 + 8) >> 4);

    /* Step 1: Blend samples vertically in columns s0 and s1. */
    v128_t s0A = wasm_v128_load(inptr0);
    v128_t s0B = wasm_v128_load(inptr1);
    v128_t s0C = wasm_v128_load(inptr2);
    v128_t s0colsum0_l = BLEND3_LOW(s0A, s0B, three);
    v128_t s0colsum0_h = BLEND3_HIGH(s0A, s0B, three);
    v128_t s0colsum1_l = BLEND3_LOW(s0C, s0B, three);
    v128_t s0colsum1_h = BLEND3_HIGH(s0C, s0B, three);
    v128_t s1A = wasm_v128_load(inptr0 + 1);
    v128_t s1B = wasm_v128_load(inptr1 + 1);
    v128_t s1C = wasm_v128_load(inptr2 + 1);
    v128_t s1colsum0_l = BLEND3_LOW(s1A, s1B, three);
    v128_t s1colsum0_h = BLEND3_HIGH(s1A, s1B, three);
    v128_t s1colsum1_l = BLEND3_LOW(s1C, s1B, three);
    v128_t s1colsum1_h = BLEND3_HIGH(s1C, s1B, three);

    /* Step 2: Blend the already-blended columns. */
    v128_t output0_p1_l = wasm_i16x8_add(MLA3(s1colsum0_l, s0colsum0_l, three), seven);
    v128_t output0_p1_h = wasm_i16x8_add(MLA3(s1colsum0_h, s0colsum0_h, three), seven);
    v128_t output0_p2_l = MLA3(s0colsum0_l, s1colsum0_l, three);
    v128_t output0_p2_h = MLA3(s0colsum0_h, s1colsum0_h, three);
    v128_t output1_p1_l = wasm_i16x8_add(MLA3(s1colsum1_l, s0colsum1_l, three), seven);
    v128_t output1_p1_h = wasm_i16x8_add(MLA3(s1colsum1_h, s0colsum1_h, three), seven);
    v128_t output1_p2_l = MLA3(s0colsum1_l, s1colsum1_l, three);
    v128_t output1_p2_h = MLA3(s0colsum1_h, s1colsum1_h, three);

    st2_u8(outptr0 + 1, SHRN4(output0_p1_l, output0_p1_h),
           RSHRN4(output0_p2_l, output0_p2_h, eight));
    st2_u8(outptr1 + 1, SHRN4(output1_p1_l, output1_p1_h),
           RSHRN4(output1_p2_l, output1_p2_h, eight));

    /* Re-align on a 32-byte boundary (32/33 pixel boundary) to stay within
     * the padded sample buffers, exactly as the NEON implementation does. */
    for (colctr = 16; colctr < downsampled_width; colctr += 16) {
      s0A = wasm_v128_load(inptr0 + colctr - 1);
      s0B = wasm_v128_load(inptr1 + colctr - 1);
      s0C = wasm_v128_load(inptr2 + colctr - 1);
      s0colsum0_l = BLEND3_LOW(s0A, s0B, three);
      s0colsum0_h = BLEND3_HIGH(s0A, s0B, three);
      s0colsum1_l = BLEND3_LOW(s0C, s0B, three);
      s0colsum1_h = BLEND3_HIGH(s0C, s0B, three);
      s1A = wasm_v128_load(inptr0 + colctr);
      s1B = wasm_v128_load(inptr1 + colctr);
      s1C = wasm_v128_load(inptr2 + colctr);
      s1colsum0_l = BLEND3_LOW(s1A, s1B, three);
      s1colsum0_h = BLEND3_HIGH(s1A, s1B, three);
      s1colsum1_l = BLEND3_LOW(s1C, s1B, three);
      s1colsum1_h = BLEND3_HIGH(s1C, s1B, three);

      output0_p1_l = wasm_i16x8_add(MLA3(s1colsum0_l, s0colsum0_l, three), seven);
      output0_p1_h = wasm_i16x8_add(MLA3(s1colsum0_h, s0colsum0_h, three), seven);
      output0_p2_l = MLA3(s0colsum0_l, s1colsum0_l, three);
      output0_p2_h = MLA3(s0colsum0_h, s1colsum0_h, three);
      output1_p1_l = wasm_i16x8_add(MLA3(s1colsum1_l, s0colsum1_l, three), seven);
      output1_p1_h = wasm_i16x8_add(MLA3(s1colsum1_h, s0colsum1_h, three), seven);
      output1_p2_l = MLA3(s0colsum1_l, s1colsum1_l, three);
      output1_p2_h = MLA3(s0colsum1_h, s1colsum1_h, three);

      st2_u8(outptr0 + 2 * colctr - 1, SHRN4(output0_p1_l, output0_p1_h),
             RSHRN4(output0_p2_l, output0_p2_h, eight));
      st2_u8(outptr1 + 2 * colctr - 1, SHRN4(output1_p1_l, output1_p1_h),
             RSHRN4(output1_p2_l, output1_p2_h, eight));
    }

    /* Last pixel component value in this row of the original image */
    int s1colsum0 = GETJSAMPLE(inptr1[downsampled_width - 1]) * 3 +
                    GETJSAMPLE(inptr0[downsampled_width - 1]);
    outptr0[2 * downsampled_width - 1] = (JSAMPLE)((s1colsum0 * 4 + 7) >> 4);
    int s1colsum1 = GETJSAMPLE(inptr1[downsampled_width - 1]) * 3 +
                    GETJSAMPLE(inptr2[downsampled_width - 1]);
    outptr1[2 * downsampled_width - 1] = (JSAMPLE)((s1colsum1 * 4 + 7) >> 4);
    inrow++;
  }
}

/* Truncating and rounding narrowing shifts by 2; values are
 * <= (3*255 + 255 + 2) >> 2 = 255, so the saturating narrow is exact. */
#define SHRN2(lo, hi) \
  wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(lo, 2), wasm_u16x8_shr(hi, 2))
#define RSHRN2(lo, hi, two) \
  wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(wasm_i16x8_add(lo, two), 2), \
                          wasm_u16x8_shr(wasm_i16x8_add(hi, two), 2))

/* Transliteration of jsimd_h2v1_fancy_upsample_neon() (which matches the
 * scalar h2v1_fancy_upsample() in jdsample.c bit for bit: 3:1 blends with a
 * +1 truncating shift for the left-neighbor pixel and a +2 rounding shift
 * for the right-neighbor pixel). Same 16-column blocks with no scalar tail,
 * relying on the 32-byte padding of the sample buffers. */
void jsimd_h2v1_fancy_upsample_wasm(int max_v_samp_factor,
                                    JDIMENSION downsampled_width,
                                    JSAMPARRAY input_data,
                                    JSAMPARRAY *output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  int inrow;
  unsigned colctr;

  const v128_t three = wasm_i16x8_splat(3);
  const v128_t one = wasm_i16x8_splat(1);
  const v128_t two = wasm_i16x8_splat(2);

  for (inrow = 0; inrow < max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    /* First pixel component value in this row of the original image */
    *outptr = (JSAMPLE)GETJSAMPLE(*inptr);

    /*    3/4 * containing sample + 1/4 * nearest neighboring sample
     * For p1: containing sample = s0, nearest neighboring sample = s1
     * For p2: containing sample = s1, nearest neighboring sample = s0
     */
    v128_t s0 = wasm_v128_load(inptr);
    v128_t s1 = wasm_v128_load(inptr + 1);
    v128_t s1_add_3s0_l = BLEND3_LOW(s1, s0, three);
    v128_t s1_add_3s0_h = BLEND3_HIGH(s1, s0, three);
    /* Add ordered dithering bias to odd pixel values. */
    v128_t s0_add_3s1_l = wasm_i16x8_add(BLEND3_LOW(s0, s1, three), one);
    v128_t s0_add_3s1_h = wasm_i16x8_add(BLEND3_HIGH(s0, s1, three), one);

    st2_u8(outptr + 1, RSHRN2(s1_add_3s0_l, s1_add_3s0_h, two),
           SHRN2(s0_add_3s1_l, s0_add_3s1_h));

    /* Re-align on a 32-byte boundary to stay within the padded sample
     * buffers, exactly as the NEON implementation does. */
    for (colctr = 16; colctr < downsampled_width; colctr += 16) {
      s0 = wasm_v128_load(inptr + colctr - 1);
      s1 = wasm_v128_load(inptr + colctr);
      s1_add_3s0_l = BLEND3_LOW(s1, s0, three);
      s1_add_3s0_h = BLEND3_HIGH(s1, s0, three);
      s0_add_3s1_l = wasm_i16x8_add(BLEND3_LOW(s0, s1, three), one);
      s0_add_3s1_h = wasm_i16x8_add(BLEND3_HIGH(s0, s1, three), one);

      st2_u8(outptr + 2 * colctr - 1,
             RSHRN2(s1_add_3s0_l, s1_add_3s0_h, two),
             SHRN2(s0_add_3s1_l, s0_add_3s1_h));
    }

    /* Last pixel component value in this row of the original image */
    outptr[2 * downsampled_width - 1] =
      GETJSAMPLE(inptr[downsampled_width - 1]);
  }
}

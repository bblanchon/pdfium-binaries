/*
 * jquanti-wasm.c - sample conversion and integer quantization
 *                  (WebAssembly SIMD128)
 *
 * Transliteration of jsimd_convsamp_neon() and jsimd_quantize_neon() from
 * simd/arm/jquanti-neon.c, producing the same output as the scalar
 * convsamp()/quantize() in jcdctmgr.c.
 *
 * wasm SIMD has no per-lane variable shift (NEON's vshlq_u16 with a negated
 * shift vector), so the final "value >> shift" of quantization is done as a
 * multiply by a per-lane power of two followed by taking the high 16 bits:
 * (x * 2^(16-s)) >> 16 == x >> s exactly (both truncate). The 2^(16-s)
 * factors are built from the shift lanes with byte swizzles; s == 0 lanes
 * (2^16 does not fit u16) are blended back to the unshifted value.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

void jsimd_convsamp_wasm(JSAMPARRAY sample_data, JDIMENSION start_col,
                         DCTELEM *workspace)
{
  const v128_t center = wasm_i16x8_splat(CENTERJSAMPLE);
  int i;

  for (i = 0; i < DCTSIZE; i++) {
    /* Load 8 samples zero-extended to u16, then center around 0. */
    v128_t row = wasm_u16x8_load8x8(sample_data[i] + start_col);
    wasm_v128_store(workspace + i * DCTSIZE, wasm_i16x8_sub(row, center));
  }
}

/* Truncating >>16 on a pair of u32x4 products: take bytes 2..3 of each
 * lane. */
#define SHRN16(lo, hi) \
  wasm_i8x16_shuffle(lo, hi, 2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, \
                     27, 30, 31)

/* Low byte of 2^(16-s) for s = 0..15 (s = 0 is masked out separately). */
static const uint8_t jquanti_pow2_lut_lo[16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 64, 32, 16, 8, 4, 2
};
/* High byte of 2^(16-s) for s = 0..15. */
static const uint8_t jquanti_pow2_lut_hi[16] = {
  0, 128, 64, 32, 16, 8, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0
};

void jsimd_quantize_wasm(JCOEFPTR coef_block, DCTELEM *divisors,
                         DCTELEM *workspace)
{
  JCOEFPTR out_ptr = coef_block;
  UDCTELEM *recip_ptr = (UDCTELEM *)divisors;
  UDCTELEM *corr_ptr = (UDCTELEM *)divisors + DCTSIZE2;
  DCTELEM *shift_ptr = divisors + 3 * DCTSIZE2;
  const v128_t lut_lo = wasm_v128_load(jquanti_pow2_lut_lo);
  const v128_t lut_hi = wasm_v128_load(jquanti_pow2_lut_hi);
  const v128_t byte_odd_mask = wasm_i16x8_splat((int16_t)0xff00);
  const v128_t zero = wasm_i16x8_splat(0);
  int i;

  for (i = 0; i < DCTSIZE; i++) {
    v128_t row = wasm_v128_load(workspace + i * DCTSIZE);
    v128_t recip = wasm_v128_load(recip_ptr + i * DCTSIZE);
    v128_t corr = wasm_v128_load(corr_ptr + i * DCTSIZE);
    v128_t shift = wasm_v128_load(shift_ptr + i * DCTSIZE);

    /* Extract sign and take the absolute value. */
    v128_t sign_row = wasm_i16x8_shr(row, 15);
    v128_t abs_row = wasm_i16x8_abs(row);
    /* Add correction. */
    abs_row = wasm_i16x8_add(abs_row, corr);

    /* Multiply by the quantization reciprocals and keep the high 16 bits. */
    v128_t prod_l = wasm_u32x4_extmul_low_u16x8(abs_row, recip);
    v128_t prod_h = wasm_u32x4_extmul_high_u16x8(abs_row, recip);
    row = SHRN16(prod_l, prod_h);

    /* Per-lane logical right shift by the shift lanes: multiply by
     * 2^(16-s) and keep the high 16 bits; lanes with s == 0 keep row. */
    v128_t idx = wasm_i8x16_shuffle(shift, shift, 0, 0, 2, 2, 4, 4, 6, 6, 8,
                                    8, 10, 10, 12, 12, 14, 14);
    v128_t pow2 = wasm_v128_bitselect(wasm_i8x16_swizzle(lut_hi, idx),
                                      wasm_i8x16_swizzle(lut_lo, idx),
                                      byte_odd_mask);
    v128_t shifted_l = wasm_u32x4_extmul_low_u16x8(row, pow2);
    v128_t shifted_h = wasm_u32x4_extmul_high_u16x8(row, pow2);
    v128_t shifted = SHRN16(shifted_l, shifted_h);
    row = wasm_v128_bitselect(row, shifted, wasm_i16x8_eq(shift, zero));

    /* Restore the sign. */
    row = wasm_i16x8_sub(wasm_v128_xor(row, sign_row), sign_row);

    wasm_v128_store(out_ptr + i * DCTSIZE, row);
  }
}

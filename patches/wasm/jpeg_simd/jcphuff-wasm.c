/*
 * jcphuff-wasm.c - progressive Huffman encoding data preparation
 *                  (WebAssembly SIMD128)
 *
 * Transliteration of jsimd_encode_mcu_AC_first_prepare_neon() and
 * jsimd_encode_mcu_AC_refine_prepare_neon() from simd/arm/jcphuff-neon.c,
 * producing the same output as the scalar functions in jcphuff.c.
 *
 * wasm is a 32-bit target, so the size_t bitmap words follow the AArch32
 * layout: two 32-bit words per 64-coefficient bitmap, bit k of word w set
 * for coefficient (32 * w + k). The NEON bitmap construction (AND with a
 * per-lane bit mask, then a vpadd reduction tree) is replaced with
 * wasm_i8x16_bitmask on narrowed compare masks, which produces the same
 * LSB-first bit order directly.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

/* Gather 8 coefficients through the natural-order index table
 * (vld1q_lane_s16 sequence in the NEON version). */
static inline v128_t gather8_s16(const JCOEF *block, const int *order)
{
  v128_t v = wasm_v128_load16_splat(block + order[0]);
  v = wasm_v128_load16_lane(block + order[1], v, 1);
  v = wasm_v128_load16_lane(block + order[2], v, 2);
  v = wasm_v128_load16_lane(block + order[3], v, 3);
  v = wasm_v128_load16_lane(block + order[4], v, 4);
  v = wasm_v128_load16_lane(block + order[5], v, 5);
  v = wasm_v128_load16_lane(block + order[6], v, 6);
  v = wasm_v128_load16_lane(block + order[7], v, 7);
  return v;
}

/* Gather the trailing (1..8) coefficients, zeroing the rest (the
 * FALLTHROUGH switch in the NEON version). */
static inline v128_t gather_partial_s16(const JCOEF *block, const int *order,
                                        int count)
{
  v128_t v = wasm_i16x8_splat(0);
  switch (count) {
  case 8:
    v = wasm_v128_load16_lane(block + order[7], v, 7);
    /* FALLTHROUGH */
  case 7:
    v = wasm_v128_load16_lane(block + order[6], v, 6);
    /* FALLTHROUGH */
  case 6:
    v = wasm_v128_load16_lane(block + order[5], v, 5);
    /* FALLTHROUGH */
  case 5:
    v = wasm_v128_load16_lane(block + order[4], v, 4);
    /* FALLTHROUGH */
  case 4:
    v = wasm_v128_load16_lane(block + order[3], v, 3);
    /* FALLTHROUGH */
  case 3:
    v = wasm_v128_load16_lane(block + order[2], v, 2);
    /* FALLTHROUGH */
  case 2:
    v = wasm_v128_load16_lane(block + order[1], v, 1);
    /* FALLTHROUGH */
  case 1:
    v = wasm_v128_load16_lane(block + order[0], v, 0);
    /* FALLTHROUGH */
  default:
    break;
  }
  return v;
}

/* 16 "coefficient == 0" bits (LSB first) for two rows of 8 u16 values. */
static inline uint32_t eq0_bits16(v128_t rowa, v128_t rowb)
{
  const v128_t zero = wasm_i16x8_splat(0);
  return (uint32_t)wasm_i8x16_bitmask(wasm_i8x16_narrow_i16x8(
      wasm_i16x8_eq(rowa, zero), wasm_i16x8_eq(rowb, zero)));
}

void jsimd_encode_mcu_AC_first_prepare_wasm
  (const JCOEF *block, const int *jpeg_natural_order_start, int Sl, int Al,
   UJCOEF *values, size_t *zerobits)
{
  UJCOEF *values_ptr = values;
  UJCOEF *diff_values_ptr = values + DCTSIZE2;
  int i, rows_to_zero = 8;

  for (i = 0; i < Sl / 16; i++) {
    v128_t coefs1 = gather8_s16(block, jpeg_natural_order_start);
    v128_t coefs2 = gather8_s16(block, jpeg_natural_order_start + 8);

    /* Isolate sign, compute absolute value, apply point transform Al. */
    v128_t sign_coefs1 = wasm_i16x8_shr(coefs1, 15);
    v128_t sign_coefs2 = wasm_i16x8_shr(coefs2, 15);
    v128_t abs_coefs1 = wasm_u16x8_shr(wasm_i16x8_abs(coefs1), Al);
    v128_t abs_coefs2 = wasm_u16x8_shr(wasm_i16x8_abs(coefs2), Al);

    /* diff = abs ^ sign (~abs for negative coefficients). */
    wasm_v128_store(values_ptr, abs_coefs1);
    wasm_v128_store(values_ptr + DCTSIZE, abs_coefs2);
    wasm_v128_store(diff_values_ptr, wasm_v128_xor(abs_coefs1, sign_coefs1));
    wasm_v128_store(diff_values_ptr + DCTSIZE,
                    wasm_v128_xor(abs_coefs2, sign_coefs2));
    values_ptr += 16;
    diff_values_ptr += 16;
    jpeg_natural_order_start += 16;
    rows_to_zero -= 2;
  }

  /* Same operation but for the remaining partial vector. */
  int remaining_coefs = Sl % 16;
  if (remaining_coefs > 8) {
    v128_t coefs1 = gather8_s16(block, jpeg_natural_order_start);
    v128_t coefs2 = gather_partial_s16(block, jpeg_natural_order_start + 8,
                                       remaining_coefs - 8);

    v128_t sign_coefs1 = wasm_i16x8_shr(coefs1, 15);
    v128_t sign_coefs2 = wasm_i16x8_shr(coefs2, 15);
    v128_t abs_coefs1 = wasm_u16x8_shr(wasm_i16x8_abs(coefs1), Al);
    v128_t abs_coefs2 = wasm_u16x8_shr(wasm_i16x8_abs(coefs2), Al);

    wasm_v128_store(values_ptr, abs_coefs1);
    wasm_v128_store(values_ptr + DCTSIZE, abs_coefs2);
    wasm_v128_store(diff_values_ptr, wasm_v128_xor(abs_coefs1, sign_coefs1));
    wasm_v128_store(diff_values_ptr + DCTSIZE,
                    wasm_v128_xor(abs_coefs2, sign_coefs2));
    values_ptr += 16;
    diff_values_ptr += 16;
    rows_to_zero -= 2;

  } else if (remaining_coefs > 0) {
    v128_t coefs = gather_partial_s16(block, jpeg_natural_order_start,
                                      remaining_coefs);

    v128_t sign_coefs = wasm_i16x8_shr(coefs, 15);
    v128_t abs_coefs = wasm_u16x8_shr(wasm_i16x8_abs(coefs), Al);

    wasm_v128_store(values_ptr, abs_coefs);
    wasm_v128_store(diff_values_ptr, wasm_v128_xor(abs_coefs, sign_coefs));
    values_ptr += 8;
    diff_values_ptr += 8;
    rows_to_zero--;
  }

  /* Zero remaining memory in the values and diff_values blocks. */
  for (i = 0; i < rows_to_zero; i++) {
    wasm_v128_store(values_ptr, wasm_i16x8_splat(0));
    wasm_v128_store(diff_values_ptr, wasm_i16x8_splat(0));
    values_ptr += 8;
    diff_values_ptr += 8;
  }

  /* Construct zerobits bitmap.  A set bit means that the corresponding
   * coefficient != 0.
   */
  v128_t row0 = wasm_v128_load(values + 0 * DCTSIZE);
  v128_t row1 = wasm_v128_load(values + 1 * DCTSIZE);
  v128_t row2 = wasm_v128_load(values + 2 * DCTSIZE);
  v128_t row3 = wasm_v128_load(values + 3 * DCTSIZE);
  v128_t row4 = wasm_v128_load(values + 4 * DCTSIZE);
  v128_t row5 = wasm_v128_load(values + 5 * DCTSIZE);
  v128_t row6 = wasm_v128_load(values + 6 * DCTSIZE);
  v128_t row7 = wasm_v128_load(values + 7 * DCTSIZE);

  zerobits[0] = ~(eq0_bits16(row0, row1) | (eq0_bits16(row2, row3) << 16));
  zerobits[1] = ~(eq0_bits16(row4, row5) | (eq0_bits16(row6, row7) << 16));
}

int jsimd_encode_mcu_AC_refine_prepare_wasm
  (const JCOEF *block, const int *jpeg_natural_order_start, int Sl, int Al,
   UJCOEF *absvalues, size_t *bits)
{
  /* Temporary storage buffers for data used to compute the signbits bitmap
   * and the end-of-block (EOB) position.
   */
  uint8_t coef_sign_bits[64];
  uint8_t coef_eq1_bits[64];

  UJCOEF *absvalues_ptr = absvalues;
  uint8_t *coef_sign_bits_ptr = coef_sign_bits;
  uint8_t *eq1_bits_ptr = coef_eq1_bits;

  const v128_t one = wasm_i16x8_splat(1);
  int i, rows_to_zero = 8;

  for (i = 0; i < Sl / 16; i++) {
    v128_t coefs1 = gather8_s16(block, jpeg_natural_order_start);
    v128_t coefs2 = gather8_s16(block, jpeg_natural_order_start + 8);

    /* Compute and store data for the signbits bitmap. */
    v128_t sign_coefs1 = wasm_i16x8_shr(coefs1, 15);
    v128_t sign_coefs2 = wasm_i16x8_shr(coefs2, 15);
    wasm_v128_store(coef_sign_bits_ptr,
                    wasm_i8x16_narrow_i16x8(sign_coefs1, sign_coefs2));

    /* Compute absolute value and apply point transform Al. */
    v128_t abs_coefs1 = wasm_u16x8_shr(wasm_i16x8_abs(coefs1), Al);
    v128_t abs_coefs2 = wasm_u16x8_shr(wasm_i16x8_abs(coefs2), Al);
    wasm_v128_store(absvalues_ptr, abs_coefs1);
    wasm_v128_store(absvalues_ptr + DCTSIZE, abs_coefs2);

    /* Test whether transformed coefficient values == 1 (used to find the
     * EOB position.)
     */
    wasm_v128_store(eq1_bits_ptr,
                    wasm_i8x16_narrow_i16x8(wasm_i16x8_eq(abs_coefs1, one),
                                            wasm_i16x8_eq(abs_coefs2, one)));

    absvalues_ptr += 16;
    coef_sign_bits_ptr += 16;
    eq1_bits_ptr += 16;
    jpeg_natural_order_start += 16;
    rows_to_zero -= 2;
  }

  /* Same operation but for the remaining partial vector. */
  int remaining_coefs = Sl % 16;
  if (remaining_coefs > 8) {
    v128_t coefs1 = gather8_s16(block, jpeg_natural_order_start);
    v128_t coefs2 = gather_partial_s16(block, jpeg_natural_order_start + 8,
                                       remaining_coefs - 8);

    v128_t sign_coefs1 = wasm_i16x8_shr(coefs1, 15);
    v128_t sign_coefs2 = wasm_i16x8_shr(coefs2, 15);
    wasm_v128_store(coef_sign_bits_ptr,
                    wasm_i8x16_narrow_i16x8(sign_coefs1, sign_coefs2));

    v128_t abs_coefs1 = wasm_u16x8_shr(wasm_i16x8_abs(coefs1), Al);
    v128_t abs_coefs2 = wasm_u16x8_shr(wasm_i16x8_abs(coefs2), Al);
    wasm_v128_store(absvalues_ptr, abs_coefs1);
    wasm_v128_store(absvalues_ptr + DCTSIZE, abs_coefs2);

    wasm_v128_store(eq1_bits_ptr,
                    wasm_i8x16_narrow_i16x8(wasm_i16x8_eq(abs_coefs1, one),
                                            wasm_i16x8_eq(abs_coefs2, one)));

    absvalues_ptr += 16;
    coef_sign_bits_ptr += 16;
    eq1_bits_ptr += 16;
    rows_to_zero -= 2;

  } else if (remaining_coefs > 0) {
    v128_t coefs = gather_partial_s16(block, jpeg_natural_order_start,
                                      remaining_coefs);

    v128_t sign_coefs = wasm_i16x8_shr(coefs, 15);
    v128_t sign_bytes = wasm_i8x16_narrow_i16x8(sign_coefs, sign_coefs);
    wasm_v128_store64_lane(coef_sign_bits_ptr, sign_bytes, 0);

    v128_t abs_coefs = wasm_u16x8_shr(wasm_i16x8_abs(coefs), Al);
    wasm_v128_store(absvalues_ptr, abs_coefs);

    v128_t eq1 = wasm_i16x8_eq(abs_coefs, one);
    v128_t eq1_bytes = wasm_i8x16_narrow_i16x8(eq1, eq1);
    wasm_v128_store64_lane(eq1_bits_ptr, eq1_bytes, 0);

    absvalues_ptr += 8;
    coef_sign_bits_ptr += 8;
    eq1_bits_ptr += 8;
    rows_to_zero--;
  }

  /* Zero remaining memory in the blocks. */
  for (i = 0; i < rows_to_zero; i++) {
    wasm_v128_store(absvalues_ptr, wasm_i16x8_splat(0));
    wasm_v128_store64_lane(coef_sign_bits_ptr, wasm_i16x8_splat(0), 0);
    wasm_v128_store64_lane(eq1_bits_ptr, wasm_i16x8_splat(0), 0);
    absvalues_ptr += 8;
    coef_sign_bits_ptr += 8;
    eq1_bits_ptr += 8;
  }

  /* Construct zerobits bitmap. */
  v128_t abs_row0 = wasm_v128_load(absvalues + 0 * DCTSIZE);
  v128_t abs_row1 = wasm_v128_load(absvalues + 1 * DCTSIZE);
  v128_t abs_row2 = wasm_v128_load(absvalues + 2 * DCTSIZE);
  v128_t abs_row3 = wasm_v128_load(absvalues + 3 * DCTSIZE);
  v128_t abs_row4 = wasm_v128_load(absvalues + 4 * DCTSIZE);
  v128_t abs_row5 = wasm_v128_load(absvalues + 5 * DCTSIZE);
  v128_t abs_row6 = wasm_v128_load(absvalues + 6 * DCTSIZE);
  v128_t abs_row7 = wasm_v128_load(absvalues + 7 * DCTSIZE);

  bits[0] = ~(eq0_bits16(abs_row0, abs_row1) |
              (eq0_bits16(abs_row2, abs_row3) << 16));
  bits[1] = ~(eq0_bits16(abs_row4, abs_row5) |
              (eq0_bits16(abs_row6, abs_row7) << 16));

  /* Construct signbits bitmap. The stored sign bytes are 0xFF/0x00 masks,
   * so wasm_i8x16_bitmask() extracts them directly. */
  uint32_t sign01 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_sign_bits + 0 * DCTSIZE));
  uint32_t sign23 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_sign_bits + 2 * DCTSIZE));
  uint32_t sign45 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_sign_bits + 4 * DCTSIZE));
  uint32_t sign67 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_sign_bits + 6 * DCTSIZE));
  bits[2] = ~(sign01 | (sign23 << 16));
  bits[3] = ~(sign45 | (sign67 << 16));

  /* Construct bitmap to find the EOB position (the index of the last
   * coefficient equal to 1.)
   */
  uint32_t eq1_01 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_eq1_bits + 0 * DCTSIZE));
  uint32_t eq1_23 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_eq1_bits + 2 * DCTSIZE));
  uint32_t eq1_45 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_eq1_bits + 4 * DCTSIZE));
  uint32_t eq1_67 = (uint32_t)wasm_i8x16_bitmask(
      wasm_v128_load(coef_eq1_bits + 6 * DCTSIZE));
  uint32_t bitmap0 = eq1_01 | (eq1_23 << 16);
  uint32_t bitmap1 = eq1_45 | (eq1_67 << 16);

  /* Return EOB position. */
  if (bitmap0 == 0 && bitmap1 == 0) {
    /* EOB position is defined to be 0 if all coefficients != 1. */
    return 0;
  } else if (bitmap1 != 0) {
    return 63 - __builtin_clz(bitmap1);
  } else {
    return 31 - __builtin_clz(bitmap0);
  }
}

/*
 * jidctint-wasm.c - accurate integer IDCT (WebAssembly SIMD128)
 *
 * Transliteration of simd/arm/jidctint-neon.c to wasm_simd128 intrinsics,
 * preserving its arithmetic exactly:
 *  - NEON 64-bit int16x4 vectors are carried in the low half of a v128; the
 *    high lanes hold garbage and are never observed.
 *  - vmull/vmlal/vmlsl by constant lanes become widen + i32 multiply by a
 *    splatted constant.
 *  - vrshrn_n_s32 (rounding shift + modular narrow) becomes add-round, shift,
 *    and a byte shuffle taking the low 16 bits of each lane.
 *  - vaddhn/vsubhn pairs (take the high 16 bits of each 32-bit lane) are
 *    fused into single two-source byte shuffles.
 *  - The vqrshrn+centering output stage is computed as
 *    saturate_u8(((x + 2) >> 2) + 128), which equals NEON's
 *    saturate_s8((x + 2) >> 2) + 128 (both are clamp(descale + 128, 0, 255)).
 *
 * The two-step descale (high-half narrow by 16, then rounding shift by
 * DESCALE_P2 - 16) is exactly equal to the scalar DESCALE(x, DESCALE_P2)
 * for all inputs: floor((floor(x / 2^16) + 2) / 4) == floor((x + 2^17) / 2^18).
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <wasm_simd128.h>

#define CONST_BITS  13
#define PASS1_BITS  2

#define DESCALE_P1  (CONST_BITS - PASS1_BITS)
#define DESCALE_P2  (CONST_BITS + PASS1_BITS + 3)

#define F_0_298  2446
#define F_0_390  3196
#define F_0_541  4433
#define F_0_765  6270
#define F_0_899  7373
#define F_1_175  9633
#define F_1_501  12299
#define F_1_847  15137
#define F_1_961  16069
#define F_2_053  16819
#define F_2_562  20995
#define F_3_072  25172

#define F_1_175_MINUS_1_961  (F_1_175 - F_1_961)
#define F_1_175_MINUS_0_390  (F_1_175 - F_0_390)
#define F_0_541_MINUS_1_847  (F_0_541 - F_1_847)
#define F_3_072_MINUS_2_562  (F_3_072 - F_2_562)
#define F_0_298_MINUS_0_899  (F_0_298 - F_0_899)
#define F_1_501_MINUS_0_899  (F_1_501 - F_0_899)
#define F_2_053_MINUS_2_562  (F_2_053 - F_2_562)
#define F_0_541_PLUS_0_765   (F_0_541 + F_0_765)

/* NEON int16x4 emulation: 4 valid lanes in the low half of a v128. */
#define LD_S16X4(p)  wasm_v128_load64_zero(p)
#define MUL16(a, b)  wasm_i16x8_mul(a, b)
#define ADD16(a, b)  wasm_i16x8_add(a, b)
#define SHL16(a, n)  wasm_i16x8_shl(a, n)
#define WIDEN(a)  wasm_i32x4_extend_low_i16x8(a)
#define MULL(a, c)  wasm_i32x4_mul(WIDEN(a), wasm_i32x4_splat(c))
#define MLAL(acc, a, c)  wasm_i32x4_add(acc, MULL(a, c))
#define MLSL(acc, a, c)  wasm_i32x4_sub(acc, MULL(a, c))
#define SHLL(a, n)  wasm_i32x4_shl(WIDEN(a), n)
#define ADD32(a, b)  wasm_i32x4_add(a, b)
#define SUB32(a, b)  wasm_i32x4_sub(a, b)
#define LOW64(a)  wasm_i64x2_extract_lane(a, 0)

/* vrshrn_n_s32(x, DESCALE_P1) as low-4-lane i16: rounding shift, then take
 * the low 16 bits of each 32-bit lane (modular narrow). */
static inline v128_t rshrn_p1(v128_t x)
{
  v128_t r = wasm_i32x4_shr(
      wasm_i32x4_add(x, wasm_i32x4_splat(1 << (DESCALE_P1 - 1))), DESCALE_P1);
  return wasm_i8x16_shuffle(r, r, 0, 1, 4, 5, 8, 9, 12, 13, 0, 1, 4, 5, 8, 9,
                            12, 13);
}

/* vcombine(vaddhn(a), vaddhn(b)): the high 16 bits of each 32-bit lane of a
 * (low half) and b (high half), as 8 i16 lanes. */
#define ADDHN2(a0, a1, b0, b1) \
  wasm_i8x16_shuffle(ADD32(a0, a1), ADD32(b0, b1), 2, 3, 6, 7, 10, 11, 14, \
                     15, 18, 19, 22, 23, 26, 27, 30, 31)
#define SUBHN2(a0, a1, b0, b1) \
  wasm_i8x16_shuffle(SUB32(a0, a1), SUB32(b0, b1), 2, 3, 6, 7, 10, 11, 14, \
                     15, 18, 19, 22, 23, 26, 27, 30, 31)

/* vst4_s16 of four low-4-lane i16 vectors: interleave into 32 bytes. */
static inline void st4_s16(int16_t *ws, v128_t r0, v128_t r1, v128_t r2,
                           v128_t r3)
{
  v128_t p = wasm_i8x16_shuffle(r0, r1, 0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20,
                                21, 6, 7, 22, 23);
  v128_t q = wasm_i8x16_shuffle(r2, r3, 0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20,
                                21, 6, 7, 22, 23);
  wasm_v128_store(ws, wasm_i8x16_shuffle(p, q, 0, 1, 2, 3, 16, 17, 18, 19, 4,
                                         5, 6, 7, 20, 21, 22, 23));
  wasm_v128_store(ws + 8,
                  wasm_i8x16_shuffle(p, q, 8, 9, 10, 11, 24, 25, 26, 27, 12,
                                     13, 14, 15, 28, 29, 30, 31));
}

/* Output stage shared by both pass-2 variants: descale the eight 16-bit
 * column pairs, clamp, transpose and store 8 pixels to each of 4 rows. */
static inline void pass2_store(v128_t cols_02_s16, v128_t cols_13_s16,
                               v128_t cols_46_s16, v128_t cols_57_s16,
                               JSAMPARRAY output_buf, JDIMENSION output_col,
                               unsigned buf_offset)
{
  const v128_t two = wasm_i16x8_splat(2);
  const v128_t center = wasm_i16x8_splat(CENTERJSAMPLE);
  /* clamp(((x + 2) >> 2) + 128, 0, 255) for each lane. */
  v128_t v02 = ADD16(wasm_i16x8_shr(ADD16(cols_02_s16, two), DESCALE_P2 - 16),
                     center);
  v128_t v13 = ADD16(wasm_i16x8_shr(ADD16(cols_13_s16, two), DESCALE_P2 - 16),
                     center);
  v128_t v46 = ADD16(wasm_i16x8_shr(ADD16(cols_46_s16, two), DESCALE_P2 - 16),
                     center);
  v128_t v57 = ADD16(wasm_i16x8_shr(ADD16(cols_57_s16, two), DESCALE_P2 - 16),
                     center);
  v128_t u02 = wasm_u8x16_narrow_i16x8(v02, v02);
  v128_t u13 = wasm_u8x16_narrow_i16x8(v13, v13);
  v128_t u46 = wasm_u8x16_narrow_i16x8(v46, v46);
  v128_t u57 = wasm_u8x16_narrow_i16x8(v57, v57);

  /* u02 bytes: c0r0..c0r3 c2r0..c2r3 (etc.); regroup per row. */
  v128_t x = wasm_i8x16_shuffle(u02, u13, 0, 16, 4, 20, 1, 17, 5, 21, 2, 18,
                                6, 22, 3, 19, 7, 23);
  v128_t y = wasm_i8x16_shuffle(u46, u57, 0, 16, 4, 20, 1, 17, 5, 21, 2, 18,
                                6, 22, 3, 19, 7, 23);

  JSAMPROW outptr0 = output_buf[buf_offset + 0] + output_col;
  JSAMPROW outptr1 = output_buf[buf_offset + 1] + output_col;
  JSAMPROW outptr2 = output_buf[buf_offset + 2] + output_col;
  JSAMPROW outptr3 = output_buf[buf_offset + 3] + output_col;
  wasm_v128_store64_lane(outptr0,
                         wasm_i8x16_shuffle(x, y, 0, 1, 2, 3, 16, 17, 18, 19,
                                            0, 0, 0, 0, 0, 0, 0, 0), 0);
  wasm_v128_store64_lane(outptr1,
                         wasm_i8x16_shuffle(x, y, 4, 5, 6, 7, 20, 21, 22, 23,
                                            0, 0, 0, 0, 0, 0, 0, 0), 0);
  wasm_v128_store64_lane(outptr2,
                         wasm_i8x16_shuffle(x, y, 8, 9, 10, 11, 24, 25, 26,
                                            27, 0, 0, 0, 0, 0, 0, 0, 0), 0);
  wasm_v128_store64_lane(outptr3,
                         wasm_i8x16_shuffle(x, y, 12, 13, 14, 15, 28, 29, 30,
                                            31, 0, 0, 0, 0, 0, 0, 0, 0), 0);
}

static inline void jsimd_idct_islow_wasm_pass1_regular(
    v128_t row0, v128_t row1, v128_t row2, v128_t row3, v128_t row4,
    v128_t row5, v128_t row6, v128_t row7, v128_t quant_row0,
    v128_t quant_row1, v128_t quant_row2, v128_t quant_row3,
    v128_t quant_row4, v128_t quant_row5, v128_t quant_row6,
    v128_t quant_row7, int16_t *workspace_1, int16_t *workspace_2)
{
  /* Even part */
  v128_t z2_s16 = MUL16(row2, quant_row2);
  v128_t z3_s16 = MUL16(row6, quant_row6);

  v128_t tmp2 = MULL(z2_s16, F_0_541);
  v128_t tmp3 = MULL(z2_s16, F_0_541_PLUS_0_765);
  tmp2 = MLAL(tmp2, z3_s16, F_0_541_MINUS_1_847);
  tmp3 = MLAL(tmp3, z3_s16, F_0_541);

  z2_s16 = MUL16(row0, quant_row0);
  z3_s16 = MUL16(row4, quant_row4);

  v128_t tmp0 = SHLL(ADD16(z2_s16, z3_s16), CONST_BITS);
  v128_t tmp1 = SHLL(wasm_i16x8_sub(z2_s16, z3_s16), CONST_BITS);

  v128_t tmp10 = ADD32(tmp0, tmp3);
  v128_t tmp13 = SUB32(tmp0, tmp3);
  v128_t tmp11 = ADD32(tmp1, tmp2);
  v128_t tmp12 = SUB32(tmp1, tmp2);

  /* Odd part */
  v128_t tmp0_s16 = MUL16(row7, quant_row7);
  v128_t tmp1_s16 = MUL16(row5, quant_row5);
  v128_t tmp2_s16 = MUL16(row3, quant_row3);
  v128_t tmp3_s16 = MUL16(row1, quant_row1);

  z3_s16 = ADD16(tmp0_s16, tmp2_s16);
  v128_t z4_s16 = ADD16(tmp1_s16, tmp3_s16);

  v128_t z3 = MULL(z3_s16, F_1_175_MINUS_1_961);
  v128_t z4 = MULL(z3_s16, F_1_175);
  z3 = MLAL(z3, z4_s16, F_1_175);
  z4 = MLAL(z4, z4_s16, F_1_175_MINUS_0_390);

  tmp0 = MULL(tmp0_s16, F_0_298_MINUS_0_899);
  tmp1 = MULL(tmp1_s16, F_2_053_MINUS_2_562);
  tmp2 = MULL(tmp2_s16, F_3_072_MINUS_2_562);
  tmp3 = MULL(tmp3_s16, F_1_501_MINUS_0_899);

  tmp0 = MLSL(tmp0, tmp3_s16, F_0_899);
  tmp1 = MLSL(tmp1, tmp2_s16, F_2_562);
  tmp2 = MLSL(tmp2, tmp1_s16, F_2_562);
  tmp3 = MLSL(tmp3, tmp0_s16, F_0_899);

  tmp0 = ADD32(tmp0, z3);
  tmp1 = ADD32(tmp1, z4);
  tmp2 = ADD32(tmp2, z3);
  tmp3 = ADD32(tmp3, z4);

  /* Final output stage: descale and narrow to 16-bit. */
  st4_s16(workspace_1, rshrn_p1(ADD32(tmp10, tmp3)),
          rshrn_p1(ADD32(tmp11, tmp2)), rshrn_p1(ADD32(tmp12, tmp1)),
          rshrn_p1(ADD32(tmp13, tmp0)));
  st4_s16(workspace_2, rshrn_p1(SUB32(tmp13, tmp0)),
          rshrn_p1(SUB32(tmp12, tmp1)), rshrn_p1(SUB32(tmp11, tmp2)),
          rshrn_p1(SUB32(tmp10, tmp3)));
}

static inline void jsimd_idct_islow_wasm_pass1_sparse(
    v128_t row0, v128_t row1, v128_t row2, v128_t row3, v128_t quant_row0,
    v128_t quant_row1, v128_t quant_row2, v128_t quant_row3,
    int16_t *workspace_1, int16_t *workspace_2)
{
  /* Even part (z3 is all 0) */
  v128_t z2_s16 = MUL16(row2, quant_row2);

  v128_t tmp2 = MULL(z2_s16, F_0_541);
  v128_t tmp3 = MULL(z2_s16, F_0_541_PLUS_0_765);

  z2_s16 = MUL16(row0, quant_row0);
  v128_t tmp0 = SHLL(z2_s16, CONST_BITS);
  v128_t tmp1 = SHLL(z2_s16, CONST_BITS);

  v128_t tmp10 = ADD32(tmp0, tmp3);
  v128_t tmp13 = SUB32(tmp0, tmp3);
  v128_t tmp11 = ADD32(tmp1, tmp2);
  v128_t tmp12 = SUB32(tmp1, tmp2);

  /* Odd part (tmp0 and tmp1 are both all 0) */
  v128_t tmp2_s16 = MUL16(row3, quant_row3);
  v128_t tmp3_s16 = MUL16(row1, quant_row1);

  v128_t z3 = MULL(tmp2_s16, F_1_175_MINUS_1_961);
  z3 = MLAL(z3, tmp3_s16, F_1_175);
  v128_t z4 = MULL(tmp2_s16, F_1_175);
  z4 = MLAL(z4, tmp3_s16, F_1_175_MINUS_0_390);

  tmp0 = MLSL(z3, tmp3_s16, F_0_899);
  tmp1 = MLSL(z4, tmp2_s16, F_2_562);
  tmp2 = MLAL(z3, tmp2_s16, F_3_072_MINUS_2_562);
  tmp3 = MLAL(z4, tmp3_s16, F_1_501_MINUS_0_899);

  /* Final output stage: descale and narrow to 16-bit. */
  st4_s16(workspace_1, rshrn_p1(ADD32(tmp10, tmp3)),
          rshrn_p1(ADD32(tmp11, tmp2)), rshrn_p1(ADD32(tmp12, tmp1)),
          rshrn_p1(ADD32(tmp13, tmp0)));
  st4_s16(workspace_2, rshrn_p1(SUB32(tmp13, tmp0)),
          rshrn_p1(SUB32(tmp12, tmp1)), rshrn_p1(SUB32(tmp11, tmp2)),
          rshrn_p1(SUB32(tmp10, tmp3)));
}

static inline void jsimd_idct_islow_wasm_pass2_regular(int16_t *workspace,
                                                       JSAMPARRAY output_buf,
                                                       JDIMENSION output_col,
                                                       unsigned buf_offset)
{
  /* Even part */
  v128_t z2_s16 = LD_S16X4(workspace + 2 * DCTSIZE / 2);
  v128_t z3_s16 = LD_S16X4(workspace + 6 * DCTSIZE / 2);

  v128_t tmp2 = MULL(z2_s16, F_0_541);
  v128_t tmp3 = MULL(z2_s16, F_0_541_PLUS_0_765);
  tmp2 = MLAL(tmp2, z3_s16, F_0_541_MINUS_1_847);
  tmp3 = MLAL(tmp3, z3_s16, F_0_541);

  z2_s16 = LD_S16X4(workspace + 0 * DCTSIZE / 2);
  z3_s16 = LD_S16X4(workspace + 4 * DCTSIZE / 2);

  v128_t tmp0 = SHLL(ADD16(z2_s16, z3_s16), CONST_BITS);
  v128_t tmp1 = SHLL(wasm_i16x8_sub(z2_s16, z3_s16), CONST_BITS);

  v128_t tmp10 = ADD32(tmp0, tmp3);
  v128_t tmp13 = SUB32(tmp0, tmp3);
  v128_t tmp11 = ADD32(tmp1, tmp2);
  v128_t tmp12 = SUB32(tmp1, tmp2);

  /* Odd part */
  v128_t tmp0_s16 = LD_S16X4(workspace + 7 * DCTSIZE / 2);
  v128_t tmp1_s16 = LD_S16X4(workspace + 5 * DCTSIZE / 2);
  v128_t tmp2_s16 = LD_S16X4(workspace + 3 * DCTSIZE / 2);
  v128_t tmp3_s16 = LD_S16X4(workspace + 1 * DCTSIZE / 2);

  z3_s16 = ADD16(tmp0_s16, tmp2_s16);
  v128_t z4_s16 = ADD16(tmp1_s16, tmp3_s16);

  v128_t z3 = MULL(z3_s16, F_1_175_MINUS_1_961);
  v128_t z4 = MULL(z3_s16, F_1_175);
  z3 = MLAL(z3, z4_s16, F_1_175);
  z4 = MLAL(z4, z4_s16, F_1_175_MINUS_0_390);

  tmp0 = MULL(tmp0_s16, F_0_298_MINUS_0_899);
  tmp1 = MULL(tmp1_s16, F_2_053_MINUS_2_562);
  tmp2 = MULL(tmp2_s16, F_3_072_MINUS_2_562);
  tmp3 = MULL(tmp3_s16, F_1_501_MINUS_0_899);

  tmp0 = MLSL(tmp0, tmp3_s16, F_0_899);
  tmp1 = MLSL(tmp1, tmp2_s16, F_2_562);
  tmp2 = MLSL(tmp2, tmp1_s16, F_2_562);
  tmp3 = MLSL(tmp3, tmp0_s16, F_0_899);

  tmp0 = ADD32(tmp0, z3);
  tmp1 = ADD32(tmp1, z4);
  tmp2 = ADD32(tmp2, z3);
  tmp3 = ADD32(tmp3, z4);

  /* Final output stage: descale and narrow to 16-bit (high halves). */
  v128_t cols_02_s16 = ADDHN2(tmp10, tmp3, tmp12, tmp1);
  v128_t cols_13_s16 = ADDHN2(tmp11, tmp2, tmp13, tmp0);
  v128_t cols_46_s16 = SUBHN2(tmp13, tmp0, tmp11, tmp2);
  v128_t cols_57_s16 = SUBHN2(tmp12, tmp1, tmp10, tmp3);

  pass2_store(cols_02_s16, cols_13_s16, cols_46_s16, cols_57_s16, output_buf,
              output_col, buf_offset);
}

static inline void jsimd_idct_islow_wasm_pass2_sparse(int16_t *workspace,
                                                      JSAMPARRAY output_buf,
                                                      JDIMENSION output_col,
                                                      unsigned buf_offset)
{
  /* Even part (z3 is all 0) */
  v128_t z2_s16 = LD_S16X4(workspace + 2 * DCTSIZE / 2);

  v128_t tmp2 = MULL(z2_s16, F_0_541);
  v128_t tmp3 = MULL(z2_s16, F_0_541_PLUS_0_765);

  z2_s16 = LD_S16X4(workspace + 0 * DCTSIZE / 2);
  v128_t tmp0 = SHLL(z2_s16, CONST_BITS);
  v128_t tmp1 = SHLL(z2_s16, CONST_BITS);

  v128_t tmp10 = ADD32(tmp0, tmp3);
  v128_t tmp13 = SUB32(tmp0, tmp3);
  v128_t tmp11 = ADD32(tmp1, tmp2);
  v128_t tmp12 = SUB32(tmp1, tmp2);

  /* Odd part (tmp0 and tmp1 are both all 0) */
  v128_t tmp2_s16 = LD_S16X4(workspace + 3 * DCTSIZE / 2);
  v128_t tmp3_s16 = LD_S16X4(workspace + 1 * DCTSIZE / 2);

  v128_t z3 = MULL(tmp2_s16, F_1_175_MINUS_1_961);
  z3 = MLAL(z3, tmp3_s16, F_1_175);
  v128_t z4 = MULL(tmp2_s16, F_1_175);
  z4 = MLAL(z4, tmp3_s16, F_1_175_MINUS_0_390);

  tmp0 = MLSL(z3, tmp3_s16, F_0_899);
  tmp1 = MLSL(z4, tmp2_s16, F_2_562);
  tmp2 = MLAL(z3, tmp2_s16, F_3_072_MINUS_2_562);
  tmp3 = MLAL(z4, tmp3_s16, F_1_501_MINUS_0_899);

  /* Final output stage: descale and narrow to 16-bit (high halves). */
  v128_t cols_02_s16 = ADDHN2(tmp10, tmp3, tmp12, tmp1);
  v128_t cols_13_s16 = ADDHN2(tmp11, tmp2, tmp13, tmp0);
  v128_t cols_46_s16 = SUBHN2(tmp13, tmp0, tmp11, tmp2);
  v128_t cols_57_s16 = SUBHN2(tmp12, tmp1, tmp10, tmp3);

  pass2_store(cols_02_s16, cols_13_s16, cols_46_s16, cols_57_s16, output_buf,
              output_col, buf_offset);
}

void jsimd_idct_islow_wasm(void *dct_table, JCOEFPTR coef_block,
                           JSAMPARRAY output_buf, JDIMENSION output_col)
{
  ISLOW_MULT_TYPE *quantptr = dct_table;

  int16_t workspace_l[8 * DCTSIZE / 2];
  int16_t workspace_r[8 * DCTSIZE / 2];

  /* Compute IDCT first pass on left 4x8 coefficient block. */
  v128_t row0 = LD_S16X4(coef_block + 0 * DCTSIZE);
  v128_t row1 = LD_S16X4(coef_block + 1 * DCTSIZE);
  v128_t row2 = LD_S16X4(coef_block + 2 * DCTSIZE);
  v128_t row3 = LD_S16X4(coef_block + 3 * DCTSIZE);
  v128_t row4 = LD_S16X4(coef_block + 4 * DCTSIZE);
  v128_t row5 = LD_S16X4(coef_block + 5 * DCTSIZE);
  v128_t row6 = LD_S16X4(coef_block + 6 * DCTSIZE);
  v128_t row7 = LD_S16X4(coef_block + 7 * DCTSIZE);

  v128_t quant_row0 = LD_S16X4(quantptr + 0 * DCTSIZE);
  v128_t quant_row1 = LD_S16X4(quantptr + 1 * DCTSIZE);
  v128_t quant_row2 = LD_S16X4(quantptr + 2 * DCTSIZE);
  v128_t quant_row3 = LD_S16X4(quantptr + 3 * DCTSIZE);
  v128_t quant_row4 = LD_S16X4(quantptr + 4 * DCTSIZE);
  v128_t quant_row5 = LD_S16X4(quantptr + 5 * DCTSIZE);
  v128_t quant_row6 = LD_S16X4(quantptr + 6 * DCTSIZE);
  v128_t quant_row7 = LD_S16X4(quantptr + 7 * DCTSIZE);

  v128_t bitmap = wasm_v128_or(row7, row6);
  bitmap = wasm_v128_or(bitmap, row5);
  bitmap = wasm_v128_or(bitmap, row4);
  int64_t bitmap_rows_4567 = LOW64(bitmap);

  if (bitmap_rows_4567 == 0) {
    bitmap = wasm_v128_or(bitmap, row3);
    bitmap = wasm_v128_or(bitmap, row2);
    bitmap = wasm_v128_or(bitmap, row1);
    int64_t left_ac_bitmap = LOW64(bitmap);

    if (left_ac_bitmap == 0) {
      v128_t dcval = SHL16(MUL16(row0, quant_row0), PASS1_BITS);
      /* Interleave of four identical vectors. */
      v128_t w0 = wasm_i8x16_shuffle(dcval, dcval, 0, 1, 0, 1, 0, 1, 0, 1, 2,
                                     3, 2, 3, 2, 3, 2, 3);
      v128_t w1 = wasm_i8x16_shuffle(dcval, dcval, 4, 5, 4, 5, 4, 5, 4, 5, 6,
                                     7, 6, 7, 6, 7, 6, 7);
      wasm_v128_store(workspace_l, w0);
      wasm_v128_store(workspace_l + 8, w1);
      wasm_v128_store(workspace_r, w0);
      wasm_v128_store(workspace_r + 8, w1);
    } else {
      jsimd_idct_islow_wasm_pass1_sparse(row0, row1, row2, row3, quant_row0,
                                         quant_row1, quant_row2, quant_row3,
                                         workspace_l, workspace_r);
    }
  } else {
    jsimd_idct_islow_wasm_pass1_regular(
        row0, row1, row2, row3, row4, row5, row6, row7, quant_row0,
        quant_row1, quant_row2, quant_row3, quant_row4, quant_row5,
        quant_row6, quant_row7, workspace_l, workspace_r);
  }

  /* Compute IDCT first pass on right 4x8 coefficient block. */
  row0 = LD_S16X4(coef_block + 0 * DCTSIZE + 4);
  row1 = LD_S16X4(coef_block + 1 * DCTSIZE + 4);
  row2 = LD_S16X4(coef_block + 2 * DCTSIZE + 4);
  row3 = LD_S16X4(coef_block + 3 * DCTSIZE + 4);
  row4 = LD_S16X4(coef_block + 4 * DCTSIZE + 4);
  row5 = LD_S16X4(coef_block + 5 * DCTSIZE + 4);
  row6 = LD_S16X4(coef_block + 6 * DCTSIZE + 4);
  row7 = LD_S16X4(coef_block + 7 * DCTSIZE + 4);

  quant_row0 = LD_S16X4(quantptr + 0 * DCTSIZE + 4);
  quant_row1 = LD_S16X4(quantptr + 1 * DCTSIZE + 4);
  quant_row2 = LD_S16X4(quantptr + 2 * DCTSIZE + 4);
  quant_row3 = LD_S16X4(quantptr + 3 * DCTSIZE + 4);
  quant_row4 = LD_S16X4(quantptr + 4 * DCTSIZE + 4);
  quant_row5 = LD_S16X4(quantptr + 5 * DCTSIZE + 4);
  quant_row6 = LD_S16X4(quantptr + 6 * DCTSIZE + 4);
  quant_row7 = LD_S16X4(quantptr + 7 * DCTSIZE + 4);

  bitmap = wasm_v128_or(row7, row6);
  bitmap = wasm_v128_or(bitmap, row5);
  bitmap = wasm_v128_or(bitmap, row4);
  bitmap_rows_4567 = LOW64(bitmap);
  bitmap = wasm_v128_or(bitmap, row3);
  bitmap = wasm_v128_or(bitmap, row2);
  bitmap = wasm_v128_or(bitmap, row1);
  int64_t right_ac_bitmap = LOW64(bitmap);

  /* If this remains non-zero, a "regular" second pass will be performed. */
  int64_t right_ac_dc_bitmap = 1;

  if (right_ac_bitmap == 0) {
    bitmap = wasm_v128_or(bitmap, row0);
    right_ac_dc_bitmap = LOW64(bitmap);

    if (right_ac_dc_bitmap != 0) {
      v128_t dcval = SHL16(MUL16(row0, quant_row0), PASS1_BITS);
      v128_t w0 = wasm_i8x16_shuffle(dcval, dcval, 0, 1, 0, 1, 0, 1, 0, 1, 2,
                                     3, 2, 3, 2, 3, 2, 3);
      v128_t w1 = wasm_i8x16_shuffle(dcval, dcval, 4, 5, 4, 5, 4, 5, 4, 5, 6,
                                     7, 6, 7, 6, 7, 6, 7);
      wasm_v128_store(workspace_l + 4 * DCTSIZE / 2, w0);
      wasm_v128_store(workspace_l + 4 * DCTSIZE / 2 + 8, w1);
      wasm_v128_store(workspace_r + 4 * DCTSIZE / 2, w0);
      wasm_v128_store(workspace_r + 4 * DCTSIZE / 2 + 8, w1);
    }
  } else {
    if (bitmap_rows_4567 == 0) {
      jsimd_idct_islow_wasm_pass1_sparse(row0, row1, row2, row3, quant_row0,
                                         quant_row1, quant_row2, quant_row3,
                                         workspace_l + 4 * DCTSIZE / 2,
                                         workspace_r + 4 * DCTSIZE / 2);
    } else {
      jsimd_idct_islow_wasm_pass1_regular(
          row0, row1, row2, row3, row4, row5, row6, row7, quant_row0,
          quant_row1, quant_row2, quant_row3, quant_row4, quant_row5,
          quant_row6, quant_row7, workspace_l + 4 * DCTSIZE / 2,
          workspace_r + 4 * DCTSIZE / 2);
    }
  }

  /* Second pass: compute IDCT on rows in workspace. */
  if (right_ac_dc_bitmap == 0) {
    jsimd_idct_islow_wasm_pass2_sparse(workspace_l, output_buf, output_col,
                                       0);
    jsimd_idct_islow_wasm_pass2_sparse(workspace_r, output_buf, output_col,
                                       4);
  } else {
    jsimd_idct_islow_wasm_pass2_regular(workspace_l, output_buf, output_col,
                                        0);
    jsimd_idct_islow_wasm_pass2_regular(workspace_r, output_buf, output_col,
                                        4);
  }
}

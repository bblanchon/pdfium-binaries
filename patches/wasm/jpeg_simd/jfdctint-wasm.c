/*
 * jfdctint-wasm.c - accurate integer FDCT (WebAssembly SIMD128)
 *
 * Transliteration of jsimd_fdct_islow_neon() from simd/arm/jfdctint-neon.c,
 * which produces exactly the same output as the scalar jpeg_fdct_islow().
 * The NEON load/transpose trickery (vld4q + vuzp, vtrn pyramids) is replaced
 * with a generic zip-shuffle 8x8 transpose; the arithmetic (widening s16*s16
 * multiply-accumulate, rounding narrowing shifts) is preserved exactly.
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
#define DESCALE_P2  (CONST_BITS + PASS1_BITS)

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

/* vmull_lane_s16 / vmlal_lane_s16 against a splatted constant. */
#define MULL_L(v, c)      wasm_i32x4_extmul_low_i16x8(v, c)
#define MULL_H(v, c)      wasm_i32x4_extmul_high_i16x8(v, c)
#define MLAL_L(acc, v, c) wasm_i32x4_add(acc, MULL_L(v, c))
#define MLAL_H(acc, v, c) wasm_i32x4_add(acc, MULL_H(v, c))

/* vrshrn_n_s32 pair: round, arithmetic shift, modular narrow to s16x8. */
#define RSHRN_S32(lo, hi, bias, sh) \
  wasm_i8x16_shuffle(wasm_i32x4_shr(wasm_i32x4_add(lo, bias), sh), \
                     wasm_i32x4_shr(wasm_i32x4_add(hi, bias), sh), 0, 1, 4, \
                     5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29)

#define ZIP16_LO(a, b) \
  wasm_i8x16_shuffle(a, b, 0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20, 21, 6, 7, \
                     22, 23)
#define ZIP16_HI(a, b) \
  wasm_i8x16_shuffle(a, b, 8, 9, 24, 25, 10, 11, 26, 27, 12, 13, 28, 29, 14, \
                     15, 30, 31)
#define ZIP32_LO(a, b) \
  wasm_i8x16_shuffle(a, b, 0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, \
                     22, 23)
#define ZIP32_HI(a, b) \
  wasm_i8x16_shuffle(a, b, 8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, \
                     29, 30, 31)
#define ZIP64_LO(a, b) \
  wasm_i8x16_shuffle(a, b, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, \
                     22, 23)
#define ZIP64_HI(a, b) \
  wasm_i8x16_shuffle(a, b, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, \
                     29, 30, 31)

/* 8x8 transpose of s16 lanes: out vector j holds lane j of every input. */
static inline void transpose_8x8_s16(const v128_t in[8], v128_t out[8])
{
  v128_t t0 = ZIP16_LO(in[0], in[1]);
  v128_t t1 = ZIP16_HI(in[0], in[1]);
  v128_t t2 = ZIP16_LO(in[2], in[3]);
  v128_t t3 = ZIP16_HI(in[2], in[3]);
  v128_t t4 = ZIP16_LO(in[4], in[5]);
  v128_t t5 = ZIP16_HI(in[4], in[5]);
  v128_t t6 = ZIP16_LO(in[6], in[7]);
  v128_t t7 = ZIP16_HI(in[6], in[7]);

  v128_t u0 = ZIP32_LO(t0, t2);
  v128_t u1 = ZIP32_HI(t0, t2);
  v128_t u2 = ZIP32_LO(t1, t3);
  v128_t u3 = ZIP32_HI(t1, t3);
  v128_t u4 = ZIP32_LO(t4, t6);
  v128_t u5 = ZIP32_HI(t4, t6);
  v128_t u6 = ZIP32_LO(t5, t7);
  v128_t u7 = ZIP32_HI(t5, t7);

  out[0] = ZIP64_LO(u0, u4);
  out[1] = ZIP64_HI(u0, u4);
  out[2] = ZIP64_LO(u1, u5);
  out[3] = ZIP64_HI(u1, u5);
  out[4] = ZIP64_LO(u2, u6);
  out[5] = ZIP64_HI(u2, u6);
  out[6] = ZIP64_LO(u3, u7);
  out[7] = ZIP64_HI(u3, u7);
}

void jsimd_fdct_islow_wasm(DCTELEM *data)
{
  const v128_t c_0_298 = wasm_i16x8_splat(F_0_298);
  const v128_t c_n0_390 = wasm_i16x8_splat(-F_0_390);
  const v128_t c_0_541 = wasm_i16x8_splat(F_0_541);
  const v128_t c_0_765 = wasm_i16x8_splat(F_0_765);
  const v128_t c_n0_899 = wasm_i16x8_splat(-F_0_899);
  const v128_t c_1_175 = wasm_i16x8_splat(F_1_175);
  const v128_t c_1_501 = wasm_i16x8_splat(F_1_501);
  const v128_t c_n1_847 = wasm_i16x8_splat(-F_1_847);
  const v128_t c_n1_961 = wasm_i16x8_splat(-F_1_961);
  const v128_t c_2_053 = wasm_i16x8_splat(F_2_053);
  const v128_t c_n2_562 = wasm_i16x8_splat(-F_2_562);
  const v128_t c_3_072 = wasm_i16x8_splat(F_3_072);
  const v128_t bias_p1 = wasm_i32x4_splat(1 << (DESCALE_P1 - 1));
  const v128_t bias_p2 = wasm_i32x4_splat(1 << (DESCALE_P2 - 1));

  /* Load the 8x8 block and transpose so that each vector holds one column,
   * allowing all rows to be processed at once (as the NEON version does via
   * vld4q + vuzp). */
  v128_t rows[8], cols[8];
  rows[0] = wasm_v128_load(data + 0 * DCTSIZE);
  rows[1] = wasm_v128_load(data + 1 * DCTSIZE);
  rows[2] = wasm_v128_load(data + 2 * DCTSIZE);
  rows[3] = wasm_v128_load(data + 3 * DCTSIZE);
  rows[4] = wasm_v128_load(data + 4 * DCTSIZE);
  rows[5] = wasm_v128_load(data + 5 * DCTSIZE);
  rows[6] = wasm_v128_load(data + 6 * DCTSIZE);
  rows[7] = wasm_v128_load(data + 7 * DCTSIZE);
  transpose_8x8_s16(rows, cols);

  v128_t col0 = cols[0], col1 = cols[1], col2 = cols[2], col3 = cols[3];
  v128_t col4 = cols[4], col5 = cols[5], col6 = cols[6], col7 = cols[7];

  /* Pass 1: process rows. */

  v128_t tmp0 = wasm_i16x8_add(col0, col7);
  v128_t tmp7 = wasm_i16x8_sub(col0, col7);
  v128_t tmp1 = wasm_i16x8_add(col1, col6);
  v128_t tmp6 = wasm_i16x8_sub(col1, col6);
  v128_t tmp2 = wasm_i16x8_add(col2, col5);
  v128_t tmp5 = wasm_i16x8_sub(col2, col5);
  v128_t tmp3 = wasm_i16x8_add(col3, col4);
  v128_t tmp4 = wasm_i16x8_sub(col3, col4);

  /* Even part */
  v128_t tmp10 = wasm_i16x8_add(tmp0, tmp3);
  v128_t tmp13 = wasm_i16x8_sub(tmp0, tmp3);
  v128_t tmp11 = wasm_i16x8_add(tmp1, tmp2);
  v128_t tmp12 = wasm_i16x8_sub(tmp1, tmp2);

  col0 = wasm_i16x8_shl(wasm_i16x8_add(tmp10, tmp11), PASS1_BITS);
  col4 = wasm_i16x8_shl(wasm_i16x8_sub(tmp10, tmp11), PASS1_BITS);

  v128_t tmp12_add_tmp13 = wasm_i16x8_add(tmp12, tmp13);
  v128_t z1_l = MULL_L(tmp12_add_tmp13, c_0_541);
  v128_t z1_h = MULL_H(tmp12_add_tmp13, c_0_541);

  col2 = RSHRN_S32(MLAL_L(z1_l, tmp13, c_0_765),
                   MLAL_H(z1_h, tmp13, c_0_765), bias_p1, DESCALE_P1);
  col6 = RSHRN_S32(MLAL_L(z1_l, tmp12, c_n1_847),
                   MLAL_H(z1_h, tmp12, c_n1_847), bias_p1, DESCALE_P1);

  /* Odd part */
  v128_t z1 = wasm_i16x8_add(tmp4, tmp7);
  v128_t z2 = wasm_i16x8_add(tmp5, tmp6);
  v128_t z3 = wasm_i16x8_add(tmp4, tmp6);
  v128_t z4 = wasm_i16x8_add(tmp5, tmp7);
  /* sqrt(2) * c3 */
  v128_t z5_l = MLAL_L(MULL_L(z3, c_1_175), z4, c_1_175);
  v128_t z5_h = MLAL_H(MULL_H(z3, c_1_175), z4, c_1_175);

  v128_t tmp4_l = MULL_L(tmp4, c_0_298);
  v128_t tmp4_h = MULL_H(tmp4, c_0_298);
  v128_t tmp5_l = MULL_L(tmp5, c_2_053);
  v128_t tmp5_h = MULL_H(tmp5, c_2_053);
  v128_t tmp6_l = MULL_L(tmp6, c_3_072);
  v128_t tmp6_h = MULL_H(tmp6, c_3_072);
  v128_t tmp7_l = MULL_L(tmp7, c_1_501);
  v128_t tmp7_h = MULL_H(tmp7, c_1_501);

  z1_l = MULL_L(z1, c_n0_899);
  z1_h = MULL_H(z1, c_n0_899);
  v128_t z2_l = MULL_L(z2, c_n2_562);
  v128_t z2_h = MULL_H(z2, c_n2_562);
  v128_t z3_l = MULL_L(z3, c_n1_961);
  v128_t z3_h = MULL_H(z3, c_n1_961);
  v128_t z4_l = MULL_L(z4, c_n0_390);
  v128_t z4_h = MULL_H(z4, c_n0_390);

  z3_l = wasm_i32x4_add(z3_l, z5_l);
  z3_h = wasm_i32x4_add(z3_h, z5_h);
  z4_l = wasm_i32x4_add(z4_l, z5_l);
  z4_h = wasm_i32x4_add(z4_h, z5_h);

  tmp4_l = wasm_i32x4_add(wasm_i32x4_add(tmp4_l, z1_l), z3_l);
  tmp4_h = wasm_i32x4_add(wasm_i32x4_add(tmp4_h, z1_h), z3_h);
  col7 = RSHRN_S32(tmp4_l, tmp4_h, bias_p1, DESCALE_P1);

  tmp5_l = wasm_i32x4_add(wasm_i32x4_add(tmp5_l, z2_l), z4_l);
  tmp5_h = wasm_i32x4_add(wasm_i32x4_add(tmp5_h, z2_h), z4_h);
  col5 = RSHRN_S32(tmp5_l, tmp5_h, bias_p1, DESCALE_P1);

  tmp6_l = wasm_i32x4_add(wasm_i32x4_add(tmp6_l, z2_l), z3_l);
  tmp6_h = wasm_i32x4_add(wasm_i32x4_add(tmp6_h, z2_h), z3_h);
  col3 = RSHRN_S32(tmp6_l, tmp6_h, bias_p1, DESCALE_P1);

  tmp7_l = wasm_i32x4_add(wasm_i32x4_add(tmp7_l, z1_l), z4_l);
  tmp7_h = wasm_i32x4_add(wasm_i32x4_add(tmp7_h, z1_h), z4_h);
  col1 = RSHRN_S32(tmp7_l, tmp7_h, bias_p1, DESCALE_P1);

  /* Transpose to work on columns in pass 2. */
  cols[0] = col0; cols[1] = col1; cols[2] = col2; cols[3] = col3;
  cols[4] = col4; cols[5] = col5; cols[6] = col6; cols[7] = col7;
  transpose_8x8_s16(cols, rows);

  v128_t row0 = rows[0], row1 = rows[1], row2 = rows[2], row3 = rows[3];
  v128_t row4 = rows[4], row5 = rows[5], row6 = rows[6], row7 = rows[7];

  /* Pass 2: process columns. */

  tmp0 = wasm_i16x8_add(row0, row7);
  tmp7 = wasm_i16x8_sub(row0, row7);
  tmp1 = wasm_i16x8_add(row1, row6);
  tmp6 = wasm_i16x8_sub(row1, row6);
  tmp2 = wasm_i16x8_add(row2, row5);
  tmp5 = wasm_i16x8_sub(row2, row5);
  tmp3 = wasm_i16x8_add(row3, row4);
  tmp4 = wasm_i16x8_sub(row3, row4);

  /* Even part */
  tmp10 = wasm_i16x8_add(tmp0, tmp3);
  tmp13 = wasm_i16x8_sub(tmp0, tmp3);
  tmp11 = wasm_i16x8_add(tmp1, tmp2);
  tmp12 = wasm_i16x8_sub(tmp1, tmp2);

  /* vrshrq_n_s16(x, PASS1_BITS) */
  const v128_t rnd_pass1 = wasm_i16x8_splat(1 << (PASS1_BITS - 1));
  row0 = wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_add(tmp10, tmp11),
                                       rnd_pass1), PASS1_BITS);
  row4 = wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_sub(tmp10, tmp11),
                                       rnd_pass1), PASS1_BITS);

  tmp12_add_tmp13 = wasm_i16x8_add(tmp12, tmp13);
  z1_l = MULL_L(tmp12_add_tmp13, c_0_541);
  z1_h = MULL_H(tmp12_add_tmp13, c_0_541);

  row2 = RSHRN_S32(MLAL_L(z1_l, tmp13, c_0_765),
                   MLAL_H(z1_h, tmp13, c_0_765), bias_p2, DESCALE_P2);
  row6 = RSHRN_S32(MLAL_L(z1_l, tmp12, c_n1_847),
                   MLAL_H(z1_h, tmp12, c_n1_847), bias_p2, DESCALE_P2);

  /* Odd part */
  z1 = wasm_i16x8_add(tmp4, tmp7);
  z2 = wasm_i16x8_add(tmp5, tmp6);
  z3 = wasm_i16x8_add(tmp4, tmp6);
  z4 = wasm_i16x8_add(tmp5, tmp7);
  /* sqrt(2) * c3 */
  z5_l = MLAL_L(MULL_L(z3, c_1_175), z4, c_1_175);
  z5_h = MLAL_H(MULL_H(z3, c_1_175), z4, c_1_175);

  tmp4_l = MULL_L(tmp4, c_0_298);
  tmp4_h = MULL_H(tmp4, c_0_298);
  tmp5_l = MULL_L(tmp5, c_2_053);
  tmp5_h = MULL_H(tmp5, c_2_053);
  tmp6_l = MULL_L(tmp6, c_3_072);
  tmp6_h = MULL_H(tmp6, c_3_072);
  tmp7_l = MULL_L(tmp7, c_1_501);
  tmp7_h = MULL_H(tmp7, c_1_501);

  z1_l = MULL_L(z1, c_n0_899);
  z1_h = MULL_H(z1, c_n0_899);
  z2_l = MULL_L(z2, c_n2_562);
  z2_h = MULL_H(z2, c_n2_562);
  z3_l = MULL_L(z3, c_n1_961);
  z3_h = MULL_H(z3, c_n1_961);
  z4_l = MULL_L(z4, c_n0_390);
  z4_h = MULL_H(z4, c_n0_390);

  z3_l = wasm_i32x4_add(z3_l, z5_l);
  z3_h = wasm_i32x4_add(z3_h, z5_h);
  z4_l = wasm_i32x4_add(z4_l, z5_l);
  z4_h = wasm_i32x4_add(z4_h, z5_h);

  tmp4_l = wasm_i32x4_add(wasm_i32x4_add(tmp4_l, z1_l), z3_l);
  tmp4_h = wasm_i32x4_add(wasm_i32x4_add(tmp4_h, z1_h), z3_h);
  row7 = RSHRN_S32(tmp4_l, tmp4_h, bias_p2, DESCALE_P2);

  tmp5_l = wasm_i32x4_add(wasm_i32x4_add(tmp5_l, z2_l), z4_l);
  tmp5_h = wasm_i32x4_add(wasm_i32x4_add(tmp5_h, z2_h), z4_h);
  row5 = RSHRN_S32(tmp5_l, tmp5_h, bias_p2, DESCALE_P2);

  tmp6_l = wasm_i32x4_add(wasm_i32x4_add(tmp6_l, z2_l), z3_l);
  tmp6_h = wasm_i32x4_add(wasm_i32x4_add(tmp6_h, z2_h), z3_h);
  row3 = RSHRN_S32(tmp6_l, tmp6_h, bias_p2, DESCALE_P2);

  tmp7_l = wasm_i32x4_add(wasm_i32x4_add(tmp7_l, z1_l), z4_l);
  tmp7_h = wasm_i32x4_add(wasm_i32x4_add(tmp7_h, z1_h), z4_h);
  row1 = RSHRN_S32(tmp7_l, tmp7_h, bias_p2, DESCALE_P2);

  wasm_v128_store(data + 0 * DCTSIZE, row0);
  wasm_v128_store(data + 1 * DCTSIZE, row1);
  wasm_v128_store(data + 2 * DCTSIZE, row2);
  wasm_v128_store(data + 3 * DCTSIZE, row3);
  wasm_v128_store(data + 4 * DCTSIZE, row4);
  wasm_v128_store(data + 5 * DCTSIZE, row5);
  wasm_v128_store(data + 6 * DCTSIZE, row6);
  wasm_v128_store(data + 7 * DCTSIZE, row7);
}

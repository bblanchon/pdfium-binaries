/*
 * jccolext-wasm.c - colorspace conversion for compression (WebAssembly SIMD128)
 *
 * This file is included by jccolor-wasm.c, once per RGB pixel layout,
 * mirroring how simd/arm/aarch64/jccolext-neon.c is included by
 * jccolor-neon.c. The arithmetic matches the NEON version exactly: widening
 * u16->u32 multiply-accumulate against the 2^-16 fixed-point constants, a
 * rounding >>16 for Y and a truncating >>16 (with the pre-added 32767 + 128
 * bias) for Cb/Cr.
 */

/* This file is included by jccolor-wasm.c */


/* vrshrn_n_u32(x, 16) on a pair of u32x4 vectors: add the rounding bias and
 * take bytes 2..3 of every u32 lane (modular narrow to u16). */
#define RSHRN16(lo, hi, half) \
  wasm_i8x16_shuffle(wasm_i32x4_add(lo, half), wasm_i32x4_add(hi, half), 2, \
                     3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31)
/* vshrn_n_u32(x, 16) on a pair of u32x4 vectors (truncating). */
#define SHRN16(lo, hi) \
  wasm_i8x16_shuffle(lo, hi, 2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, \
                     27, 30, 31)

void jsimd_rgb_ycc_convert_wasm_internal(JDIMENSION image_width,
                                         JSAMPARRAY input_buf,
                                         JSAMPIMAGE output_buf,
                                         JDIMENSION output_row, int num_rows)
{
  JSAMPROW inptr;
  JSAMPROW outptr0, outptr1, outptr2;
  /* Temporary buffer for the final (image_width % 16) pixels in a row, so
   * the vector loads never read past the caller's buffer. */
  uint8_t tmp_buf[16 * RGB_PIXELSIZE];

  const v128_t c_y_r = wasm_i32x4_splat(F_0_298);
  const v128_t c_y_g = wasm_i32x4_splat(F_0_587);
  const v128_t c_y_b = wasm_i32x4_splat(F_0_113);
  const v128_t c_cb_r = wasm_i32x4_splat(F_0_168);
  const v128_t c_cb_g = wasm_i32x4_splat(F_0_331);
  const v128_t c_half = wasm_i32x4_splat(F_0_500);
  const v128_t c_cr_g = wasm_i32x4_splat(F_0_418);
  const v128_t c_cr_b = wasm_i32x4_splat(F_0_081);
  /* 128.5 in 16.16 fixed point: makes the truncating shift round. */
  const v128_t scaled_128_5 = wasm_i32x4_splat((128 << 16) + 32767);
  const v128_t rnd_half = wasm_i32x4_splat(32768);

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    output_row++;

    int cols_remaining = image_width;
    while (cols_remaining > 0) {
      if (cols_remaining < 16) {
        /* Run the same 16-pixel body once on a copy; the outputs may write
         * up to 16 bytes, which the padded sample buffers permit. */
        memcpy(tmp_buf, inptr, cols_remaining * RGB_PIXELSIZE);
        inptr = tmp_buf;
        cols_remaining = 16;
      }

#if RGB_PIXELSIZE == 4
      v128_t planes[4];
      wasm_load_deinterleave_4(inptr, planes);
#else
      v128_t planes[3];
      wasm_load_deinterleave_3(inptr, planes);
#endif
      v128_t r = planes[RGB_RED];
      v128_t g = planes[RGB_GREEN];
      v128_t b = planes[RGB_BLUE];
      v128_t r_l = wasm_u16x8_extend_low_u8x16(r);
      v128_t g_l = wasm_u16x8_extend_low_u8x16(g);
      v128_t b_l = wasm_u16x8_extend_low_u8x16(b);
      v128_t r_h = wasm_u16x8_extend_high_u8x16(r);
      v128_t g_h = wasm_u16x8_extend_high_u8x16(g);
      v128_t b_h = wasm_u16x8_extend_high_u8x16(b);

#define MULL(v, c)  wasm_i32x4_mul(v, c)
#define EXT_LL(v)   wasm_u32x4_extend_low_u16x8(v)
#define EXT_LH(v)   wasm_u32x4_extend_high_u16x8(v)

      /* Y = 0.29900 * R + 0.58700 * G + 0.11400 * B */
      v128_t y_ll = wasm_i32x4_add(
          wasm_i32x4_add(MULL(EXT_LL(r_l), c_y_r), MULL(EXT_LL(g_l), c_y_g)),
          MULL(EXT_LL(b_l), c_y_b));
      v128_t y_lh = wasm_i32x4_add(
          wasm_i32x4_add(MULL(EXT_LH(r_l), c_y_r), MULL(EXT_LH(g_l), c_y_g)),
          MULL(EXT_LH(b_l), c_y_b));
      v128_t y_hl = wasm_i32x4_add(
          wasm_i32x4_add(MULL(EXT_LL(r_h), c_y_r), MULL(EXT_LL(g_h), c_y_g)),
          MULL(EXT_LL(b_h), c_y_b));
      v128_t y_hh = wasm_i32x4_add(
          wasm_i32x4_add(MULL(EXT_LH(r_h), c_y_r), MULL(EXT_LH(g_h), c_y_g)),
          MULL(EXT_LH(b_h), c_y_b));

      /* Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B + 128.5 (u32 wrap,
       * exactly as the NEON vmlsl accumulation wraps) */
      v128_t cb_ll = wasm_i32x4_add(
          wasm_i32x4_sub(wasm_i32x4_sub(scaled_128_5,
                                        MULL(EXT_LL(r_l), c_cb_r)),
                         MULL(EXT_LL(g_l), c_cb_g)),
          MULL(EXT_LL(b_l), c_half));
      v128_t cb_lh = wasm_i32x4_add(
          wasm_i32x4_sub(wasm_i32x4_sub(scaled_128_5,
                                        MULL(EXT_LH(r_l), c_cb_r)),
                         MULL(EXT_LH(g_l), c_cb_g)),
          MULL(EXT_LH(b_l), c_half));
      v128_t cb_hl = wasm_i32x4_add(
          wasm_i32x4_sub(wasm_i32x4_sub(scaled_128_5,
                                        MULL(EXT_LL(r_h), c_cb_r)),
                         MULL(EXT_LL(g_h), c_cb_g)),
          MULL(EXT_LL(b_h), c_half));
      v128_t cb_hh = wasm_i32x4_add(
          wasm_i32x4_sub(wasm_i32x4_sub(scaled_128_5,
                                        MULL(EXT_LH(r_h), c_cb_r)),
                         MULL(EXT_LH(g_h), c_cb_g)),
          MULL(EXT_LH(b_h), c_half));

      /* Cr = 0.50000 * R - 0.41869 * G - 0.08131 * B + 128.5 */
      v128_t cr_ll = wasm_i32x4_sub(
          wasm_i32x4_sub(wasm_i32x4_add(scaled_128_5,
                                        MULL(EXT_LL(r_l), c_half)),
                         MULL(EXT_LL(g_l), c_cr_g)),
          MULL(EXT_LL(b_l), c_cr_b));
      v128_t cr_lh = wasm_i32x4_sub(
          wasm_i32x4_sub(wasm_i32x4_add(scaled_128_5,
                                        MULL(EXT_LH(r_l), c_half)),
                         MULL(EXT_LH(g_l), c_cr_g)),
          MULL(EXT_LH(b_l), c_cr_b));
      v128_t cr_hl = wasm_i32x4_sub(
          wasm_i32x4_sub(wasm_i32x4_add(scaled_128_5,
                                        MULL(EXT_LL(r_h), c_half)),
                         MULL(EXT_LL(g_h), c_cr_g)),
          MULL(EXT_LL(b_h), c_cr_b));
      v128_t cr_hh = wasm_i32x4_sub(
          wasm_i32x4_sub(wasm_i32x4_add(scaled_128_5,
                                        MULL(EXT_LH(r_h), c_half)),
                         MULL(EXT_LH(g_h), c_cr_g)),
          MULL(EXT_LH(b_h), c_cr_b));

#undef MULL
#undef EXT_LL
#undef EXT_LH

      /* Descale to u16 (Y rounds, Cb/Cr truncate - the bias already rounds
       * them), then narrow to u8. All values fit 0..255, so the saturating
       * narrow is exact. */
      v128_t y_l = RSHRN16(y_ll, y_lh, rnd_half);
      v128_t y_h = RSHRN16(y_hl, y_hh, rnd_half);
      v128_t cb_l = SHRN16(cb_ll, cb_lh);
      v128_t cb_h = SHRN16(cb_hl, cb_hh);
      v128_t cr_l = SHRN16(cr_ll, cr_lh);
      v128_t cr_h = SHRN16(cr_hl, cr_hh);

      wasm_v128_store(outptr0, wasm_u8x16_narrow_i16x8(y_l, y_h));
      wasm_v128_store(outptr1, wasm_u8x16_narrow_i16x8(cb_l, cb_h));
      wasm_v128_store(outptr2, wasm_u8x16_narrow_i16x8(cr_l, cr_h));

      inptr += (16 * RGB_PIXELSIZE);
      outptr0 += 16;
      outptr1 += 16;
      outptr2 += 16;
      cols_remaining -= 16;
    }
  }
}

#undef RSHRN16
#undef SHRN16

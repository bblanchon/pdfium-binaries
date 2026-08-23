/*
 * jdcolext-wasm.c - colorspace conversion (WebAssembly SIMD128)
 *
 * Template for YCbCr -> RGB conversion, included by jdcolor-wasm.c once per
 * output pixel layout (RGB_RED/RGB_GREEN/RGB_BLUE/RGB_ALPHA/RGB_PIXELSIZE).
 *
 * Unlike the NEON/SSE2 kernels, this implementation is bit-identical to
 * libjpeg-turbo's scalar C path (src/jdcolor.c + src/jdcolext.c): it uses the
 * same fixed-point constants (FIX(x) = round(x * 2^16)), the same +ONE_HALF
 * rounding and arithmetic right shifts, and the final saturating narrow to
 * u8 performs exactly the scalar path's range_limit clamp (all intermediate
 * sums fit in [-227, 482]).
 */

/* Widen the low/high halves of a u8x16 to two i32x4 pairs. */
#define WASM_U8_TO_U16(v, lo, hi) \
  lo = wasm_u16x8_extend_low_u8x16(v); \
  hi = wasm_u16x8_extend_high_u8x16(v)

void jsimd_ycc_rgb_convert_wasm_internal(JDIMENSION output_width,
                                         JSAMPIMAGE input_buf,
                                         JDIMENSION input_row,
                                         JSAMPARRAY output_buf, int num_rows)
{
  const v128_t c128 = wasm_i32x4_splat(128);
  const v128_t half = wasm_i32x4_splat(ONE_HALF_WASM);
  const v128_t f_r = wasm_i32x4_splat(FIX_WASM(1.40200));
  const v128_t f_b = wasm_i32x4_splat(FIX_WASM(1.77200));
  const v128_t f_gcr = wasm_i32x4_splat(-FIX_WASM(0.71414));
  const v128_t f_gcb = wasm_i32x4_splat(-FIX_WASM(0.34414));

  while (--num_rows >= 0) {
    JSAMPROW inptr0 = input_buf[0][input_row];
    JSAMPROW inptr1 = input_buf[1][input_row];
    JSAMPROW inptr2 = input_buf[2][input_row];
    JSAMPROW outptr = *output_buf++;
    JDIMENSION col = 0;
    input_row++;

    for (; col + 16 <= output_width; col += 16) {
      v128_t y8 = wasm_v128_load(inptr0 + col);
      v128_t cb8 = wasm_v128_load(inptr1 + col);
      v128_t cr8 = wasm_v128_load(inptr2 + col);

      v128_t y16l, y16h, cb16l, cb16h, cr16l, cr16h;
      WASM_U8_TO_U16(y8, y16l, y16h);
      WASM_U8_TO_U16(cb8, cb16l, cb16h);
      WASM_U8_TO_U16(cr8, cr16l, cr16h);

      v128_t r16[2], g16[2], b16[2];
      int half_idx;
      for (half_idx = 0; half_idx < 2; half_idx++) {
        v128_t y16 = half_idx ? y16h : y16l;
        v128_t cb16 = half_idx ? cb16h : cb16l;
        v128_t cr16 = half_idx ? cr16h : cr16l;
        v128_t r32[2], g32[2], b32[2];
        int q;
        for (q = 0; q < 2; q++) {
          v128_t y32 = q ? wasm_u32x4_extend_high_u16x8(y16)
                         : wasm_u32x4_extend_low_u16x8(y16);
          v128_t xb = wasm_i32x4_sub(q ? wasm_u32x4_extend_high_u16x8(cb16)
                                       : wasm_u32x4_extend_low_u16x8(cb16),
                                     c128);
          v128_t xr = wasm_i32x4_sub(q ? wasm_u32x4_extend_high_u16x8(cr16)
                                       : wasm_u32x4_extend_low_u16x8(cr16),
                                     c128);
          /* R = y + ((FIX(1.40200) * xr + ONE_HALF) >> SCALEBITS) */
          r32[q] = wasm_i32x4_add(
              y32, wasm_i32x4_shr(
                       wasm_i32x4_add(wasm_i32x4_mul(f_r, xr), half), 16));
          /* G = y + ((-FIX(0.34414) * xb + ONE_HALF - FIX(0.71414) * xr)
                      >> SCALEBITS) */
          g32[q] = wasm_i32x4_add(
              y32,
              wasm_i32x4_shr(
                  wasm_i32x4_add(wasm_i32x4_add(wasm_i32x4_mul(f_gcb, xb),
                                                wasm_i32x4_mul(f_gcr, xr)),
                                 half),
                  16));
          /* B = y + ((FIX(1.77200) * xb + ONE_HALF) >> SCALEBITS) */
          b32[q] = wasm_i32x4_add(
              y32, wasm_i32x4_shr(
                       wasm_i32x4_add(wasm_i32x4_mul(f_b, xb), half), 16));
        }
        r16[half_idx] = wasm_i16x8_narrow_i32x4(r32[0], r32[1]);
        g16[half_idx] = wasm_i16x8_narrow_i32x4(g32[0], g32[1]);
        b16[half_idx] = wasm_i16x8_narrow_i32x4(b32[0], b32[1]);
      }
      /* Saturating narrow to u8: exactly range_limit[] for our value range. */
      v128_t ch[4];
      ch[RGB_RED] = wasm_u8x16_narrow_i16x8(r16[0], r16[1]);
      ch[RGB_GREEN] = wasm_u8x16_narrow_i16x8(g16[0], g16[1]);
      ch[RGB_BLUE] = wasm_u8x16_narrow_i16x8(b16[0], b16[1]);

#if RGB_PIXELSIZE == 4
      ch[RGB_ALPHA] = wasm_i8x16_splat((int8_t)0xFF);
      /* Interleave four channel vectors: zip bytes, then zip 16-bit pairs. */
      {
        v128_t p_lo = wasm_i8x16_shuffle(ch[0], ch[1], 0, 16, 1, 17, 2, 18, 3,
                                         19, 4, 20, 5, 21, 6, 22, 7, 23);
        v128_t p_hi = wasm_i8x16_shuffle(ch[0], ch[1], 8, 24, 9, 25, 10, 26,
                                         11, 27, 12, 28, 13, 29, 14, 30, 15,
                                         31);
        v128_t q_lo = wasm_i8x16_shuffle(ch[2], ch[3], 0, 16, 1, 17, 2, 18, 3,
                                         19, 4, 20, 5, 21, 6, 22, 7, 23);
        v128_t q_hi = wasm_i8x16_shuffle(ch[2], ch[3], 8, 24, 9, 25, 10, 26,
                                         11, 27, 12, 28, 13, 29, 14, 30, 15,
                                         31);
        wasm_v128_store(outptr,
                        wasm_i8x16_shuffle(p_lo, q_lo, 0, 1, 16, 17, 2, 3, 18,
                                           19, 4, 5, 20, 21, 6, 7, 22, 23));
        wasm_v128_store(outptr + 16,
                        wasm_i8x16_shuffle(p_lo, q_lo, 8, 9, 24, 25, 10, 11,
                                           26, 27, 12, 13, 28, 29, 14, 15, 30,
                                           31));
        wasm_v128_store(outptr + 32,
                        wasm_i8x16_shuffle(p_hi, q_hi, 0, 1, 16, 17, 2, 3, 18,
                                           19, 4, 5, 20, 21, 6, 7, 22, 23));
        wasm_v128_store(outptr + 48,
                        wasm_i8x16_shuffle(p_hi, q_hi, 8, 9, 24, 25, 10, 11,
                                           26, 27, 12, 13, 28, 29, 14, 15, 30,
                                           31));
      }
      outptr += 64;
#else
      /* Interleave three channel vectors into 48 bytes. ch[0], ch[1], ch[2]
       * are the bytes at offsets 0, 1, 2 of each pixel. */
      {
        v128_t s0 = wasm_i8x16_shuffle(ch[0], ch[1], 0, 16, 0, 1, 17, 0, 2,
                                       18, 0, 3, 19, 0, 4, 20, 0, 5);
        v128_t o0 = wasm_i8x16_shuffle(s0, ch[2], 0, 1, 16, 3, 4, 17, 6, 7,
                                       18, 9, 10, 19, 12, 13, 20, 15);
        v128_t s1 = wasm_i8x16_shuffle(ch[0], ch[1], 21, 0, 6, 22, 0, 7, 23,
                                       0, 8, 24, 0, 9, 25, 0, 10, 26);
        v128_t o1 = wasm_i8x16_shuffle(s1, ch[2], 0, 21, 2, 3, 22, 5, 6, 23,
                                       8, 9, 24, 11, 12, 25, 14, 15);
        v128_t s2 = wasm_i8x16_shuffle(ch[0], ch[1], 0, 11, 27, 0, 12, 28, 0,
                                       13, 29, 0, 14, 30, 0, 15, 31, 0);
        v128_t o2 = wasm_i8x16_shuffle(s2, ch[2], 26, 1, 2, 27, 4, 5, 28, 7,
                                       8, 29, 10, 11, 30, 13, 14, 31);
        wasm_v128_store(outptr, o0);
        wasm_v128_store(outptr + 16, o1);
        wasm_v128_store(outptr + 32, o2);
      }
      outptr += 48;
#endif
    }

    /* Scalar tail: identical arithmetic to src/jdcolext.c. */
    for (; col < output_width; col++) {
      int y = inptr0[col];
      int xb = inptr1[col] - 128;
      int xr = inptr2[col] - 128;
      int r = y + (int)((FIX_WASM(1.40200) * xr + ONE_HALF_WASM) >> 16);
      int g = y + (int)((-FIX_WASM(0.34414) * xb + ONE_HALF_WASM -
                         FIX_WASM(0.71414) * xr) >> 16);
      int b = y + (int)((FIX_WASM(1.77200) * xb + ONE_HALF_WASM) >> 16);
      outptr[RGB_RED] = (JSAMPLE)(r < 0 ? 0 : (r > 255 ? 255 : r));
      outptr[RGB_GREEN] = (JSAMPLE)(g < 0 ? 0 : (g > 255 ? 255 : g));
      outptr[RGB_BLUE] = (JSAMPLE)(b < 0 ? 0 : (b > 255 ? 255 : b));
#ifdef RGB_ALPHA
      outptr[RGB_ALPHA] = 0xFF;
#endif
      outptr += RGB_PIXELSIZE;
    }
  }
}

#undef WASM_U8_TO_U16

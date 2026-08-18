/*
 * jsimd.c - SIMD dispatch for WebAssembly (wasm_simd128)
 *
 * Based on the structure of simd/arm/aarch64/jsimd.c. Every kernel reports
 * "not supported" except the ones ported to wasm SIMD below. Unlike the
 * NEON/SSE2 implementations, the wasm kernels are written to be bit-identical
 * to libjpeg-turbo's scalar C paths, so enabling them does not change decoded
 * output.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

/* Ported kernels. */
extern void jsimd_h2v2_fancy_upsample_wasm(int max_v_samp_factor,
                                           JDIMENSION downsampled_width,
                                           JSAMPARRAY input_data,
                                           JSAMPARRAY *output_data_ptr);
extern void jsimd_idct_islow_wasm(void *dct_table, JCOEFPTR coef_block,
                                  JSAMPARRAY output_buf,
                                  JDIMENSION output_col);
extern void jsimd_ycc_rgb_convert_wasm(JDIMENSION output_width,
                                       JSAMPIMAGE input_buf,
                                       JDIMENSION input_row,
                                       JSAMPARRAY output_buf, int num_rows);
extern void jsimd_ycc_extrgb_convert_wasm(JDIMENSION output_width,
                                          JSAMPIMAGE input_buf,
                                          JDIMENSION input_row,
                                          JSAMPARRAY output_buf, int num_rows);
extern void jsimd_ycc_extrgbx_convert_wasm(JDIMENSION output_width,
                                           JSAMPIMAGE input_buf,
                                           JDIMENSION input_row,
                                           JSAMPARRAY output_buf,
                                           int num_rows);
extern void jsimd_ycc_extbgr_convert_wasm(JDIMENSION output_width,
                                          JSAMPIMAGE input_buf,
                                          JDIMENSION input_row,
                                          JSAMPARRAY output_buf, int num_rows);
extern void jsimd_ycc_extbgrx_convert_wasm(JDIMENSION output_width,
                                           JSAMPIMAGE input_buf,
                                           JDIMENSION input_row,
                                           JSAMPARRAY output_buf,
                                           int num_rows);
extern void jsimd_ycc_extxbgr_convert_wasm(JDIMENSION output_width,
                                           JSAMPIMAGE input_buf,
                                           JDIMENSION input_row,
                                           JSAMPARRAY output_buf,
                                           int num_rows);
extern void jsimd_ycc_extxrgb_convert_wasm(JDIMENSION output_width,
                                           JSAMPIMAGE input_buf,
                                           JDIMENSION input_row,
                                           JSAMPARRAY output_buf,
                                           int num_rows);
extern void jsimd_rgb_ycc_convert_wasm(JDIMENSION image_width,
                                       JSAMPARRAY input_buf,
                                       JSAMPIMAGE output_buf,
                                       JDIMENSION output_row, int num_rows);
extern void jsimd_extrgb_ycc_convert_wasm(JDIMENSION image_width,
                                          JSAMPARRAY input_buf,
                                          JSAMPIMAGE output_buf,
                                          JDIMENSION output_row, int num_rows);
extern void jsimd_extrgbx_ycc_convert_wasm(JDIMENSION image_width,
                                           JSAMPARRAY input_buf,
                                           JSAMPIMAGE output_buf,
                                           JDIMENSION output_row,
                                           int num_rows);
extern void jsimd_extbgr_ycc_convert_wasm(JDIMENSION image_width,
                                          JSAMPARRAY input_buf,
                                          JSAMPIMAGE output_buf,
                                          JDIMENSION output_row, int num_rows);
extern void jsimd_extbgrx_ycc_convert_wasm(JDIMENSION image_width,
                                           JSAMPARRAY input_buf,
                                           JSAMPIMAGE output_buf,
                                           JDIMENSION output_row,
                                           int num_rows);
extern void jsimd_extxbgr_ycc_convert_wasm(JDIMENSION image_width,
                                           JSAMPARRAY input_buf,
                                           JSAMPIMAGE output_buf,
                                           JDIMENSION output_row,
                                           int num_rows);
extern void jsimd_extxrgb_ycc_convert_wasm(JDIMENSION image_width,
                                           JSAMPARRAY input_buf,
                                           JSAMPIMAGE output_buf,
                                           JDIMENSION output_row,
                                           int num_rows);

GLOBAL(int)
jsimd_can_rgb_ycc(void)
{
  /* Same preconditions as the other SIMD backends. */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  return 1;
}

GLOBAL(int)
jsimd_can_rgb_gray(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_ycc_rgb(void)
{
  /* Same preconditions as the other SIMD backends. */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  return 1;
}

GLOBAL(int)
jsimd_can_ycc_rgb565(void)
{
  return 0;
}

GLOBAL(void)
jsimd_rgb_ycc_convert(j_compress_ptr cinfo, JSAMPARRAY input_buf, JSAMPIMAGE output_buf, JDIMENSION output_row, int num_rows)
{
  void (*wasmfct) (JDIMENSION, JSAMPARRAY, JSAMPIMAGE, JDIMENSION, int);

  switch (cinfo->in_color_space) {
  case JCS_EXT_RGB:
    wasmfct = jsimd_extrgb_ycc_convert_wasm;
    break;
  case JCS_EXT_RGBX:
  case JCS_EXT_RGBA:
    wasmfct = jsimd_extrgbx_ycc_convert_wasm;
    break;
  case JCS_EXT_BGR:
    wasmfct = jsimd_extbgr_ycc_convert_wasm;
    break;
  case JCS_EXT_BGRX:
  case JCS_EXT_BGRA:
    wasmfct = jsimd_extbgrx_ycc_convert_wasm;
    break;
  case JCS_EXT_XBGR:
  case JCS_EXT_ABGR:
    wasmfct = jsimd_extxbgr_ycc_convert_wasm;
    break;
  case JCS_EXT_XRGB:
  case JCS_EXT_ARGB:
    wasmfct = jsimd_extxrgb_ycc_convert_wasm;
    break;
  default:
    wasmfct = jsimd_rgb_ycc_convert_wasm;
    break;
  }

  wasmfct(cinfo->image_width, input_buf, output_buf, output_row, num_rows);
}

GLOBAL(void)
jsimd_rgb_gray_convert(j_compress_ptr cinfo, JSAMPARRAY input_buf, JSAMPIMAGE output_buf, JDIMENSION output_row, int num_rows)
{
}

GLOBAL(void)
jsimd_ycc_rgb_convert(j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows)
{
  void (*wasmfct) (JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY, int);

  switch (cinfo->out_color_space) {
  case JCS_EXT_RGB:
    wasmfct = jsimd_ycc_extrgb_convert_wasm;
    break;
  case JCS_EXT_RGBX:
  case JCS_EXT_RGBA:
    wasmfct = jsimd_ycc_extrgbx_convert_wasm;
    break;
  case JCS_EXT_BGR:
    wasmfct = jsimd_ycc_extbgr_convert_wasm;
    break;
  case JCS_EXT_BGRX:
  case JCS_EXT_BGRA:
    wasmfct = jsimd_ycc_extbgrx_convert_wasm;
    break;
  case JCS_EXT_XBGR:
  case JCS_EXT_ABGR:
    wasmfct = jsimd_ycc_extxbgr_convert_wasm;
    break;
  case JCS_EXT_XRGB:
  case JCS_EXT_ARGB:
    wasmfct = jsimd_ycc_extxrgb_convert_wasm;
    break;
  default:
    wasmfct = jsimd_ycc_rgb_convert_wasm;
    break;
  }

  wasmfct(cinfo->output_width, input_buf, input_row, output_buf, num_rows);
}

GLOBAL(void)
jsimd_ycc_rgb565_convert(j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows)
{
}

GLOBAL(int)
jsimd_can_h2v2_downsample(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_downsample(void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_downsample(j_compress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY output_data)
{
}

GLOBAL(void)
jsimd_h2v1_downsample(j_compress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY output_data)
{
}

GLOBAL(int)
jsimd_can_h2v2_upsample(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_upsample(void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
}

GLOBAL(void)
jsimd_h2v1_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
}

GLOBAL(int)
jsimd_can_h2v2_fancy_upsample(void)
{
  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  return 1;
}

GLOBAL(int)
jsimd_can_h2v1_fancy_upsample(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h1v2_fancy_upsample(void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_fancy_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
  jsimd_h2v2_fancy_upsample_wasm(cinfo->max_v_samp_factor,
                                 compptr->downsampled_width, input_data,
                                 output_data_ptr);
}

GLOBAL(void)
jsimd_h2v1_fancy_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
}

GLOBAL(void)
jsimd_h1v2_fancy_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr, JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
}

GLOBAL(int)
jsimd_can_h2v2_merged_upsample(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_merged_upsample(void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_merged_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf)
{
}

GLOBAL(void)
jsimd_h2v1_merged_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf)
{
}

GLOBAL(int)
jsimd_can_convsamp(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_convsamp_float(void)
{
  return 0;
}

GLOBAL(void)
jsimd_convsamp(JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM *workspace)
{
}

GLOBAL(void)
jsimd_convsamp_float(JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace)
{
}

GLOBAL(int)
jsimd_can_fdct_islow(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_fdct_ifast(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_fdct_float(void)
{
  return 0;
}

GLOBAL(void)
jsimd_fdct_islow(DCTELEM *data)
{
}

GLOBAL(void)
jsimd_fdct_ifast(DCTELEM *data)
{
}

GLOBAL(void)
jsimd_fdct_float(FAST_FLOAT *data)
{
}

GLOBAL(int)
jsimd_can_quantize(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_quantize_float(void)
{
  return 0;
}

GLOBAL(void)
jsimd_quantize(JCOEFPTR coef_block, DCTELEM *divisors, DCTELEM *workspace)
{
}

GLOBAL(void)
jsimd_quantize_float(JCOEFPTR coef_block, FAST_FLOAT *divisors, FAST_FLOAT *workspace)
{
}

GLOBAL(int)
jsimd_can_idct_2x2(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_idct_4x4(void)
{
  return 0;
}

GLOBAL(void)
jsimd_idct_2x2(j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col)
{
}

GLOBAL(void)
jsimd_idct_4x4(j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col)
{
}

GLOBAL(int)
jsimd_can_idct_islow(void)
{
  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  return 1;
}

GLOBAL(int)
jsimd_can_idct_ifast(void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_idct_float(void)
{
  return 0;
}

GLOBAL(void)
jsimd_idct_islow(j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col)
{
  jsimd_idct_islow_wasm(compptr->dct_table, coef_block, output_buf,
                        output_col);
}

GLOBAL(void)
jsimd_idct_ifast(j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col)
{
}

GLOBAL(void)
jsimd_idct_float(j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col)
{
}

GLOBAL(int)
jsimd_can_huff_encode_one_block(void)
{
  return 0;
}

GLOBAL(JOCTET *)
jsimd_huff_encode_one_block(void *state, JOCTET *buffer, JCOEFPTR block, int last_dc_val, c_derived_tbl *dctbl, c_derived_tbl *actbl)
{
  return NULL;
}

GLOBAL(int)
jsimd_can_encode_mcu_AC_first_prepare(void)
{
  return 0;
}

GLOBAL(void)
jsimd_encode_mcu_AC_first_prepare(const JCOEF *block,
                                  const int *jpeg_natural_order_start, int Sl,
                                  int Al, UJCOEF *values, size_t *zerobits)
{
}

GLOBAL(int)
jsimd_can_encode_mcu_AC_refine_prepare(void)
{
  return 0;
}

GLOBAL(int)
jsimd_encode_mcu_AC_refine_prepare(const JCOEF *block,
                                   const int *jpeg_natural_order_start,
                                   int Sl, int Al, UJCOEF *absvalues,
                                   size_t *bits)
{
  return 0;
}

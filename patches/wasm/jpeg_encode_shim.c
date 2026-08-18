/*
 * jpeg_encode_shim.c - expose libjpeg-turbo's baseline JPEG encoder from the
 * pdfium wasm module.
 *
 * PDFium itself only decodes JPEG, so without this shim the compressor
 * objects (which are compiled into libpdfium.a) are dead-stripped at link
 * time. The shim drives the whole libjpeg compression API inside the guest -
 * including setjmp-based error handling, which the wasm exception-handling
 * build supports - so the host only copies pixels in and the encoded bytes
 * out.
 *
 * Compiled with MANGLE_JPEG_NAMES to link against Chromium's mangled
 * (chromium_jpeg_*) symbols.
 */

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "jpeglib.h"

struct gopdfium_jpeg_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

static void gopdfium_jpeg_error_exit(j_common_ptr cinfo)
{
  struct gopdfium_jpeg_error_mgr *err =
      (struct gopdfium_jpeg_error_mgr *)cinfo->err;
  longjmp(err->setjmp_buffer, 1);
}

/* Input pixel formats. */
#define GOPDFIUM_JPEG_FORMAT_RGB   0  /* 3 bytes per pixel */
#define GOPDFIUM_JPEG_FORMAT_RGBA  1  /* 4 bytes per pixel, alpha ignored */
#define GOPDFIUM_JPEG_FORMAT_BGRA  2  /* 4 bytes per pixel, alpha ignored */
#define GOPDFIUM_JPEG_FORMAT_GRAY  3  /* 1 byte per pixel */

/*
 * Encode packed pixel data to a baseline JPEG.
 *
 * data/stride describe the input rows; quality is 1..100 (libjpeg
 * semantics, chroma subsampling per libjpeg defaults). On success returns 1
 * and stores a malloc()ed buffer pointer and its size in *out_buf/*out_size;
 * the caller must release it with gopdfium_jpeg_free(). Returns 0 on failure.
 */
__attribute__((used, visibility("default")))
int gopdfium_jpeg_encode(const unsigned char *data, int width, int height,
                         int stride, int format, int quality,
                         unsigned char **out_buf, unsigned long *out_size)
{
  struct jpeg_compress_struct cinfo;
  struct gopdfium_jpeg_error_mgr jerr;
  JSAMPROW row;
  int y;

  *out_buf = NULL;
  *out_size = 0;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = gopdfium_jpeg_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_compress(&cinfo);
    if (*out_buf) {
      free(*out_buf);
      *out_buf = NULL;
      *out_size = 0;
    }
    return 0;
  }

  jpeg_create_compress(&cinfo);
  jpeg_mem_dest(&cinfo, out_buf, out_size);

  cinfo.image_width = (JDIMENSION)width;
  cinfo.image_height = (JDIMENSION)height;
  switch (format) {
  case GOPDFIUM_JPEG_FORMAT_RGBA:
    cinfo.input_components = 4;
    cinfo.in_color_space = JCS_EXT_RGBA;
    break;
  case GOPDFIUM_JPEG_FORMAT_BGRA:
    cinfo.input_components = 4;
    cinfo.in_color_space = JCS_EXT_BGRA;
    break;
  case GOPDFIUM_JPEG_FORMAT_GRAY:
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
    break;
  case GOPDFIUM_JPEG_FORMAT_RGB:
  default:
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    break;
  }

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  for (y = 0; y < height; y++) {
    row = (JSAMPROW)(data + (size_t)y * (size_t)stride);
    jpeg_write_scanlines(&cinfo, &row, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  return 1;
}

__attribute__((used, visibility("default")))
void gopdfium_jpeg_free(unsigned char *buf)
{
  free(buf);
}

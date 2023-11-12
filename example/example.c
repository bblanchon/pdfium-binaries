// A simple program that renders the first page of a PDF file into a binary PPM file

#include <fpdfview.h>
#include <fpdf_formfill.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s input output\n", argv[0]);
    return 1;
  }

  const char *input_filename = argv[1];
  const char *output_filename = argv[2];

  FPDF_InitLibrary();

  FPDF_DOCUMENT doc = FPDF_LoadDocument(input_filename, NULL);
  if (!doc) {
    fprintf(stderr, "Failed to load %s\n", input_filename);
    return 1;
  }

  FPDF_FORMFILLINFO form_callbacks = {0};
  form_callbacks.version = 2;
  FPDF_FORMHANDLE form = FPDFDOC_InitFormFillEnvironment(doc, &form_callbacks);

  FPDF_PAGE page = FPDF_LoadPage(doc, 0);

  int width = (int)FPDF_GetPageWidth(page);
  int height = (int)FPDF_GetPageHeight(page);

  FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
  FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFF);
  FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);
  FPDF_FFLDraw(form, bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);

  const char *pixels = FPDFBitmap_GetBuffer(bitmap);

  FILE *fp = fopen(output_filename, "wb");
  fprintf(fp, "P6\n%d %d\n255\n", width, height);
  for (int i = 0; i < height * width; ++i) {
    putc(pixels[4 * i + 2], fp);
    putc(pixels[4 * i + 1], fp);
    putc(pixels[4 * i + 0], fp);
  }
  fclose(fp);

  FPDFDOC_ExitFormFillEnvironment(form);
  FPDFBitmap_Destroy(bitmap);
  FPDF_ClosePage(page);
  FPDF_CloseDocument(doc);
  FPDF_DestroyLibrary();
  return 0;
}
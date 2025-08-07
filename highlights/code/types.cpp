#include "types.h"

TessBaseAPICreate_t* TessBaseAPICreate = NULL;
TessBaseAPIInit3_t* TessBaseAPIInit3 = NULL;
TessBaseAPIDelete_t* TessBaseAPIDelete = NULL;
TessBaseAPISetPageSegMode_t* TessBaseAPISetPageSegMode = NULL;
TessBaseAPISetVariable_t* TessBaseAPISetVariable = NULL;

bool load_ocr_lib(void)
{
  HMODULE OCRLib = LoadLibraryA("tesseract55.dll");
  if(!OCRLib)
  {
    printf("ERROR: tesseract55.dll not found. OCR Engine init failed");
    return false;
  }

  TessBaseAPICreate = (TessBaseAPICreate_t*)GetProcAddress(OCRLib, "TessBaseAPICreate");
  TessBaseAPIDelete = (TessBaseAPIDelete_t*)GetProcAddress(OCRLib, "TessBaseAPIDelete");
  TessBaseAPISetPageSegMode = (TessBaseAPISetPageSegMode_t*)GetProcAddress(OCRLib, 
									   "TessBaseAPISetPageSegMode"); 
  TessBaseAPIInit3 = (TessBaseAPIInit3_t*)GetProcAddress(OCRLib, "TessBaseAPIInit3");
  return true;
}

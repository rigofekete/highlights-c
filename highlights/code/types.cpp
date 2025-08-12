#include "types.h"

TessBaseAPICreate_t* TessBaseAPICreate = NULL;
TessBaseAPIInit3_t* TessBaseAPIInit3 = NULL;
TessBaseAPISetPageSegMode_t* TessBaseAPISetPageSegMode = NULL;
TessBaseAPISetVariable_t* TessBaseAPISetVariable = NULL;
TessBaseAPISetImage_t* TessBaseAPISetImage = NULL;
TessBaseAPIGetUTF8Text_t* TessBaseAPIGetUTF8Text = NULL;
TessBaseAPIMeanTextConf_t* TessBaseAPIMeanTextConf = NULL;
TessDeleteText_t* TessDeleteText = NULL;

TessBaseAPIDelete_t* TessBaseAPIDelete = NULL;
TessBaseAPIEnd_t* TessBaseAPIEnd = NULL;

bool load_ocr_lib(void)
{
  HMODULE OCRLib = LoadLibraryA("tesseract55.dll");
  if(!OCRLib)
  {
    printf("ERROR: tesseract55.dll not found. OCR Engine init failed");
    return false;
  }

  TessBaseAPICreate = (TessBaseAPICreate_t*)GetProcAddress(OCRLib, "TessBaseAPICreate");
  if(!TessBaseAPICreate)
  {
    printf("Error loading TessBaseAPICreate");
    return false;
  }
  TessBaseAPIInit3 = (TessBaseAPIInit3_t*)GetProcAddress(OCRLib, "TessBaseAPIInit3");
  if(!TessBaseAPIInit3)
  {
    printf("Error loading TessBaseAPIInit3");
    return false;
  }
  TessBaseAPISetPageSegMode = (TessBaseAPISetPageSegMode_t*)GetProcAddress(OCRLib, 
									   "TessBaseAPISetPageSegMode"); 
  
  if(!TessBaseAPISetPageSegMode)
  {
    printf("Error loading TessBaseAPISetPageSegMode");
    return false;
  }

  TessBaseAPISetVariable = (TessBaseAPISetVariable_t*)GetProcAddress(OCRLib, "TessBaseAPISetVariable");
  if(!TessBaseAPISetVariable)
  {
    printf("Error loading TessBaseAPISetVariable");
    return false;
  }
  TessBaseAPISetImage = (TessBaseAPISetImage_t*)GetProcAddress(OCRLib, "TessBaseAPISetImage"); 
  if(!TessBaseAPISetImage)
  {
    printf("Error loading TessBaseAPISetImage");
    return false;
  }
  TessBaseAPIGetUTF8Text = (TessBaseAPIGetUTF8Text_t*)GetProcAddress(OCRLib, "TessBaseAPISetImage"); 
  if(!TessBaseAPIGetUTF8Text)
  {
    printf("Error loading TessBaseAPIGetUTF8Text");
    return false;
  }
  TessBaseAPIMeanTextConf = (TessBaseAPIMeanTextConf_t*)GetProcAddress(OCRLib, "TessBaseAPIMeanTextConf");
  if(!TessBaseAPIMeanTextConf)
  {
    printf("Error loading TessBaseAPIMeanTextConf");
    return false;
  }
  TessDeleteText = (TessDeleteText_t*)GetProcAddress(OCRLib, "TessDeleteText");
  if(!TessDeleteText)
  {
    printf("Error loading TessDeleteText");
    return false;
  }
  TessBaseAPIDelete = (TessBaseAPIDelete_t*)GetProcAddress(OCRLib, "TessBaseAPIDelete");
  if(!TessBaseAPIDelete)
  {
    printf("Error loading TessBaseAPIDelete");
    return false;
  }
  TessBaseAPIEnd = (TessBaseAPIEnd_t*)GetProcAddress(OCRLib, "TessBaseAPIEnd");
  if(!TessBaseAPIEnd)
  {
    printf("Error loading TessBaseAPIEnd");
    return false;
  }

  printf("\nTesseract functions dynamically loaded successfully\n");

  return true;
}

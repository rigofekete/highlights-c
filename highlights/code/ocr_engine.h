#ifndef OCR_ENGINE_H
#define OCR_ENGINE_H

#include "types.h"
// #include <tesseract/capi.h>

// OCR result struct
typedef struct  {
  char* text;
  int confidence;
  bool valid;
  double processing_time_ms;
} OCRResult;


// OCR engine struct
typedef struct {
  TessBaseAPI* api;
  bool init;
  char language[16];
} OCREngine;


bool ocr_engine_init(OCREngine* engine, const char* language);

#endif // OCR_ENGINE_H

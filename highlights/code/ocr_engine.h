#ifndef OCR_ENGINE_H
#define OCR_ENGINE_H

#include "types.h"
#include <tesseract/capi.h>

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
void ocr_engine_config_game_text(OCREngine* engine);
OCRResult* ocr_detect_txt_region(OCREngine* engine, CroppedRegion* region);
void ocr_engine_set_whitelist(OCREngine* engine, const char* whitelist);
char* ocr_clean_text(const char* raw_text);
bool ocr_is_valid_player_name(const char* text);
void ocr_free_result(OCRResult* result);

bool ocr_engine_cleanup(OCREngine* engine);

#endif // OCR_ENGINE_H

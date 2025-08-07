#include "ocr_engine.h"

bool ocr_engine_init(OCREngine* engine, const char* language)
{
  if(!engine) return false;

  // Init struct 
  engine->api = NULL;
  engine->init = false;
  strncpy(engine->language, language ? language : "eng", sizeof(engine->language) -1);
  engine->language[sizeof(engine->language) - 1] = '\0';

  // Create Tesseract API
  engine->api = TessBaseAPICreate();
  if(!engine->api)
  {
    printf("ERROR: Could not create Tesseract API\n");
    return false;
  }

  // Initialize Tesseract
  if(TessBaseAPIInit3(engine->api, NULL, engine->language) != 0)
  {
    printf("ERROR: Could not initialize Tesseract with language: %s\n", engine->language);
    TessBaseAPIDelete(engine->api);
    engine->api = NULL;
    return false;
  }

  engine->init = true;
  printf("OCR Engine initialized with language: %s\n", engine->language);
  return true;
}



void ocr_engine_configure_for_game_text(OCREngine* engine)
{
  if(!engine || !engine->init)
  {
    printf("ERROR: OCR Enginde not initialized\n");
    return;
  }

  // Configure for single block text detection (player names)
  TessBaseAPISetPageSegMode(engine->api, PSM_SINGLE_BLOCK);


  // Set whitelist for uppercase letters only
  TessBaseAPISetVariable(engine->api, "tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  // Configure for better accuracy
  TessBaseAPISetVariable(engine->api, "tessedit_ocr_engine_mode", "1"); // LSTM engine
  TessBaseAPISetVariable(engine->api, "classify_bln_numeric_mode", "0"); 

  printf("OCR Engine configured for game text detection\n");
}

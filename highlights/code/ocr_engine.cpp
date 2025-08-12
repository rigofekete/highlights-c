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

  // Set tessdata path for trained language or other data if needed 
  const char* tessdata_path = "./tessdata";

  // Initialize Tesseract
  if(TessBaseAPIInit3(engine->api, tessdata_path, engine->language) != 0)
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



void ocr_engine_config_game_text(OCREngine* engine)
{
  if(!engine || !engine->init)
  {
    printf("ERROR: OCR Engine not initialized\n");
    return;
  }

  // Configure for single block text detection (player names)
  TessBaseAPISetPageSegMode(engine->api, tesseract::PSM_SINGLE_WORD);


  // Set whitelist for uppercase letters only
  TessBaseAPISetVariable(engine->api, "tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  // Configure for better accuracy
  TessBaseAPISetVariable(engine->api, "tessedit_ocr_engine_mode", "1"); // LSTM engine
  TessBaseAPISetVariable(engine->api, "classify_bln_numeric_mode", "0"); 

  printf("OCR Engine configured for game text detection\n");
}


void ocr_engine_set_whitelist(OCREngine* engine, const char* whitelist)
{
  if(!engine || !engine->init || !whitelist)
  {
    return;
  }

  TessBaseAPISetVariable(engine->api, "tessedit_char_whitelist", whitelist);
  printf("OCR whitelist set to: %s\n", whitelist);
}


OCRResult* ocr_detect_txt_region(OCREngine* engine, CroppedRegion* region)
{
    OCRResult* result = (OCRResult*)malloc(sizeof(OCRResult));
    if (!result) {
        printf("ERROR: Could not allocate OCR result\n");
        return NULL;
    }

    result->text = NULL;
    result->confidence = 0;
    result->valid = false;
    result->processing_time_ms = 0.0;

    if (!engine || !engine->init || !region) {
        printf("ERROR: Invalid engine or region\n");
        return result;
    }

    clock_t start_time = clock();

    // Allocate RGB buffer with 4-byte aligned stride
    int bytes_per_pixel = 3;
    int tight_stride = region->width * bytes_per_pixel;
    int aligned_stride = ((tight_stride + 3) / 4) * 4;

    uint8_t* rgb_data = (uint8_t*)malloc(region->height * aligned_stride);
    if (!rgb_data) {
        printf("ERROR: Could not allocate RGB conversion buffer\n");
        return result;
    }

    // Convert BGR → RGB with padding for alignment
    for (int y = 0; y < region->height; y++) {
        uint8_t* dst_row = rgb_data + (y * aligned_stride);
        uint8_t* src_row = region->data + (y * region->linesize);
        for (int x = 0; x < region->width; x++) {
            dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // R
            dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // B
        }
    }

    // Give image to Tesseract — keep buffer alive until after Clear()
    TessBaseAPISetImage(engine->api, rgb_data, region->width, region->height,
                        bytes_per_pixel, aligned_stride);

    // OCR
    char* text = TessBaseAPIGetUTF8Text(engine->api);
    if (text) 
    {
	char* cleaned_text = ocr_clean_text(text);
	if(cleaned_text && strlen(cleaned_text) > 0)
	{
	  result->text = cleaned_text; //  Pass pointer ownership and let the caller free it 
	  cleaned_text = NULL;
	  result->valid = true;
	  result->confidence = TessBaseAPIMeanTextConf(engine->api);
	  // NOTE: Left this here for debugging 
	  // printf("\nOCR detected: '%s' (confidence: %d%%)\n", result->text, result->confidence);
	}
	else
	{
	  free(cleaned_text);
	  cleaned_text = NULL;
	}
    }
    

    clock_t end_time = clock();
    // TODO: Check this line and why are we computing it like that 
    result->processing_time_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;

    // TODO: check if we really need to clear the OCR API state
    // Clear Tesseract state BEFORE freeing rgb_data
    // TessBaseAPIClear(engine->api);

    // Now safe to free buffer
    free(rgb_data);

    return result;
}


char* ocr_clean_text(const char* raw_text)
{
  if(!raw_text)
  {
    return NULL;
  }

  size_t len = strlen(raw_text);
  char* cleaned = (char*)malloc(len + 1);
  if(!cleaned)
  {
    return NULL;
  }

  int j = 0;
  for(int i = 0; raw_text[i] != '\0' && j < (int)len; i++)
  {
    if(isalpha(raw_text[i]))
    {
      cleaned[j++] = toupper(raw_text[i]);
    }
  }
  cleaned[j] = '\0';

  // Return NULL if empty string
  if(j == 0)
  {
    free(cleaned);
    return NULL;
  }

  return cleaned;
}



bool ocr_is_valid_player_name(const char* text)
{
  if(!text)
  {
    return false;
  }

  size_t len = strlen(text);
  if(len < 3 || len > 20)
  {
    return false;
  }

  for(size_t i = 0; i < len; i++)
  {
    if(!isalpha(text[i]))
    {
      return false;
    }
  }
  return true;
}



void ocr_free_result(OCRResult* result)
{
  if(result)
  {
    if(result->text)
    {
      free(result->text);
    }
    free(result);
  }
}


bool ocr_engine_cleanup(OCREngine* engine)
{
  if(engine && engine->api)
  {
    TessBaseAPIEnd(engine->api);
    TessBaseAPIDelete(engine->api);
    engine->api = NULL;
    engine->init = false;
    printf("\nOCR Engine cleaned up\n");
    return true;
  }
  return false;
}









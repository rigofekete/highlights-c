#include "text_detection.h"


TextDetection text_detection = {};

global_variable int confidence_threshold = 30;



bool text_detection_init(void)
{
  memset(&text_detection, 0, sizeof(TextDetection));

  if(!text_detection_set_db())
  {
    printf("ERROR: Failed to setup DB\n");
    return false;
  }

  // TODO: Setup OCR engine
  if(!text_detection_set_ocr("eng"))
  {
    printf("ERROR: Failed to setup OCR engine\n");
    return false;
  }

  text_detection.init = true;
  printf("Text Detection initialzed successfully\n");
  return true;
}


internal bool text_detection_set_db(void)
{
  int rc;
  char* err_msg = NULL;

  CreateDirectory(DB_PATH, NULL);

  // Connect to SQLite DB
  rc = sqlite3_open(DB_FILE, &text_detection.conn_player);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot open DB: %s\n", sqlite3_errmsg(text_detection.conn_player));
    sqlite3_close(text_detection.conn_player);
    return false;
  }

  printf("Connected to player DB: %s\n", DB_FILE);

  // Create score_detection table
  const char* create_score_table = 
    "CREATE TABLE IF NOT EXISTS score_detections ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "timestamp REAL, "
    "detected_text TEXT, "
    "confidence INTEGER, "
    "frame_saved INTEGER DEFAULT 0, "
    "processing_time_ms REAL, "
    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");";

  rc = sqlite3_exec(text_detection.conn_player, create_score_table, NULL, NULL, &err_msg);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot create score_detections table: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(text_detection.conn_player);
    return false;
  }

  // Create target_patterns table
  const char* create_patterns_table = 
    "CREATE TABLE IF NOT EXISTS target_patterns ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "pattern TEXT UNIQUE, "
    "active INTEGER DEFAULT 1, "
    "matched_count INTEGER DEFAULT 0, "
    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");";

  rc = sqlite3_exec(text_detection.conn_player, create_patterns_table, NULL, NULL, &err_msg);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot create target_patterns table: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(text_detection.conn_player);
    return false;
  }

  // Insert default target patterns
  const char* default_patterns[] = 
  {
    "CONCEICAO",
    "PAULETA",
    "JOAO PINTO",
    "MF",
    NULL
  };

  for(int i = 0; default_patterns[i] != NULL; i++)
  {
    text_add_pattern(default_patterns[i]);
  }

  printf("DB setup completed successfully\n");
  return true;
}



internal bool text_detection_set_ocr(const char* language)
{
  if(!ocr_engine_init(&text_detection.ocr_engine, language))
  {
    printf("ERROR: Failed to initialize OCR engine\n");
    return false;
  }

  // Text detection OCR setup
  ocr_engine_config_game_text(&text_detection.ocr_engine);

  printf("OCR engine setup completed successfully\n");
  return true;
}

DetectionResult* text_detection_process_region(CroppedRegion* region, double timestamp)
{
  DetectionResult* detection_result = (DetectionResult*)malloc(sizeof(DetectionResult));
  if(!detection_result)
  {
    printf("ERROR: Could not allocate detection result\n");
    return NULL;
  }

  // Init result
  detection_result->detected_text = NULL;
  detection_result->confidence = 0;
  detection_result->timestamp = timestamp;
  detection_result->is_target = false;
  detection_result->saved_to_db = false;

  if(!text_detection.init || !region)
  { printf("ERROR: System not initialized or invalid region\n");
    return detection_result;
  }

  // Perform OCR on the region
  OCRResult* ocr_result = ocr_detect_txt_region(&text_detection.ocr_engine, region);
  if(!ocr_result)
  {
    printf("ERROR: OCR detection failed\n");
    return detection_result;
  }

  if(ocr_result->valid && ocr_result->confidence >= confidence_threshold)
  {
    // Copy text
    detection_result->detected_text = (char*)malloc(strlen(ocr_result->text) + 1);
    if(detection_result->detected_text)
    {
      strcpy(detection_result->detected_text, ocr_result->text);
      detection_result->confidence = ocr_result->confidence;

      // Check if it's a target pattern
      detection_result->is_target = text_detection_is_pattern(detection_result->detected_text);

      if(detection_result->is_target)
      {
	printf("TARGET PATTERN DETECTED: %s (confidence: %d%%)\n",
	       detection_result->detected_text, detection_result->confidence);

	// Save to DB
	detection_result->saved_to_db = text_detection_insert_result(detection_result->detected_text,
							   detection_result->confidence,
							   timestamp);
      }
    }
  }

  ocr_free_result(ocr_result);
  ocr_result = NULL;
  return detection_result;
}



internal bool text_detection_process_multi_regions(CroppedRegion** regions, int count, double timestamp)
{
  if(!regions || count <= 0)
  {
    return false;
  }

  bool any_target_found = false;

  for(int i = 0; i < count; i++)
  {
    if(regions[i])
    {
      DetectionResult* result = text_detection_process_region(regions[i], timestamp);
      if(result && result->is_target)
      {
	any_target_found = true;
	printf("Region %d: Found target '%s'\n", i, result->detected_text);
      }
      text_detection_free_result(result);
    }
  }
  return any_target_found;
}


internal bool text_detection_insert_result(const char* detected_text, int confidence, double timestamp)
{
  if(!text_detection.conn_player || !detected_text)
  {
    return false;
  }

  const char* sql = "INSERT INTO score_detections (timestamp, detected_text, confidence) VALUES (?,?,?);";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(text_detection.conn_player, sql, -1, &stmt, NULL);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot prepare detection insert: %s\n",
	   sqlite3_errmsg(text_detection.conn_player));
    return false;
  }

  sqlite3_bind_double(stmt, 1, timestamp);
  sqlite3_bind_text(stmt, 2, detected_text, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, confidence);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if(rc != SQLITE_DONE)
  {
    printf("ERROR: Cannot insert detection: %s\n",
	   sqlite3_errmsg(text_detection.conn_player));
    return false;
  }

  printf("Detection saved to database: %s (confidence: %d%%)\n", detected_text, confidence);
  return true;
}



internal bool text_detection_is_pattern(const char* text)
{
  if(!text_detection.conn_player || !text)
  {
    return false;
  }

  const char* sql = "SELECT COUNT(*) FROM target_patterns WHERE pattern = ? AND active = 1;";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(text_detection.conn_player, sql, -1, &stmt, NULL);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot prepare pattern check: %s\n",
	   sqlite3_errmsg(text_detection.conn_player));
    return false;
  }

  sqlite3_bind_text(stmt, 1, text, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  int count = 0;
  if(rc == SQLITE_ROW)
  {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count > 0;
}



bool text_detection_is_uniform(CroppedRegion* region, int variation_threshold)
{
  if(!region || !region->data) 
  {
    printf("\nNo region or empty data\n");
    return true;
  }

  // Sample pixels across the region for speed
  int sample_count = 0;
  int total_variation = 0;
  // Sample every 45th pixel for speed
  int step = 45;

  // Get reference pixel (first pixel)
  uint8 ref_r = region->data[0];
  uint8 ref_g = region->data[1];
  uint8 ref_b = region->data[2];

  // Check variation across the region
  for(int i = 0; i < region->width * region->height * 3; i += step)
  {
    // Condition to check if the 3rd byte (b channel from rgb) for the current pixel 
    // overflows the total buffer data / region data array size 
    if(i + 2 >= region->width * region->height * 3)
    {
      printf("\nBuffer overflow... stopping uniform detection\n");
      break;
    }

    uint8 r = region->data[i];
    uint8 g = region->data[i + 1];
    uint8 b = region->data[i + 2];

    // Calculate color difference from ref pixel
    int variation = abs(r - ref_r) + abs(g - ref_g) + abs(b - ref_b);
    total_variation += variation;
    sample_count++;

    // Early exit if we find significant variation
    if(sample_count >= 20 && total_variation > variation_threshold)
    {
      // // NOTE: Debug print block 
      // int avg_variation = total_variation / sample_count;
      // printf("(inner) Region variation: %d (threshold: %d) - %s\n",
      // avg_variation, variation_threshold,
      // avg_variation <= variation_threshold ? "UNIFORM" : "HAS CONTENT");
      return true;
    }
  }

  if(sample_count == 0)
  {
    return true;
  }

  // Calculate average variation per pixel
  int avg_variation = total_variation / sample_count;

  // NOTE: Debugging print log
  printf("Region variation: %d (threshold: %d) - %s\n",
	 avg_variation, variation_threshold,
	 avg_variation <= variation_threshold ? "UNIFORM" : "HAS CONTENT");

  // Low variation means uniform background 
  return avg_variation <= variation_threshold; 
}



internal bool text_add_pattern(const char* pattern)
{
  if(!text_detection.conn_player || !pattern)
  {
    return false;
  }

  const char* sql =
    "INSERT OR IGNORE INTO target_patterns (pattern, active) VALUES (?, 1)";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(text_detection.conn_player, sql, -1, &stmt, NULL);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot prepare pattern insert: %s\n", 
	   sqlite3_errmsg(text_detection.conn_player));
    return false;
  }

  sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if(rc == SQLITE_DONE)
  {
    printf("Pattern added: %s\n", pattern);
    return true;
  }

  printf("ERROR: pattern was not added to the DB");
  return false;
}



internal bool text_detection_get_recent_detections(int limit)
{
  if(!text_detection.conn_player)
  {
    return false;
  }

  const char* sql = 
    "SELECT detected_text, confidence, timestamp FROM score_detections "
    "ORDER BY timestamp DESC LIMIT ?;";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(text_detection.conn_player, sql, -1, &stmt, NULL);
  if(rc != SQLITE_OK) {
    printf("ERROR: Cannot prepare recent detections query: %s\n",
	   sqlite3_errmsg(text_detection.conn_player));
    return false;
  }

  sqlite3_bind_int(stmt, 1, limit);

  printf("\n=== Recent Detections ===\n");
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const char* text = (const char*)sqlite3_column_text(stmt, 0);
    int confidence = sqlite3_column_int(stmt, 1);
    double timestamp = sqlite3_column_double(stmt, 2);

    printf("Text: %s | Confidence: %d%% | Time: %.2f\n",
	   text, confidence, timestamp);
  }

  sqlite3_finalize(stmt);
  return true;
}



internal void text_detection_free_result(DetectionResult* result)
{
  if(result)
  {
    if(result->detected_text)
    {
      free(result->detected_text);
    }
    free(result);
  }
}


void text_detection_config_game(void)
{
  if(text_detection.init)
  {
    ocr_engine_config_game_text(&text_detection.ocr_engine);
    ocr_engine_set_whitelist(&text_detection.ocr_engine, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  }
}



void text_detection_set_confidence_threshold(int threshold)
{
  confidence_threshold = threshold;
  printf("Confidence threshold set to: %d%%\n", threshold);
}


internal void text_detection_cleanup(void)
{
  ocr_engine_cleanup(&text_detection.ocr_engine);
  
  if(text_detection.conn_player)
  {
    sqlite3_close(text_detection.conn_player);
    text_detection.conn_player = NULL;
  }

  text_detection.init = false;
  printf("\nText Detection System cleaned up\n");
}




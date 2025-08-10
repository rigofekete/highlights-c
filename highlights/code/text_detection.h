#ifndef TEXT_DETECTION_H
#define TEXT_DETECTION_H

#include "types.h"
#include "ocr_engine.h"
#include <sqlite3.h>

#define DB_PATH "database"
#define DB_FILE "database\\player_detections.db"

// Text detection system structure
typedef struct {
  sqlite3* conn_player;
  OCREngine ocr_engine;
  char db_path[256];
  bool init;
} TextDetection;


// Detection result struct
typedef struct {
  char* detected_text;
  int confidence;
  double timestamp;
  bool is_target;
  bool saved_to_db;
} DetectionResult;


extern TextDetection text_detection;


// System init and cleanup
bool text_detection_init(void);
internal bool text_detection_set_db(void);
internal bool text_detection_set_ocr(const char* language);
internal void text_detection_cleanup(void);

// DB functions
internal bool text_detection_insert_result(const char* detected_text, int confidence, double timestamp);
internal bool text_detection_is_pattern(const char* text);
internal bool text_add_pattern(const char* pattern);
internal bool text_detection_get_recent_detections(int limit);

// Text detection processing
DetectionResult* text_detection_process_region(CroppedRegion* region, double timestamp);
internal bool text_detection_process_multi_regions(CroppedRegion** regions, int count, double timestamp);
internal void text_detection_free_result(DetectionResult* result);

// Config
void text_detection_config_game(void);
void text_detection_set_confidence_threshold(int threshold);


#endif // TEXT_DETECTION_H

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


extern TextDetection txt_detection;


// System initialization and cleanup
bool text_detection_init(void);
internal bool text_detection_set_db(void);
internal bool text_detection_set_ocr(const char* language);
internal void text_detection_cleanup(void);

internal bool text_add_pattern(const char* pattern);


#endif // TEXT_DETECTION_H

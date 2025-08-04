#include "text_detection.h"


TextDetection txt_detection = {};

global_variable int confidence_threshold = 30;



internal bool detection_init(void)
{
  memset(&txt_detection, 0, sizeof(TextDetection));

  if(!detection_db())
  {
    printf("ERROR: Failed to setup DB\n");
    return false;
  }

  // TODO: Setup OCR engine
  //

  txt_detection.init = true;
  printf("Text Detection initialzed successfully\n");
  return true;
}





internal bool detection_db(void)
{
  int rc;
  char* err_msg = NULL;

  CreateDirectory(DB_PATH, NULL);

  // Connect to SQLite DB
  rc = sqlite3_open(DB_FILE, &txt_detection.conn_player);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot open DB: %s\n", sqlite3_errmsg(txt_detection.conn_player));
    sqlite3_close(txt_detection.conn_player);
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

  rc = sqlite3_exec(txt_detection.conn_player, create_score_table, NULL, NULL, &err_msg);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot create score_detections table: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(txt_detection.conn_player);
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

  rc = sqlite3_exec(txt_detection.conn_player, create_patterns_table, NULL, NULL, &err_msg);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot create target_patterns table: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(txt_detection.conn_player);
    return false;
  }

  // Insert default target patterns
  const char* default_patterns[] = 
  {
    "CONCEICAO",
    "PAULETA",
    "JOAO PINTO",
    NULL
  };

  for(int i = 0; default_patterns[i] != NULL; i++)
  {
    add_pattern(default_patterns[i]);
  }

  printf("DB setup completed successfully\n");
  return true;
}

// internal bool set_ocr(const char* language);

internal bool add_pattern(const char* pattern)
{
  if(!txt_detection.conn_player || !pattern)
  {
    return false;
  }

  const char* sql =
    "INSERT OR IGNORE INTO target_patterns (pattern, active) VALUES (?, 1)";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(txt_detection.conn_player, sql, -1, &stmt, NULL);
  if(rc != SQLITE_OK)
  {
    printf("ERROR: Cannot prepare pattern insert: %s\n", 
	   sqlite3_errmsg(txt_detection.conn_player));
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




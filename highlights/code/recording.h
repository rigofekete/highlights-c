#include "ffmpeg_recording.h"
#include "types.h"

ScreenRecorder recorder = {};

internal bool get_dpi_aware_window_rect(const char* window_name);

internal void scale_coordinates_for_ddagrab(); 

internal bool capture_screen(const char* window_name);

internal bool init_screen_recorder(const char* window_name, int fps);

internal bool setup_output();

internal bool process_frames();

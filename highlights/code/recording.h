#include "ffmpeg_recording.h"
#include "types.h"

#define FRAME_BUFFER_SIZE 30

ScreenRecorder recorder = {};
// // if compiled in C
// ScreenRecorder recorder = 0;

// CroppedRegion regions = {};

internal bool get_dpi_aware_window_rect(const char* window_name);

bool region_targets(const char* window_name);

internal bool capture_screen(const char* window_name);

CroppedRegion* crop_region(uint8* frame_data, int frame_width, int frame_height,
			    int frame_linesize, ROI roi);

void free_cropped_region(CroppedRegion* region);

bool save_cropped_region(CroppedRegion* region, int frame_count, const char* region_name);

internal bool init_screen_recorder(const char* window_name, int fps);

internal bool setup_output();

internal bool process_frames();

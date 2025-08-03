#include "ffmpeg_recording.h"
#include "types.h"

#define FRAME_BUFFER_SIZE 30

ScreenRecorder recorder = {};
// // if compiled in C
// ScreenRecorder recorder = 0;

// CroppedRegion regions = {};

internal bool get_dpi_aware_window_rect(const char* window_name);

internal bool set_region_targets(const char* window_name);

internal bool capture_screen(const char* window_name);

internal CroppedRegion* crop_region(uint8* frame_data, int frame_width, int frame_height,
			    int frame_linesize, ROI roi);

internal void free_cropped_region(CroppedRegion* region);

internal bool save_cropped_region(int frame_count);

internal bool init_screen_recorder(const char* window_name, int fps);

internal bool setup_recording();

internal bool decode_set_frame(int frame_count);
internal bool encode_frame();

internal bool process_frames();

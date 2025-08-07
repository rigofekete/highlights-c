#ifndef FFMPEG_RECORDING_H
#define FFMPEG_RECORDING_H

#include "types.h"

// extern ScreenRecorder recorder;

static char current_recording_timestamp[20];

// Start FFmpeg segment recording with ddagrab
void start_segment_recording(); 

// Save last X seconds when ROI detected
void save_highlight_pre(); 

// Start live recording with ddagrab
void start_live_recording();

// Stop live recording
void stop_live_recording();

void fix_mkv_file(const char* filename); 

void batch_fix_all_mkv_files(); 

bool crop_video_file(const char* input_file, const char* output_file);

void batch_crop_all_files_in_live_folder(); 

#endif // FFMPEG_RECORDING_H

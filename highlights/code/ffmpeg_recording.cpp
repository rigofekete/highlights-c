#include "ffmpeg_recording.h"

// Start FFmpeg segment recording with ddagrab
void start_segment_recording() 
{
    if (segment_process_running) return;
    
    // Create segments directory
    CreateDirectoryA("segments", NULL);
    
    // Build FFmpeg command using ddagrab
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "start /B ffmpeg -f lavfi -i \"ddagrab=framerate=%d\" "
        "-c:v h264_nvenc -preset p1 -cq 18 "
        "-f segment -segment_time %d -segment_wrap %d "
        "-y segments\\seg_%%03d.mp4",
        recorder.fps,
        segment_duration, 
        buffer_seconds / segment_duration + 2
    );
    
    system(cmd);
    segment_process_running = 1;
    printf("Started ddagrab segment recording\n");
}


// Save last X seconds when ROI detected
void save_highlight_pre() 
{
    // Get current timestamp for filename
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    
    // Concatenate all recent segments (15 segments × 2 seconds = 30 seconds)
    char concat_cmd[2048];
    snprintf(concat_cmd, sizeof(concat_cmd),
        "ffmpeg -f concat -safe 0 -i \"concat:"
        "segments\\seg_000.mp4|segments\\seg_001.mp4|segments\\seg_002.mp4|"
        "segments\\seg_003.mp4|segments\\seg_004.mp4|segments\\seg_005.mp4|"
        "segments\\seg_006.mp4|segments\\seg_007.mp4|segments\\seg_008.mp4|"
        "segments\\seg_009.mp4|segments\\seg_010.mp4|segments\\seg_011.mp4|"
        "segments\\seg_012.mp4|segments\\seg_013.mp4|segments\\seg_014.mp4\" "
        "-c copy -y highlight_pre_%s.mp4",
        timestamp
    );
    
    system(concat_cmd);
    printf("Saved pre-highlight: highlight_pre_%s.mp4\n", timestamp);
}

// Start live recording with ddagrab
void start_live_recording() 
{
    if (live_process_running) return;


    CreateDirectoryA("live", NULL);
    
    // Get timestamp for filename
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    
    char live_cmd[1024];
    snprintf(live_cmd, sizeof(live_cmd),
        "start /B ffmpeg -f lavfi -i \"ddagrab=framerate=%d\" "
        "-c:v h264_nvenc -preset p4 -cq 18 "
        "-y live\\highlight_live_%s.mkv",
        recorder.fps, timestamp
    );
    
    system(live_cmd);
    live_process_running = 1;
    printf("Started ddagrab live recording: highlight_live_%s.mkv\n", timestamp);
}



void stop_live_recording() 
{
    if (!live_process_running) return;
    
    printf("Force stopping FFmpeg...\n");
    
    // Force kill FFmpeg   
    system("taskkill /f /im ffmpeg.exe");
    // Give file system time to flush
    Sleep(2000);      
    live_process_running = 0;
    printf("FFmpeg stopped - files may need repair\n");
}





void batch_fix_all_mkv_files() 
{
    printf("=== Fixing all MKV files in live folder ===\n");
    
    // Look for MKV files in the live folder
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("live\\*.mkv", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("No MKV files found in live folder\n");
        return;
    }
    
    int total_files = 0;
    
    do 
    {
        if (strncmp(findData.cFileName, "temp_", 5) != 0) 
	{
            total_files++;
            
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "live\\%s", findData.cFileName);
            
            printf("Processing: %s\n", full_path);
            fix_mkv_file(full_path);
        }
        
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    printf("Processed %d files in live folder\n", total_files);
}




void fix_mkv_file(const char* filename) 
{
    printf("Repairing: %s\n", filename);
    
    char temp_name[512];
    snprintf(temp_name, sizeof(temp_name), "live\\temp_%s", 
             filename + strlen("live\\"));  // Remove "live\\" prefix for temp name
    
    char fix_cmd[1024];
    snprintf(fix_cmd, sizeof(fix_cmd),
        "ffmpeg -i \"%s\" -c copy -y \"%s\" 2>nul",
        filename, temp_name
    );
    
    int result = system(fix_cmd);
    
    if (result == 0) 
    {
        DeleteFileA(filename);  // Delete original
        
        // Rename temp to original name
        char rename_cmd[1024];
        snprintf(rename_cmd, sizeof(rename_cmd), "ren \"%s\" \"%s\"", 
                 temp_name, filename + strlen("live\\"));
        system(rename_cmd);
        
        printf("Fixed: %s\n", filename);
    }
    else
    {
        printf("Failed to fix: %s\n", filename);
        DeleteFileA(temp_name);
    }
}

void batch_crop_all_files_in_live_folder() 
{
    printf("=== Cropping all files in live folder ===\n");
    
    // Create cropped subfolder
    CreateDirectoryA("live\\cropped", NULL);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("live\\*.mkv", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        printf("No MKV files found in live folder\n");
        return;
    }
    
    int total_files = 0;
    int cropped_files = 0;
    
    do
    {
        // Skip temp files and already cropped files
        if (strncmp(findData.cFileName, "temp_", 5) != 0) {
            char input_path[512];
            char output_path[512];
            
            snprintf(input_path, sizeof(input_path), "live\\%s", findData.cFileName);
            snprintf(output_path, sizeof(output_path), "live\\cropped\\%s", findData.cFileName);
            
            printf("Cropping: %s\n", findData.cFileName);
            
            if (crop_video_file(input_path, output_path))
	    {
                cropped_files++;
                printf("Cropped: %s\n", findData.cFileName);
            }
	    else {
                printf("Failed to crop: %s\n", findData.cFileName);
            }
            
            total_files++;
        }
        
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    
    printf("\n=== Crop Complete ===\n");
    printf("Total files: %d\n", total_files);
    printf("Successfully cropped: %d\n", cropped_files);
}



bool crop_video_file(const char* input_file, const char* output_file) 
{
    // Use the ORIGINAL coordinates (before gdigrab scaling)
    // These should match your DPI-aware window coordinates
    int crop_x = recorder.x;        // Your original window x
    int crop_y = recorder.y;        // Your original window y  
    int crop_width = recorder.width;   // Your original window width
    int crop_height = recorder.height; // Your original window height
    
    char crop_cmd[1024];
    snprintf(crop_cmd, sizeof(crop_cmd),
        "ffmpeg -i \"%s\" "
        "-vf \"crop=%d:%d:%d:%d\" "
        "-c:v h264_nvenc -preset p4 -cq 18 "
        "-y \"%s\" 2>nul",
        input_file,
        crop_width, crop_height, crop_x, crop_y,
        output_file
    );
    
    int result = system(crop_cmd);
    return (result == 0);
}





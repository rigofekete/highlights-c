#ifndef TYPES_H
#define TYPES_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
// #include <libavutil/time.h>
}

#include <windows.h>
#include <stdio.h>
#include <time.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int64_t int64;

#define global_variable static
#define internal static

typedef struct
{
  int x, y, width, height;
} ROI;


typedef struct
{
  uint8* data;
  int width, height;
  int linesize;
} CroppedRegion;



typedef struct ScreenRecorder 
{
    // Containers and streams
    AVFormatContext* input_container;
    const AVInputFormat* input_format;

    AVFormatContext* output_container;
    AVStream* output_stream;

    AVCodecContext* codec_context;
    const AVCodec* codec;

    AVCodecContext* decoder_context;

    AVFrame* input_frame;
    AVFrame* output_frame;
    AVPacket* input_packet;
    AVPacket* output_packet;

    int v_stream_index;

    struct SwsContext* sws_ctx;

    int output_index; 
    bool output_ready;

    int fps;
    int x, y, width, height;

    // Array of ROI structs
    ROI* targets;
    int num_targets;
} ScreenRecorder; 


#endif

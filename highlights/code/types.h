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


struct ScreenRecorder 
{
    // Containers and streams
    AVFormatContext* input_container;
    const AVInputFormat* input_format;

    AVFormatContext* output_container;
    AVStream* output_stream;

    AVCodecContext* codec_context;
    const AVCodec* codec;

    int output_index = 0;
    bool output_ready = false;


    int fps;
    int x, y, width, height;
};


struct DdagrabDimensions
{
  int x, y, width, height;
};


#endif

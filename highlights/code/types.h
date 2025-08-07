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

// NOTE: pound defines to represent the enums from the TessPageSegMode enum
// to be used as the 2nd arg in the TessBaseAPISetPageSegMode function 
#define PSM_AUTO 3
#define	PSM_AUTO_ONLY	2
#define	PSM_AUTO_OSD	1
#define	PSM_CIRCLE_WORD	9
#define	PSM_COUNT	13
#define	PSM_OSD_ONLY	0
#define	PSM_SINGLE_BLOCK 6
#define	PSM_SINGLE_BLOCK_VERT_TEXT 5
#define	PSM_SINGLE_CHAR	10
#define	PSM_SINGLE_COLUMN 4
#define	PSM_SINGLE_LINE	7
#define	PSM_SINGLE_WORD	8
#define	PSM_SPARSE_TEXT	11
#define	PSM_SPARSE_TEXT_OSD 12

struct TessBaseAPI;

#define OCR_ENGINE_CREATE(name) TessBaseAPI* name(void);
typedef OCR_ENGINE_CREATE(TessBaseAPICreate_t);

#define OCR_ENGINE_INIT(name) int name(TessBaseAPI* handle, const char* datapath, const char* language);

typedef OCR_ENGINE_INIT(TessBaseAPIInit3_t);

#define OCR_ENGINE_DEL(name)  void name(TessBaseAPI *handle);
typedef OCR_ENGINE_DEL(TessBaseAPIDelete_t);

// NOTE: the 2nd expected arg is an enumerator from the TessPageSegMode enum, so we just define the enums
// and pass the one we need directly as an int. Check tess source code to see original function signature
#define OCR_ENGINE_PSM(name)  void name(TessBaseAPI *handle, int mode);
typedef OCR_ENGINE_PSM(TessBaseAPISetPageSegMode_t);

#define OCR_ENGINE_SET_VAR(name)  bool name(TessBaseAPI *handle, const char* name, const char* value);
typedef OCR_ENGINE_SET_VAR(TessBaseAPISetVariable_t);

extern TessBaseAPICreate_t* TessBaseAPICreate;
extern TessBaseAPIInit3_t* TessBaseAPIInit3;
extern TessBaseAPIDelete_t* TessBaseAPIDelete;
extern TessBaseAPISetPageSegMode_t* TessBaseAPISetPageSegMode;
extern TessBaseAPISetVariable_t* TessBaseAPISetVariable;

bool load_ocr_lib(void);

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


// Main data struct declared globally in recording.cpp
extern ScreenRecorder recorder;

#endif // TYPES_H 

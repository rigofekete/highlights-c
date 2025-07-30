// The extern "C" block tells the C++ compiler "don't mangle these function names"
// When compiling in c++ we will get a dll linking error if we do not do this
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
// #include <stdlib.h>
// #include <stdarg.h>
// #include <string.h>
// #include <inttypes.h>


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int64_t int64;

#define global_variable static
#define internal static


// Add this function to check FFmpeg version
void check_ffmpeg_version() 
{
    unsigned int version = avutil_version();
    int major = (version >> 16) & 0xFF;
    int minor = (version >> 8) & 0xFF;
    int micro = version & 0xFF;
    
    printf("FFmpeg libavutil version: %d.%d.%d\n", major, minor, micro);
    printf("FFmpeg version string: %s\n", av_version_info());
    
    // FFmpeg 6.0+ is needed for ddagrab
    if (major >= 60) {  // libavutil 58+ corresponds to FFmpeg 6.0+
        printf("ddagrab should be available\n");
    } else {
        printf("ddagrab requires FFmpeg 6.0+, upgrade needed\n");
    }
}

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


// NOTE: when defining a struct as static, all of its members are automatically set to 0/NULL/false
// But, we still explicitly initialize all members to 0 for clarity.
// NOTE: Since we are compiling in C++ we need to use the initialization list syntax {} instead of = 0 
global_variable ScreenRecorder recorder = {};


internal bool get_dpi_aware_window_rect(const char* window_name)
{
	// logging("Initializing window: %s", window_name);
	printf("Initializing window: %s\n", window_name);

	HWND hwnd = FindWindow(NULL, window_name);
	// if(!FindWindow(NULL, window_name))
	if(!hwnd)
	{
		printf("Window not found\n");
		return false;
	}

	// TODO: check if this is really needed in this version
	// ShowWindow(hwnd, SW_MINIMIZE);
	ShowWindow(hwnd, SW_RESTORE);
	SetForegroundWindow(hwnd);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	uint8 dpi = GetDpiForWindow(hwnd);

	float dpi_scale = dpi / 96.0;
	
	recorder.x = rect.left * dpi_scale + 10;
	recorder.y = rect.top * dpi_scale + 20;
	recorder.width = (rect.right - rect.left) * dpi_scale - 20;
	recorder.height = (rect.bottom - rect.top) * dpi_scale - 20;

	recorder.width = recorder.width - (recorder.width % 2);
	recorder.height = recorder.height - (recorder.height % 2);

	return true;
}




internal bool capture_screen(const char* window_name)
{
	if(!get_dpi_aware_window_rect(window_name))
	{
		printf("\nError while adjusting window title dpi");
		return false;
	}

	// Find gdigrab input format
	recorder.input_format = av_find_input_format("gdigrab");
	if(!recorder.input_format) {
	  printf("ERROR could not find gdigrab");
	  return false;
	}

	AVDictionary* options = NULL;

	char fps_str[8], offset_x_str[8], offset_y_str[8], video_size_str[16];
	snprintf(fps_str, sizeof(fps_str), "%d", recorder.fps);
	snprintf(offset_x_str, sizeof(offset_x_str), "%d", recorder.x);
	snprintf(offset_y_str, sizeof(offset_y_str), "%d", recorder.y);
	snprintf(video_size_str, sizeof(video_size_str), "%dx%d", recorder.width, recorder.height);


	av_dict_set(&options, "framerate", fps_str, 0);
	av_dict_set(&options, "offset_x", offset_x_str, 0);
	av_dict_set(&options, "offset_y", offset_y_str, 0);
	av_dict_set(&options, "video_size", video_size_str, 0);
	av_dict_set(&options, "show_region", "0", 0);


	recorder.input_container = avformat_alloc_context();
	if(!recorder.input_container) {
	  printf("ERROR could not allocate format context");
	  av_dict_free(&options);
	  return false;
	}


	recorder.input_container = avformat_alloc_context();
	if(!recorder.input_container) {
	  printf("\nERROR could not allocate format context");
	  return false;
	}

	if(avformat_open_input(&recorder.input_container, "desktop",
	   recorder.input_format, &options) != 0)
	{
	  printf("ERROR could not open desktop capture");
	  avformat_free_context(recorder.input_container);
	  av_dict_free(&options);
	  return false;
	}


	// NOTE: Once the input container is open we dont need the dict any longer.
	av_dict_free(&options);
	
	printf("\nCapture setup successfully started.\n");
	return true;
}



internal bool init_screen_recorder(const char* window_name, int fps)
{
  memset(&recorder, 0, sizeof(ScreenRecorder));
  recorder.fps = fps;

  avdevice_register_all();

  return capture_screen(window_name);
}


internal bool setup_output()
{
  if(recorder.output_ready)
  {
    return true;
  }

  time_t rawtime;
  struct tm timeinfo;
  char time_str[16];

  time(&rawtime);
  localtime_s(&timeinfo, &rawtime);
  strftime(time_str, sizeof(time_str), "%H%M%S", &timeinfo);


  // TODO: Check if this gives a compiler error if dir already exists.
  CreateDirectoryA("output", NULL);


  char output_path[256];
  snprintf(output_path, sizeof(output_path), "output\\%d-live-%s.mp4",
	   recorder.output_index, time_str);

  if(avformat_alloc_output_context2(&recorder.output_container, NULL, NULL, output_path) < 0)
  {
    printf("ERROR: Could not create output context");
    return false;
  }
  
  recorder.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if(!recorder.codec)
  {
    printf("ERROR: Could not find H.256 encoder");
    avformat_free_context(recorder.output_container);
    recorder.output_container = NULL;
    return false;
  }


  recorder.output_stream = avformat_new_stream(recorder.output_container, recorder.codec);
  if(!recorder.output_stream)
  {
    printf("ERROR: Could not create output stream");
    avformat_free_context(recorder.output_container);
    return false;
  }

  recorder.codec_context = avcodec_alloc_context3(recorder.codec);
  if(!recorder.codec_context)
  {
    printf("ERROR: Could not allocate codec context");
    avformat_free_context(recorder.output_container);
    recorder.output_container = NULL;
    return false;
  }


  // Set codec parameters  
  recorder.codec_context->width = recorder.width;
  recorder.codec_context->height = recorder.height;
  AVRational timebase = {};
  timebase.num = 1;
  timebase.den = recorder.fps;
  recorder.codec_context->time_base = timebase;
  recorder.output_stream->time_base = timebase;
  AVRational framerate = {};
  framerate.num = recorder.fps;
  framerate.den = 1;
  recorder.codec_context->framerate = framerate;
  recorder.codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
  
  // Set codec options 
  // // NOTE: If we need to pass additional parameters we can fill the av dict like this 
  // AVDictionary* codec_options = NULL;
  // av_dict_set(&codec_options, "preset", "medium", 0);
  
  // Open codec
  if(avcodec_open2(recorder.codec_context, recorder.codec, NULL) < 0)
  {
    printf("Error: Could not open codec");
    // av_dict_free(&codec_options);
    avcodec_free_context(&recorder.codec_context);
    // NOTE: this is legacy inconsistency of the ffmpeg API. This function does not receive the object pointer by reference (making it a **) so it wont be able to NULL the pointer after deallocating like the other av free function do (like the avcode_free_context, called previously)
    // Because of this, we need to manually NULL the pointer out for safety 
    avformat_free_context(recorder.output_container);
    recorder.output_container = NULL;
    return false;
  }


  // // If used
  // av_dict_free(&codec_options);
  

  // Copy codec parameters to stream 
  if(avcodec_parameters_from_context(recorder.output_stream->codecpar, recorder.codec_context) < 0)
  {
    printf("\nERROR: Could not copy codec parameters");
    avcodec_free_context(&recorder.codec_context);
    avformat_free_context(recorder.output_container);
    recorder.output_container = NULL;
    return false;
  }


  // Open output file
  if(!(recorder.output_container->oformat->flags & AVFMT_NOFILE))
  {
    if(avio_open(&recorder.output_container->pb, output_path, AVIO_FLAG_WRITE) < 0)
    {
      printf("\nERROR: Could not open output file: %s", output_path);

      // Get the specific error
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(AVERROR(errno), errbuf, AV_ERROR_MAX_STRING_SIZE);
      printf("\nAV error details: %s", errbuf);

      avcodec_free_context(&recorder.codec_context);
      avformat_free_context(recorder.output_container);
      recorder.output_container = NULL;
      return false;
    }
  }


  // Write header
  if(avformat_write_header(recorder.output_container, NULL) < 0)
  {
    printf("\nERROR: Could not write header");
    avio_closep(&recorder.output_container->pb);
    avcodec_free_context(&recorder.codec_context);
    avformat_free_context(recorder.output_container);
    recorder.output_container = NULL;
    return false;
  }

  recorder.output_ready = true;
  printf("\nOutput setup complete");
  return true;
}






internal bool process_frames()
{
  if(!setup_output())
  {
    printf("\nError preparing ouput container and stream");
    return false;
  }


  printf("\nFinding stream info from format");

  if(avformat_find_stream_info(recorder.input_container, NULL) < 0)
  {
    printf("\nERROR could not get the stream info");
    return false;
  }


  int v_stream_index = -1;
  AVCodecContext* decoder_context = NULL;
  AVCodecParameters* local_codec_param = NULL;
  const AVCodec* decoder = NULL;

  for(size_t i = 0; i < recorder.input_container->nb_streams; ++i)
  {
    local_codec_param = recorder.input_container->streams[i]->codecpar;

    if(local_codec_param->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      v_stream_index = i;
      decoder = avcodec_find_decoder(local_codec_param->codec_id);

      printf("\nVideo Codec: resolution %d x %d",
	     local_codec_param->width,
	     local_codec_param->height);

      break;
    }
  }

  if(v_stream_index == -1)
  {
    printf("\nERROR: no streams available");
    return false;
  }

  decoder_context = avcodec_alloc_context3(decoder);
  if(!decoder_context)
  {
    printf("\nERROR: failed to allocate codec context");
    return false;
  }

  avcodec_parameters_to_context(
		      decoder_context, 
		      // NOTE: Isn't this the same as the line below? It is the same current index from this loop
		      local_codec_param
		      // recorder.input_container->streams[v_stream_index]->codecpar
		      );

  if(avcodec_open2(decoder_context, decoder, NULL) < 0)
  {
    printf("\nERROR: Could not open decoder");
    avcodec_free_context(&decoder_context);
    return false;
  }
  
  // Allocate frames and packets
  AVFrame* input_frame = av_frame_alloc();
  AVFrame* output_frame = av_frame_alloc();
  AVPacket* input_packet = av_packet_alloc();
  AVPacket* output_packet = av_packet_alloc();

  if(!input_frame || !output_frame || !input_packet || !output_packet) 
  {
    printf("\nERROR: Could not allocate frames/packets");
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);
    av_packet_free(&input_packet);
    av_packet_free(&output_packet);
    return false;
  }


  printf("\nStarting frame processing loop...");


  int frame_count = 0;
  // int64 pts = 0;

  
  // Scalling context for BGRA -> YUV420P conversion 
  struct SwsContext* sws_ctx = NULL;

  sws_ctx = sws_getContext(recorder.width, recorder.height, AV_PIX_FMT_BGRA,
  	   recorder.width, recorder.height, AV_PIX_FMT_YUV420P,
  	   SWS_BILINEAR, NULL, NULL, NULL);
  

  if(!sws_ctx)
  {
    printf("\nERROR: Could not create scalling context\n");
    return false;
  }

  DWORD start_time = GetTickCount();
  int64 pts = 0;

  // Main processing loop
  while(true)
  {
    if(GetAsyncKeyState('Q') & 0x8001)
    {
      printf("\n'Q' key press, stopping capture...");
      break;
    }

    if(av_read_frame(recorder.input_container, input_packet) < 0)
    {

      printf("\nEnd of input stream or error");
      break;
    }

    DWORD elapsed_ms = GetTickCount() - start_time;
    output_frame->pts = av_rescale_q(elapsed_ms, {1, 1000}, recorder.codec_context->time_base);
    
    if(input_packet->stream_index == v_stream_index)
    {
      if(avcodec_send_packet(decoder_context, input_packet) >= 0)
      {
	while(avcodec_receive_frame(decoder_context, input_frame) >= 0)
	{

	  frame_count++;

	  output_frame->format = AV_PIX_FMT_YUV420P; 
	  output_frame->width =  recorder.codec_context->width;
	  output_frame->height =  recorder.codec_context->height;
	  // output_frame->pts = pts++;

	  // Allocate buffer for output frame
	  if(av_frame_get_buffer(output_frame, 0) < 0)
	  {
	    printf("\nERROR: Could not allocate output frame buffer");
	    break;
	  }
	  
	  // Convert BGRA to YUV420P using sws_scale
	  sws_scale(sws_ctx,
		    (const uint8* const*)input_frame->data,
		    input_frame->linesize,
		    0,
		    recorder.height,
		    output_frame->data,
		    output_frame->linesize);

	  // Encode frame
	  if(avcodec_send_frame(recorder.codec_context, output_frame) >= 0)
	  {
	    while(avcodec_receive_packet(recorder.codec_context, output_packet) >=0)
	    {
	      // Set packet stream index and rescale timestamps
	      output_packet->stream_index = recorder.output_stream->index;
	      av_packet_rescale_ts(output_packet,
				   recorder.codec_context->time_base,
				   recorder.output_stream->time_base);

	      // Write packet to output file
	      if(av_write_frame(recorder.output_container, output_packet) < 0)
	      {
		printf("\nERROR: Coudld not write frame");

	      }

	      av_packet_unref(output_packet);
	    }
	  }
	  
	  av_frame_unref(output_frame);

	  if(frame_count % 30 == 0)
	  {
	    printf("\nProcessed %d frames", frame_count);
	  }
	}
      }
    }

    // Packet cleanup call for resue on each iteration for new frame
    av_packet_unref(input_packet);

  }

  // Flush encoder
  avcodec_send_frame(recorder.codec_context, NULL);
  while(avcodec_receive_packet(recorder.codec_context, output_packet) >= 0)
  {
    output_packet->stream_index = recorder.output_stream->index;
    av_packet_rescale_ts(output_packet,
			 recorder.codec_context->time_base,
			 recorder.output_stream->time_base);
    av_write_frame(recorder.output_container, output_packet);
    av_packet_unref(output_packet);
  }

  // Write trailer
  av_write_trailer(recorder.output_container);

  // Cleanup
  av_frame_free(&input_frame);
  av_frame_free(&output_frame);
  av_packet_free(&input_packet);
  av_packet_free(&output_packet);
  avcodec_free_context(&decoder_context);

  printf("\nFrame processing complete. Total frames: %d", frame_count);
  return true;
}


int main(int argc, const char* argv[])
{
	// check_ffmpeg_version();
	if(!init_screen_recorder("Ultimate World Soccer Winning Eleven 7 - Hack Edition", 60))
	// if(!init_screen_recorder("Pro Evolution Soccer 2", 60))
	// if(!init_screen_recorder("PCSX2 v2.2.0", 30))
	{
		printf("Error capturing screen");
	}

	process_frames();

	


	return 0;
}

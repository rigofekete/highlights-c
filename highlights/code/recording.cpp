#include "recording.h"

ScreenRecorder recorder = {};
AVFrame* crop_frame = NULL;
// DetectionResult* result = NULL;  
CroppedRegion* name_region = NULL;
// BGR24 for ROI processing
struct SwsContext* sws_ctx_bgr = NULL;

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

	printf("\nDPI aware coordinates: x=%d y=%d w=%d h=%d\n", 
	       recorder.x, recorder.y, recorder.width, recorder.height);
	return true;
}



internal bool set_region_targets(const char* window_name)
{
  // TODO: Always set the num of targets we will compute and allocate
  recorder.num_targets = 1;

  // // Goal scorer player name
  // int x_name = recorder.width / 2 - 390;
  // int y_name = recorder.height - 380;
  //
  // int w_name = recorder.width / 6;
  // int h_name = recorder.height / 10 - 120;
  
  // Goal scorer player name
  int x_name = recorder.width / 2 - 330;
  int y_name = recorder.height - 360;

  int w_name = recorder.width / 10 - 100;
  int h_name = recorder.height / 10 - 120;

  w_name = w_name - (w_name % 2);
  h_name = h_name - (h_name % 2);

  recorder.targets = (ROI*)malloc(recorder.num_targets * sizeof(ROI));
  if(!recorder.targets)
  {
    return false;
  }

  // TODO: assign ROIs according to defined num_targets value!
  *recorder.targets = {x_name, y_name, w_name, h_name};
  // *(recorder.targets + 1) = {x_name, y_name, w_name, h_name};

  // // OR simple array syntax
  // recorder.targets[0] = {x_name, y_name, w_name, h_name};
  
  return true;
}


CroppedRegion* crop_region(uint8* frame_data, int frame_width, int frame_height,
			    int frame_linesize, ROI roi)
{
  CroppedRegion* region = (CroppedRegion*)malloc(sizeof(CroppedRegion));
  if(!region)
  {
    printf("ERROR: CroppedRegion allocation failed");
    return NULL;
  }

  if (roi.x < 0 || roi.y < 0 ||
      roi.x + roi.width  > frame_width ||
      roi.y + roi.height > frame_height ||
      roi.width <= 0 || roi.height <= 0) 
  {
      printf("ERROR: Invalid ROI %d,%d %dx%d\n", roi.x, roi.y, roi.width, roi.height);
      free(region);
      return NULL;
  }

  region->width = roi.width;
  region->height = roi.height;
  // 3 bytes per pixel for BGR24
  region->linesize = roi.width * 3; 

  // Allocate memory for cropped region
  region->data = (uint8*)malloc(region->height * region->linesize);
  if(!region->data)
  {
    free(region);
    printf("ERROR: data allocation for cropped region failed");
    return NULL;
  }

  // copy cropped data
  for(int y = 0; y < roi.height; y++)
  {
    uint8* src = frame_data + ((roi.y + y) * frame_linesize) + (roi.x * 3);
    uint8* dst = region->data + (y * region->linesize);
    memcpy(dst, src, roi.width * 3);
  }

  return region;
}



// Replica of the av_frame_free and av_packer_free like functions where we take the address of the pointer 
// so we can also null it out of deallocating the internal data
 internal void free_cropped_region(CroppedRegion** region)
{
  if(region && *region)
  {
    if((*region)->data)
    {
      free((*region)->data);
    }

    free(*region);
    *region = NULL;
  }
  return;
}


internal bool capture_screen(const char* window_name)
{
  if(!get_dpi_aware_window_rect(window_name))
  {
	  printf("\nError while adjusting window title dpi");
	  return false;
  }

  if(!set_region_targets(window_name))
  {
    printf("ROI targets set up and allocations failed");
    return false;
  }
  else
  {
    printf("ROI targets successfully generated!");
  }


  // Find gdigrab input format
  recorder.input_format = av_find_input_format("gdigrab");
  if(!recorder.input_format) 
  {
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
  if(!recorder.input_container) 
  {
    printf("ERROR could not allocate format context");
    av_dict_free(&options);
    return false;
  }

  // recorder.input_container = avformat_alloc_context();
  // if(!recorder.input_container) {
  //   printf("\nERROR could not allocate format context");
  //   return false;
  // }

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


internal bool setup_recording()
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

  // Prepare decoder, input/output frames and packets 

  printf("\nFinding stream info from format");

  if(avformat_find_stream_info(recorder.input_container, NULL) < 0)
  {
    printf("\nERROR could not get the stream info");
    return false;
  }

  recorder.v_stream_index = -1;
  AVCodecParameters* local_codec_param = NULL;
  const AVCodec* decoder = NULL;

  for(size_t i = 0; i < recorder.input_container->nb_streams; ++i)
  {
    local_codec_param = recorder.input_container->streams[i]->codecpar;

    if(local_codec_param->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      recorder.v_stream_index = i;
      decoder = avcodec_find_decoder(local_codec_param->codec_id);

      printf("\nVideo Codec: resolution %d x %d",
	     local_codec_param->width,
	     local_codec_param->height);

      break;
    }
  }

  if(recorder.v_stream_index == -1)
  {
    printf("\nERROR: no streams available");
    return false;
  }

  recorder.decoder_context = avcodec_alloc_context3(decoder);
  if(!recorder.decoder_context)
  {
    printf("\nERROR: failed to allocate codec context");
    return false;
  }

  avcodec_parameters_to_context(
		      recorder.decoder_context, 
		      // NOTE: Isn't this the same as the line below? It is the same current index from this loop
		      local_codec_param
		      // recorder.input_container->streams[v_stream_index]->codecpar
		      );

  if(avcodec_open2(recorder.decoder_context, decoder, NULL) < 0)
  {
    printf("\nERROR: Could not open decoder");
    avcodec_free_context(&recorder.decoder_context);
    return false;
  }
  
  // Allocate frames and packets
  recorder.input_frame = av_frame_alloc();
  recorder.output_frame = av_frame_alloc();
  recorder.input_packet = av_packet_alloc();
  recorder.output_packet = av_packet_alloc();

  if(!recorder.input_frame || !recorder.output_frame || !recorder.input_packet || !recorder.output_packet) 
  {
    printf("\nERROR: Could not allocate frames/packets");
    av_frame_free(&recorder.input_frame);
    av_frame_free(&recorder.output_frame);
    av_packet_free(&recorder.input_packet);
    av_packet_free(&recorder.output_packet);
    return false;
  }


  // printf("\nStarting frame processing loop...");


  // Scalling context for BGRA -> YUV420P conversion 
  recorder.sws_ctx = sws_getContext(recorder.width, recorder.height, AV_PIX_FMT_BGRA,
			   recorder.width, recorder.height, AV_PIX_FMT_YUV420P,
			   SWS_BILINEAR, NULL, NULL, NULL);
  
  if(!recorder.sws_ctx)
  {
    printf("\nERROR: Could not create scalling context\n");
    return false;
  }


  recorder.output_ready = true;
  printf("\nOutput setup complete");
  return true;
}


internal bool decode_set_frame(int frame_count)
{
  recorder.output_frame->format = AV_PIX_FMT_YUV420P; 
  recorder.output_frame->width =  recorder.codec_context->width;
  recorder.output_frame->height =  recorder.codec_context->height;
  // output_frame->pts = pts++;

  // Allocate buffer for output frame
  if(av_frame_get_buffer(recorder.output_frame, 0) < 0)
  {
    printf("\nERROR: Could not allocate output frame buffer");
    return false;
  }
  // Convert BGRA to YUV420P using sws_scale
  sws_scale(recorder.sws_ctx,
	    (const uint8* const*)recorder.input_frame->data,
	    recorder.input_frame->linesize,
	    0,
	    recorder.height,
	    recorder.output_frame->data,
	    recorder.output_frame->linesize);

  return true;
}


internal bool encode_frame()
{

  if(avcodec_send_frame(recorder.codec_context, recorder.output_frame) >= 0)
  {
   while(avcodec_receive_packet(recorder.codec_context, recorder.output_packet) >=0)
   {
     // Set packet stream index and rescale timestamps
     recorder.output_packet->stream_index = recorder.output_stream->index;
     av_packet_rescale_ts(recorder.output_packet,
	   recorder.codec_context->time_base,
	   recorder.output_stream->time_base);

     // Write packet to output file
     if(av_write_frame(recorder.output_container, recorder.output_packet) < 0)
     {
	printf("\nERROR: Could not write frame");
     }

     av_packet_unref(recorder.output_packet);
   }
  }
  
  return true;
}



internal bool save_cropped_region(int frame_count)
// bool save_cropped_region(CroppedRegion* region, int frame_count, const char* region_name)
{
    // BGR24 for ROI processing
    struct SwsContext* sws_ctx_bgr = NULL;

    sws_ctx_bgr = sws_getContext(recorder.width, recorder.height, AV_PIX_FMT_BGRA,
				 recorder.width, recorder.height, AV_PIX_FMT_BGR24,
				 SWS_BILINEAR, NULL, NULL, NULL);
    if(!sws_ctx_bgr)
    {
      printf("\nError: Could not create BGR conversion context\n");
      return false;
    }

    // Allocate BGR frame for ROI processing
    crop_frame = av_frame_alloc();
    // crop_frame->pts = av_rescale_q(elapsed_ms, {1, 1000}, recorder.codec_context->time_base);
    crop_frame->format = AV_PIX_FMT_BGR24;
    crop_frame->width = recorder.width;
    crop_frame->height = recorder.height;


    if(av_frame_get_buffer(crop_frame, 0) < 0)
    {
      printf("\nERROR: could not allocate BGR frame buffer");
      av_frame_free(&crop_frame);
      return false;
    }

    // Convert to BGR24 for region processing
    sws_scale(sws_ctx_bgr,
	      (const uint8* const*)recorder.input_frame->data,
	      recorder.input_frame->linesize,
	      0,
	      recorder.height,
	      crop_frame->data,
	      crop_frame->linesize);

    // Crop regions for processing
    CroppedRegion* name_region = crop_region(crop_frame->data[0],
					     crop_frame->width,
					     crop_frame->height,
					     crop_frame->linesize[0],
					     recorder.targets[0]);


    CreateDirectoryA("ppm", NULL);

    char filename[256];
    sprintf(filename, "ppm\\debug_%s_frame_%d.ppm", "region", frame_count);
    
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("\nERROR: Could not open %s for writing", filename);
        return false;
    }
    
    // Write PPM header (simple image format)
    fprintf(f, "P6\n%d %d\n255\n", name_region->width, name_region->height);
    
    // Write BGR data (convert to RGB while writing)
    for (int y = 0; y < name_region->height; y++) {
        for (int x = 0; x < name_region->width; x++) {
            uint8_t* pixel = name_region->data + (y * name_region->linesize) + (x * 3);
            // Write RGB (swap BGR to RGB)
            fputc(pixel[2], f); // R
            fputc(pixel[1], f); // G  
            fputc(pixel[0], f); // B
	}
    }
    
    fclose(f);
    printf("\nSaved %s to %s (%dx%d)\n", "region", filename, name_region->width, name_region->height);

    // Free BGR ROI frames, regions and pixel color context
    av_frame_free(&crop_frame);
    free_cropped_region(&name_region);
    if(sws_ctx_bgr)
    {
      sws_freeContext(sws_ctx_bgr);
      sws_ctx_bgr = NULL;
    }

    return true;
}



internal bool detect_cropped_region(double timestamp)
// bool save_cropped_region(CroppedRegion* region, int frame_count, const char* region_name)
{
  if(!recorder.is_recording)
  {
    sws_ctx_bgr = sws_getContext(recorder.width, recorder.height, AV_PIX_FMT_BGRA,
				 recorder.width, recorder.height, AV_PIX_FMT_BGR24,
				 SWS_BILINEAR, NULL, NULL, NULL);
    if(!sws_ctx_bgr)
    {
      printf("\nError: Could not create BGR conversion context\n");
      return false;
    }

    // allocate bgr frame for roi processing
    crop_frame = av_frame_alloc();
    // crop_frame->pts = av_rescale_q(elapsed_ms, {1, 1000}, recorder.codec_context->time_base);
    crop_frame->format = AV_PIX_FMT_BGR24;
    crop_frame->width = recorder.width;
    crop_frame->height = recorder.height;


    if(av_frame_get_buffer(crop_frame, 0) < 0)
    {
      printf("\nERROR: could not allocate BGR frame buffer");
      av_frame_free(&crop_frame);
      return false;
    }

    // Convert to BGR24 for region processing
    sws_scale(sws_ctx_bgr,
	      (const uint8* const*)recorder.input_frame->data,
	      recorder.input_frame->linesize,
	      0,
	      recorder.height,
	      crop_frame->data,
	      crop_frame->linesize);

    // Crop regions for processing
    name_region = crop_region(crop_frame->data[0],
					     crop_frame->width,
					     crop_frame->height,
					     crop_frame->linesize[0],
					     recorder.targets[0]);
    
    recorder.start_time_rec = time(NULL);

    DetectionResult* result = text_detection_process_region(name_region, timestamp); 
    // result = text_detection_process_region(name_region, timestamp); 
    if(result && result->is_target)
    {
      recorder.is_recording = true;
      start_live_recording();
      printf("Detected pattern! Starting live recording!");
      // return true;
    } 
    // else
    // {
    //   free(result);
    //   result = NULL;
    // }
  

    // Free BGR ROI frames, regions and pixel color context
    if(crop_frame)
    {
      av_frame_free(&crop_frame);
    }
     
    if(name_region)
    {
      free_cropped_region(&name_region);
    }

    if(sws_ctx_bgr)
    {
      sws_freeContext(sws_ctx_bgr);
      sws_ctx_bgr = NULL;
    }
    
  }
  return true;
}


//
//
// internal bool detect_cropped_region(double timestamp)
// // bool save_cropped_region(CroppedRegion* region, int frame_count, const char* region_name)
// {
//   if(!recorder.is_recording)
//   {
//     sws_ctx_bgr = sws_getContext(recorder.width, recorder.height, AV_PIX_FMT_BGRA,
// 				 recorder.width, recorder.height, AV_PIX_FMT_BGR24,
// 				 SWS_BILINEAR, NULL, NULL, NULL);
//     if(!sws_ctx_bgr)
//     {
//       printf("\nError: Could not create BGR conversion context\n");
//       return false;
//     }
//
//     // allocate bgr frame for roi processing
//     crop_frame = av_frame_alloc();
//     // crop_frame->pts = av_rescale_q(elapsed_ms, {1, 1000}, recorder.codec_context->time_base);
//     crop_frame->format = AV_PIX_FMT_BGR24;
//     crop_frame->width = recorder.width;
//     crop_frame->height = recorder.height;
//
//
//     if(av_frame_get_buffer(crop_frame, 0) < 0)
//     {
//       printf("\nERROR: could not allocate BGR frame buffer");
//       av_frame_free(&crop_frame);
//       return false;
//     }
//
//     // Convert to BGR24 for region processing
//     sws_scale(sws_ctx_bgr,
// 	      (const uint8* const*)recorder.input_frame->data,
// 	      recorder.input_frame->linesize,
// 	      0,
// 	      recorder.height,
// 	      crop_frame->data,
// 	      crop_frame->linesize);
//
//     // Crop regions for processing
//     name_region = crop_region(crop_frame->data[0],
// 					     crop_frame->width,
// 					     crop_frame->height,
// 					     crop_frame->linesize[0],
// 					     recorder.targets[0]);
//
//     recorder.start_time_rec = time(NULL);
//
//     result = text_detection_process_region(name_region, timestamp); 
//     if(result && result->is_target)
//     {
//       recorder.is_recording = true;
//       start_live_recording();
//       printf("Detected pattern! Starting live recording!");
//     } 
//     else
//     {
//       free(result);
//       result = NULL;
//     }
//     return true;
//   }
//   else
//   {
//     // TODO: implement a trigger to stop recording after the replay sequence is done
//     if(time(NULL) - recorder.start_time_rec >= recorder.timeout)
//     {
//       stop_live_recording();
//       recorder.is_recording = false;
//       printf("Stopping live recording....");
//
//     }
//
//
//     if(!recorder.is_recording)
//     {
//       // Free BGR ROI frames, regions and pixel color context
//       if(crop_frame)
//       {
// 	av_frame_free(&crop_frame);
//       }
//
//       if(name_region)
//       {
// 	free_cropped_region(&name_region);
//       }
//
//       if(sws_ctx_bgr)
//       {
// 	sws_freeContext(sws_ctx_bgr);
// 	sws_ctx_bgr = NULL;
//       }
//     }
//   }
// }
//




internal bool process_frames()
{
  if(!setup_recording())
  {
    printf("\nError preparing ouput container and stream");
    return false;
  }


  int frame_count = 0;

  DWORD start_time = GetTickCount();
  int64 pts = 0;

  // start_live_recording();

  printf("\nStarting frame processing loop...");

  // Main processing loop
  while(true)
  {
    // TODO this is here only for testing, we will need to detect ROIs and trigger recording and ring buffer flush
    if(GetAsyncKeyState('S') & 0x8001)
    {
      printf("\n'S' key press, stopping manual frame capture...");
      stop_live_recording();
      break;
    }

    
    if(av_read_frame(recorder.input_container, recorder.input_packet) < 0)
    {

      printf("\nEnd of input stream or error");
      break;
    }

    DWORD elapsed_ms = GetTickCount() - start_time;
    recorder.output_frame->pts = av_rescale_q(elapsed_ms, {1, 1000}, recorder.codec_context->time_base);
    frame_count++;

    if(recorder.input_packet->stream_index == recorder.v_stream_index)
    {
      if(avcodec_send_packet(recorder.decoder_context, recorder.input_packet) >= 0)
      {
	while(avcodec_receive_frame(recorder.decoder_context, recorder.input_frame) >= 0)
	{
	  // TODO: Make this a manual live recording function for testing purposes which will flush encoders and trailers internally 
	  if(!decode_set_frame(frame_count)) 
	  {
	    printf("Error decoding and setting up frames");
	    return false;
	  }
	  
	  // // if(frame_count % 30 == 0)
	  // if(frame_count % recorder.fps == 0)
	  // {
	  //   if(!save_cropped_region(frame_count))
	  //   {
	  //     printf("Error cropping frame");
	  //     return false;
	  //   }
	  // }
	  //
	  // if(frame_count % recorder.fps == 0)
	  // {
	  //   double timestamp = elapsed_ms / 1000.0;
	  //   if(!detect_cropped_region(timestamp))
	  //   {
	  //     printf("\nERROR: Detection of cropped region failed\n");
	  //     return false;
	  //   }
	  // }

	  double timestamp = elapsed_ms;
	  if(!detect_cropped_region(timestamp))
	  {
	    printf("\nERROR: Detection of cropped region failed\n");
	    return false;
	  }

	  if(!encode_frame())
	  {
	    printf("Error encoding frame");
	    return false;
	  }


	  av_frame_unref(recorder.output_frame);

	  if(frame_count % recorder.fps == 0)
	  {
	    printf("\nProcessed %d frames", frame_count);
	  }
	}
      }
    }

    // Packet cleanup call for reuse on each iteration for new frame
    av_packet_unref(recorder.input_packet);
  }

  // TODO: wrap the flush and write trailer into a function as well as the cleanup in a separate one
  // Flush encoder internal buffers
  avcodec_send_frame(recorder.codec_context, NULL);
  while(avcodec_receive_packet(recorder.codec_context, recorder.output_packet) >= 0)
  {
    recorder.output_packet->stream_index = recorder.output_stream->index;
    av_packet_rescale_ts(recorder.output_packet,
  	 recorder.codec_context->time_base,
  	 recorder.output_stream->time_base);
    av_write_frame(recorder.output_container, recorder.output_packet);
    av_packet_unref(recorder.output_packet);
  }

  // Write trailer
  // flush remaining container data and properly closes the output file
  av_write_trailer(recorder.output_container);

  // Cleanup
  // NOTE: All of tgghe av free funcs that take ** pointers already check if pointers are NULL 
  // and also set the pointer to NULL after cleaning since we have the address of the pointer 
  av_frame_free(&recorder.input_frame);
  av_frame_free(&recorder.output_frame);
  av_packet_free(&recorder.input_packet);
  av_packet_free(&recorder.output_packet);
  avcodec_free_context(&recorder.decoder_context);
  // Regular dealloc so we check for NULl and NULL it after cleaning
  if(recorder.sws_ctx)
  {
    sws_freeContext(recorder.sws_ctx);
    recorder.sws_ctx = NULL;
  }
  if(recorder.targets)
  {
    free(recorder.targets);
    recorder.targets = NULL;
  }

  printf("\nFrame processing complete. Total frames: %d", frame_count);
  return true;
}


int main(int argc, const char* argv[])
{

  // if(!load_ocr_lib())
  // {
  //   printf("Error OCR load lib failure");
  //   return 1;
  // }

  if(!text_detection_init())
  {
    printf("Failed to initialize text detection system\n");
    return 1;
  }

  text_detection_config_game();
  text_detection_set_confidence_threshold(40);

  printf("Text Detection System is ready!\n");

  if(!init_screen_recorder("Ultimate World Soccer Winning Eleven 7 - Hack Edition", 30))
  // if(!init_screen_recorder("Pro Evolution Soccer 2", 60))
  // if(!init_screen_recorder("PCSX2 v2.2.0", 30))
  {
	  printf("Error capturing screen");
  }

  process_frames();

  printf("\nFixing all MKV files...\n");
  batch_fix_all_mkv_files();


  printf("\nCropping all MKV files...\n");
  batch_crop_all_files_in_live_folder(); 

  if(!ocr_engine_cleanup(&text_detection.ocr_engine))
  {
    printf("\nOCR cleanup failed\n");
  }

  return 0;
}

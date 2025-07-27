// The extern "C" block tells the C++ compiler "don't mangle these function names"
// When compiling in c++ we will get a dll linking error if we do not do this 
extern "C" {
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libavdevice\avdevice.h>
#include <libavutil\time.h>
}

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>




static const char* get_error_text(int error_code);

static void logging(const char* fmt, ...);

static int decode_packet(
		AVPacket* pPacket, 
		AVCodecContext* pCodecContext,
		AVFrame* pFrame
		);

static void save_gray_frame(
		unsigned char* buf,
		int wrap, 
		int xsize, int ysize,
		char* filename
		);

int main(int argc, const char* argv[])
{
	if(argc < 2)
	{
		printf("You need to specify a media input.\n");
		return -1;
	}

	
	logging("initializing all the containers, codecs and protocols.");

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	AVFormatContext* pFormatContext = avformat_alloc_context();
	if(!pFormatContext)
	{
		logging("ERROR could not allocate memory for Format Context");
		return -1;
	}



	// BLOCK TO OPEN INPUT FILES
	// logging("opening the input file (%s) and loading format (container) header", argv[1]);
	// // Open the file and read its header. The codecs are not opened.
	// // The function arguments are:
	// // AVFormatContext (the component we allocated memory for),
	// // url (filename/input source),
	// // AVInputFormat (if you pass NULL it'll do the auto detect)
	// // and AVDictionary (which are options to the demuxer)
	// // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
	// if(avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0)
	// {
	// 	logging("ERROR could not open the file")
	// 	return -1;
	// }
	//
	// // now we have access to some information about our file
	// // since we read its header we can say what format (container) it's
	// // and some other information related to the format itself.
	// logging("format %s, duration %lld us, bit_rate %lld", 
	// 	pFormatContext->iformat->name, pFormatContext->duration,
	// 	pFormatContext->bit-rate
	// 	);
	//
	// logging("finding stream info from format");
	
	
	// Initilize device support
	avdevice_register_all();

	// Find the gdigrab input format
	const AVInputFormat* inputFormat = av_find_input_format("gdigrab");
	if(!inputFormat)
	{
		logging("ERROR could not find gdigrab input format");
		return -1;
	}

	logging("opening desktop capture");

	// Open desktop capture instead of file 
	// "desktop" capture the entire desktop
	if(avformat_open_input(&pFormatContext, argv[1], inputFormat, NULL) != 0)
	{
		logging("ERROR could not open desktop capture");
		return -1;
	}


	// INPUT STREAM INFO BLOCK	
	logging("format %s, duration %lld us, bit_rate %lld", 
		pFormatContext->iformat->name, pFormatContext->duration,
		pFormatContext->bit_rate
		);

	logging("finding stream info from format");


	if(avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		logging("ERROR could not get the stream info");
		return -1;
	}

	// Find the video stream
	int video_stream_index = -1;
	AVCodecParameters* pCodecParameters = NULL;
	const AVCodec* pCodec = NULL;
	
	// Loop through all streams to find video stream
	for(int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters* pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

		if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = i;
			pCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
			pCodecParameters = pLocalCodecParameters;

			logging("Video Codec: resolution %d x %d",
				pLocalCodecParameters->width,
				pLocalCodecParameters->height
				);
			break;
		}
	}

	if(video_stream_index == -1)
	{
		logging("ERROR failed to allocate codec context");
		return -1;
	}

	// Set up codec context
	AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
	if(!pCodecContext)
	{
		logging("ERROR failed to allocate codec context");
		return -1;
	}

	// Copy codec parameters to context
	if(avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
	{
		logging("ERROR failed to copy codec parameters to context");
		return -1;

	}

	// Open codec
	if(avcodec_open2(pCodecContext, pCodec, NULL) < 0)
	{
		logging("ERROR failed to open codec");
		return -1;
	}

	// Allocate frame and packet
	AVFrame* pFrame = av_frame_alloc();
	AVPacket* pPacket = av_packet_alloc();

	if(!pFrame || !pPacket)
	{
		logging("ERROR could not allocate frame or packet");
		return -1;
	}

	logging("Starting 5-second desktop capture...");

	// Capture for 5 seconds
	// get current time in microseconds
	int64_t start_time = av_gettime(); 
	// 5 seconds in microsecs
	int64_t capture_duration = 5 * 1000000;
	int frame_count = 0;

	while(av_gettime() - start_time < capture_duration)
	{
		if(av_read_frame(pFormatContext, pPacket) >= 0)
		{
		
			if(pPacket->stream_index == video_stream_index)
			{
				int response = decode_packet(pPacket, pCodecContext, pFrame);
				if(response == 0)
				{
					frame_count++;
					logging("Frame %d captured (size: %dx%d)",
						frame_count, pFrame->width, pFrame->height);
				}
			}
			av_packet_unref(pPacket);
		}
	}

	logging("Capture complete! Total frames: %d", frame_count);

	// Clean up 
	av_frame_free(&pFrame);
	av_packet_free(&pPacket);
	avcodec_free_context(&pCodecContext);
	avformat_close_input(&pFormatContext);
	avformat_free_context(pFormatContext);


	return 0;
}

static const char* get_error_text(int error_code)
{
    static char error_buffer[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(error_code, error_buffer, AV_ERROR_MAX_STRING_SIZE);
    return error_buffer;
}


static void logging(const char* fmt, ...)
{
	va_list args;
	fprintf(stderr, "LOG: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}



static int decode_packet(
		AVPacket* pPacket, 
		AVCodecContext* pCodecContext,
		AVFrame* pFrame
		)
{
	// Supply raw packet data as input to a decoder 
	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
	int response = avcodec_send_packet(pCodecContext, pPacket);

	if(response < 0)
	{
		logging("Error while sending a packet to the decoder: %s", get_error_text(response));
		return response;
	}
	
	while(response >= 0)
	{
		// Return decoded ouptut data (into a frame) from a decoder
	    	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
		response = avcodec_receive_frame(pCodecContext, pFrame);
		if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
		{
			break;

		} else if (response < 0)
		{
			logging("Error while receiving a frame from the decoder: %s", get_error_text(response));
			return response;
		}

		if(response >= 0)
		{
			logging("Frame %d captured (size: %dx%d)", 
				pCodecContext->frame_num, pFrame->width, pFrame->height);


			char frame_filename[1024];
			snprintf(frame_filename, sizeof(frame_filename), "%s-%lld.pgm", "frame",
				pCodecContext->frame_num
				);
			// Check if the frame is a planar YUV 4:2:0, 12 bpp
			// That is the format of the provided .mp4 file?? TODO: we want to capture from desktop!
			// RGB formats will definitely not give a gray image
			// Other YUV image may do so, but unstested, so give a warning
			if(pFrame->format != AV_PIX_FMT_YUV420P)
			{

				logging("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");

				// save a grayscale frame into a .pgm file
				save_gray_frame(pFrame->data[0],
						pFrame->linesize[0],
						pFrame->width,
						pFrame->height,
						frame_filename);
			}
		}
		return 0;
	}
}



static void save_gray_frame(unsigned char* buf, int wrap, int xsize, int ysize, char* filename)
{
	FILE* f;
	int i;
	f = fopen(filename, "w");
	// writing the minimal required header for a pgm file format
	// portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

	// writing line by line
	for(i = 0; i < ysize; i++)
	{
		fwrite(buf + i * wrap, 1, xsize, f);
	}
	fclose(f);
}






// The extern "C" block tells the C++ compiler "don't mangle these function names"
// When compiling in c++ we will get a dll linking error if we do not do this
extern "C" {
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libavdevice\avdevice.h>
// #include <libavutil\time.h>
}


#include <windows.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>
// #include <string.h>
// #include <inttypes.h>


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


#define global_variable static
#define internal static

// TODO: place this in a helper or global vars file
static void logging(const char* fmt, ...)
{
	va_list args;
	fprintf(stderr, "LOG: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

struct ScreenRecorder 
{
	AVFormatContext* input_container;
	const AVInputFormat* input_format;
	int fps;
	int x, y, width, height;
};

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

	ShowWindow(hwnd, SW_RESTORE);
	SetForegroundWindow(hwnd);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	uint8 dpi = GetDpiForWindow(hwnd);

	dpi = dpi / 96;



	return true;
}




internal bool CaptureScreen(ScreenRecorder* recorder, const char* window_name)
{
	// TODO: check if it isn't better to make a global dimension struct
	RECT rect;
	
	if(!get_dpi_aware_window_rect(window_name))
	{
		logging("Error while adjusting window title dpi");
		return false;
	}


	return true;
}

int main(int argc, const char* argv[])
{
	ScreenRecorder recorder;

	if(!CaptureScreen(&recorder, "Calculator"))
	{
		logging("Error capturing screen");
	}


	return 0;
}

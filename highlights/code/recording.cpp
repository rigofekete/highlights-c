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
struct ScreenRecorder 
{
	AVFormatContext* input_container;
	const AVInputFormat* input_format;
	int fps;
	int x, y, width, height;
};

global_variable ScreenRecorder recorder;


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




internal bool CaptureScreen(const char* window_name)
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
	if(!CaptureScreen("PCSX2 v2.4.0"))
	{
		printf("Error capturing screen");
	}


	return 0;
}

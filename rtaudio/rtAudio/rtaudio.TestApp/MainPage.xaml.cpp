//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include "Logger.hxx"
#include "MainPage.xaml.h"



using namespace rtaudio_TestApp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16

// Platform-dependent sleep routines.
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ ) || defined( __WINDOWS_RT__ )
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

void usage(void) {
	// Error function in case of incorrect command-line
	// argument specifications
	std::cout << "\nuseage: record N fs <duration> <device> <channelOffset>\n";
	std::cout << "    where N = number of channels,\n";
	std::cout << "    fs = the sample rate,\n";
	std::cout << "    duration = optional time in seconds to record (default = 2.0),\n";
	std::cout << "    device = optional device to use (default = 0),\n";
	std::cout << "    and channelOffset = an optional channel offset on the device (default = 0).\n\n";
	exit(0);
}

struct InputData {
	MY_TYPE* buffer;
	unsigned long bufferBytes;
	unsigned long totalFrames;
	unsigned long frameCounter;
	unsigned int channels;
};

typedef struct {
	unsigned int	nRate;		/* Sampling Rate (sample/sec) */
	unsigned int	nChannel;	/* Channel Number */
	unsigned int	nFrame;		/* Frame Number of Wave Table */
	float		*wftable;	/* Wave Form Table(interleaved) */
	unsigned int	cur;		/* current index of WaveFormTable(in Frame) */
} CallbackData;

static int
rtaudio_callback(
	void			*outbuf,
	void			*inbuf,
	unsigned int		nFrames,
	double			streamtime,
	RtAudioStreamStatus	status,
	void			*userdata)
{
	(void)inbuf;
	float	*buf = (float*)outbuf;
	unsigned int remainFrames;
	CallbackData	*data = (CallbackData*)userdata;

	remainFrames = nFrames;
	while (remainFrames > 0) {
		unsigned int sz = data->nFrame - data->cur;
		if (sz > remainFrames)
			sz = remainFrames;
		memcpy(buf, data->wftable + (data->cur*data->nChannel),
			sz * data->nChannel * sizeof(float));
		data->cur = (data->cur + sz) % data->nFrame;
		buf += sz * data->nChannel;
		remainFrames -= sz;
	}
	return 0;
}

// Interleaved buffers
int input(void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
	double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data)
{
	InputData *iData = (InputData *)data;

	// Simply copy the data to our allocated buffer.
	unsigned int frames = nBufferFrames;
	if (iData->frameCounter + nBufferFrames > iData->totalFrames) {
		frames = iData->totalFrames - iData->frameCounter;
		iData->bufferBytes = frames * iData->channels * sizeof(MY_TYPE);
	}

	unsigned long offset = iData->frameCounter * iData->channels;
	memcpy(iData->buffer + offset, inputBuffer, iData->bufferBytes);
	iData->frameCounter += frames;

	if (iData->frameCounter >= iData->totalFrames) return 2;
	return 0;
}

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
	double streamTime, RtAudioStreamStatus status, void *userData)
{
	if (status)
		std::cout << "Stream overflow detected!" << std::endl;
	// Do something with the data in the "inputBuffer" buffer.
	return 0;
}

// Pass-through function.
int inout(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
	double streamTime, RtAudioStreamStatus status, void *data)
{
	// Since the number of input and output channels is equal, we can do
	// a simple buffer copy operation here.
	if (status) std::cout << "Stream over/underflow detected." << std::endl;
	unsigned long *bytes = (unsigned long *)data;
	memcpy(outputBuffer, inputBuffer, *bytes);
	//DebugOut("\nKUNLQT bytes = %d!\n", bytes);
	//DebugOut("\nKUNLQT inputBuffer = %d!\n", inputBuffer);
	//DebugOut("\nKUNLQT outputBuffer = %d!\n", outputBuffer);
	return 0;
}

MainPage::MainPage()
{
	InitializeComponent();
	
}

#pragma region UI Event Handlers
void MainPage::btnStartCapture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//InitializeCapture(sender, e);
	//RtAudio adac;
	RtAudio *adac=0;

	try {
		adac = new RtAudio(RtAudio::WINDOWS_RT);
	}
	catch (RtError e) {
		fprintf(stderr, "fail to create RtAudio: %s¥n", e.what());
		DebugOut("\nKUNLQT fail to create RtAudio!\n");
		//return 1;
	}

	// Determine the number of devices available
	unsigned int devices = adac->getDeviceCount();
	if (devices < 1) {
		std::cout << "\nNo audio devices found!\n";
		DebugOut("\nKUNLQT No audio devices found!\n");
		exit(0);
	}

	// Set the same number of channels for both input and output.
	unsigned int bufferBytes;
	RtAudio::StreamParameters iParams, oParams;
	iParams.deviceId = 0; // first available device
	iParams.nChannels = 2;
	oParams.deviceId = 0; // first available device
	oParams.nChannels = 2;
	unsigned int sampleRate = 44100;
	unsigned int bufferFrames = 512; // 512 sample frames
	try {
		DebugOut("\nKUNLQT STARTING openStream!\n");
		//adac->openStream(NULL, &iParams, RTAUDIO_SINT16, sampleRate, &bufferFrames, &rtaudio_callback);

		adac->openStream(&oParams, &iParams, RTAUDIO_SINT32, sampleRate, &bufferFrames, &inout, (void *)&bufferBytes);

		/*void RtAudio::openStream(RtAudio::StreamParameters *outputParameters,
		RtAudio::StreamParameters *inputParameters,
		RtAudioFormat format, unsigned int sampleRate,
		unsigned int *bufferFrames,
		RtAudioCallback callback, void *userData,
		RtAudio::StreamOptions *options,
		RtAudioErrorCallback errorCallback)*/
	}
	catch (RtError& e) {
		e.printMessage();
		exit(0);
	}
	bufferBytes = bufferFrames * 2 * 4;
	try {
		adac->startStream();
		//char input;
		//std::cout << "\nRunning ... press <enter> to quit.\n";
		DebugOut("\nKUNLQT Running ... press <enter> to quit.\n");
		//std::cin.get(input);
		// Stop the stream.
		//adac->stopStream();
	}

	catch (RtError& e) {
		DebugOut("\nKUNLQT startStream Exception.\n");
		e.printMessage();
		//goto cleanup;
	}
//cleanup:
//	if (adac->isStreamOpen()) {
//		DebugOut("\nKUNLQT closeStream.....\n");
//		//adac->closeStream();
//	}
}

void MainPage::btnStopCapture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//StopCapture(sender, e);
}
#pragma endregion

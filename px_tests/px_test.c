#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "portmixer.h"
#include "portaudio.h"

#define STREAM_SAMPLE_RATE  48000

static int DummyCallbackFunc(const void *input, void *output,
                             unsigned long frameCount,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags, void *userData)
{
   return 0;
}

static void print_input_mixer_info(PxMixer *mixer)
{
        int num, j;
        printf("  Master volume: %.2f\n", Px_GetMasterVolume(mixer));
        num = Px_GetNumInputSources(mixer);
        printf("  Num input sources: %d\n", num);
        for (j = 0; j < num; j++) {
                printf("    Input %d (%s) %s\n",
                        j,
                        Px_GetInputSourceName(mixer, j),
                        (Px_GetCurrentInputSource(mixer) == j ?
                                "SELECTED" : ""));
        }
        printf("  Input volume: %.2f\n", Px_GetInputVolume(mixer));
}

static void print_output_mixer_info(PxMixer *mixer)
{
    int num, j;
	printf("  Master volume: %.2f\n", Px_GetMasterVolume(mixer));
	printf("  PCM output volume: %.2f\n", Px_GetPCMOutputVolume(mixer));
	num = Px_GetNumOutputVolumes(mixer);
	printf("  Num outputs: %d\n", num);
	for (j = 0; j < num; j++) {
		printf("    Output %d (%s): %.2f\n",
			j,
			Px_GetOutputVolumeName(mixer, j),
			Px_GetOutputVolume(mixer, j));
	}
}

static void set_mixer_volume(PxMixer *mixer, PxVolume new_vol, bool isInput, bool isOutput)
{
    if(isOutput)
    {
        Px_SetMasterVolume(mixer, new_vol);
        printf("  New master volume: %.2f\n", Px_GetMasterVolume(mixer));
    }
    if(isInput)
    {
//        Px_SetInputVolume(mixer, new_vol);
        printf("  New input volume: %.2f\n", Px_GetInputVolume(mixer));
    }
}

static void print_mixer_info(PxMixer *mixer, bool isInput, bool isOutput)
{
    if(isInput)
    {
        print_input_mixer_info(mixer);
    }
    if(isOutput)
    {
        print_output_mixer_info(mixer);
    }
}

static void print_default_device(PaDeviceIndex i, PaHostApiIndex api_idx)
{
/* Mark global and API specific default devices */
    int defaultDisplayed = 0;
    const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(api_idx);
    if( i == Pa_GetDefaultInputDevice() )
    {
        printf( "[ Default Input" );
        defaultDisplayed = 1;
    }
    else if( i == hostInfo->defaultInputDevice )
    {
        printf( "[ Default %s Input", hostInfo->name );
        defaultDisplayed = 1;
    }

    if( i == Pa_GetDefaultOutputDevice() )
    {
        printf( (defaultDisplayed ? "," : "[") );
        printf( " Default Output" );
        defaultDisplayed = 1;
    }
    else if( i == hostInfo->defaultOutputDevice )
    {
        printf( (defaultDisplayed ? "," : "[") );
        printf( " Default %s Output", hostInfo->name );
        defaultDisplayed = 1;
    }

    if( defaultDisplayed )
        printf( " ]\n" );
}

int main(int argc, char **argv)
{
   (void)argc; (void)argv;
   PaError error;
   PaStream *stream;
   PxMixer *main_mixer;
   PaStreamParameters *in_params, *out_params;
   PaStreamParameters inputParameters, outputParameters;

   srand((unsigned)time(NULL));
   if( (error = Pa_Initialize()) != paNoError ) {
       printf("Pa_Initialize() error %d: %s\n", error,
              Pa_GetErrorText(error));
       return -1;
   }
   printf( "PortAudio version: 0x%08X\n", Pa_GetVersion());
   printf( "Version text: '%s'\n", Pa_GetVersionInfo()->versionText );

   PaDeviceIndex numDevices = Pa_GetDeviceCount();
   if( numDevices < 0 )
   {
       printf( "Error: Pa_GetDeviceCount returned 0x%x\n", numDevices );
       Pa_Terminate();
       return -1;
   }
   printf( "Number of devices: %d\n", numDevices );

   for(PaDeviceIndex i = 0; i < numDevices; i++)
   {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        printf("--------------------------------------- device #%d\n", i);
        print_default_device(i, deviceInfo->hostApi);
#ifdef WIN32
        {   /* Use wide char on windows, so we can show UTF-8 encoded device names */
            wchar_t wideName[4096];
            MultiByteToWideChar(CP_UTF8, 0, deviceInfo->name, -1, wideName, 4096);
            wprintf( L"Name                        = %s\n", wideName );
        }
#else
        printf( "Name                        = %s\n", deviceInfo->name );
#endif
        printf( "Host API                    = %s\n",  Pa_GetHostApiInfo( deviceInfo->hostApi )->name );
        printf( "Default sample rate         = %8.2f\n", deviceInfo->defaultSampleRate );

        if(deviceInfo->maxInputChannels > 0)
        {
            memset(& inputParameters, 0, sizeof(inputParameters));
            inputParameters.device = i;
            inputParameters.channelCount = deviceInfo->maxInputChannels;
            inputParameters.sampleFormat = paFloat32;
            in_params = & inputParameters;
        } else {
            in_params = NULL;
        }
        if(deviceInfo->maxOutputChannels > 0)
        {
            memset(& outputParameters, 0, sizeof(outputParameters));
            outputParameters.device = i;
            outputParameters.channelCount = deviceInfo->maxOutputChannels;
            outputParameters.sampleFormat = paFloat32;
            out_params = & outputParameters;
        } else {
            out_params = NULL;
        }

        error = Pa_OpenStream(&stream, in_params, out_params, deviceInfo->defaultSampleRate,
                              512, paClipOff | paDitherOff, DummyCallbackFunc, NULL);
        if (error != paNoError)
        {
           printf("Pa_OpenStream() error %d: %s\n", error, Pa_GetErrorText(error));
           continue;
        }

        main_mixer = Px_OpenMixer(stream, 0);
        if (!main_mixer) {
           printf("Px_OpenMixer() error: Could not open mixer!\n");
           Pa_CloseStream(stream);
           continue;
        }
        const PxVolume vol = rand() / (float)RAND_MAX;

        print_mixer_info(main_mixer, in_params != NULL, out_params != NULL);
        set_mixer_volume(main_mixer, vol, in_params != NULL, out_params != NULL);
        const int num_mixers = Px_GetNumMixers(main_mixer);
        if (num_mixers > 0)
        {
            printf("Number of mixers: %d\n", num_mixers);
            for (int i = 0; i < num_mixers; i++)
            {
                printf("Mixer %d: %s\n", i, Px_GetMixerName(stream, i));
                PxMixer *mixer = Px_OpenMixer(stream, i);
                if (!mixer) {
                    printf("Px_OpenMixer() error: Could not open mixer %d!\n", i);
                    continue;
                }
                print_mixer_info(mixer, in_params != NULL, out_params != NULL);
                set_mixer_volume(mixer, vol, in_params != NULL, out_params != NULL);
                Px_CloseMixer(mixer);
            }
        }
        Px_CloseMixer(main_mixer);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
    return 0;
}

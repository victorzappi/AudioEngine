/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * AudioEngine.h
 *
 *  Created on: Oct 14, 2013
 *      Author: Victor Zappi
 *
 *  This class has been created starting from the example file /test/pcm.c
 *  Published by
 *  Jaroslav Kysela [perex@perex.cz]
 *  Abramo Bagnara [abramo@alsa-project.org],
*   Takashi Iwai [tiwai@suse.de]
*   Frank van de Pol [fvdpol@coil.demon.nl]
*   at
*   https://www.alsa-project.org/alsa-doc/alsa-lib/index.html
*   under the GPL license
*
 */

#ifndef AUDIO_ENGINE_H_
#define AUDIO_ENGINE_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include <iostream>
#include <string>

// comment or uncomment this if you have or don't have installed the SSE libraries [which are mainly includes in /usr/local]
//#define SSE_SUPPORT

// this only if on BBB, which is an ARM platform
#ifdef __arm__
//mem fix
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>


#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
#endif


#include "AudioGenerator.h"

#define MAX_NUM_OF_AUDIOMODULES_OUT 10
#define MAX_NUM_OF_AUDIOMODULES_INOUT MAX_NUM_OF_AUDIOMODULES_OUT

struct audioThread_data {
   int  argc;
   char **argv;
};



//TODO introduce asynchronous callback for both write and read/write cases
// https://alsa.opensrc.org/Asynchronous_Playback_(Howto)
// and
// https://github.com/robelsharma/IdeaAudio


class AudioEngine {
public:
	struct transfer_method {
		const char *name;
		snd_pcm_access_t access;
		int (AudioEngine::*transfer_loop)();
	};

	// for all data structures and settings that may differ between capture and playback
	struct audioStructure {
		snd_pcm_t *handle;  			// device handle
		char *rawSamples;	    			// period buffer for interleaved/non-interleaved raw samples [in bytes, smallest and most generic type]
		unsigned char **rawSamplesStartAddr;		// address of first raw sample of each channel
		double **frameBuffer;			// multidimensional period buffers for float samples, one per each channel
		snd_pcm_channel_area_t *areas;	// to easily deal with interleaved/non-interleaved samples
		snd_pcm_hw_params_t *hwparams;  // hardware parameter settings
		snd_pcm_sw_params_t *swparams;  // software parameter settings

		// params
		char device[15];		 // device, e.g., hw:0,0. we don't wanna deal with this, better to use cardName
		std::string cardName;	 // audio card name, e.g., HDA Intel PCH
		snd_pcm_format_t format; // sample format
		unsigned short channels;	 // count of channels
		bool isBlocking;		 // is read/write command blocking?

		// low level settings, derived from format
		int byteStep;	     // distance between consecutive samples in buffer of frames [char *samples]
		int formatBits;	     // number of bits per sample
		unsigned int maxVal; // maximum value that can be represented
		int bps;  			 // bytes per sample
		int physBps;		 // bytes per physical representation of sample [???]
		bool isBigEndian;	 // is big endian?
		bool isUnsigned;     // is unsigned?
		bool isFloat;		 // is float?
		int mask;			 // used for 2's complement, capture only
	};

	transfer_method transfer_methods[1]; //TODO re-enable other methods

	AudioEngine();
	virtual ~AudioEngine();

	// usage method
	int initEngine();
	int startEngine();
	int stopEngine();

	virtual void addAudioModule(AudioModule *mod);


	// external params setup [inline methods]
	void setFullDuplex(bool duplex);
	void setPeriodSize(int size);
	void setBufferSize(int size);
	void setRate(int srate);
	void setTransferMethod(int i);

	void setPlaybackDevice(const char *name);
	void setPlaybackDevice(int card, int dev);
	void setPlaybackCardName(std::string name);
	virtual void setPlaybackAudioFormat(snd_pcm_format_t fmt);
	void setPlaybackChannelNum(int n);

	void setCaptureDevice(const char *name);
	void setCaptureDevice(int card, int dev);
	void setCaptureCardName(std::string name);
	virtual void setCaptureAudioFormat(snd_pcm_format_t fmt);
	void setCaptureChannelNum(int n);

	void setVerbose(int v);
	void setResample(int r);
	void setPeriodEvent(int p);
	void setVolume(double v);

	// getters (;
	//double getVolume();
	unsigned int getRate();
	unsigned long getPeriodSize();
	unsigned long getBufferSize();
	unsigned short getPlaybackChannelsNum();
	unsigned short getCaptureChannelsNum();

protected:
	bool isFullDuplex; // to enable capture

	// settings shared between playback and capture
	unsigned int rate;          	// audio rate
	snd_pcm_uframes_t buffer_size;	// ring buffer length in frames
	snd_pcm_uframes_t period_size;	// period length in frames
	unsigned short ringbuffer_ratio; // buffer size / period size
	int method;						// alsa low level buffer transfer method
	bool resample;      	        // enable alsa-lib resampling
	bool periodEvent;               // produce poll event after each period?

	// specific settings
	audioStructure playback;
	audioStructure capture;

	// utils
	//double volume;		 // master volume
	//double ref_volume;	 // to interpolate volume [MUST interpolate to modify volume]
	//double inc_volume;	 // to interpolate volume [MUST interpolate to modify volume]
	//double delta_volume; // to interpolate volume [MUST interpolate to modify volume]
	bool verbose;        // verbose flag

	// shared data structures
	int numOfAudioModulesOut;   // number of audio outputs
	int numOfAudioModulesInOut; // number of audio input/outputs
	int numOfAudioModules;		// sum
	AudioModuleOut* audioModulesOut[MAX_NUM_OF_AUDIOMODULES_OUT];
	AudioModuleInOut *audioModulesInOut[MAX_NUM_OF_AUDIOMODULES_INOUT];
	unsigned short audioModulesChannels[MAX_NUM_OF_AUDIOMODULES_OUT+MAX_NUM_OF_AUDIOMODULES_INOUT];
	unsigned short audioModulesChnOffset[MAX_NUM_OF_AUDIOMODULES_OUT+MAX_NUM_OF_AUDIOMODULES_INOUT];


	double **moduleFramebuffer[MAX_NUM_OF_AUDIOMODULES_OUT+MAX_NUM_OF_AUDIOMODULES_INOUT];

	//float audioSample;							   // to read from audio outputs and input/outputs a single sample at a time...lame but whatever
	double **silenceBuff; // used when no full duplex but in/out modules are used

	// global status
	bool engineReady;	  // engine has been initialized?
	bool engineIsRunning; // engine is running?


	int findDevice(std::string cardName, char *device);
	int printCardName(char *device);
	int setHwParams(audioStructure &audio);
	int setSwParams(audioStructure audio);
	int setLowLevelParams(audioStructure &audio);

	virtual void readAudioModulesBuffers(int numOfSamples, double **framebufferOut, double **framebufferIn);

	int shutEngine();
	int preparePcm(bool reset=false);
	int underrunRecovery(int err);
	int overrunRecovery(int err);

	//int interpolateVolume(); // maybe i was not clear, MUST interpolate to modify volume

	virtual void initRender();
	virtual void render(float sampleRate, int numOfSamples, int numOutChannels, double **framebufferOut, int numInChannels, double **framebufferIn);
	virtual void cleanUpRender();

	int (AudioEngine::*writeAudio)(long);
	int writeAudio_block(long numSamples);
	int writeAudio_nonBlock(long numSamples);

	long (AudioEngine::*readAudio)(long);
	long readAudio_block(long numSamples);
	long readAudio_nonBlock(long numSamples);

	int audioLoop_write();
	int audioLoop_readWrite();
	int (AudioEngine::*audioLoop)();

	//VIC continue this splitting, also for capture, then check capture quality...
	void (AudioEngine::*byteSplit)(int, unsigned char **, int);
	void byteSplit_littleEndian(int chn, unsigned char **sampleBytes, int value);
	void byteSplit_bigEndian(int chn, unsigned char **sampleBytes, int value);

	int (AudioEngine::*byteCombine)(int, unsigned char **);
	int byteCombine_littleEndian(int chn, unsigned char **sampleBytes);
	int byteCombine_bigEndian(int chn, unsigned char **sampleBytes);

	void (AudioEngine::*fromFloatToRaw)(snd_pcm_uframes_t, int);
	virtual void fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromFloatToRaw_uint(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromFloatToRaw_float32(snd_pcm_uframes_t offset, int numSamples);
	//virtual void fromFloatToRaw_ufloat(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromFloatToRaw_float64(snd_pcm_uframes_t offset, int numSamples);

	void (AudioEngine::*fromRawToFloat)(snd_pcm_uframes_t, int);
	virtual void fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromRawToFloat_uint(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromRawToFloat_float32(snd_pcm_uframes_t offset, int numSamples);
	//virtual void fromRawToFloat_ufloat(snd_pcm_uframes_t offset, int numSamples);
	virtual void fromRawToFloat_float64(snd_pcm_uframes_t offset, int numSamples);


};



//-----------------------------------------------------------
//-----------------------------------------------------------
// other core headers that are handy to the user
#include "MonoEngine_int32LE.h"
//-----------------------------------------------------------
//-----------------------------------------------------------



//-----------------------------------------------------------
// external params setup
//-----------------------------------------------------------
inline void AudioEngine::setFullDuplex(bool duplex) {
	if(engineReady) {
		printf("Cannot change full duplex setting after engine is initialized!\n");
		return;
	}
	isFullDuplex = duplex;
}

inline void AudioEngine::setPeriodSize(int size) {
	if(engineReady) {
		printf("Cannot set period size after engine is initialized!\n");
		return;
	}
	period_size = (snd_pcm_uframes_t) size;
}
inline void AudioEngine::setBufferSize(int size) {
	if(engineReady) {
		printf("Cannot set buffer size after engine is initialized!\n");
		return;
	}
	buffer_size = (snd_pcm_uframes_t) size;
}
inline void AudioEngine::setRate(int srate) {
	if(engineReady) {
		printf("Cannot set audio rate after engine is initialized!\n");
		return;
	}
	rate = (unsigned int) srate;
}
inline void AudioEngine::setTransferMethod(int i) {
	if(engineReady) {
		printf("Cannot set transfer method after engine is initialized!\n");
		return;
	}
	method = i;
}

inline void AudioEngine::setPlaybackDevice(const char *name) {
	if(engineReady) {
		printf("Cannot set playback device after engine is initialized!\n");
		return;
	}
	strcpy(playback.device, name);
}
inline void AudioEngine::setPlaybackDevice(int card, int dev) {
	if(engineReady) {
		printf("Cannot set playback device after engine is initialized!\n");
		return;
	}
	sprintf(playback.device, "hw:%d,%d", card, dev);
}
inline void AudioEngine::setPlaybackCardName(std::string name) {
	if(engineReady) {
		printf("Cannot set playback card name after engine is initialized!\n");
		return;
	}
	playback.cardName = name;
}
inline void AudioEngine::setPlaybackAudioFormat(snd_pcm_format_t fmt) {
	if(engineReady) {
		printf("Cannot set playback audio format after engine is initialized!\n");
		return;
	}
	playback.format = fmt;
}
inline void AudioEngine::setPlaybackChannelNum(int n) {
	if(engineReady) {
		printf("Cannot set playback channel number after engine is initialized!\n");
		return;
	}
	playback.channels = n;
}

inline void AudioEngine::setCaptureDevice(const char *name) {
	if(engineReady) {
		printf("Cannot set capture device after engine is initialized!\n");
		return;
	}
	strcpy(capture.device, name);
}
inline void AudioEngine::setCaptureDevice(int card, int dev) {
	if(engineReady) {
		printf("Cannot set capture device after engine is initialized!\n");
		return;
	}
	sprintf(capture.device, "hw:%d,%d", card, dev);
}
inline void AudioEngine::setCaptureCardName(std::string name) {
	if(engineReady) {
		printf("Cannot set capture card name after engine is initialized!\n");
		return;
	}
	capture.cardName = name;
}
inline void AudioEngine::setCaptureAudioFormat(snd_pcm_format_t fmt) {
	if(engineReady) {
		printf("Cannot set capture audio format after engine is initialized!\n");
		return;
	}
	capture.format = fmt;
}
inline void AudioEngine::setCaptureChannelNum(int n) {
	if(engineReady) {
		printf("Cannot set capture channel number after engine is initialized!\n");
		return;
	}
	capture.channels = n;
}

inline void AudioEngine::setVerbose(int v) {
	verbose = v;
}
inline void AudioEngine::setResample(int r) {
	if(engineReady) {
		printf("Cannot set resampling behavior after engine is initialized!\n");
		return;
	}
	resample = r;
}
inline void AudioEngine::setPeriodEvent(int p) {
	if(engineReady) {
		printf("Cannot set period event behavior after engine is initialized!\n");
		return;
	}
	periodEvent = p;
}
/*inline void AudioEngine::setVolume(double v) {
	//pthread_mutex_lock (&vol_lock);
	ref_volume = v;
	inc_volume = (ref_volume-volume)/buffer_size;
	//pthread_mutex_unlock (&vol_lock);
}*/
//-----------------------------------------------------------


/*inline double AudioEngine::getVolume() {
	return volume;
}*/
inline unsigned int AudioEngine::getRate() {
	return rate;
}
inline unsigned long AudioEngine::getPeriodSize() {
	if(!engineReady) {
		printf("Warning! Cannot retrieve period size from Engine if engine has not been initialized!\n");
		return -1;
	}
	return period_size;
}
inline unsigned long AudioEngine::getBufferSize() {
	if(!engineReady) {
		printf("Warning! Cannot retrieve buffer size from Engine if engine has not been initialized!\n");
		return -1;
	}
	return buffer_size;
}

inline unsigned short AudioEngine::getPlaybackChannelsNum() {
	if(!engineReady) {
		printf("Warning! Cannot retrieve number of playback channels from Engine if engine has not been initialized!\n");
		return -1;
	}
	return playback.channels;
}
inline unsigned short AudioEngine::getCaptureChannelsNum() {
	if(!engineReady) {
		printf("Warning! Cannot retrieve number of capture channels from Engine if engine has not been initialized!\n");
		return -1;
	}
	return capture.channels;
}


/*inline int AudioEngine::interpolateVolume() {
	delta_volume = fabs(ref_volume-volume);

	if(delta_volume>0.0001) {
		volume += inc_volume;//volume += deltaParam/200.0;
		return 1;
	}
	else if(delta_volume!=0) {
		volume = ref_volume;
		return 1;
	}
	else
		return 0;
}*/

inline int AudioEngine::preparePcm(bool reset) {
	int err;

	if(reset) {
		snd_pcm_drop(playback.handle);
		if(isFullDuplex)
			snd_pcm_drop(capture.handle);
	}

	if ((err = snd_pcm_prepare(playback.handle)) < 0) {
		printf("Playback prepare error: %s\n", snd_strerror(err));
		return err;
	}
	if (snd_pcm_format_set_silence(playback.format, playback.rawSamples, period_size*playback.channels) < 0) {
		fprintf(stderr, "silence error\n");
		return err;
	}

	if(reset && isFullDuplex) {
		if ((err = snd_pcm_unlink(capture.handle)) < 0) {
			printf("Capture and Playback streams unlink error: %s\n", snd_strerror(err));
			return err;
		}
	}

	// if full duplex link and start capture device
	if(isFullDuplex) {
		if ((err = snd_pcm_link(capture.handle, playback.handle)) < 0) {
			printf("Capture and Playback Streams link error: %s\n", snd_strerror(err));
			return err;
		}
		if (snd_pcm_format_set_silence(capture.format, capture.rawSamples, period_size*capture.channels) < 0) {
			fprintf(stderr, "silence error\n");
			return err;
		}
		for(unsigned short i=0; i<ringbuffer_ratio; i++) {
			if ((*this.*writeAudio)(period_size) < 0) {
				fprintf(stderr, "write error\n");
				return err;
			}
		}
		if ((err = snd_pcm_start(capture.handle)) < 0) {
			printf("Capture start error: %s\n", snd_strerror(err));
			return err;
		}
	}

	return 0;
}


// in all these byte split methods, value will contain a number of bytes equal to the number of bytes of each sample, thanks to the normalization done in calling method!
// e.g., if we are using format SND_PCM_FORMAT_S16_LE, samples will be 2 bytes and only the first 2 value's bytes will contain the actual sample value [other bytes will be filled with zeros]!
inline void AudioEngine::byteSplit_littleEndian(int chn, unsigned char **sampleBytes, int value) {
	for (int i = 0; i < playback.bps; i++)
		*(sampleBytes[chn] + i) = (value >>  i * 8) & 0xff; // splits each integer sample over more bytes [1 or more, according to format]
}
inline void AudioEngine::byteSplit_bigEndian(int chn, unsigned char **sampleBytes, int value) {
	for (int i = 0; i < playback.bps; i++)
		*(sampleBytes[chn] + playback.physBps - 1 - i) = (value >> i * 8) & 0xff; // splits each integer sample over more bytes [1 or more, according to format], but reverses the order
}

// similarly, in these byte combine, the output value will have a number of non-zero bytes equal to the number of bytes within each sample
// then the normalization in the calling method will change the representation of these value to use all the available bytes in our sample buffers
inline int AudioEngine::byteCombine_littleEndian(int chn, unsigned char **sampleBytes) {
	int value = 0;
	for (int i = 0; i < capture.bps; i++)
		value += (int) (*(sampleBytes[chn] + i)) << i * 8;  // combines each sample [which stretches over 1 or more bytes, according to format] in single integer
	return value;
}
inline int AudioEngine::byteCombine_bigEndian(int chn, unsigned char **sampleBytes) {
	int value = 0;
	for (int i = 0; i < capture.bps; i++)
		value += (int) (*(sampleBytes[chn]  + capture.physBps - 1 - i)) << i * 8; // combines each sample [which stretches over 1 or more bytes, according to format] in single integer, but reverses the order
	return value;
}


#endif /* AUDIO_ENGINE_H_ */

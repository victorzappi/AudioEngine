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


#include "../include/AudioGenerator.h"


#define MAX_NUM_OF_GENERATORS 10

struct audioThread_data {
   int  argc;
   char **argv;
};



class AudioEngine {
public:
	struct transfer_method {
		const char *name;
		snd_pcm_access_t access;
		int (AudioEngine::*transfer_loop)();
	} transfer_methods[1];


	AudioOutput *generator[MAX_NUM_OF_GENERATORS];

	AudioEngine();
	virtual ~AudioEngine();

	// usage method
	int initEngine();
	int startEngine();
	int stopEngine();

	void addGenerator(AudioOutput *g);

	// external params setup [inline methods]
	void setPeriodSize(int size);
	void setBufferSize(int size);
	void setDevice(const char *name);
	void setDevice(int card, int dev);
	void setCardName(std::string name);
	void setRate(int srate);
	void setChannelNum(int n);
	void setTransferMethod(int i);
	void setAudioFormat(snd_pcm_format_t fmt);
	void setVerbose(int v);
	void setResample(int r);
	void setPeriodEvent(int p);
	void setVolume(double v);

	double getVolume();
	unsigned int getRate();
	unsigned long getPeriodSize();
	unsigned long getBufferSize();

	double readGeneratorsSample();
	float *readGeneratorsBuffer(int numOfSamples);

protected:
	int err;
	snd_pcm_t *handle;
	snd_pcm_uframes_t frames;
	signed short *samples;
	unsigned int chn;
	snd_pcm_channel_area_t *areas;
	snd_output_t *output;
	int formatInt;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;

	snd_pcm_uframes_t period_size;	/* period length in frames */
	snd_pcm_uframes_t buffer_size;	/* ring buffer length in frames */
	std::string cardName;			/* audio card name, e.g., HDA Intel PCH*/
	char device[15];				/* playback device, e.g., hw:0,0 -> possibly we do not wanna deal with this shit!*/
	snd_pcm_format_t format;    	/* sample format */
	unsigned int rate;          	/* stream rate */
	unsigned int channels;			/* count of channels */
	unsigned int period_time;      	/* period time in us */
	float **gSampleBuffers;         /* Buffers that hold samples from render() */
	int verbose;                    /* verbose flag */
	int resample;      	            /* enable alsa-lib resampling */
	int period_event;               /* produce poll event after each period */
	int method;						/* alsa low level buffer transfer method */

	double volume;					/* overall volume*/
	bool engineReady;				/* engine has been initialized*/
	bool engineIsRunning;			/* current state */



	int byteStep;					/* step between samples on the same channel [in bytes]*/
	int format_bits;
	unsigned int maxval;
	int bps;  						/* bytes per sample */
	int phys_bps;
	int big_endian;
	int to_unsigned;
	int is_float;
	float audioSample;		   // no sue for double...read below
	float *audioSampleBuffer;  // no use for double version, cos audio engine passes buffer directly to audio card, as float numbers
	float *genSampleBuffer[MAX_NUM_OF_GENERATORS]; // to store sample buffers before volume interpolation
	int numOfOutputs;



	pthread_mutex_t vol_lock;

	double ref_volume;
	double inc_volume;
	double delta_volume;

	int printCardName();
	int findDevice();
	int set_hwparams();
	int set_swparams();
	int write_loop();
	int shut_engine();
	int xrun_recovery();
	void fill_period(snd_pcm_uframes_t offset, int count);
	void clampGeneratorSample();

	int interpolateVolume();

	virtual void initRender();
	virtual void render(float sampleRate, int numChannels, int numSamples,
		float **sampleBuffers);
	virtual void cleanUpRender();


	// this only if on BBB, which is an ARM platform
	#ifdef __arm__
	void pokeMem(const char *argv1, const char *argv2, const char *argv3);
	#endif

};

//TODO use Out object container to include both Out and InOut objects [and do not use "Generator" in names anymore]
inline void AudioEngine::addGenerator(AudioOutput *g) {
	generator[numOfOutputs++] = g;
}

//-----------------------------------------------------------
// external params setup
//-----------------------------------------------------------
inline void AudioEngine::setPeriodSize(int size) {
	period_size = (snd_pcm_uframes_t) size;
}

inline void AudioEngine::setBufferSize(int size) {
	buffer_size = (snd_pcm_uframes_t) size;
}

inline void AudioEngine::setDevice(const char *name) {
	strcpy(device, name);
}

inline void AudioEngine::setDevice(int card, int dev) {
	sprintf(device, "hw:%d,%d", card, dev);
}

inline void AudioEngine::setCardName(std::string name) {
	cardName = name;
}

inline void AudioEngine::setRate(int srate) {
	rate = (unsigned int) srate;
}

inline void AudioEngine::setChannelNum(int n) {
	channels = n;
}

inline void AudioEngine::setTransferMethod(int i) {
	method = i;
}

inline void AudioEngine::setAudioFormat(snd_pcm_format_t fmt) {
	format = fmt;
}

inline void AudioEngine::setVerbose(int v) {
	verbose = v;
}

inline void AudioEngine::setResample(int r) {
	resample = r;
}

inline void AudioEngine::setPeriodEvent(int p) {
	period_event = p;
}

inline void AudioEngine::setVolume(double v) {
	//pthread_mutex_lock (&vol_lock);
	ref_volume = v;
	inc_volume = (ref_volume-volume)/buffer_size;
	//pthread_mutex_unlock (&vol_lock);
}
//-----------------------------------------------------------


inline double AudioEngine::getVolume() {
	return volume;
}

inline unsigned int AudioEngine::getRate() {
	return rate;
}

inline unsigned long AudioEngine::getPeriodSize() {
	return period_size;
}

inline unsigned long AudioEngine::getBufferSize() {
	return buffer_size;
}



inline int AudioEngine::interpolateVolume() {
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
}


#endif /* AUDIO_ENGINE_H_ */

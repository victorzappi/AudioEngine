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
 * AudioModules.h
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

#ifndef AUDIOCMODULES_H_
#define AUDIOCMODULES_H_

#include <iostream>  // NULL
#include <math.h>    // M_PI
#include <cstring>   // memcopy, memset
#include <sndfile.h> // to load audio files

enum oscillator_type {osc_sin_, osc_square_, osc_tri_, osc_saw_, osc_whiteNoise_, osc_impTrain_, osc_const_, /*osc_w_*/}; /// shared definition between Waveforms and Oscillator



//----------------------------------------------------------------------------------
// Abstract base classes
//----------------------------------------------------------------------------------
class AudioModule {
public:
	AudioModule();
	virtual void init(unsigned int periodSize) = 0;
	virtual double getLevel();
	virtual void setLevel(double level);

	virtual ~AudioModule();


protected:
	unsigned int period_size;
	double level;
	double **framebuffer;

	virtual void allocateFramebuffer(unsigned short channels);
	virtual void deleteFramebuffer(unsigned short channels);

};
inline AudioModule::AudioModule() {
	period_size = -1;
	level = 1;
	framebuffer = NULL;
}
inline void AudioModule::setLevel(double level) {
	this->level = level;
}
inline double AudioModule::getLevel() {
	return level;
}
inline AudioModule::~AudioModule() {
}

inline void AudioModule::allocateFramebuffer(unsigned short channels) {
	framebuffer = new double *[channels];
	for(int i=0; i<channels; i++) {
		framebuffer[i] = new double[period_size];
		memset(framebuffer[i], 0, sizeof(double)*period_size);
	}
}

inline void AudioModule::deleteFramebuffer(unsigned short channels) {
	if(framebuffer != NULL) {
		for(int i=0; i<channels; i++)
			delete[] framebuffer[i];
		delete[] framebuffer;
	}
}
//------------------------------------------------------------------------------------------





//------------------------------------------------------------------------------------------
// second level, still abstract
//------------------------------------------------------------------------------------------

class MultichannelOutUtils; // for friendship


class AudioModuleOut : public AudioModule {
public:
	friend class MultichannelOutUtils;

	AudioModuleOut();
	void init(unsigned int periodSize);
	virtual void init(unsigned int periodSize, unsigned short outChannels, unsigned short outChnOffset=0);
	virtual void retrigger() = 0;
	virtual double **getFrameBuffer(int numOfSamples) = 0;
	int getOutChannnelsNum();
	int getOutChannnelOffset();

	virtual ~AudioModuleOut();
protected:
	unsigned short out_channels;
	unsigned short out_chn_offset;
};
inline AudioModuleOut::AudioModuleOut() {
	out_channels = 0;
	out_chn_offset = 0;
}
inline void AudioModuleOut::init(unsigned int periodSize) {
	init(periodSize, 1, 0);
}

inline void AudioModuleOut::init(unsigned int periodSize, unsigned short outChannels, unsigned short outChnOffset) {
	if(framebuffer!=NULL)
		AudioModule::deleteFramebuffer(out_channels+out_chn_offset);

	period_size = periodSize;
	out_channels = outChannels;
	out_chn_offset = outChnOffset;
	AudioModule::allocateFramebuffer(out_channels+out_chn_offset);
}
inline AudioModuleOut::~AudioModuleOut() {
	AudioModule::deleteFramebuffer(out_channels);
}
inline int AudioModuleOut::getOutChannnelsNum() {
	return out_channels;
}
inline int AudioModuleOut::getOutChannnelOffset() {
	return out_chn_offset;
}





class AudioModuleInOut : public AudioModuleOut  {
public:
	AudioModuleInOut();
	void init(unsigned int periodSize, unsigned short inChannels=1, unsigned short inChnOffset=0, unsigned short outChannels=1, unsigned short outChnOffset=0);
	virtual double **getFrameBuffer(int numOfSamples, double **input) = 0;
	double **getFrameBuffer(int numOfSamples);
	int getInChannnelsNum();
	int getInChannnelOffset();

	//virtual ~AudioModuleInOut();
protected:
	unsigned short in_channels;
	unsigned short in_chn_offset;
};
inline AudioModuleInOut::AudioModuleInOut() {
	in_channels = 0;
	in_chn_offset = 0;
}
inline void AudioModuleInOut::init(unsigned int periodSize, unsigned short inChannels, unsigned short inChnOffset, unsigned short outChannels, unsigned short outChnOffset) {
	AudioModuleOut::init(periodSize, outChannels, outChnOffset);
	in_channels = inChannels;
	in_chn_offset = inChnOffset;
}
inline double** AudioModuleInOut::getFrameBuffer(int numOfSamples) {
	return getFrameBuffer(numOfSamples, NULL);
}
/*inline AudioModuleInOut::~AudioModuleInOut() {
}*/
inline int AudioModuleInOut::getInChannnelsNum() {
	return in_channels;
}
inline int AudioModuleInOut::getInChannnelOffset() {
	return in_chn_offset;
}


//------------------------------------------------------------------------------------------









//------------------------------------------------------------------------------------------
// utility class
//------------------------------------------------------------------------------------------
class MultichannelOutUtils {
protected:
	MultichannelOutUtils(AudioModuleOut *outModule);
	//inline ~MultichannelOutUtils() {};
	void cloneFrameChannels(int numOfSamples);

	AudioModuleOut *out_module;
};
inline MultichannelOutUtils::MultichannelOutUtils(AudioModuleOut *outModule) {
	out_module = outModule;
}

// copies passed sample buffer to chosen consecutive channels within the frame buffer
inline void MultichannelOutUtils::cloneFrameChannels(int numOfSamples) {
	for(int i=1; i<out_module->out_channels; i++)
		memcpy(out_module->framebuffer[out_module->out_chn_offset+i], out_module->framebuffer[out_module->out_chn_offset], sizeof(double)*numOfSamples);
}





#endif /* AUDIOCMODULES_H_ */

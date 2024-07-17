/*
 * AudioGeneratorOut.h
 *
 *  Created on: Oct 22, 2013
 *      Author: Victor Zappi
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_


#include <iostream>  // NULL
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>    // M_PI
#include <cstring>   // memcopy, memset

#include "Waveforms.h"
#include "Oscillator.h"
#include "Passthrough.h"

#include "Biquad.h"
#include "ADSR.h"
#include "PinkNoise.h"

#define MAX_MODULES_NUM 10



//----------------------------------------------------------------------------------
// Non-instantiatable  base class
//----------------------------------------------------------------------------------
class Generator {
public:
	void setVolume(double v);
	double getVolume();

	virtual ~Generator() {};

protected:
	double volume;
	double volumeInterp[2];

	unsigned int rate;
	unsigned long interpSize;

	Generator();
	void prepareInterpolateParam(double param, double *interpParam, double val);
	int interpolateParam(double ref_param, double &param, double inc_param);
};

inline 	Generator::	Generator() {
	volume = -1;
	volumeInterp[0] = volumeInterp[1] = -1;
	rate = 0;
	interpSize = 0;
}


inline void Generator::prepareInterpolateParam(double param, double *interpParam, double val) {
	double ref_val = val;
	double inc_val = (ref_val-param)/(double)interpSize;

	double interp[2] = {ref_val, inc_val};

	memcpy(interpParam, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
}

// useful method to interpolate generic parameter controls coming from parallel threads
inline int Generator::interpolateParam(double ref_param, double &param, double inc_param) {
	double delta_param = ref_param-param;//fabs(ref_param-param);

	if(delta_param>0.0001 || delta_param<-0.0001) {
		param += inc_param;//param += delta_param/200.0;
		return 0;
	}
	else if(delta_param!=0) {
		param = ref_param;
		return 0;
	}
	else
		return 1;
}

// set reference and prepare increment for linear interpolation
inline void Generator::setVolume(double v) {
	double ref_vol = v;
	double inc_vol = (ref_vol-volume)/(double)interpSize;

	double interp[2] = {ref_vol, inc_vol};

	memcpy(volumeInterp, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
}

inline double Generator::getVolume() {
	return volume;
}



//-------------------------------------------------------------------------------------------
// First level child customizable abstract classes, to create your own generators
//-------------------------------------------------------------------------------------------
class AudioGeneratorOut : public Generator, public AudioModuleOut {
public:
	AudioGeneratorOut();

	void init(unsigned int periodSize, unsigned short outChannels, unsigned short outChnOffset=0);
	void init(unsigned int periodSize);


protected:
	AudioModuleOut **audioModulesOut; // may use this or not, as you please
};

inline 	AudioGeneratorOut::AudioGeneratorOut() : Generator() {
	audioModulesOut = NULL;
}

inline void AudioGeneratorOut::init(unsigned int periodSize, unsigned short outChannels, unsigned short outChnOffset) {
	AudioModuleOut::init(periodSize, outChannels, outChnOffset);

	interpSize = period_size;
}

inline void AudioGeneratorOut::init(unsigned int periodSize) {
	init(periodSize, 1, 0);
}


class AudioGeneratorInOut : public /*AudioGeneratorOut*/Generator, public AudioModuleInOut {
public:
	AudioGeneratorInOut();
	void init(unsigned int periodSize, unsigned short inChannels=1, unsigned short inChnOffset=0, unsigned short outChannels=1, unsigned short outChnOffset=0);
	//void init(unsigned int periodSize);


protected:
	AudioModuleOut **audioModulesOut;     // may use this or not, as you please
	AudioModuleInOut **audioModulesInOut; // may use this or not, as you please

};

inline 	AudioGeneratorInOut::AudioGeneratorInOut() : Generator() {
	audioModulesOut = NULL;
	audioModulesInOut = NULL;

}

inline void AudioGeneratorInOut::init(unsigned int periodSize, unsigned short inChannels, unsigned short inChnOffset, unsigned short outChannels, unsigned short outChnOffset) {
	AudioModuleInOut::init(periodSize, inChannels, inChnOffset, outChannels, outChnOffset);
	interpSize = period_size;
}

/*inline void AudioGeneratorInOut::init(unsigned int periodSize) {
	init(periodSize, 1, 0, 1, 0);
}*/

//-------------------------------------------------------------------------------------------
// Second level child class, a generic class that can be instantiated and works as a simple wrapper for several Audio Out Modules that will be summed together [and whose volume can be finally interpolated (: ]
//-------------------------------------------------------------------------------------------
class ModuleOutAdder : public AudioGeneratorOut {
public:
	~ModuleOutAdder();
	void init(unsigned int periodSize, double vol=1, unsigned short outChannels=1, unsigned short outChnOffset=0);
	double **getFrameBuffer(int numOfSamples);
	int addAudioModuleOut(AudioModuleOut *mod);

protected:
		unsigned short modulesNum;
		double **modulesFramebuff[MAX_MODULES_NUM];
		bool *modulesChannels[MAX_MODULES_NUM];
		int currentSample;
};

inline 	ModuleOutAdder::~ModuleOutAdder() {
	for(int i=0; i<MAX_MODULES_NUM; i++) {
		if(modulesChannels[i] != NULL)
			delete[] modulesChannels[i];
	}
}


inline void ModuleOutAdder::init(unsigned int periodSize, double vol, unsigned short outChannels, unsigned short outChnOffset) {
	AudioGeneratorOut::init(periodSize, outChannels, outChnOffset);

	audioModulesOut = new AudioModuleOut *[MAX_MODULES_NUM];

	for(int i=0; i<MAX_MODULES_NUM; i++)
		modulesChannels[i] = new bool[out_channels+out_chn_offset];

	volume = vol;
	volumeInterp[0] = vol;
	volumeInterp[1] = 0;

}

inline double **ModuleOutAdder::getFrameBuffer(int numOfSamples) {
	// these are retrieved in advance...
	for(unsigned short i=0; i<modulesNum; i++)
		modulesFramebuff[i] = audioModulesOut[i]->getFrameBuffer(numOfSamples);


	for(unsigned short j=0; j<out_channels; j++) {
		memset(framebuffer[j+out_chn_offset], 0, numOfSamples*sizeof(double));
		for(int n=0; n<numOfSamples; n++) {
			for(unsigned short i=0; i<modulesNum; i++)
				if(modulesChannels[i][j])
					framebuffer[j+out_chn_offset][n] += modulesFramebuff[i][j][n];
			framebuffer[j+out_chn_offset][n] *= volume; //... so that we can do this multiplication and the interpolation once per each sample
			interpolateParam(volumeInterp[0], volume, volumeInterp[1]);	// remove crackles through interpolation
		}
	}

	return framebuffer;
}

inline int ModuleOutAdder::addAudioModuleOut(AudioModuleOut *mod) {
	if(modulesNum>=MAX_MODULES_NUM)
		return 1;
	audioModulesOut[modulesNum++] = mod;

	int moduleTotChns = mod->getOutChannnelOffset() + mod->getOutChannnelsNum();

	for(int i=0; i<moduleTotChns; i++) {
		if(i<out_channels+out_chn_offset)
			modulesChannels[modulesNum][i] = true;
	}

	return 0;
}
#endif /* GENERATOR_H_ */

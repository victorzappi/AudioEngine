/*
 * AudioGenerator.h
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

#include "AudioComponents.h"
#include "Biquad.h"
#include "ADSR.h"
#include "PinkNoise.h"

//TODO new structure:
// define AudioGenerator as abstract base class
// define AudioGeneratorOut derived both from Out and AudioGenerator
// define AudioGeneratorInOut derived both from InOut and AudioGenerator

//----------------------------------------------------------------------------------
// Abstract base class
//----------------------------------------------------------------------------------
class AudioGenerator : public AudioOutput {
public:
	//virtual double getSample() = 0;
	virtual double *getBuffer(int sampleNum) = 0;
	virtual float *getBufferFloat(int sampleNum);
	void setVolume(double v);
	double getVolume();

	virtual ~AudioGenerator();

protected:
	double volume;
	double volumeInterp[2];
	double synthsample;
	double *synthsamplebuffer;
	//float *floatsamplebuffer;

	AudioComponent **audioComponents;

	unsigned int rate;
	unsigned long periodSize;

	void init(unsigned short period_size);
	void prepareInterpolateParam(double param, double *interpParam, double val);
	int interpolateParam(double ref_param, double &param, double inc_param);
};

inline void AudioGenerator::init(unsigned short int period_size) {
	periodSize = period_size;
	synthsamplebuffer = new double[periodSize];
	floatsamplebuffer = new float[periodSize];
	memset(synthsamplebuffer, 0, sizeof(double)*periodSize);
	memset(floatsamplebuffer, 0, sizeof(float)*periodSize);
}

inline float *AudioGenerator::getBufferFloat(int sampleNum) {
	std::copy( getBuffer(sampleNum), synthsamplebuffer + sampleNum, floatsamplebuffer);
	return floatsamplebuffer;
}

inline void AudioGenerator::prepareInterpolateParam(double param, double *interpParam, double val) {
	double ref_val = val;
	double inc_val = (ref_val-param)/(double)periodSize;

	double interp[2] = {ref_val, inc_val};

	memcpy(interpParam, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
}

// useful method to interpolate generic parameter controls coming from parallel threads
inline int AudioGenerator::interpolateParam(double ref_param, double &param, double inc_param) {
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
inline void AudioGenerator::setVolume(double v) {
	double ref_vol = v;
	double inc_vol = (ref_vol-volume)/(double)periodSize;

	double interp[2] = {ref_vol, inc_vol};

	memcpy(volumeInterp, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
}

inline double AudioGenerator::getVolume() {
	return volume;
}

inline AudioGenerator::~AudioGenerator() {
	if(synthsamplebuffer!=NULL)
		delete[] synthsamplebuffer;

	if(floatsamplebuffer!=NULL)
		delete[] floatsamplebuffer;
}

#endif /* GENERATOR_H_ */

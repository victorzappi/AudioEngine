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
 * AudioComponents.h
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

#ifndef AUDIOCOMPONENTS_H_
#define AUDIOCOMPONENTS_H_

#include <iostream>  // NULL
#include <math.h>    // M_PI
#include <cstring>   // memcopy, memset
#include <sndfile.h> // to load audio files

//TODO introduce Out and InOut : In abstract classes, to be extended by AudioComponent and AudioGenerator

//----------------------------------------------------------------------------------
// Abstract base class
//----------------------------------------------------------------------------------
class AudioOutput {
public:
	virtual double getSample() = 0; // to retrieve double sample buffer
	virtual float *getBufferFloat(int numOfSamples) = 0;

	virtual ~AudioOutput() {};

protected:
	float *floatsamplebuffer; // float sample buffer, always same name...double sample buffer can be declared/implemented as you please instead
};


//----------------------------------------------------------------------------------
// Abstract class
//----------------------------------------------------------------------------------
// level interpolations [for smooth changes in vol] should be designed within the AudioGenerator that contains the AudioComponent
class AudioComponent : public AudioOutput {
public:
	AudioComponent();
	virtual void init(unsigned short period_size);
	virtual double getSample()=0;
	virtual double *getBuffer(int numOfSamples) = 0;
	virtual float *getBufferFloat(int numOfSamples);
	virtual double getLevel();
	virtual void setLevel(double level);

	virtual ~AudioComponent();

protected:
	unsigned short periodSize;
	double *samplebuffer;
	double level;

};

inline AudioComponent::AudioComponent() {
	periodSize = -1;
	samplebuffer = NULL;
	floatsamplebuffer = NULL;
	level = -1;
}

inline void AudioComponent::init(unsigned short period_size) {
	periodSize = period_size;
	samplebuffer = new double[periodSize];
	floatsamplebuffer = new float[periodSize];
	memset(samplebuffer, 0, sizeof(double)*periodSize);
	memset(floatsamplebuffer, 0, sizeof(float)*periodSize);
}

inline float *AudioComponent::getBufferFloat(int numOfSamples) {
	std::copy( getBuffer(numOfSamples), samplebuffer + numOfSamples, floatsamplebuffer);
	return floatsamplebuffer;
}

inline AudioComponent::~AudioComponent() {
	if(samplebuffer!=NULL)
		delete[] samplebuffer;

	if(floatsamplebuffer!=NULL)
		delete[] floatsamplebuffer;
}

inline void AudioComponent::setLevel(double level) {
	this->level = level;
}

inline double AudioComponent::getLevel() {
	return level;
}






enum advanceType {adv_oneShot_, adv_loop_, adv_backAndForth_};

//----------------------------------------------------------------------------------
// First level child class
//----------------------------------------------------------------------------------
class Waveform : public AudioComponent {
public:
	Waveform();
	Waveform(advanceType adv);
	void init(double level, double *samples, int len, unsigned short period_size);
	void reverse(bool toggle);
	void setAdvanceType(advanceType adv);
	void retrigger(unsigned long frameN=0, char dir=1);
	virtual double getSample();
	virtual double *getBuffer(int numOfSamples);
	double *getWaveform(int  *len);
	double *getWaveform();
	int getWaveformLen();
	~Waveform();
protected:
	double *waveFormBuffer; // contains the whole waveform, different from samplebuffer that contains only period
	advanceType advType;
	char direction; // 1 forward, -1 backwards. 0 NULL
	long unsigned currentSample;
	long unsigned frameNum;

	void setAdvanceSampleMethod();
	void (Waveform::*advanceSample)();
	void advanceSampleOneShot();
	void advanceSampleLoop();
	void advanceSampleBackAndForth();

	double *(Waveform::*getBufferMethod)(int numOfSamples);
	double *getBufferOneShot(int numOfSamples);
	double *getBufferLoop(int numOfSamples);
	double *getBufferBackAndForth(int numOfSamples);
};

inline void Waveform::setAdvanceType(advanceType adv) {
	advType = adv;
	setAdvanceSampleMethod();
}

inline void Waveform::retrigger(unsigned long frameN, char dir) {
	currentSample = frameN;
	direction     = dir;
}

inline Waveform::~Waveform(){
	if(waveFormBuffer!=NULL)
		delete[] waveFormBuffer;
}



//----------------------------------------------------------------------------------
// Second level child class
//----------------------------------------------------------------------------------
class AudioFile : public Waveform {
public:
	AudioFile();
	AudioFile(advanceType adv);
	int init(double level, std::string filename, unsigned short period_size, int rate, int takeRightCh=0);

private:
	int resample(double buff[], std::string filename, double *& interpBuff);
	int upsample(double buff[], int mul, double *& interpBuff);
	int decimate(double buff[], int fact, double *& interpBuff);
	void computeLowPass(double cuttFreq, double *a, double *b);
	SNDFILE *sndfile;
	SF_INFO sfinfo;
	int samplerate;
};






enum oscillator_type {osc_sin_, osc_square_, osc_tri_, osc_saw_, osc_whiteNoise_, osc_impTrain_, osc_const_, osc_w_};

//----------------------------------------------------------------------------------
// First level child class [sibling of Waveform]
//----------------------------------------------------------------------------------
// does not have concept of level interpolation, cos actual smooth changes in volume are supposed to be handled within a AudioGenerator
class Oscillator : public AudioComponent
{
public:
	void init(unsigned int rate, unsigned short period_size, oscillator_type type, double level=1, double freq=0, double phase=0, bool half=false);
	void setFrequency(double freq);
	void setDutyCycle(float dutyCyle);

	double getSample();
	double *getBuffer(int numOfSamples);

	double getFrequency();
	double getStartingLevel();
	double getStartingFreq();

private:

	unsigned int _rate;
	oscillator_type _type;
	double _startLevel;
	double _startFreq;
	double _frequency;
	double _phase;
	float _dutyCycle;
	double _half_shift;
	double _half_denom;

	double _step;

	static constexpr double max_phase = 2. * M_PI;

	double (Oscillator::*getSampleMethod)();
	double getSinSample();
	double getSquareSample();
	double getTriangularSample();
	double getSawToothSample();
	double getImpTrainSample();
	double getWhiteNoiseSample();
	double getConstSample();
	double getWSample();


	void increaseStep();

};

inline void Oscillator::setDutyCycle(float dutyCyle) {
	_dutyCycle = dutyCyle;
}

inline double Oscillator::getFrequency()
{
	return _frequency;
}

inline double Oscillator::getStartingLevel()
{
	return _startLevel;
}

inline double Oscillator::getStartingFreq()
{
	return _startFreq;
}



//----------------------------------------------------------------------------------
//TODO WaveTable, child of both Waveform and Oscillator
//----------------------------------------------------------------------------------



#endif /* AUDIOCOMPONENTS_H_ */

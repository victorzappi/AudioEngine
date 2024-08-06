/*
 * Waveforms.h
 *
 *  Created on: Apr 10, 2019
 *      Author: Victor Zappi
 */
#ifndef WAVEFORMS_H_
#define WAVEFORMS_H_

#include "AudioModules.h"

enum advanceType {adv_oneShot_, adv_loop_, adv_backAndForth_};

//----------------------------------------------------------------------------------
// Second level child classes, these can be instantiated, used on their own in the engine or as components of a Generator
//----------------------------------------------------------------------------------
class Waveform : public AudioModuleOut, public MultichannelOutUtils {
public:
	Waveform();
	Waveform(advanceType adv);
	virtual void init(double *samples, int len, unsigned int periodSize, double level=1, unsigned short outChannels=1, unsigned short outChnOffset=0);
	virtual void reverse();
	virtual void setAdvanceType(advanceType adv);
	virtual void stop();
	virtual void resume();
	void retrigger();
	virtual void retrigger(unsigned long frameN/*, char dir=1*/);
	virtual double getSample();
	//virtual double *getBuffer(int numOfSamples);
	double **getFrameBuffer(int numOfSamples);
	virtual double *getWaveform();
	virtual int getWaveform(double *&buff);
	virtual int getWaveformLen();
	virtual int getCurrentFramePos();
	~Waveform();
protected:
	double *waveFormBuffer; // contains the whole waveform, different from samplebuffer that contains only period
	advanceType advType;
	char direction; // 1 forward, -1 backwards. 0 NULL
	long unsigned currentFrame;
	long unsigned frameNum;
	long unsigned lastFrame;
	bool isPlaying;

	virtual void setAdvanceSampleMethod();
	void (Waveform::*advanceSample)();
	virtual void advanceSampleOneShot();
	virtual void advanceSampleLoop();
	virtual void advanceSampleBackAndForth();

	double *(Waveform::*getBufferMethod)(int numOfSamples);
	double *getBufferOneShot(int numOfSamples);
	double *getBufferLoop(int numOfSamples);
	double *getBufferBackAndForth(int numOfSamples);
};

inline void Waveform::setAdvanceType(advanceType adv) {
	advType = adv;
	setAdvanceSampleMethod();
}

inline void Waveform::stop() {
	isPlaying = false;
}

inline void Waveform::resume() {
	retrigger(currentFrame);
}


inline void Waveform::retrigger() {
	retrigger(0);
}

inline void Waveform::retrigger(unsigned long frameN/*, char dir*/) {
	currentFrame = frameN;
	//direction     = dir; //VIC not useful and over-complicated to handle in getBuffer...()
	isPlaying = true; // just in case
}

inline int Waveform::getCurrentFramePos() {
	return currentFrame;
}

inline double *Waveform::getWaveform() {
	return waveFormBuffer;
}

inline int Waveform::getWaveform(double *&buff) {
	buff = getWaveform();
	return frameNum;
}

inline int Waveform::getWaveformLen() {
	return frameNum; //return frameNum+1; // +1 for silent ending frame
}

inline Waveform::~Waveform(){
	if(waveFormBuffer!=NULL)
		delete[] waveFormBuffer;
}

inline void Waveform::advanceSampleOneShot() {
	if(currentFrame<frameNum)
		currentFrame++;
	// else do nothing for now...just stick to silent frame
}

inline void Waveform::advanceSampleLoop() {
	if(++currentFrame >= frameNum)
		currentFrame = 0; // restart
}

inline void Waveform::advanceSampleBackAndForth() {
	// going forward
	if(direction == 1) {
		if(++currentFrame >= frameNum) { // probably last sample played was frameNum-1
			currentFrame = frameNum-2;   // so let's start from frameNum-2
			direction = -1;			     // and go backwards
		}
	}
	// same for backwards
	else if(direction == -1) {
		if(--currentFrame == lastFrame) {
			currentFrame = 1;
			direction = 1;
		}
	}
}
//----------------------------------------------------------------------------------






//----------------------------------------------------------------------------------
// Third level child class
//----------------------------------------------------------------------------------
class AudioFile : public Waveform {
public:
	AudioFile();
	AudioFile(advanceType adv);
	int init(std::string filename, int rate, unsigned int periodSize, double level=1, int chnIndex=-1, unsigned short outChannels=1, unsigned short outChnOffset=0);

protected:
	void init(double *samples, int len, unsigned int periodSize, double level=1, unsigned short outChannels=1, unsigned short outChnOffset=0);
	int resample(double buff[], int numOfSamples, std::string filename, double *& interpBuff);
	int upsample(double buff[], int numOfSamples, int mul, double *& interpBuff);
	int decimate(double buff[], int numOfSamples, int fact, double *& interpBuff);
	void computeLowPass(double cuttFreq, double *a, double *b);
	SNDFILE *sndfile;
	SF_INFO sfinfo;
	int samplerate;
};

inline void AudioFile::init(double *samples, int len, unsigned int periodSize, double level, unsigned short outChannels, unsigned short outChnOffset) {
	Waveform::init(samples, len, periodSize, level, outChannels, outChnOffset);
}
//----------------------------------------------------------------------------------







enum interpType {interp_no_, interp_lin_, interp_cubic_};

//----------------------------------------------------------------------------------
// further offspring, child of AudioFile
//----------------------------------------------------------------------------------
class Wavetable : public AudioFile {
public:
	Wavetable();
	Wavetable(interpType interp);
	void init(double *samples, int len, unsigned int rate, unsigned int periodSize, double level=1, unsigned short outChannels=1, unsigned short outChnOffset=0);
	int init(std::string filename, unsigned int rate, unsigned int periodSize, double level=1, int chnIndex=-1, unsigned short outChannels=1, unsigned short outChnOffset=0);
	void setFrequency(double freq);
	double getFrequency();
	double getSample();
	double **getFrameBuffer(int numOfSamples);
	double *getWaveform();
	int getWaveform(double *&buff);
	int getCurrentFramePos();
	int getWaveformLen();

protected:
	interpType interpolation;
	double currentPos;
	double step;
	int waveFrameNum; // actual length of wave passed [as opposed to table's num of frames]

	void init(double *samples, int len, unsigned int periodSize, double level=1, unsigned short outChannels=1, unsigned short outChnOffset=0);

	//double *getBuffer(int numOfSamples);

	void setAdvanceSampleMethod();
	void advanceSampleLoop();

	double (Wavetable::*computeSample)();
	double computeSampleInterpNone();
	double computeSampleInterpLin();
	double computeSampleInterpCubic();
};

inline void Wavetable::setFrequency(double freq) {
	if(samplerate==-1) {
		printf("Cannot set frequency of Wavetable before defining its sample rate and length");
		return;
	}

	step = waveFrameNum*(freq/samplerate); // with waveFrameNum we get a more precise value than with frameNum [if we use guard frames for interpolation -> see protected init(...)]
	// a nice explanation can be found here:
	// https://en.wikibooks.org/wiki/Sound_Synthesis_Theory/Oscillators_and_Wavetables#Wavetables
}

inline double Wavetable::getFrequency() {
	if(samplerate==-1) {
		printf("Cannot get frequency of Wavetable before defining its sample rate and length");
		return -1;
	}

	return (step*samplerate)/waveFrameNum;
}

inline double *Wavetable::getWaveform() {
	if(interpolation == interp_cubic_)
		return &waveFormBuffer[1]; // skip first guard frame
	else
		return waveFormBuffer;
}

inline int Wavetable::getWaveform(double *&buff) {
	buff = getWaveform();
	return waveFrameNum;
}

inline int Wavetable::getCurrentFramePos() {
	return int(currentPos);
}

inline int Wavetable::getWaveformLen() {
	return waveFrameNum;
}

inline int Wavetable::init(std::string filename, unsigned int rate, unsigned int periodSize, double level, int chnIndex, unsigned short outChannels, unsigned short outChnOffset) {
	return AudioFile::init(filename, rate, periodSize, level, chnIndex, outChannels, outChnOffset);
}

inline void Wavetable::advanceSampleLoop() {
	currentPos+=step;
	if(currentPos >= frameNum)
		currentPos = currentPos-frameNum; // pac-man effect
}

inline double Wavetable::getSample() {
	double sample = (this->*computeSample) (); // inline
	(this->*advanceSample) (); // inline

	return sample*level*isPlaying;
}

inline double Wavetable::computeSampleInterpNone() {
	return waveFormBuffer[int(currentPos)];
}

inline double Wavetable::computeSampleInterpLin() {
	return ( waveFormBuffer[int(currentPos)] + waveFormBuffer[int(currentPos+0.5)] )/2.0;
}

// taken from the source code of awesome Pure Data!
// https://sourceforge.net/p/pure-data/pure-data/ci/master/tree/src/d_array.c#l593
inline double Wavetable::computeSampleInterpCubic() {
	double a =  waveFormBuffer[int(currentPos)-1];
	double b =  waveFormBuffer[int(currentPos)];
	double c =  waveFormBuffer[int(currentPos)+1];
	double d =  waveFormBuffer[int(currentPos)+2];

	double frac = currentPos-int(currentPos);

	double cminusb = c-b;

	return b + frac * ( cminusb - 0.1666667f * (1.-frac) * ( (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b) ) );
}
//----------------------------------------------------------------------------------







//----------------------------------------------------------------------------------
// WavetableOsc, child of Waveform
//----------------------------------------------------------------------------------

class WavetableOsc : public Wavetable {
public:
	WavetableOsc();
	WavetableOsc(interpType interp);
	void init(oscillator_type type, unsigned int rate, unsigned int periodSize, double level=1,
			  unsigned int numOfSamples=1024, unsigned short outChannels=1, unsigned short outChnOffset=0);

protected:
	void init(double *samples, int len, unsigned int rate, unsigned int periodSize, double level=1,
			  unsigned short outChannels=1, unsigned short outChnOffset=0); // shadows to remove
	int init(std::string filename, unsigned int rate, unsigned int periodSize, double level=1,
			 int chnIndex=-1, unsigned short outChannels=1, unsigned short outChnOffset=0); // shadows to remove
};












#endif /* INCLUDE_WAVEFORMS_H_ */

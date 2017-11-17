/*
 * WAveforms.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

#include "AudioComponents.h"

#include <algorithm> // reverse_copy


//----------------------------------------------------------------------------------------------------------------------------
// Waveform
//----------------------------------------------------------------------------------------------------------------------------
Waveform::Waveform() : AudioComponent() {
	advType 	  = adv_loop_;
	currentSample = -1;
	direction	  = 0;
	waveFormBuffer = NULL;
	frameNum      = -1;
	setAdvanceSampleMethod();
}

Waveform::Waveform(advanceType adv) : AudioComponent() {
	advType 	  = adv;
	currentSample = -1;
	direction	  = 0;
	waveFormBuffer = NULL;
	frameNum      = -1;
	setAdvanceSampleMethod();
}

void Waveform::init(double level, double *samples, int len, unsigned short period_size) {
	AudioComponent::init(period_size);

	frameNum       = len;
	waveFormBuffer = new double[frameNum+1]; // +1 for silent frame
	// copy samples
	memcpy(waveFormBuffer, samples, sizeof(double)*frameNum);
	waveFormBuffer[frameNum] = 0; // silent frame

	direction	  = 1; // go forward
	currentSample = 0;

	setLevel(level);
}

double Waveform::getSample() {
	double sample = waveFormBuffer[currentSample];
	(this->*advanceSample) (); // inline

	return sample*level;
}


double *Waveform::getBuffer(int numOfSamples) {
	double *buff = (this->*getBufferMethod) (numOfSamples);

	for(int n=0; n<numOfSamples; n++)
		buff[n] *= level;

	return buff;

}

double *Waveform::getWaveform(int  *len) {
	*len = frameNum+1; // +1 for silent ending frame
	return getWaveform();
}

double *Waveform::getWaveform() {
	return waveFormBuffer;
}

int Waveform::getWaveformLen() {
	return frameNum+1; // +1 for silent ending frame
}

//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------


void Waveform::setAdvanceSampleMethod() {
	switch(advType) {
		case adv_oneShot_:
			advanceSample   = &Waveform::advanceSampleOneShot;
			getBufferMethod = &Waveform::getBufferOneShot;
			break;
		case adv_loop_:
			advanceSample   = &Waveform::advanceSampleLoop;
			getBufferMethod = &Waveform::getBufferLoop;
			break;
		case adv_backAndForth_:
			advanceSample   = &Waveform::advanceSampleBackAndForth;
			getBufferMethod = &Waveform::getBufferBackAndForth;
			break;
		default:
			advanceSample   = &Waveform::advanceSampleLoop;
			getBufferMethod = &Waveform::getBufferLoop;
			break;
	}
}

inline void Waveform::advanceSampleOneShot() {
	if(currentSample<frameNum)
		currentSample++;
	// else do nothing for now...just stick to silent frame
}

inline void Waveform::advanceSampleLoop() {
	if(++currentSample>= frameNum)
		currentSample = 0; // restart
}

inline void Waveform::advanceSampleBackAndForth() {
	// going forward
	if(direction == 1) {
		if(++currentSample>= frameNum) { // probably last sample played was frameNum-1
			currentSample = frameNum-2;  // so let's start from frameNum-2
			direction     = -1;			 // and go backwards
		}
	}
	// same for backwards
	else if(direction == -1) {
		if(--currentSample<= 0) {
			currentSample = 1;
			direction     = 1;
		}
	}
}


double *Waveform::getBufferOneShot(int numOfSamples) {
	if(currentSample<frameNum) {
		int overflow = (currentSample+numOfSamples)-frameNum;
		if(overflow<=0) {
			memcpy(samplebuffer, waveFormBuffer+currentSample,  sizeof(double)*numOfSamples); // simply put in beginning of samplebuffer all the file samples that are in a row from current position
			currentSample += numOfSamples; // update
		} else { // otherwise we have to pad with zeros
			memcpy(samplebuffer, waveFormBuffer+currentSample, sizeof(double)*(frameNum-currentSample)); // put in beginning of samplebuffer all the file samples that are in a row
			memset(samplebuffer+frameNum-currentSample+1, 0, sizeof(double)*overflow); // then fill the rest of the sample buffer with zeros
			currentSample = frameNum; // update
		}
	}
	else
		memset(samplebuffer, 0, sizeof(double)*numOfSamples);
	return samplebuffer;
}
double *Waveform::getBufferLoop(int numOfSamples){
	int overflow = (currentSample+numOfSamples)-frameNum;
	// if we pick frames that are all in a row within the buffer
	if(overflow<=0) {
		memcpy(samplebuffer, waveFormBuffer+currentSample,  sizeof(double)*numOfSamples); // simply put at the beginning of samplebuffer all the file samples that are in a row from current position
		currentSample += numOfSamples; // update
	} else { // otherwise we have to start from beginning
		memcpy(samplebuffer, waveFormBuffer+currentSample, sizeof(double)*(frameNum-currentSample)); // put in beginning of samplebuffer all the file samples that are in a row
		memcpy(samplebuffer+frameNum-currentSample+1, waveFormBuffer, sizeof(double)*overflow); // then fill the rest of the sample buffer with the first file samples
		currentSample = overflow+1; // update
	}
	return samplebuffer;
}

double *Waveform::getBufferBackAndForth(int numOfSamples) {
	// forward
	if(direction == 1) {
		int overflow = (currentSample+numOfSamples)-frameNum;
		// if we pick frames that are all in a row within the buffer
		if(overflow<=0) {
			memcpy(samplebuffer, waveFormBuffer+currentSample,  sizeof(double)*numOfSamples); // simply put at the beginning of samplebuffer all the file samples that are in a row from current position
			currentSample += numOfSamples; // update
		} else { // otherwise we reach the end of the file and then go backwards
			memcpy(samplebuffer, waveFormBuffer+currentSample, sizeof(double)*(frameNum-currentSample)); // put at the beginning of samplebuffer all the file samples that are in a row
			std::reverse_copy(waveFormBuffer+frameNum-1-overflow, waveFormBuffer+frameNum-1, samplebuffer+frameNum-currentSample+1); // then fill the rest of the sample buffer with the reversed file samples [the last sample is skipped backwards]
			currentSample = frameNum-2-overflow; // update
			direction = -1;	// officially change direction
		}
	}
	else { // backwards
		int overflow = numOfSamples-currentSample;
		if(overflow<=0) { // if we pick frames that are all in a row within the buffer
			std::reverse_copy(waveFormBuffer+currentSample+1-numOfSamples, waveFormBuffer+currentSample+1, samplebuffer);// simply put at the beginning of samplebuffer all the file samples that are in a row from current position, in reverse order
			currentSample -= numOfSamples; // update
		} else { // otherwise we go backwards
			std::reverse_copy(waveFormBuffer, waveFormBuffer+currentSample+1, samplebuffer); // put at the beginning of samplebuffer all the reversed file samples that are in a row
			memcpy(samplebuffer+numOfSamples-overflow, waveFormBuffer+1,  sizeof(double)*overflow); // then fill the rest of the sample buffer with the first file samples [the first sample is skipped forward]
			currentSample = overflow+1; // update
			direction = 1; // officially change direction
		}
	}

	return samplebuffer;
}
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------













//----------------------------------------------------------------------------------------------------------------------------
// AudioFile
//----------------------------------------------------------------------------------------------------------------------------

AudioFile::AudioFile() : Waveform() {
	samplerate = -1;
	sndfile    = NULL;
	memset(&sfinfo, 0, sizeof(sfinfo)); // this to avoid a sporadic error on file load. Thanks to snappizz in https://github.com/supercollider/supercollider/issues/2474
}

AudioFile::AudioFile(advanceType adv) : Waveform(adv){
	samplerate = -1;
	sndfile    = NULL;
	memset(&sfinfo, 0, sizeof(sfinfo));  // this to avoid a sporadic error on file load. Thanks to snappizz in https://github.com/supercollider/supercollider/issues/2474
}

int AudioFile::init(double level, std::string filename, unsigned short period_size, int rate, int takeRightCh){
	if( !(sndfile = sf_open((const char*)(filename.c_str()), SFM_READ, &sfinfo)) ) {
		sf_perror(sndfile);
		printf("Audiofile %s can't be loaded.../:\n", filename.c_str());
		return -1;
	}

	//TODO: support stereo
	if(sfinfo.channels !=1) {
		printf("Audiofile %s not mono\n", filename.c_str());
		return 1;
	}

	//double fileBuff[sfinfo.frames];
	double *fileBuff = new double[sfinfo.frames];

	int subformat = sfinfo.format & SF_FORMAT_SUBMASK;
	int readcount = sf_read_double(sndfile, fileBuff, sfinfo.frames);

	// Pad with zeros in case we couldn't read whole file
	memset(fileBuff+readcount, 0, sfinfo.frames-readcount);

	if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE) {
		double	scale ;

		sf_command (sndfile, SFC_CALC_SIGNAL_MAX, &scale, sizeof (scale)) ;
		if (scale < 1e-10)
			scale = 1.0 ;
		else
			scale = 32700.0 / scale ;
		printf("Scale = %f\n", scale);

		for (int f = 0; f < sfinfo.frames; f++)
			fileBuff[f] *= scale;
	}
	sf_close(sndfile);

	samplerate = rate;
	if(samplerate == sfinfo.samplerate)
		Waveform::init(level, fileBuff, sfinfo.frames, period_size); // simply use file as waveform
	else {
		double *interpBuff = NULL;
		int numOfFrames = resample(fileBuff, filename, interpBuff); // interpolated file buffer allocated and filled inside of here
		// if something went wrong with resampling
		if(numOfFrames <= 0) {
			if(interpBuff==NULL)
				delete[] interpBuff;
			return numOfFrames;
		}

		Waveform::init(level, interpBuff, numOfFrames, period_size); // now we can use the file as waveform
		delete[] interpBuff; // clear
	}

	delete[] fileBuff;

	return 0;
}

int AudioFile::resample(double fileBuff[], std::string filename, double *& interpBuff) {
	int numOfFrames = 0;

	if(samplerate > sfinfo.samplerate) {
		if(samplerate % sfinfo.samplerate==0) {
			int mult = samplerate/sfinfo.samplerate;
			numOfFrames = upsample(fileBuff, mult, interpBuff);
		} else
			numOfFrames = -1;
	}
	else if(samplerate < sfinfo.samplerate) {
		if(sfinfo.samplerate % samplerate==0) {
			int fact = sfinfo.samplerate/samplerate;
			numOfFrames = decimate(fileBuff, fact, interpBuff);
		} else
			numOfFrames = -2;
	}
	else {
		//TODO: more resampling of file:
		// direct downsample
		// up/downsample when file not direct integer multiple or submultiple

		numOfFrames = 0;
	}


	if(numOfFrames <= 0)
		printf("Audiofile %s's sample rate %d can't be converted to audio rate %d\n", filename.c_str(), sfinfo.samplerate, samplerate);

	return numOfFrames;
}

int AudioFile::upsample(double fileBuff[], int mul, double *& interpBuff) {
	int numOfFrames = sfinfo.frames*mul;

	interpBuff = new double[numOfFrames+1]; // +1 for silent frame and frameNum has been properly sized in resample()
	memset(interpBuff, 0, sizeof(double)*(numOfFrames+1)); // all zero, including last silent frame

	// copy file samples, properly spaced...basically we do a zero padding
	for(int i=0; i<sfinfo.frames; i++)
		interpBuff[i*mul] = fileBuff[i];

	// now we filter with low pass to prevent aliasing
	double a[3] = {1, 0, 0};
	double b[3] = {0, 0, 0};
	double normCutFreq = 0.5; // frequency = half of file sample rate [samples can't contain freqs higher than that]
	computeLowPass(normCutFreq, a, b);

	// prepare previous i/o
	double x0 = 0;
	double x1 = 0;
	double x2 = 0;
	double y1 = 0;
	double y2 = 0;

	// filter
	for(int i=0; i<numOfFrames; i++) {
		x0 = interpBuff[i];
		interpBuff[i] = b[0]*x0 + b[1]*x1 + b[2]*x2 - a[1]*y1 - a[2]*y2;

		// prepare previous i/o
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = interpBuff[i];
	}

	return numOfFrames;
}

int AudioFile::decimate(double fileBuff[], int fact, double *& interpBuff) {
	int numOfFrames = 1 + sfinfo.frames/fact;

	interpBuff = new double[numOfFrames+1]; // +1 for silent frame and frameNum has been properly sized in resample()
	memset(interpBuff, 0, sizeof(double)*(numOfFrames+1)); // all zero, including last silent frame

	// we filter original file with low pass to prevent aliasing
	double a[3] = {1, 0, 0};
	double b[3] = {0, 0, 0};
	double cutFreq = 0.5; // frequency = half of the sample rate [samples can't contain freqs higher than that]
	computeLowPass(cutFreq, a, b);

	// prepare previous i/o
	double x0 = 0;
	double x1 = 0;
	double x2 = 0;
	double y1 = 0;
	double y2 = 0;

	// filter
	for(long int i=0; i<sfinfo.frames; i++) {
		x0 = fileBuff[i];
		fileBuff[i] = b[0]*x0 + b[1]*x1 + b[2]*x2 - a[1]*y1 - a[2]*y2;

		// prepare previous i/o
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = fileBuff[i];
	}

	// then we downsample
	// copying file samples, properly spaced
	for(int i=0; i<numOfFrames; i++)
		interpBuff[i] = fileBuff[i*fact];

	return numOfFrames;

	//TODO test this! done definitely on a rush...need a card that does 22050 or a file that is 88200 Hz
}

void AudioFile::computeLowPass(double normCutFreq, double *a, double *b) {
	// second order low pass butterworth filter
	// taken from Bela IIR filter example
	double normCutOmega = 2*M_PI*normCutFreq; // normalized cut angular velocity
	double denom = 4+2*sqrt(2.0)*normCutOmega+normCutOmega*normCutOmega;
	b[0] = normCutOmega*normCutOmega/denom;
	b[1] = 2*b[0];
	b[2] = b[0];
	a[1] = (2*normCutOmega*normCutOmega-8)/denom;
	a[2] = (normCutOmega*normCutOmega+4-2*sqrt(2)*normCutOmega)/denom;
}
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


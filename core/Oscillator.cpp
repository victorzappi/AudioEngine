/*
 * Oscillator.cpp
 *
 *  Created on: Oct 15, 2013
 *      Author: Victor Zappi
 */



#include <stdlib.h>

#include "Oscillator.h"


void Oscillator::init(oscillator_type type, unsigned int rate, unsigned int periodSize, double level, double freq,
					  double phase, bool half, unsigned short outChannels, unsigned short outChnOffset) {// no freq and phase needed for noise
	AudioModuleOut::init(periodSize, outChannels, outChnOffset);

	_rate		 = rate;
	_type  		 = type;
	_startLevel  = level;
	_startFreq   = freq;
	_phase       = phase; // starting phase
	_dutyCycle   = 0.5; // default duty cycle for square

	if(half) {
		_half_shift = 1;
		_half_denom = 2;
	}
	else {
		_half_shift = 0;
		_half_denom = 1;
	}

	setFrequency(freq); //set frequency and update step

	switch(_type)
	{
		case osc_sin_:
			getSampleMethod = &Oscillator::getSinSample;
			break;
		case osc_square_:
			getSampleMethod = &Oscillator::getSquareSample;
			break;
		case osc_tri_:
			getSampleMethod = &Oscillator::getTriangularSample;
			break;
		case osc_saw_:
			getSampleMethod = &Oscillator::getSawToothSample;
			break;
		case osc_whiteNoise_:
			getSampleMethod = &Oscillator::getWhiteNoiseSample;
			break;
		case osc_impTrain_:
			getSampleMethod = &Oscillator::getImpTrainSample;
			break;
		case osc_const_:
			getSampleMethod = &Oscillator::getConstSample;
			break;
		//case osc_w_:
		//	getSampleMethod = &Oscillator::getWSample;
		//	break;
		default:
			getSampleMethod = &Oscillator::getSinSample;
			printf("Oscillator type %d not recognized! Falling back toi sin\n", _type);
			break;
	}

	setLevel(level);
}

double **Oscillator::getFrameBuffer(int numOfSamples) {
	for(int n=0; n<numOfSamples; n++)
		framebuffer[out_chn_offset][n] =(this->*getSampleMethod) (); // methods referred to by getSample() are all inline

	memset(framebuffer[out_chn_offset]+numOfSamples, 0, (period_size-numOfSamples)*sizeof(double)); // reset part of buffer that has been potentially left untouched


	MultichannelOutUtils::cloneFrameChannels(numOfSamples);

	return framebuffer;
}


void Oscillator::setFrequency(double freq) {
	_frequency	= freq;
	_step = max_phase*_frequency/(double)_rate; // update step each time frequency is modified
}




//----------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------

//--------------------------------
// samplebuffer retrieval
//--------------------------------
/*
double *Oscillator::getSinBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getSinSample();

	return samplebuffer;
}

double *Oscillator::getSquareBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getSquareSample();

	return samplebuffer;
}

double *Oscillator::getTriangularBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getTriangularSample();

	return samplebuffer;
}

double *Oscillator::getSawToothBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getSawToothSample();

	return samplebuffer;
}

double *Oscillator::getImpTrainBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getImpTrainSample();

	return samplebuffer;
}

double *Oscillator::getWhiteNoiseBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getWhiteNoiseSample();

	return samplebuffer;
}

double *Oscillator::getConstBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] = getConstSample();

	return samplebuffer;
}


*/

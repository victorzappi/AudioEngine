/*
 * Oscillator.cpp
 *
 *  Created on: Oct 15, 2013
 *      Author: Victor Zappi
 */



#include <stdlib.h>

#include "AudioComponents.h"


void Oscillator::init(unsigned int rate, unsigned short period_size, oscillator_type type, double level,double freq, double phase, bool half) // no freq and phase needed for noise
{
	AudioComponent::init(period_size);

	_rate		 = rate;
	_type  		 = type;
	_startLevel  = level;
	_startFreq   = freq;
	_phase       = phase; // starting phase
	periodSize = period_size;
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
		case osc_w_:
			getSampleMethod = &Oscillator::getWSample;
			break;
		default:
			break;
	}

	setLevel(level);
}


double Oscillator::getSample() {
	return level*(this->*getSampleMethod) ();
}

double *Oscillator::getBuffer(int numOfSamples)
{
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] =(this->*getSampleMethod) (); // methods referred to by getSample() are all inline

	return samplebuffer;
}

void Oscillator::setFrequency(double freq)
{
	_frequency	= freq;
	_step = max_phase*_frequency/(double)_rate; // update step each time frequency is modified
}


//----------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------

//--------------------------------
// sample retrieval ---> every oscillator type returns values in [-1, +1]
//--------------------------------
inline double Oscillator::getSinSample()
{
	double retval = sin(_phase);

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getSquareSample()
{
	double retval;

	if(_phase <= max_phase*_dutyCycle )
		retval = 1;
	else
		retval = -1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getTriangularSample()
{
	double retval;

	// first half goes from -1 to 1
	if(_phase <= max_phase/2 )
		retval = (2*_phase/max_phase)*2 - 1; // ( _phase/(max_phase/2) ) *2 - 1  => from [0, 1] to [-1, 1]
	else // second half goes from 1 to -1
		retval = -( ((_phase-max_phase/2)*2/max_phase)*2 - 1 ); // -( ( (_phase-max_phase/2)/(max_phase/2) ) *2 - 1 ) => from [0, 1] to [1, -1]

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getSawToothSample()
{
	double retval;

	retval = (1 - (_phase/max_phase))*2 - 1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}


inline double Oscillator::getWhiteNoiseSample()
{
	double retval;

	retval = (double) rand()/ ((double)RAND_MAX);

	retval = (retval*2)-1;

	retval = (retval+_half_shift)/_half_denom;

	return  retval*level;
}

inline double Oscillator::getImpTrainSample()
{
	double retval;

	if( _phase-_step < 0)
		retval = 1;
	else retval = -1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getConstSample()
{
	increaseStep();

	return 1.0*level;
}

inline double Oscillator::getWSample()
{
	double retval;

	// sariff w shape!
	if(_phase <= max_phase/2 )
	{
		if(_phase <= max_phase/4 )
			retval = _phase/(max_phase/4);
		else
			retval = -( 1 - ( ( _phase-(max_phase/4) )/(max_phase/4) ) );
	}
	else
	{
		if(_phase <= 3*max_phase/4 )
			retval =  -( ( _phase-(2*max_phase/4) )/(max_phase/4) );
		else
			retval = 1 - ( ( _phase-(3*max_phase/4) )/(max_phase/4) );
	}

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}


inline void Oscillator::increaseStep()
{
	_phase += _step;
	if (_phase >= max_phase)
		_phase -= max_phase;
}


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

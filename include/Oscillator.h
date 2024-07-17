/*
 * Oscilaltor.h
 *
 *  Created on: Apr 10, 2019
 *      Author: Victor Zappi
 */
#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "AudioModules.h"

//----------------------------------------------------------------------------------
// Second level child class [sibling of Waveform]
//----------------------------------------------------------------------------------
// does not have concept of level interpolation, cos actual smooth changes in volume are supposed to be handled within an AudioGenerator
class Oscillator : public AudioModuleOut, public MultichannelOutUtils {
public:
	Oscillator();
	void init(oscillator_type type, unsigned int rate, unsigned int periodSize, double level=1, double freq=0,
			  double phase=0, bool half=false, unsigned short outChannels=1, unsigned short outChnOffset=0);
	void setFrequency(double freq);
	void setDutyCycle(float dutyCyle);

	double getSample();
	//double *getBuffer(int numOfSamples);

	double **getFrameBuffer(int numOfSamples);

	void retrigger();

	double getFrequency();

	//VIC needed?
	double getStartingLevel();
	double getStartingFreq();

	void setPhase(double ph);
	double getPhase();

	inline ~Oscillator() {};

protected:
	unsigned int _rate;
	oscillator_type _type;
	double _startLevel;//VIC needed?
	double _startFreq;//VIC needed?
	double _frequency;//VIC needed?
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

	//virtual void fillFrameChannels(int numOfSamples);


	void increaseStep();

};

inline Oscillator::Oscillator() : MultichannelOutUtils(this) {
	_rate = 0;
	_type = osc_const_;
	_startLevel = 0;//VIC needed?
	_startFreq = 0;//VIC needed?
	_frequency = 0;//VIC needed?
	_phase = -1;
	_dutyCycle = -1;
	_half_shift = -1;
	_half_denom = -1;
	_step = 0;
	getSampleMethod = NULL;
}

inline double Oscillator::getSample() {
	return level*(this->*getSampleMethod) ();
}

inline void Oscillator::retrigger() {
	_phase = 0;
}

inline void Oscillator::setDutyCycle(float dutyCyle) {
	_dutyCycle = dutyCyle;
}

inline double Oscillator::getFrequency() {
	return _frequency;
}

inline double Oscillator::getStartingLevel() {
	return _startLevel;
}

inline double Oscillator::getStartingFreq() {
	return _startFreq;
}

inline void Oscillator::setPhase(double ph) {
	_phase = ph;
}

inline double Oscillator::getPhase() {
	return _phase;
}

//----------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------

//--------------------------------
// sample retrieval ---> every oscillator type returns values in [-1, +1]
//--------------------------------
inline double Oscillator::getSinSample() {
	double retval = sin(_phase);

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getSquareSample() {
	double retval;

	if(_phase <= max_phase*_dutyCycle )
		retval = 1;
	else
		retval = -1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getTriangularSample() {
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

inline double Oscillator::getSawToothSample() {
	double retval;

	retval = (1 - (_phase/max_phase))*2 - 1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}


inline double Oscillator::getWhiteNoiseSample() {
	double retval;

	retval = (double) rand()/ ((double)RAND_MAX);

	retval = (retval*2)-1;

	retval = (retval+_half_shift)/_half_denom;

	return  retval*level;
}

inline double Oscillator::getImpTrainSample() {
	double retval;

	if( _phase-_step < 0)
		retval = 1;
	else retval = -1;

	retval = (retval+_half_shift)/_half_denom;

	increaseStep();

	return retval*level;
}

inline double Oscillator::getConstSample() {
	increaseStep();

	return 1.0*level;
}

/*inline double Oscillator::getWSample() {
	double retval;

	// w shape!
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
}*/


inline void Oscillator::increaseStep() {
	_phase += _step;
	if (_phase >= max_phase)
		_phase -= max_phase;
}


#endif /* INCLUDE_OSCILALTOR_H_ */

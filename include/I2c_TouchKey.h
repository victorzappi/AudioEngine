/*
 * I2c.h
 *
 *  Created on: Oct 14, 2013
 *      Author: Victor Zappi
 */

#ifndef I2CTK_H_
#define I2CTK_H_

#include "I2c.h"

#define NUM_BYTES 9

class I2c_TouchKey : public I2c
{
private:
	// read NUM_BYTES bytes, which have to be properly parsed
	char dataBuffer[NUM_BYTES];
	int bytesRead;

	int rawSliderPosition[3];
	int rawSliderPositionH;

	int touchCount;
	float sliderSize[3];
	float sliderPosition[3];
	float sliderPositionH;



public:
	int initTouchKey();
	int readI2C();
	int getTouchCount();
	float * getSlidersize();
	float * getSliderPosition();
	float getSliderPositionH();


	I2c_TouchKey();
	~I2c_TouchKey();

};

inline int I2c_TouchKey::getTouchCount()
{
	return touchCount;
}

inline float * I2c_TouchKey::getSlidersize()
{
	return sliderSize;
}

inline float * I2c_TouchKey::getSliderPosition()
{
	return sliderPosition;
}

inline float I2c_TouchKey::getSliderPositionH()
{
	return sliderPositionH;
}





#endif /* I2CTK_H_ */

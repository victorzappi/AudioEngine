/*
 * I2c_TouchKey.cpp
 *
 *  Created on: Oct 14, 2013
 *      Author: Victor Zappi
 */



#include "I2c_TouchKey.h"

I2c_TouchKey::I2c_TouchKey()
{
	touchCount = 0;
	sliderSize[0] = sliderSize[1] = sliderSize[2] = -1;
	sliderPosition[0] = sliderPosition[1] = sliderPosition[2] = -1;
	sliderPositionH = -1;
	rawSliderPositionH = -1;
	bytesRead = -1;
}

int I2c_TouchKey::initTouchKey()
{
	char buf[3] = { 0x00, 0x01, 0x00 }; // code for centroid mode
	if(write(i2C_file, buf, 3) !=3)
	{
		cout << "Failed to set TouchKey in \"Centroid Mode\" " << endl;
		return 1;
	}

	usleep(5000); // need to give TouchKey enough time to process command


	buf[0] = 0x06; // code for data collection
	if(write(i2C_file, buf, 1) !=1)
	{
		cout << "Failed to prepare data collection " << endl;
		return 2;
	}

	usleep(5000); // need to give TouchKey enough time to process command


	return 0;
}


int I2c_TouchKey::readI2C()
{
	bytesRead = read(i2C_file, dataBuffer, NUM_BYTES);
	if (bytesRead != NUM_BYTES)
	{
		cout << "Failure to read Byte Stream" << endl;
		return 2;
	}
	/*cout << NUM_BYTES << " bytes read" << endl;
	for(int j=0; j<9; j++)
		cout << "\t" << (int)dataBuffer[j];
	cout << endl;
	*/

	touchCount = 0;

	rawSliderPosition[0] = (((dataBuffer[0] & 0xF0) << 4) + dataBuffer[1]);
	rawSliderPosition[1] = (((dataBuffer[0] & 0x0F) << 8) + dataBuffer[2]);
	rawSliderPosition[2] = (((dataBuffer[3] & 0xF0) << 4) + dataBuffer[4]);

	rawSliderPositionH   = (((dataBuffer[3] & 0x0F) << 8) + dataBuffer[5]);



	for(int i = 0; i < 3; i++)
	{
		if(rawSliderPosition[i] != 0x0FFF) // 0x0FFF means no touch
		{
			sliderPosition[i] = (float)rawSliderPosition[i] / 1536.0;	// Black keys, vertical (128 * 12);
			sliderSize[i]     = (float)dataBuffer[i + 6] / 255.0;
			touchCount++;
		}
		else {
			sliderPosition[i] = -1.0;
			sliderSize[i]     = 0.0;
		}
	}

	if(rawSliderPositionH != 0x0FFF)
	{
		sliderPositionH = (float)rawSliderPositionH / 256.0;			// White keys, horizontal (1 byte + 1 bit)
	}
	else
		sliderPositionH = -1.0;

	/*
	if(touchCount>0)
	{
		cout << "---------------------------------------------------------------------------------------------" << endl;
		for(int i = 0; i < touchCount; i++)
		{
			cout << "SliderPos[" << i << "]: " << sliderPosition[i] << "\t\t-----\t\t";
			cout << "sliderSize[" << i << "]: " << sliderSize[i] << endl;
		}
		cout << endl;
		cout << "SliderPosH: " << sliderPositionH << endl;
	}
	//else
	//	cout << endl;
	*/

	//usleep(5000);

	return 0;
}


I2c_TouchKey::~I2c_TouchKey()
{}


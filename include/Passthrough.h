/*
 * Passtrough.h
 *
 *  Created on: Apr 10, 2019
 *      Author: Victor Zappi
 */

#ifndef PASSTHROUGH_H_
#define PASSTHROUGH_H_

#include "AudioModules.h"


//VIC first modify all OpenGL stuff to work with multichannel engine
// create getFrameBuffer with fixed params [mono]
// make container synths multichannel with fixed params [mono]
// switch to monoEngine in all examples

//TODO turn this file in AudioFilters
// will contain abstract class AudioFilter, special child of AudioInOut
// and its inplace version [framebuffer is not allocated, it simply modifies passed input]
// should apply to Biquad, it works in place already
// and TextureFilter too, which should copy its output [from getSample()] into areaInputBuffer, even if these buffers may be longer because of rate_mul...wa can ignore the elements in excess (:
// Passthrough does not work inplace instead!


// simple passthrough of a chosen number of consecutive input channels, routed to an equal number of consecutive output channels
class Passthrough : public AudioModuleInOut {
public:
	void init(unsigned int periodSize, unsigned short chns, unsigned short inChnOffset=0, unsigned short outChnOffset=0);
	double **getFrameBuffer(int numOfSamples, double **input);
	inline void retrigger(){};

protected:
	unsigned short channels;
};


inline void Passthrough::init(unsigned int periodSize, unsigned short chns, unsigned short inChnOffset, unsigned short outChnOffset) {
	AudioModuleInOut::init(periodSize, chns, inChnOffset, chns, outChnOffset);
	channels = chns;
}

inline double **Passthrough::getFrameBuffer(int numOfSamples, double **input) {
	for(int i=0; i<channels; i++)
		memcpy(framebuffer[i+out_chn_offset], input[i+in_chn_offset], sizeof(double)*numOfSamples);

	return framebuffer;
}



// pass through of a single input channel, which can be duplicated in any number of consecutive output channels though
class MultiPassthrough : public AudioModuleInOut, public MultichannelOutUtils {
public:
	MultiPassthrough();
	void init(unsigned int periodSize, unsigned short inChannel, unsigned short outChannels=1, unsigned short outChnOffset=0);
	double **getFrameBuffer(int numOfSamples, double **input);
	inline void retrigger(){};
};

inline MultiPassthrough::MultiPassthrough() : MultichannelOutUtils(this) {
}

inline void MultiPassthrough::init(unsigned int periodSize, unsigned short inChannel, unsigned short outChannels, unsigned short outChnOffset) {
	AudioModuleInOut::init(periodSize, 1, inChannel, outChannels, outChnOffset);
}

inline double **MultiPassthrough::getFrameBuffer(int numOfSamples, double **input) {
	memcpy(framebuffer[out_chn_offset], input[in_chn_offset], sizeof(double)*numOfSamples);

	memset(framebuffer[out_chn_offset]+numOfSamples, 0, (period_size-numOfSamples)*sizeof(double)); // reset part of buffer that has been potentially left untouched


	MultichannelOutUtils::cloneFrameChannels(numOfSamples);

	return framebuffer;
}





#endif /* INCLUDE_PASSTROUGH_H_ */

/*
 * MonoEngine_int32LE.h
 *
 *  Created on: Apr 30, 2019
 *      Author: Victor Zappi
 */

#ifndef INCLUDE_MONOENGINE_INT32LE_H_
#define INCLUDE_MONOENGINE_INT32LE_H_

class AudioEngine;

#include "AudioEngine.h"

// a simple AudioEngine child class that has same output on all out buffers and works with fixed format = int 32 Little Endian
// this is the most likely case, cos audio cards almost often support integers [float/double far are less common]
// and CPUs are very likely to be Little Endian and to represent integers with 32 bits
class MonoEngine_int32LE : public AudioEngine {
public:
	MonoEngine_int32LE();

protected:
	// simplified int 32 LW versions of base ones
	void fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples);
	void fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples);

	// simplified mono version of base one
	void readAudioModulesBuffers(int numOfSamples, double **framebufferOut, double **framebufferIn);

	// these foo methods hide base ones, cos user is not allowed to access them [format is fixed]
	inline void setPlaybackAudioFormat(snd_pcm_format_t fmt) { (void)fmt; };
	inline void setCaptureAudioFormat(snd_pcm_format_t fmt) { (void)fmt; };

};


// we receive/send Little Endian 32 bit integers from/to audio card, using a CPU that represents integers with 32 bits Little Endian -> format match
// this is the most likely case, cos audio cards almost often support integers [float/double far are less common]
// and CPUs are very likely to be Little Endian and to represent integers with 32 bits
inline void MonoEngine_int32LE::fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples) {
	(void)offset; // to mute warnings
	int *insamples = (int *)capture.rawSamples;
	for(int n = 0; n < numSamples; n++) {
		for(unsigned int chn = 0; chn < capture.channels; chn++) {
			capture.frameBuffer[chn][n] = insamples[n*capture.channels + chn]/double(capture.maxVal);
		}
	}
}
/*inline void MonoEngine_int32LE::fromRawToFloat_int(snd_pcm_uframes_t offset, int numOfSamples) {

	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res = (*this.*byteCombine)(chn, sampleBytes);

			capture.frameBuffer[chn][n] = res/float(capture.maxVal);
			sampleBytes[chn] += capture.byteStep;
		}
	}
}*/



inline void MonoEngine_int32LE::fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples) {
	(void)offset; // to mute warnings
	int *outsamples = (int *)playback.rawSamples;
	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		for(int n = 0; n < numSamples; n++) {
			outsamples[n*playback.channels + chn] = playback.frameBuffer[chn][n]*double(playback.maxVal);
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}

#endif /* INCLUDE_MONOENGINE_INT32LE_H_ */

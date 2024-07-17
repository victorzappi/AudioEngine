/*
 * MonoEngine_int32LE.cpp
 *
 *  Created on: Apr 30, 2019
 *      Author: Victor Zappi
 */


#include "MonoEngine_int32LE.h"

MonoEngine_int32LE::MonoEngine_int32LE() : AudioEngine() {
	// fixed formats
	playback.format = SND_PCM_FORMAT_S32;
	capture.format  = SND_PCM_FORMAT_S32;
}


//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------

// simply copies a single buffer in all output buffers
void MonoEngine_int32LE::readAudioModulesBuffers(int numOfSamples, double **framebufferOut, double **framebufferIn) {
	// read all buffers

	// first buffers from output audio modules
	for(int i=0; i<numOfAudioModulesOut; i++)
		moduleFramebuffer[i] = audioModulesOut[i]->getFrameBuffer(numOfSamples);

	// then buffers from input/output ones
	// even if not in full duplex mode...
	double **inBuff;
	if(isFullDuplex)
		inBuff = framebufferIn;
	else
		inBuff = silenceBuff; //...by setting silent input buffers
	for(int i=0; i<numOfAudioModulesInOut; i++)
		moduleFramebuffer[numOfAudioModulesOut+i] = audioModulesInOut[i]->getFrameBuffer(numOfSamples, inBuff); // we simply pass pointer to all input channels, no matter the number of in channels of the module

	// sum altogether in first output buffer
	//TODO use SMID to sum samples from AudioModules
	for(int i=0; i<numOfAudioModules; i++) {
		if(audioModulesChnOffset[i]==0) {
			for(int n=0; n<numOfSamples; n++)
				framebufferOut[0][n] += moduleFramebuffer[i][0][n];
		}
	}

	// copy first buffer in all other output buffers -> same mono output across all channels
	for(int ch=1; ch<playback.channels; ch++) {
		memcpy(framebufferOut[ch], framebufferOut[0], sizeof(double)*numOfSamples);
	}
}



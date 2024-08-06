/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * AudioEngine.cpp
 *
 *  Created on: Oct 14, 2013
 *      Author: Victor Zappi
 *
 *  This class has been created starting from the example file /test/pcm.c
 *  Published by
 *  Jaroslav Kysela [perex@perex.cz]
 *  Abramo Bagnara [abramo@alsa-project.org],
 *  Takashi Iwai [tiwai@suse.de]
 *  Frank van de Pol [fvdpol@coil.demon.nl]
 *  at
 *  https://www.alsa-project.org/alsa-doc/alsa-lib/index.html
 *  under the GPL license
 *
 */


#include "AudioEngine.h"

#include <iostream>

using namespace std;

//TODO maybe use mutexes to add and remove AudioGenerators

AudioEngine::AudioEngine() {
	isFullDuplex = false; // to enable/disable capture

	// settings shared between playback and capture
	rate        = 44100; // audio rate, most likely value
	period_size = 0;	 // ring buffer length in frames, if not set by user will be set to the maximum value supported by card
	buffer_size = 0;	 // period length in frames, if not set by user will be set to double the period
	ringbuffer_ratio = 0;
	transfer_methods[method].name			= "write";
	transfer_methods[method].access		   	= SND_PCM_ACCESS_RW_INTERLEAVED;
	transfer_methods[method].transfer_loop 	= NULL; // this will be done in initEngine()
	/*{
		{ "write", 					SND_PCM_ACCESS_RW_INTERLEAVED, 		audioLoop },
		{ "write_and_poll", 		SND_PCM_ACCESS_RW_INTERLEAVED, 		write_and_poll_loop },
		{ "async", 					SND_PCM_ACCESS_RW_INTERLEAVED, 		async_loop },
		{ "async_direct", 			SND_PCM_ACCESS_MMAP_INTERLEAVED, 	async_direct_loop },
		{ "direct_interleaved", 	SND_PCM_ACCESS_MMAP_INTERLEAVED, 	direct_loop },
		{ "direct_noninterleaved", 	SND_PCM_ACCESS_MMAP_NONINTERLEAVED, direct_loop },
		{ "direct_write", 			SND_PCM_ACCESS_MMAP_INTERLEAVED, 	direct_audioLoop },
		{ NULL, 					SND_PCM_ACCESS_RW_INTERLEAVED, 		NULL }
	};*/
	method = 0;     // alsa low level buffer transfer method
	resample    = true;  // enable alsa-lib resampling
	periodEvent = false; // produce poll event after each period [???]


	//---------------------------------------------------------------
	// specific settings
	//---------------------------------------------------------------
	// playback data structures
	playback.handle   = NULL;      // device handle
	playback.rawSamples  = NULL;   // period buffer for interleaved/non-interleaved raw samples [in bytes, smallest and most generic type]
	playback.rawSamplesStartAddr = NULL; // address of first raw sample of each channel
	playback.frameBuffer = NULL; // multidimensional period buffers for float samples, one per each channel
	playback.areas    = NULL;      // to easily deal with interleaved/non-interleaved samples
	playback.hwparams = NULL;      // hardware parameter settings
	playback.swparams = NULL;      // software parameter settings

	// playback params
	strcpy(playback.device, "hw:0,0"); // device, e.g., hw:0,0. we don't wanna deal with this, better to use cardName
	playback.cardName = "";	    // audio card name, e.g., HDA Intel PCH
	playback.format   = SND_PCM_FORMAT_S16; // sample format
	playback.channels = 0;	    // count of channels...if not set by user, it will be automatically set to the minimum num of channels supported by card
    playback.isBlocking = true;  // is write command blocking?

	// playback low level settings, derived from format
    playback.byteStep    = 0;	   // distance between consecutive samples in buffer of frames [char *samples]
    playback.formatBits  = 0;	   // number of bits per sample
    playback.maxVal      = -1;     // maximum value that can be represented
    playback.bps         = 0;  	   // bytes per sample
    playback.physBps     = 0;	   // bytes per physical representation of sample [???]
    playback.isBigEndian = false;  // is big endian? checked automatically
    playback.isUnsigned  = false; // is unsigned?
	playback.isFloat     = false;  // is float?

	//---------------------------------------------------------------
	// capture data structures
	capture.handle   = NULL;  // device handle
	capture.rawSamples  = NULL;  // period buffer for interleaved/non-interleaved raw samples [in bytes, smallest and most generic type]
	capture.rawSamplesStartAddr = NULL; // address of first sample of each channel
	capture.frameBuffer = NULL;  // multidimensional period buffers, one per each channel
	capture.areas    = NULL;  // to easily deal with interleaved/non-interleaved frames
	capture.hwparams = NULL;  // hardware parameter settings
	capture.swparams = NULL;  // software parameter settings

	// capture params
	strcpy(capture.device, "hw:0,0"); // device, e.g., hw:0,0. we don't wanna deal with this, better to use cardName
	capture.cardName = "";	    // audio card name, e.g., HDA Intel PCH
	capture.format   = SND_PCM_FORMAT_S16; // sample format
	capture.channels = 0;	     // count of channels...if not set by user, it will be automatically set to the minimum num of channels supported by card
    capture.isBlocking = true;  // is read command blocking?

	// capture low level settings, derived from format
    capture.byteStep    = 0;	  // distance between consecutive samples in buffer of frames [char *samples]
    capture.formatBits  = 0;	  // number of bits per sample
    capture.maxVal      = -1;     // maximum value that can be represented
    capture.bps         = 0;  	  // bytes per sample
    capture.physBps     = 0;	  // bytes per physical representation of sample [???]
    capture.isBigEndian = false;  // is big endian? checked automatically
    capture.isUnsigned   = false; // is unsigned?
	capture.isFloat     = false;  // is float?
	//---------------------------------------------------------------


	// utils
	//volume = ref_volume = 1; 	 // starts as maximum
	//inc_volume 			= 0; 	 // no need to interpolate at the beginning [but MUST interpolate to modify volume]
	//delta_volume        = 0; 	 // no need to interpolate at the beginning [but MUST interpolate to modify volume]
	verbose             = false; // verbose flag


	// shared data structures
	numOfAudioModulesOut = 0;          // number of outs and in/outs objects
	for(int i=0; i<MAX_NUM_OF_AUDIOMODULES_OUT; i++) {
		audioModulesOut[i] = NULL;
		audioModulesChannels[i] = 0;
		audioModulesChnOffset[i] = 0;
		moduleFramebuffer[i] = NULL;
	}
	numOfAudioModulesInOut = 0;          // number of outs and in/outs objects
	for(int i=0; i<MAX_NUM_OF_AUDIOMODULES_INOUT; i++) {
		audioModulesInOut[i] = NULL;
		audioModulesChannels[MAX_NUM_OF_AUDIOMODULES_OUT+i] = 0;
		audioModulesChnOffset[MAX_NUM_OF_AUDIOMODULES_OUT+i] = 0;
		moduleFramebuffer[MAX_NUM_OF_AUDIOMODULES_OUT+i] = NULL;
	}

	numOfAudioModules = 0;

	//audioSample = 0;		   // to read from gneerators a single sample at a time...lame but whatever

	silenceBuff = NULL;


	// global status
	engineReady         = false; // engine has been initialized?
	engineIsRunning     = false; // engine is running?


	// function pointers
	writeAudio = NULL;
	readAudio  = NULL;
	audioLoop  = NULL;
	byteSplit   = NULL;
	byteCombine = NULL;
	fromFloatToRaw = NULL;
	fromRawToFloat  = NULL;
}

AudioEngine::~AudioEngine() {
	// to avoid double free
	if(engineReady) {
		// if running...
		if(engineIsRunning)
			engineIsRunning = false; // ...use automatic shutdown
		else
			shutEngine(); 			// ...otherwise shut manually
	}
}

int AudioEngine::initEngine() {
	snd_output_t *output;

	int err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		printf("Output failed: %s\n", snd_strerror(err));
		return 11;
	}

	printf("\n==========================================\nAlsa Audio Engine\n==========================================\n");
	printf("Audio rate: %iHz\n", rate);
	printf("Using transfer method: %s\n", transfer_methods[method].name);



	snd_pcm_hw_params_alloca(&playback.hwparams);
	snd_pcm_sw_params_alloca(&playback.swparams);

	printf("\nPlayback settings:\n");

	int retval;
	if(playback.cardName!="")
		retval = findDevice(playback.cardName, playback.device); // simplest way is to set card name and look for device 0 of that card
	else
		retval = printCardName(playback.device); // otherwise we check the name of the card to which the passed or default device belongs
	printf("\n");

	if(retval!=0)
		return retval;

	if ((err = snd_pcm_open(&playback.handle, playback.device, SND_PCM_STREAM_PLAYBACK, playback.isBlocking ? 0 : SND_PCM_NONBLOCK)) < 0) {
		printf("Device open error: %s\n", snd_strerror(err));
		return 0;
	}

	if ((err = setHwParams(playback)) < 0) {
		printf("Setting of hwparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	ringbuffer_ratio = buffer_size/period_size;
	if ((err = setSwParams(playback)) < 0) {
		printf("Setting of swparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if (verbose > 0) {
		snd_pcm_dump(playback.handle, output);
		if(!isFullDuplex)
		    fflush(stdout);
	}



	// do same for capture device if full duplex engine
	if(isFullDuplex) {
		snd_pcm_hw_params_alloca(&capture.hwparams);
		snd_pcm_sw_params_alloca(&capture.swparams);

		printf("\n\nCapture settings:\n");

		if(capture.cardName!="")
			retval = findDevice(capture.cardName, capture.device); // simplest way is to set card name and look for device 0 of that card
		else
			retval = printCardName(capture.device); // otherwise we check the name of the card to which the passed or default device belongs
		printf("\n");

		if(retval!=0)
			return retval;

		if ((err = snd_pcm_open(&capture.handle, capture.device, SND_PCM_STREAM_CAPTURE, capture.isBlocking ? 0 : SND_PCM_NONBLOCK)) < 0) {
			printf("Device open error: %s\n", snd_strerror(err));
			return 0;
		}

		if ((err = setHwParams(capture)) < 0) {
			printf("Setting of hwparams failed: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}
		if ((err = setSwParams(capture)) < 0) {
			printf("Setting of swparams failed: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}

		if (verbose > 0) {
			snd_pcm_dump(capture.handle, output);
		    fflush(stdout);
		}
	}


	setLowLevelParams(playback);
	// do same for capture device if full duplex engine
	if(isFullDuplex) {
		setLowLevelParams(capture);
		// plus compute the mask necessary to the two's complement of received raw samples
		// in other words, we build a mask to extend the sign whenever the format uses fewer bits than the integer container
		capture.mask = 0x00000000;
		for(int i=capture.formatBits; i<sizeof(int)*8; i++)
			capture.mask |= (1 << i); // put a 1 ub all bits that are beyond those used by the format
	}



	// function pointers
	if(playback.isBlocking)
		writeAudio = &AudioEngine::writeAudio_block;
	else
		writeAudio = &AudioEngine::writeAudio_nonBlock;

	if(capture.isBlocking)
		readAudio = &AudioEngine::readAudio_block;
	else
		readAudio = &AudioEngine::readAudio_nonBlock;

	if(isFullDuplex)
		audioLoop = &AudioEngine::audioLoop_readWrite;
	else
		audioLoop = &AudioEngine::audioLoop_write;

	transfer_methods[method].transfer_loop = AudioEngine::audioLoop; // we'd need a & if audioLoop() was a regular method and not a function pointer

	if(playback.isBigEndian)
		byteSplit = &AudioEngine::byteSplit_bigEndian;
	else
		byteSplit = &AudioEngine::byteSplit_littleEndian;

	if(!playback.isFloat && !playback.isUnsigned)
		fromFloatToRaw = &AudioEngine::fromFloatToRaw_int;
	else if(!playback.isFloat && playback.isUnsigned)
		fromFloatToRaw = &AudioEngine::fromFloatToRaw_uint;
	else if(playback.isFloat && !playback.isUnsigned)
		fromFloatToRaw = &AudioEngine::fromFloatToRaw_float32;
	//else if(playback.isFloat && playback.isUnsigned)
	//	fromFloatToRaw = &AudioEngine::fromFloatToRaw_ufloat;
	// no type like that exists!

	if(capture.isBigEndian)
		byteCombine = &AudioEngine::byteCombine_bigEndian;
	else
		byteCombine = &AudioEngine::byteCombine_littleEndian;

	if(!capture.isFloat && !capture.isUnsigned)
		fromRawToFloat = &AudioEngine::fromRawToFloat_int;
	else if(!capture.isFloat && capture.isUnsigned)
		fromRawToFloat = &AudioEngine::fromRawToFloat_uint;
	else if(capture.isFloat && !capture.isUnsigned)
		fromRawToFloat = &AudioEngine::fromRawToFloat_float32;
	//else if(capture.isFloat && capture.isUnsigned)
	//	fromRawToFloat = &AudioEngine::fromRawToFloat_ufloat;
	// no type like that exists!


	// last touch
	if(preparePcm()<0)
		exit(EXIT_FAILURE);

	if(!isFullDuplex)
		capture.channels = 40; // an arbitrary big number, to provide enough silence buffers to any AudioModuleInOut
	silenceBuff = new double *[capture.channels];
	for(unsigned int i=0; i<capture.channels; i++) {
		silenceBuff[i] = new double[period_size];
		memset(silenceBuff[i], 0, sizeof(double)*period_size);
	}

	engineReady = true;

	return 0;
}

int AudioEngine::startEngine() {
	if(verbose==1)
		printf("Starting AudioEngine\n");
	engineIsRunning = true;

	initRender();

	int err = (this->*(transfer_methods[method].transfer_loop))();


	if (err < 0) {
		printf("Transfer failed: %s\n", snd_strerror(err));
		return 1;
	}

	cleanUpRender();

	shutEngine();

	return 0;
}


int AudioEngine::stopEngine() {
	engineIsRunning = false;
	return 0;
}

void AudioEngine::addAudioModule(AudioModule *mod) {
	if ( dynamic_cast<AudioModuleInOut*>(mod) != NULL) {
		if(numOfAudioModulesInOut >= MAX_NUM_OF_AUDIOMODULES_INOUT) {
			printf("Warning! Cannot add module! \nEngine holds the maximum number of AudioModuleInOut objects already (%d)\n", MAX_NUM_OF_AUDIOMODULES_INOUT);
			return;
		}

		audioModulesInOut[numOfAudioModulesInOut] = (AudioModuleInOut *)mod;

		// add channels info of newly added module
		audioModulesChannels[numOfAudioModules] = ((AudioModuleInOut*)mod)->getOutChannnelsNum();
		audioModulesChnOffset[numOfAudioModules] = ((AudioModuleInOut*)mod)->getOutChannnelOffset();
		numOfAudioModulesInOut++;
	}
	else if ( dynamic_cast<AudioModuleOut*>(mod) != NULL) {
		if(numOfAudioModulesOut >= MAX_NUM_OF_AUDIOMODULES_OUT) {
			printf("Warning! Cannot add module! \nEngine holds the maximum number of AudioModuleOut objects already (%d)\n", MAX_NUM_OF_AUDIOMODULES_OUT);
			return;
		}

		audioModulesOut[numOfAudioModulesOut] = (AudioModuleOut *)mod;

		// make space for channels info of the newly added module, by shifting all the values of AudioModuleInOut objects
		for(int i=numOfAudioModulesOut; i<numOfAudioModules; i++) {
			audioModulesChannels[i+1] = audioModulesChannels[i];
			audioModulesChnOffset[i+1] = audioModulesChnOffset[i];
		}
		audioModulesChannels[numOfAudioModulesOut] = ((AudioModuleOut*)mod)->getOutChannnelsNum(); // add channel num
		audioModulesChnOffset[numOfAudioModulesOut] = ((AudioModuleOut*)mod)->getOutChannnelOffset(); // add channel offset
		numOfAudioModulesOut++;
	}

	numOfAudioModules=numOfAudioModulesOut+numOfAudioModulesInOut;
}


/*inline void AudioEngine::addAudioOutput(AudioOutput *a) {
	audioOut[numOfAudioModulesOut++] = a;
}
inline void AudioEngine::addAudioInputOutput(AudioInputOutput *a) {
	if(!isFullDuplex) {
		printf("Be careful! Cannot add AudioInputOutput objects if engine is not full duplex!\n");
		return;
	}
	audioInOut[numOfAudioModulesInOut++] = a;
}*/

//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------

// adapted from https://gist.github.com/dontknowmyname/4536535
int AudioEngine::findDevice(string cardName, char *device) {
	// search for card [from card name] and then build device id

	bool cardFound = false;
	int cardNum = -1;
	snd_ctl_t *cardhandle;

	while(!cardFound) {
		int err = snd_card_next(&cardNum);
		if (err < 0) {
			fprintf(stderr, "\tCan't get the next card number: %s\n", snd_strerror(err));
			break;
		}

		if (cardNum < 0)
			break; // no more cards

		char str[64];
		sprintf(str, "hw:%i", cardNum);
		if ((err = snd_ctl_open(&cardhandle, str, 0)) < 0)	//Now cardhandle becomes your sound card.
		{
			printf("\tCan't open card %i: %s\n", cardNum, snd_strerror(err));
			continue;
		}

		snd_ctl_card_info_t *cardInfo;	// use to hold card information
		// we need to get a snd_ctl_card_info_t, just allocate it on the stack
		snd_ctl_card_info_alloca(&cardInfo);
		// tell ALSA to fill in our snd_ctl_card_info_t with info about this card
		if ((err = snd_ctl_card_info(cardhandle, cardInfo)) < 0) {
			printf("\tCan't get info for card %i: %s\n", cardNum, snd_strerror(err));
			break;
		}

		// check if this card is the one we're looking for
		std::string name(snd_ctl_card_info_get_name(cardInfo));
		size_t found = name.find(cardName);

		// if different
		if(found==std::string::npos)
			continue;

		// here we build device id starting from card num
		sprintf(device, "hw:%d,0", cardNum);

		printf("\tDevice is %s\n", device);
		printf("\t%s\n", snd_ctl_card_info_get_name(cardInfo));
		cardFound = true;
	}


	if(!cardFound)
		return 1; // gne'


	return 0;
}
// adapted from https://gist.github.com/dontknowmyname/4536535
int AudioEngine::printCardName(char *device) {
	// get card name from device id

	snd_ctl_t *cardhandle;
	string deviceStr = string(device); // includes card, device and possibly subdevice
	string card; // we want no device name nor sub-device

	// let's scrap them!
	string::size_type pos = deviceStr.find(','); // comma should always separate card name from rest
	 if (pos != string::npos)
		card =  deviceStr.substr(0, pos);
	 else {
		 printf("\tAudio device name is probably wrong...\n");
		 return -1;
	 }

	int err = snd_ctl_open(&cardhandle, card.c_str(), 0);
	if (err < 0) {
		printf("\tCan't open device ]: - %s\n", snd_strerror(err));
		return err;
	}

	snd_ctl_card_info_t *cardInfo;
	// we need to get a snd_ctl_card_info_t, just allocate it on the stack
	snd_ctl_card_info_alloca(&cardInfo);
	// tell ALSA to fill in our snd_ctl_card_info_t with info about this card
	if ((err = snd_ctl_card_info(cardhandle, cardInfo)) < 0) {
		printf("\tCan't get info for device -  %s\n", snd_strerror(err));
		return err;
	}

	// close the card's control interface after we're done with it
	snd_ctl_close(cardhandle);

	// ALSA allocates some mem to load its config file when we call some of the above functions
	// now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
	snd_config_update_free_global();


	printf("\tDevice is %s\n", device);
	printf("\t%s\n", snd_ctl_card_info_get_name(cardInfo));

	return 0;
}

int AudioEngine::setHwParams(audioStructure &audio) {
	// choose all parameters
	int err = snd_pcm_hw_params_any(audio.handle, audio.hwparams);
	if (err < 0) {
		printf("Broken configuration: no configurations available: %s\n", snd_strerror(err));
		return err;
	}


	// set the stream rate
	unsigned int rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(audio.handle, audio.hwparams, &rrate, 0);
	if (err < 0) {
		printf("Rate %iHz not available: %s\n", rate, snd_strerror(err));
		return err;
	}
	if (rrate != rate) {
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		return -EINVAL;
	}

	int dir;
	snd_pcm_uframes_t frames;
	snd_pcm_hw_params_get_period_size_min(audio.hwparams, &frames, &dir);
	printf("\tMin Period size: %d frames\n", (int)frames);
	snd_pcm_hw_params_get_period_size_max(audio.hwparams, &frames, &dir);
	printf("\tMax Period size: %d frames\n", (int)frames);
	// if zero (undefined), set to max by default
	if(period_size == 0)
		period_size = frames;

	snd_pcm_hw_params_get_buffer_size_min(audio.hwparams, &frames);
	printf("\tMin Buffer size: %d frames\n", (int)frames);
	snd_pcm_hw_params_get_buffer_size_max(audio.hwparams, &frames);
	printf("\tMax Buffer size: %d frames\n", (int)frames);
	// if zero (undefined). set to double period by default
	if(buffer_size == 0)
		buffer_size = 2*period_size;

	// set the period size
	err = snd_pcm_hw_params_set_period_size(audio.handle, audio.hwparams, period_size, dir);
	if (err < 0) {
		printf("\tUnable to set period size %i: %s\n", (int)period_size, snd_strerror(err));
		return err;
	}
	// check it
	snd_pcm_uframes_t size;
	err = snd_pcm_hw_params_get_period_size(audio.hwparams, &size, &dir);
	if (err < 0) {
		printf("\tUnable to get period size: %s\n", snd_strerror(err));
		return err;
	}
	period_size = size;
	printf("\t----------Period size: %d frames\n", (int)period_size);

	// set the buffer size
	err = snd_pcm_hw_params_set_buffer_size	(audio.handle, audio.hwparams, buffer_size);
	if (err < 0) {
		printf("\tUnable to set buffer size %i: %s\n", (int)buffer_size, snd_strerror(err));
		return err;
	}
	// check it
	err = snd_pcm_hw_params_get_buffer_size(audio.hwparams, &size);
	if (err < 0) {
		printf("\tUnable to get buffer size: %s\n", snd_strerror(err));
		return err;
	}
	buffer_size = size;
	printf("\t----------Buffer size: %d frames\n", (int)buffer_size);


	// set the read/write access method
	err = snd_pcm_hw_params_set_access(audio.handle, audio.hwparams, transfer_methods[method].access);
	if (err < 0) {
		printf("\t\nAccess type %s not supported: %s\n", snd_pcm_access_name(transfer_methods[method].access), snd_strerror(err));
		/*if(transfer_methods[method].access == SND_PCM_ACCESS_MMAP_NONINTERLEAVED || transfer_methods[method].access == SND_PCM_ACCESS_RW_NONINTERLEAVED)
			printf("\t->Try to switch to an interleaved access type.");
		else if(transfer_methods[method].access == SND_PCM_ACCESS_MMAP_INTERLEAVED || transfer_methods[method].access == SND_PCM_ACCESS_RW_INTERLEAVED)
			printf("\t->Try to switch to a non-interleaved access type.");*/
		return err;
	}
	if(verbose)
		printf("\n\tAccess type: %s\n", snd_pcm_access_name(transfer_methods[method].access));

	// set hardware resampling
	err = snd_pcm_hw_params_set_rate_resample(audio.handle, audio.hwparams, resample);
	if (err < 0) {
		printf("\tResampling setup failed: %s\n", snd_strerror(err));
		return err;
	}


	printf("\n");
	// set the sample format
	err = snd_pcm_hw_params_set_format(audio.handle, audio.hwparams, audio.format);
	if (err < 0) {
		printf("\tFormat %s not supported: %s\n", snd_pcm_format_name(audio.format), snd_strerror(err));
		printf("\tSearching for a supported one...\n");

		for(int i=SND_PCM_FORMAT_S32; i<SND_PCM_FORMAT_FLOAT64; i+=2) {
			audio.format = (snd_pcm_format_t)i;
			err = snd_pcm_hw_params_set_format(audio.handle, audio.hwparams, audio.format);
			if(err==0)
				break;
		}

		if(err<0) {
			printf("\t->No formats supported among the standard ones ):\n");
			return err;
		}
		else
			printf("\t->Found supported format!\n");
	}
	printf("\tFormat: %s\n", snd_pcm_format_name(audio.format));


	unsigned int ch = 0;
	snd_pcm_hw_params_get_channels_min(audio.hwparams, &ch);
	printf("\n\tMin num of channels: %d\n", (int)ch);
	// if zero (undefined), set to min by default
	if(audio.channels == 0)
		audio.channels = ch;
	snd_pcm_hw_params_get_channels_max(audio.hwparams, &ch);
	printf("\tMax num of channels: %d\n", (int)ch);

	// set the count of playback channels
	err = snd_pcm_hw_params_set_channels(audio.handle, audio.hwparams, audio.channels);
	if (err < 0) {
		printf("\tChannels count (%i) not available: %s\n", audio.channels, snd_strerror(err));
		return err;
	}
	printf("\t----------Num of channels: %d\n", audio.channels);


	// write the parameters to device
	err = snd_pcm_hw_params(audio.handle, audio.hwparams);
	if (err < 0) {
		printf("\tUnable to set hw params: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}



int AudioEngine::setSwParams(audioStructure audio) {
	// get the current swparams
	int err = snd_pcm_sw_params_current(audio.handle, audio.swparams);
	if (err < 0) {
		printf("Unable to determine current swparams: %s\n", snd_strerror(err));
		return err;
	}

	// start the transfer when the buffer is almost full:
	// (buffer_size / avail_min) * avail_min
	err = snd_pcm_sw_params_set_start_threshold(audio.handle, audio.swparams,  isFullDuplex ? 0x7fffffff : ( (buffer_size / period_size) * period_size )); //VIC
	if (err < 0) {
		printf("Unable to set start threshold mode: %s\n", snd_strerror(err));
		return err;
	}
	// allow the transfer when at least period_size samples can be processed
	// or disable this mechanism when period event is enabled (aka interrupt like style processing)
	err = snd_pcm_sw_params_set_avail_min(audio.handle, audio.swparams, periodEvent ? buffer_size : period_size);
	if (err < 0) {
		printf("Unable to set avail min: %s\n", snd_strerror(err));
		return err;
	}
	// enable period events when requested
	if (periodEvent) {
		err = snd_pcm_sw_params_set_period_event(audio.handle, audio.swparams, 1);
		if (err < 0) {
			printf("Unable to set period event: %s\n", snd_strerror(err));
			return err;
		}
	}

	// write the parameters to the playback device
	err = snd_pcm_sw_params(audio.handle, audio.swparams);
	if (err < 0) {
		printf("Unable to set sw params: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}

// needs reference cos we allocate members of the structure within the method
int AudioEngine::setLowLevelParams(audioStructure &audio) {
	// allocate buffers
	audio.rawSamples = (char *) malloc((period_size * audio.channels * snd_pcm_format_physical_width(audio.format)) / 8);
	if (audio.rawSamples == NULL) {
		printf("No enough memory\n");
		exit(EXIT_FAILURE);
	}

	audio.frameBuffer = new double*[audio.channels];

	audio.areas = (snd_pcm_channel_area_t *) calloc(audio.channels, sizeof(snd_pcm_channel_area_t));
	if (audio.areas == NULL) {
		printf("No enough memory\n");
		delete[] audio.frameBuffer;
		exit(EXIT_FAILURE);
	}

	for (unsigned int chn = 0; chn < audio.channels; chn++) {
		audio.frameBuffer[chn] = new double[period_size];
		if(audio.frameBuffer[chn] == NULL) {
			printf("No enough memory\n");
			exit(EXIT_FAILURE);
		}
		bzero(audio.frameBuffer[chn], period_size * sizeof(double));

		audio.areas[chn].addr  = audio.rawSamples;
		audio.areas[chn].first = chn * snd_pcm_format_physical_width(audio.format);
		audio.areas[chn].step  = audio.channels * snd_pcm_format_physical_width(audio.format);
	}

	audio.rawSamplesStartAddr = new unsigned char*[audio.channels];
	for (unsigned int chn = 0; chn < audio.channels; chn++) {
		if ((audio.areas[chn].first % 8) != 0) {
			printf("audio.areas[%i].first == %i, aborting...\n", chn, audio.areas[chn].first);
			exit(EXIT_FAILURE);
		}
		if ((audio.areas[chn].step % 16) != 0) {
			printf("audio.areas[%i].step == %i, aborting...\n", chn, audio.areas[chn].step);
			exit(EXIT_FAILURE);
		}

		audio.rawSamplesStartAddr[chn] = (((unsigned char *)audio.areas[chn].addr) + (audio.areas[chn].first / 8));
	}

	// set low level params
	audio.byteStep = audio.areas[0].step / 8;
	audio.formatBits = snd_pcm_format_width(audio.format);
	audio.maxVal = (1 << (audio.formatBits - 1)) - 1;
	audio.bps = audio.formatBits / 8;  /* bytes per sample */
	audio.physBps = snd_pcm_format_physical_width(audio.format) / 8;
	audio.isBigEndian = snd_pcm_format_big_endian(audio.format) == 1;
	audio.isUnsigned = snd_pcm_format_unsigned(audio.format) == 1;
	audio.isFloat = (audio.format == SND_PCM_FORMAT_FLOAT_LE || audio.format == SND_PCM_FORMAT_FLOAT_BE);


	return 0;
}

int AudioEngine::shutEngine() {
	if(playback.areas != NULL)
		free(playback.areas);
	if(capture.areas != NULL)
		free(capture.areas);

	for(unsigned int i=0; i<playback.channels; i++) {
		if(playback.frameBuffer[i] != NULL)
			delete[] playback.frameBuffer[i];
	}
	if(playback.frameBuffer != NULL)
		delete[] playback.frameBuffer;
	if(playback.rawSamples != NULL)
		free(playback.rawSamples);
	if(playback.rawSamplesStartAddr != NULL)
		delete[] playback.rawSamplesStartAddr;
	if(playback.handle != NULL)
		snd_pcm_close(playback.handle);

	if(capture.frameBuffer != NULL) {
		for(unsigned int i=0; i<capture.channels; i++) {
			if(capture.frameBuffer[i] != NULL)
				delete[] capture.frameBuffer[i];
		}
		delete[] capture.frameBuffer;
	}
	if(capture.rawSamples != NULL)
		free(capture.rawSamples);
	if(capture.rawSamplesStartAddr != NULL)
		delete[] capture.rawSamplesStartAddr;
	if(capture.handle != NULL)
		snd_pcm_close(capture.handle);

	if(silenceBuff!=NULL) {
		for(unsigned int i=0; i<capture.channels; i++)
			delete[] silenceBuff[i];
		delete[] silenceBuff;
	}

	engineReady = false;

	printf("AudioEngine stopped\n");
	return 0;
}

// underrun and suspend recovery
int AudioEngine::underrunRecovery(int err) {
	printf("underrun!\n");
	if (verbose)
		printf("stream recovery\n");
	if (err == -EPIPE) {    // under-run
		//err = snd_pcm_prepare(playback.handle);

		err = preparePcm(true);
		if (err < 0)
			printf("Can't recovery from underrun, prepare failed\n");

		//err = 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(playback.handle)) == -EAGAIN)
			sleep(1);       /* wait until the suspend flag is released */
		if (err < 0) {
			//err = snd_pcm_prepare(playback.handle);

			err = preparePcm(true);
			if (err < 0)
				printf("Can't recovery from underrun, prepare failed\n");

			//err = 0;
		}
	}
	return err;
}
// overrun and suspend recovery [same as underrun, but different print]
int AudioEngine::overrunRecovery(int err) {
	printf("overrun!\n");
	if (verbose)
		printf("stream recovery\n");
	if (err == -EPIPE) {    // over-run
		err = preparePcm(true);
		if (err < 0)
			printf("Can't recovery from overrun, prepare failed\n");

		//err=0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(capture.handle)) == -EAGAIN)
			sleep(1);       /* wait until the suspend flag is released */
		if (err < 0) {
			err = preparePcm(true);
			if (err < 0)
				printf("Can't recovery from suspend, prepare failed\n");

			//err=0;
		}
	}
	return err;
}

int AudioEngine::writeAudio_block(long numOfSamples) {
	long written;
	char *samples = playback.rawSamples;

	do {
		written = snd_pcm_writei(playback.handle, samples, numOfSamples);

		if(written > 0) {
			samples += written * playback.channels * playback.physBps;
			numOfSamples -= written;
		}
		else if(written < 0) {
			if (underrunRecovery(written) < 0) {
				printf("Write error: %s\n", snd_strerror(written));
				exit(EXIT_FAILURE);
			}
			//reset = true;
			break;  // skip one period
		}
	} while (written >= 1 && numOfSamples > 0);

	return numOfSamples;
}
int AudioEngine::writeAudio_nonBlock(long numOfSamples) {
	long written;
	char *samples = playback.rawSamples;
	do {
		written = snd_pcm_writei(playback.handle, samples, numOfSamples);

		if(written > 0) {
			samples += written * playback.channels * playback.physBps;
			numOfSamples -= written;
		}
		else if (written<0) {
			if(written == -EAGAIN)
				continue; // try again [can happen cos non blocking!]
			if (underrunRecovery(written) < 0){
				printf("Write error: %s\n", snd_strerror(written));
				exit(EXIT_FAILURE);
			}
			break;  // skip one period
		}
	} while (numOfSamples>0);

	return numOfSamples;
}

long AudioEngine::readAudio_block(long numOfSamples) {
	long read;
	char *samples = capture.rawSamples;

	do {
		read = snd_pcm_readi(capture.handle, samples, numOfSamples);
		if(read > 0) {
			samples += read * capture.channels * capture.physBps;
			numOfSamples -= read;
		}
		else if(read < 0) {
			if(overrunRecovery(read) < 0) {
				printf("Read error: %s\n", snd_strerror(read));
				exit(EXIT_FAILURE);
			}
			break;  // skip one period
		}
	} while (read >= 1 && numOfSamples > 0);

	return read;
}
long AudioEngine::readAudio_nonBlock(long numOfSamples) {
	long read;
	char *samples = capture.rawSamples;
	do {
		read = snd_pcm_readi(capture.handle, samples, numOfSamples);
		if(read > 0) {
			samples += read * capture.channels * capture.physBps;
			numOfSamples -= read;
		}
		else if (read<0) {
			if(read == -EAGAIN)
				continue; // try again [can happen cos non blocking!]
			if (overrunRecovery(read) < 0){
				printf("Write error: %s\n", snd_strerror(read));
				exit(EXIT_FAILURE);
			}
			break;  // skip one period
		}
	} while (numOfSamples > 0);
	return read;
}



// Transfer method - write only //TODO re-introduce the other methods, possibly maintaining the same write/read audio methods
int AudioEngine::audioLoop_write() {
	while(engineIsRunning) {

		// call the render function, which is conveniently detached in AudioEngine_render.cpp -> playback samples maybe synthesized
		render((float)rate, period_size, playback.channels, playback.frameBuffer, 0/*capture.channels*/, capture.frameBuffer/*silence*/);

		// from float samples to playback raw samples
		(*this.*fromFloatToRaw)(0, period_size);
		(*this.*writeAudio)(period_size);
	}
	return 0;
}

int AudioEngine::audioLoop_readWrite() {
	long numOfSamples;
	while(engineIsRunning) {

		/*if(false) {
			//printf("RESET\n");
			reset = false;

			int err = 0;


			if ((err = preparePcm(true)) < 0)
				exit(0);


			snd_pcm_drop(capture.handle);
			snd_pcm_drop(playback.handle);

			snd_pcm_prepare(playback.handle);

			if ((err = snd_pcm_unlink(capture.handle)) < 0) {
				printf("Streams unlink error: %s\n", snd_strerror(err));
				exit(0);
			}
			if ((err = snd_pcm_link(capture.handle, playback.handle)) < 0) {
				printf("Streams link error: %s\n", snd_strerror(err));
				exit(0);
			}
			if (snd_pcm_format_set_silence(capture.format, capture.rawSamples, period_size*capture.channels) < 0) {
				fprintf(stderr, "silence error\n");
				exit(0);
			}
			if ((*this.*writeAudio)(buffer_size) < 0) {
				fprintf(stderr, "write error\n");
				exit(0);
			}
			if ((err = snd_pcm_start(capture.handle)) < 0) {
				printf("Go error: %s\n", snd_strerror(err));
				exit(0);
			}
		}*/

		if ((numOfSamples = (*this.*readAudio)(period_size)) >= 0) {
			// from raw capture samples to more intelligible float samples (;
			(*this.*fromRawToFloat)(0, numOfSamples);

			// call the render function, which is conveniently detached in AudioEngine_render.cpp -> capture may be processed and playback samples maybe synthesized, both possibly combined
			render((float)rate, period_size, playback.channels, playback.frameBuffer, capture.channels, capture.frameBuffer);

			// from float samples to playback raw samples
			(*this.*fromFloatToRaw)(0, numOfSamples);
			(*this.*writeAudio)(period_size);
		}
	}
	return 0;
}




// we can't simply send our computed samples to card! we must take into account:
// the number of bytes that is contained in every card sample (format)
// the architecture we are running on... if big endian the computed samples have bytes in reverese order!
void AudioEngine::fromFloatToRaw_int(snd_pcm_uframes_t offset, int numOfSamples) {
	unsigned char *sampleBytes[playback.channels];

	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		sampleBytes[chn] = playback.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * playback.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res = playback.maxVal * playback.frameBuffer[chn][n];

			(*this.*byteSplit)(chn, sampleBytes, res);

			sampleBytes[chn] += playback.byteStep;
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}
void AudioEngine::fromFloatToRaw_uint(snd_pcm_uframes_t offset, int numOfSamples) {
	unsigned char *sampleBytes[playback.channels];

	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		sampleBytes[chn] = playback.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * playback.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res = playback.maxVal * playback.frameBuffer[chn][n];
			res ^= 1U << (playback.formatBits - 1);

			(*this.*byteSplit)(chn, sampleBytes, res);

			sampleBytes[chn] += playback.byteStep;
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}
void AudioEngine::fromFloatToRaw_float32(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes[playback.channels];

	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		sampleBytes[chn] = playback.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * playback.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			fval.f = playback.frameBuffer[chn][n]; // safe, cos float is at least 32 bits
			int res = fval.i;

			(*this.*byteSplit)(chn, sampleBytes, res);

			sampleBytes[chn] += playback.byteStep;
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}
/*void AudioEngine::fromFloatToRaw_ufloat(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes[playback.channels];

	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		sampleBytes[chn] = playback.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * playback.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			fval.f = playback.frameBuffer[chn][n];
			int res = fval.i;
			res ^= 1U << (playback.formatBits - 1);

			(*this.*byteSplit)(chn, sampleBytes, res);

			sampleBytes[chn] += playback.byteStep;
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}*/
void AudioEngine::fromFloatToRaw_float64(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		double d;
		int i;
	} dval;

	unsigned char *sampleBytes[playback.channels];

	for(unsigned int chn = 0; chn < playback.channels; chn++) {
		sampleBytes[chn] = playback.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * playback.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			dval.d = playback.frameBuffer[chn][n]; // safe, cos double is at least 64 bits
			int res = dval.i;

			(*this.*byteSplit)(chn, sampleBytes, res);

			sampleBytes[chn] += playback.byteStep;
		}

		// clean up all channels for next period
		memset(playback.frameBuffer[chn], 0, period_size*sizeof(double));
	}
}



void AudioEngine::fromRawToFloat_int(snd_pcm_uframes_t offset, int numOfSamples) {

	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res = (*this.*byteCombine)(chn, sampleBytes);

			// if number we retrieve is larger than max, then this is a negative num using fewer bits than its int container
			if(res>capture.maxVal)
				res |= capture.mask; // we extend its sign to complete the two's complement

			capture.frameBuffer[chn][n] = res/float(capture.maxVal);
			sampleBytes[chn] += capture.byteStep;
		}
	}
}
void AudioEngine::fromRawToFloat_uint(snd_pcm_uframes_t offset, int numOfSamples) {
	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = (((unsigned char *)capture.areas[chn].addr) + (capture.areas[chn].first / 8));
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res=(*this.*byteCombine)(chn, sampleBytes);

			res ^= 1U << (capture.formatBits - 1);
			capture.frameBuffer[chn][n] = res/float(capture.maxVal);
			sampleBytes[chn] += capture.byteStep;
		}
	}
}
void AudioEngine::fromRawToFloat_float32(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res=(*this.*byteCombine)(chn, sampleBytes);

			fval.i = res; // safe
			capture.frameBuffer[chn][n] = fval.f;
			sampleBytes[chn] += capture.byteStep;
		}
	}
}
/*void AudioEngine::fromRawToFloat_ufloat(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res=(*this.*byteCombine)(chn, sampleBytes);

			fval.i = res;
			res ^= 1U << (capture.formatBits - 1);
			capture.frameBuffer[chn][n] = fval.f;
			sampleBytes[chn] += capture.byteStep;
		}
	}
}*/
void AudioEngine::fromRawToFloat_float64(snd_pcm_uframes_t offset, int numOfSamples) {
	union {
		double d;
		int i;
	} dval;

	unsigned char *sampleBytes[capture.channels];

	for(unsigned int chn = 0; chn < capture.channels; chn++) {
		sampleBytes[chn] = capture.rawSamplesStartAddr[chn];
		sampleBytes[chn] += offset * capture.byteStep;

		for(int n = 0; n < numOfSamples; n++)  {
			int res=(*this.*byteCombine)(chn, sampleBytes);

			dval.i = res;
			capture.frameBuffer[chn][n] = dval.d;
			sampleBytes[chn] += capture.byteStep;
		}
	}
}


/*double AudioEngine::readGeneratorsSample() {
	audioSample = 0;
	for(int i=0; i<numOfAudioModulesOut; i++)
		audioSample += audioOut[i]->getSample();

	interpolateVolume();

	audioSample *= volume; // overall engine audio volume

	clampGeneratorSample();

	return audioSample;
}*/


void AudioEngine::readAudioModulesBuffers(int numOfSamples, double **framebufferOut, double **framebufferIn) {
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

	// sum altogether
	//TODO use SMID to sum samples from AudioModules
	for(int i=0; i<numOfAudioModules; i++) {
		for(int j=audioModulesChnOffset[i]; j<audioModulesChnOffset[i]+audioModulesChannels[i]; j++) {
			for(int n=0; n<numOfSamples; n++)
				framebufferOut[j][n] += moduleFramebuffer[i][j][n];
		}
	}
}


/*void AudioEngine::clampGeneratorSample() {
	if(audioSample > 1)
		audioSample = 1;
	else if(audioSample < -1)
		audioSample = -1;
}*/

//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


/*
//-----------------------------------------------------------------------------------------------------------
// Audio cape memory fix, only if on BBB, which is an ARM platform
//-----------------------------------------------------------------------------------------------------------
#ifdef __arm__
void AudioEngine::pokeMem(const char *argv1, const char *argv2, const char *argv3)
{
	int fd;
    void *map_base, *virt_addr;
	unsigned long read_result, writeval;
	off_t target;
	int access_type = 'w';

	bool _verbose = false;

	target = strtoul(argv1, 0, 0);

	access_type = tolower(argv2[0]);

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    if(_verbose)
        	printf("/dev/mem opened.\n");
    fflush(stdout);

    // Map one page
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) FATAL;
    if(_verbose)
        	printf("Memory mapped at address %p.\n", map_base);
    fflush(stdout);

    virt_addr = map_base + (target & MAP_MASK);
    switch(access_type) {
		case 'b':
			read_result = *((unsigned char *) virt_addr);
			break;
		case 'h':
			read_result = *((unsigned short *) virt_addr);
			break;
		case 'w':
			read_result = *((unsigned long *) virt_addr);
			break;
		default:
			fprintf(stderr, "Illegal data type '%c'.\n", access_type);
			exit(2);
	}
    if(_verbose)
    	printf("Value at address 0x%X (%p): 0x%X\n", (unsigned int) target, virt_addr, (unsigned int) read_result);
    fflush(stdout);


	writeval = strtoul(argv3, 0, 0);
	switch(access_type) {
		case 'b':
			*((unsigned char *) virt_addr) = writeval;
			read_result = *((unsigned char *) virt_addr);
			break;
		case 'h':
			*((unsigned short *) virt_addr) = writeval;
			read_result = *((unsigned short *) virt_addr);
			break;
		case 'w':
			*((unsigned long *) virt_addr) = writeval;
			read_result = *((unsigned long *) virt_addr);
			break;
	}
    if(_verbose)
    	printf("Written 0x%X; readback 0x%X\n", (unsigned int) writeval, (unsigned int) read_result);
	fflush(stdout);


	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
    close(fd);
    return;
}
#endif
*/
//-----------------------------------------------------------------------------------------------------------


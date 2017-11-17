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
	// default params
	output = NULL;
	format = SND_PCM_FORMAT_S16;    /* sample format */
	period_size = 256;				/* period length in frames */
	buffer_size = 512;				/* ring buffer length in frames */
	rate = 44100;                   /* stream rate */
	channels = 2;                   /* count of channels */
	period_time = 100000;           /* period time in us */
	verbose = 0;                    /* verbose flag */
	resample = 1;                   /* enable alsa-lib resampling */
	period_event = 0;               /* produce poll event after each period */
	strcpy( device, "hw:0,0" );     /* playback device...just in case */
	cardName = "";

	method = 0;						/* alsa low level buffer transfer method...other methods still to implement...*/
	transfer_methods[method].name			= "write";
	transfer_methods[method].access		   	= SND_PCM_ACCESS_RW_INTERLEAVED;
	transfer_methods[method].transfer_loop 	= &AudioEngine::write_loop;
	/*{
		{ "write", 					SND_PCM_ACCESS_RW_INTERLEAVED, 		write_loop },
		{ "write_and_poll", 		SND_PCM_ACCESS_RW_INTERLEAVED, 		write_and_poll_loop },
		{ "async", 					SND_PCM_ACCESS_RW_INTERLEAVED, 		async_loop },
		{ "async_direct", 			SND_PCM_ACCESS_MMAP_INTERLEAVED, 	async_direct_loop },
		{ "direct_interleaved", 	SND_PCM_ACCESS_MMAP_INTERLEAVED, 	direct_loop },
		{ "direct_noninterleaved", 	SND_PCM_ACCESS_MMAP_NONINTERLEAVED, direct_loop },
		{ "direct_write", 			SND_PCM_ACCESS_MMAP_INTERLEAVED, 	direct_write_loop },
		{ NULL, 					SND_PCM_ACCESS_RW_INTERLEAVED, 		NULL }
	};*/

	gSampleBuffers = NULL;

	volume = ref_volume = 1;						// starts as maximum
	inc_volume 			= 0;

	engineReady		= false;
	engineIsRunning = false;

	numOfOutputs = 0;

	// misc stuff
	areas        = NULL;
	big_endian   = -1;
	bps	         = -1;
	byteStep     = -1;
	chn   		 = 1;
	delta_volume = 0;
	err			 = 0;
	format_bits  = 0;
	formatInt    = 0;
	frames	     = 0;
	handle	     = NULL;
	hwparams     = NULL;
	is_float     = -1;
	maxval       = 0;
	phys_bps     = -1;
	samples      = NULL;
	swparams	 = NULL;
	audioSample    = 0;
	to_unsigned  = -1;

	audioSampleBuffer = NULL;
}

AudioEngine::~AudioEngine() {
	pthread_mutex_destroy(&vol_lock);

	// to avoid double free
	if(engineReady) {
		// if running...
		if(engineIsRunning)
			engineIsRunning = false; // ...use automatic shutdown
		else
			shut_engine(); 			// ...otherwise shut manually
	}
}

int AudioEngine::initEngine() {
	audioSampleBuffer = new float[period_size];
	memset(audioSampleBuffer, 0, sizeof(float)*period_size);

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);


	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		printf("Output failed: %s\n", snd_strerror(err));
		return 11;
	}

	int retval;
	printf("\n");
	if(cardName!="")
		retval = findDevice(); // simplest way is to set card name and look for device 0 of that card
	else
		retval = printCardName(); // otherwise we check the name of the card to which the passed or default device belongs
	printf("\n");

	if(retval!=0)
		return retval;



	printf("Stream parameters are %iHz, %s, %i channels\n", rate, snd_pcm_format_name(format), channels);
	printf("Using transfer method: %s\n", transfer_methods[method].name);
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		return 0;
	}

	if ((err = set_hwparams()) < 0) {
		printf("Setting of hwparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	if ((err = set_swparams()) < 0) {
		printf("Setting of swparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	if (verbose > 0)
		snd_pcm_dump(handle, output);
	samples = (short int *) malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
	if (samples == NULL) {
		printf("No enough memory\n");
		exit(EXIT_FAILURE);
	}

	gSampleBuffers = new float*[channels];

	areas = (snd_pcm_channel_area_t *) calloc(channels, sizeof(snd_pcm_channel_area_t));
	if (areas == NULL) {
		printf("No enough memory\n");
		delete[] gSampleBuffers;
		exit(EXIT_FAILURE);
	}

	for (chn = 0; chn < channels; chn++) {
		 /* Allocate buffers */
		gSampleBuffers[chn] = new float[period_size];
		if(gSampleBuffers[chn] == NULL) {
			printf("No enough memory\n");
			exit(EXIT_FAILURE);
		}
		bzero(gSampleBuffers[chn], period_size * sizeof(float));

		areas[chn].addr  = samples;
		areas[chn].first = chn * snd_pcm_format_physical_width(format);
		areas[chn].step  = channels * snd_pcm_format_physical_width(format);
	}


	// this only if on BBB, which is an ARM platform
	#ifdef __arm__
	if(strcmp(device, "plughw:0,0")==0)
		pokeMem("0x480380ac", "w", "0x100");
	#endif

	engineReady = true;


	// set low level params
	byteStep = areas[0].step / 8;
	format_bits = snd_pcm_format_width(format);
	maxval = (1 << (format_bits - 1)) - 1;
	bps = format_bits / 8;  /* bytes per sample */
	phys_bps = snd_pcm_format_physical_width(format) / 8;
	big_endian = snd_pcm_format_big_endian(format) == 1;
	to_unsigned = snd_pcm_format_unsigned(format) == 1;
	is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

	return 0;
}

int AudioEngine::startEngine() {
	if(verbose==1)
		printf("Starting AudioEngine\n");
	engineIsRunning = true;

	initRender();

	err = (this->*(transfer_methods[method].transfer_loop))();


	if (err < 0) {
		printf("Transfer failed: %s\n", snd_strerror(err));
		return 1;
	}

	cleanUpRender();

	shut_engine();

	return 0;
}


int AudioEngine::stopEngine() {
	engineIsRunning = false;
	return 0;
}




//----------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------

// adapted from https://gist.github.com/dontknowmyname/4536535
int AudioEngine::printCardName() {
	// get card name from device id

	snd_ctl_t *cardHandle;
	string deviceStr = string(device); // includes card, device and possibly subdevice
	string card; // we want no device name nor sub-device

	// let's scrap them!
	string::size_type pos = deviceStr.find(','); // comma should always separates card name from rest
	 if (pos != string::npos)
		card =  deviceStr.substr(0, pos);
	 else {
		 printf("device name is probably wrong...\n");
		 return -1;
	 }

	if ((err = snd_ctl_open(&cardHandle, card.c_str(), 0)) < 0) {
		printf("Can't open device ]: - %s\n", snd_strerror(err));
		return err;
	}

	snd_ctl_card_info_t *cardInfo;
	// we need to get a snd_ctl_card_info_t, just allocate it on the stack
	snd_ctl_card_info_alloca(&cardInfo);
	// tell ALSA to fill in our snd_ctl_card_info_t with info about this card
	if ((err = snd_ctl_card_info(cardHandle, cardInfo)) < 0) {
		printf("Can't get info for device -  %s\n", snd_strerror(err));
		return err;
	}

	// close the card's control interface after we're done with it
	snd_ctl_close(cardHandle);

	// ALSA allocates some mem to load its config file when we call some of the above functions
	// now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
	snd_config_update_free_global();


	printf("Playback device is %s\n", device);
	printf("%s\n", snd_ctl_card_info_get_name(cardInfo));

	return 0;
}

// adapted from https://gist.github.com/dontknowmyname/4536535
int AudioEngine::findDevice() {
	// search for card [from card name] and then build device id

	bool cardFound = false;
	int cardNum = -1;
	snd_ctl_t *cardHandle;

	while(!cardFound) {
		if ((err = snd_card_next(&cardNum)) < 0) {
			fprintf(stderr, "Can't get the next card number: %s\n", snd_strerror(err));
			break;
		}

		if (cardNum < 0)
			break; // no more cards

		char str[64];
		sprintf(str, "hw:%i", cardNum);
		if ((err = snd_ctl_open(&cardHandle, str, 0)) < 0)	//Now cardHandle becomes your sound card.
		{
			printf("Can't open card %i: %s\n", cardNum, snd_strerror(err));
			continue;
		}

		snd_ctl_card_info_t *cardInfo;	// use to hold card information
		// we need to get a snd_ctl_card_info_t, just allocate it on the stack
		snd_ctl_card_info_alloca(&cardInfo);
		// tell ALSA to fill in our snd_ctl_card_info_t with info about this card
		if ((err = snd_ctl_card_info(cardHandle, cardInfo)) < 0) {
			printf("Can't get info for card %i: %s\n", cardNum, snd_strerror(err));
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

		printf("Playback device is %s\n", device);
		printf("%s\n", snd_ctl_card_info_get_name(cardInfo));
		cardFound = true;
	}


	if(!cardFound)
		return 1; // gne'


	return 0;
}


int AudioEngine::set_hwparams() {
	unsigned int rrate;
	snd_pcm_uframes_t size;
	int err, dir;
	/* choose all parameters */
	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) {
		printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		return err;
	}
	/* set hardware resampling */
	err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, resample);
	if (err < 0) {
		printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, hwparams, transfer_methods[method].access);
	if (err < 0) {
		printf("Access type not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the sample format */
	err = snd_pcm_hw_params_set_format(handle, hwparams, format);
	if (err < 0) {
		printf("Sample format not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
	if (err < 0) {
		printf("Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(err));
		return err;
	}
	/* set the stream rate */
	rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, 0);
	if (err < 0) {
		printf("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
		return err;
	}
	if (rrate != rate) {
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		return -EINVAL;
	}

	snd_pcm_hw_params_get_period_size_min(hwparams, &frames, &dir);
	printf("\nMin Period size: %d frames\n", (int)frames);
	snd_pcm_hw_params_get_period_size_max(hwparams, &frames, &dir);
	printf("Max Period size: %d frames\n", (int)frames);

	snd_pcm_hw_params_get_buffer_size_min(hwparams, &frames);
	printf("Min Buffer size: %d frames\n", (int)frames);
	snd_pcm_hw_params_get_buffer_size_max(hwparams, &frames);
	printf("Max Buffer size: %d frames\n", (int)frames);


	/* set the period size */
	err = snd_pcm_hw_params_set_period_size(handle, hwparams, period_size, dir);
	if (err < 0) {
		printf("Unable to set period size %i for playback: %s\n", (int)period_size, snd_strerror(err));
		return err;
	}
	// check it
	err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
	if (err < 0) {
		printf("Unable to get period size for playback: %s\n", snd_strerror(err));
		return err;
	}
	period_size = size;
	printf("----------period size: %d frames\n", (int)period_size);

	/* set the buffer size */
	err = snd_pcm_hw_params_set_buffer_size	(handle, hwparams, buffer_size);
	if (err < 0) {
		printf("Unable to set buffer size %i for playback: %s\n", (int)buffer_size, snd_strerror(err));
		return err;
	}
	// check it
	err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
	if (err < 0) {
		printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
		return err;
	}
	buffer_size = size;
	printf("----------buffer size: %d frames\n", (int)buffer_size);


	/* write the parameters to device */
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) {
		printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}



int AudioEngine::set_swparams()
{
	int err;
	/* get the current swparams */
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
	if (err < 0) {
		printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
	if (err < 0) {
		printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* enable period events when requested */
	if (period_event) {
		err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
		if (err < 0) {
			printf("Unable to set period event: %s\n", snd_strerror(err));
			return err;
		}
	}
	/* write the parameters to the playback device */
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}


/*
 *   Transfer method - write only
 */
int AudioEngine::write_loop() {
	signed short *ptr;
	int cptr;

	while (engineIsRunning) {
		fill_period(0, period_size);
		ptr = samples;
		cptr = period_size;
		while (cptr > 0) {
			err = snd_pcm_writei(handle, ptr, cptr);
			if (err == -EAGAIN)
				continue;
			if (err < 0) {
				if (xrun_recovery() < 0){
					printf("Write error: %s\n", snd_strerror(err));
					exit(EXIT_FAILURE);
				}
				break;  // skip one period
			}
			ptr += err * channels;
			cptr -= err;
		}
	}
	return 0;
}


int AudioEngine::shut_engine() {
	// free mem
	if(audioSampleBuffer!=NULL)
		delete[] audioSampleBuffer;
	if(areas != NULL) {
		free(areas);
		//free(gSampleBuffers[0]);
	    //free(gSampleBuffers[1]);
		delete[] gSampleBuffers;
	}
	if(samples != NULL)
		free(samples);
	if(handle != NULL)
		snd_pcm_close(handle);

	engineReady = false;

	printf("AudioEngine stopped\n");
	return 0;
}


/*
 *   Underrun and suspend recovery
 */
int AudioEngine::xrun_recovery() {
	printf("underrun!\n");
	if (verbose)
			printf("stream recovery\n");
	if (err == -EPIPE) {    /* under-run */
			err = snd_pcm_prepare(handle);
			if (err < 0)
					printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
			return 0;
	} else if (err == -ESTRPIPE) {
			while ((err = snd_pcm_resume(handle)) == -EAGAIN)
					sleep(1);       /* wait until the suspend flag is released */
			if (err < 0) {
					err = snd_pcm_prepare(handle);
					if (err < 0)
							printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
			}
			return 0;
	}
	return err;
}

void AudioEngine::fill_period(snd_pcm_uframes_t offset, int count) {
	unsigned int chn;
	unsigned char *samples[channels];

	/* verify and prepare the contents of areas */
	for (chn = 0; chn < channels; chn++) {
		if ((areas[chn].first % 8) != 0) {
			printf("areas[%i].first == %i, aborting...\n", chn, areas[chn].first);
			exit(EXIT_FAILURE);
		}
		samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
		if ((areas[chn].step % 16) != 0) {
			printf("areas[%i].step == %i, aborting...\n", chn, areas[chn].step);
			exit(EXIT_FAILURE);
		}
		samples[chn] += offset * byteStep;
	}

	/* Call the render function, which is conveniently detached in AudioEngine_render.cpp */
	render((float)rate, channels, count, gSampleBuffers);


	for(int n = 0; n < count; n++) {
		for(chn = 0; chn < channels; chn++) {
			union {
					float f;
					int i;
			} fval;

			int res, i;
			if (is_float) {
					fval.f = gSampleBuffers[chn][n];
					res = fval.i;
			} else {
				res = maxval * gSampleBuffers[chn][n];
			}
			if (to_unsigned)
					res ^= 1U << (format_bits - 1);
			/*for (chn = 0; chn < channels; chn++)*/ { //VIC this loop would force mono by copying gSampleBuffers[0] on every channel
					/* Generate data in native endian format */
					if (big_endian) {
							for (i = 0; i < bps; i++)
									*(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
					} else {
							for (i = 0; i < bps; i++)
									*(samples[chn] + i) = (res >>  i * 8) & 0xff;
					}
					samples[chn] += byteStep;
			}
		}
	}
}


double AudioEngine::readGeneratorsSample() {
	audioSample = 0;
	for(int i=0; i<numOfOutputs; i++)
		audioSample += generator[i]->getSample();

	interpolateVolume();

	audioSample *= volume; // overall engine audio volume

	clampGeneratorSample();

	return audioSample;
}

//TODO use SMID to sum samples from AudioGenerators
//TODO pass audio input to InOut objects
float *AudioEngine::readGeneratorsBuffer(int numOfSamples) {
	// read all buffers
	for(int i=0; i<numOfOutputs; i++)
		genSampleBuffer[i] = generator[i]->getBufferFloat(numOfSamples);

	// sum all samples and apply volume
	for(int n=0; n<numOfSamples; n++) {
		audioSampleBuffer[n] = 0;
		for(int i=0; i<numOfOutputs; i++)
			audioSampleBuffer[n] += genSampleBuffer[i][n];
		audioSampleBuffer[n] *= volume;
		interpolateVolume();	// remove crackles through interpolation
	}
	return audioSampleBuffer;
}


void AudioEngine::clampGeneratorSample() {
	if(audioSample > 1)
		audioSample = 1;
	else if(audioSample < -1)
		audioSample = -1;
}

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


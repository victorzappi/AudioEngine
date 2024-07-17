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
 *  Created on: June 14, 2015
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
 *  as well as
 *
 *  sin_generator_rt
 *
 *  Play a sine wave in real-time using ALSA
 *
 *  Victor Zappi and Andrew McPherson, based on ALSA demos
 *  for ECS732 Real-Time DSP
 *  Queen Mary, University of London
 *
 */

#include <math.h>
#include "AudioEngine.h"

#include "sndfile.h" // save audio (;

/* M_PI is not declared in all C implementations... */
#ifndef		M_PI
#define		M_PI		3.14159265358979323846264338
#endif

// maybe useful with 32float
#define MAX_P 1.2031
#define MIN_P -0.1422


// save audio
//#define BOUNCE
#ifdef BOUNCE
#include <string>
#include <dirent.h>  // to handle files in dirs
#include <fstream>   // read/write to file ifstream/ofstream
#include <sstream>   // ostringstream
using namespace std;

float *outBuffer;

SF_INFO sfinfo;
SNDFILE * outfile;
string outfiledir = ".";
string outfilename = "bounce_";

int getNumOfWavFiles(string dirname) {
	DIR *dir;
	struct dirent *ent;
	int fileCnt = 0;

	// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
	if ((dir = opendir (dirname.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// Ignore dotfiles and . and .. paths
			if(!strncmp(ent->d_name, ".", 1))
				continue;

			//printf("%s\n", ent->d_name);

			// take only .wav files
			string name = string(ent->d_name);
			int len		= name.length();
			if( (name[len-4]=='.') && (name[len-3]=='w') && (name[len-2]=='a') && (name[len-1]=='v') )
				fileCnt++;
		}
		closedir (dir);
	} else {
		/* could not open directory */
		printf("Could not open directory %s!\n", dirname.c_str());
		return -1;
	}
	return fileCnt;
}
#endif





/* Init function, called once before start of render loop */
void AudioEngine::initRender() {

#ifdef BOUNCE
	outBuffer = new float[period_size];
	// change out file name according to how many wav files we have in the dir
	// so that we don't over write
	int fileNum = getNumOfWavFiles(outfiledir);
	ofstream dumpfile;
	ostringstream convert;
	convert << fileNum;
	outfilename += convert.str() + ".wav";

	// save audio
	sfinfo.channels = 1;
	sfinfo.samplerate = rate;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	outfile = sf_open(outfilename.c_str(), SFM_WRITE, &sfinfo);
#endif
}

/* Render function, called when new samples are needed */
void AudioEngine::render(float sampleRate, int numOfSamples, int numOutChannels, double **framebufferOut, int numInChannels, double **framebufferIn) {

	// by default, we call this method that fills the output framebuffer with the sum of the framebuffers of all the modules loaded into the engine
	readAudioModulesBuffers(numOfSamples, framebufferOut, framebufferIn);

#ifdef BOUNCE
	// copy audio buffer to file
	std::copy(framebufferOut[0], framebufferOut[0]+numOfSamples, outBuffer);
	/*sf_count_t count = */sf_write_float(outfile, outBuffer, numOfSamples);
	/*if(count<1)
		printf("): written samples count: %d\n", (int)count);*/
#endif

}

/* Clean up function, called once after end of render loop and before engine stops*/
void AudioEngine::cleanUpRender() {
#ifdef BOUNCE
	delete[] outBuffer;
	// to properly save audio
	sf_write_sync(outfile);
	sf_close(outfile);
#endif
}

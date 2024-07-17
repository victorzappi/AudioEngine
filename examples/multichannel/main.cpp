/*
 * A simple but growing Linux audio engine based on ALSA_
 *
 *
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

// examples using multi channel input/output via external audio interface

#include "AudioEngine.h" // back end
#include <csignal>  // to catch ctrl-c

using std::string;

void ctrlC_handler(int s); // to catch ctrl-c


// engine and global settings
AudioEngine audioEngine;
unsigned short periodSize = 128;
unsigned int rate = 44100;

// external interface ['card' on Linux], originally a Focusrite Scarlett 18i8
string cardName = "Scarlett"; // but a portion of the name is enough for the engine to find it!


// oscillator and its settings
Oscillator *sine;
oscillator_type type = osc_sin_;
double level = 0.07;
double freq = 440; // meeehhh

int main(int argc, char *argv[]) {
	audioEngine.setFullDuplex(true);

	audioEngine.setRate(rate); // same as default setting
	audioEngine.setPeriodSize(periodSize);
	audioEngine.setBufferSize(2*periodSize);

	audioEngine.setPlaybackCardName(cardName);
	audioEngine.setCaptureCardName(cardName);

	audioEngine.initEngine(); // ready to go



	sine = new Oscillator(); 
	sine->init(type, rate, periodSize, level*0.1, freq, 0, false, 1, 2); // mono oscillator, outputted from on channel 2
	audioEngine.addAudioModule(sine); 


	// more audio sources!
	MultiPassthrough *pass = new MultiPassthrough();
	pass->init(periodSize, 0, 2, 2);  // mono input from channel 0, passed through channels 2 and 3
	audioEngine.addAudioModule(pass);

	WavetableOsc *wosc = new WavetableOsc();
	wosc->init(osc_square_, rate, periodSize, level*0.1, 1024, 1, 3);  // mono oscillator, outputted from on channel 3
	wosc->setFrequency(260);
	audioEngine.addAudioModule(wosc);


	// let's catch ctrl-c to nicely stop the application
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = ctrlC_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);



	// triggers an infinite audio loop, can stop with ctrl-c, other similar critical stop commands or via multithreading
	audioEngine.startEngine();


	// these will be reached only when engine is stopped
	delete sine;
	delete pass;
	delete wosc;

	printf("Program ended\nBye bye_\n");
}




// this is called by ctrl-c [note: it does not work in Eclipse]
void ctrlC_handler(int s){
   printf("\nCaught signal %d, stopping engine\n",s);
   audioEngine.stopEngine();
}


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

// make a infinite and annoying sine with the audio engine

#include "AudioEngine.h" // back end

// engine and global settings
AudioEngine audioEngine;
unsigned short periodSize = 256;
unsigned int rate = 44100;


// oscillator and its settings
Oscillator *sine;
oscillator_type type = osc_sin_;
double level = 0.3;
double freq = 440; // meeehhh

int main(int argc, char *argv[]) {

	// these 3 are same as default settings, put here explicitly for clarity only
	audioEngine.setPeriodSize(periodSize);
	audioEngine.setBufferSize(2*periodSize); // this influences buffer latency the most
	audioEngine.setRate(rate);

	audioEngine.initEngine(); // ready to go

	sine = new Oscillator(); // handy to deal with this as pointer or address
	sine->init(rate, periodSize, type, level, freq); // when inited, oscillator is automatically triggered

	// tells engine to retrieve samples from oscillator
	audioEngine.addGenerator(sine); // add generators or any object whose class derives from AudioOutput abstract class

	audioEngine.startEngine(); // triggers an infinite audio loop, can stop with ctrl-c or similar critical stop commands

	// this will never be reached q:
	return 0;
}

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
 * FFT.cpp
 *
 *  Created on: 2016-06-29
 *      Author: Victor Zappi
 *      Comments: modification of FFTW examples [http://www.fftw.org/fftw3_doc/, version 3.3.4]
 *
 *      FFTW is copyright © 1997--1999 Massachusetts Institute of Technology.
 *      FFTW is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation;
 *      either version 2 of the License, or (at your option) any later version.
 *
 */

#include <fftw3.h> // fft stuff
#include <cstring> // memcpy...seems like it is my best friend, always with me...

/*i set up the following code so that fftw3 will try to use SSE. it's gonna be a transparent process*/


void calculateFFT(float samples[], int numOfFFTsamples, float outputSamples[][2]) {
	/* test
	memset(samples, 0, numOfFFTsamples*sizeof(float));
	numOfFFTsamples = 8;
	for(int i=0;i<6;i++)
		samples[i] = i+1;*/

	int numOfOutputSamples = ((float)numOfFFTsamples)/2.0 + 1;
	// prepare working structure, memory aligned to 16 bytes [to work on sse]
	float *fftIn = fftwf_alloc_real(numOfFFTsamples);
	fftwf_complex *fftOut = fftwf_alloc_complex(numOfOutputSamples);
	fftwf_plan p = fftwf_plan_dft_r2c_1d(numOfFFTsamples, fftIn, fftOut, FFTW_ESTIMATE);

	// fill input, after instantiating plan [just in caaase]
	memcpy(fftIn, samples, numOfFFTsamples*sizeof(float)); // can use memcpy even if alignment is different in the 2 arrays, cos in both cases all elements are contiguous

	// do fft
	fftwf_execute(p);

	//fftwf_print_plan(p); // check if we're using sse [look for 'v' subscript in printed text]

	// copy fft output to return array
	memcpy(outputSamples, fftOut, numOfOutputSamples*2*sizeof(float));

	// get rid of stuff
	fftwf_destroy_plan(p);
	fftwf_free(fftOut);

	/* slower complex version, not useful for audio
  	fftwf_complex *in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * numOfFFTsamples);
	fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * numOfFFTsamples);
	fftwf_plan p2 = fftwf_plan_dft_1d(numOfFFTsamples, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

	for(int i=0; i<numOfFFTsamples; i++) {
	   in[i][0] = samples[i];
	   in[i][1] = 0;
	}

	fftwf_execute(p2);

	// get rid of stuff
	fftwf_destroy_plan(p2);
	fftwf_free(in);
	fftwf_free(out);*/
}

void calculateFFT(double samples[], int numOfFFTsamples, double outputSamples[][2]) {
	/* test
	memset(samples, 0, numOfFFTsamples*sizeof(float));
	numOfFFTsamples = 8;
	for(int i=0;i<6;i++)
		samples[i] = i+1;*/

	int numOfOutputSamples = ((double)numOfFFTsamples)/2.0 + 1;
	// prepare working structure, memory aligned to 16 bytes [to work on sse]
	double *fftIn = fftw_alloc_real(numOfFFTsamples);
	fftw_complex *fftOut = fftw_alloc_complex(numOfOutputSamples);
	fftw_plan p = fftw_plan_dft_r2c_1d(numOfFFTsamples, fftIn, fftOut, FFTW_ESTIMATE);

	// fill input, after instantiating plan [just in caaase]
	memcpy(fftIn, samples, numOfFFTsamples*sizeof(double)); // can use memcpy even if alignment is different in the 2 arrays, cos in both cases all elements are contiguous

	// do fft
	fftw_execute(p);

	//fftw_print_plan(p); // check if we're using sse [look for 'v' subscript in printed text]

	// copy fft output to return array
	memcpy(outputSamples, fftOut, numOfOutputSamples*2*sizeof(double));

	// get rid of stuff
	fftw_destroy_plan(p);
	fftw_free(fftOut);
}

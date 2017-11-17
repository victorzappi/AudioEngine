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
 * PinkNoise.h
 *
 *  Created on: Oct 15, 2013
 *      Author: Victor Zappi
 *
 *  This class has been created starting from:
 *
 *  Technique by Larry "RidgeRat" Trammell 3/2006
 *  http://home.earthlink.net/~ltrammell/tech/pinkalg.htm
 *  implementation and optimization by David Lowenfels
 */


#ifndef _PinkNoise_H
#define _PinkNoise_H



#include <cstdlib>
#include <ctime>
#include <stdlib.h>

#define PINK_NOISE_NUM_STAGES 3

class PinkNoise {
public:
  PinkNoise() {
  srand ( time(NULL) ); // initialize random generator
    clear();
  }

  void clear() {
    for( size_t i=0; i< PINK_NOISE_NUM_STAGES; i++ )
      state[ i ] = 0.0;
    }

  float tick() {
    static const float RMI2 = 2.0 / float(RAND_MAX); // + 1.0; // change for range [0,1)
    static const float offset = A[0] + A[1] + A[2];

  // unrolled loop
    float temp = float( rand() );
    state[0] = P[0] * (state[0] - temp) + temp;
    temp = float( rand() );
    state[1] = P[1] * (state[1] - temp) + temp;
    temp = float( rand() );
    state[2] = P[2] * (state[2] - temp) + temp;
    return ( A[0]*state[0] + A[1]*state[1] + A[2]*state[2] )*RMI2 - offset;
  }

protected:
  float state[ PINK_NOISE_NUM_STAGES ];
  static const float A[ PINK_NOISE_NUM_STAGES ];
  static const float P[ PINK_NOISE_NUM_STAGES ];
};

//const float PinkNoise::A[] = { 0.02109238, 0.07113478, 0.68873558 }; // rescaled by (1+P)/(1-P)
//const float PinkNoise::P[] = { 0.3190,  0.7756,  0.9613  };

#endif

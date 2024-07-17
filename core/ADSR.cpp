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
 * ADSR.h
 *
 *  Created on: 2014-10
 *      Author: Victor Zappi
 *
 *  This class has been created starting from:
 *
 *  ADSR.h
 *
 *  Created by Nigel Redmon on 12/18/12.
 *  EarLevel Engineering: earlevel.com
 *  Copyright 2012 Nigel Redmon
 *
 *  For a complete explanation of the ADSR envelope generator and code,
 *  read the series of articles by the author, starting here:
 *  http: *www.earlevel.com/main/2013/06/01/envelope-generators/
 *
 *  License:
 *
 *  This source code is provided as is, without warranty.
 *  You may copy and distribute verbatim copies of this document.
 *  You may modify and use this source code to create binary code for your own purposes, free or commercial.
 *
 */
#include "ADSR.h"
#include <math.h>
#include <stdio.h> // NULL


ADSR::ADSR(void) {
	buffer = NULL;
	setPeriodSize(1);
    reset();
    setAttackRate(0);
    setDecayRate(0);
    setReleaseRate(0);
    setSustainLevel(1.0);
    setTargetRatioA(0.3);
    setTargetRatioDR(0.0001);
}

ADSR::ADSR(unsigned int period_size) {
	buffer = NULL;
	setPeriodSize(period_size);
    reset();
    setAttackRate(0);
    setDecayRate(0);
    setReleaseRate(0);
    setSustainLevel(1.0);
    setTargetRatioA(0.3);
    setTargetRatioDR(0.0001);
}

void ADSR::setPeriodSize(unsigned int period_size) {
	if(buffer != NULL)
		delete[] buffer;

	periodSize = period_size;
	buffer = new float[periodSize];
	reset();
}


ADSR::~ADSR(void) {
	if(buffer != NULL)
		delete[] buffer;
}

void ADSR::setAttackRate(float rate) {
    attackRate = rate;
    attackCoef = calcCoef(rate, targetRatioA);
    attackBase = (1.0 + targetRatioA) * (1.0 - attackCoef);
}

void ADSR::setDecayRate(float rate) {
    decayRate = rate;
    decayCoef = calcCoef(rate, targetRatioDR);
    decayBase = (sustainLevel - targetRatioDR) * (1.0 - decayCoef);
}

void ADSR::setReleaseRate(float rate) {
    releaseRate = rate;
    releaseCoef = calcCoef(rate, targetRatioDR);
    releaseBase = -targetRatioDR * (1.0 - releaseCoef);
}

float ADSR::calcCoef(float rate, float targetRatio) {
    return exp(-log((1.0 + targetRatio) / targetRatio) / rate);
}

void ADSR::setSustainLevel(float level) {
    sustainLevel = level;
    decayBase = (sustainLevel - targetRatioDR) * (1.0 - decayCoef);
}

void ADSR::setTargetRatioA(float targetRatio) {
    if (targetRatio < 0.000000001)
        targetRatio = 0.000000001;  // -180 dB
    targetRatioA = targetRatio;
    attackBase = (1.0 + targetRatioA) * (1.0 - attackCoef);
}

void ADSR::setTargetRatioDR(float targetRatio) {
    if (targetRatio < 0.000000001)
        targetRatio = 0.000000001;  // -180 dB
    targetRatioDR = targetRatio;
    decayBase = (sustainLevel - targetRatioDR) * (1.0 - decayCoef);
    releaseBase = -targetRatioDR * (1.0 - releaseCoef);
}

float *ADSR::processBuffer(int numOfSamples) {
	for(int n=0; n<numOfSamples; n++)
		buffer[n] = process();
	return buffer;
}

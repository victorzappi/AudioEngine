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
 * Biquad.h
 *
 *  Created on: 2014-10
 *      Author: Victor Zappi
 *
 *  This class has been created starting from:
 *
 *  Biquad.h
 *
 *  Created by Nigel Redmon on 11/24/12
 *  EarLevel Engineering: earlevel.com
 *  Copyright 2012 Nigel Redmon
 *
 *  For a complete explanation of the Biquad code:
 *  http: *www.earlevel.com/main/2012/11/25/biquad-c-source-code/
 *
 *  License:
 *
 *  This source code is provided as is, without warranty.
 *  You may copy and distribute verbatim copies of this document.
 *  You may modify and use this source code to create binary code
 *  for your own purposes, free or commercial.
 *
 */


#ifndef Biquad_h
#define Biquad_h

enum {
    bq_type_lowpass = 0,
    bq_type_highpass,
    bq_type_bandpass,
    bq_type_notch,
    bq_type_peak,
    bq_type_lowshelf,
    bq_type_highshelf
};

class Biquad {
public:
    Biquad();
    Biquad(int type, double Fc, double Q, double peakGainDB);
    ~Biquad();
    void setType(int type);
    void setQ(double Q);
    void setFc(double Fc);
    void setPeakGain(double peakGainDB);
    void setBiquad(int type, double Fc, double Q, double peakGain);
    double process(double in);
    void process(double *inout, int numOfSamples);
    
    double getQ();
    double getFc();
    double getPeakGain();

    double getStartingQ();
	double getStartingFc();
	double getStartingPeakGain();

protected:
    void calcBiquad(void);

    int type;
    double a0, a1, a2, b1, b2;
    double Fc, Q, peakGain;
    double startFc, startQ, startPeakGain;
    double z1, z2;
};

inline double Biquad::getQ()
{
	return Q;
}

inline double Biquad::getFc()
{
	return Fc;
}

inline double Biquad::getPeakGain()
{
	return peakGain;
}

inline double Biquad::getStartingQ()
{
	return startQ;
}

inline double Biquad::getStartingFc()
{
	return startFc;
}

inline double Biquad::getStartingPeakGain()
{
	return startPeakGain;
}

inline double Biquad::process(double in) {
    double out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
}

inline void Biquad::process(double *inout, int numOfSamples) {
    double out;

    for(int i=0; i<numOfSamples; i++) {
		out = inout[i] * a0 + z1;
		z1 = inout[i] * a1 + z2 - b1 * out;
		z2 = inout[i] * a2 - b2 * out;
		inout[i] = out;
    }
}

#endif // Biquad_h

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
 * AnalogInput.h
 *
 *  Created on: Oct 17, 2013
 *      Author: Victor Zappi
 */

#ifndef ANALOGINPUT_H_
#define ANALOGINPUT_H_

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>

using namespace std;

class AnalogInput
{
private:
	FILE *ActivateAnalogHnd;
	string activateAnalogPath;
	bool analogIsSet;

	FILE *AnalogInHnd;
	string analogInPath;
	bool helperNumFound;

	// suport var for init
	string startPath;
	string readPath;

	glob_t  globbuf;

	// support vars for pin reading
	long lSize;
	char * buffer;
	size_t result;

	bool verbose;

public:
	AnalogInput();
	~AnalogInput();

	int initAnalogInputs();
	int read(int index);

};




#endif /* ANALOGINPUT_H_ */

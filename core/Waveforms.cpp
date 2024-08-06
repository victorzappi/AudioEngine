/*
 * WAveforms.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

#include "Waveforms.h"

#include <algorithm> // reverse_copy
#include <limits> // for last frame


//----------------------------------------------------------------------------------------------------------------------------
// Waveform
//----------------------------------------------------------------------------------------------------------------------------
Waveform::Waveform() : MultichannelOutUtils(this) {
	advType 	  = adv_loop_;
	currentFrame = -1;
	direction	  = 0;
	waveFormBuffer = NULL;
	frameNum      = -1;
	lastFrame	  = std::numeric_limits<unsigned long>::max();
	isPlaying     = false;

	advanceSample   = NULL;
	getBufferMethod = NULL;
}

Waveform::Waveform(advanceType adv) : MultichannelOutUtils(this) {
	advType 	  = adv;
	currentFrame = -1;
	direction	  = 0;
	waveFormBuffer = NULL;
	frameNum      = -1;
	isPlaying     = false;

	advanceSample   = NULL;
	getBufferMethod = NULL;
}

void Waveform::init(double *samples, int len, unsigned int periodSize, double level, unsigned short outChannels, unsigned short outChnOffset) {
	AudioModuleOut::init(periodSize, outChannels, outChnOffset);

	frameNum = len;

	if(waveFormBuffer!=NULL)
		delete waveFormBuffer;
	waveFormBuffer = new double[frameNum+1]; // +1 for silent frame
	// copy samples
	memcpy(waveFormBuffer, samples, sizeof(double)*frameNum);
	waveFormBuffer[frameNum] = 0; // silent frame

	direction	  = 1; // go forward
	currentFrame = 0;

	setAdvanceSampleMethod();

	setLevel(level);

	isPlaying = true; // starts playing
}

void Waveform::reverse() {
	if(waveFormBuffer==NULL) {
		printf("Cannot reverse Waveform, it was not inited yet!\n");
		return;
	}

	double tmp;
	// reverse frames, leaving silent frame at the end [full length would be frameNum+1]
	for(unsigned int i=0; i<frameNum/2; i++) {
		tmp = waveFormBuffer[frameNum-i];
		waveFormBuffer[frameNum-i] = waveFormBuffer[i];
		waveFormBuffer[i] = tmp;
	}
}

double Waveform::getSample() {
	if(!isPlaying)
		return 0;

	double sample = waveFormBuffer[currentFrame];
	(this->*advanceSample) (); // inline

	return sample*level;
}

double **Waveform::getFrameBuffer(int numOfSamples){
	if(!isPlaying)
		memset(framebuffer[out_chn_offset], 0, numOfSamples*sizeof(double));
	else {
		(this->*getBufferMethod) (numOfSamples);
		for(int n=0; n<numOfSamples; n++)
			framebuffer[out_chn_offset][n] *= level;

		memset(framebuffer[out_chn_offset]+numOfSamples, 0, (period_size-numOfSamples)*sizeof(double)); // reset part of buffer that has been potentially left untouched
	}

	MultichannelOutUtils::cloneFrameChannels(numOfSamples);

	return framebuffer;
}
//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------

void Waveform::setAdvanceSampleMethod() {
	switch(advType) {
		case adv_oneShot_:
			advanceSample   = &Waveform::advanceSampleOneShot;
			getBufferMethod = &Waveform::getBufferOneShot;
			break;
		case adv_loop_:
			advanceSample   = &Waveform::advanceSampleLoop;
			getBufferMethod = &Waveform::getBufferLoop;
			break;
		case adv_backAndForth_:
			advanceSample   = &Waveform::advanceSampleBackAndForth;
			getBufferMethod = &Waveform::getBufferBackAndForth;
			break;
		default:
			advanceSample   = &Waveform::advanceSampleLoop;
			getBufferMethod = &Waveform::getBufferLoop;
			break;
	}
}

double *Waveform::getBufferOneShot(int numOfSamples) {
	if(currentFrame<frameNum) {
		int overflow = (currentFrame+numOfSamples)-frameNum;
		if(overflow<=0) {
			memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame,  sizeof(double)*numOfSamples); // simply put at the  beginning of framebuffer[out_chn_offset] all the file samples that are in a row from current position
			currentFrame += numOfSamples; // update
		} else { // otherwise we have to pad with zeros
			memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame, sizeof(double)*(frameNum-currentFrame)); // put at the beginning of framebuffer[out_chn_offset] all the file samples that are in a row
			memset(framebuffer[out_chn_offset]+frameNum-currentFrame+1, 0, sizeof(double)*overflow); // then fill the rest of the sample buffer with zeros
			currentFrame = frameNum; // update
		}
	}
	else {
		memset(framebuffer[out_chn_offset], 0, sizeof(double)*numOfSamples);
		isPlaying = false;
	}
	return framebuffer[out_chn_offset];
}
double *Waveform::getBufferLoop(int numOfSamples){
	int overflow = (currentFrame+numOfSamples)-frameNum;
	// if we pick frames that are all in a row within the buffer
	if(overflow<=0) {
		memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame,  sizeof(double)*numOfSamples); // simply put at the beginning of framebuffer[out_chn_offset] all the file samples that are in a row from current position
		currentFrame += numOfSamples; // update
	} else { // otherwise we have to start from beginning
		memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame, sizeof(double)*(frameNum-currentFrame)); // put in beginning of framebuffer[out_chn_offset] all the file samples that are in a row
		memcpy(framebuffer[out_chn_offset]+frameNum-currentFrame+1, waveFormBuffer, sizeof(double)*overflow); // then fill the rest of the sample buffer with the first file samples
		currentFrame = overflow+1; // update
	}
	return framebuffer[out_chn_offset];
}

double *Waveform::getBufferBackAndForth(int numOfSamples) {
	// forward
	if(direction == 1) {
		int overflow = (currentFrame+numOfSamples)-frameNum;
		// if we pick frames that are all in a row within the buffer
		if(overflow<=0) {
			memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame,  sizeof(double)*numOfSamples); // simply put at the beginning of framebuffer[out_chn_offset] all the file samples that are in a row from current position
			currentFrame += numOfSamples; // update
		} else { // otherwise we reach the end of the file and then go backwards
			memcpy(framebuffer[out_chn_offset], waveFormBuffer+currentFrame, sizeof(double)*(frameNum-currentFrame)); // put at the beginning of framebuffer[out_chn_offset] all the file samples that are in a row
			std::reverse_copy(waveFormBuffer+frameNum-1-overflow, waveFormBuffer+frameNum-1, framebuffer[out_chn_offset]+frameNum-currentFrame+1); // then fill the rest of the sample buffer with the reversed file samples [the last sample is skipped backwards]
			currentFrame = frameNum-2-overflow; // update
			direction = -1;	// officially change direction
		}
	}
	else { // backwards
		int overflow = numOfSamples-currentFrame;
		if(overflow<=0) { // if we pick frames that are all in a row within the buffer
			std::reverse_copy(waveFormBuffer+currentFrame+1-numOfSamples, waveFormBuffer+currentFrame+1, framebuffer[out_chn_offset]);// simply put at the beginning of framebuffer[out_chn_offset] all the file samples that are in a row from current position, in reverse order
			currentFrame -= numOfSamples; // update
		} else { // otherwise we go backwards
			std::reverse_copy(waveFormBuffer, waveFormBuffer+currentFrame+1, framebuffer[out_chn_offset]); // put at the beginning of framebuffer[out_chn_offset] all the reversed file samples that are in a row
			memcpy(framebuffer[out_chn_offset]+numOfSamples-overflow, waveFormBuffer+1,  sizeof(double)*overflow); // then fill the rest of the sample buffer with the first file samples [the first sample is skipped forward]
			currentFrame = overflow+1; // update
			direction = 1; // officially change direction
		}
	}

	return framebuffer[out_chn_offset];
}
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------













//----------------------------------------------------------------------------------------------------------------------------
// AudioFile
//----------------------------------------------------------------------------------------------------------------------------

AudioFile::AudioFile() : Waveform() {
	samplerate = -1;
	sndfile    = NULL;
	memset(&sfinfo, 0, sizeof(sfinfo)); // this to avoid a sporadic error on file load. Thanks to snappizz in https://github.com/supercollider/supercollider/issues/2474
}

AudioFile::AudioFile(advanceType adv) : Waveform(adv){
	samplerate = -1;
	sndfile    = NULL;
	memset(&sfinfo, 0, sizeof(sfinfo));  // this to avoid a sporadic error on file load. Thanks to snappizz in https://github.com/supercollider/supercollider/issues/2474
}

int AudioFile::init(std::string filename, int rate, unsigned int periodSize, double level, int chnIndex, unsigned short outChannels, unsigned short outChnOffset){
	if( !(sndfile = sf_open((const char*)(filename.c_str()), SFM_READ, &sfinfo)) ) {
		sf_perror(sndfile);
		printf("Audiofile %s can't be loaded.../:\n", filename.c_str());
		return -1;
	}

	if(sfinfo.channels !=1 && chnIndex>=sfinfo.channels) {
		printf("Audiofile %s error! The requested channel (%d) cannot be extracted, since file only has %d channels\n", filename.c_str(), chnIndex, sfinfo.channels);
		return 1;
	}

	int numOfSamples = sfinfo.frames/sfinfo.channels;

	double *frameBuff = new double[sfinfo.frames]; // this contains all channels, interleaved [hopefully]
	double *fileBuff = new double[numOfSamples]; // this will contain either one channel or the average of all channels

	int subformat = sfinfo.format & SF_FORMAT_SUBMASK;
	int readcount = sf_read_double(sndfile, frameBuff, sfinfo.frames);

	memset(frameBuff+readcount, 0, sfinfo.frames-readcount); // pad with zeros in case we couldn't read whole file
	memset(fileBuff, 0, numOfSamples); // silence file buffer


	if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE) {
		double	scale ;

		sf_command (sndfile, SFC_CALC_SIGNAL_MAX, &scale, sizeof (scale)) ;
		if (scale < 1e-10)
			scale = 1.0 ;
		else
			scale = 32700.0 / scale ;
		printf("Scale = %f\n", scale);

		for (int f = 0; f < sfinfo.frames; f++)
			frameBuff[f] *= scale;
	}
	sf_close(sndfile);

	// if we are dealing with multi-channel files
	if(sfinfo.channels!=1) {
		// average of all channels
		if(chnIndex==-1) {
			for(int i=0; i<numOfSamples; i++) {
				fileBuff[i] = 0; //VIC this is needed, otherwise a bug manifests...some other audiofiles' samples are mixed with current one, not sure why!
				for(int j=0; j<sfinfo.channels; j++)
					fileBuff[i]+= frameBuff[i*sfinfo.channels + j];
				fileBuff[i] /= sfinfo.channels;
			}
		}
		// take specific channel
		else {
			for(int i=0,j=0; i<numOfSamples; i++, j+=sfinfo.channels)
				fileBuff[i] = frameBuff[j+chnIndex];
		}

	}
	else // actually this extra case would be covered already in the code just above, but this way is faster
		memcpy(fileBuff, frameBuff, sfinfo.frames*sizeof(double));

	samplerate = rate;
	if(samplerate == sfinfo.samplerate)
		init(fileBuff, numOfSamples, periodSize, level, outChannels, outChnOffset); // simply use file as waveform
	else {
		double *interpBuff = NULL;
		int newSampleNum = resample(fileBuff, numOfSamples, filename, interpBuff); // interpolated file buffer allocated and filled inside of here
		// if something went wrong with resampling
		if(newSampleNum <= 0) {
			if(interpBuff==NULL)
				delete[] interpBuff;
			return newSampleNum;
		}

		init(interpBuff, newSampleNum, periodSize, level, outChannels, outChnOffset); // now we can use the file as waveform
		delete[] interpBuff; // clear
	}

	delete[] fileBuff;
	delete[] frameBuff;

	return 0;
}


//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------

int AudioFile::resample(double fileBuff[], int numOfSamples, std::string filename, double *& interpBuff) {
	int newSampleNum = 0;

	if(samplerate > sfinfo.samplerate) {
		if(samplerate % sfinfo.samplerate==0) {
			int mult = samplerate/sfinfo.samplerate;
			newSampleNum = upsample(fileBuff, numOfSamples, mult, interpBuff);
		} else
			newSampleNum = -1;
	}
	else if(samplerate < sfinfo.samplerate) {
		if(sfinfo.samplerate % samplerate==0) {
			int fact = sfinfo.samplerate/samplerate;
			newSampleNum = decimate(fileBuff, numOfSamples, fact, interpBuff);
		} else
			newSampleNum = -2;
	}
	else {
		//TODO: more resampling of file:
		// direct downsample
		// up/downsample when file not direct integer multiple or submultiple

		newSampleNum = 0;
	}


	if(newSampleNum <= 0)
		printf("Audiofile %s's sample rate %d can't be converted to audio rate %d\n", filename.c_str(), sfinfo.samplerate, samplerate);

	return newSampleNum;
}

int AudioFile::upsample(double fileBuff[], int numOfSamples, int mul, double *& interpBuff) {
	int newSampleNum = numOfSamples*mul;

	interpBuff = new double[newSampleNum+1]; // +1 for silent frame and frameNum has been properly sized in resample()
	memset(interpBuff, 0, sizeof(double)*(newSampleNum+1)); // all zero, including last silent frame

	// copy file samples, properly spaced...basically we do a zero padding
	for(int i=0; i<numOfSamples; i++)
		interpBuff[i*mul] = fileBuff[i];

	// now we filter with low pass to prevent aliasing
	double a[3] = {1, 0, 0};
	double b[3] = {0, 0, 0};
	double normCutFreq = 0.5; // frequency = half of file sample rate [samples can't contain freqs higher than that]
	computeLowPass(normCutFreq, a, b);

	// prepare previous i/o
	double x0 = 0;
	double x1 = 0;
	double x2 = 0;
	double y1 = 0;
	double y2 = 0;

	// filter
	for(int i=0; i<newSampleNum; i++) {
		x0 = interpBuff[i];
		interpBuff[i] = b[0]*x0 + b[1]*x1 + b[2]*x2 - a[1]*y1 - a[2]*y2;

		// prepare previous i/o
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = interpBuff[i];
	}

	return newSampleNum;
}

int AudioFile::decimate(double fileBuff[], int numOfSamples, int fact, double *& interpBuff) {
	int newSampleNum = 1 + numOfSamples/fact;

	interpBuff = new double[newSampleNum+1]; // +1 for silent frame and frameNum has been properly sized in resample()
	memset(interpBuff, 0, sizeof(double)*(newSampleNum+1)); // all zero, including last silent frame

	// we filter original file with low pass to prevent aliasing
	double a[3] = {1, 0, 0};
	double b[3] = {0, 0, 0};
	double cutFreq = 0.5; // frequency = half of the sample rate [samples can't contain freqs higher than that]
	computeLowPass(cutFreq, a, b);

	// prepare previous i/o
	double x0 = 0;
	double x1 = 0;
	double x2 = 0;
	double y1 = 0;
	double y2 = 0;

	// filter
	for(long int i=0; i<sfinfo.frames; i++) {
		x0 = fileBuff[i];
		fileBuff[i] = b[0]*x0 + b[1]*x1 + b[2]*x2 - a[1]*y1 - a[2]*y2;

		// prepare previous i/o
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = fileBuff[i];
	}

	// then we downsample
	// copying file samples, properly spaced
	for(int i=0; i<newSampleNum; i++)
		interpBuff[i] = fileBuff[i*fact];

	return newSampleNum;

	//TODO test this! done definitely on a rush...need a card that does 22050 or a file that is 88200 Hz
}

void AudioFile::computeLowPass(double normCutFreq, double *a, double *b) {
	// second order low pass butterworth filter
	// taken from Bela IIR filter example
	double normCutOmega = 2*M_PI*normCutFreq; // normalized cut angular velocity
	double denom = 4+2*sqrt(2.0)*normCutOmega+normCutOmega*normCutOmega;
	b[0] = normCutOmega*normCutOmega/denom;
	b[1] = 2*b[0];
	b[2] = b[0];
	a[1] = (2*normCutOmega*normCutOmega-8)/denom;
	a[2] = (normCutOmega*normCutOmega+4-2*sqrt(2)*normCutOmega)/denom;
}
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------





//----------------------------------------------------------------------------------------------------------------------------
// Wavetable
//----------------------------------------------------------------------------------------------------------------------------
Wavetable::Wavetable() : AudioFile(adv_loop_) {
	interpolation = interp_no_;
	currentPos = -1;
	step = 0;
	waveFrameNum = 0;
	computeSample = NULL;
}

Wavetable::Wavetable(interpType interp) : AudioFile(adv_loop_) {
	interpolation = interp;
	currentPos = -1;
	step = 0;
	waveFrameNum = 0;
	computeSample = NULL;
}

void Wavetable::init(double *samples, int len, unsigned int rate, unsigned int periodSize, double level, unsigned short outChannels, unsigned short outChnOffset) {
	samplerate = rate;

	init(samples, len, periodSize, level, outChannels, outChnOffset);
}


/*
double *Wavetable::getBuffer(int numOfSamples) {
	for(int i=0; i<numOfSamples; i++)
		framebuffer[out_chn_offset][i] = getSample();
	return framebuffer[out_chn_offset];
}
*/

double **Wavetable::getFrameBuffer(int numOfSamples) {
	for(int i=0; i<numOfSamples; i++)
		framebuffer[out_chn_offset][i] = getSample(); // methods referred to by getSample() are all inline

	memset(framebuffer[out_chn_offset]+numOfSamples, 0, (period_size-numOfSamples)*sizeof(double)); // reset part of buffer that has been potentially left untouched

	MultichannelOutUtils::cloneFrameChannels(numOfSamples);

	return framebuffer;
}

//----------------------------------------------------------------------------------------------------------------------------
// protected methods
//----------------------------------------------------------------------------------------------------------------------------

void Wavetable::init(double *samples, int len, unsigned int periodSize, double level, unsigned short outChannels, unsigned short outChnOffset) {
	waveFrameNum = len;

	double *buff;
	if(interpolation == interp_lin_) {
		buff = new double[len+1]; // need to add 1 guard frames at the beginning
		memcpy(buff, samples, len*sizeof(double)); // copy with offset of one element, for guard frame
		len = len+1; // new len! -> this will become frameNum
		buff[len-1] = samples[0]; // guard frame is just a copy of the first original samples

		AudioFile::init(buff, len, periodSize, level, outChannels, outChnOffset);
		delete buff;
	}
	if(interpolation == interp_cubic_) {
		buff = new double[len+3]; // need to add 3 guard frames, one at the beginning, the other two at the end
		memcpy(&buff[1], samples, len*sizeof(double)); // copy with offset of one element, for first guard frame
		len = len+3; // new len! -> this will become frameNum
		buff[0] = samples[len-1]; // first guard frame: last of the original samples
		buff[len-2] = samples[0]; // second guard frame: first of the original samples
		buff[len-1] = samples[1]; // third guard frame: second of the original samples

		AudioFile::init(buff, len, periodSize, level, outChannels, outChnOffset);
		delete buff;
	}
	else
		AudioFile::init(samples, len, periodSize, level, outChannels, outChnOffset);
		//buff = samples; // no interpolation case -> nothing to change on samples passed



	currentPos = 0;

	setFrequency(440); // default 440 Hz -> sets the step
}

void Wavetable::setAdvanceSampleMethod() {
	Waveform::setAdvanceSampleMethod();
	getBufferMethod = NULL; // cos this is not used in this class!

	// alternatively, instead of calling Waveform::setAdvanceSampleMethod() we could have done this:
	//advanceSample = reinterpret_cast<void(Waveform::*)()>(&Wavetable::advanceSampleOneShot);

	switch(interpolation) {
		case interp_no_:
			computeSample = &Wavetable::computeSampleInterpNone;
			break;
		case interp_lin_:
			computeSample = &Wavetable::computeSampleInterpLin;
			break;
		case interp_cubic_:
			computeSample = &Wavetable::computeSampleInterpCubic;
			break;
		default:
			computeSample = &Wavetable::computeSampleInterpNone;
			break;
	}
}



//----------------------------------------------------------------------------------------------------------------------------
// WavetableOsc
//----------------------------------------------------------------------------------------------------------------------------
WavetableOsc::WavetableOsc() : Wavetable() {

}

WavetableOsc::WavetableOsc(interpType interp) : Wavetable(interp) {

}

void WavetableOsc::init(oscillator_type type, unsigned int rate, unsigned int periodSize, double level,
						unsigned int numOfSamples, unsigned short outChannels, unsigned short outChnOffset) {
	double *buff = new double[numOfSamples];

	switch(type) {
			case osc_sin_:
				for(unsigned int i=0; i<numOfSamples; i++)
					buff[i] =sin(2*M_PI * i/double(numOfSamples-1));
				break;
			case osc_square_:
				for(unsigned int i=0; i<numOfSamples/2; i++)
					buff[i] = 1;
				for(unsigned int i=numOfSamples/2; i<numOfSamples; i++)
					buff[i] = 0;
				break;
			case osc_tri_:
				for(unsigned int i=0; i<numOfSamples/2; i++)
					buff[i] = i/double(numOfSamples/2);
				for(unsigned int i=numOfSamples/2; i<numOfSamples; i++)
					buff[i] = (numOfSamples-1-i)/double(numOfSamples/2);
				break;
			case osc_saw_:
				for(unsigned int i=0; i<numOfSamples; i++)
					buff[i] = i/double(numOfSamples-1);
				break;
			case osc_whiteNoise_:
				for(unsigned int i=0; i<numOfSamples; i++)
					buff[i] = 2*( (double) rand()/ ((double)RAND_MAX) ) - 1;
				break;
			case osc_impTrain_:
				memset(buff, 0, numOfSamples*sizeof(double));
				buff[0] = 1;
				break;
			case osc_const_:
				for(unsigned int i=0; i<numOfSamples; i++)
					buff[i] = 1;
				break;
			/*case osc_w_:
				getSampleMethod = &Oscillator::getWSample;
				break;*/
			default:
				for(unsigned int i=0; i<numOfSamples; i++)
					buff[i] =sin(2*M_PI * i/double(numOfSamples-1));
				printf("WaveteableOsc type %d not recognized! Falling back toi sin\n", type);
				break;
	}

	Wavetable::init(buff, numOfSamples, rate, periodSize, level, outChannels, outChnOffset);

	delete buff;
}



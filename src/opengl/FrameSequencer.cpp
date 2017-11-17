/*
 * PixelDrawer.cpp
 *
 *  Created on: 2016-09-06
 *      Author: vic
 */

#include "opengl/PixelDrawer.h"

#include <dirent.h>  // to handle files in dirs
#include <algorithm> // sort
#include <cstring>   // strcmp, memcpy
#include <fstream>   // read/write to file ifstream/ofstream

FrameSequencer::FrameSequencer() {
	playbackMode    = playback_oneShot;
	frameInc        = 1;
	isLoaded  	    = false;
	isPlaying 	    = false;
	currentFrameNum = -1;
	exciteValue     = -1;

	frmSeq.frameNum      = -1;
	frmSeq.frameDuration = -1;
	frmSeq.frames        = NULL;
	frmSeq.nameAndPath   = "";
}

FrameSequencer::~FrameSequencer() {
	deleteSequence();
}

int FrameSequencer::compute(string foldername, int frameW, int frameH, float frameDur, float exValue) {
	vector<string> files;
	int fileNum = PixelDrawer::getFrameFileList(foldername, files, true);

	// just in case...
	if(fileNum<1) {
		printf("No .frm files in %s\n", foldername.c_str());
		return 1;
	}

	int frameSize = frameW*frameH; // size of each frame

	exciteValue = exValue; // to distinguish excitation cells

	// init frame sequence
	frmSeq.frameNum      = fileNum;
	frmSeq.frameDuration = frameDur;
	frmSeq.frames        = new frame [fileNum];
	frmSeq.nameAndPath   = foldername;

	for(int i=0; i<fileNum; i++) {
		// init frames and excitations
		frmSeq.frames[i].pixels = new float[frameSize];

		// fill frames
		if(!PixelDrawer::reader(foldername+"/"+files[i], &frmSeq.frames[i]))
			return 2;
		if( (frmSeq.frames[i].width != frameW) || (frmSeq.frames[i].height != frameH) ) {
			printf("Size mismatch in frame %d [file \"%s\"] of new frame sequence \"%s\"\n", i, files[i].c_str(), foldername.c_str());
			printf("Read frame size is %d x %d, expected size was %d x %d\n", frameW, frameH, frmSeq.frames[i].width, frmSeq.frames[i].height);
			return 3;
		}
	}

	isLoaded = true;

	return 0;
}

bool FrameSequencer::dumpFrameSequence() {
	if(!isLoaded) {
		printf("No frame sequence to dump /: \n");
		return false;
	}

	string name = frmSeq.nameAndPath;
	int len = name.length();
	if( (name.c_str()[len-4] != '.') && (name.c_str()[len-3] != 'f') && (name.c_str()[len-2] != 's') && (name.c_str()[len-1] != 'q') )
		name += ".fsq"; // add extension to name if it is folder
	ofstream dumpfile;
	dumpfile.open (name.c_str(), ios::out | ios::binary);

	// dump
	if (dumpfile.is_open()) {
		// second parameter is equivalent to number of char in the memory block [i knooow it's a bit redundant!]
		// had to split into different write commands because of dyamically allocated struct members
		dumpfile.write((char *) &frmSeq.frameNum, sizeof(int)/sizeof(char));
		dumpfile.write((char *) &frmSeq.frameDuration, sizeof(float)/sizeof(char));
		int size = -1;
		// cycle each frame
		for(int i=0; i<frmSeq.frameNum; i++) {
			dumpfile.write((char *) &frmSeq.frames[i].width, sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frmSeq.frames[i].height, sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frmSeq.frames[i].numOfExciteCells, sizeof(int)/sizeof(char));
			size = frmSeq.frames[i].width*frmSeq.frames[i].height;
			dumpfile.write((char *) frmSeq.frames[i].pixels, size*sizeof(float)/sizeof(char));
		}
		// we leave aside name, cos we'll get it directly from the file when we read
		dumpfile.close();

		return true;
	}
	return false;
}

bool FrameSequencer::loadFrameSequence(string filename) {
	ifstream readfile;
	readfile.open (filename.c_str(),  ios::in | ios::binary | ios::ate);

	if(readfile.is_open()) {
		deleteSequence();

		readfile.seekg (0, ios::beg);

		readfile.read((char *) &frmSeq.frameNum, sizeof(int)/sizeof(char));
		readfile.read((char *) &frmSeq.frameDuration, sizeof(float)/sizeof(char));

		frmSeq.frames = new frame [frmSeq.frameNum];
		int size = -1;
		// cycle each frame
		for(int i=0; i<frmSeq.frameNum; i++) {
			readfile.read((char *) &frmSeq.frames[i].width, sizeof(int)/sizeof(char));
			readfile.read((char *) &frmSeq.frames[i].height, sizeof(int)/sizeof(char));
			readfile.read((char *) &frmSeq.frames[i].numOfExciteCells, sizeof(int)/sizeof(char));

			size = frmSeq.frames[i].width*frmSeq.frames[i].height;
			frmSeq.frames[i].pixels = new float[size];
			readfile.read((char *) frmSeq.frames[i].pixels, size*sizeof(float)/sizeof(char));
		}
		readfile.close();

		frmSeq.nameAndPath = filename; // it always has extension

		isLoaded = true;

		return true;
	}
	else {
		printf("Can't open file \"%s\"\n", filename.c_str());
		return false;
	}
}


frame *FrameSequencer::getNextFrame() {
	currentFrameNum +=frameInc;

	if( (currentFrameNum < frmSeq.frameNum) && (currentFrameNum > -1) ) {
		return &frmSeq.frames[currentFrameNum];
	}
	else if( (frameInc==1) && (currentFrameNum >= frmSeq.frameNum) ) {
		if(playbackMode == playback_oneShot)
			return NULL;
		else if(playbackMode == playback_loop) {
			currentFrameNum = 0;
			return &frmSeq.frames[currentFrameNum];
		}
		else /*if(playbackMode == playback_backAndForth)*/ {
			frameInc = -1;
			currentFrameNum = frmSeq.frameNum-1;
			return &frmSeq.frames[currentFrameNum];
		}
	}
	else if( (frameInc==-1) && (currentFrameNum <= -1) ) {
		if(playbackMode == playback_oneShot)
			return NULL;
		else if(playbackMode == playback_loop) {
			currentFrameNum = 0;
			return &frmSeq.frames[currentFrameNum];
		}
		else /*if(playbackMode == playback_backAndForth)*/ {
			frameInc = 1;
			currentFrameNum = 0;
			return &frmSeq.frames[currentFrameNum];
		}
	}
	else
		return NULL;
}

//------------------------------------------------------------------
// private
//------------------------------------------------------------------
void FrameSequencer::deleteSequence(){
	if(isLoaded) {
		for(int i=0; i<frmSeq.frameNum; i++) {
			delete[] frmSeq.frames[i].pixels;
		}
		delete[] frmSeq.frames;
	}
}


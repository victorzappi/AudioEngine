/*
 * PixelDrawer.h
 *
 *  Created on: 2016-04-09
 *      Author: vic
 */

#ifndef PIXELDRAWER_H_
#define PIXELDRAWER_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library
#include <vector>
#include <cstring>   // strcmp, memcpy


#include "Texture2D.h"
#include "PixelBufferObject.h"
#include "Shader.h"


//TODO make it compliant with HyperPixelDrawer and then swap it!

class PixelDrawer;


struct frame {
	int width;		 	  // width of all frames
	int height;	 	   	  // height of all frames
	float *pixels;        // frames' pixels type [frameNum][width*height]
	int numOfExciteCells; // num of excitation cells per each frame, cos they will have to be rendered separately [frameNum]
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
enum playback_mode {playback_oneShot, playback_loop, playback_backAndForth};

class FrameSequencer {

struct frameSequence {
	int frameNum;		 // total num of frames
	float frameDuration; // transition time between frames
	frame *frames;       // frames' pixels type [frameNum][frameWidth*frameHeight]
	string nameAndPath;	 // name of the sequence [comes from the files we read, that's why there is a path]
};

public:
	FrameSequencer();
	~FrameSequencer();
	int compute(string foldername, int frameW, int frameH, float frameDur, float exValue); // this creates a frameSequence
	bool dumpFrameSequence(); // this dumps the current frameSequence to a bin file [.fsq]
	bool loadFrameSequence(string filename); // this loads a frameSequence from a frameSequence bin file [.fsq]
	frame *play();
	void stop();
	bool getIsPlaying();
	float getFrameDuration();
	bool setFrameDuration(float dur);
	void setPlayBackMode(playback_mode mode);
	frame *getNextFrame();
private:
	void deleteSequence();
	playback_mode playbackMode;
	int frameInc;
	bool isLoaded;
	bool isPlaying;
	int currentFrameNum;  // current frame in sequence
	frameSequence frmSeq; // this is written and read in one shot to and from a binary file
	float exciteValue;
};

inline void FrameSequencer::stop() {
	isPlaying= false;
}
inline bool FrameSequencer::getIsPlaying() {
	return isPlaying;
}

inline void FrameSequencer::setPlayBackMode(playback_mode mode) {
	playbackMode = mode;
}

inline float FrameSequencer::getFrameDuration() {
	if(isLoaded)
		return frmSeq.frameDuration;
	else
		return -1;
}

inline bool FrameSequencer::setFrameDuration(float dur) {
	if(!isLoaded){
		printf("Can't set frame sequence duration, simply because there is no frame sequence loaded yet...\n");
		return false;
	}

	frmSeq.frameDuration = dur;
	return true;
}

inline frame *FrameSequencer::play() {
	if(isLoaded) {
		currentFrameNum = -1;
		isPlaying = true;
		return getNextFrame();
	}
	else {
		printf("Can't play frame sequence, nothing loaded yet...you jackass\n");
		return NULL;
	}
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------




class PixelDrawer {
	friend class FrameSequencer; // to give access to private static reader
public:
	PixelDrawer(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
				float frameDuration, int audioRate, int exCType, int airCellType, int wallCType);
	virtual ~PixelDrawer();
	void clear();
	virtual int init(GLint active_unitA, GLint active_unitB, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]);
	virtual void update(int state, bool excitationChange)=0;
	int updateFrame(int state, int &exciteNum);
	virtual void reset()=0;
	GLfloat getCellType(int x, int y);
	int setCellType(GLfloat type, int x, int y);
	int resetCellType(int x, int y);
	int getWidth();
	int getHeight();
	void setAllWallBetas(float wallBeta);
	int dumpFrame(int numExCells); //TODO dumpers need to be more sophisticated in the way file names are passed and returned
	bool dumpFrameSequence();
	int openFrame(string filename);
	int openFrame(frame *f);
	int computeFrame(string filename, frame &f);
	int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);
	bool loadFrame(frame *f);

protected:
	bool initShaders(string shaderLocation);
	void setUpAllWallBetas();
	static int dumper(frame *f);
	static bool reader(string filename, frame *f);
	static int getFrameFileList(string dirname, vector<string> &files, bool ordered=false);


	string shaders[2];

	int gl_width;
	int gl_height;
	int numOfStates;
	int numOfResetQuads;
	GLfloat *framePixelsA;
	GLfloat *framePixelsB;
	GLfloat *currentFramePixels;
	GLfloat *clearFramePixels;
	int frameExciteNum;
	Texture2D textureA;
	Texture2D textureB;
	PboWriter *currentPbo;
	PboWriter pboA;
	PboWriter pboB;
	GLint operation_loc;
	float crossfade;
	GLint crossfade_loc;
	int audio_rate;
	float crossIncrement; // this is the amount we move from one frame to another one per each audio sample!
	int deck;
	GLint deck_loc;
	Shader shaderDraw;	// shader that updates texture framePixels
	Shader &shaderFbo;	// main fbo shader program [for FDTD]
	int excitationCellType;
	int airCellType;
	int wallCellType;
	bool removeFormerExciteCells;

	int drawArraysValues[13][2]; // 4 drawing quads + 4 clean up excitation quads + 4 clean up excitation follow up quads + 1 audio quad

	int **drawArraysMainQuads;
	int **drawArraysResetQuads;

	// to change betas all walls in one shot
	float allWallBetas;
	bool updateAllWallBetas;

	FrameSequencer frameSequencer;

};

inline int PixelDrawer::getWidth() {
	return gl_width;
}

inline int PixelDrawer::getHeight() {
	return gl_height;
}


inline void PixelDrawer::setAllWallBetas(float wallBeta) {
	allWallBetas = wallBeta;
	updateAllWallBetas = true;
}

inline int PixelDrawer::dumpFrame(int numExCells) {
	frame *f = new frame();
	f->width  = gl_width;
	f->height = gl_height;
	f->numOfExciteCells = numExCells;
	f->pixels = new float [gl_width*gl_height];
	memcpy(f->pixels, currentFramePixels, gl_width*gl_height*sizeof(float));

	int retval =  dumper(f);

	delete[] f;

	return retval;
}

// dumps only if we have one loaded
inline bool PixelDrawer::dumpFrameSequence() {
	return frameSequencer.dumpFrameSequence();
}

inline int PixelDrawer::openFrame(string filename) {
	frameSequencer.stop(); // just in case

	// create empty frame
	frame *f  = new frame();
	f->pixels = new float[gl_width*gl_height];
	// fill it from file
	if(!reader(filename, f))
		return 1;
	// use it [take it and use it, like Don Porco]
	if(!loadFrame(f))
		return 2;

	return 0;
}

inline int PixelDrawer::openFrame(frame *f) {
	frameSequencer.stop(); // just in case

	if(!loadFrame(f))
		return 2;

	return 0;
}

inline int PixelDrawer::computeFrame(string filename, frame &f) {
	f.pixels = new float[gl_width*gl_height];
	// fill it from file
	if(!reader(filename, &f))
		return 1;
	return 0;
}


inline int PixelDrawer::computeFrameSequence(string foldername) {
	return frameSequencer.compute(foldername, gl_width, gl_height, 1.0/(audio_rate*crossIncrement), excitationCellType);
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------


class PDrawerExplicitFDTD : public PixelDrawer {
public:
	PDrawerExplicitFDTD(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
					float frameDuration, int audioRate, int exCType, int airCType, int wallCType);
	void update(int state, bool excitationChange);
	void reset();
};
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------


class PDrawerImplicitFDTD : public PixelDrawer {
public:
	PDrawerImplicitFDTD(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
					float frameDuration, int audioRate, int exCType, int airCType, int wallCType);
	void update(int state, bool excitationChange);
	void reset();
};
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
#endif /* PIXELDRAWER_H_ */

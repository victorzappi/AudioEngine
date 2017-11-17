/*
 * FDTD.h
 *
 *  Created on: 2016-11-01
 *      Author: Victor Zappi
 */

#ifndef FDTD_H_
#define FDTD_H_

#include "TextureFilter.h"
#include "PixelDrawer.h"

#define ISOLATION_LAYER 1 // texture area [1 row] that isolates frame from audio [to avoid interferences]

// generic value for sound in air...
#define C_AIR   340     // DA SPEED OF SOUND!

#define FRAME_DURATION /*0.009*/0.005//0.00022675736 //promusica/pif/generic

//#define TIME_RT_FBO
//#define TIME_RT_RENDER
#if defined(TIME_RT_FBO) != defined(TIME_RT_RENDER)
#include <sys/time.h> // for time test
#endif


class FDTD : public TextureFilter {
public:
	enum quad_type { quad_0, quad_1,
					 quad_drawer_0, quad_drawer_1,
					 quad_audio,
					 quad_numOfQuads
					};
	enum cell_type {cell_wall, cell_air, cell_excitation, cell_pml0, cell_pml1, cell_pml2, cell_pml3, cell_pml4, cell_pml5, cell_dynamic, cell_dead, cell_numOfTypes};

	FDTD();
	~FDTD();
	int init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			         float ***domain, int audio_rate, int rate_mul, int periodSize, float mag=1, int *listCoord=NULL);
	int init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize,
					 float mag=1, int *listCoord=NULL);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);
	GLFWwindow *getWindow(int *size);
	float *getBufferFloat(int numSamples, float *inputBuffer);
	void getListenerPosition(int &x, int &y);
	int shiftListenerV(int delta = 1);
	int shiftListenerH(int delta = 1);
	int setMaxPressure(float maxP);
	int fromRawWindowToGlWindow(float *coords);
	int fromRawWindowToDomain(int *coords);
	int fromDomainToGlWindow(int *coords);
	int fromDomainToTexture(int *coords);
	int fromTextureToDomain(int *coords);
	float getDS();
	float setDS(float ds);
	virtual int setListenerPosition(int x, int y);
	virtual int setPhysicalParameters(void *params);
	int setCell(float type, int x, int y);
	int resetCell(int x, int y);
	virtual int clearDomain();
	int reset();
	void setAllWallBetas(float wallBeta);
	void setSlowMotion(bool slowMo);
	int setMousePosition(int x, int y);
	int saveFrame();
	int saveFrameSequence();
	int openFrame(string filename);
	int openFrame(frame *f);
	int computeFrame(string filename, frame &f);
	int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);
	int loadFrame(frame *f);

	// some utilities
	static float getPixelAlphaValue(int type, int PMLlayer=0, int dynamicIndex=0, int numOfPMLlayers=0);
	static void fillTexturePixel(int x, int y, float value, float ***pixels);
	static void fillTextureRow(int index, int start, int end, float value, float ***pixels);
	static void fillTextureColumn(int index, int start, int end, float value, float ***pixels);
	static float estimateDs(float rate, int rateMul, float c);
protected:
	void preInit(string shaderLocation, int *listCoord=NULL);
	virtual void postInit();
	// these methods define the specific behavior of this filter
	void initPhysicalParameters();
	virtual void allocateResources();
	void initImage(int domainWidth, int domainHeigth);
	void initVao(GLuint vbo, GLuint shader_program_fbo, GLuint shader_program_render, GLuint &vao);
	virtual int fillVertices();
	void initFrame(int *excitationCoord, int *excitationDimensions, float ***domain);
	void fillImage(float ***pixels);
	int initAdditionalUniforms();
	void resetAudioRow();
	int computeListenerPos(int x, int y);
	virtual void computeNextListenerFrag();
	virtual int computeListenerFrag(int x, int y);
	virtual void computeSamples(int numSamples, float *inputBuffer);
	virtual void renderWindow();
	virtual void renderWindowInner();
	virtual void getSamples(int numSamples);
	virtual void updatePixelDrawer();
	virtual void updatePixelDrawerFrame();
	virtual void updateUniforms();
	virtual void updateUniformsRender();
	virtual void updateCriticalUniforms();
	virtual void setUpListenerRender();
	void initPixelReader();
	virtual int initPixelDrawer();
	void setUpMaxPressure();
	virtual void setUpPhysicalParams();
	void computeDeltaS();
	void setUpExCellNum();
	void setUpExCellNumRender();
	virtual int computeMouseFrag(int x, int y);
	void setUpMouseRender();
	void readAllTextureDebug();
	virtual bool computePMLcoeff(float * &coeff);

	// utils
	static float computeDs(float _dt, float _c);
	static float getPixelBlueValue(int type);


	// VBO/VAO params
	int numOfVerticesPerQuad;
	int numOfAttributes;

	bool updateListener;

	// PML and dead cell layers
	int numOfPML;
	//GLint typeValues_loc[cell_numOfTypes];
	GLint *typeValues_loc;
	int numOfDeadCellLayers;

	// pressure normalization
	float maxPressure;
	GLint maxPressure_loc;
	GLint maxPressure_loc_render;
	bool updateMaxPress;

	// physical params and coefficients
	bool updatePhysParams;
	float ds;
	float c;
	float minDs;
	GLint c2_dt2_invDs2_loc;

	// excitation
	int excitationCellNum;
	GLint inv_exCellNum_loc;
	GLint exCellNum_loc_render;
	bool updateExCellNum;

	// pixel drawer
	PixelDrawer *pixelDrawer;
	float ***startFramePixels; // where to save clear domain including PML and dead layers
	bool updatePixels;
	bool updateFramePixels;
	bool resetPixels;
	float frameDuration; // duration of frame crossfade

	// slow motion
	bool slowMotion;
	float slowMo_stretch;
	int slowMo_sleep_ns;
	float slowMo_time;
	unsigned int slowMo_step;

	// mouse pos
	float mouseFragCoord[2];
	GLint mouseCoord_loc_render;
	bool updateMouseCoord;

	// misc
	GLint quad_loc;
	bool render;
	//bool updateListenerRender; // to cope with crossfade
	int domainShift[2];
	int textureShift[2];
	int singleRowAudioCapacity;
	int audioRowsUsed;

#if defined(TIME_RT_FBO) != defined(TIME_RT_RENDER)
	// time test
	int elapsedTime;
	timeval start_, end_;
#endif
};

inline GLFWwindow *FDTD::getWindow(int *size) {
	if(pixelDrawer == NULL)
		return NULL;

	return TextureFilter::getWindow(size);
}

inline int FDTD::fromRawWindowToGlWindow(float *coords) {
	// XD
	if( coords[0]<0 || coords[1]<0)
		return -1;

	// simply cope with magnification
	coords[0]/=magnifier;
	coords[1]/=magnifier;

	return 0;
}

//----------------------------------------------------------
// private
//----------------------------------------------------------
inline void FDTD::updatePixelDrawerFrame() {
	if(updateFramePixels) {
		int excitationCells = -1;
		int retval = pixelDrawer->updateFrame(state, excitationCells);
		// still has to finish
		if (retval == 0)
			updateFramePixels = true;
		// still needs to run but update excitations cells
		else if (retval == 1) {
			updateFramePixels = true;
			excitationCellNum = excitationCells;
			setUpExCellNum(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
		// it's finished, simply update number of excitation cells
		else if (retval == 2){
			updateFramePixels = false;
			excitationCellNum = excitationCells;
			setUpExCellNum(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
	}
}

inline void FDTD::updateCriticalUniforms() {
	// in the simplest case there are no uniforms that critically need to be updated at every sample,
	// other than the default excitation of course [hardcoded in getBUffer()]
}

inline void FDTD::setUpListenerRender() {
	// we always render quad 0, no matter what state it is, cos we go so fast that we don't care if we're slightly out of date...
	// this makes easier to detect the listener, cos we don't have to cope with multi-state coordinates
	glUniform2f(listenerFragCoord_loc_render, listenerFragCoord[0][0], listenerFragCoord[0][1]); // listener for state 0 holds value for sate numOfStates-1 (:
}

inline void FDTD::setUpExCellNum() {
	if(excitationCellNum > 0)
		glUniform1f(inv_exCellNum_loc, 1.0/excitationCellNum);
	else
		glUniform1f(inv_exCellNum_loc, 1.0); // cos it's a denominator in the shader
}

inline void FDTD::setUpExCellNumRender() {
	if(excitationCellNum > 0)
		glUniform1i(exCellNum_loc_render, excitationCellNum);
	else
		glUniform1i(exCellNum_loc_render, 1); // 1's work better than zeros in general...just in case

	checkError("exCellNum_loc_render");
}

// writes maxPressure into shaders' uniforms
inline void FDTD::setUpMaxPressure() {
	glUniform1f(maxPressure_loc, maxPressure);
	//checkError("glUniform1f maxPressure FBO");

	shaderRender.use();
	glUniform1f(maxPressure_loc_render, maxPressure);
	//checkError("glUniform1f maxPressure");

	shaderFbo.use();
}

inline void FDTD::setUpPhysicalParams() {
	float c2_dt2_invDs2 = c*c*dt*dt/(ds*ds);
	glUniform1f(c2_dt2_invDs2_loc, c2_dt2_invDs2);
	//checkError("glUniform1f c2_dt2_invDs2");
}

inline void FDTD::setUpMouseRender() {
	glUniform2f(mouseCoord_loc_render, mouseFragCoord[0], mouseFragCoord[1]);
}

inline float FDTD::getDS() {
	return ds;
}

inline void FDTD::setSlowMotion(bool slowMo) {
	slowMotion  = slowMo;
	slowMo_time = 0;
	slowMo_step = 0;
}

inline void FDTD::setAllWallBetas(float wallBeta) {
	if(!inited || (pixelDrawer == NULL))
		return;
	pixelDrawer->setAllWallBetas(wallBeta);
	updatePixels = true;
}

inline float FDTD::estimateDs(float rate, int rateMul, float c) {
	return computeDs(1/(rate*rateMul), c);
}

inline int FDTD::saveFrame() {
	if(inited && (pixelDrawer != NULL))
		return pixelDrawer->dumpFrame(excitationCellNum);
	else
		return -1;
}

inline int FDTD::saveFrameSequence() {
	if(inited && (pixelDrawer != NULL))
		if(pixelDrawer->dumpFrameSequence())
			return 0;
		else
			return 1;
	else
		return -1;
}


inline int FDTD::openFrame(string filename) {
	if(inited && (pixelDrawer != NULL)) {
		int retval = pixelDrawer->openFrame(filename);
		if(retval == 0)
			updateFramePixels = true;
		return retval;
	}
	return -1;
}

inline int FDTD::loadFrame(frame *f) {
	if(inited && (pixelDrawer != NULL)) {
		if(pixelDrawer->loadFrame(f)) {
			updateFramePixels = true;
			return 0;
		}
		return 1;
	}
	return -1;
}

inline int FDTD::openFrame(frame *f) {
	if(inited && (pixelDrawer != NULL)) {
		return pixelDrawer->openFrame(f);
	}
	return -1;
}

inline int FDTD::computeFrame(string filename, frame &f){
	if(inited && (pixelDrawer != NULL)) {
		if(&f != NULL)
			return pixelDrawer->computeFrame(filename, f);
		else {
			printf("Frame passed with file \"%s\" is empty\n", filename.c_str());
			return 2;
		}
	}
	return -1;
}

inline int FDTD::openFrameSequence(string filename, bool isBinary) {
	if(inited && (pixelDrawer != NULL)) {
		int retval = pixelDrawer->openFrameSequence(filename, isBinary);
		if(retval == 0)
			updateFramePixels = true;
		return retval;
	}
	return -1;
}

// this compute and loads fram seq but does not play it [use to dump new sequences]
inline int FDTD::computeFrameSequence(string foldername) {
	if(inited && (pixelDrawer != NULL))
		return pixelDrawer->computeFrameSequence(foldername);
	else
		return -1;
}

#endif /* SIMPLEFDTD_H_ */

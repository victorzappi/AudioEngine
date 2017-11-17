/*
 * CoupledFDTD.h
 *
 *  Created on: 2015-09-23
 *      Author: Victor Zappi
 */

#ifndef COUPLEDFDTD_H
#define COUPLEDFDTD_H

//#include "TextureFilter.h"
//#include "PixelDrawer.h"
#include "FDTD.h"

// generic value for sound in air...
#define RHO_AIR 1.22     // DA MEAN DENSITY!

//-------------------------------------------------
// simple struct to pass excitation models
struct excitationModels {
	int num;
	int excitationModel;
	std::string *names;
};

//-------------------------------------------------

class CoupledFDTD : public FDTD {
public:

	enum quad_type { quad_0, quad_1, quad_2, quad_3,
					 quad_excitation_0, quad_excitation_1, quad_excitation_2, quad_excitation_3,
					 quad_excitation_fu_0, quad_excitation_fu_1, quad_excitation_fu_2, quad_excitation_fu_3,
					 quad_drawer_0, quad_drawer_1, quad_drawer_2, quad_drawer_3,
					 quad_audio,
					 quad_numOfQuads
					};

	enum cell_type {cell_wall, cell_air, cell_excitation, cell_pml0, cell_pml1, cell_pml2, cell_pml3, cell_pml4, cell_pml5, cell_dynamic, cell_dead, cell_nopressure, cell_numOfTypes};


	CoupledFDTD();
	~CoupledFDTD();
	int init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions, excitationModels exMods,
			         float ***domain, int audio_rate, int rate_mul, int periodSize, float mag=1, int *listCoord=NULL, float exLpF=22050);
	int init(string shaderLocation, excitationModels exMods, int *domainSize, int audio_rate, int rate_mul, int periodSize,
					 float mag=1, int *listCoord=NULL, float exLpF=22050);
	//int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);
	//GLFWwindow *getWindow(int *size);
	//float *getBuffer(int numSamples, float *inputBuffer);
	//void getListenerPosition(int &x, int &y);
	//int shiftListenerV(int delta = 1);
	//int shiftListenerH(int delta = 1);
	int shiftFeedbackV(int delta = 1);
	int shiftFeedbackH(int delta = 1);
	//int setMaxPressure(float maxP);
//	int fromRawWindowToDomain(int *coords);
//	int fromRawWindowToGlWindow(float *coords);
//	int fromDomainToRawWindow(int *coords);
//	float getDS();
//	float setDS(float ds);
	int setExcitationModel(int index);
	int setExcitationModel(std::string name);
	int getExcitationModelIndex();
	std::string getExcitationModelName();
	int getNumOfExcitationModels();
	int setExcitationDirection(float angle);
	int setListenerPosition(int x, int y);
	int setFeedbackPosition(int x, int y);
	void getFeedbackPosition(int &x, int &y);
	//virtual int setPhysicalParameters(float c, float rho, float mu, float prandtl, float gamma);
	int setPhysicalParameters(void *params);
	//int setCell(float type, int x, int y);
	//int resetCell(int x, int y);
	//int clearDomain();
	//int reset();
	//void setSlowMotion(bool slowMo);
	//int setMousePosition(int x, int y);
	int setExcitationLowPassToggle(int toggle);
//	int saveFrame();
//	int saveFrameSequence();
//	int openFrame(string filename);
//	int openFrame(frame *f);
//	int computeFrame(string filename, frame &f);
//	int openFrameSequence(string filename, bool isBinary=true);
//	int computeFrameSequence(string foldername);
//	int loadFrame(frame *f);

	// some utilities
	static float getPixelAlphaValue(int type, int PMLlayer=0, int dynamicIndex=0, int numOfPMLlayers=0);
	static void fillTexturePixel(int x, int y, float value, float ***pixels);
	static void fillTextureRow(int index, int start, int end, float value, float ***pixels);
	static void fillTextureColumn(int index, int start, int end, float value, float ***pixels);
	static float estimateDs(float rate, int rateMul, float c);
protected:
	void preInit(string shaderLocation, excitationModels exMods, float exLpF, int *listCoord=NULL);
	void postInit();
	// these methods define the specific behavior of this filter
	void initPhysicalParameters();
	void allocateResources();
	void initImage(int domainWidth, int domainHeigth);
	//void initVao(GLuint vbo, GLuint shader_program_fbo, GLuint shader_program_render, GLuint &vao);
	int fillVertices();
	//void initFrame(int *excitationCoord, int *excitationDimensions, float ***domain);
	//void fillImage(float ***pixels);
	virtual int initAdditionalUniforms();
	//void resetAudioRow();
//	int computeListenerPos(int x, int y);
	void computeNextListenerFrag();
	int computeListenerFrag(int x, int y);
	int computeListenerFragCrossfade(int x, int y);
	void computeSamples(int numSamples, float *inputBuffer);
	//virtual void renderWindow();
	void renderWindowInner();
	void getSamples(int numSamples);
	//virtual void updatePixelDrawer();
	//virtual void updatePixelDrawerFrame();
	void updateUniforms();
	void updateUniformsRender();
	void updateCriticalUniforms();
	void setUpListener();
	void setUpListenerRender();
	//void initPbo();
	int initPixelDrawer();
	//void setUpMaxPressure();
	void setUpPhysicalParams();
	void setUpExcitationTypeFbo();
	void setUpExcitationTypeRender();
	void setUpExcitationDirectionFbo();
	void setUpExcitationDirectionRender();
	int computeFeedbackPos(int x, int y);
	void computeNextFeedbackFrag();
	int computeFeedbackFrag(int x, int y);
	virtual void setUpFeedback();
	virtual void setUpFeedbackRender();
	//void computeDeltaS();
	//void setUpExCellNumFbo();
	//void setUpExCellNum();
	int computeMouseFrag(int x, int y);
	//void setUpMouseRender();
	virtual void computeExcitationLPCoeff( float &a1, float &a2, float &b0, float &b1, float &b2);
	virtual void setUpExLpToggle();
	//void readAllTextureDebug();

	// misc simulation params
	//int numOfVerticesPerQuad;
	//int numOfAttributes;

	// to smoothly change listener position
	bool doCrossfade;
	GLint crossfade_loc;
	GLint *listenerAFragCoord_loc;
	GLint *listenerBFragCoord_loc;
	GLint *nextListenFragCoord_loc;

	float **listenerAFragCoord;
	float **listenerBFragCoord;
	float **nextListenerFragCoord;
	float crossfade;
	float crossfadeDelta;
	char currentDeck;
	//bool updateListener;
	bool updateListenerRender; // to cope with crossfade

	// PML
	//int numPMLlayers;
	//GLint typeValues_loc[cell_numOfTypes];

	// pressure normalization
	//float maxPressure;
	//GLint maxPressure_loc;
	//GLint maxPressure_loc_render;
	//bool updateMaxPress;

	// physical params and coefficients
	//bool updatePhysParams;
	//float minDs;
	float rho;
	GLint rho_c2_dt_invDs_loc;
	GLint dt_invDs_invRho_loc;

	// excitation
	GLint excitationModel_loc;
	GLint excitationModel_loc_render;
	excitationModels exModels;
	bool updateExType;
	//int excitationCellNum;
	//GLint inv_exCellNum_loc;
	//GLint exCellNum_loc_render;
	//bool updateExCellNum;
	float excitationDirection[4];
	float excitationAngle;
	GLint exDirection_loc;
	GLint exDirection_loc_render;
	bool updateExDir;
	float **exFragCoord;
	int numOfExQuads;
	int numOfExFolQuads;
	// excitation low pass
	GLint toggle_exLp_loc;
	int toggle_exLp;
	bool updateExLpToggle;
	float exLpFreq;


	// feedback
	GLint *feedbFragCoord_loc;
	GLint feedbFragCoord_loc_render;
	float **feedbFragCoord;
	float feedbackCoord[2];
	bool  updateFeedback;

	// pixel drawer
	//PixelDrawer *pixelDrawer;
	//float ***clearFramePixels; // where to save clear domain including PML and dead layers
	//bool updatePixels;
	//bool updateFramePixels;
	//bool resetPixels;
	//float frameDuration; // duration of frame crossfade

	// slow motion
	//bool slowMotion;
	//float slowMo_stretch;
	//int slowMo_sleep_ns;
	//float slowMo_time;
	//unsigned int slowMo_step;

	// mouse pos
	//float mouseFragCoord[2];
	//GLint mouseCoord_loc_render;
	//bool updateMouseCoord;

	// misc
	//GLint quad_loc;
	//bool render;
	//int domainShift[2];
	//int textureShift[2];
	//int singleRowAudioCapacity;
	//int audioRowsUsed;
};

//VIC this should be useless, since super class does the same, but there is a bug! pixelDrawer is always seen as NULL in FDTD::getWindow(size), even after initialization [and i debugged...it is initialized and different from NULL!]
/*inline GLFWwindow *CoupledFDTD::getWindow(int *size) {
	if(pixelDrawer == NULL)
		return NULL;

	return TextureFilter::getWindow(size);
}*/

/*inline int CoupledFDTD::fromRawWindowToGlWindow(float *coords) {
	// XD
	if( coords[0]<0 || coords[1]<0)
		return -1;

	// simply cope with magnification
	coords[0]/=magnifier;
	coords[1]/=magnifier;

	return 0;
}*/

//----------------------------------------------------------
// private
//----------------------------------------------------------
/*inline void CoupledFDTD::updatePixelDrawerFrame() {
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
			setUpExCellNumFbo(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
		// it's finished, simply update number of excitation cells
		else if (retval == 2){
			updateFramePixels = false;
			excitationCellNum = excitationCells;
			setUpExCellNumFbo(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
	}
}*/

inline void CoupledFDTD::setUpListener() {
	for (int i =0; i<numOfStates; i++)
		glUniform2f(nextListenFragCoord_loc[i], nextListenerFragCoord[i][0], nextListenerFragCoord[i][1]);
}

inline void CoupledFDTD::setUpListenerRender() {
	// we always render quad 0, showing state N, cos we go so fast that we don't care if we're slightly out of date...
	// this makes easier to detect the listener, cos we don't have to cope with multi-state coordinates
	glUniform2f(listenerFragCoord_loc_render, nextListenerFragCoord[0][0], nextListenerFragCoord[0][1]); // listener for state 0 holds value for sate numOfStates-1 (:
}

inline void CoupledFDTD::setUpFeedback() {
	for (int i =0; i<numOfStates; i++)
		glUniform2f(feedbFragCoord_loc[i], feedbFragCoord[i][0], feedbFragCoord[i][1]);
}

inline void CoupledFDTD::setUpFeedbackRender() {
	glUniform2f(feedbFragCoord_loc_render, feedbFragCoord[0][0], feedbFragCoord[0][1]); // feedback for state 0 holds value for sate numOfStates-1 (:
}

/*inline void CoupledFDTD::setUpExCellNumFbo() {
	if(excitationCellNum > 0)
		glUniform1f(inv_exCellNum_loc, 1.0/excitationCellNum);
	else
		glUniform1f(inv_exCellNum_loc, 1.0); // cos it's a denominator in the shader
}

inline void CoupledFDTD::setUpExCellNum() {
	if(excitationCellNum > 0)
		glUniform1i(exCellNum_loc_render, excitationCellNum);
	else
		glUniform1i(exCellNum_loc_render, 1); // 1's work better than zeros in general...just in case
}*/

// writes maxPressure into shaders' uniforms
/*inline void CoupledFDTD::setUpMaxPressure() {
	glUniform1f(maxPressure_loc, maxPressure);
	//checkError("glUniform1f maxPressure FBO");

	shaderRender.use();
	glUniform1f(maxPressure_loc_render, maxPressure);
	//checkError("glUniform1f maxPressure");

	shaderFbo.use();
}*/

inline void CoupledFDTD::setUpPhysicalParams() {
	float rho_c2_dt_invDs = rho*c*c*dt/ds;
	glUniform1f(rho_c2_dt_invDs_loc, rho_c2_dt_invDs);
	//checkError("glUniform3fv rho_c2_dt_invDs");

	float dt_invDs_invRho = dt/(ds*rho);
	glUniform1f(dt_invDs_invRho_loc, dt_invDs_invRho);
	//checkError("glUniform3fv dt_invDs_invRho");
}

inline void CoupledFDTD::setUpExcitationTypeFbo() {
	glUniform1i(excitationModel_loc, exModels.excitationModel);
	//checkError("glUniform1i excitationModel");
}

inline void CoupledFDTD::setUpExcitationTypeRender() {
	glUniform1i(excitationModel_loc_render, exModels.excitationModel);
	//checkError("glUniform1i excitationModel");
}

inline void CoupledFDTD::setUpExcitationDirectionFbo() {
	glUniform4fv(exDirection_loc, 1, excitationDirection);
	//checkError("glUniform4fv excitationDirection");
}

inline void CoupledFDTD::setUpExcitationDirectionRender() {
	glUniform4fv(exDirection_loc_render, 1, excitationDirection);
	//checkError("glUniform4fv excitationDirection");
}

/*inline void CoupledFDTD::setUpMouseRender() {
	glUniform2f(mouseCoord_loc_render, mouseFragCoord[0], mouseFragCoord[1]);
}*/

inline void CoupledFDTD::setUpExLpToggle() {
	glUniform1i(toggle_exLp_loc, toggle_exLp);
	//checkError("glUniform1i toggle_exLp");
}

inline void CoupledFDTD::updateCriticalUniforms() {
	// in the simplest case there are no uniforms that critically need to be updated at every sample,
	// other than the default excitation of course [hardcoded in getBuffer()]
}


/*inline float CoupledFDTD::getDS() {
	return ds;
}*/

inline int CoupledFDTD::getExcitationModelIndex() {
	if(inited)
		return exModels.excitationModel;
	else
		return -1;
}

inline std::string CoupledFDTD::getExcitationModelName() {
	if(inited)
		return exModels.names[exModels.excitationModel];
	else
		return NULL;
}

inline int CoupledFDTD::getNumOfExcitationModels() {
	if(inited)
		return exModels.num;
	else
		return -1;
}


/*inline void CoupledFDTD::setSlowMotion(bool slowMo) {
	slowMotion  = slowMo;
	slowMo_time = 0;
	slowMo_step = 0;
}*/

// utility
// to make these static methods more visible
inline void CoupledFDTD::fillTexturePixel(int x, int y, float value, float ***pixels) {
	 FDTD::fillTexturePixel(x, y, value, pixels);
}


inline void CoupledFDTD::fillTextureRow(int index, int start, int end, float value, float ***pixels) {
	FDTD::fillTextureRow(index, start, end, value, pixels);
}

inline void CoupledFDTD::fillTextureColumn(int index, int start, int end, float value, float ***pixels) {
	FDTD::fillTextureColumn(index, start, end, value, pixels);
}


inline float CoupledFDTD::estimateDs(float rate, int rateMul, float c) {
	return FDTD::estimateDs(rate, rateMul, c);
}

/*inline int CoupledFDTD::saveFrame() {
	if(inited && (pixelDrawer != NULL))
		return pixelDrawer->dumpFrame(excitationCellNum);
	else
		return -1;
}

inline int CoupledFDTD::saveFrameSequence() {
	if(inited && (pixelDrawer != NULL))
		if(pixelDrawer->dumpFrameSequence())
			return 0;
		else
			return 1;
	else
		return -1;
}


inline int CoupledFDTD::openFrame(string filename) {
	if(inited && (pixelDrawer != NULL)) {
		int retval = pixelDrawer->openFrame(filename);
		if(retval == 0)
			updateFramePixels = true;
		return retval;
	}
	return -1;
}

inline int CoupledFDTD::loadFrame(frame *f) {
	if(inited && (pixelDrawer != NULL)) {
		if(pixelDrawer->loadFrame(f)) {
			updateFramePixels = true;
			return 0;
		}
		return 1;
	}
	return -1;
}

inline int CoupledFDTD::openFrame(frame *f) {
	if(inited && (pixelDrawer != NULL)) {
		return pixelDrawer->openFrame(f);
	}
	return -1;
}

inline int CoupledFDTD::computeFrame(string filename, frame &f){
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

inline int CoupledFDTD::openFrameSequence(string filename, bool isBinary) {
	if(inited && (pixelDrawer != NULL)) {
		int retval = pixelDrawer->openFrameSequence(filename, isBinary);
		if(retval == 0)
			updateFramePixels = true;
		return retval;
	}
	return -1;
}

// this compute and loads fram seq but does not play it [use to dump new sequences]
inline int CoupledFDTD::computeFrameSequence(string foldername) {
	if(inited && (pixelDrawer != NULL))
		return pixelDrawer->computeFrameSequence(foldername);
	else
		return -1;
}*/


#endif /* COUPLEDFDTD_H */

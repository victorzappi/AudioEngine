/*
 * CoupledFDTD.cpp
 *
 *  Created on: 2015-09-23
 *      Author: Victor Zappi
 */

#include "opengl/CoupledFDTD.h"
#include "opengl/Quad.h"
#include "priority_utils.h"

#include <iostream>
#include <stdio.h>
#include <assert.h>

#include <cstring> // memcopy
#include <math.h>  // ceil [not to be confused with Celi] and isfinite()---->with cmath the latter does not work!
//#include <stdarg.h>

#include <unistd.h> // usleep, for debugging



using namespace std;

	//-----------------------------------------------------------
	// vertices and texture coordinates
	//-----------------------------------------------------------
	/*
	 *
	 * quad0 copies texture on quad2
	 * quad1 copies texture on quad0
	 * quad2 copies texture on quad1
	 *
	 * quad3 is not used
	 *  ___________ ___________
	 * |		   |		   |
	 * | quad1     | quad2     |
	 * |___________|___________|
	 * |		   |		   |
	 * | quad0     | [quad3]   |
	 * |___________|___________|
	 * |_______|_|_|_|_|_|_|_|_| } lower extension [audio rows+excitation+excitation followup]  ---> this structure might change if we need bigger audio buffer, have a look down here in fillVertices()
	 *	^audio^	 ^ side quads ^
	 *	 [side quads are excitation+excitation followup]
	 *
	 *
	 * here is how states are related to quads
	 * state0
	 *  ___________ ___________
	 * |		   |		   |
	 * | quad1,N-1 | quad2,N   |
	 * |___________|___________|
	 * |		   |		   |
	 * | quad0,N+1 | quad3,-   |
	 * |___________|___________|
	 *
	 *
	 * state1
	 *  ___________ ___________
	 * |		   |		   |
	 * | quad1,N+1 | quad2,N-1 |
	 * |___________|___________|
	 * |		   |		   |
	 * | quad0,N   | quad3,-   |
	 * |___________|___________|
	 *
	 *
	 * state2
	 *  ___________ ___________
	 * |		   |		   |
	 * | quad1,N   | quad2,N+1 |
	 * |___________|___________|
	 * |		   |		   |
	 * | quad0,N-1 | quad3,-   |
	 * |___________|___________|
	 *
	 */









//------------------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------------------
CoupledFDTD::CoupledFDTD() : FDTD(){
	numOfStates = 4;

	state 			 = 0;
	audioRowChannel  = 0;
	printFormatCheck = false;

	name = "FDTD Filter Coupled with Excitation Model";

	// VBO/VAO params
	numOfVerticesPerQuad = 4;  // 1 quad = 1 triangle + vertex = 4 vertices
	numOfAttributes      = 24; //see explanation below + it must be multiple of 4!

    // crossfade
    doCrossfade            = false;
    crossfade_loc          = -1;
	listenerAFragCoord    = NULL;
	listenerBFragCoord    = NULL;
	nextListenerFragCoord = NULL;

	listenerAFragCoord_loc  = NULL;
	listenerBFragCoord_loc  = NULL;
	nextListenFragCoord_loc = NULL;

    crossfade      = 0;
    crossfadeDelta = 0;
    listenUnifName = "listenerAFragCoord"; // we define the name of the first listener, so that parent class retrieves correct uniform location in TextureFilter::init(). The second one is hardcoded in initAdditionalUniforms()
    currentDeck    = 0; // 0 = deck A, 1 = deck B

    updateListener  = false;
    updateListenerRender = false;

    //numOfPML = 6; // hardcoded in whole structure...

    typeValues_loc = new GLint[cell_numOfTypes];

    // pressure normalization
	maxPressure     = 1;
	maxPressure_loc = -1;
	maxPressure_loc_render = -1;
	updateMaxPress  = false;

	// physical params
	updatePhysParams       = false;
    rho                    = -1;
	minDs                  = -1;
	rho_c2_dt_invDs_loc    = -1;
	dt_invDs_invRho_loc    = -1;

	// excitation
	excitationModel_loc  = -1;
	excitationModel_loc_render= -1;
	exModels.num         = 0;
	exModels.excitationModel = -1;
	updateExType         = false;
	inv_exCellNum_loc    = -1;
	exCellNum_loc_render = -1;
    excitationCellNum    = -1;
	updateExCellNum      = false;
    //-------------------------
    // init to all directions
	excitationDirection[0] = 1;
	excitationDirection[1] = 1;
	excitationDirection[2] = -1;
	excitationDirection[3] = -1;
	excitationAngle = -1;
	//-------------------------
	exDirection_loc = -1;
	exDirection_loc_render = -1;
	updateExDir     = false;
	exFragCoord		= NULL;
	numOfExQuads    = -1;
	numOfExFolQuads = -1;
	// excitation low pass
	toggle_exLp_loc  = -1;
	toggle_exLp      = 1;
	updateExLpToggle = false;
	exLpFreq         = 22050;


	// feedback
	feedbFragCoord_loc = NULL;
	feedbFragCoord_loc_render    = -1;
	feedbFragCoord     = NULL;
	feedbackCoord[0]   = 0;
	feedbackCoord[1]   = 0;
	updateFeedback 	   = false;

	// pixel drawer
	pixelDrawer       = NULL;
	startFramePixels  = NULL;
	updatePixels      = false;
	updateFramePixels = false;
	resetPixels       = false;
	frameDuration     = -1;

	// slow motion
	slowMotion      = false;;
	slowMo_stretch  = 44100*10000; // this is a reference, it means that if we run at 44100 Hz each slow motion frame will last 10000 ns
	slowMo_sleep_ns = -1; // here the actual nano seconds will be saved, according to the simulation rate
	slowMo_time     = 0;
	slowMo_step     = 0;


	// mouse pos
	mouseFragCoord[0]     = -1;
	mouseFragCoord[1]     = -1;
	mouseCoord_loc_render = -1;
	updateMouseCoord      = false;

    // misc
    quad_loc		= -1;
    render  	    = true;
    domainShift[0]  = 0;
    domainShift[1]  = 0;
    textureShift[0] = 0;
    textureShift[1] = 0;
    singleRowAudioCapacity = -1;
	audioRowsUsed   = -1;
}

int CoupledFDTD::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions, excitationModels exMods,
						float ***domain, int audio_rate, int rate_mul, int periodSize, float mag, int *listCoord, float exLpF) {
	preInit(shaderLocation, exMods, exLpF, listCoord);
	excitationCellNum = excitationDimensions[0]*excitationDimensions[1]; // before TextureFilter::init(), cos this value is used inside initAdditionaUniforms(), that is called from TextureFilter::init()
	int sim_rate = TextureFilter::init(domainSize, numOfStates, excitationCoord, excitationDimensions, domain, audio_rate, rate_mul, periodSize, mag);
	// when using the other init overload, this is done when seDomain() is called from the outside
	if(initPixelDrawer() != 0)
		return -1;
	postInit();

	return sim_rate;
}

int CoupledFDTD::init(string shaderLocation, excitationModels exMods, int *domainSize, int audio_rate, int rate_mul, int periodSize,
				      float mag, int *listCoord, float exLpF) {
	preInit(shaderLocation, exMods, exLpF, listCoord);
	int sim_rate = TextureFilter::init(domainSize, numOfStates, audio_rate, rate_mul, periodSize, mag);
	postInit();

	return sim_rate;
}

/*
int CoupledFDTD::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	excitationCellNum = excitationDimensions[0]*excitationDimensions[1]; // before TextureFilter::init(), cos this value is used inside initAdditionaUniforms(), that is called from TextureFilter::seteDomain()

	if(TextureFilter::setDomain(excitationCoord, excitationDimensions, domain) != 0)
		return 1;

	if(initPixelDrawer() != 0)
		return -1;

	return 0;
}
*/

/*float *CoupledFDTD::getBuffer(int numSamples, float *inputBuffer) {

	// handle domain changes
	updatePixelDrawer();

	// control parameters modified from the outside
	updateUniforms();

	// calculate samples on GPU
	computeSamples(numSamples, inputBuffer);

	// render
	renderWindow();

	// get samples from GPU
	getSamples(numSamples);

	return outputBuffer;
}*/

/*void CoupledFDTD::getListenerPosition(int &x, int &y) {
	x = listenerCoord[0]-domainShift[0]-textureShift[0];
	y = listenerCoord[1]-domainShift[1]-textureShift[1];
}*/

// these 2 methods move in domain reference [shifting the window reference value stored in filter] and return in domain reference [again shifting the window reference value modified in filter]
/*int CoupledFDTD::shiftListenerV(int delta) {
	int posx = listenerCoord[0]-domainShift[0]-textureShift[0];
	int posy = listenerCoord[1]+delta-domainShift[1]-textureShift[1];
	if(setListenerPosition(posx, posy) != 0)
		printf("Can't move listener here: (%d, %d)!\n", posx, posy);

	return listenerCoord[1]-domainShift[1]-textureShift[1];
}

int CoupledFDTD::shiftListenerH(int delta){
	int posx = listenerCoord[0]+delta-domainShift[0]-textureShift[0];
	int posy = listenerCoord[1]-domainShift[1]-textureShift[1];
	if(setListenerPosition(posx, posy) != 0)
		printf("Can't move listener here: (%d, %d)!\n", posx, posy);

	return listenerCoord[0]-domainShift[0]-textureShift[0];
}*/

int CoupledFDTD::setListenerPosition(int x, int y) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	if(computeListenerFragCrossfade(x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		return 1;

	updateListener = true;

	return 0;
}

int CoupledFDTD::setFeedbackPosition(int x, int y) {
	if(!inited)
		return -1;

	if(computeFeedbackFrag(x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		return 1;

	updateFeedback = true;

	return 0;
}

void CoupledFDTD::getFeedbackPosition(int &x, int &y) {
	x = feedbackCoord[0]-domainShift[0]-textureShift[0];
	y = feedbackCoord[1]-domainShift[1]-textureShift[1];
}

// these 2 methods move in domain reference [shifting the window reference value stored in filter] and return in domain reference [again shifting the window reference value modified in filter]
int CoupledFDTD::shiftFeedbackV(int delta) {
	int posx = feedbackCoord[0]-domainShift[0-textureShift[0]];
	int posy = feedbackCoord[1]+delta-domainShift[1]-textureShift[1];
	if(setFeedbackPosition(posx, posy) != 0)
		printf("Can't move feedback here: (%d, %d)!\n", posx, posy);

	return feedbackCoord[1]-domainShift[1]-textureShift[1];
}
int CoupledFDTD::shiftFeedbackH(int delta){
	int posx = feedbackCoord[0]+delta-domainShift[0]-textureShift[0];
	int posy = feedbackCoord[1]-domainShift[1]-textureShift[1];
	if(setFeedbackPosition(posx, posy) != 0)
		printf("Can't move feedback here: (%d, %d)!\n", posx, posy);

	return feedbackCoord[0]-domainShift[0]-textureShift[0];
}

CoupledFDTD::~CoupledFDTD() {
	if(listenerBFragCoord_loc != NULL)
		delete[] listenerBFragCoord_loc;

	if(feedbFragCoord_loc != NULL)
		delete[] feedbFragCoord_loc;

	// single check...
	if(exFragCoord!=NULL) {
		for(int i=0; i<numOfStates; i++) {
			delete[] exFragCoord[i];
			delete[] feedbFragCoord[i];
			delete[] listenerBFragCoord[i];
		}
		delete[] exFragCoord;
		delete[] feedbFragCoord;
		delete[] listenerBFragCoord;
	}

	if(exModels.num>0)
		delete[] exModels.names;
}

/*int CoupledFDTD::setMaxPressure(float maxP) {
	if(inited) {
		maxPressure = maxP;
		updateMaxPress = true;
		return 0;
	}
	return -1;
}*/

/*int CoupledFDTD::fromRawWindowToDomain(int *coords) {
	// XD
	if( coords[0]<0 || coords[1]<0)
		return -1;

	// cope with magnification
	coords[0]/=magnifier;
	coords[1]/=magnifier;

	// apply shift
	coords[0]-=domainShift[0];
	coords[1]-=domainShift[1]-shiftkludge;

	return 0;
}

int CoupledFDTD::fromDomainToTexture(int *coords) {
	if(coords[0]<-domainShift[0] || coords[0]>domain_width+domainShift[0]-1  || coords[1]<-domainShift[1] || coords[1]>domain_height+domainShift[1]-1)
		return 1;

	// from domain reference to window reference, including the total texture shift coming from auduo rows
	coords[0] = coords[0]+domainShift[0]+textureShift[0];
	coords[1] = coords[1]+domainShift[1]+textureShift[1];
	return 0;
}

float CoupledFDTD::setDS(float ds) {
	if(!inited)
		return -1;
	if(ds >= minDs) {
		this->ds = ds;
		updatePhysParams = true;
	}
	else {
		this->ds = minDs;
		updatePhysParams = true;
		printf("value %f is lower than minimum ds possible %f\n", ds, minDs);
	}

	return this->ds;
}*/

int CoupledFDTD::setExcitationDirection(float angle) {
	if(!inited)
		return -1;

	excitationAngle = angle;

	if (excitationAngle < 0 ) {
		excitationDirection[0] = 1;
		excitationDirection[1] = 1;
		excitationDirection[2] = -1;
		excitationDirection[3] = -1;
	}
	else {
		float excite_x = 1.0f;
		float excite_y = 0.0f;
		float cosT = cos(excitationAngle * 3.14159f / 180.0f);
		float sinT = sin(excitationAngle * 3.14159f / 180.0f);
		double nx = excite_x * cosT - excite_y * sinT;
		double ny = excite_x * sinT + excite_y * cosT;
		excite_x = nx;
		excite_y = ny;

		float l, u, r, d;
		if (excite_x > 0) {
			r = excite_x;
			l = 0;
		}
		else {
			r = 0;
			l = excite_x;
		}
		if (excite_y > 0) {
			u = excite_y;
			d = 0;
		}
		else {
			u = 0;
			d = excite_y;
		}

		excitationDirection[0] = l;
		excitationDirection[1] = d;
		excitationDirection[2] = r;
		excitationDirection[3] = u;
	}
	updateExDir = true;

	return 0;
}

int CoupledFDTD::setExcitationModel(int index) {
	if(!inited)
		return -1;
	// since this is used from the outside, check we're not sending horseshit to the shader
	if( (index<exModels.num) && (index>=0) ) {
		reset(); // reset simulation, just in case
		exModels.excitationModel = index;
		updateExType = true;
		return 0;
	}
	return 1;
}

int CoupledFDTD::setExcitationModel(std::string name) {
	if(!inited)
		return -1;

	// look for the name and turn it into an index!
	for(int i=0; i<exModels.num; i++) {
		if( strcmp(name.c_str(), exModels.names[i].c_str())==0 ) {
			setExcitationModel(i);
			break;
		}

	}
	return 1;
}

//int CoupledFDTD::setPhysicalParameters(float c, float rho, float mu, float prandtl, float gamma) {
int CoupledFDTD::setPhysicalParameters(void *params) {
	if(!inited)
		return -1;

	this->c   = ((float *)params)[0];
	this->rho = ((float *)params)[1];

	computeDeltaS(); // this is fundamental! when we modify physical params, also ds changes accordingly

	updatePhysParams = true;

	return 0;
}
/*

// type is float, cos it can be cell_type [for a generic static cell] or a value in [-1, 0) [for a soft wall cell]
int CoupledFDTD::setCell(float type, int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	//VIC we have to remove texture shift for some reason related to audio quad...not sure yet
	x = coords[0] - textureShift[0];
	y = coords[1] - textureShift[1];

	bool wasExcitation = (pixelDrawer->getCellType(x, y)== cell_excitation);
	if(pixelDrawer->setCellType((GLfloat)type, x, y) != 0)
		return 1;
	// update excitation cells num if we are modifying one of those
	// add new one
	if(type == cell_excitation && !wasExcitation) {
		excitationCellNum++;
		updateExCellNum = true;
	}
	// turn one into something else
	else if(wasExcitation && type != cell_excitation) {
		excitationCellNum--;
		updateExCellNum = true;
	}

	updatePixels = true;

	startFramePixels[y][x][3] = type;

	float *frame = new float[gl_width*gl_height];
	for(int i=0; i<gl_width; i++) {
		for(int j=0; j<gl_height; j++)
			frame[i*gl_width+j] = startFramePixels[i][j][3];
	}

	pixelDrawer->loadFrame(frame);
	updateFramePixels = true;

	return 0;
}
int CoupledFDTD::resetCell(int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	//VIC we have to remove texture shift for some reason related to audio quad...not sure yet
	x = coords[0] - textureShift[0];
	y = coords[1] - textureShift[1];

	bool wasExcitation = (pixelDrawer->getCellType(x, y)== cell_excitation);
	if(pixelDrawer->resetCellType(x, y) != 0)
		return 1;
	// update excitation cells num if we are modifying one of those
	if(wasExcitation) {
		excitationCellNum--;
		updateExCellNum = true;
	}

	updatePixels = true;

	return 0;
}
*/
/*
int CoupledFDTD::clearDomain() {
	if(!inited)
		return -1;
	pixelDrawer->clear();
	excitationCellNum = 1;
	updateExCellNum   = true;
	updatePixels      = true;

	return 0;
}

int CoupledFDTD::reset() {
	if(!inited)
		return -1;

	resetPixels = true;

	return 0;
}*/

/*int CoupledFDTD::setMousePosition(int x, int y) {
	if(!inited)
		return -1;

	if(computeMouseFrag(x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		return 1;

	updateMouseCoord = true;

	return 0;
}*/

int CoupledFDTD::setExcitationLowPassToggle(int toggle) {
	if(!inited)
		return -1;

	toggle_exLp = toggle;
	updateExLpToggle = true;

	return 0;
}

//TODO turn into private and use directly the enum when makes sense [especially outside]
// utility
float CoupledFDTD::getPixelAlphaValue(int type, int PMLlayer, int dynamicIndex, int numOfPMLlayers){
	float retVal;
	switch(type) {
	//VIC this does cool crazy shit, tested if starts with this
		/*case cell_air:
			retVal = cell_air + PMLlayer;
		break;
		case cell_wall:
			retVal = cell_wall;
			break;
		case cell_excitation:
			retVal = cell_excitation;
			break;
		case cell_dynamic:
			retVal = cell_air + numOfPMLlayers + dynamicIndex;
			break;
		case cell_dead:
			retVal = cell_dead;
			break;
		default:
			retVal = -100;*/

	case cell_wall:
		retVal = cell_wall;
		break;
	case cell_air:
		retVal = cell_air + (PMLlayer!=0) + PMLlayer; // take a look at the order of cell_type enum to understand this
		break;
	case cell_excitation:
		retVal = cell_excitation;
		break;
	case cell_dynamic:
		retVal = cell_air + 1 + numOfPMLlayers + dynamicIndex; // take a look at the order of cell_type enum to understand this
		break;
	case cell_dead:
		retVal = cell_dead;
		break;
	case cell_nopressure:
		retVal = cell_nopressure;
		break;
	default:
		retVal = -100;
	}

	return retVal;
}
//------------------------------------------------------------------------------------------
// private methods ---> everything that defines the specific behavior of the filter is set here!
//------------------------------------------------------------------------------------------
void CoupledFDTD::preInit(string shaderLocation, excitationModels exMods, float exLpF, int *listCoord) {
	shaderLoc  = shaderLocation;
	shaders[0] = shaderLocation+"/fbo_vs.glsl";
	shaders[1] = shaderLocation+"/fbo_fs.glsl";
	shaders[2] = shaderLocation+"/render_vs.glsl";
	shaders[3] = shaderLocation+"/render_fs.glsl";

	/*  Attributes:
	 *  2 for vertex pos (x, y) -> used only in vertex shader
	 *  2 for texture pos (x, y) of this cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of left neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of up neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of right neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of down neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of left/up neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of right/down neighbor cell at current state -> needed for FDTD implicit stencil
	 *  2 for texture pos (x, y) of this cell at previous state -> needed for visco thermal filter
	 *  2 for texture pos (x, y) of left neighbor cell at previous state -> needed for visco thermal filter
	 *  2 for texture pos (x, y) of up neighbor cell at previous state -> needed for visco thermal filter
	 *
	 *  it should be a multiple of 4 cos we pass attributes packed up as vec4, to be more efficient
	 *  if we need a num of attributes that is not multiple of 4 [as whne we use absorption ainstead of
	 *  the visco thermal filter] the extra values are ignored. this causes an harmless error in SetUpVao() (:
	 *
	*/

	if(listCoord != NULL) {
		listenerCoord[0] = listCoord[0];
		listenerCoord[1] = listCoord[1];
	}

	exLpFreq = exLpF;

	exModels.num = exMods.num;
	exModels.names = new string[exMods.num];
	for(int i=0; i<exModels.num; i++)
		exModels.names[i] = exMods.names[i];
	exModels.excitationModel = exMods.excitationModel; // assign starting one
}

void CoupledFDTD::postInit() {
	minDs          = ds;
	crossfadeDelta = 1.0/(period_size-1); //TODO: make duration of crossfade proportional to deltaS

	slowMo_sleep_ns = slowMo_stretch/simulation_rate;

	frameDuration = FRAME_DURATION;
}

void CoupledFDTD::initPhysicalParameters() {
	// default values
	FDTD::initPhysicalParameters();
	rho = RHO_AIR; // this is specific of this class

	// super class will print \n and c value
    printf("\trho: \t%f\n", rho);
}

// here we set the values and offsets that characterize our image [size, quads displacement, etc]!
// the image is what goes into the texture, i.e., all the quads, contained in the frames or not
void CoupledFDTD::initImage(int domainWidth, int domainHeigth) {

	// excitation and excitation follow up quads [each is a single cell]
	numOfExQuads    = numOfStates;
	numOfExFolQuads = numOfStates;

	// we don't include PML in simulation domain, to make things easier when we want to update texture from outside

	gl_width  = domainWidth + 2*numOfPML + 2*numOfDeadCellLayers;
	gl_height = domainHeigth + 2*numOfPML + 2*numOfDeadCellLayers;

	// domain is inside window, shifted as follows
	domainShift[0] = (numOfPML+numOfDeadCellLayers);
	domainShift[1] = (numOfPML+numOfDeadCellLayers);


	textureWidth  = gl_width*2;
	textureHeight = gl_height*2;

	// the audio row is at the bottom portion of the texture, audio samples are stored there.
	// each pixel contains 4 samples.
	// there must be enough space to store all the buffer samples requested by audio card in one period,
	audioRowW = gl_width*2 - numOfExQuads - numOfExFolQuads; // whole texture width - excitation and excitation follow up pixels, cos by default they are on same row as audio
	audioRowH = audioRowsUsed = 1;
	// so, now last row is numOfStates excitation pixels, numOfStates excitation follow up pixels and rest is audio row

	// if audio row space is not enough to store all the audio samples...
	if( period_size > (audioRowH*audioRowW*4) ) {
		//...we want an equal repartition of the audio samples on multiple rows, so that we can easily retrieve them in getBuffer()

		// first of all lets count how many fragments we need to save the whole sample buffer as fragments
		int numOfAudioFrags = period_size/4;

		// then let's start placing the ex quads and followup quads side by side, vertically, at the bottom left of the texture
		audioRowH = numOfExQuads;
		audioRowW = textureWidth-2;

		// we can use only an even number of fragments if we wanna split audio samples on multiple rows,
		// since period sizes are always even
		audioRowsUsed = numOfExQuads;
		if((audioRowsUsed%2)!=0)
			audioRowsUsed--; // if it was not even, we discard one


		// this is the number of frags needed on each row for an equal repartition of the audio samples
		int neededFrags = numOfAudioFrags/audioRowsUsed;
		// do we have enough space?
		while(neededFrags > audioRowW) {
			// if not, lets add rows, always an even number in total!
			audioRowsUsed *= 2;
			audioRowH   = audioRowsUsed;
			neededFrags = numOfAudioFrags/audioRowsUsed;
		}
		// if we have enough space, let's fix the width that we need to contain all samples
		audioRowW = neededFrags;

		// please have a look at fillVertices() method for a graphical explanation of these positioning
	}

	textureHeight += audioRowH+ISOLATION_LAYER; // put extra line to isolate frame from audio [to avoid interferences]

	singleRowAudioCapacity = audioRowW*4; // this is the actual number of samples that we can store in each audio row [NOT FRAGS! each frag contains 4 samples!]

	// the full texture is 4 frames + these extra layers
	textureShift[0] = 0;
	textureShift[1] = audioRowH+ISOLATION_LAYER;

	allocateResources();


	//-----------------------------------------------------
	// these are the chunks of data drawn in each quad
	drawArraysValues[quad_0][0] = 0;
	drawArraysValues[quad_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_1][0] = 4;
	drawArraysValues[quad_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_2][0] = 8;
	drawArraysValues[quad_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_3][0] = 12;
	drawArraysValues[quad_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_0][0] = 16;
	drawArraysValues[quad_excitation_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_1][0] = 20;
	drawArraysValues[quad_excitation_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_2][0] = 24;
	drawArraysValues[quad_excitation_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_3][0] = 28;
	drawArraysValues[quad_excitation_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_0][0] = 32;
	drawArraysValues[quad_excitation_fu_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_1][0] = 36;
	drawArraysValues[quad_excitation_fu_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_2][0] = 40;
	drawArraysValues[quad_excitation_fu_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_3][0] = 44;
	drawArraysValues[quad_excitation_fu_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_0][0] = 48;
	drawArraysValues[quad_drawer_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_1][0] = 52;
	drawArraysValues[quad_drawer_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_2][0] = 56;
	drawArraysValues[quad_drawer_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_3][0] = 60;
	drawArraysValues[quad_drawer_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_audio][0] = 64;
	drawArraysValues[quad_audio][1] = numOfVerticesPerQuad;
	//-----------------------------------------------------
}

// dynamically allocates the resources needed for our simulation domain
void CoupledFDTD::allocateResources() {
	drawArraysValues  = new int *[quad_numOfQuads];
	for(int i=0; i<quad_numOfQuads; i++) {
		drawArraysValues[i] = new int[2];
	}

	audioRowWriteTexCoord  = new float [numOfStates];

	listenerBFragCoord_loc = new GLint [numOfStates];
	feedbFragCoord_loc     = new GLint [numOfStates];
	exFragCoord            = new float *[numOfStates];
	feedbFragCoord         = new float *[numOfStates];
	listenerAFragCoord    = listenerFragCoord;
	listenerBFragCoord    = new float *[numOfStates];
	for(int i=0; i<numOfStates; i++) {
		exFragCoord[i]           = new float[2];
		feedbFragCoord[i]        = new float[2];
		listenerBFragCoord[i]    = new float[2];
	}
	texturePixels = new float[textureWidth*textureHeight*4]; // 4 is RGBA channels

	// this will contain clear domain pixels
	startFramePixels = new float **[gl_width];
	for(int i = 0; i <gl_width; i++) {
		startFramePixels[i] = new float *[gl_height];
		for(int j = 0; j <gl_height; j++) {
			startFramePixels[i][j] = new float[4];
		}
	}
}

// here we prepare the vao to get the vertices' data we defined in fillVertices()
/*void CoupledFDTD::initVao(GLuint vbo, GLuint shader_program_fbo, GLuint shader_program_render, GLuint &vao) {
	glGenVertexArrays (1, &vao);

	glBindVertexArray (vao);
	glBindBuffer (GL_ARRAY_BUFFER, vbo); // bind to points VBO [means shader fetches data from there when using this attribute]
	checkError("initVao glBindBuffer");

	// attributes are shared between the 2 programs
	GLint in_loc_fbo;    // we target FBO, cos it needs more attributes
	int sizeOfInVar = 4; // we pack 2 attributes [vec2 + vec2] in each in var [fast], that's why the num of attributes must be multiple of 4!
	string inVarName = "";
	for(int i=0; i<numOfAttributes/sizeOfInVar; i++){
		inVarName = "in"+to_string(i);
		in_loc_fbo = glGetAttribLocation(shader_program_fbo, inVarName.c_str());
		glEnableVertexAttribArray(in_loc_fbo);
		//                     in var location       4 float elements         access stride                        elements to skip
		glVertexAttribPointer (in_loc_fbo, sizeOfInVar, GL_FLOAT, GL_FALSE, numOfAttributes * sizeof(GLfloat), (void*)(i*sizeOfInVar * sizeof(GLfloat)));
	}
}*/

// here we write the stuff we'll pass to the VBO
int CoupledFDTD::fillVertices() {
	int numOfDrawingQuads = numOfStates;
	// total number of quads: state quads + audio quad + excitation quads + excitation follow up quads + drawing quads
	int numOfQuads = numOfStates+numOfExQuads+numOfExFolQuads+numOfDrawingQuads+1; // 1 is audio quad

	QuadImplicitFDTD quads[numOfQuads]; // save everything in here

	QuadImplicitFDTD quad0;
	QuadImplicitFDTD quad1;
	QuadImplicitFDTD quad2;
	QuadImplicitFDTD quad3;

	QuadImplicitFDTD quadsEx[numOfExQuads+numOfExFolQuads];

	QuadImplicitFDTD drawingQuad0;
	QuadImplicitFDTD drawingQuad1;
	QuadImplicitFDTD drawingQuad2;
	QuadImplicitFDTD drawingQuad3;

	QuadImplicitFDTD quadAudio;

	// numOfStates to correctly calculate texture coordinates for excitation quads
	// numOfAttributes to allocate dynamic data structures where to save attributes
	// textureW/H since quads have to normalize pixel ranges, they need to know size of whole texture
	// deltaTexX/Y is to get neighbors
	quad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad2.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad3.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	for(int i=0; i<numOfExQuads+numOfExFolQuads; i++)
		quadsEx[i].init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	drawingQuad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad2.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad3.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	quadAudio.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	// if one row is enough to store audio buffer samples and have excitation, the excitation stuff is placed as depicted in scheme on top of this file and here:
	/*
	 * _______________________
	 *|_______|_|_|_|_|_|_|_|_| } lower extension [audio rows+excitation+excitation followup]
	 *	^audio^	 ^ side quads ^
	 *	 [side quads are excitation+excitation followup]
	*/


	//TODO update diagram
	// otherwise we reserve to audio more than one row at bottom of global texture and move excitation cells at the top righ of texture, as a column
	/*				 _
	 * 				|_|   < excitation and followup cells in column
	 * 				|_|
	 * 				|_|
	 * _____________ ...  continues...
	 *|             |
	 *|_____________|
	 *	^audio^
	 *	 [side quads are excitation+excitation followup]
	*/

	//	pixels:	              top left coord (x, y)                width and height                 x and y shift for texture coord N and N-1
	int quad0Attributes[8] = {0, textureHeight-gl_height/*-1*/,        gl_width, gl_height, 		 	    gl_width, 0, gl_width, gl_height};
	int quad1Attributes[8] = {0, textureHeight/*-1*/,                  gl_width, gl_height, 				0, -gl_height, gl_width, -gl_height};
	int quad2Attributes[8] = {gl_width, textureHeight/*-1*/,           gl_width, gl_height, 			    -gl_width, 0, -gl_width, -gl_height};
	int quad3Attributes[8] = {gl_width, textureHeight-gl_height/*-1*/, gl_width, gl_height, 			    0, gl_height, -gl_width, gl_height};

	// main quads... they are all the same [HARD CODED, change with numOfStates but not parametrically]
	quad0.setQuad(quad0Attributes); // bottom left quad
	quad1.setQuad(quad1Attributes); // top left quad
	quad2.setQuad(quad2Attributes); // top right quad
	quad3.setQuad(quad3Attributes); // bottom left quad

	// same as
	/*quad0.setQuad(0, textureHeight-gl_height-1,        gl_width, gl_height, 		 	    gl_width, 0, gl_width, gl_height);
	quad1.setQuad(0, textureHeight-1,                  gl_width, gl_height, 				0, -gl_height, gl_width, -gl_height);
	quad2.setQuad(gl_width, textureHeight-1,           gl_width, gl_height, 			    -gl_width, 0, -gl_width, -gl_height);
	quad3.setQuad(gl_width, textureHeight-gl_height-1, gl_width, gl_height, 			    0, gl_height, -gl_width, gl_height);*/

	// same vertices as main simulation quads...but they cover the whole texture area!
	drawingQuad0.setQuadDrawer(0, textureHeight-gl_height/*-1*/, 			gl_width, gl_height); // bottom left quad
	drawingQuad1.setQuadDrawer(0, textureHeight/*-1*/, 						gl_width, gl_height); // top left quad
	drawingQuad2.setQuadDrawer(gl_width, textureHeight/*-1*/, 				gl_width, gl_height); // top right quad
	drawingQuad3.setQuadDrawer(gl_width, textureHeight-gl_height/*-1*/, 	gl_width, gl_height); // bottom left quad


	// we can understand if one row is enough checking if the height of audio rows [means excitation cells are horizontally within last row and not vertically over multiple audio rows]
	if(audioRowH == 1){
		int rightx = textureWidth-(numOfExQuads+numOfExFolQuads); // save length of audio quad, useful to determine leftmost coordinate of excitation quads
		quadAudio.setQuadAudio(0, 0+1, rightx, 1);

		// excitation cells
		for(int i=0; i<numOfExQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor,   x and y shift for follow up,    index to quad this excitation is linked to
			quadsEx[i].setQuadExcitation( rightx, 0+1,    1, 1, 	1, 0,    numOfStates, 0, 	i);
			rightx++;
		}

		// excitation follow up cells
		for(int i=0; i<numOfExFolQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor, x and y shift for follow up [which is excitation in this case, cos this is followu p],    index to quad this excitation is linked to
			quadsEx[numOfExQuads+i].setQuadExcitation( rightx, 0+1, 1, 1, 	1, 0, -numOfStates, 0, 	i);
			rightx++;
		}
	}
	else {
		quadAudio.setQuadAudio(0, audioRowH/*-1*/, audioRowW, audioRowH);

		int top = audioRowH/*-1*/;
		for(int i=0; i<numOfExQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor,   x and y shift for follow up,    index to quad this excitation is linked to
			quadsEx[i].setQuadExcitation( textureWidth-2, top, 	1, 1, 	0, -1, 		1, 0, 	i);
			top--;
		}

		// excitation follow up cells
		top = audioRowH/*-1*/;
		for(int i=0; i<numOfExFolQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor, x and y shift for follow up [which is excitation in this case, cos this is followu p],    index to quad this excitation is linked to
			quadsEx[numOfExQuads+i].setQuadExcitation( textureWidth-1, top, 	1, 1, 	0, -1, 		-1, 0, 	i);
			top--;
		}
	}

	// save these values so that main quads' pixels know where to look for excitation in texture in shader
	// please note that we have to fetch the coordinates pointing to this specific quad among the attributes of the next quad [cos tex coords always point to previous step]
	// PLUS
	// we want quad x to point to excitation quad x-1!!! see ridiculous explanation in getBuffer()
	for(int i=0; i<numOfStates; i++) {
		exFragCoord[i][0] = quadsEx[i].getAttribute(2)+deltaCoordX/2; // x texture coordinate for state N of 3rd entry [bottom left vertex]
		exFragCoord[i][1] = quadsEx[i].getAttribute(3)+deltaCoordY/2; // y texture coordinate for state N of 3rd entry [bottom left vertex]
	}
	//VIC we added deltaCoordX/2 cos this shift otherwise would be needed in the FBO shader [and repeated for each fragment!]. We have to remove it in the render shader though...not sure why they differ!


	// HARDCODED
	// put all quad in same array
	quads[0].copy(&quad0);
	quads[1].copy(&quad1);
	quads[2].copy(&quad2);
	quads[3].copy(&quad3);

	for(int i=0; i<numOfExQuads+numOfExFolQuads; i++)
		quads[numOfStates+i].copy(&quadsEx[i]);

	quads[numOfStates+numOfExQuads+numOfExFolQuads].copy(&drawingQuad0);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 1].copy(&drawingQuad1);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 2].copy(&drawingQuad2);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 3].copy(&drawingQuad3);

	quads[numOfStates+numOfExQuads+numOfExFolQuads+numOfDrawingQuads].copy(&quadAudio);

	// prepare VBO data structure
	int verticesNum = numOfQuads*numOfVerticesPerQuad*numOfAttributes;
	vertices_data   = new float[verticesNum];

	// fill it
	int numOfElements = numOfVerticesPerQuad*numOfAttributes;
	for(int i=0; i<numOfQuads; i++) {
		for(int j=0; j<numOfElements; j++) {
			vertices_data[i*numOfElements + j] = quads[i].getAttribute(j);
			//printf("vertices_data[%d]: %f\n", i*numOfElements + j, vertices_data[i*numOfElements + j]);
		}
	}

	//printf("vertSize: %d\n", verticesNum*sizeof(float));
	return verticesNum*sizeof(float);
}

// let's write the frame, i.e., the pixels of the domain+pml+whatever that are repeated per each state!
// pass excitation and use some built in boundaries that also handle PML
/*void CoupledFDTD::initFrame(int *excitationCoord, int *excitationDimensions, float ***domain) {
	//-----------------------------------------------------
	// prepare pixels
	//-----------------------------------------------------
	cell_type cellType;
	int PMLlayer;
	float val;

	// clear out all pixels and set all states to air with no PML absorption
	cellType = cell_air;
	PMLlayer  = 0;
	val = getPixelAlphaValue(cellType, PMLlayer);
	for(int i=0; i<gl_width; i++) {
		for(int j=0; j<gl_height; j++) {
			startFramePixels[i][j][0] = 0.0;
			startFramePixels[i][j][1] = 0.0;
			startFramePixels[i][j][2] = 0.0;
			startFramePixels[i][j][3] = val;
		}
	}

	// top and bottom PML layers [leaving space for dead rows and columns at the extremes]
	cellType = cell_air;
	PMLlayer   = numOfPML+1;
	int toprow = gl_height-1;
	int btmrow = 0;
	int start  = 0;
	int stop   = gl_width-1;

	for(int i=0; i<numOfPML; i++) {
		toprow--;
		btmrow++;
		start++;
		stop--;
		PMLlayer--;
		val = getPixelAlphaValue(cellType, PMLlayer);
		fillTextureRow(toprow, start, stop, val, startFramePixels);
		fillTextureRow(btmrow, start, stop, val, startFramePixels);
	}

	// left and right PML layers [leaving space for dead rows and columns at the extremes]
	PMLlayer     = numOfPML+1;
	int rightcol = gl_width-1;
	int leftcol  = 0;
	start        = 0;
	stop         = gl_height-1;
	for(int i=0; i<numOfPML; i++) {
		rightcol--;
		leftcol++;
		start++;
		stop--;
		PMLlayer--;
		val = getPixelAlphaValue(cellType, PMLlayer);
		fillTextureColumn(rightcol, start, stop, val, startFramePixels);
		fillTextureColumn(leftcol, start, stop, val, startFramePixels);
	}

	// put one dead layer in leftmost and rightmost columns, to completely disconnect audio rows from simulation
	float deadVal = getPixelAlphaValue(cell_dead);
	fillTextureColumn(0, 0, gl_height-1, deadVal, startFramePixels);
	fillTextureColumn(gl_width-1, 0, gl_height-1, deadVal, startFramePixels);

	// put dead cell layers also on top and bottom edges
	fillTextureRow(0, 0, gl_width-1, deadVal, startFramePixels);
	fillTextureRow(gl_height-1, 0, gl_width-1, deadVal, startFramePixels);




	// copy domain into pixels
	for(int i = 0; i <domain_width; i++) {
		for(int j = 0; j <domain_height; j++) {
			startFramePixels[numOfPML+1+i][numOfPML+1+j][3] = domain[i][j][3];
		}
	}

	// add excitation layer
	float excitationVal = getPixelAlphaValue(cell_excitation);
	for(int i = 0; i <excitationDimensions[0]; i++) {
		fillTextureColumn(numOfPML+1+excitationCoord[0]+i, numOfPML+1+excitationCoord[1],
						  numOfPML+1+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, startFramePixels); // +1's are for dead layers
	}



	//-----------------------------------------------------

	// then put the frame into the image
	fillImage(startFramePixels);

	return;
}*/

// this fills texture with passed pixels
/*void CoupledFDTD::fillImage(float ***pixels) {
	// texture is divided in quads, representing a state
	// state0: bottom left quad
	// state1: top left quad
	// state2: top right quad
	// state3: bottom right quad

	// wipe out everything
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			texturePixels[(i*textureWidth+j)*4]   = 0;
			texturePixels[(i*textureWidth+j)*4+1] = 0;
			texturePixels[(i*textureWidth+j)*4+2] = 0;
			texturePixels[(i*textureWidth+j)*4+3] = 0;
		}
	}

	// copy pixels in sate0 and clear out the rest of the texture
	// making sure to offset pixels to skip audio rows at bottom
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			if( (i<gl_height) && (j<gl_width) ) {
				texturePixels[((i+audioRowH)*textureWidth+j)*4+3] = pixels[j][i][3];
			}
		}
	}
}*/

// i'd like to be able to say something useful...
int CoupledFDTD::initAdditionalUniforms() {
	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// location of pressure propagation coefficient, PML values, etc.

	// get locations
	rho_c2_dt_invDs_loc = shaderFbo.getUniformLocation("rho_c2_dt_invDs");
	if(rho_c2_dt_invDs_loc == -1) {
		printf("rho_c2_dt_invDs uniform can't be found in fbo shader program\n");
		//return -1;
	}
	dt_invDs_invRho_loc = shaderFbo.getUniformLocation("dt_invDs_invRho");
	if(dt_invDs_invRho_loc == -1) {
		printf("dt_invDs_invRho uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// cell types' beta and sigma prime dt
	for(int i=0; i<cell_numOfTypes; i++) {
		string typeValues_uniform_name = "typeValues["+to_string(i)+"]";
		typeValues_loc[i] = shaderFbo.getUniformLocation(typeValues_uniform_name.c_str());
		if(typeValues_loc[i] == -1) {
			printf("typeValues[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	quad_loc= shaderFbo.getUniformLocation("quad");
	if(quad_loc == -1) {
		printf("quad uniform can't be found in fbo shader program\n");
		//return -1;
	}

	maxPressure_loc = shaderFbo.getUniformLocation("maxPressure");
	if(maxPressure_loc == -1) {
		printf("maxPressure uniform can't be found in fbo shader program\n");
		//return -1;
	}
	// location of crossfade control
	crossfade_loc = shaderFbo.getUniformLocation("crossfade");
	if(crossfade_loc == -1) {
		printf("crossfade uniform can't be found in fbo shader program\n");
		//return -1;
	}
	// location of number of excitation cells [to normalize input velocity]
	inv_exCellNum_loc = shaderFbo.getUniformLocation("inv_exCellNum");
	if(inv_exCellNum_loc == -1) {
		printf("exCellNum uniform can't be found in fbo shader program\n");
		//return -1;
	}
	// location of number of excitation cells [to normalize input velocity]
	excitationModel_loc = shaderFbo.getUniformLocation("excitationModel");
	if(excitationModel_loc == -1) {
		printf("excitationModel uniform can't be found in fbo shader program\n");
		//return -1;
	}

	GLint exFragCoord_loc[numOfStates];
	for(int i=0; i<numOfStates; i++) {
		string name = "exFragCoord[" + to_string(i) + "]";
		exFragCoord_loc[i] = shaderFbo.getUniformLocation(name.c_str());
		if(exFragCoord_loc[i] == -1) {
			printf("exFragCoord[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	// location of direction of excitation [velocity]
	exDirection_loc = shaderFbo.getUniformLocation("excitationDirection");
	if(exDirection_loc == -1) {
		printf("excitationDirection uniform can't be found in fbo shader program\n");
		//return -1;
	}

	for(int i=0; i<numOfStates; i++) {
		string name = "feedbFragCoord[" + to_string(i) + "]";
		feedbFragCoord_loc[i] = shaderFbo.getUniformLocation(name.c_str());
		if(feedbFragCoord_loc[i] == -1) {
			printf("feedbFragCoord[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	for(int i=0; i<numOfStates; i++) {
		string name = "listenerBFragCoord[" + to_string(i) + "]";
		listenerBFragCoord_loc[i] = shaderFbo.getUniformLocation(name.c_str());
		if(listenerBFragCoord_loc[i] == -1) {
			printf("listenerBFragCoord[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	// location of excitation low pass toggle
	toggle_exLp_loc = shaderFbo.getUniformLocation("toggle_exLp");
	if(toggle_exLp_loc == -1) {
		printf("toggle_exLp uniform can't be found in fbo shader program\n");
		//return -1;
	}
	// excitation low pass coefficients
	GLint exLp_a1_loc = shaderFbo.getUniformLocation("exLp_a1");
	if(exLp_a1_loc == -1) {
		printf("exLp_a1 uniform can't be found in fbo shader program\n");
		//return -1;
	}
	GLint exLp_a2_loc = shaderFbo.getUniformLocation("exLp_a2");
	if(exLp_a2_loc == -1) {
		printf("exLp_a2 uniform can't be found in fbo shader program\n");
		//return -1;
	}
	GLint exLp_b0_loc = shaderFbo.getUniformLocation("exLp_b0");
	if(exLp_b0_loc == -1) {
		printf("exLp_b0 uniform can't be found in fbo shader program\n");
		//return -1;
	}
	GLint exLp_b1_loc = shaderFbo.getUniformLocation("exLp_b1");
	if(exLp_b1_loc == -1) {
		printf("exLp_b1 uniform can't be found in fbo shader program\n");
		//return -1;
	}
	GLint exLp_b2_loc = shaderFbo.getUniformLocation("exLp_b2");
	if(exLp_b2_loc == -1) {
		printf("exLp_b2 uniform can't be found in fbo shader program\n");
		//return -1;
	}



	// set
	shaderFbo.use();

	float *sigmasdt;
	if(!computePMLcoeff(sigmasdt))
		return -2;

	//                           beta, sigma_prime*deltaT [ (1-beta+sigma)*deltaT]
	//glUniform2f(typeValues_loc[1], 0, /*deltaT*/maxSigmaDt); // wall
	//glUniform2f(typeValues_loc[2], 0, /*deltaT*/maxSigmaDt); // excitation
	glUniform2f(typeValues_loc[0], 0, dt); // wall
	glUniform2f(typeValues_loc[1], 1, 0);      // air
	glUniform2f(typeValues_loc[2], 0, dt); // excitation
	for(int i=0; i<numOfPML; i++)		   // pml layers
		glUniform2f(typeValues_loc[cell_pml0+i], 1, sigmasdt[i]);
	glUniform2f(typeValues_loc[numOfPML+3], 0, dt);  // dynamic wall [starts as wall]
	glUniform2f(typeValues_loc[numOfPML+4], 0, 1000000); // dead cell
	glUniform2f(typeValues_loc[numOfPML+5], 1, 0);      // no pressure

	delete[] sigmasdt;

	glUniform1f(crossfade_loc, 0);
	checkError("glUniform1f crossfade");

	setUpExCellNum();
	checkError("glUniform1f 1/excitationCellNum");

	// where the main quads are retrieving excitation
	for(int i=0; i<numOfStates; i++)
		glUniform2f(exFragCoord_loc[i], exFragCoord[i][0], exFragCoord[i][1]);

	// excitation low pass
	// coefficients
	float a1, a2, b0, b1, b2;
	computeExcitationLPCoeff(a1, a2, b0, b1, b2);
	glUniform1f(exLp_a1_loc, a1);
	glUniform1f(exLp_a2_loc, a2);
	glUniform1f(exLp_b0_loc, b0);
	glUniform1f(exLp_b1_loc, b1);
	glUniform1f(exLp_b2_loc, b2);

	// set dynamically
	computeFeedbackFrag(feedbackCoord[0], feedbackCoord[1]);
	setUpFeedback();
	setUpPhysicalParams();
	setUpExcitationTypeFbo();
	setUpExcitationDirectionFbo();
	setUpExLpToggle();

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------



	//--------------------------------------------------------------------------------------------
	// render shader uniforms
	//--------------------------------------------------------------------------------------------
	// get locations
	GLint deltaCoord_by2_loc = shaderRender.getUniformLocation("deltaCoord_by2");
	if(deltaCoord_by2_loc == -1) {
		printf("deltaCoord_by2 uniform can't be found in render shader program\n");
		//return -1;
	}
	GLint deltaCoord_times3_by2_loc = shaderRender.getUniformLocation("deltaCoord_times3_by2");
	if(deltaCoord_times3_by2_loc == -1) {
		printf("deltaCoord_times3_by2 uniform can't be found in render shader program\n");
		//return -1;
	}

	maxPressure_loc_render = shaderRender.getUniformLocation("maxPressure");
	if(maxPressure_loc_render == -1) {
		printf("maxPressure uniform can't be found in render shader program\n");
		//return -1;
	}

	feedbFragCoord_loc_render = shaderRender.getUniformLocation("feedbFragCoord");
	if(feedbFragCoord_loc_render == -1) {
	   printf("feedbFragCoord uniform can't be found in render shader program\n");
	   //return -1;
	}

	// location of number of excitation cells [to normalize input velocity]
	excitationModel_loc_render = shaderRender.getUniformLocation("excitationModel");
	if(excitationModel_loc_render == -1) {
		printf("excitationModel uniform can't be found in render shader program\n");
		//return -1;
	}

	mouseCoord_loc_render = shaderRender.getUniformLocation("mouseFragCoord");
	if(mouseCoord_loc_render == -1) {
		printf("mouseFragCoord uniform can't be found in render shader program\n");
		//return -1;
	}

	//VIC: not used yet
	// location of number of excitation cells [to visualize input velocity]
	//exCellNum_loc = shaderRender.getUniformLocation("exCellNum");
	//if(exCellNum_loc == -1) {
	//	printf("exCellNum uniform can't be found in render shader program\n");
		//return -1;
	//}

	// location of direction of excitation [velocity]
	//exDirection_loc = shaderRender.getUniformLocation("excitationDirection");
	//if(exDirection_loc == -1) {
	//	printf("excitationDirection uniform can't be found in render shader program\n");
		//return -1;
	//}

	// set
	shaderRender.use();

	// set once
	glUniform2f(deltaCoord_by2_loc, deltaCoordX/2, deltaCoordY/2);
	checkError("glUniform2f deltaCoord_by2");

	glUniform2f(deltaCoord_times3_by2_loc, 3*deltaCoordX/2, 3*deltaCoordY/2);
	checkError("glUniform2f deltaCoord_times3_by2");

	setUpExCellNumRender();
	checkError("glUniform1i excitationCellNum");

	// set dynamically
	setUpListenerRender();
	setUpFeedbackRender();
	setUpExcitationTypeRender();
	setUpExcitationDirectionRender();
	setUpMouseRender();
	checkError("glUniform2i mouseFragCoord");

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------

	// init variable uniforms
	shaderFbo.use();
	setUpMaxPressure(); // sets also in render shader
	glUseProgram (0);

	return 0;
}

/*void CoupledFDTD::resetAudioRow() {
	// audio starts at tex coordinates (0,0), always
	audioRowWriteTexCoord[0] = 0;
	audioRowWriteTexCoord[1] = 0;

	return;
}*/

/*int CoupledFDTD::computeListenerPos(int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	listenerCoord[0] = coords[0];
	listenerCoord[1] = coords[1];
	return 0;
}*/

void CoupledFDTD::computeNextListenerFrag() {
	// listener coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 1      reads audio       from quad 0
	nextListenerFragCoord[1][0] = (float)(listenerCoord[0]+0.5) / (float)textureWidth;
	nextListenerFragCoord[1][1] = (float)(listenerCoord[1]+0.5) / (float)textureHeight;
	// quad 2      reads audio       from quad 1
	nextListenerFragCoord[2][0] = (float)(listenerCoord[0]+0.5) / (float)textureWidth;
	nextListenerFragCoord[2][1] = (float)(listenerCoord[1]+0.5+gl_height) / (float)textureHeight;
	// quad 3      reads audio       from quad 2
	nextListenerFragCoord[3][0] = (float)(listenerCoord[0]+0.5+gl_width)  / (float)textureWidth;
	nextListenerFragCoord[3][1] = (float)(listenerCoord[1]+0.5+gl_height) / (float)textureHeight;
	// quad 0      reads audio       from quad 3
	nextListenerFragCoord[0][0] = (float)(listenerCoord[0]+0.5+gl_width)  / (float)textureWidth;
	nextListenerFragCoord[0][1] = (float)(listenerCoord[1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()
}


// this is for initialization, no crossfade, automatically called by TextureFilter::initUniforms()
// it calculates the new position of the listener in all the states [that's why is filter specific] ---> values expected in domain reference [but saved as window, cos we have to deal with texture]!
int CoupledFDTD::computeListenerFrag(int x, int y) {
	listenerAFragCoord_loc = listenerFragCoord_loc; // we link the location of the listener A to this var here, cos we are sure that listenerFragCoord_loc has just been initialized!

	if(computeListenerPos(x, y)!=0)
		return -1;

	// fixed deck, no crossfade
	nextListenerFragCoord   = listenerAFragCoord;
	nextListenFragCoord_loc = listenerAFragCoord_loc;

	// and calculate the position to put in that deck
	computeNextListenerFrag();

	return 0;
}

// this is the method called at runtime
// again, it calculates the new position of the listener in all the states, but includes crossfade [decks handling]
int CoupledFDTD::computeListenerFragCrossfade(int x, int y) {

	/*if(computeListenerPos(x, y)!=0)
		return -1;*/
	computeListenerPos(x, y); //VIC removed that control to avoid missing movements when buffer size is big, hence crossfade is slow

	// target the next deck
	if(currentDeck==0) {
		nextListenerFragCoord   = listenerBFragCoord;
		nextListenFragCoord_loc = listenerBFragCoord_loc;
	}
	else {
		nextListenerFragCoord   = listenerAFragCoord;
		nextListenFragCoord_loc = listenerAFragCoord_loc;
	}

	// and calculate the position to put in that deck [in getBuffer() the actual crossfade between new and old position will happen]
	computeNextListenerFrag();

	return 0;
}

void CoupledFDTD::computeSamples(int numSamples, float *inputBuffer) {
 	// this is NO CROSSFADE and should come with crossfade if in
 	// // Iterate over each sample
	// for(int n = 0; n < numSamples; n++)
	// commented out


/*
	if(doCrossfade) {
		if(currentDeck==0)
			crossfade=1;
		else
			crossfade=0;
		glUniform1f(crossfade_loc, crossfade);
	}
*/

	const int bits = GL_TEXTURE_FETCH_BARRIER_BIT;

	// Iterate over each sample
	for(int n = 0; n < numSamples; n++) {

		updatePixelDrawerFrame(); // this needs sample level precision [but it's inline]

		if(doCrossfade) {
			if(currentDeck==0)
				crossfade+=crossfadeDelta;
			else
				crossfade-=crossfadeDelta;
			glUniform1f(crossfade_loc, crossfade);
		}

		// CoupledFDTD simulation on GPU!
		//------------------
		// FBO passes, render texture on FBO
		for(int i=0; i<rate_mul; i++) {

			#ifdef TIME_RT_FBO
				gettimeofday(&start_, NULL);
			#endif

			if(slowMotion && render) {
				// prepare FBO passes
				shaderFbo.use();
				checkError("glUseProgram SLO MO");
				glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to our framebuffer
				glViewport(0, 0, textureWidth, textureHeight);
			}

			// ok, down here the ridiculous explanation of the order of these operations and of the mapping of listener, excitation and feedback coordinates...
			// here we go...
			//
			// suppose we go to state x
			// this is what we do:
			// 0_excitation     ---> send excitation input and calculate excitation for state x, reading feedback [if any] from x-1
			// 1_save audio     ---> in particular we want to read audio from listener position in state x-1, COS WE ARE SURE IT'S READY [FDTD for x will be computed later]; this explains the weird backwards mapping of the listener coordinates among the state quads [see setUpListener()]
			// 2_compute FDTD   ---> solve for state x. To solve we need excitation cell. To be sure that no one is still messing up with it while solving for state x, we read excitation from state x-1: this explains step 0 AND the weird backwards mapping of the excitation coordinates among the state quads [see fillVertices()]
			// 3_excitation_fu  ---> follow up excitation to calculate excitation for state x+1, reading feedback [if any] from x-1 and possibly excitation from x
			// 4_memory barrier ---> cos step 2 take the longest, so we need to be sure that all pixels are done before we restart the loop for next state [x+1]
			// ...
			// although painful, makes a lot of sense.
			// with this structure we have only a 3 sample per each buffer before the computed audio sample is actually sent to the audio card


			state = (state+1)%numOfStates; // offset value for next round
			// send excitation input
			glUniform1f(excitationInput_loc, inputBuffer[n*rate_mul+i]);
			// send other critical uniforms that need to be updated at every sample
			updateCriticalUniforms();

			//==========================================================================
			// 0_excitation ============================================================
			// send quad identifier for one excitation quads and draw FBO
			glUniform1i(quad_loc, quad_excitation_0 + state); // excitation quads ids are != in the shader from CPU, see MACRO function on top
			glDrawArrays (GL_TRIANGLE_STRIP, drawArraysValues[quad_excitation_0 + state][0], drawArraysValues[quad_excitation_0 + state][1]); // state+numOfStates+1 -> excitation quads must be offseted by the number of state quads + audio quad


			//==========================================================================
			// 1_save audio ============================================================
			if(i==rate_mul-1) {
				// prepare audio write coords [where on the audio row - what pixel and channel - the new sample should be saved]
				audioWriteCoords[0] = audioRowWriteTexCoord[0]; // audio column x coord
				audioWriteCoords[1] = audioRowWriteTexCoord[1]; // audio column y coord
				audioWriteCoords[2] = audioRowChannel; 	    // RGBA channel where to save audio

				// send coords
				glUniform3fv(audioWriteCoords_loc, 1, audioWriteCoords);

				// send quad identifier for audio quad and draw FBO
				glUniform1i(quad_loc, quad_audio + state); // audio quad id is != in the shader from CPU, see MACRO function on top
				glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_audio][0], drawArraysValues[quad_audio][1]);
				//checkError("glDrawArrays fbo");
				//------------------

				// update for next audio write coords
				audioRowChannel = (audioRowChannel+1)%4; // cos we have 4 available channels in each pixel where to store audio
				// increment pixel Y pos, once every 4, i.e., only when all the 4 channels in the pixel have been used
				if(audioRowChannel==0) {
					audioRowWriteTexCoord[0] += deltaCoordX;
					// if audio is split into more rows and it's time to move to the next one...
					if( (audioRowH > 1) && ((n+1)%singleRowAudioCapacity) == 0 ) {
						//...reposition write coord
						audioRowWriteTexCoord[0] = 0;			 // x goes back to 0
						audioRowWriteTexCoord[1] += deltaCoordY; // y increases of one row
					}
				}
			}


			//==========================================================================
			// 2_compute FDTD ==========================================================
			// send quad identifier for one of main domain quads and draw FBO
			glUniform1i(quad_loc, quad_0+state);
			glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_0+state][0], drawArraysValues[quad_0+state][1]);
			//checkError("glDrawArrays fbo");


			//==========================================================================
			// 3_excitation_fu =========================================================
			// excitation follow up quad
			glUniform1i(quad_loc, quad_excitation_fu_0+state);
			glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_excitation_fu_0+state][0], drawArraysValues[quad_excitation_fu_0+state][1]);


			//==========================================================================
			// 4_memory barrier ========================================================
			glMemoryBarrier(bits); // to re-sync all our parallel GPU threads [this is also called implicitly when buffers are swapped]
			//------------------


			#ifdef TIME_RT_FBO
				gettimeofday(&end_, NULL);
				elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
				printf("FBO cycle %d:%d\n", i, elapsedTime);

				printf("---------------------------------------------------------\n");
			#endif

			if(slowMotion && render) {
				// in slow motion mode we simply draw every frame and do all those checks that are done once for buffer when running at regular speed
				renderWindowInner();

/*				printf("%d Slow motion time: %f sec\t\tinput: %f\n", slowMo_step, slowMo_time, inputBuffer[n*rate_mul+i]);
				printf("state: %d\n", state);
				printf("-wait time: %f sec-\n", 0.000001*(slowMo_sleep_ns));*/
				slowMo_step++;
				slowMo_time += dt;
				usleep(slowMo_sleep_ns);
			}
		}

		//@debug
		//readAllTextureDebug();
	}
}

/*void CoupledFDTD::renderWindow() {
	if(render && !slowMotion) {
		#ifdef TIME_RT_RENDER
			gettimeofday(&start_, NULL);
		#endif
		renderWindowInner();

		#ifdef TIME_RT_RENDER
			gettimeofday(&end_, NULL);
			elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
			printf("FBO cycle %d:%d\n", i, elapsedTime);

			printf("---------------------------------------------------------\n");
		#endif

		// prepare FBO passes
		shaderFbo.use();
		//checkError("glUseProgram back to fbo");
		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to our framebuffer
		glViewport(0, 0, textureWidth, textureHeight);
	}
}*/

void CoupledFDTD::renderWindowInner() {
	//------------------
	// second pass, render texture on screen
	shaderRender.use();

	updateUniformsRender();

	//checkError("glUseProgram from fbo to screen");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);   // disable framebuffer rendering
#ifndef VISUAL_DEBUG
	glViewport(0, (-audioRowH-ISOLATION_LAYER)*magnifier, textureWidth*magnifier, textureHeight*magnifier);
#else
	glViewport(0, 0, textureWidth*magnifier, textureHeight*magnifier);
#endif

	glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_0][0], drawArraysValues[quad_0][1]);
#ifdef VISUAL_DEBUG
	glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_1][0], drawArraysValues[quad_1][1]);
	glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_2][0], drawArraysValues[quad_2][1]);
	glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_3][0], drawArraysValues[quad_3][1]);
	glDrawArrays (GL_TRIANGLE_STRIP, drawArraysValues[0+numOfStates+1][0], drawArraysValues[0+numOfStates+1][1]);
	glDrawArrays (GL_TRIANGLE_STRIP, drawArraysValues[1+numOfStates+1][0], drawArraysValues[1+numOfStates+1][1]);
	glDrawArrays (GL_TRIANGLE_STRIP, drawArraysValues[2+numOfStates+1][0], drawArraysValues[2+numOfStates+1][1]);
	glDrawArrays (GL_TRIANGLE_STRIP, drawArraysValues[3+numOfStates+1][0], drawArraysValues[3+numOfStates+1][1]);
	glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_audio][0], drawArraysValues[quad_audio][1]);
#endif
	//------------------

	// update other events like input handling
	glfwPollEvents ();
	// put the stuff we've been drawing onto the display
	glfwSwapBuffers (window);
}

void CoupledFDTD::getSamples(int numSamples) {
	if(!slowMotion) {
		//---------------------------------------------------------------------------------------------------------------------------
		// get data
		//---------------------------------------------------------------------------------------------------------------------------
		int readPixelsH = audioRowsUsed;
		int readPixelsW;

		if(audioRowH == 1)
			readPixelsW = (numSamples)/4; // 4, cos each pixel in the column contains 4 values [stored as RGBA]
		else
			readPixelsW = audioRowW; // multi row

		outputBuffer = pbo.getPixels(readPixelsW, readPixelsH);

		// reset simulation if gets mad!
		if(!isfinite(outputBuffer[0])) {
			printf("---out[0]: %.16f\n", outputBuffer[0]);

			reset();
			printf("Simulation went nuts, so I had to reset it_\n");
		}

		if(doCrossfade) {
			if(currentDeck==0) {
				currentDeck = 1;
				crossfade   = 1;
			}
			else {
				currentDeck = 0;
				crossfade   = 0;
			}
			glUniform1f(crossfade_loc, crossfade);
			doCrossfade    = false;
			updateListener = false;
		}
		//@VIC
		//VIC
	/*	static int a = 0;
		for(int n=0; n<numSamples; n++) {
			//printf("in[%d]: %f\n", n, inputBuffer[n]);
			//printf("---out[%d]: %.16f\n", n, outputBuffer[n]);

			//printf("---out[%d]: %.16f\n", a, outputBuffer[n]);
			printf("%.8f\n", outputBuffer[n]);
			a++;
			if(a==44100) {
				a=0;
				printf("STOP------------------------------------------------\n");
			}
		}*/
		//a++;
		//printf("---out[0]: %.16f\n", outputBuffer[0]);
		/*printf("---out[1]: %.16f\n", outputBuffer[1]);
		printf("---out[2]: %.16f\n", outputBuffer[2]);
		printf("---out[3]: %.16f\n", outputBuffer[3]);
		printf("-----------------\n");*/
		//---------------------------------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------------------------------
	}
	else
		memset(outputBuffer, 0, numSamples);

	resetAudioRow();
}

//TODO turn all bool into mutex lock
/*void CoupledFDTD::updatePixelDrawer() {
	if(resetPixels) {
		pixelDrawer->reset();
		resetPixels = false;
	}

	if(updatePixels) {
		pixelDrawer->update(state, updateExCellNum);
		updatePixels = false;
	}
}*/

//TODO turn all bool into mutex lock, here and in derived classes
void CoupledFDTD::updateUniforms() {
	// update number of excitation cells if modified
	if(updateExCellNum) {
		setUpExCellNum();
		if(!render)
			updateExCellNum = false;
	}
	// update listener in FBO if externally modified
	if(updateListener) {
		setUpListener();
		doCrossfade = true;
		updateListenerRender = true;
	}
	// update feedback in FBO if externally modified
	if(updateFeedback) {
		setUpFeedback();
		if(!render)
			updateFeedback = false;
	}

	// update max pressure if externally modified
	if(updateMaxPress) {
		setUpMaxPressure();
		updateMaxPress = false;
	}

	// update simulation parameters [ds, rho, etc] if externally modified
	if(updatePhysParams) {
		setUpPhysicalParams();
		updatePhysParams = false;
	}

	// update excitation type in FBO if externally modified
	if(updateExType) {
		setUpExcitationTypeFbo();
		if(!render)
			updateExType = false;
	}

	// update excitation direction in FBO if externally modified
	if(updateExDir) {
		setUpExcitationDirectionFbo();
		if(!render)
			updateExDir = false;
	}

	// update exictation low pass toggle if externally modified
	if(updateExLpToggle) {
		setUpExLpToggle();
		updateExLpToggle = false;
	}
}

void CoupledFDTD::updateUniformsRender() {
	if(updateListenerRender) {
		setUpListenerRender();
		updateListenerRender = false;
	}
	if(updateFeedback) {
		setUpFeedbackRender();
		updateFeedback = false;
	}

	// update excitation type in render shader if externally modified
	if(updateExType) {
		setUpExcitationTypeRender();
		updateExType = false;
	}

	// update excitation direction in render shader if externally modified
	if(updateExDir) {
		setUpExcitationDirectionRender();
		updateExDir = false;
	}

	// update number of excitation cells in render shader if externally modified
	if(updateExCellNum) {
		setUpExCellNumRender();
		updateExCellNum = false;
	}

	// update mouse position in render shader
	if(updateMouseCoord) {
		setUpMouseRender();
		updateMouseCoord = false;
	}
}

// this determines the size of what we read back from the texture
/*void CoupledFDTD::initPbo() {
	// the size here is crucial

	// whole texture
	pbo_allTexture.init( sizeof(float)*4*textureWidth*textureHeight);
	pbo_allTexture.unbind(); // bind this only if need to debug whole texture

	// audio quad
	pbo.init( sizeof(float)*period_size);
}*/

int CoupledFDTD::initPixelDrawer() {
	int numOfResetQuads = numOfExQuads+numOfExFolQuads+1;
	pixelDrawer = new PDrawerImplicitFDTD(gl_width, gl_height, shaderFbo, startFramePixels,
			                      numOfStates, numOfResetQuads, frameDuration, audio_rate, cell_excitation, cell_air, cell_wall);
/*	int drawArrays[13][2];
	// copy draw array values calculated in this class
	// main quads [drawing version]
	drawArrays[0][0] = drawArraysValues[quad_drawer_0][0];
	drawArrays[0][1] = drawArraysValues[quad_drawer_0][1];
	drawArrays[1][0] = drawArraysValues[quad_drawer_1][0];
	drawArrays[1][1] = drawArraysValues[quad_drawer_1][1];
	drawArrays[2][0] = drawArraysValues[quad_drawer_2][0];
	drawArrays[2][1] = drawArraysValues[quad_drawer_2][1];
	drawArrays[3][0] = drawArraysValues[quad_drawer_3][0];
	drawArrays[3][1] = drawArraysValues[quad_drawer_3][1];
	// excitation quads [for reset]
	drawArrays[4][0] = drawArraysValues[quad_excitation_0][0];
	drawArrays[4][1] = drawArraysValues[quad_excitation_0][1];
	drawArrays[5][0] = drawArraysValues[quad_excitation_1][0];
	drawArrays[5][1] = drawArraysValues[quad_excitation_1][1];
	drawArrays[6][0] = drawArraysValues[quad_excitation_2][0];
	drawArrays[6][1] = drawArraysValues[quad_excitation_2][1];
	drawArrays[7][0] = drawArraysValues[quad_excitation_3][0];
	drawArrays[7][1] = drawArraysValues[quad_excitation_3][1];
	// excitation follow up quad [for reset]
	drawArrays[8][0] = drawArraysValues[quad_excitation_fu_0][0];
	drawArrays[8][1] = drawArraysValues[quad_excitation_fu_0][1];
	drawArrays[9][0] = drawArraysValues[quad_excitation_fu_1][0];
	drawArrays[9][1] = drawArraysValues[quad_excitation_fu_1][1];
	drawArrays[10][0] = drawArraysValues[quad_excitation_fu_2][0];
	drawArrays[10][1] = drawArraysValues[quad_excitation_fu_2][1];
	drawArrays[11][0] = drawArraysValues[quad_excitation_fu_3][0];
	drawArrays[11][1] = drawArraysValues[quad_excitation_fu_3][1];
	// audio quad [for reset]
	drawArrays[12][0] = drawArraysValues[quad_audio][0];
	drawArrays[12][1] = drawArraysValues[quad_audio][1];*/


	int drawArraysMain[numOfStates][2];
	// copy draw array values calculated in this class
	// main quads [drawing version]
	drawArraysMain[0][0] = drawArraysValues[quad_drawer_0][0];
	drawArraysMain[0][1] = drawArraysValues[quad_drawer_0][1];
	drawArraysMain[1][0] = drawArraysValues[quad_drawer_1][0];
	drawArraysMain[1][1] = drawArraysValues[quad_drawer_1][1];
	drawArraysMain[2][0] = drawArraysValues[quad_drawer_2][0];
	drawArraysMain[2][1] = drawArraysValues[quad_drawer_2][1];
	drawArraysMain[3][0] = drawArraysValues[quad_drawer_3][0];
	drawArraysMain[3][1] = drawArraysValues[quad_drawer_3][1];


	int drawArraysReset[numOfResetQuads][2];
	// excitation quads [for reset]
	drawArraysReset[0][0] = drawArraysValues[quad_excitation_0][0];
	drawArraysReset[0][1] = drawArraysValues[quad_excitation_0][1];
	drawArraysReset[1][0] = drawArraysValues[quad_excitation_1][0];
	drawArraysReset[1][1] = drawArraysValues[quad_excitation_1][1];
	drawArraysReset[2][0] = drawArraysValues[quad_excitation_2][0];
	drawArraysReset[2][1] = drawArraysValues[quad_excitation_2][1];
	drawArraysReset[3][0] = drawArraysValues[quad_excitation_3][0];
	drawArraysReset[3][1] = drawArraysValues[quad_excitation_3][1];
	// excitation follow up quad [for reset]
	drawArraysReset[4][0] = drawArraysValues[quad_excitation_fu_0][0];
	drawArraysReset[4][1] = drawArraysValues[quad_excitation_fu_0][1];
	drawArraysReset[5][0] = drawArraysValues[quad_excitation_fu_1][0];
	drawArraysReset[5][1] = drawArraysValues[quad_excitation_fu_1][1];
	drawArraysReset[6][0] = drawArraysValues[quad_excitation_fu_2][0];
	drawArraysReset[6][1] = drawArraysValues[quad_excitation_fu_2][1];
	drawArraysReset[7][0] = drawArraysValues[quad_excitation_fu_3][0];
	drawArraysReset[7][1] = drawArraysValues[quad_excitation_fu_3][1];
	// audio quad [for reset]
	drawArraysReset[8][0] = drawArraysValues[quad_audio][0];
	drawArraysReset[8][1] = drawArraysValues[quad_audio][1];

	GLint textureA = nextAvailableTex;
	nextAvailableTex = nextAvailableTex+1;
	GLint textureB = nextAvailableTex;
	nextAvailableTex = nextAvailableTex+1;

	int retval = pixelDrawer->init(textureA, textureB, shaderLoc, drawArraysMain, drawArraysReset);

	return retval;
}

int CoupledFDTD::computeFeedbackPos(int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	feedbackCoord[0] = coords[0];
	feedbackCoord[1] = coords[1];
	return 0;
}

void CoupledFDTD::computeNextFeedbackFrag() {
	// HARDCODED
	// this is backwards mapping! no there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// quad 1      reads audio       from quad 0
	feedbFragCoord[1][0] = ((float)feedbackCoord[0]+0.5) / (float)textureWidth;
	feedbFragCoord[1][1] = ((float)feedbackCoord[1]+0.5) / (float)textureHeight;
	// quad 2      reads audio       from quad 1
	feedbFragCoord[2][0] = ((float)feedbackCoord[0]+0.5) / (float)textureWidth;
	feedbFragCoord[2][1] = (float)(feedbackCoord[1]+0.5+gl_height) / (float)textureHeight;
	// quad 3      reads audio       from quad 2
	feedbFragCoord[3][0] = (float)(feedbackCoord[0]+0.5+gl_width)  / (float)textureWidth;
	feedbFragCoord[3][1] = (float)(feedbackCoord[1]+0.5+gl_height) / (float)textureHeight;
	// quad 0      reads audio       from quad 3
	feedbFragCoord[0][0] = (float)(feedbackCoord[0]+0.5+gl_width)  / (float)textureWidth;
	feedbFragCoord[0][1] = ((float)feedbackCoord[1]+0.5) / (float)textureHeight;
}

int CoupledFDTD::computeFeedbackFrag(int x, int y) {
	if(computeFeedbackPos(x, y) != 0)
		return 1;

	// Calculate the position for the different states in texture coordinates
	computeNextFeedbackFrag();

	return 0;
}

/*
void CoupledFDTD::computeDeltaS(){
	TextureFilter::computeDeltaS();
	minDs = ds;
}
*/

// this calculates the new position of the listener in all the 4 states [that's why is filter specific] ---> values expected in domain reference [but saved as window, cos we have to deal with texture]!
int CoupledFDTD::computeMouseFrag(int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	// mouse coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 0      reads audio       from quad 3
	mouseFragCoord[0] = (float)(coords[0]+0.5+gl_width) / (float)textureWidth;
	mouseFragCoord[1] = ((float)coords[1]+0.5) / (float)textureHeight;

	// see ridiculous explanation of this mapping in getBuffer()

	// mouse pos is used only in render. since quad 0 is the only quad rendered, we read from quad 3 [previous]

	return 0;
}

void CoupledFDTD::computeExcitationLPCoeff(float &a1, float &a2, float &b0, float &b1, float &b2) {
	//double normCutFreq = 0.20; // frequency = 15% of the rate

	// second order low pass butterworth filter
	// taken from old BeagleRT IIR filter example
	double normCutOmega = 2*M_PI*exLpFreq/simulation_rate; // normalized cut angular velocity
	double denom = 4+2*sqrt(2.0)*normCutOmega+normCutOmega*normCutOmega;
	b0 = normCutOmega*normCutOmega/denom;
	b1 = 2*b0;
	b2 = b0;
	a1 = (2*normCutOmega*normCutOmega-8)/denom;
	a2 = (normCutOmega*normCutOmega+4-2*sqrt(2)*normCutOmega)/denom;

/*	alternative method...almost same shit
  	const double ita =1.0/ tan(M_PI*cutFreq);
	const double q=sqrt(2.0);
	b0 = 1.0 / (1.0 + q*ita + ita*ita);
	b1= 2*b0;
	b2= b0;
	a1 = 2.0 * (ita*ita - 1.0) * b0;
	a2 = -(1.0 - q*ita + ita*ita) * b0;
*/
}
/*
void CoupledFDTD::readAllTextureDebug() {
	pbo.unbind();
	pbo_allTexture.bind();

	//float *allTexture = pbo_allTexture.getPixels(textureWidth, textureHeight); // returns 4 channels per each pixel
	//checkError("pbo debug");


	//---------------------------------------------------------------------
	float ex[4][4];
	float ex_fu[4][4];

	float v[4];
	float x1[4];
	float x2[4];
	float xb[4];

	float x_1[4];
	float x_2[4];
	float x_b[4];
	float a1[4];

	float e0[4];
	float e1[4];
	float e2[4];
	float e3[4];

	float f0[4];
	float f1[4];
	float f2[4];
	float f3[4];

	for (int i =0; i<numOfExQuads; i++) {
		for (int j=0; j<4; j++) {
			ex[i][j] = allTexture[(textureWidth-numOfExQuads-numOfExFolQuads+i)*4 + j];
			ex_fu[i][j] = allTexture[(textureWidth-numOfExFolQuads+i)*4 + j];
		}
	}

	for (int i =0; i<numOfStates; i++) {
		v[i]  = ex[i][0];
		x1[i] = ex[i][1];
		x2[i] = ex[i][2];
		xb[i] = ex[i][3];

		x_1[i] = ex_fu[i][0];
		x_2[i] = ex_fu[i][1];
		x_b[i] = ex_fu[i][2];
		a1[i]  = ex_fu[i][3];
	}

	//---------------------------------------------------------------------


	pbo_allTexture.unbind();
	pbo.bind();
}

*/

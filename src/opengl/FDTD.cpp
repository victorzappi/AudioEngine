/*
 * FDTD.cpp
 *
 *  Created on: 2016-11-01
 *      Author: Victor Zappi
 */
#include "opengl/FDTD.h"
#include "opengl/Quad.h"
#include "priority_utils.h"

#include <iostream>
#include <stdio.h>
#include <assert.h>

#include <cstring> // memcopy
#include <math.h>  // ceil [not to be confused with Celi] and isfinite()---->with cmath the latter does not work!
#include <stdarg.h>

#include <unistd.h> // usleep, for debugging

using namespace std;

//------------------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------------------
FDTD::FDTD() : TextureFilter(){
	numOfStates = 2;

	state 			 = 0;
	audioRowChannel  = 0;
	printFormatCheck = false;

	name = "FDTD simulation";

	// VBO/VAO params
	numOfVerticesPerQuad = 4;  // 1 quad = 1 triangle + vertex = 4 vertices
	numOfAttributes      = 12; // see explanation in fillVertices() + it must be multiple of 4!

	// listener
    updateListener  = false;
    listenUnifName = "listenerFragCoord"; // we define the name of the first listener, so that parent class retrieves correct uniform location in TextureFilter::init(). The second one is hardcoded in initAdditionalUniforms()

    // hardcoded in whole structure...
    numOfPML            = 6;
    numOfDeadCellLayers = 0;
    numOfPML = (numOfPML>1) ? numOfPML : 0; // can't be 1, cos the first layer does not damp AT ALL!

    typeValues_loc = new GLint[cell_numOfTypes];

    // pressure normalization
	maxPressure     = 1000; // in Pascal
	maxPressure_loc = -1;
	maxPressure_loc_render = -1;
	updateMaxPress  = false;

	// physical params
	updatePhysParams  = false;
	minDs             = -1;
	ds                = -1;
	c                 = -1;
	c2_dt2_invDs2_loc = -1;

	// excitation
	inv_exCellNum_loc    = -1;
	exCellNum_loc_render = -1;
    excitationCellNum    = -1;
	updateExCellNum      = false;

	// pixel drawer
	pixelDrawer       = NULL;
	startFramePixels  = NULL;
	updatePixels      = false;
	updateFramePixels = false;
	resetPixels       = false;
	frameDuration     = -1;

	// slow motion
	slowMotion      = false;; // double semi colon = colon, no?
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

#if defined(TIME_RT_FBO) != defined(TIME_RT_RENDER)
	// time test
	elapsedTime = -1;
	start_      = timeval{0};
	end_        = timeval{0};
#endif
}

FDTD::~FDTD() {
	if(drawArraysValues != NULL) {
		for(int i=0; i<quad_numOfQuads; i++) {
			delete[] drawArraysValues[i];
		}
		delete[] drawArraysValues;
	}

	if(audioRowWriteTexCoord != NULL)
		delete[] audioRowWriteTexCoord;

	if(texturePixels != NULL)
		delete[] texturePixels;

	if(startFramePixels != NULL) {
		for(int i=0; i<gl_width; i++) {
			for(int j=0; j<gl_height; j++){
				delete[] startFramePixels[i][j];
			}
			delete[] startFramePixels[i];
		}
		delete[] startFramePixels;
	}

	if(vertices_data != NULL)
		delete[] vertices_data;

	if(typeValues_loc != NULL)
		delete[] typeValues_loc;

	if(pixelDrawer != NULL)
		delete pixelDrawer;
}

int FDTD::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
						float ***domain, int audio_rate, int rate_mul, int periodSize, float mag, int *listCoord) {
	preInit(shaderLocation, listCoord);
	excitationCellNum = excitationDimensions[0]*excitationDimensions[1]; // before TextureFilter::init(), cos this value is used inside initAdditionaUniforms(), that is called from TextureFilter::init()
	int sim_rate = TextureFilter::init(domainSize, numOfStates, excitationCoord, excitationDimensions, domain, audio_rate, rate_mul, periodSize, mag);

	// when using the other init overload, this is done when seDomain() is called from the outside
	if(initPixelDrawer() != 0)
		return -1;
	postInit();

	return sim_rate;
}

int FDTD::init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize, float mag, int *listCoord) {
	preInit(shaderLocation, listCoord);
	int sim_rate = TextureFilter::init(domainSize, numOfStates, audio_rate, rate_mul, periodSize, mag);
	postInit();

	return sim_rate;
}

int FDTD::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	excitationCellNum = excitationDimensions[0]*excitationDimensions[1]; // before TextureFilter::init(), cos this value is used inside initAdditionaUniforms(), that is called from TextureFilter::seteDomain()

	if(TextureFilter::setDomain(excitationCoord, excitationDimensions, domain) != 0)
		return 1;

	if(initPixelDrawer() != 0)
		return -1;

	return 0;
}

float *FDTD::getBufferFloat(int numSamples, float *inputBuffer) {
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
}

// returns listener pos in domain reference
void FDTD::getListenerPosition(int &x, int &y) {
	int pos[2] = {listenerCoord[0], listenerCoord[1]};
	fromTextureToDomain(pos);
	x = pos[0];
	y = pos[1];
}

// these 2 methods move in domain reference [shifting the window reference value stored in filter] and return in domain reference [again shifting the window reference value modified in filter]
int FDTD::shiftListenerV(int delta) {
	int pos[2] = {listenerCoord[0], listenerCoord[1]+delta};
	fromTextureToDomain(pos);
	if(setListenerPosition(pos[0], pos[1]) != 0)
		printf("Can't move listener here: (%d, %d)!\n", pos[0], pos[1]);

	return pos[1];
}

int FDTD::shiftListenerH(int delta){
	int pos[2] = {listenerCoord[0]+delta, listenerCoord[1]};
	fromTextureToDomain(pos);
	if(setListenerPosition(pos[0], pos[1]) != 0)
		printf("Can't move listener here: (%d, %d)!\n", pos[0], pos[1]);

	return pos[0];
}

int FDTD::setListenerPosition(int x, int y) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	if(computeListenerFrag(x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		return 1;

	updateListener = true;

	return 0;
}

int FDTD::setMaxPressure(float maxP) {
	if(inited) {
		maxPressure = maxP;
		updateMaxPress = true;
		return 0;
	}
	return -1;
}

// this is the only frame of reference transformation that should be visible to the user
// cos the user sees the raw window and knows what the domain is [she/he defined it]
int FDTD::fromRawWindowToDomain(int *coords) {
	// XD
	if( coords[0]<0 || coords[1]<0)
		return -1;

	// cope with magnification
	coords[0]/=magnifier;
	coords[1]/=magnifier;

	// apply shift
	coords[0]-=domainShift[0];
	coords[1]-=domainShift[1]/*-shiftkludge*/;

	return 0;
}

int FDTD::fromDomainToGlWindow(int *coords) {
	if(coords[0]<-domainShift[0] || coords[0]>domain_width+domainShift[0]-1  || coords[1]<-domainShift[1] || coords[1]>domain_height+domainShift[1]-1)
		return 1;

	// from domain reference to window reference, including the total texture shift coming from audio rows
	coords[0] = coords[0]+domainShift[0];
	coords[1] = coords[1]+domainShift[1];
	return 0;
}


// here we go from the domain [what the user uses] to the full texture frame of reference
int FDTD::fromDomainToTexture(int *coords) {
	if(coords[0]<-domainShift[0] || coords[0]>domain_width+domainShift[0]-1  || coords[1]<-domainShift[1] || coords[1]>domain_height+domainShift[1]-1)
		return 1;

	// from domain reference to texture reference, including the total texture shift coming from audio rows, isolation...
	coords[0] = coords[0]+domainShift[0]+textureShift[0];
	coords[1] = coords[1]+domainShift[1]+textureShift[1];
	return 0;
}

// here we go from the domain [what the user uses] to the full texture frame of reference
int FDTD::fromTextureToDomain(int *coords) {

	// from texture reference to domain
	int x = coords[0]-domainShift[0]-textureShift[0];
	int y = coords[1]-domainShift[1]-textureShift[1];

	if(x<-domainShift[0] || x>domain_width+domainShift[0]-1  || y<-domainShift[1] || y>domain_height+domainShift[1]-1)
		return 1;

	coords[0] = x;
	coords[1] = y;

	return 0;
}

float FDTD::setDS(float ds) {
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
}

int FDTD::setPhysicalParameters(void *params) {
	if(!inited)
		return -1;

	this->c   = ((float *)params)[0]; // only parameter in this case
    printf("\n");
    printf("\tc: %f\n", c);
	computeDeltaS(); // this is fundamental! when we modify physical params, also ds changes accordingly

	updatePhysParams = true;

	return 0;
}

// type is float, cos it can be cell_type [for a generic static cell] or a value in [-1, 0) [for a soft wall cell]
int FDTD::setCell(float type, int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

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

	return 0;
}

int FDTD::resetCell(int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

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

int FDTD::clearDomain() {
	if(!inited)
		return -1;
	pixelDrawer->clear();
	excitationCellNum = 0;
	updateExCellNum   = true;
	updatePixels      = true;

	return 0;
}

int FDTD::reset() {
	if(!inited)
		return -1;

	resetPixels = true;

	return 0;
}

int FDTD::setMousePosition(int x, int y) {
	if(!inited)
		return -1;

	computeMouseFrag(x, y); // we wanna be able to make it disappear, as opposed to the commented old version
	//if(computeMouseFrag(x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		//return 1;

	updateMouseCoord = true;

	return 0;
}

//TODO turn into private and use directly the enum when makes sense [especially outside]
// utility
// returns the cell type value in parametrized way [convenient to build PML]
float FDTD::getPixelAlphaValue(int type, int PMLlayer, int dynamicIndex, int numOfPMLlayers){
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
	default:
		retVal = -100;
	}

	return retVal;
}

void FDTD::fillTexturePixel(int x, int y, float value, float ***pixels) {
	pixels[x][y][0] = 0;
	pixels[x][y][1] = 0;
	pixels[x][y][2] = getPixelBlueValue((cell_type)value);
	pixels[x][y][3] = value;

}

void FDTD::fillTextureRow(int index, int start, int end, float value, float ***pixels) {
	for(int i=start; i<=end; i++) {
		pixels[i][index][0] = 0;
		pixels[i][index][1] = 0;
		pixels[i][index][2] = getPixelBlueValue((cell_type)value);
		pixels[i][index][3] = value;
	}
}

void FDTD::fillTextureColumn(int index, int start, int end, float value, float ***pixels) {
	for(int j=start; j<=end; j++) {
		pixels[index][j][0] = 0;
		pixels[index][j][1] = 0;
		pixels[index][j][2] = getPixelBlueValue((cell_type)value);
		pixels[index][j][3] = value;
	}
}




//------------------------------------------------------------------------------------------
// private methods ---> everything that defines the specific behavior of the filter is set here!
//------------------------------------------------------------------------------------------

void FDTD::preInit(string shaderLocation, int *listCoord) {
	shaderLoc  = shaderLocation;
	shaders[0] = shaderLocation+"/fbo_vs.glsl";
	shaders[1] = shaderLocation+"/fbo_fs.glsl";
	shaders[2] = shaderLocation+"/render_vs.glsl";
	shaders[3] = shaderLocation+"/render_fs.glsl";

	/*  Attributes:
	 *  2 for vertex pos (x, y) -> used only in vertex shader
	 *  2 for texture pos (x, y) of this cell at current state -> needed for FDTD explicit stencil
	 *  2 for texture pos (x, y) of left neighbor cell at current state -> needed for FDTD explicit stencil
	 *  2 for texture pos (x, y) of up neighbor cell at current state -> needed for FDTD explicit stencil
	 *  2 for texture pos (x, y) of right neighbor cell at current state -> needed for FDTD explicit stencil
	 *  2 for texture pos (x, y) of down neighbor cell at current state -> needed for FDTD explicit stencil
	 *
	 *  it should be a multiple of 4 cos we pass attributes packed up as vec4, to be more efficient
	 *  if we need a num of attributes that is not multiple of 4 the extra values are ignored.
	 *
	*/

	if(listCoord != NULL) {
		listenerCoord[0] = listCoord[0];
		listenerCoord[1] = listCoord[1];
	}
}

void FDTD::postInit() {
	minDs           = ds;
	slowMo_sleep_ns = slowMo_stretch/simulation_rate;
	frameDuration   = FRAME_DURATION;
}

void FDTD::initPhysicalParameters() {
    c = C_AIR;
    printf("\n");
    printf("\tc: \t%f\n", c);
    computeDeltaS();
}

// dynamically allocates the resources needed for our simulation domain
void FDTD::allocateResources() {
	drawArraysValues  = new int *[quad_numOfQuads];
	for(int i=0; i<quad_numOfQuads; i++) {
		drawArraysValues[i] = new int[2];
	}
	audioRowWriteTexCoord  = new float [numOfStates];
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


// here we set the values and offsets that characterize our image [size, quads displacement, etc]!
// the image is what goes into the texture, i.e., all the quads, contained in the frames or not
void FDTD::initImage(int domainWidth, int domainHeigth) {
	// we don't include PML in simulation domain, to make things easier when we want to update texture from outside

	gl_width  = domainWidth + 2*numOfPML + 2*numOfDeadCellLayers;
	gl_height = domainHeigth + 2*numOfPML + 2*numOfDeadCellLayers;

	// domain is inside window, shifted as follows
	domainShift[0] = (numOfPML+numOfDeadCellLayers);
	domainShift[1] = (numOfPML+numOfDeadCellLayers);


	textureWidth  = gl_width*2;
	textureHeight = gl_height;

	// the audio row is at the bottom portion of the texture, audio samples are stored there.
	// each pixel contains 4 samples.
	// there must be enough space to store all the buffer samples requested by audio card in one period,
	audioRowW = gl_width*2; // whole texture width - excitation and excitation follow up pixels, cos by default they are on same row as audio
	audioRowH = audioRowsUsed = 1;
	// so, now last row is numOfStates excitation pixels, numOfStates excitation follow up pixels and rest is audio row

	// if audio row space is not enough to store all the audio samples...
	if( period_size > (audioRowH*audioRowW*4) ) {
		//...we want an equal repartition of the audio samples on multiple rows, so that we can easily retrieve them in getBuffer()

		// first of all lets count how many fragments we need to save the whole sample buffer as fragments
		int numOfAudioFrags = period_size/4;

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

	textureShift[0] = 0;
	textureShift[1] = audioRowH+ISOLATION_LAYER;

	allocateResources();

	//-----------------------------------------------------
	// these are the chunks of data drawn in each quad
	drawArraysValues[quad_0][0] = 0;
	drawArraysValues[quad_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_1][0] = 4;
	drawArraysValues[quad_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_0][0] = 8;
	drawArraysValues[quad_drawer_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_1][0] = 12;
	drawArraysValues[quad_drawer_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_audio][0] = 16;
	drawArraysValues[quad_audio][1] = numOfVerticesPerQuad;
	//-----------------------------------------------------
}

// here we prepare the vao to get the vertices' data we defined in fillVertices()
void FDTD::initVao(GLuint vbo, GLuint shader_program_fbo, GLuint shader_program_render, GLuint &vao) {
	glGenVertexArrays (1, &vao);

	glBindVertexArray(vao);
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
}

// here we write the stuff we'll pass to the VBO
int FDTD::fillVertices() {
	int numOfDrawingQuads = numOfStates;
	// total number of quads: state quads + audio quad + drawing quads
	int numOfQuads = numOfStates+numOfDrawingQuads+1; // 1 is audio quad

	QuadExplicitFDTD quads[numOfQuads]; // save everything in here

	QuadExplicitFDTD quad0;
	QuadExplicitFDTD quad1;

	QuadExplicitFDTD drawingQuad0;
	QuadExplicitFDTD drawingQuad1;

	QuadExplicitFDTD quadAudio;

	// numOfStates to correctly calculate texture coordinates for excitation quads
	// numOfAttributes to allocate dynamic data structures where to save attributes
	// textureW/H since quads have to normalize pixel ranges, they need to know size of whole texture
	// deltaTexX/Y is to get neighbors
	quad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	drawingQuad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	quadAudio.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	// main quads... they are all the same [HARD CODED, change with numOfStates but not parametrically]
	//	pixels:	   top left coord (x, y)              width and height              x shift for texture coord N [texture coord N-1 is current]
	quad0.setQuad(0, textureHeight/*-1*/,                gl_width, gl_height, 		 	gl_width);  // left quad
	quad1.setQuad(gl_width, textureHeight/*-1*/,         gl_width, gl_height, 			-gl_width); // right quad

	// same vertices as main simulation quads...but they cover the whole texture area!
	drawingQuad0.setQuadDrawer(0, textureHeight/*-1*/,        gl_width, gl_height); // left quad
	drawingQuad1.setQuadDrawer(gl_width, textureHeight/*-1*/, gl_width, gl_height); // right quad

	// we can understand if one row is enough checking if the height of audio rows [means excitation cells are horizontally within last row and not vertically over multiple audio rows]
	if(audioRowH == 1)
		quadAudio.setQuadAudio(0, 0+1, textureWidth, 1);
	else
		quadAudio.setQuadAudio(0, audioRowH-1+1, audioRowW, audioRowH);


	// HARDCODED
	// put all quad in same array
	quads[0].copy(&quad0);
	quads[1].copy(&quad1);

	quads[numOfStates].copy(&drawingQuad0);
	quads[numOfStates+ 1].copy(&drawingQuad1);

	quads[numOfStates+numOfDrawingQuads].copy(&quadAudio);

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

// let's write the frame, i.e., the pixels of the domain+pml+dead cell layers that are repeated per each state!
// pass excitation and use some built in boundaries that also handle PML
void FDTD::initFrame(int *excitationCoord, int *excitationDimensions, float ***domain) {
	//-----------------------------------------------------
	// we start with preparing an empty frame, still without dead cells and PML
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

	// now we have to add dead cells and PML

	// top and bottom PML layers [leaving space for dead rows and columns at the extremes]
	cellType = cell_air;
	PMLlayer   = numOfPML+1;
	int toprow = gl_height-numOfDeadCellLayers;
	int btmrow = -1+numOfDeadCellLayers;
	int start  = -1+numOfDeadCellLayers;
	int stop   = gl_width-numOfDeadCellLayers;
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

	// top and bottom dead cell layers
	toprow = gl_height;
	btmrow = -1;
	start  = -1;
	stop   = gl_width;
	for(int i=0; i<numOfDeadCellLayers; i++) {
		toprow--;
		btmrow++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureRow(toprow, start, stop, val, startFramePixels);
		fillTextureRow(btmrow, start, stop, val, startFramePixels);
	}


	// left and right PML layers [leaving space for dead rows and columns at the extremes]
	PMLlayer     = numOfPML+1;
	int rightcol = gl_width-numOfDeadCellLayers;
	int leftcol  = -1+numOfDeadCellLayers;
	start        = -1+numOfDeadCellLayers;
	stop         = gl_height-numOfDeadCellLayers;
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

	// left and right dead cell layers
	rightcol = gl_width;
	leftcol  = -1;
	start    = -1;
	stop     = gl_height;
	for(int i=0; i<numOfDeadCellLayers; i++) {
		rightcol--;
		leftcol++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureColumn(rightcol, start, stop, val, startFramePixels);
		fillTextureColumn(leftcol, start, stop, val, startFramePixels);
	}

	// then we copy the full domain [received from outside] into the frame, preserving the dead cells and the PML structure!
	for(int i = 0; i <domain_width; i++) {
		for(int j = 0; j <domain_height; j++) {
			startFramePixels[numOfPML+numOfDeadCellLayers+i][numOfPML+numOfDeadCellLayers+j][3] = domain[i][j][3]; // the cell type is set from the outside
			startFramePixels[numOfPML+numOfDeadCellLayers+i][numOfPML+numOfDeadCellLayers+j][2] = getPixelBlueValue(/*cell_type*/(domain[i][j][3])); // we deduce the cell beta from it (:
			// to clarify, this is needed for the standard structure of the example FDTD,
			// where:
			// R is pressure [so we leave it empty at init time]
			// G is previous pressure [same]
			// B is beta, that allows for transition from wall to air and must have specific values for all the other types [that's why we need getPixelBlueValue]
			// A is type, which instead is passed from the outside in a usable from already [hopefully...]
		}
	}

	// finally we add excitation
	float excitationVal = getPixelAlphaValue(cell_excitation);
	for(int i = 0; i <excitationDimensions[0]; i++) {
		fillTextureColumn(numOfPML+numOfDeadCellLayers+excitationCoord[0]+i, numOfPML+numOfDeadCellLayers+excitationCoord[1],
						  numOfPML+numOfDeadCellLayers+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, startFramePixels);
		fillTextureColumn(numOfPML+numOfDeadCellLayers+excitationCoord[0]+i, numOfPML+numOfDeadCellLayers+excitationCoord[1],
						  numOfPML+numOfDeadCellLayers+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, startFramePixels);
	}
	//-----------------------------------------------------

	// and we conclude by putting the frame into the image [a.k.a. texture]
	fillImage(startFramePixels);

	return;
}

// this fills texture with passed pixels
void FDTD::fillImage(float ***pixels) {
	// texture is divided in quads, representing a state

	// wipe out everything
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			texturePixels[(i*textureWidth+j)*4]   = 0;
			texturePixels[(i*textureWidth+j)*4+1] = 0;
			texturePixels[(i*textureWidth+j)*4+2] = 0;
			texturePixels[(i*textureWidth+j)*4+3] = 0;
		}
	}

	// copy pixels in quad0 and clear out the rest of the texture
	// making sure to offset pixels to skip audio rows at bottom
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			if( (i<gl_height) && (j<gl_width) ) {
				// we skip the texture pixels where the audio is + the 1 line that isolates the frame from the audio
				texturePixels[((i+audioRowH+ISOLATION_LAYER)*textureWidth+j)*4+1] = pixels[j][i][1];
				texturePixels[((i+audioRowH+ISOLATION_LAYER)*textureWidth+j)*4+2] = pixels[j][i][2];
				texturePixels[((i+audioRowH+ISOLATION_LAYER)*textureWidth+j)*4+3] = pixels[j][i][3];
				// we copied everything but the first of the 4 elements, cos it contains no domain info [pressure will be save there, so it should start empty]
			}
		}
	}
}

// i'd like to be able to say something useful...
int FDTD::initAdditionalUniforms() {
	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// location of pressure propagation coefficient, PML values, etc.

	// get locations
	c2_dt2_invDs2_loc = shaderFbo.getUniformLocation("c2_dt2_invDs2");
	if(c2_dt2_invDs2_loc == -1) {
		printf("c2_dt2_invDs2 uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// cell types' beta and sigma prime dt
	string typeValues_uniform_name = "";
	for(int i=0; i<cell_numOfTypes; i++) {
		typeValues_uniform_name = "typeValues["+to_string(i)+"]";
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

	// location of number of excitation cells [to normalize input velocity]
	inv_exCellNum_loc = shaderFbo.getUniformLocation("inv_exCellNum");
	if(inv_exCellNum_loc == -1) {
		printf("exCellNum uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// set
	// fbo
	shaderFbo.use();

	// set once for all

	float *sigmasdt;
	if(!computePMLcoeff(sigmasdt))
		return -2;

/*
	//                           beta, 2*sigma*deltaT-s*s*deltaT*deltaT, 1 + 2*sigma*deltaT
	glUniform3f(typeValues_loc[0], 0, 0, 1); // wall
	glUniform3f(typeValues_loc[1], 1, 0, 1); // air
	glUniform3f(typeValues_loc[2], 1, 0, 1); // excitation
	for(int i=0; i<numOfPML; i++)	    // pml layers
		glUniform3f(typeValues_loc[cell_pml0+i], 1, 2*sigmasdt[i]-sigmasdt[i]*sigmasdt[i], 1+2*sigmasdt[i]);
	glUniform3f(typeValues_loc[numOfPML+3], 0, 0, 1);  // dynamic wall [starts as wall]
	glUniform3f(typeValues_loc[numOfPML+4], 1, 0, 1000000); // dead cell [no pressure]
*/

	//                          2*sigma*deltaT-s*s*deltaT*deltaT, 1 + 2*sigma*deltaT
	glUniform2f(typeValues_loc[0], 0, 1); // wall
	glUniform2f(typeValues_loc[1], 0, 1); // air
	glUniform2f(typeValues_loc[2], 0, 1); // excitation
	for(int i=0; i<numOfPML; i++)	    // pml layers
		glUniform2f(typeValues_loc[cell_pml0+i], 2*sigmasdt[i]-sigmasdt[i]*sigmasdt[i], 1+2*sigmasdt[i]);
	glUniform2f(typeValues_loc[numOfPML+3], 0, 1);  // dynamic wall [starts as wall]
	glUniform2f(typeValues_loc[numOfPML+4], 0, 1000000); // dead cell [no pressure]

	delete[] sigmasdt;

	setUpExCellNum();
	checkError("glUniform1f 1/excitationCellNum");

	// set dynamically
	setUpPhysicalParams();

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

void FDTD::resetAudioRow() {
	// audio starts at tex coordinates (0,0), always
	audioRowWriteTexCoord[0] = 0;
	audioRowWriteTexCoord[1] = 0;

	return;
}

int FDTD::computeListenerPos(int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	listenerCoord[0] = coords[0];
	listenerCoord[1] = coords[1];
	return 0;
}

void FDTD::computeNextListenerFrag() {
	// listener coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 1      reads audio       from quad 0
	listenerFragCoord[1][0] = (float)(listenerCoord[0]+0.5) / (float)textureWidth;
	listenerFragCoord[1][1] = (float)(listenerCoord[1]+0.5) / (float)textureHeight;
	// quad 0      reads audio       from quad 1
	listenerFragCoord[0][0] = (float)(listenerCoord[0]+0.5+gl_width) / (float)textureWidth;
	listenerFragCoord[0][1] = (float)(listenerCoord[1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()
}

// automatically called by TextureFilter::initUniforms()
// it calculates the new position of the listener in all the states [that's why is filter specific] ---> values expected in domain reference [but saved as window, cos we have to deal with texture]!
int FDTD::computeListenerFrag(int x, int y) {
	if(computeListenerPos(x, y)!=0)
		return -1;

	// and calculate the position to put in that deck
	computeNextListenerFrag();

	return 0;
}

void FDTD::computeSamples(int numSamples, float *inputBuffer) {
	const int bits = GL_TEXTURE_FETCH_BARRIER_BIT;

	// Iterate over each sample
	for(int n = 0; n < numSamples; n++) {

		updatePixelDrawerFrame(); // this needs sample level precision [but it's inline]

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
			// 0_save audio     ---> in particular we want to read audio from listener position in state x-1, COS WE ARE SURE IT'S READY [FDTD for x will be computed later]; this explains the weird backwards mapping of the listener coordinates among the state quads [see setUpListener()]
			// 1_compute FDTD   ---> solve for state x. To solve we need excitation cell. To be sure that no one is still messing up with it while solving for state x, we read excitation from state x-1: this explains step 0 AND the weird backwards mapping of the excitation coordinates among the state quads [see fillVertices()]
			// 3_memory barrier ---> cos step 1 take the longest, so we need to be sure that all pixels are done before we restart the loop for next state [x+1]
			// ...
			// although painful, makes a lot of sense.
			// with this structure we have only a 3 sample per each buffer before the computed audio sample is actually sent to the audio card


			state = (state+1)%numOfStates; // offset value for next round
			// send excitation input
			glUniform1f(excitationInput_loc, inputBuffer[n*rate_mul+i]);
			// send other critical uniforms that need to be updated at every sample
			updateCriticalUniforms();

			//==========================================================================
			// 0_save audio ============================================================
			if(i==rate_mul-1) {
				// prepare audio write coords [where on the audio row - what pixel and channel - the new sample should be saved]
				audioWriteCoords[0] = audioRowWriteTexCoord[0]; // audio column x coord
				audioWriteCoords[1] = audioRowWriteTexCoord[1]; // audio column y coord
				audioWriteCoords[2] = audioRowChannel; 	    // RGBA channel where to save audio

				// send coords
				glUniform3fv(audioWriteCoords_loc, 1, audioWriteCoords);

				// send quad identifier for audio quad and draw FBO
				glUniform1i(quad_loc, quad_audio + state); // audio quad id is != in the shader from CPU, see MACRO function on top
				glDrawArrays( GL_TRIANGLE_STRIP, drawArraysValues[quad_audio][0], drawArraysValues[quad_audio][1]);
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
			// 1_compute FDTD ==========================================================
			// send quad identifier for one of main domain quads and draw FBO
			glUniform1i(quad_loc, quad_0+state);
			glDrawArrays( GL_TRIANGLE_STRIP, drawArraysValues[quad_0+state][0], drawArraysValues[quad_0+state][1]);
			//checkError("glDrawArrays fbo");

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

void FDTD::renderWindow() {
	if(render && !slowMotion) {
		#ifdef TIME_RT_RENDER
			gettimeofday(&start_, NULL);
		#endif
		renderWindowInner();

		#ifdef TIME_RT_RENDER
			gettimeofday(&end_, NULL);
			elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
			printf("Render cycle: %d ns\n", elapsedTime);

			printf("---------------------------------------------------------\n");
		#endif

		// prepare FBO passes
		shaderFbo.use();
		//checkError("glUseProgram back to fbo");
		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to our framebuffer
		glViewport(0, 0, textureWidth, textureHeight);
	}
}

void FDTD::renderWindowInner() {
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

	glDrawArrays( GL_TRIANGLE_STRIP, drawArraysValues[quad_0][0], drawArraysValues[quad_0][1]);
#ifdef VISUAL_DEBUG
	glDrawArrays( GL_TRIANGLE_STRIP, drawArraysValues[quad_1][0], drawArraysValues[quad_1][1]);
	glDrawArrays( GL_TRIANGLE_STRIP, drawArraysValues[quad_audio][0], drawArraysValues[quad_audio][1]);
#endif
	//------------------

	// update other events like input handling
	glfwPollEvents();
	// put the stuff we've been drawing onto the display
	glfwSwapBuffers(window);
}

void FDTD::getSamples(int numSamples) {
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

		//VIC
		/*int a = 0;
		for(int n=0; n<numSamples; n++) {
			//printf("in[%d]: %f\n", n, inputBuffer[n]);
			printf("---out[%d]: %.16f\n", n, outputBuffer[n]);
			a++;
		}
		a++;*/
		//printf("---out[0]: %.16f\n", outputBuffer[0]);
		//---------------------------------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------------------------------
	}
	else
		memset(outputBuffer, 0, numSamples);

	resetAudioRow();
}

void FDTD::updatePixelDrawer() {
	if(resetPixels) {
		pixelDrawer->reset();
		resetPixels = false;
	}

	if(updatePixels) {
		pixelDrawer->update(state, updateExCellNum);
		updatePixels = false;
	}
}

//TODO turn all bool into mutex lock, here and in derived classes
void FDTD::updateUniforms() {
	// update number of excitation cells if modified
	if(updateExCellNum) {
		setUpExCellNum();
		if(!render)
			updateExCellNum = false;
	}

	// update listener in FBO if externally modified
	if(updateListener) {
		setUpListener();
		if(!render)
			updateListener = false;
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
}

void FDTD::updateUniformsRender() {
	if(updateListener) {
		setUpListenerRender();
		updateListener = false;
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
void FDTD::initPixelReader() {
	// the size here is crucial

	// whole texture, for debug
	pbo_allTexture.init( sizeof(float)*4*textureWidth*textureHeight);
	pbo_allTexture.unbind(); // bind this only if need to debug whole texture

	// audio quad
	pbo.init( sizeof(float)*period_size);
}

void FDTD::computeDeltaS(){
	ds = computeDs(dt, c);
	minDs = ds;

	printf("\n");
	printf("\tdt: %f\n", dt);
	printf("\tds: %f\n", ds);
}

float FDTD::computeDs(float _dt, float _c) {
	return _dt * _c * sqrt( (double)2.0 );
}
int FDTD::initPixelDrawer() {
	pixelDrawer = new PDrawerExplicitFDTD(gl_width, gl_height, shaderFbo, startFramePixels,
			                      numOfStates, 1, frameDuration, audio_rate, cell_excitation, cell_air, cell_wall);
	int drawArraysMain[numOfStates][2];
	// copy draw array values calculated in this class
	// main quads [drawing version]
	drawArraysMain[0][0] = drawArraysValues[quad_drawer_0][0];
	drawArraysMain[0][1] = drawArraysValues[quad_drawer_0][1];
	drawArraysMain[1][0] = drawArraysValues[quad_drawer_1][0];
	drawArraysMain[1][1] = drawArraysValues[quad_drawer_1][1];


	int drawArraysAudio[1][2];
	// audio quad [for reset]
	drawArraysAudio[0][0] = drawArraysValues[quad_audio][0];
	drawArraysAudio[0][1] = drawArraysValues[quad_audio][1];

	GLint textureA = nextAvailableTex;
	nextAvailableTex = nextAvailableTex+1;
	GLint textureB = nextAvailableTex;
	nextAvailableTex = nextAvailableTex+1;

	int retval = pixelDrawer->init(textureA, textureB, shaderLoc, drawArraysMain, drawArraysAudio);

	return retval;
}

// this calculates the new position of the listener in all the 2 states [that's why is filter specific] ---> values expected in domain reference [but saved as window, cos we have to deal with texture]!
int FDTD::computeMouseFrag(int x, int y) {
	int coords[2] = {x, y};
	fromDomainToTexture(coords); // we wanna be able to make it disappear
	//if(fromDomainToTexture(coords) != 0) // as opposed to this old version
		//return 1;

	// mouse coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 0      reads audio       from quad 1
	mouseFragCoord[0] = (float)(coords[0]+0.5+gl_width) / (float)textureWidth;
	mouseFragCoord[1] = ((float)coords[1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()

	// mouse pos is used only in render. since quad 0 is the only quad rendered, we read from quad 1 [previous]

	return 0;
}


void FDTD::readAllTextureDebug() {
	pbo.unbind();
	pbo_allTexture.bind();

	//float *allTexture = pbo_allTexture.getPixels(textureWidth, textureHeight); // returns 4 channels per each pixel
	//checkError("pbo debug");


	//---------------------------------------------------------------------
	/*float ex[4][4];
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
	}*/

	//---------------------------------------------------------------------


	pbo_allTexture.unbind();
	pbo.bind();
}

bool FDTD::computePMLcoeff(float * &coeff) {
	if(numOfPML<2) {
		printf("Problem! numOfPML is %d and it should be at least 2 [cos the first layer does not damp]!\n", numOfPML);
		return false;
	}

	coeff = new float[numOfPML];

	// set once for all
	float maxSigmaDt = 0.5;
	// in this case we add some hard coded attenuation layers to the regular 0 layer
	for(int i=0; i<numOfPML; i++) {
		coeff[i] = ( (float)(i) / (numOfPML-1) ) * maxSigmaDt;
		//printf("sigmas[%d]: %f\n", i, sigmasdt[i]);
	}

	return true;
}

// returns the beta value of the specific cell type [beta is "airness"]
float FDTD::getPixelBlueValue(int type) {
	if(type==cell_wall || type >= cell_dynamic)
		return 0;
	if(type>=cell_air && type<=cell_pml5)
		return 1;
	return -1;
}

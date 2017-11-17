/*
 * Quad.cpp
 *
 *  Created on: 2016-01-21
 *      Author: vic
 */

#include "opengl/Quad.h"
#include <stdio.h>
#include <string.h> // for my dear memset



Quad::Quad(){
	attributes      = NULL;
	numOfStates     = 1;
	numOfAttributes = 2;
	textureW        = 1;
	textureH        = 1;
	deltaTexX       = 0;
	deltaTexY       = 0;
}

Quad::~Quad(){
	deallocateStructures();
}

void Quad::init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy) {
	numOfStates 	= stateNum;
	numOfAttributes = attrNum;
	textureW 		= texW;
	textureH 		= texH;
	deltaTexX 		= deltaTx;
	deltaTexY 		= deltaTy;
}

void Quad::copy(Quad *q){
	numOfStates     = q->numOfStates;
	numOfAttributes = q->numOfAttributes;
	textureW        = q->textureW;
	textureH        = q->textureH;
	deltaTexX       = q->deltaTexX;
	deltaTexY       = q->deltaTexY;

	deallocateStructures();
	allocateStructures();

	int numOfentries = 4*numOfAttributes;
	for(int i=0; i<numOfentries; i++)
		attributes[i] = q->getAttribute(i);
}

//-----------------------------------------------------------------------------------------------------------------------------
// private
//-----------------------------------------------------------------------------------------------------------------------------
void Quad::allocateStructures() {
	attributes = new float[4*numOfAttributes]; // 4 vertices * numOfAttributes attributes...if you use triangles, num of vertices will always be 6
}

void Quad::deallocateStructures() {
	if(attributes != NULL) {
		delete[] attributes;
	}
}





//----------------------------------------------------------------------------------------------------------------------------------
QuadExplicitFDTD::QuadExplicitFDTD() : Quad() {
}

QuadExplicitFDTD::~QuadExplicitFDTD(){
}

void QuadExplicitFDTD::init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy) {
	numOfStates 	= stateNum;
	numOfAttributes = attrNum;
	textureW 		= texW;
	textureH 		= texH;
	deltaTexX 		= deltaTx;
	deltaTexY 		= deltaTy;

	if(numOfStates != 2)
		printf("Explicit FDTD method is designed for 2 states, not %d!\n", numOfStates);

	if(numOfAttributes != 12)
		printf("Explicit FDTD method is designed for 12 attributes, not %d!\n", numOfAttributes);
}

void QuadExplicitFDTD::setQuad(int *params) {
	int topleftX     = params[0];
	int topleftY     = params[1];
	int width        = params[2];
	int height       = params[3];
	int coordNShiftX = params[4];
	setQuad(topleftX, topleftY, width, height, coordNShiftX);
}

// receives coordinates of extremes and shift values in pixels, with origin in bottom left corner [all is HUMAN INTELLIGIBLE]
// then
// normalizes vertices positions [-1.0, 1.0] and tex coords [0.0, 1]
// this is for main quads [also audio, but audio is simpler...no need for neighbors]
void QuadExplicitFDTD::setQuad(int topleftX, int topleftY, int width, int height, int coordNShiftX) {

	deallocateStructures();
	allocateStructures();
	// start with vertices
	setVertexPositions(topleftX, topleftY, width, height);

	// now we work out tex coords for states N and N-1

	// tmp vars to save vertices positions normalized and scaled between [0.0, 1] == u v texture coord for state N+1
	float scaledVx;
	float scaledVy;

	// tmp vars to convert the pixel shift to frag shift [0.0, 1,0]
	float coordNShiftFragX;

	for(int i=0; i<4; i++) {
		// scale position
		scaledVx = (vertx[i]+1)/2.0f;
		scaledVy = (verty[i]+1)/2.0f;

		// convert shift to texture N
		coordNShiftFragX = ((float)coordNShiftX)/((float)textureW);

		// center texture N
		texCoord0x[i] = scaledVx + coordNShiftFragX;
		texCoord0y[i] = scaledVy;

		// left texture N
		texCoord1x[i] = texCoord0x[i] - deltaTexX;
		texCoord1y[i] = texCoord0y[i];

		// up texture N
		texCoord2x[i] = texCoord0x[i];
		texCoord2y[i] = texCoord0y[i] + deltaTexY;

		// right texture N
		texCoord3x[i] = texCoord0x[i] + deltaTexX;
		texCoord3y[i] = texCoord0y[i];

		// down texture N
		texCoord4x[i] = texCoord0x[i];
		texCoord4y[i] = texCoord0y[i] - deltaTexY;


		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];
	}
}

void QuadExplicitFDTD::setQuadAudio( int topleftX, int topleftY, int width, int height) {
	setQuad(topleftX, topleftY, width, height, 0);
}

// this is for excitation quads! They need coordinates to previous values and follow ups!
// consider that excitation cells can be displaced according different layouts,
// so this methods gets parameters to accommodate all the layouts
void QuadExplicitFDTD::setQuadDrawer(int topleftX, int topleftY, int width, int height) {
	deallocateStructures();
	allocateStructures();

	setVertexPositions(topleftX, topleftY, width, height);

	// now we work out tex coords


	// to cover the whole texture in the shader
	float normalPlane_texCoord[4][2] = { {0, 0}, // bottom left
										 {0, 1}, // top left
									     {1, 0}, // bottom right
									     {1, 1}  // top right
										};

	for(int i=0; i<4; i++) {
		// normalized tex coords
		texCoord0x[i] = normalPlane_texCoord[i][0];
		texCoord0y[i] = normalPlane_texCoord[i][1];

		// not used
		texCoord1x[i] = 0;
		texCoord1y[i] = 0;
		texCoord2x[i] = 0;
		texCoord2y[i] = 0;
		texCoord3x[i] = 0;
		texCoord3y[i] = 0;
		texCoord4x[i] = 0;
		texCoord4y[i] = 0;

		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
// private
//-----------------------------------------------------------------------------------------------------------------------------
void QuadExplicitFDTD::setVertexPositions(int topleftX, int topleftY, int width, int height) {
	// each quad is composed of 1 triangle + 1 vertex, i.e., 4 vertices
	// 4 vertices [bottom left, top left, bottom right, top right]

	// first triangle
	// top left
	//             (     this middle part is [0, 1]        )   then mapped to [-1, 1]
	vertx[1] = 2 * ( ((float)topleftX)/((float)textureW)   ) - 1; 	// this will never reach +1, cos it is x coordinate of top left vertex, think about it!
	verty[1] = 2 * ( ((float)topleftY/*+1*/)/((float)textureH) ) - 1;   // this will never reach -1, cos it is y coordinate of top left vertex, think about it!

	// bottom right -> add width and height to top left
	//        top left    (   this part turns sizes to [0, 1] )    then double up cos [-1, 1] has double the range
	vertx[2] = vertx[1] + ( ((float)width)/((float)textureW)  )  * 2;
	verty[2] = verty[1] - ( ((float)height)/((float)textureH) )  * 2;

	// bottom left
	vertx[0] = vertx[1];
	verty[0] = verty[2];

	// second triangle
	// top right
	vertx[3] = vertx[2];
	verty[3] = verty[1];
}




//----------------------------------------------------------------------------------------------------------------------------------

QuadImplicitFDTD::QuadImplicitFDTD() : Quad() {
}

QuadImplicitFDTD::~QuadImplicitFDTD(){
}

void QuadImplicitFDTD::init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy) {
	numOfStates 	= stateNum;
	numOfAttributes = attrNum;
	textureW 		= texW;
	textureH 		= texH;
	deltaTexX 		= deltaTx;
	deltaTexY 		= deltaTy;

	if(numOfStates != 4)
		printf("Implicit FDTD method is designed for 4 states, not %d!\n", numOfStates);

	if(numOfAttributes != 24)
		printf("Implicit FDTD method is designed for 24 attributes, not %d!\n", numOfAttributes);
}

void QuadImplicitFDTD::setQuad(int *params) {
	int topleftX       = params[0];
	int topleftY       = params[1];
	int width          = params[2];
	int height         = params[3];
	int coordNShiftX   = params[4];
	int coordNShiftY   = params[5];
	int coordN_1ShiftX = params[6];
	int coordN_1ShiftY = params[7];
	setQuad(topleftX, topleftY, width, height,
		coordNShiftX, coordNShiftY, coordN_1ShiftX, coordN_1ShiftY);
}

// receives coordinates of extremes and shift values in pixels, with origin in bottom left corner [all is HUMAN INTELLIGIBLE]
// then
// normalizes vertices positions [-1.0, 1.0] and tex coords [0.0, 1]
// this is for main quads [also audio, but audio is simpler...no need for neighbors]
void QuadImplicitFDTD::setQuad(int topleftX, int topleftY, int width, int height,
	    int coordNShiftX, int coordNShiftY, int coordN_1ShiftX, int coordN_1ShiftY) {

	deallocateStructures();
	allocateStructures();

	// start with vertices
	setVertexPositions(topleftX, topleftY, width, height);


/*	// first triangle
	// top left
	//             (     this middle part is [0, 1]        )   then mapped to [-1, 1]
	vertx[1] = 2 * ( ((float)topleftX)/((float)textureW)   ) - 1; 	// this will never reach +1, cos it is x coordinate of top left vertex, think about it!
	verty[1] = 2 * ( ((float)topleftY+1)/((float)textureH) ) - 1;   // this will never reach -1, cos it is y coordinate of top left vertex, think about it!

	// bottom right -> add width and height to top left
	//        top left    (   this part turns sizes to [0, 1] )    then double up cos [-1, 1] has double the range
	vertx[2] = vertx[1] + ( ((float)width)/((float)textureW)  )  * 2;
	verty[2] = verty[1] - ( ((float)height)/((float)textureH) )  * 2;

	// bottom left
	vertx[0] = vertx[1];
	verty[0] = verty[2];

	// second triangle
	// top right
	vertx[3] = vertx[2];
	verty[3] = verty[1];*/



	// now we work out tex coords for states N and N-1

	// tmp vars to save vertices positions normalized and scaled between [0.0, 1] == u v texture coord for state N+1
	float scaledVx;
	float scaledVy;

	// tmp vars to convert the pixel shift to frag shift [0.0, 1,0]
	float coordNShiftFragX;
	float coordNShiftFragY;
	float coordN_1ShiftFragX;
	float coordN_1ShiftFragY;

	for(int i=0; i<4; i++) {
		// scale position
		scaledVx = (vertx[i]+1)/2.0f;
		scaledVy = (verty[i]+1)/2.0f;

		// convert shift to texture N
		coordNShiftFragX = ((float)coordNShiftX)/((float)textureW);
		coordNShiftFragY = ((float)coordNShiftY)/((float)textureH);

		// center texture N
		texCoord0x[i] = scaledVx + coordNShiftFragX;
		texCoord0y[i] = scaledVy + coordNShiftFragY;

		// left texture N
		texCoord1x[i] = texCoord0x[i] - deltaTexX;
		texCoord1y[i] = texCoord0y[i];

		// up texture N
		texCoord2x[i] = texCoord0x[i];
		texCoord2y[i] = texCoord0y[i] + deltaTexY;

		// right texture N
		texCoord3x[i] = texCoord0x[i] + deltaTexX;
		texCoord3y[i] = texCoord0y[i];

		// down texture N
		texCoord4x[i] = texCoord0x[i];
		texCoord4y[i] = texCoord0y[i] - deltaTexY;

		// left up texture N
		texCoord5x[i] = texCoord0x[i] - deltaTexX;
		texCoord5y[i] = texCoord0y[i] + deltaTexY;

		// right down texture N
		texCoord6x[i] = texCoord0x[i] + deltaTexX;
		texCoord6y[i] = texCoord0y[i] - deltaTexY;



		// convert shift to texture N-1
		coordN_1ShiftFragX = ((float)coordN_1ShiftX)/((float)textureW);
		coordN_1ShiftFragY = ((float)coordN_1ShiftY)/((float)textureH);

		// center texture N-1
		texCoord7x[i] = scaledVx + coordN_1ShiftFragX;
		texCoord7y[i] = scaledVy + coordN_1ShiftFragY;

		// right texture N-1
		texCoord8x[i] = texCoord7x[i] + deltaTexX;
		texCoord8y[i] = texCoord7y[i];

		// up texture N-1
		texCoord9x[i] = texCoord7x[i];
		texCoord9y[i] = texCoord7y[i] + deltaTexY;


		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];

		attributes[index+12] = texCoord5x[i];
		attributes[index+13] = texCoord5y[i];
		attributes[index+14] = texCoord6x[i];
		attributes[index+15] = texCoord6y[i];

		attributes[index+16] = texCoord7x[i];
		attributes[index+17] = texCoord7y[i];
		attributes[index+18] = texCoord8x[i];
		attributes[index+19] = texCoord8y[i];

		attributes[index+20] = texCoord9x[i];
		attributes[index+21] = texCoord9y[i];
		attributes[index+22] = 0; // not used, but needed to make num of attr multiple of 4 [so that they can be quickly packed in vec4]
		attributes[index+23] = 0; // not used, same
	}
}

void QuadImplicitFDTD::setQuadAudio( int topleftX, int topleftY, int width, int height) {
	setQuad(topleftX, topleftY, width, height, 0, 0, 0, 0);
}

// this is for excitation quads! They need coordinates to previous values and follow ups!
// consider that excitation cells can be displaced according different layouts,
//so this method gets parameters to accommodate all the layouts
void QuadImplicitFDTD::setQuadExcitation(int topleftX, int topleftY, int width, int height,
	int prevShiftX, int prevShiftY, int folShiftX, int folShiftY, int quadIndex) {
	deallocateStructures();
	allocateStructures();

	setVertexPositions(topleftX, topleftY, width, height);

/*    // first triangle
	// top left
	//             (     this middle part is [0, 1]        )   then mapped to [-1, 1]
	vertx[1] = 2 * ( ((float)topleftX)/((float)textureW)   ) - 1; 	// this will never reach +1, cos it is x coordinate of top left vertex, think about it!
	verty[1] = 2 * ( ((float)topleftY+1)/((float)textureH) ) - 1;   // this will never reach -1, cos it is y coordinate of top left vertex, think about it!

	// bottom right -> add width and height to top left
	//        top left    (   this part turns sizes to [0, 1] )    then double up cos [-1, 1] has double the range
	vertx[2] = vertx[1] + ( ((float)width)/((float)textureW)  )  * 2;
	verty[2] = verty[1] - ( ((float)height)/((float)textureH) )  * 2;

	// bottom left
	vertx[0] = vertx[1];
	verty[0] = verty[2];

	// second triangle
	// top right
	vertx[3] = vertx[2];
	verty[3] = verty[1];*/



	// now we work out tex coords for previous excitations and followups

	// tmp vars to save vertices positions normalized and scaled between [0.0, 1] == u v texture coord for state N+1
	float scaledVx;
	float scaledVy;

	// tmp vars to convert the pixel shift to frag shift [0.0, 1,0]
	float prevShiftFragX;
	float prevShiftFragY;
	float folShiftFragX;
	float folShiftFragY;

	int nextIndex;
	float nextCoordX;
	float nextCoordY;

	for(int i=0; i<4; i++) {
		// scale position
		scaledVx = (vertx[i]+1)/2.0f;
		scaledVy = (verty[i]+1)/2.0f;

		// convert shift to previous excitations [step to go from N+1 to N, from N to N-1 and so on...]
		prevShiftFragX = ((float)prevShiftX)/((float)textureW);
		prevShiftFragY = ((float)prevShiftY)/((float)textureH);

		// convert shift to follow up excitations [step to go from excitation N to excitation follow up N, from ex N-1 to fol N-1, and so on...]
		folShiftFragX = ((float)folShiftX)/((float)textureW);
		folShiftFragY = ((float)folShiftY)/((float)textureH);

		nextIndex  = quadIndex;
		nextCoordX = scaledVx;
		nextCoordY = scaledVy;

		float textCoords[numOfAttributes-8]; // excitation and follow up use only a subset of the max num of attributes
		memset(textCoords, 0, numOfAttributes-8);

		//  ___ ___ ___ ___
		// |N+1|N-2|N-1| N | example of excitation displacement for quadIndex = 0 [first quad is N+1] and numOfStates = 4
		//
		// starting from N+1 [index = 0], the next excitation is N-2...

		int prev_index;
		for(int i=0; i<numOfStates-1; i++) {
			// starts from closest cell [state = N-(numOfStates-2)] to furthest [state = N]
			prev_index = (numOfStates-2-i)*4; // this is how the state is linked to the index of the texture coords
			// excitation texture
			nextExcitation(prevShiftFragX, prevShiftFragY, nextIndex, nextCoordX, nextCoordY);
			textCoords[prev_index]   = nextCoordX;
			textCoords[prev_index+1] = nextCoordY;

			// excitation follow up texture
			textCoords[prev_index+2] = textCoords[prev_index] + folShiftFragX;
			textCoords[prev_index+3] = textCoords[prev_index+1] + folShiftFragY;
		}
		// ***this is to reach N+1 follow up, which seems stupid but it might be not
		textCoords[12] = scaledVx + folShiftFragX;
		textCoords[13] = scaledVy + folShiftFragY;

		// copy values calculated up there
		// coord N
		texCoord0x[i] = textCoords[0];
		texCoord0y[i] = textCoords[1];
		// coord N follow up
		texCoord1x[i] = textCoords[2];
		texCoord1y[i] = textCoords[3];
		// coord N-1
		texCoord2x[i] = textCoords[4];
		texCoord2y[i] = textCoords[5];
		// coord N-1 follow up
		texCoord3x[i] = textCoords[6];
		texCoord3y[i] = textCoords[7];
		// coord N-2
		texCoord4x[i] = textCoords[8];
		texCoord4y[i] = textCoords[9];
		// coord N-2 follow up
		texCoord5x[i] = textCoords[10];
		texCoord5y[i] = textCoords[11];
		// coord N+1 follow up! // read comment at ***
		texCoord6x[i] = textCoords[12];
		texCoord6y[i] = textCoords[13];
		// not used
		texCoord7x[i] = 0;
		texCoord7y[i] = 0;
		texCoord8x[i] = 0;
		texCoord8y[i] = 0;
		texCoord9x[i] = 0;
		texCoord9y[i] = 0;


		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];

		attributes[index+12] = texCoord5x[i];
		attributes[index+13] = texCoord5y[i];
		attributes[index+14] = texCoord6x[i];
		attributes[index+15] = texCoord6y[i];

		attributes[index+16] = texCoord7x[i];
		attributes[index+17] = texCoord7y[i];
		attributes[index+18] = texCoord8x[i];
		attributes[index+19] = texCoord8y[i];

		attributes[index+20] = texCoord9x[i];
		attributes[index+21] = texCoord9y[i];
		attributes[index+22] = 0; // not used, but needed to make num of attr multiple of 4 [so that they can be quicly packed in vec4]
		attributes[index+23] = 0; // not used, same
	}
}

// this is for excitation quads! They need coordinates to previous values and follow ups!
// consider that excitation cells can be displaced according different layouts,
// so this methods gets parameters to accommodate all the layouts
void QuadImplicitFDTD::setQuadDrawer(int topleftX, int topleftY, int width, int height) {
	deallocateStructures();
	allocateStructures();

	setVertexPositions(topleftX, topleftY, width, height);

    /*// first triangle
	// top left
	//             (     this middle part is [0, 1]        )   then mapped to [-1, 1]
	vertx[1] = 2 * ( ((float)topleftX)/((float)textureW)   ) - 1; 	// this will never reach +1, cos it is x coordinate of top left vertex, think about it!
	verty[1] = 2 * ( ((float)topleftY+1)/((float)textureH) ) - 1;   // this will never reach -1, cos it is y coordinate of top left vertex, think about it!

	// bottom right -> add width and height to top left
	//        top left    (   this part turns sizes to [0, 1] )    then double up cos [-1, 1] has double the range
	vertx[2] = vertx[1] + ( ((float)width)/((float)textureW)  )  * 2;
	verty[2] = verty[1] - ( ((float)height)/((float)textureH) )  * 2;

	// bottom left
	vertx[0] = vertx[1];
	verty[0] = verty[2];

	// second triangle
	// top right
	vertx[3] = vertx[2];
	verty[3] = verty[1];*/




	// now we work out tex coords


	// to cover the whole texture in the shader
	float normalPlane_texCoord[4][2] = { {0, 0}, // bottom left
										 {0, 1}, // top left
									     {1, 0}, // bottom right
									     {1, 1}  // top right
										};

	for(int i=0; i<4; i++) {
		// normalized tex coords
		texCoord0x[i] = normalPlane_texCoord[i][0];
		texCoord0y[i] = normalPlane_texCoord[i][1];

		// not used
		texCoord1x[i] = 0;
		texCoord1y[i] = 0;
		texCoord2x[i] = 0;
		texCoord2y[i] = 0;
		texCoord3x[i] = 0;
		texCoord3y[i] = 0;
		texCoord4x[i] = 0;
		texCoord4y[i] = 0;
		texCoord5x[i] = 0;
		texCoord5y[i] = 0;
		texCoord6x[i] = 0;
		texCoord6y[i] = 0;
		texCoord7x[i] = 0;
		texCoord7y[i] = 0;
		texCoord8x[i] = 0;
		texCoord8y[i] = 0;
		texCoord9x[i] = 0;
		texCoord9y[i] = 0;


		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];

		attributes[index+12] = texCoord5x[i];
		attributes[index+13] = texCoord5y[i];
		attributes[index+14] = texCoord6x[i];
		attributes[index+15] = texCoord6y[i];

		attributes[index+16] = texCoord7x[i];
		attributes[index+17] = texCoord7y[i];
		attributes[index+18] = texCoord8x[i];
		attributes[index+19] = texCoord8y[i];

		attributes[index+20] = texCoord9x[i];
		attributes[index+21] = texCoord9y[i];
		attributes[index+22] = 0; // not used, but needed to make num of attr multiple of 4 [so that they can be quicly packed in vec4]
		attributes[index+23] = 0; // not used, same
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
// private
//-----------------------------------------------------------------------------------------------------------------------------
void QuadImplicitFDTD::setVertexPositions(int topleftX, int topleftY, int width, int height) {
	// each quad is composed of 1 triangle + 1 vertex, i.e., 4 vertices
	// 4 vertices [bottom left, top left, bottom right, top right]

	// first triangle
	// top left
	//             (     this middle part is [0, 1]        )   then mapped to [-1, 1]
	vertx[1] = 2 * ( ((float)topleftX)/((float)textureW)   ) - 1; 	// this will never reach +1, cos it is x coordinate of top left vertex, think about it!
	verty[1] = 2 * ( ((float)topleftY/*+1*/)/((float)textureH) ) - 1;   // this will never reach -1, cos it is y coordinate of top left vertex, think about it!

	// bottom right -> add width and height to top left
	//        top left    (   this part turns sizes to [0, 1] )    then double up cos [-1, 1] has double the range
	vertx[2] = vertx[1] + ( ((float)width)/((float)textureW)  )  * 2;
	verty[2] = verty[1] - ( ((float)height)/((float)textureH) )  * 2;

	// bottom left
	vertx[0] = vertx[1];
	verty[0] = verty[2];

	// second triangle
	// top right
	vertx[3] = vertx[2];
	verty[3] = verty[1];
}

// according to what excitation cell we are handling, the next one [the neighbor that contains prev val]
// might be at the beginning of the row/column of excitation cells
void QuadImplicitFDTD::nextExcitation(float prevShiftFragX, float prevShiftFragY, int &nextIndex, float &nextCoordX, float &nextCoordY) {
	// this is regular procedure to point to neighbor, according to parameters passed to setQuadExcitation() method
	nextIndex++;
	nextCoordX += prevShiftFragX;
	nextCoordY += prevShiftFragY;
	// if we are going outside of the quad, go back to 0!
	if(nextIndex >= numOfStates) {
		nextCoordX -= numOfStates*prevShiftFragX;
		nextCoordY -= numOfStates*prevShiftFragY;
		nextIndex = 0;
	}
}

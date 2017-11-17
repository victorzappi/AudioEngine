/*
 * PixelDrawer.cpp
 *
 *  Created on: 2016-04-09
 *      Author: vic
 */

#include "opengl/PixelDrawer.h"
#include "opengl/CoupledFDTD.h" // for cell type
#include "strnatcmp.h" // for natural sorting


#include <cstring>   // memset, memcpy
#include <math.h>    // fabs
#include <fstream>   // read/write to file ifstream/ofstream
#include <sstream>   // ostringstream
#include <dirent.h>  // to handle files in dirs
#include <algorithm> // sort


enum operation_ {operation_oneShot, operation_crossfade, operation_formerExcite, operation_reset};

PixelDrawer::PixelDrawer(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
		float frameDuration, int audioRate, int exCType, int airCType, int wallCType) : pboA(textureA), pboB(textureB), shaderFbo(shader_fbo) {
	shaders[0] = "";
	shaders[1] = "";

	operation_loc = 0;

	crossfade     = 0;
	crossfade_loc = 0;
	deck 		  = 0;
	deck_loc 	  = 0;

	audio_rate     = audioRate;
	crossIncrement = 1.0/(audio_rate*frameDuration); // this means the frame crossfade should last frameDuration, no matter what audio rate we use

	currentPbo = &pboA;

	gl_width  = w;
	gl_height = h;

	numOfStates     = stateNum;
	numOfResetQuads = resetQuadsNum;

	drawArraysMainQuads  = new int *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		drawArraysMainQuads[i] = new int[2];
	drawArraysResetQuads = new int *[numOfResetQuads];
	for(int i=0; i<numOfResetQuads; i++)
		drawArraysResetQuads[i] = new int[2];

	// allocate and wipe
	framePixelsA = new GLfloat[gl_width*gl_height];
	memset(framePixelsA, 0, sizeof(GLfloat)*gl_width*gl_height);

	framePixelsB = new GLfloat[gl_width*gl_height];
	memset(framePixelsB, 0, sizeof(GLfloat)*gl_width*gl_height);

	currentFramePixels = framePixelsA;

	frameExciteNum = -1;

	excitationCellType = exCType;
	airCellType        = airCType;
	wallCellType       = wallCType;

	removeFormerExciteCells = false;

	allWallBetas = 0; // airness of walls
	updateAllWallBetas = false;

	// create and save a copy of the original image [will come handy for clear()]
	// it still contains possible initial excitation and walls, which will be later wiped in init
	clearFramePixels = new GLfloat[gl_width*gl_height];
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++)
			clearFramePixels[i*gl_width+j] = (GLfloat)startFrame[j][i][3]; // all air + PML + dead layers
	}
}

PixelDrawer::~PixelDrawer() {
	for(int i=0; i<numOfStates; i++)
		delete[] drawArraysMainQuads[i];
	delete[] drawArraysMainQuads;
	for(int i=0; i<numOfResetQuads; i++)
		delete[] drawArraysResetQuads[i];
	delete[] drawArraysResetQuads;

	delete[] framePixelsA;
	delete[] framePixelsB;
	delete[] clearFramePixels;
}


void PixelDrawer::clear() {
	memcpy(currentFramePixels, clearFramePixels, sizeof(GLfloat)*gl_width*gl_height); // copy from clear pixels that we wisely saved in constructor
}

int PixelDrawer::init(GLint active_unitA, GLint active_unitB, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]) {
	clear();
	//textureA.loadPixels(gl_width, gl_height, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, framePixelsA, active_unit);
	textureA.loadPixels(gl_width, gl_height, GL_R32F, GL_RED, GL_FLOAT, framePixelsA, active_unitA);
	textureB.loadPixels(gl_width, gl_height, GL_R32F, GL_RED, GL_FLOAT, framePixelsB, active_unitB);

	// verify type and format
	if(false) {
		GLint textureInternalFormat = GL_R32F;
		printf("\nFormat check for: %d\n", textureInternalFormat);

		GLint format, type;

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);
	}

	initShaders(shaderLocation);

	shaderDraw.use();

	// these is the drawing textures we will use in the shader to update the main texture [color attachment of FBO]
	glUniform1i(shaderDraw.getUniformLocation("drawingTextureA"), active_unitA - GL_TEXTURE0);
	glUniform1i(shaderDraw.getUniformLocation("drawingTextureB"), active_unitB - GL_TEXTURE0);

	operation_loc = shaderDraw.getUniformLocation("operation");
	glUniform1i(operation_loc, operation_oneShot); // most likely case
	if(operation_loc == -1)
		printf("operation uniform can't be found in pixel drawer fragment shader program\n");

	crossfade_loc = shaderDraw.getUniformLocation("crossfade");
	glUniform1f(crossfade_loc, crossfade); // most likely case
	if(crossfade_loc == -1)
		printf("crossfade uniform can't be found in pixel drawer fragment shader program\n");

	deck_loc = shaderDraw.getUniformLocation("deck");
	glUniform1i(deck_loc, deck); // most likely case
	if(deck_loc == -1)
		printf("deck uniform can't be found in pixel drawer fragment shader program\n");

	shaderFbo.use();

	pboA.init(gl_width, gl_height, framePixelsA);
	pboB.init(gl_width, gl_height, framePixelsB);

	// copy vertices indices and strides for the 4 drawing quads and 4 excitation quads and 4 excitation follow up quads and audio quad
	for(int i=0; i<numOfStates; i++) {
		drawArraysMainQuads[i][0] = drawArraysMain[i][0];
		drawArraysMainQuads[i][1] = drawArraysMain[i][1];
	}
	for(int i=0; i<numOfResetQuads; i++) {
		drawArraysResetQuads[i][0] = drawArraysReset[i][0];
		drawArraysResetQuads[i][1] = drawArraysReset[i][1];
	}

	int retval = pboA.trigger(); // test if working

	// apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	retval = pboB.trigger(); // test if working

	// same, apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	// transform the init image into a clear image!
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			// turn into air any excitation and wall cells...cos we do not want them in the original clear frame [they make noise q:]
			if(clearFramePixels[i*gl_width+j] == excitationCellType || clearFramePixels[i*gl_width+j] == wallCellType)
				clearFramePixels[i*gl_width+j] = airCellType;
		}
	}
	return retval;
}

bool PixelDrawer::loadFrame(frame *f) {
	//@VIC
	//crossIncrement =  (crossIncrement/abs(crossIncrement)) *  (1.0/(audio_rate*0.01));
	// check if we are opening a frame that has the same size as the current one!
	if( (f->width != gl_width) || (f->height != gl_height) ) {
		printf("Heck! Trying to open a frame with size different from current simulation domain!\n");
		return false;
	}

	// swap pointers and target deck
	if(deck==0) {
		currentPbo     = &pboB;
		currentFramePixels  = framePixelsB;
		deck           = 1;
		crossfade	   = 0; // just in case
		crossIncrement = fabs(crossIncrement); // goes forward
	}
	else {
		currentPbo     = &pboA;
		currentFramePixels  = framePixelsA;
		deck           = 0;
		crossfade	   = 1; // just in case
		crossIncrement = -fabs(crossIncrement); // goes backwards
	}
	memcpy(currentFramePixels, f->pixels, f->width*f->height*sizeof(float)); // copy the new frame image

	frameExciteNum = f->numOfExciteCells; // will send this back when frame is fully loaded

	return true;
}

//TODO consider moving all crossfade update into main fbo shader...move there the 2 extra textures and simply crossfade with built in shader function
// excitation cells handling can stay here
int PixelDrawer::updateFrame(int state, int &exciteNum) {
	// increment and check end
	crossfade += crossIncrement;
	bool stillWorking = true;
	if(crossfade >= 1) {
		crossfade = 1;
		stillWorking = false;
		//printf("crossfade %f\n", crossfade);
	}
	if(crossfade <= 0.0) {
		crossfade = 0.0;
		stillWorking = false;
		//printf("crossfade %f\n", crossfade);
	}

	//printf("crossfade %f\n", crossfade);

	currentPbo->trigger();

	shaderDraw.use();

	if(!removeFormerExciteCells) {
		glUniform1i(operation_loc, operation_crossfade); // crossfade operation
		glUniform1f(crossfade_loc, crossfade);           // crossfade value

		// we don't touch pressure and velocities [channels rgb]...we can do this because excitation cells that disappear are treated separately [and they need pressure and vel to be set to 0!]
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glTextureBarrierNV();
	}
	// at the very end, update excitation cells that disappear are treated separately [putting pressure and velocity to 0]
	else {
		glUniform1i(operation_loc, operation_formerExcite); // update cells that turned from excitation to air
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
		glTextureBarrierNV();
	}

	shaderFbo.use();

	// doing crossfade
	if(stillWorking)
		return 0;
	// crossfade ended but...
	else {
		// ...if we still have to update former excitations, do it
		if(!removeFormerExciteCells) {
			removeFormerExciteCells = true;
			return 0;
		}
		else
			removeFormerExciteCells = false;

		//... maybe a sequence is playing
		if(frameSequencer.getIsPlaying()) {
			frame *f = frameSequencer.getNextFrame();
			exciteNum = frameExciteNum; // we update num of excitation cells
			// if there is a next frame, load it
			if(f != NULL) {
				loadFrame(f);
				return 1;
			}
			// if it was last frame, we're done
			else{
				frameSequencer.stop();
				return 2;
			}
		}
		// ...if there is no sequence playing, we're done!
		else {
			exciteNum = frameExciteNum; // we update num of excitation cells
			return 2;
		}
	}
}

GLfloat PixelDrawer::getCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return currentFramePixels[y*gl_width+x];
}

int PixelDrawer::setCellType(GLfloat type, int x, int y) {

	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	// nothing to change
	if(currentFramePixels[y*gl_width+x] == type)
		return 1;

	currentFramePixels[y*gl_width+x] = type;

	//printf(" = %f\n", type);

	return 0;
}

int PixelDrawer::resetCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	// nothing to change
	if(currentFramePixels[y*gl_width+x] == clearFramePixels[y*gl_width+x])
		return 1;

	currentFramePixels[y*gl_width+x] = clearFramePixels[y*gl_width+x]; // back to original value

	return 0;
}

int PixelDrawer::openFrameSequence(string filename, bool isBinary) {
	if(isBinary) {
		if(!frameSequencer.loadFrameSequence(filename))
			return 1;
		//@VIC
		//frameSequencer.setFrameDuration(0.05);
		//frameSequencer.setPlayBackMode(playback_backAndForth);
		crossIncrement = 1.0/(audio_rate*frameSequencer.getFrameDuration()); // use the one saved in the frameSequence
	}
	//filename supposedly with no extension
	else {
		int retval = frameSequencer.compute(filename, gl_width, gl_height, 1.0/(audio_rate*crossIncrement), excitationCellType); // we pass the frame duration related to current crossIncrement
		if(retval>0)
			return retval;
	}
	// try to load first frame
	frame *f = frameSequencer.play();
	if(f != NULL) {
		if(!loadFrame(f))
			return 5;
	}
	else
		return 4;

	return 0;
}


//------------------------------------------------------------------
// private methods
//------------------------------------------------------------------
bool PixelDrawer::initShaders(string shaderLocation) {
	shaders[0] = shaderLocation+"/pixelDrawer_vs.glsl";
	shaders[1] = shaderLocation+"/pixelDrawer_fs.glsl";
	const char* vertex_path_fbo      = {shaders[0].c_str()};
	const char* fragment_path_fbo    = {shaders[1].c_str()};

	return shaderDraw.init(vertex_path_fbo, fragment_path_fbo);
}

void PixelDrawer::setUpAllWallBetas() {
	// change the beta value of every wall-ish cell to the one saved in the allWallBetas field
	// for wall-ish cells, beta is indeed passed instead of the type, cose wall type is 0 and beta is (0, 1],
	// so on the GPU side it's easy to extract beta and type from one single value
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			if(currentFramePixels[i*gl_width+j] < airCellType)
				currentFramePixels[i*gl_width+j] = allWallBetas;
		}
	}
}

int PixelDrawer::dumper(frame *f) {
	// check how many frames are in the current dir and change name of dumped frame accordingly
	vector<string> files;
	int fileNum = getFrameFileList(".", files);

	if(fileNum<0) {
		printf("Can't open directory \".\"\n");
		return 1;
	}

	ofstream dumpfile;

	string filename = "framedump_";
	ostringstream convert;
	convert << fileNum;
	filename += convert.str() + ".frm";
	dumpfile.open (filename.c_str(), ios::out | ios::binary);


	// dump
	if (dumpfile.is_open()) {
		// second parameter is equivalent to number of char in the memory block [i knooow it's a bit redundant!]
		// had to split into different write commands cos pixels is a dynamically allocated array!
		dumpfile.write((char *) &f->width, sizeof(int)/sizeof(char));
		dumpfile.write((char *) &f->height, sizeof(int)/sizeof(char));
		dumpfile.write((char *) &f->numOfExciteCells, sizeof(int)/sizeof(char));
		dumpfile.write((char *) f->pixels, f->width*f->height*sizeof(float)/sizeof(char));
		dumpfile.close();
	}
	else {
		printf("Can't create new file \"%s\"\n", filename.c_str());
		return 2;
	}

	return 0;
}

bool PixelDrawer::reader(string filename, frame *f) {
	ifstream readfile;
	readfile.open (filename.c_str(),  ios::in | ios::binary | ios::ate);

	if(readfile.is_open()) {
		//streampos filesize = readfile.tellg();
		readfile.seekg (0, ios::beg);
		//readfile.read ((char *)f, filesize);
		readfile.read((char *) &f->width, sizeof(int)/sizeof(char));
		readfile.read((char *) &f->height, sizeof(int)/sizeof(char));
		readfile.read((char *) &f->numOfExciteCells, sizeof(int)/sizeof(char));
		readfile.read((char *) f->pixels, f->width*f->height*sizeof(float)/sizeof(char));
		readfile.close();
		return true;
	}
	else {
		printf("Can't open file \"%s\"\n", filename.c_str());
		return false;
	}
}

int PixelDrawer::getFrameFileList(string dirname, vector<string> &files, bool ordered) {
	DIR *dir;
	struct dirent *ent;
	int fileCnt = 0;


	// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
	if ((dir = opendir (dirname.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// Ignore dotfiles and . and .. paths
			if(!strncmp(ent->d_name, ".", 1))
				continue;

			//printf("%s\n", ent->d_name);

			// take only .dbx and .txt files
			string name = string(ent->d_name);
			int len		= name.length();

			bool frameFile = false;

			if( (name[len-4]=='.') && (name[len-3]=='f') && (name[len-2]=='r') && (name[len-1]=='m') )
				frameFile = true;

			if(frameFile) {
				fileCnt++;
				printf("%s\n", ent->d_name);
				files.push_back( string( ent->d_name ) );
			}
		}
		closedir (dir);
	} else {
		/* could not open directory */
		printf("Could not open directory %s!\n", dirname.c_str());
		return -1;
	}

	// order by name
	if(ordered)
		sort( files.begin(), files.end(), naturalsort);

	return fileCnt;
}




//----------------------------------------------------------------------------------------------------------------------------------
PDrawerExplicitFDTD::PDrawerExplicitFDTD(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
					float frameDuration, int audioRate, int exCType, int airCType, int wallCType)
					: PixelDrawer(w, h, shader_fbo, startFrame, stateNum, resetQuadsNum, frameDuration, audioRate, exCType, airCType, wallCType) {
}

void PDrawerExplicitFDTD::update(int state, bool excitationChange) {
	currentPbo->trigger();

	shaderDraw.use();

	if(updateAllWallBetas) {
		setUpAllWallBetas();
		updateAllWallBetas = false;
	}


	glUniform1i(operation_loc, operation_oneShot); // drawing
	glUniform1i(deck_loc, deck);   // target deck [once would be enough, but still...]

	glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);

	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glTextureBarrierNV();

	shaderFbo.use();
}

void PDrawerExplicitFDTD::reset() {
	shaderDraw.use();

	// first put to zero the rgb channels in the main quads [pressure and velocity]
	glUniform1i(operation_loc, operation_oneShot); // back to drawing
	glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glTextureBarrierNV();

	// then put all channels to zero in the other quads
	glUniform1i(operation_loc, operation_reset); // reset excitations
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads
	glTextureBarrierNV();

	shaderFbo.use();
}


//----------------------------------------------------------------------------------------------------------------------------------
PDrawerImplicitFDTD::PDrawerImplicitFDTD(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int resetQuadsNum,
					float frameDuration, int audioRate, int exCType, int airCType, int wallCType)
					: PixelDrawer(w, h, shader_fbo, startFrame, stateNum, resetQuadsNum, frameDuration, audioRate, exCType, airCType, wallCType){

}

void PDrawerImplicitFDTD::update(int state, bool excitationChange) {
	currentPbo->trigger();

	shaderDraw.use();

	if(updateAllWallBetas) {
		setUpAllWallBetas();
		updateAllWallBetas = false;
	}


	glUniform1i(operation_loc, operation_oneShot); // drawing
	glUniform1i(deck_loc, deck);   // target deck [once would be enough, but still...]

	// if we use hard excitation with an explicit FDTD method...
	if(!excitationChange) // ...when the modification entails walls [no add or removal of excitations], we change only the alpha channel, leaving the same pressure and velocity
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE); // this saves us a bit of extra noise!
	// if we inject excitation as pressure in an implicit solver, we do not have to bother (:

	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glTextureBarrierNV();

	shaderFbo.use();
}

void PDrawerImplicitFDTD::reset() {
	shaderDraw.use();

	// first put to zero the rgb channels in the main quads [pressure and velocity]
	glUniform1i(operation_loc, operation_oneShot); // back to drawing
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glTextureBarrierNV();

	// then put all channels to zero in the other quads
	glUniform1i(operation_loc, operation_reset); // reset excitations
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads
	glTextureBarrierNV();

	shaderFbo.use();
}

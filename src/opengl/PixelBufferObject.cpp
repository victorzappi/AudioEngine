/*
 * PixelBufferObject.cpp
 *
 *  Created on: 2016-04-08
 *      Author: vic
 */

#include "opengl/PixelBufferObject.h"

#include <cstring> // memset, memcpy

//#define STOP_WATCH
#ifdef STOP_WATCH
#include <sys/time.h> // for time test
int elapsedTime;
timeval start_, end_;
#endif

PboReader::PboReader() {
	pbo = -1;
}

void PboReader::init(int size) {
	glGenBuffers(1, &pbo);
	bind();
	glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STREAM_READ);
}

float *PboReader::getPixels(int w, int h) {
	glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, 0);
	#ifdef STOP_WATCH
		gettimeofday(&start_, NULL);
	#endif
	float *pixelBuffer = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	#ifdef STOP_WATCH
		gettimeofday(&end_, NULL);
		elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
		printf("Read pixel cycle:%d\n", elapsedTime);
		printf("---------------------------------------------------------\n");
	#endif
	return pixelBuffer;
}









PboWriter::PboWriter(Texture2D &tex) : texture(tex) {
	pbo       = -1;
	gl_width  = -1;
	gl_height = -1;
	pixelSize = -1;
	pixels    = NULL;
}

void PboWriter::init(int w, int h, GLfloat *pix, int wrChannels) {
	gl_width  = w;
	gl_height = h;
	pixelSize = gl_width*gl_height * wrChannels * sizeof(GLfloat);
	pixels    = pix; // reference

	init(pixelSize);
}

int PboWriter::trigger() {
	bind();//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

	void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, pixels, pixelSize);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glActiveTexture(texture.getUnit());
	glBindTexture(GL_TEXTURE_2D, texture.getHandle());
	checkError("before glTexSubImage2D");
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gl_width, gl_height, texture.getFormat(), texture.getType(), 0);
	int retval = checkError("glTexSubImage2D");

	unbind();//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return retval;
}



//------------------------------------------------------------------
// private
//------------------------------------------------------------------

void PboWriter::init(int size) {
	glGenBuffers(1, &pbo);
	bind();//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, pixelSize, NULL, GL_DYNAMIC_DRAW);
	unbind();//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


int PboWriter::checkError(string lastCall) {
	int error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("%s ---> ", lastCall.c_str());
		switch(error) {
			case GL_INVALID_ENUM:
				printf("GL Error: GL_INVALID_ENUM ):\n");
				break;
			case GL_INVALID_VALUE:
				printf("GL Error: GL_INVALID_VALUE ):\n");
				break;
			case GL_INVALID_OPERATION:
				printf("GL Error: GL_INVALID_OPERATION ):\n");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				printf("GL Error: GL_INVALID_FRAMEBUFFER_OPERATION ):\n");
				break;
			case GL_OUT_OF_MEMORY:
				printf("GL Error: GL_OUT_OF_MEMORY ):\n");
				break;
			case GL_STACK_UNDERFLOW:
				printf("GL Error: GL_STACK_UNDERFLOW ):\n");
				break;
			case GL_STACK_OVERFLOW:
				printf("GL Error: GL_STACK_OVERFLOW ):\n");
				break;
			default:
				printf("GL Error...must be serious, can't even tell what the hell happened |:\n");
		}
		return error;
	}
	return 0;
}



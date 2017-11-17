/*
 * PixelBufferObject.h
 *
 *  Created on: 2016-04-08
 *      Author: vic
 */

#ifndef PIXELBUFFEROBJECT_H_
#define PIXELBUFFEROBJECT_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include "Texture2D.h"

class PixelBufferObject {
public:
	virtual ~PixelBufferObject();
	virtual void bind() = 0;
	virtual void unbind() = 0;

protected:
	virtual void init(int size) = 0;
	GLuint pbo;
};


inline PixelBufferObject::~PixelBufferObject() {
	if(pbo!=(unsigned int)-1)
		glDeleteBuffers(1, &pbo);
}






class PboReader : public PixelBufferObject {
public:
	PboReader();
	void init(int size);
	float *getPixels(int w, int h);
	void bind();
	void unbind();
};


inline void PboReader::bind(){
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
}

inline void PboReader::unbind(){
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}





class PboWriter : public PixelBufferObject {
public:
	PboWriter(Texture2D &tex);
	void init(int w, int h, GLfloat *pix, int wrChannels=1);
	int trigger();
	void bind();
	void unbind();

private:
	void init(int size);
	int checkError(string lastCall);

	int gl_width;
	int gl_height;
	int pixelSize;
	GLfloat *pixels;
	Texture2D &texture;
};

inline void PboWriter::bind(){
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
}

inline void PboWriter::unbind(){
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}




#endif /* PIXELBUFFEROBJECT_H_ */

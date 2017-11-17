/*
 * Texture2D.h
 *
 *  Created on: 2016-04-08
 *      Author: vic
 */

#ifndef TEXTURE2D_H_
#define TEXTURE2D_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library
#include <string>

using namespace std;

class Texture2D {
public:
	Texture2D();
	~Texture2D();

	void loadPixels(int w, int h, GLint internal_format, GLint format_,
			GLint type_, const GLvoid* data, GLint active_unit);
	GLuint getHandle();
	GLint getUnit();
	GLint getFormat();
	GLint getType();

private:
	GLuint handle;
	int width;
	int height;
	GLint unit;
	GLint format;
	GLint type;
};


inline GLuint Texture2D::getHandle() {
	return handle;
}

inline GLint Texture2D::getUnit() {
	return unit;
}


inline GLint Texture2D::getFormat() {
	return format;
}

inline GLint Texture2D::getType() {
	return type;
}

#endif /* TEXTURE2D_H_ */

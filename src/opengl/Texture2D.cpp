/*
 * Texture2D.cpp
 *
 *  Created on: 2016-04-08
 *      Author: vic
 */

#include "opengl/Texture2D.h"

using namespace std;

Texture2D::Texture2D() {
	handle = 0;
	width  = -1;
	height = -1;
	unit   = 0;
	format = -1;
	type   = -1;
}

Texture2D::~Texture2D() {
	glDeleteTextures(1, &handle);
}

void Texture2D::loadPixels(int w, int h, GLint internal_format, GLint format_,
							GLint type_, const GLvoid* data, int active_unit) {
	width  = w;
	height = h;
	format = format_;
	type   = type_;
	unit   = active_unit;

	// prepare texture memory
	glGenTextures(1, &handle);

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(active_unit);
	glBindTexture(GL_TEXTURE_2D, handle);

	// texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	GLfloat border_color[] = { 0, 0, 0, 0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// load pixels
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
}


/*
 * Shader.h
 *
 *  Created on: 2016-04-09
 *      Author: vic
 */

#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include <string>

using namespace std;

class Shader {
public:
	Shader();
	~Shader();
	bool init(const char *vertShaderPath, const char *fragShaderPath, string fragShaderExPath[] = NULL, int fragShaderExNUm = 0);
	void use();
	GLuint getUniformLocation(const char* uniformName);
	GLuint getHandle();

private:
	string readFile(const char *filePath);
	bool checkCompileErrors(GLuint shader_index);
	void printShaderInfoLog (GLuint shader_index);
	bool checkLinkErrors(GLuint prog);
	void printProgramInfoLog (GLuint prog);

	GLuint program;
	GLuint fs;
	GLuint vs;
};

inline void Shader::use() {
	glUseProgram(program);
}

inline GLuint Shader::getUniformLocation(const char* uniformName)
{
	return glGetUniformLocation(program, uniformName);
}

inline GLuint Shader::getHandle() {
	return program;
}

#endif /* SHADER_H_ */

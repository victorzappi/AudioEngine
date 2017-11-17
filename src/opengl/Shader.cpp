/*
 * Shader.cpp
 *
 *  Created on: 2016-04-09
 *      Author: vic
 */

#include "opengl/Shader.h"

#include <iostream>
#include <fstream>

using namespace std;

Shader::Shader() {
	program = -1;
	fs 		= -1;
	vs      = -1;
}

Shader::~Shader() {
	if(program==(unsigned int)-1)
		return;
	glDeleteProgram(program);
	glDeleteShader(fs);
	glDeleteShader(vs);
}

bool Shader::init(const char *vertShaderPath, const char *fragShaderPath, string fragShaderExPath[], int fragShaderExNUm)
{
	// load shaders' content

	// vertex shader file path
	string vertShaderStr = readFile(vertShaderPath);
	const char *vertex_shader = vertShaderStr.c_str();

	// fragment shader file path
	string fragShaderStr = readFile(fragShaderPath);
	// if additional excitation models are passed, append them to the main fbo frag shader
	// N.B. additional excitation model files should contain only functions, which need to be declared on top of main frag shader together with needed uniforms!
	for(int i=0; i<fragShaderExNUm; i++)
		fragShaderStr =  fragShaderStr + readFile( fragShaderExPath[i].c_str());
	const char *fragment_shader = fragShaderStr.c_str();

	// create and compile shaders
	vs = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);
	if(!checkCompileErrors(vs))
		return false;

	fs = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);
	if(!checkCompileErrors(fs))
		return false;

	// create GPU programme with those v and f shaders
	program = glCreateProgram ();
	glAttachShader(program, fs);
	glAttachShader(program, vs);
	glLinkProgram(program);
	if(!checkLinkErrors(program))
		return false;

	return true;
}


















//------------------------------------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------------------------------------

// adapted from: http://www.nexcius.net/2012/11/20/how-to-load-a-glsl-shader-in-opengl-using-c/
string Shader::readFile(const char *filePath) {
    string content;
    ifstream fileStream(filePath, ios::in);

    if(!fileStream.is_open()) {
        cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
        return "";
    }

    std::string line = "";
    while(!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}


bool Shader::checkCompileErrors(GLuint shader_index)
{
	// check for compile errors
	int params = -1;
	glGetShaderiv (shader_index, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", shader_index);
		printShaderInfoLog (shader_index);
		return false; // or exit or something
	}
	return true;
}

void Shader::printShaderInfoLog (GLuint shader_index) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetShaderInfoLog (shader_index, max_length, &actual_length, log);
	printf ("shader info log for GL index %u:\n%s\n", shader_index, log);
}



bool Shader::checkLinkErrors(GLuint prog) {
	// check if link was successful
	int params = -1;
	glGetProgramiv (prog, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf ( stderr, "ERROR: could not link shader program GL index %u\n", prog);
		printProgramInfoLog (prog);
		return false;
	}
	return true;
}

void Shader::printProgramInfoLog (GLuint prog) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetProgramInfoLog (prog, max_length, &actual_length, log);
	printf ("program info log for GL index %u:\n%s", prog, log);
}

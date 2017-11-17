/*
 * TextureFilter.h
 *
 *  Created on: 2015-09-23
 *      Author: Victor Zappi
 */

#ifndef TEXTUREFILTER_H_
#define TEXTUREFILTER_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library
#include <string>

#include "opengl/Texture2D.h"
#include "opengl/PixelBufferObject.h"
#include "opengl/Shader.h"

//#define VISUAL_DEBUG

using namespace std;

//TODO derive from future inout class or something of the sort
class TextureFilter {
public:
	TextureFilter();
	virtual ~TextureFilter();
	int init(int *domainSize, int numOfStates, int *excitationCoord, int *excitationDimensions,
			 float ***domain, int audioRate, int rateMul, int periodSize, float mag = 1);
	int init(int *domainSize, int numOfStates, int audioRate, int rateMul, int periodSize, float mag = 1);
	virtual int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);
	virtual GLFWwindow *getWindow(int *size);
	virtual float *getBufferFloat(int numSamples, float *inputBuffers) = 0;
	virtual bool setFullscreen(bool fullScreen, int monIndex=-1);


protected:
	// the domain is the pixel/cell area where the propagation happens; the window is bigger, cos has to contain some additional and specialized pixels/cells
	int domain_width;
	int domain_height;
	int gl_width;
	int gl_height;
	int period_size;
	int audio_rate;
	int simulation_rate;
	int rate_mul;
	float magnifier;
	bool use32F;

	// fbo vars
	GLFWwindow *window;
	Shader shaderFbo;
	GLuint fbo;
	int state;
	int numOfStates;
	int **drawArraysValues;
	GLint excitationInput_loc;
	GLint audioWriteCoords_loc;
	GLint *listenerFragCoord_loc;
	float dt;
	string listenUnifName;
	float **listenerFragCoord;
	float deltaCoordX;
	float deltaCoordY;
	float *audioRowWriteTexCoord;
	int audioRowChannel; // 0 = R, 1 = G, 2 = B
	int audioRowW;
	int audioRowH;
	float audioWriteCoords[3]; // 0: current audio write x coordinate, 1: current audio write y coordinate, 2: current audio write RGBA value
	int listenerCoord[2];

	// texture
	Texture2D inOutTexture;
	int textureWidth;
	int textureHeight;
	GLint nextAvailableTex;

	// read frame vars
	PboReader pbo; //For Audio
	PboReader pbo_allTexture; //Whole texture for debug

	// shader_program_render shader vars
	Shader shaderRender;
	GLint listenerFragCoord_loc_render;


	// other dynamically allocated structures
	float *texturePixels;
	GLuint vbo;
	GLuint vao;
	float *vertices_data;

	// misc
	bool printFormatCheck;
	string name;
	string shaderLoc;
	string shaders[4];
	float *outputBuffer; // this can be just a pointer if we use a PBO to read from texture...cos PBOs handle their own memory buffers
	bool inited;
	bool needDomain;
	bool fullscreen;
	int monitorIndex;

	//----------------------------
	// these methods needs to be overridden to define the specific behavior of this filter
	//----------------------------
	virtual void initPhysicalParameters() = 0;
	virtual void initImage(int domainWidth, int domainHeigth) = 0;
	virtual void initVao(GLuint vbo, GLuint shader_program_fbo, GLuint shader_program_render, GLuint &vao) = 0;
	virtual int fillVertices() = 0;
	virtual void initFrame(int *excitationCoord, int *excitationDimensions, float ***domain) = 0;
	virtual int initAdditionalUniforms() = 0; // this could be not striclty pure virtual, but let's make it pure to highlight its existence
	virtual void resetAudioRow() = 0;
	virtual int computeListenerFrag(int x, int y) = 0;
	virtual void initPixelReader() = 0;
	//----------------------------
	// native init methods
	//----------------------------
	virtual void initSimulationParameters(int audioRate, int rateMul);
	virtual int initWindow(int *domainSize, int numOfStates, int periodSize, float mag);
	virtual int initShaders();
	virtual void initGLDataStruct();
	virtual void initTexture(int *excitationCoord, int *excitationDimensions, float ***domain);
	virtual int initFbo();
	virtual int initUniforms();
	virtual GLFWwindow * initOpenGL(int width, int height, std::string windowName, GLFWwindowsizefun cbfun, bool fullScreen = false, int monitorIndex=-1);
	virtual bool loadShaderProgram(const char *vertShaderPath, const char *fragShaderPath, GLuint &vs, GLuint &fs, GLuint &shader_program);
	virtual void initVbo(int size, float *vertices, GLuint &vbo); // this is fixed, differently from VAO!
	//virtual void loadTexture(); // also this one is fixed, cos does a really simple task [see to believe]
	virtual void completeInit();
	virtual void setUpListener();

	//----------------------------

	//----------------------------
	// utils methods
	//----------------------------
	string readFile(const char *filePath);

	// log file
	bool restart_gl_log ();
	bool gl_log (const char* message, ...);

	// log functions
	void log_gl_params ();
	void _print_shader_info_log (GLuint shader_index);
	bool checkCompileErrors(GLuint shader_index);
	void _print_programme_info_log (GLuint program);
	bool checkLinkErrors(GLuint programme);
	const char* GL_type_to_string (GLenum type);
	void print_all (GLuint program);
	bool is_valid (GLuint program);

	// show fps in window bar
	void _update_fps_counter (GLFWwindow* window);

	void checkError(string lastCall = "");
	//----------------------------

};


inline bool TextureFilter::setFullscreen(bool fullScreen, int monIndex) {
	if(inited) {
		printf("Too late to go full screen, window has already been inited\n");
		return false;
	}

	fullscreen   = fullScreen;
	monitorIndex = monIndex;

	return true;
}
#endif /* TEXTUREFILTER_H_ */

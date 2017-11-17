/*
 * TextureFilter.cpp
 *
 *  Created on: 2015-09-23
 *      Author: Victor Zappi
 */

#include "opengl/TextureFilter.h"

#include <cstdarg>
#include <fstream>
#include <iostream>
#include <time.h>
#include <cmath>
#include <assert.h>

#define GL_LOG_FILE "gl.log"

using namespace std;

TextureFilter::TextureFilter(){
	domain_width		= -1;
	domain_height		= -1;
	gl_width            = -1;
	gl_height           = -1;
	period_size			= -1;
	audio_rate			= -1;
	simulation_rate 	= -1;
	rate_mul        	= -1;
	magnifier			= -1;
	use32F              = true;
	window     		    = NULL;
	fbo                 = -1;
	state               = -1;
	numOfStates         = -1;
	drawArraysValues    = NULL;
	excitationInput_loc = -1;
	audioWriteCoords_loc = -1;
	deltaCoordX         = -1;
	deltaCoordY         = -1;
	listenerFragCoord_loc = NULL;
	listenerFragCoord  = NULL;
	dt                  = -1;
	audioRowWriteTexCoord  = NULL;
	audioRowChannel     = -1;
	audioRowW   		= -1;
	audioRowH    	    = -1;
    listenerFragCoord_loc_render = -1;
    vbo                 = -1;
    vao                 = -1;
    vertices_data		= NULL;
    printFormatCheck    = false;
    texturePixels		= NULL;
	textureWidth        = -1;
	textureHeight       = -1;
	nextAvailableTex    = GL_TEXTURE0;

    listenUnifName = "listenerFragCoord";
    name           = "VirtualTextureFilter";

    listenerCoord[0] = 0;
    listenerCoord[1] = 0;

    shaderLoc  = "";
    shaders[0] = "";
    shaders[1] = "";
    shaders[2] = "";
    shaders[3] = "";

    outputBuffer = NULL;

    inited       = false;
    needDomain   = true;
	fullscreen   = false;
	monitorIndex = -1;
    // shit! so annoying
}

TextureFilter::~TextureFilter(){
/*	if(outputBuffer != NULL)
		delete[] outputBuffer;*/

	if(listenerFragCoord_loc != NULL)
		delete[] listenerFragCoord_loc;

	if(listenerFragCoord != NULL) {
		for(int i=0; i<numOfStates; i++)
			delete[] listenerFragCoord[i];
		delete[] listenerFragCoord;
	}

	if(inited) {
		// clear resources
		glDeleteFramebuffers(1, &fbo);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);

		glfwSetWindowShouldClose (window, 1);
		// close GL context and any other GLFW resources
		glfwTerminate();
	}
}



// everything about the filter that is more or less standard is set in here; specific hardcoded behaviors are defined in private methods called inside
int TextureFilter::init(int *domainSize, int numOfStates, int *excitationCoord, int *excitationDimensions,
						float ***domain, int audioRate, int rateMul, int periodSize, float mag) {
	printf("\n");
	// rates...
	initSimulationParameters(audioRate, rateMul);

	// others possible params...
	initPhysicalParameters(); // this is pure virtual, overridden by specific child class implementation



	// simulation domain + boarders, delta coordintes, opengGL window
	if(initWindow(domainSize, numOfStates, periodSize, mag) != 0)
		return -1;

	// vertex and frag shaders
	if(initShaders() != 0)
		return -1;

	// VBO and VAO
	initGLDataStruct();

	// Texture, openGL handle and user defined pixels
	initTexture(excitationCoord, excitationDimensions, domain);

	// guess what? FBO
	if(initFbo()!=0)
		return -1;

	// location and initialization of ALL the uniforms, with calls to child classes overrides
	if(initUniforms()!=0)
		return -1;

	resetAudioRow(); // prepare to write samples at beginning of audio row


	// pixel Buffer
	initPixelReader(); // we don't necessarily need to read only from the audio quad!

	//--------------------------------------------------------
	// Ready to go!
	//--------------------------------------------------------
	completeInit();

	return simulation_rate;
}

// if we call this, we need to set the domain before we can finish the initialization!
int TextureFilter::init(int *domainSize, int numOfStates, int audioRate, int rateMul, int periodSize, float mag) {
	printf("\n");
	// rates...
	initSimulationParameters(audioRate, rateMul);

	// others possible params...
	initPhysicalParameters(); // this is pure virtual, overridden by specific child class implementation

	// simuilation domain + boarders, delta coordintes, opengGL window
	if(initWindow(domainSize, numOfStates, periodSize, mag) != 0)
		return -1;

	// vertex and frag shaders
	if(initShaders() != 0)
		return -1;

	// VBO and VAO
	initGLDataStruct();

	// no texture and FBO and uniforms setups here! will be called when settin domain in setDomain() [called from the outside]

	// prepare to write samples at beginning of audio row
	resetAudioRow(); // pure virtual

	// PBO reader
	initPixelReader(); // we decide where to read from in the texture. pure virtual

	return simulation_rate;
}

// this can be called only after init(NO DOMAIN) and it finished the initialization
int TextureFilter::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	if(!needDomain)
		return -1;

	// Texture, openGL handle and user defined pixels
	initTexture(excitationCoord, excitationDimensions, domain);

	// guess what? FBO
	if(initFbo()!=0)
		return -1;

	// location and initialization of ALL the uniforms, with calls to child classes overrides
	if(initUniforms()!=0)
		return -1;

	//--------------------------------------------------------
	// Ready to go!
	//--------------------------------------------------------
	completeInit();

	return 0;
}


GLFWwindow *TextureFilter::getWindow(int *size) {
	if(!inited)
		return NULL;

	size[0] = gl_width*magnifier;
	size[1] = gl_height*magnifier;

	return window;
}


//------------------------------------------------------------------------------------------------------------------------------------------------
// protected
//------------------------------------------------------------------------------------------------------------------------------------------------
void TextureFilter::initSimulationParameters(int audioRate, int rateMul) {
	// FDTD settings and coefficients
	rate_mul         = rateMul;
	audio_rate       = audioRate;
	simulation_rate  = audio_rate*rate_mul;
	dt = 1.0/(float)simulation_rate;
}
//------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------

//---------------------------------
// helper functions
//---------------------------------
bool _gl_log_err (const char* message, ...) {
  va_list argptr;
  FILE* file = fopen (GL_LOG_FILE, "a");
  if (!file) {
    fprintf (
      stderr,
      "ERROR: could not open GL_LOG_FILE %s file for appending\n",
      GL_LOG_FILE
    );
    return false;
  }
  va_start (argptr, message);
  vfprintf (file, message, argptr);
  va_end (argptr);
  va_start (argptr, message);
  vfprintf (stderr, message, argptr);
  va_end (argptr);
  fclose (file);
  return true;
}

void _glfw_error_callback (int error, const char* description) {
  _gl_log_err ("GLFW ERROR: code %i msg: %s\n", error, description);
}
//---------------------------------




//---------------------------------
// utils methods
//---------------------------------

int TextureFilter::initWindow(int *domainSize, int numOfStates, int periodSize, float mag) {
	magnifier = mag; // enlarges the window!
	// domain size is exactly what specified by user, window will be bigger instead...check setUpWindow
	domain_width  = domainSize[0];
	domain_height = domainSize[1];
	period_size   = periodSize;
	this->numOfStates = numOfStates;

	// to properly allocate uniforms in child classes
	// the sooner, the better [before setupWindow()!]
	listenerFragCoord = new float *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		listenerFragCoord[i] = new float[2];

	initImage(domain_width, domain_height); // defines structure of the full image that will go into the texture [e.g., num of quads]

	// to read adjacent fragments
	// also needed to handle audio rows/columns in child classes...the sooner we have this ready, the better [but not before setupWindow()!]
	deltaCoordX = 1.0/(float)textureWidth;
	deltaCoordY = 1.0/(float)textureHeight;

	// let's start this!
#ifndef VISUAL_DEBUG
	window = TextureFilter::initOpenGL(gl_width*magnifier, gl_height*magnifier, name, NULL, fullscreen, monitorIndex);
#else
	window = TextureFilter::initOpenGL(textureWidth*magnifier, textureHeight*magnifier, name, NULL, fullscreen, monitorIndex);
#endif

	printf("\n");
	printf("Domain size: %d x %d pixels\nin\n", domainSize[0], domainSize[1]);
	printf("GL Window sized: %d x %d pixels\n", gl_width, gl_height);
	printf("Num of states: %d\n", numOfStates);
	printf("Texture size: %d x %d pixels\n", textureWidth, textureHeight);

	if(window == NULL)
		return -1;

	// this disables the VSync with the monitor! FUNDAMENTAL!
	glfwSwapInterval(0);

	return 0;
}

int TextureFilter::initShaders() {
	const char* vertex_path_fbo      = {shaders[0].c_str()};
	const char* fragment_path_fbo    = {shaders[1].c_str()};
	const char* vertex_path_render   = {shaders[2].c_str()};
	const char* fragment_path_render = {shaders[3].c_str()};

	//-----------------------------------------------------------
	// FBO shader program, first render pass
	//-----------------------------------------------------------
	if(!shaderFbo.init(vertex_path_fbo, fragment_path_fbo))
		return -1;

	//-----------------------------------------------------------
	// render shader program, second pass
	//-----------------------------------------------------------
	if(!shaderRender.init(vertex_path_render, fragment_path_render))
		return -1;

	return 0;
}

void TextureFilter::initGLDataStruct() {
	//-----------------------------------------------------------
	// Vertex Buffer Object, to pass data to graphics card [in our case, they're positions]
	//-----------------------------------------------------------
	vbo = 0;
	int sizeOfVertices = fillVertices(); // packs up data to be placed in VBO
	initVbo(sizeOfVertices, vertices_data, vbo);
	checkError("setUpVbo");

	//-----------------------------------------------------------
	// Vertex Attribute Object, tells the shader how to interpret the VBO's content [vertices and in attributes]
	//-----------------------------------------------------------
	vao = 0;
	initVao(vbo, shaderFbo.getHandle(), shaderRender.getHandle(), vao);
	checkError("setUpVao");
}

// this does something similar to the initialization of uniforms
void TextureFilter::initTexture(int *excitationCoord, int *excitationDimensions, float ***domain) {
	//--------------------------------------------------------
	// texture is shared, but used in different ways by the shader 2 programs [input/output and input]
	//--------------------------------------------------------

	// transforms the domain in an image [quads, pml, etc => texture pixels] that will be loaded into texture
	initFrame(excitationCoord, excitationDimensions, domain); // defines physical simulation domain [e.g,. boundaries, excitation]

	GLint textureInternalFormat = GL_RGBA32F;

	// uniforms to pass texture to the 2 programs
	shaderFbo.use();
	glUniform1i(shaderFbo.getUniformLocation("inOutTexture"), nextAvailableTex - GL_TEXTURE0);    // this is the name of the texture in FBO frag shader
	checkError("inOutTexture sampler2D uniform can't be found in fbo shader program\n");

	shaderRender.use();
	glUniform1i(shaderRender.getUniformLocation("inputTexture"), nextAvailableTex - GL_TEXTURE0); // this is the name of the texture in FBO vertex shader
	checkError("inputTexture sampler2D uniform can't be found in fbo vertex program\n");

	// we are basically saying that it's the same texture!
	// the one that is called inOutTexture in frag shader
	// and
	// the one that is called inputTexture in vertex shader
	// share the same memory...
	// WUNDERBAR!

	// load domain image into texture
	inOutTexture.loadPixels(textureWidth, textureHeight, textureInternalFormat, GL_RGBA, GL_FLOAT, texturePixels, nextAvailableTex++); // when we're done initializing this texture, let's incrase the counter

	needDomain = false;

	//--------------------------------------------------------
	// Formats check
	//--------------------------------------------------------
	if(printFormatCheck) {
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
}


int TextureFilter::initFbo() {
	//--------------------------------------------------------
	// fbo and draw buffer are only for first program
	//--------------------------------------------------------

	// The framebuffer, to write on the texture
	fbo = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	checkError("glBindFramebuffer");

	// Set our texture as our color attachment #0 [the freaking FBO will right on this texture]
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, inOutTexture.getHandle(), 0);
	checkError("glFramebufferTexture");

	// Set the list of draw buffers.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers); // "1" is the size of DrawBuffers
	checkError("glDrawBuffers");

	// Always check that our framebuffer is ok
	GLenum retval = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(retval != GL_FRAMEBUFFER_COMPLETE) {
		printf("Something went wrong while initializing FBO...%X ):\n", (int)retval);
		return -1;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}


int TextureFilter::initUniforms() {
	//--------------------------------------------------------
	// Uniforms
	//--------------------------------------------------------


	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// locations

	// location of excitation uniform
	excitationInput_loc = shaderFbo.getUniformLocation("excitationInput");
	if(excitationInput_loc == -1) {
		printf("excitationInput uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// location of audio write uniform
	audioWriteCoords_loc = shaderFbo.getUniformLocation("audioWriteCoords");
	if(audioWriteCoords_loc == -1) {
		printf("audioWriteCmd uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// listener in all states
	listenerFragCoord_loc = new GLint[numOfStates];
	for(int i=0; i<numOfStates; i++) {
		string name = listenUnifName + "[" + to_string(i) + "]";
		//sprintf(name, "%s[%d]", listenUnifName.c_str(), i);
		listenerFragCoord_loc[i] = shaderFbo.getUniformLocation(name.c_str());
		if(listenerFragCoord_loc[i] == -1) {
			printf("%s uniform can't be found in fbo shader program\n", name.c_str());
			//return -1;
		}
	}

	// location of coordinate delta to reach neighbor pixels
	GLint deltaCoordXLoc_fbo = shaderFbo.getUniformLocation("deltaCoordX");
	if(deltaCoordXLoc_fbo == -1) {
		printf("deltaCoordX uniform can't be found in fbo shader program\n");
		//return -1;
	}
	GLint deltaCoordYLoc_fbo = shaderFbo.getUniformLocation("deltaCoordY");
	if(deltaCoordYLoc_fbo == -1) {
		printf("deltaCoordY uniform can't be found in fbo shader program\n");
		//return -1;
	}


	// set
	// fbo

	// once for all
	// to read adjacent fragments
	// also needed to handle audio column
	deltaCoordX = 1.0/(float)textureWidth;
	deltaCoordY = 1.0/(float)textureHeight;
	// set them up
	shaderFbo.use();
	glUniform1f(deltaCoordXLoc_fbo, deltaCoordX);
	checkError("glUniform1f deltaCoordX");
	glUniform1f(deltaCoordYLoc_fbo, deltaCoordY);
	checkError("glUniform1f deltaCoordY");


	// dynamically
	computeListenerFrag(listenerCoord[0], listenerCoord[1]);
	setUpListener(); // this has to calculate the new position of the listener in all the states, according to specific arrangement of quads

	glUseProgram (0);



	//--------------------------------------------------------------------------------------------
	// render shader uniforms
	//--------------------------------------------------------------------------------------------
	// get locations

	listenerFragCoord_loc_render = shaderRender.getUniformLocation("listenerFragCoord");
	if(listenerFragCoord_loc_render == -1) {
	   printf("listenerFragCoord uniform can't be found in render shader program\n");
	   //return -1;
	}

	/*GLint deltaCoordXLoc = shaderRender.getUniformLocation("deltaCoordX");
	if(deltaCoordXLoc == -1) {
		printf("deltaCoordX uniform can't be found in vertex_path_render shader program\n");
		//return -1;
	}
	GLint deltaCoordYLoc = shaderRender.getUniformLocation("deltaCoordY");
	if(deltaCoordYLoc == -1) {
		printf("deltaCoordY uniform can't be found in vertex_path_render shader program\n");
		//return -1;
	}*/


	// set

	/*shaderRender.use();
	// once for all
	// dynamic
	*/

	glUseProgram (0);

	return initAdditionalUniforms(); // add others, just in case
}

// quick and dirty
void glfw_init_callback(int val, const char *name) {
	string error = "GLFW init error: " + val;
	error += "\t-\t";
	error += name;
	error +="\n";
	perror("error");
}

GLFWwindow *TextureFilter::initOpenGL(int width, int height, string windowName, GLFWwindowsizefun cbfun, bool fullScreen, int monIndex) {
	assert (restart_gl_log ());

	// start GL context and O/S window using the GLFW helper library
	gl_log ("starting GLFW\n%s\n", glfwGetVersionString ());
	// register the error call-back function that we wrote, above
	glfwSetErrorCallback (_glfw_error_callback);

	// let's set an error callback, just in case...
	glfwSetErrorCallback(glfw_init_callback);

	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return NULL;
	}

	// create window
	GLFWwindow* window;
	if(!fullScreen)
		window = glfwCreateWindow (width, height, windowName.c_str(), NULL, NULL);
	else {
		// full screen
		GLFWmonitor* mon;
		if(monIndex==-1)
			mon = glfwGetPrimaryMonitor ();
		else {
			int count = -1;
			mon = glfwGetMonitors(&count)[monitorIndex];
		}
		const GLFWvidmode* vmode = glfwGetVideoMode (mon);
		// if we don't do this check, fullscreen mode will  break the resolution of the screen
		if(vmode->width == width && vmode->height == height)
			window = glfwCreateWindow (width, height, "Extended GL Init", mon, NULL);
		else {
			printf("Can't go full screen! Window size is %dx%d and should be %dx%d\n", width, height, vmode->width, vmode->height);
			window = glfwCreateWindow (width, height, windowName.c_str(), NULL, NULL);
		}
	}
	if (!window) {
		fprintf (stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent (window);
	glfwSetWindowSizeCallback (window, cbfun); // register callback for screen size changes

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit ();

	// get version info
	const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
	const GLubyte* version  = glGetString (GL_VERSION);   // version as a string
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);
	log_gl_params ();

	return window;
}




bool TextureFilter::loadShaderProgram(const char *vertShaderPath, const char *fragShaderPath, GLuint &vs, GLuint &fs, GLuint &shader_program)
{
	// load shaders' content
	string vertShaderStr = readFile(vertShaderPath);
	string fragShaderStr = readFile(fragShaderPath);
	const char *vertex_shader = vertShaderStr.c_str();
	const char *fragment_shader = fragShaderStr.c_str();

	// create and compile shaders
	vs = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vs, 1, &vertex_shader, NULL);
	glCompileShader (vs);
	if(!checkCompileErrors(vs))
		return false;

	fs = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &fragment_shader, NULL);
	glCompileShader (fs);
	if(!checkCompileErrors(fs))
		return false;

	// create GPU program with those v and f shaders
	shader_program = glCreateProgram ();
	glAttachShader (shader_program, fs);
	glAttachShader (shader_program, vs);
	glLinkProgram (shader_program);
	if(!checkLinkErrors(shader_program))
		return false;

	return true;
}


void TextureFilter::initVbo(int size, float *vertices, GLuint &vbo) {
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo); // means that we work on this VBO
	glBufferData (GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

/*void TextureFilter::loadTexture() {
	GLint textureInternalFormat = 0;
	if(use32F)
		textureInternalFormat = GL_RGBA32F; // use this only when PML is implemented
	else
		textureInternalFormat = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, textureInternalFormat, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, texturePixels);
	checkError("glTexImage2D");
}*/

void TextureFilter::completeInit() {
	//--------------------------------------------------------
	// Ready to go!
	//--------------------------------------------------------
	//glBindVertexArray(vao);
	//checkError("Last Binding");

	glViewport(0, 0, textureWidth, textureHeight);

	inited = true;

	shaderFbo.use();
	checkError("TextureFilter::init() -> glUseProgram");
	glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to our framebuffer
}


void TextureFilter::setUpListener() {
	for (int i =0; i<numOfStates; i++)
		glUniform2f(listenerFragCoord_loc[i], listenerFragCoord[i][0], listenerFragCoord[i][1]);
}




// adapted from: http://www.nexcius.net/2012/11/20/how-to-load-a-glsl-shader-in-opengl-using-c/
string TextureFilter::readFile(const char *filePath) {
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






bool TextureFilter::restart_gl_log () {
  FILE* file = fopen (GL_LOG_FILE, "w");
  if (!file) {
    fprintf (
      stderr,
      "ERROR: could not open GL_LOG_FILE log file %s for writing\n",
      GL_LOG_FILE
    );
    return false;
  }
  time_t now = time (NULL);
  char* date = ctime (&now);
  fprintf (file, "GL_LOG_FILE log. local time %s\n", date);
  fclose (file);
  return true;
}

bool TextureFilter::gl_log (const char* message, ...) {
  va_list argptr;
  FILE* file = fopen (GL_LOG_FILE, "a");
  if (!file) {
    fprintf (
      stderr,
      "ERROR: could not open GL_LOG_FILE %s file for appending\n",
      GL_LOG_FILE
    );
    return false;
  }
  va_start (argptr, message);
  vfprintf (file, message, argptr);
  va_end (argptr);
  fclose (file);
  return true;
}


void TextureFilter::log_gl_params () {
  GLenum params[] = {
    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
    GL_MAX_CUBE_MAP_TEXTURE_SIZE,
    GL_MAX_DRAW_BUFFERS,
    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
    GL_MAX_TEXTURE_IMAGE_UNITS,
    GL_MAX_TEXTURE_SIZE,
    GL_MAX_VARYING_FLOATS,
    GL_MAX_VERTEX_ATTRIBS,
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    GL_MAX_VERTEX_UNIFORM_COMPONENTS,
    GL_MAX_VIEWPORT_DIMS,
    GL_STEREO,
  };
  const char* names[] = {
    "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
    "GL_MAX_CUBE_MAP_TEXTURE_SIZE",
    "GL_MAX_DRAW_BUFFERS",
    "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
    "GL_MAX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_TEXTURE_SIZE",
    "GL_MAX_VARYING_FLOATS",
    "GL_MAX_VERTEX_ATTRIBS",
    "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_VERTEX_UNIFORM_COMPONENTS",
    "GL_MAX_VIEWPORT_DIMS",
    "GL_STEREO",
  };
  gl_log ("GL Context Params:\n");
  //char msg[256];
  // integers - only works if the order is 0-10 integer return types
  for (int i = 0; i < 10; i++) {
    int v = 0;
    glGetIntegerv (params[i], &v);
    gl_log ("%s %i\n", names[i], v);
  }
  // others
  int v[2];
  v[0] = v[1] = 0;
  glGetIntegerv (params[10], v);
  gl_log ("%s %i %i\n", names[10], v[0], v[1]);
  unsigned char s = 0;
  glGetBooleanv (params[11], &s);
  gl_log ("%s %u\n", names[11], (unsigned int)s);
  gl_log ("-----------------------------\n");
}

void TextureFilter::_print_shader_info_log (GLuint shader_index) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetShaderInfoLog (shader_index, max_length, &actual_length, log);
  printf ("shader info log for GL index %u:\n%s\n", shader_index, log);
}

bool TextureFilter::checkCompileErrors(GLuint shader_index)
{
  // check for compile errors
  int params = -1;
  glGetShaderiv (shader_index, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf (stderr, "ERROR: GL shader index %i did not compile\n", shader_index);
    _print_shader_info_log (shader_index);
    return false; // or exit or something
  }
  return true;
}

void TextureFilter::_print_programme_info_log (GLuint program) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetProgramInfoLog (program, max_length, &actual_length, log);
  printf ("program info log for GL index %u:\n%s", program, log);
}

bool TextureFilter::checkLinkErrors(GLuint program)
{
  // check if link was successful
  int params = -1;
  glGetProgramiv (program, GL_LINK_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf (
      stderr,
      "ERROR: could not link shader program GL index %u\n",
      program
    );
    _print_programme_info_log (program);
    return false;
  }
  return true;
}

const char* TextureFilter::GL_type_to_string (GLenum type) {
  switch (type) {
    case GL_BOOL: return "bool";
    case GL_INT: return "int";
    case GL_FLOAT: return "float";
    case GL_FLOAT_VEC2: return "vec2";
    case GL_FLOAT_VEC3: return "vec3";
    case GL_FLOAT_VEC4: return "vec4";
    case GL_FLOAT_MAT2: return "mat2";
    case GL_FLOAT_MAT3: return "mat3";
    case GL_FLOAT_MAT4: return "mat4";
    case GL_SAMPLER_2D: return "sampler2D";
    case GL_SAMPLER_3D: return "sampler3D";
    case GL_SAMPLER_CUBE: return "samplerCube";
    case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
    default: break;
  }
  return "other";
}

void TextureFilter::print_all (GLuint program) {
  printf ("--------------------\nshader program %i info:\n", program);
  int params = -1;
  glGetProgramiv (program, GL_LINK_STATUS, &params);
  printf ("GL_LINK_STATUS = %i\n", params);

  glGetProgramiv (program, GL_ATTACHED_SHADERS, &params);
  printf ("GL_ATTACHED_SHADERS = %i\n", params);

  glGetProgramiv (program, GL_ACTIVE_ATTRIBUTES, &params);
  printf ("GL_ACTIVE_ATTRIBUTES = %i\n", params);
  for (int i = 0; i < params; i++) {
    char name[64];
    int max_length = 64;
    int actual_length = 0;
    int size = 0;
    GLenum type;
    glGetActiveAttrib (
      program,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
    );
    if (size > 1) {
      for (int j = 0; j < size; j++) {
        char long_name[64];
        sprintf (long_name, "%s[%i]", name, j);
        int location = glGetAttribLocation (program, long_name);
        printf ("  %i) type:%s name:%s location:%i\n",
          i, GL_type_to_string (type), long_name, location);
      }
    } else {
      int location = glGetAttribLocation (program, name);
      printf ("  %i) type:%s name:%s location:%i\n",
        i, GL_type_to_string (type), name, location);
    }
  }

  glGetProgramiv (program, GL_ACTIVE_UNIFORMS, &params);
  printf ("GL_ACTIVE_UNIFORMS = %i\n", params);
  for (int i = 0; i < params; i++) {
    char name[64];
    int max_length = 64;
    int actual_length = 0;
    int size = 0;
    GLenum type;
    glGetActiveUniform (
      program,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
    );
    if (size > 1) {
      for (int j = 0; j < size; j++) {
        char long_name[64];
        sprintf (long_name, "%s[%i]", name, j);
        int location = glGetUniformLocation (program, long_name);
        printf ("  %i) type:%s name:%s location:%i\n",
          i, GL_type_to_string (type), long_name, location);
      }
    } else {
      int location = glGetUniformLocation (program, name);
      printf ("  %i) type:%s name:%s location:%i\n",
        i, GL_type_to_string (type), name, location);
    }
  }

  _print_programme_info_log (program);
}

bool TextureFilter::is_valid (GLuint program) {
  glValidateProgram (program);
  int params = -1;
  glGetProgramiv (program, GL_VALIDATE_STATUS, &params);
  printf ("program %i GL_VALIDATE_STATUS = %i\n", program, params);
  if (GL_TRUE != params) {
    _print_programme_info_log (program);
    return false;
  }
  return true;
}


void TextureFilter::_update_fps_counter (GLFWwindow* window) {
  static double previous_seconds = glfwGetTime ();
  static int frame_count;
  double current_seconds = glfwGetTime ();
  double elapsed_seconds = current_seconds - previous_seconds;
  if (elapsed_seconds > 0.25) {
    previous_seconds = current_seconds;
    double fps = (double)frame_count / elapsed_seconds;
    char tmp[128];
    sprintf (tmp, "opengl @ fps: %.2f", fps);
    glfwSetWindowTitle (window, tmp);
    frame_count = 0;
  }
  frame_count++;
}


void TextureFilter::checkError(string lastCall)
{
	int error = glGetError();
	if(error != GL_NO_ERROR)
	{
		printf("%s ---> ", lastCall.c_str());
		switch(error)
		{
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
	}

}






















/*
 * SSE.h
 *
 *  Created on: 2016-06-29
 *      Author: Victor Zappi
 *      Comments: gently taken from Tuomas Tonteri's amazing tutorial at http://sci.tuomastonteri.fi/programming/sse
 */

#ifndef SSE_H_
#define SSE_H_

#include "AudioEngine.h"

// must be defined in AudioEngine.h
#ifdef SSE_SUPPORT
//#include <cstring>  // this is needed cos memcpy is used in cvalarray.h, but already includede in AudioEngine.h
#include <cvalarray.h>
#include <veclib.h>
#include <vector>

// useful type definitions that combine the types included in the 2 libs
typedef veclib::f32x4 vec4;
typedef veclib::f32x4b vec4b;
typedef veclib::i32x4 ivec4;
typedef veclib::i32x4b ivec4b;
typedef n_std::cvalarray<vec4,2> mat4x2;
typedef n_std::cvalarray<vec4,3> mat4x3;
typedef n_std::cvalarray<vec4,4> mat4x4;
typedef n_std::cvalarray<float,2> vec2;
typedef n_std::cvalarray<float,3> vec3;
typedef n_std::cvalarray<double,2> pvec2;
typedef n_std::cvalarray<double,3> pvec3;

// additional math functions
#include "SSE_mathfun.h"

#endif



#endif /* SSE_H_ */

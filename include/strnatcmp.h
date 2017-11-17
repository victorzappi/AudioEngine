/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * strnatcmp.h
 *
 *  Created on: 2016-09-09
 *      Author: Victor Zappi
 *
 *  This file has been created starting from:
 *
 *    strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
 *	  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>
 *
 *	  This software is provided 'as-is', without any express or implied
 *	  warranty.  In no event will the authors be held liable for any damages
 *	  arising from the use of this software.
 *
 *	  Permission is granted to anyone to use this software for any purpose,
 *	  including commercial applications, and to alter it and redistribute it
 *	  freely, subject to the following restrictions:
 *
 *	  1. The origin of this software must not be misrepresented; you must not
 *		 claim that you wrote the original software. If you use this software
 *		 in a product, an acknowledgment in the product documentation would be
 *		 appreciated but is not required.
 *	  2. Altered source versions must be plainly marked as such, and must not be
 *		 misrepresented as being the original software.
 *	  3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef STRNATCMP_H_
#define STRNATCMP_H_

#include <string>

/* CUSTOMIZATION SECTION
 *
 * You can change this typedef, but must then also change the inline
 * functions in strnatcmp.c */
typedef char nat_char;

int strnatcmp(nat_char const *a, nat_char const *b);
int strnatcasecmp(nat_char const *a, nat_char const *b);

bool naturalsort(std::string a, std::string b); //VIC


#endif /* STRNATCMP_H_ */

/*
    This file is a part of t18 project (C++17 framework for algotrading)
    Copyright (C) 2019, Arech (aradvert@gmail.com; https://github.com/Arech)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//min() and max() triggers very weird compiler crash while doing ::std::numeric_limits<vec_len_t>::max()
#define NOMINMAX

//\vc\include\yvals.h(112): error C2338: /RTCc rejects conformant code, so it isn't supported by the C++ Standard Library.
//Either remove this compiler option, or define _ALLOW_RTCc_IN_STL to acknowledge that you have received this warning.
#define _ALLOW_RTCc_IN_STL

#define _USE_MATH_DEFINES // for C++ math constants


#include "targetver.h"

#include <stdio.h>
//#include <tchar.h>

#include <iostream>

//to get rid of '... decorated name length exceeded, name was truncated'
#pragma warning( disable : 4503 )

#define STDCOUT(args) ::std::cout << args
#define STDCOUTL(args) STDCOUT(args) << ::std::endl

//////////////////////////////////////////////////////////////////////////
// special externals, necessary only for tests, but not nntl

#pragma warning(disable: 28182)
#include <gtest/gtest.h>
#pragma warning(default: 28182)

#ifdef _DEBUG
#define _GTEST_LIB "gtestd.lib"
#else
#define _GTEST_LIB "gtest.lib"
#endif // _DEBUG
#pragma comment(lib,_GTEST_LIB)

/////////////////////////////////////////////////////////////////////////////////////////////
#include "../t18/debug.h"

#include "cmn_defs.h"

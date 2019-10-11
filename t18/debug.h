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
#pragma once

#include "compiler.h"

//////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) || defined(DEBUG)

#define T18_DEBUG
#define T18_ASSERT(a)  { T18_COMP_SILENCE_STRING_CONV; \
     _ASSERTE(a); \
     T18_COMP_POP; }

#define T18_DEBUG_ONLY(v) v
#define T18_DEBUG_ARG(p) ,p

#define BOOST_CB_ENABLE_DEBUG 1
#define BOOST_ENABLE_ASSERT_HANDLER

#else

#ifndef T18_TESTS_DO_SLOW_TESTS
#define T18_TESTS_DO_SLOW_TESTS 1
#endif

#ifndef T18_TESTS_DO_EXPORTS
#define T18_TESTS_DO_EXPORTS 1
#endif

#ifdef T18_RELEASE_WITH_DEBUG

#define T18_DEBUG
#define T18_ASSERT(a) { T18_COMP_SILENCE_STRING_CONV; \
    if(!(a)) __debugbreak(); \
    T18_COMP_POP; }

#define T18_DEBUG_ONLY(v) v
#define T18_DEBUG_ARG(p) ,p

#else

#define T18_ASSERT(a) ((void)(0))
#define T18_DEBUG_ARG(p)
#define T18_DEBUG_ONLY(v)

#endif // !T18_RELEASE_WITH_DEBUG
#endif // defined(_DEBUG) || defined(DEBUG)


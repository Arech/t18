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

//must be included before boost headers inclusion
#include "debug.h"

//////////////////////////////////////////////////////////////////////////
//Some static-polymorphism related definitions here

#define T18_TYPEDEFS_SELF() typedef self_t& self_ref_t; \
typedef const self_t& self_cref_t;

//shortcuts for get_self() methods
//NB: ensure the self_t is defined and macro is placed under a correct visibility scope
#define T18_METHODS_SELF_CHECKED(cond, msg) \
T18_TYPEDEFS_SELF() \
self_ref_t get_self() noexcept { \
  static_assert(cond, msg); \
  return static_cast<self_ref_t>(*this); \
} \
self_cref_t get_self() const noexcept { \
  static_assert(cond, msg); \
  return static_cast<self_cref_t>(*this); \
}

#define T18_METHODS_SELF() \
T18_TYPEDEFS_SELF() \
self_ref_t get_self() noexcept { return static_cast<self_ref_t>(*this); } \
self_cref_t get_self() const noexcept {  return static_cast<self_cref_t>(*this); }

#define T18_SELFT(T) (static_cast<T&>(*this))
// By convention, lets name FCT a template parameter typename, that defines a type of the final children in inheritance tree
// so T18_SELF now makes an easy cast to it without requiring typedefs and defining get_self() family of functions
#define T18_SELF T18_SELFT(FCT)
#define T18_CSELF T18_SELFT(const FCT)

//////////////////////////////////////////////////////////////////////////

#include "_base/tsq_data.h"
#include "_base/tsTick.h"
#include "_base/tsohlcv.h"
#include "_base/tsDirTick.h"
#include "_base/tsDeal.h"


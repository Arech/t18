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

#include <limits>
#include <type_traits>

//had to include, or breaks with clang-cl
#include <boost/type_traits/is_assignable.hpp>
#include <boost/circular_buffer.hpp>

#include "../utils/obj_traits.h"
#include "../utils/hana.h"
#include "../utils/HanaDescrMaps.h"

#include "../utils/call_wrappers.h"

#include "../date_time.h"
#include "../tags.h"

namespace t18 {
	namespace hana = ::boost::hana;
	using namespace hana::literals;
	using namespace ::std::literals;

	//typedef utils::cmcforwarderWrapper<2 * sizeof(void*)>  def_call_wrapper_t; //too large internal storage leads to worse performance
	//cmcforwarderWrapper doesn't work yet
	typedef utils::simpleWrapper<::std::function>  def_call_wrapper_t;

	//////////////////////////////////////////////////////////////////////////

	typedef decltype("ts"_s) timestamp_ht;
	typedef decltype("open"_s) open_ht;
	typedef decltype("high"_s) high_ht;
	typedef decltype("low"_s) low_ht;
	typedef decltype("close"_s) close_ht;
	typedef decltype("vol"_s) volume_ht;

	typedef decltype("quote"_s) quote_ht;

	typedef decltype("bLong"_s) bLong_ht;

	//////////////////////////////////////////////////////////////////////////

	typedef double real_t;
	//typedef float real_t;

	//32-bit is not enough to represent the total possible volume quantity. Also not sure if it's ok to use integer types
	//btw, at least for MOEX integer volume seems reasonable, but leave it floating-point for better generalization
	typedef double volume_t;

	//type to hold any price*volume product result
	typedef double money_t;

	//////////////////////////////////////////////////////////////////////////

	template <typename T>
	using TsCont_t = ::boost::circular_buffer<T>;

	typedef TsCont_t<real_t> RealNCont_t;

	constexpr real_t realNan_v = ::std::numeric_limits<real_t>::quiet_NaN();

	template<typename T>
	constexpr T tNaN = ::std::numeric_limits<T>::quiet_NaN();


	//////////////////////////////////////////////////////////////////////////

	struct tsohlcv;//fwd
	struct tsTick;//fwd
}
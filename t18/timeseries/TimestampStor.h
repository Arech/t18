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

//had to include, or breaks with clang-cl
#include <boost/type_traits/is_assignable.hpp>
#include <boost/circular_buffer.hpp>

#include "../base.h"
#include "TsStor.h"
#include "../date_time.h"

namespace t18 { namespace timeseries {
	namespace hana = ::boost::hana;
	using namespace hana::literals;

	template<typename HanaMapT>
	class TimestampStor : protected TsStor<HanaMapT> {
		typedef TsStor<HanaMapT> base_class_t;
		//ensure there's a key for date and time entries in the HanaMapT
		/*static_assert(hana::any_of(hana::keys(HanaMapT()), utils::is_hana_string<date_ht>), "There must be a key for date");
		static_assert(hana::any_of(hana::keys(HanaMapT()), utils::is_hana_string<time_ht>), "There must be a key for time");*/
		
		static_assert(hana::any_of(hana::keys(HanaMapT()), utils::is_hana_string<timestamp_ht>), "There must be a key for timestamp");

	public:
		TimestampStor(size_t N) : base_class_t(N) {}

	public:

		//changing visibility of some API
		using base_class_t::size;
		using base_class_t::capacity;
		using base_class_t::get;
		using base_class_t::getTs;
		using base_class_t::TotalBars;
		using base_class_t::BarIndex;
		
		/*auto date(size_t N)const { return get(date_ht(), N); }
		auto time(size_t N)const { return get(time_ht(), N); }
		auto lastDate()const { return date(0); }
		auto lastTime()const { return time(0); }
		*/

		auto timestamp(size_t N)const noexcept { return get(timestamp_ht(), N); }
		auto lastTimestamp()const noexcept { return timestamp(0); }

		/*auto date(size_t N)const { return timestamp(N).Date(); }
		auto time(size_t N)const { return  timestamp(N).Time(); }
		auto lastDate()const { return date(0); }
		auto lastTime()const { return time(0); }*/
	};

}}

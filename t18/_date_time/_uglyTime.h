/*
    This file is a part of t18 project (C++17 framework for algotrading)
    Copyright (C) 2019, Arech (al.rech@gmail.com; https://github.com/Arech)

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

#include "_fwd.h"

namespace t18 {

	namespace _uglyTime {
		constexpr inline bool isLeapYear(int y)noexcept {
			return ((!(y % 4)) && (!!(y % 100))) || (!(y % 400));
		}
		static_assert(isLeapYear(2012) && !isLeapYear(2011) && !isLeapYear(2100) && isLeapYear(2000), "");

		constexpr timestamp_diff_t days[2 * 12] = { 0, 31, 31 + 28, 31 + 28 + 31
			, 31 + 28 + 31 + 30, 31 + 28 + 31 + 30 + 31, 31 + 28 + 31 + 30 + 31 + 30, 31 + 28 + 31 + 30 + 31 + 30 + 31
			, 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31, 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30
			, 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31, 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
			, 0, 31, 31 + 29, 31 + 29 + 31
			, 31 + 29 + 31 + 30, 31 + 29 + 31 + 30 + 31, 31 + 29 + 31 + 30 + 31 + 30, 31 + 29 + 31 + 30 + 31 + 30 + 31
			, 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31, 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30
			, 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31, 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 };
		static_assert(days[11] + 31 == 365, "");
		static_assert(days[12 + 11] + 31 == 366, "");

		constexpr inline int daysInMonth(int m, bool bLeapYear)noexcept {
			T18_ASSERT(m >= 1 && m <= 12);
			return m > 11 ? 31 : static_cast<int>(_uglyTime::days[12 * bLeapYear + m] - _uglyTime::days[12 * bLeapYear + m - 1]);
		}

		constexpr timestamp_diff_t mksInSec = 1000000;
		constexpr timestamp_diff_t mksInMin = mksInSec * 60;
		constexpr timestamp_diff_t mksInHour = mksInMin * 60;
		constexpr timestamp_diff_t mksInDay = mksInHour * 24;
	}

}

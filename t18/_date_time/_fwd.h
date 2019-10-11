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

#include <string>
#include <cstdlib>

#include "../debug.h"
#include "../utils/hana.h"

namespace t18 {

	namespace hana = ::boost::hana;

	typedef ::std::uint32_t time_ult;
	typedef ::std::uint32_t date_ult;
	typedef ::std::uint64_t timestamp_ult;
	typedef ::std::int64_t timestamp_diff_t;

	struct tag_mxDate {};
	struct tag_milDate {};
	struct tag_mxTime {};
	struct tag_milTime {};
	struct tag_milDT {};
	struct tag_mxDT {};

	struct tag_mxTimestamp {};

	//some fwd declarations
	struct mxDate;
	struct mxTime;
	//struct mxTimestamp;

}

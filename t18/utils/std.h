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

namespace std {

	template<typename T, typename A>
	inline size_t forward_list_size_slow(const forward_list<T, A>& fl)noexcept {
		size_t c{ 0 };
		//won't use ::std::for_each, because it may require an access to c variable via reference, and that is slower
		for (const auto& e : fl) { T18_UNREF(e); ++c; }
		return c;
	}

}
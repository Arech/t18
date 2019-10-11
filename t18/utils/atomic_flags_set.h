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

#include <atomic>

namespace utils {

	//the class provides a bitwise-modifiable variable suitable to use (update/query) from different threads
	template<typename BaseT>
	class atomic_flags_set {
		static_assert(::std::is_integral_v<BaseT>, "");

	public:
		typedef BaseT flags_t;

	protected:
		typedef ::std::atomic<flags_t> safe_value_t;

	protected:
		safe_value_t m_flags;

	public:
		//restrictions seem reasonable
		atomic_flags_set(const atomic_flags_set&) = delete;
		atomic_flags_set(atomic_flags_set&&) = delete;

		//btw: initialization is not atomic
		explicit atomic_flags_set(flags_t initial) noexcept : m_flags(initial) {}
		atomic_flags_set()noexcept : atomic_flags_set(flags_t(0)) {}

		template<flags_t flg> void set()noexcept {
			m_flags.fetch_or(flg);
		}
		template<flags_t flg> void clear()noexcept { 
			m_flags.fetch_and(~flg);
		}
		template<flags_t flg> bool isSet()const noexcept {
			flags_t curVal = m_flags.load();
			return (curVal & flg);
		}
	};

}
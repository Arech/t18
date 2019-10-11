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

namespace utils {

	//regHandle must be _generic_ (suitable for any algorithm) handle of single type, that is capable of doing
	//some deregister() operation as well as performing automatic deregister() call if it wasn't called earlier.
	// #WARNING!!! This is a fragile and unsafe mechanism, though it's enought for the task!
	// Make sure that regHandle object is destroyed
	// or deregister()-ed before the main object (the one, that had created the regHandle object) is destroyed!
	// Else you'll got dangling pointers/references and eventually program failure
	// #TODO: make better handle system. sometimes...probably, if needed

	T18_COMP_SILENCE_C17EXT
	class T18_ATTR_NODISCARD regHandle{
	T18_COMP_POP

	protected:
		::std::function<void(void)> m_f;

	protected:
		void _doMove(regHandle&& o) {
			if (this != &o) {
				release();
				m_f.swap(o.m_f);
			}
		}

	public:
		~regHandle() {
			deregister();
		}

		regHandle() {}

		regHandle(const regHandle&) = delete;
		regHandle(regHandle&& o) {
			_doMove(::std::move(o));
		}
		regHandle& operator=(regHandle&& o) {
			_doMove(::std::move(o));
			return *this;
		}

		template<typename F, typename = ::std::enable_if_t<::std::is_bind_expression_v<F>>>
		regHandle(F&& f) : m_f(::std::forward<F>(f)) {}

		void release() {
			deregister();
		}

		void deregister() {
			if (m_f) {
				m_f();
				m_f = nullptr;
			}
		}
	};

}
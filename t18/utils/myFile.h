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

#include <string>
#include <stdio.h>
#include <clocale>

#include "../base_filesystem.h"

namespace utils {

	class myFile {
	public:
		typedef FILE* pFILE_t;

	protected:
		FILE* m_pF = nullptr;

	public:
		~myFile() {
			close();
		}
		myFile(const char*const fname, const char*const fmode) {
			open(fname, fmode);
		}
		myFile() {}

		myFile(const myFile&) = delete;
		myFile(myFile&& o)noexcept {
			close();
			if (o.isOpened()) {
				m_pF = o.m_pF;
				o.m_pF = nullptr;
			}
		}

		void close()noexcept {
			if (m_pF) {
				fclose(m_pF);
				m_pF = nullptr;
			}
		}
		bool open(const char*const fname, const char*const fmode) {
			using namespace ::std::literals;
			close();

			if (!fname || !fmode) return false;

			auto ec = fopen_s(&m_pF, fname, fmode);
			if (ec) {
				T18_ASSERT(!m_pF);
				char buf[128];
				strerror_s(buf, ec);
				throw T18_FILESYSTEM_NAMESPACE::filesystem_error(
					"Failed to open file ("s + fname + "), reason: "s + buf
					, T18_FILESYSTEM_NAMESPACE::path(fname)
					, ::std::error_code(ec, ::std::system_category())
				);
			}
			T18_ASSERT(m_pF);
			return true;
		}

		pFILE_t get()noexcept{
			T18_ASSERT(m_pF);
			return m_pF;
		}
		operator pFILE_t() noexcept { return get(); }

		bool isOpened()const noexcept { return !!m_pF; }
		operator bool()const noexcept { return isOpened(); }

		static bool exist(const char* fn)noexcept {
			FILE *pF;
			if (0 == fopen_s(&pF, fn, "r")) {
				fclose(pF);
				return true;
			} else return false;
		}
	};

}


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

#include "_base.h"
#include <vector>

namespace t18 {
	namespace algs {
		namespace code {

			//////////////////////////////////////////////////////////////////////////
			//that's why we separate a state of an algorithm from its parameters and all of them - from the code
			//Because two algorithms may have totally the same state and different parameters
			// (like elementile and percentile does)
			// any temporary storage MUST be default constructible
			template<typename T>//T is a non-const value_type of a source data
			struct tStor_lenBased {
				typedef T value_t;
				typedef common_meta::prm_len_t prm_len_t;

				::std::vector<T> v;
				
				tStor_lenBased() {}
				tStor_lenBased(tStor_lenBased&& o) {
					if (this != &o) {
						v.swap(o.v);
					}
				}
				tStor_lenBased(const tStor_lenBased& o) = delete;

				void init(prm_len_t l) {
					T18_ASSERT(l > 0);
					T18_ASSERT(v.size() == 0 && v.capacity() == 0 || !"Already initialized!");
					v.resize(l);
				}

				void reinit(prm_len_t l) {
					T18_ASSERT(l > 0);
					v.resize(l);
				}

				size_t _len()const noexcept { return v.size(); }

				template<typename C, typename = ::std::enable_if_t< ::std::is_same_v< T, ::std::remove_const_t<typename C::value_type> >> >
				void _copyFrom(const C& src) {
					const auto s = v.size();
					T18_ASSERT(src.size() >= s);

					auto cb = src.begin();
					auto cbe = cb;
					::std::advance(cbe, s);
					::std::copy(cb, cbe, v.begin());
				}
			};

		}
	}
}

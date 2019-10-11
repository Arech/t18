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

#include "../../base.h"

namespace t18 {
	namespace algs {
		namespace code {

			struct common_meta {
				typedef decltype("len"_s) prm_len_ht;
				typedef size_t prm_len_t;

				typedef decltype("gamma"_s) prm_gamma_ht;
				typedef real_t prm_gamma_t;

				typedef decltype("percV"_s) prm_percV_ht;
				typedef real_t prm_percV_t;

				//////////////////////////////////////////////////////////////////////////
				typedef decltype("lenBased"_s) tstor_lenBased_ht;
			};


			//_i namespace contains various internal support/helper types
			namespace _i {
				struct histSimple {
					static constexpr size_t minSrcHist(size_t l)noexcept {
						T18_ASSERT(l > 0);
						return l;
					}
					static constexpr size_t minDestHist()noexcept { return 1; }


				};
			}

		}
	}
}
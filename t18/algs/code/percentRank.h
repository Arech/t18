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

#include "_base.h"

namespace t18 {
	namespace algs {
		namespace code {

			struct percentRank {
				typedef common_meta::prm_len_t prm_len_t;

				//The "period" parameter in AmiBroker specifies the amount of bar prior to the current bar, so
				//the total history must be period+1
				static constexpr size_t minSrcHist(size_t l)noexcept {
					T18_ASSERT(l > 0);
					return l+1;
				}
				static constexpr size_t minDestHist()noexcept { return 1; }

				template<typename ContDT, typename ContST>
				static void percent_rank(ContDT& dest, const ContST& src, const prm_len_t len) noexcept {
					T18_ASSERT(dest.capacity() >= minDestHist() && dest.size() > 0);
					dest[0] = percent_rank(src, len);
				}

				template<typename ContST>
				static auto percent_rank(const ContST& src, const prm_len_t len) noexcept {
					auto l = minSrcHist(len) - 1;
					T18_ASSERT(src.capacity() > l && len > 0 && l >= 0);

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					src_value_t r;
					if (UNLIKELY(src.size() <= l)) {
						r = tNaN<src_value_t>;
					} else {
						prm_len_t c = 0;
						const auto v = src[0];
						T18_ASSERT(isfinite(v));
						for (; l > 0; --l) {
							T18_ASSERT(src[l]);
							c += (v > src[l]);
						}
						r = static_cast<src_value_t>(c * 100) / static_cast<src_value_t>(len);
					}
					return r;
				}

			};
		}
	}
}


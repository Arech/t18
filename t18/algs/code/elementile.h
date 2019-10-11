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

#include "_tStor_lenBased.h"
#include <algorithm>
#include <array>

namespace t18 {
	namespace algs {
		namespace code {

			struct Elementile : public _i::histSimple {
				typedef _i::histSimple base_class_t;

				typedef common_meta::prm_len_t prm_len_t;
				typedef prm_len_t prm_rank_t;

				template<typename V>
				using TStor_tpl = tStor_lenBased<V>;
				

				template<typename ContDT, typename ContST>
				static void elementile(ContDT& dest, const ContST& src
					, TStor_tpl<typename ::std::remove_const_t<typename ContST::value_type>>& tStor
					, const prm_rank_t elmIdx) noexcept
				{
					T18_ASSERT(dest.capacity() >= base_class_t::minDestHist() && dest.size() > 0);
					dest[0] = elementile(src, tStor, elmIdx);
				}

				template<typename ContST>
				static auto elementile(const ContST& src
					, TStor_tpl<typename ::std::remove_const_t<typename ContST::value_type>>& tStor
					, const prm_rank_t elmIdx) noexcept
				{
					T18_ASSERT(src.capacity() >= minSrcHist(tStor._len()));
					T18_ASSERT(elmIdx < tStor._len());

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					src_value_t r;
					if (UNLIKELY(src.size() < tStor._len())) {
						r = tNaN<src_value_t>;
					} else {
						tStor._copyFrom(src);
						auto b = tStor.v.begin();
						auto ti = b;
						::std::advance(ti, elmIdx);
						::std::nth_element(b, ti, tStor.v.end());
						r = *ti;
						T18_ASSERT(isfinite(r));
					}
					return r;
				}

				//#TODO to calculate percentiles one needs the values of two adjacent elementiles (ranks)
				//More efficient algorithm for that case could be written using std::partial_sort_copy
				template<typename ContST, size_t N>
				static void elementiles(const ContST& src
					, TStor_tpl<typename ::std::remove_const_t<typename ContST::value_type>>& tStor
					, const ::std::array<prm_rank_t, N>& elmIdxs
					, ::std::array<typename ::std::remove_const_t<typename ContST::value_type>, N>& elms) noexcept
				{
					T18_ASSERT(src.capacity() >= minSrcHist(tStor._len()));
					T18_ASSERT(::std::all_of(elmIdxs.begin(), elmIdxs.end(), [l = tStor._len()](auto e){return e < l; }));

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					if (UNLIKELY(src.size() < tStor._len())) {
						::std::fill(elms.begin(), elms.end(), tNaN<src_value_t>);
					} else {
						tStor._copyFrom(src);
						auto& v = tStor.v;
						T18_ASSERT(::std::all_of(v.begin(), v.end(), [](const auto& e) {return isfinite(e); }));

						::std::sort(v.begin(), v.end());
						for (size_t i = 0; i < N; ++i) {
							elms[i] = v[elmIdxs[i]];
							T18_ASSERT(isfinite(elms[i]));
						}
					}
				}
			};


		}
	}
}

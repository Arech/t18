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

#include "elementile.h"

namespace t18 {
	namespace algs {
		namespace code {

			struct Percentile : protected Elementile {
				typedef Elementile base_class_t;

				using base_class_t::minSrcHist;
				using base_class_t::minDestHist;

				template<typename V> using TStor_tpl = base_class_t::template TStor_tpl<V>;

				typedef common_meta::prm_percV_t prm_percV_t;
				using base_class_t::prm_len_t;
				using base_class_t::prm_rank_t;

				template<typename ContDT, typename ContST>
				static void percentile(ContDT& dest, const ContST& src
					, TStor_tpl<typename ::std::remove_const_t<typename ContST::value_type>>& tStor
					, const prm_percV_t prcV) noexcept//prcV is always real_t, no matter what container type is
				{
					T18_ASSERT(dest.capacity() >= base_class_t::minDestHist() && dest.size() > 0);
					dest[0] = percentile(src, tStor, prcV);
				}

				template<typename ContST>
				static auto percentile(const ContST& src
					, TStor_tpl<typename ::std::remove_const_t<typename ContST::value_type>>& tStor
					, const prm_percV_t prcV) noexcept
				{
					T18_ASSERT(src.capacity() >= minSrcHist(tStor._len()));
					T18_ASSERT(prm_percV_t(0) <= prcV && prcV <= prm_percV_t(1));

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					const auto lastElm = tStor._len() - 1;
					src_value_t r;
					if (UNLIKELY(src.size() <= lastElm)) {
						r = tNaN<src_value_t>;
					} else {
						//finding corresponding elementile
						const real_t firstElmV = real_t(prcV)*lastElm;
						real_t firstElm = ::std::round(firstElmV);
						static const constexpr real_t eqTol = real_t(1000);

						T18_COMP_SILENCE_FLOAT_CMP_UNSAFE

						if (::std::round(eqTol* firstElmV) == ::std::round(eqTol* firstElm)) {

							T18_COMP_POP

							//firstElm is an exact elementile we may use
							r = base_class_t::elementile(src, tStor, static_cast<prm_rank_t>(firstElm));
						} else {
							//we must extract two nearest elementiles and interpolate between them
							firstElm = ::std::floor(firstElmV);
							::std::array<prm_rank_t, 2> idxs = { static_cast<prm_rank_t>(firstElm) ,static_cast<prm_rank_t>(::std::ceil(firstElmV)) };
							::std::array<src_value_t, 2> vals;
							T18_DEBUG_ONLY(::std::fill(vals.begin(), vals.end(), tNaN<src_value_t>));
							base_class_t::elementiles(src, tStor, idxs, vals);
							T18_ASSERT(::std::all_of(vals.begin(), vals.end(), [](const auto& e) {return isfinite(e); }));

							r = vals[0] + real_t(vals[1] - vals[0])*(firstElmV - firstElm);
						}

						T18_ASSERT(isfinite(r));
					}
					return r;
				}

			};

		}
	}
}
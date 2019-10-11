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

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>

namespace t18 {
	namespace algs {
		namespace code {

			using namespace ::boost::accumulators;

			template<typename BoostAccTagT>
			struct BoostAcc : public _i::histSimple {
				typedef _i::histSimple base_class_t;

				typedef common_meta::prm_len_t prm_len_t;

				typedef BoostAccTagT boost_acc_tag_t;
				
				template<typename ContDT, typename ContST>
				static void boostAcc(ContDT& dest, const ContST& src, prm_len_t len)noexcept {
					T18_ASSERT(dest.capacity() >= base_class_t::minDestHist() && dest.size() > 0);
					dest[0] = boostAcc(src, len);
				}

				template<typename ContST>
				static auto boostAcc(const ContST& src, const prm_len_t len)noexcept {
					T18_ASSERT(src.capacity() >= base_class_t::minSrcHist(len));

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;
					typedef accumulator_set<src_value_t, stats<boost_acc_tag_t>> acc_t;

					auto l = len - 1;
					src_value_t r;
					if (UNLIKELY(src.size() <= l)) {
						r = tNaN<src_value_t>;
					} else {
						acc_t acc;
						do {
							acc(src[l]);
						} while (l-- != 0);
						r = extract_result<boost_acc_tag_t>(acc);
						T18_ASSERT(isfinite(r));
					}
					return r;
				}
			};

		}
	}
}

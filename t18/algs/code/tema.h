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


#include "ema.h"

namespace t18 {
	namespace algs {
		namespace code {

			struct TEMA
				: public _i::EMA_SelfInit
				, public _i::EMABase
			{
				typedef _i::EMA_SelfInit base_class_t;

				typedef _i::EMABase base_ema_t;
				using typename base_ema_t::prm_len_t;
				using typename base_ema_t::prm_gamma_t;

				//BTW, state must be DefaultConstructible
				struct algState {
					prm_gamma_t r1, r2, r3;

					algState() noexcept : r1(tNaN<prm_gamma_t>), r2(tNaN<prm_gamma_t>), r3(tNaN<prm_gamma_t>) {}
				};


				template<typename ContDT, typename ContST>
				static void tema(ContDT& dest, const ContST& src, const prm_gamma_t gamma, const prm_gamma_t omgamma
					, algState& state, const bool bClose) noexcept
				{
					T18_ASSERT(src.capacity() >= base_class_t::minSrcHist(0) && dest.capacity() >= base_class_t::minDestHist());
					T18_ASSERT(dest.size() > 0);

					T18_COMP_SILENCE_FLOAT_CMP_UNSAFE
					T18_ASSERT(0 < gamma && gamma <= 1 && 0 <= omgamma && omgamma < 1 && (prm_gamma_t(1) - gamma) == omgamma);
					T18_COMP_POP

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					src_value_t r;
					const auto realMinSrcHist = base_class_t::minSrcHist(0) - 1;
					const auto ss = src.size();
					if (UNLIKELY(ss < realMinSrcHist)) {
						r = tNaN<src_value_t>;
					} else {
						prm_gamma_t r1, r2, r3;
						if (UNLIKELY(ss == realMinSrcHist)) {
							//must be initialized
							r1 = base_class_t::_initVal(src, 0);
							T18_ASSERT(isfinite(r1));
							r2 = r1;
							r3 = r1;
						} else {
							r1 = state.r1;
							r2 = state.r2;
							r3 = state.r3;
						}
						T18_ASSERT(isfinite(r1) && isfinite(r2) && isfinite(r3));
						r1 = gamma*src[0] + omgamma*r1;
						r2 = gamma*r1 + omgamma*r2;
						r3 = gamma*r2 + omgamma*r3;
						T18_ASSERT(isfinite(r1) && isfinite(r2) && isfinite(r3));
						if (bClose) {
							state.r1 = r1;
							state.r2 = r2;
							state.r3 = r3;
						}
						r = prm_gamma_t(3) * r1 - prm_gamma_t(3) * r2 + r3;
					}
					dest[0] = r;
				}
			};

		}
	}
}
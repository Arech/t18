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

#include "ma.h"

namespace t18 {
	namespace algs {
		namespace code {

			namespace _i {

				struct EMA_DestHist {
					static constexpr size_t minDestHist()noexcept { return 2; }
				};

				struct EMA_AmiMetastockInit : public EMA_DestHist {
					//adding 1 to make sure that size of a source is going to be bigger than the real required history
					//we're using the size() of source data as a flag whether the previous srcDest member was initialized
					static size_t minSrcHist(size_t l)noexcept { return 1 + code::MA::minSrcHist(l); }

				protected:
					template<typename ContST>
					static decltype(auto) _initVal(const ContST& src, size_t len) noexcept {
						T18_ASSERT(src.size() >= len);
						return code::MA::ma(src, len);
					}
				};

				struct EMA_SelfInit : public EMA_DestHist {
					//adding 1 to make sure that size of a source is going to be bigger than the real required history
					//we're using the size() of source data as a flag whether the previous srcDest member was initialized
					static size_t minSrcHist(size_t)noexcept { return 2; }

				protected:
					template<typename ContST>
					static decltype(auto) _initVal(const ContST& src, size_t) noexcept {
						T18_ASSERT(src.size() > 0);
						return src[0];
					}
				};

				template<bool bAmiMetastockInit>
				using _EMA_init = ::std::conditional_t<bAmiMetastockInit, EMA_AmiMetastockInit, EMA_SelfInit>;

				struct EMABase {
					//typedef common_meta::prm_len_ht prm_len_ht;
					//typedef common_meta::prm_gamma_ht prm_gamma_ht;
					// NO _ht types are allowed here!

					typedef common_meta::prm_len_t prm_len_t;
					typedef common_meta::prm_gamma_t prm_gamma_t;

					static constexpr prm_gamma_t makeGamma(prm_len_t len)noexcept {
						return prm_gamma_t(2) / prm_gamma_t(len + 1);
					}

					static void validateGamma(prm_gamma_t g) {
						if (g <= 0 || g > 1) {
							T18_ASSERT(!"invalid gamma!");
							throw ::std::runtime_error("invalid gamma!");
						}
					}
				};
			}

			template<bool _bAmiMetastockInit = true>
			struct tEMA
				: public _i::_EMA_init<_bAmiMetastockInit>
				, public _i::EMABase
			{
				static constexpr bool bAmiMetastockInit = _bAmiMetastockInit;
				static constexpr bool bReallyLenBased = _bAmiMetastockInit;

				typedef _i::_EMA_init<_bAmiMetastockInit> base_class_t;
				typedef _i::EMABase base_ema_t;
				using typename base_ema_t::prm_len_t;
				using typename base_ema_t::prm_gamma_t;

			public:

				template<typename ContDT, typename ContST>
				static void ema(ContDT& dest, const ContST& src, prm_len_t len) noexcept {
					const auto g = _i::EMABase::makeGamma(len);
					ema(dest, src, len, g, prm_gamma_t(1) - g);
				}

				template<typename ContDT, typename ContST>
				static void ema(ContDT& dest, const ContST& src, const prm_len_t len
					, const prm_gamma_t gamma, const prm_gamma_t omgamma) noexcept
				{
					T18_ASSERT(src.capacity() >= base_class_t::minSrcHist(len));
					T18_ASSERT(dest.capacity() >= base_class_t::minDestHist() && dest.size() > 0);

					T18_COMP_SILENCE_FLOAT_CMP_UNSAFE
					T18_ASSERT((!bReallyLenBased || len > 0) && 0 < gamma && gamma <= 1 && 0 <= omgamma && omgamma < 1 && (prm_gamma_t(1) - gamma) == omgamma);
					T18_COMP_POP

					typedef ::std::remove_const_t<typename ContST::value_type> src_value_t;

					src_value_t r;
					const auto realMinSrcHist = base_class_t::minSrcHist(len) - 1;
					const auto ss = src.size();
					if (UNLIKELY(ss < realMinSrcHist)) {
						r = tNaN<src_value_t>;
					} else if (ss == realMinSrcHist) {
						//must be initialized
						r = base_class_t::_initVal(src, len);
					} else {
						T18_ASSERT(dest.size() > 1);
						r = gamma*src[0] + omgamma*dest[1];
						T18_ASSERT(isfinite(r));
					}
					dest[0] = r;
				}
			};

			typedef tEMA<true> EMA;
			typedef tEMA<false> EMAsi;

		}
	}
}
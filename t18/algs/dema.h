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

#include "code/dema.h"

namespace t18 {
	namespace algs {

		namespace code {

			struct DEMA_meta : public tEMAsi_meta<DEMA> {
				typedef tEMAsi_meta<DEMA> base_class_t;

				// setting proper state 
				typedef typename base_class_t::algState algState_t;
			};

			template<typename BaseMetaT>
			struct tDEMA_call_base : public BaseMetaT {
				typedef BaseMetaT meta_t;

				using meta_t::minSrcHist;

				template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
				static size_t minSrcHist(const CallerT& C) noexcept { return meta_t::minSrcHist(0); }

			protected:
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getGammaPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename meta_t::prm_gamma_descr>(hmPrms);
				}
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getOmgammaIPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename meta_t::iprm_omgamma_descr>(hmPrms);
				}
			};

			struct DEMA_call : public tDEMA_call_base<DEMA_meta> {
				typedef tDEMA_call_base<DEMA_meta> base_class_t;

				//defining timeseries mapping (using standard defs)
				typedef adpt_src_ht adpt_src_ht;
				typedef adpt_dest_ht adpt_dest_ht;

				typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

				template<typename CallerT, typename SubstHMT = adptDefSubstMap_t>
				static void call(CallerT&& C, const bool bClose) noexcept {
					constexpr auto substMap = SubstHMT();
					const auto& prms = C.getPrms();
					base_class_t::dema(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()])
						, base_class_t::_getGammaPrm(prms), base_class_t::_getOmgammaIPrm(prms), C.getState(), bClose);
				}
			};
		}

		template<bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
		class tDEMA : public tAlg2ts_select<isCont, tDEMA<isCont, DVT, SVT, ContTplT>, code::DEMA_call, DVT, SVT, ContTplT> {
		public:
			typedef tAlg2ts_select<isCont, tDEMA<isCont, DVT, SVT, ContTplT>, code::DEMA_call, DVT, SVT, ContTplT> base_class_t;
			typedef typename base_class_t::prm_len_t prm_len_t;

		public:
			template<typename... Args>
			tDEMA(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

			template<typename D, typename S>
			tDEMA(D&& d, S&& s, prm_len_t len) : base_class_t(::std::forward<D>(d), ::std::forward<S>(s), base_class_t::prms2hmap(len)) {}
		};

		typedef tDEMA<false> DEMA;
		typedef tDEMA<true> DEMA_c;

		/*template<typename DVT, typename SVT>
		class tDEMA : public tAlg<memb::simpleAdpt<DVT, SVT>, code::DEMA_call> {
		public:
			typedef tAlg<memb::simpleAdpt<DVT, SVT>, code::DEMA_call> base_class_t;
			typedef tDEMA<DVT, SVT> self_t;

			typedef typename base_class_t::srcAdpt_t srcAdpt_t;
			typedef typename base_class_t::destAdpt_t destAdpt_t;

			typedef typename base_class_t::prm_len_t prm_len_t;
			typedef typename base_class_t::prm_gamma_t prm_gamma_t;

		public:
			template<typename D, typename S, typename HMT, typename = ::std::enable_if_t<
				hana::is_a<hana::map_tag, HMT> && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t>
				&& ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tDEMA(D&& d, S&& s, HMT&& prms) : base_class_t(pass2simpleAdpt(d, s), ::std::forward<HMT>(prms), *this) {}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tDEMA(D&& d, S&& s, prm_len_t len) : base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len), *this) {}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tDEMA(D&& d, S&& s, prm_gamma_t gamma) : base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(gamma), *this) {}
		};

		typedef tDEMA<real_t, real_t> DEMA;*/


	}
}
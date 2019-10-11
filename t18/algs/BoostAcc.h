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
#include "code/BoostAcc.h"
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/moment.hpp>

namespace t18 {
	namespace algs {
		//don't care much about performance now. Correctness and readability is the main priority.
		
		namespace code {
			template<typename BoostAccTagT>
			using BoostAcc_meta = tMA_meta<BoostAcc<BoostAccTagT>>;

			template<typename BoostAccTagT>
			struct BoostAcc_call : public tMA_call_base<BoostAcc_meta<BoostAccTagT>> {
				typedef tMA_call_base<BoostAcc_meta<BoostAccTagT>> base_class_t;

				//defining timeseries mapping (using standard defs)
				typedef adpt_src_ht adpt_src_ht;
				typedef adpt_dest_ht adpt_dest_ht;

				typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

				template<typename CallerT, typename SubstHMT = adptDefSubstMap_t>
				static void call(CallerT&& C, const bool = true) noexcept {
					constexpr auto substMap = SubstHMT();
					base_class_t::boostAcc(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()])
						, base_class_t::_getLenPrm(C.getPrms()));
				}
			};
		}

		template<typename BoostAccTagT, bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
		class tBoostAcc : public tAlg2ts_select<isCont, tBoostAcc<BoostAccTagT, isCont, DVT, SVT, ContTplT>, code::BoostAcc_call<BoostAccTagT>, DVT, SVT, ContTplT> {
		public:
			typedef tAlg2ts_select<isCont, tBoostAcc<BoostAccTagT, isCont, DVT, SVT, ContTplT>, code::BoostAcc_call<BoostAccTagT>, DVT, SVT, ContTplT> base_class_t;
			typedef typename base_class_t::prm_len_t prm_len_t;

		public:
			template<typename... Args>
			tBoostAcc(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

			template<typename D, typename S>
			tBoostAcc(D&& d, S&& s, prm_len_t len) : base_class_t(::std::forward<D>(d), ::std::forward<S>(s), base_class_t::prms2hmap(len)) {}
		};

		typedef tBoostAcc<::boost::accumulators::tag::min, false> MovMin;
		typedef MovMin LLV;

		typedef tBoostAcc<::boost::accumulators::tag::max, false> MovMax;
		typedef MovMax HHV;

		typedef tBoostAcc<::boost::accumulators::tag::moment<2>, false> MovVariance;
		
		typedef tBoostAcc<::boost::accumulators::tag::min, true> MovMin_c;
		typedef MovMin_c LLV_c;

		typedef tBoostAcc<::boost::accumulators::tag::max, true> MovMax_c;
		typedef MovMax_c HHV_c;

		typedef tBoostAcc<::boost::accumulators::tag::moment<2>, true> MovVariance_c;


		/*template<typename DVT, typename SVT, typename BoostAccTagT>
		class tBoostAcc : public tAlg< memb::simpleAdpt<DVT, SVT>, code::BoostAcc_call<BoostAccTagT>> {
		public:
			typedef tAlg<memb::simpleAdpt<DVT, SVT>, code::BoostAcc_call<BoostAccTagT>> base_class_t;
			typedef tBoostAcc<DVT, SVT, BoostAccTagT> self_t;

			typedef typename base_class_t::srcAdpt_t srcAdpt_t;
			typedef typename base_class_t::destAdpt_t destAdpt_t;

			typedef typename base_class_t::prm_len_t prm_len_t;

		public:

			template<typename D, typename S, typename HMT, typename = ::std::enable_if_t<
				hana::is_a<hana::map_tag, HMT> && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t>
				&& ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tBoostAcc(D&& d, S&& s, HMT&& prms) : base_class_t(pass2simpleAdpt(d, s), ::std::forward<HMT>(prms), *this) {}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tBoostAcc(D&& d, S&& s, prm_len_t len) : base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len), *this) {}
		};

		template<typename BoostAccTagT>
		using BoostAcc = tBoostAcc<real_t, real_t, BoostAccTagT>;

		typedef BoostAcc<::boost::accumulators::tag::min> MovMin;
		typedef MovMin LLV;

		typedef BoostAcc<::boost::accumulators::tag::max> MovMax;
		typedef MovMin HHV;

		typedef BoostAcc<::boost::accumulators::tag::moment<2>> MovVariance;*/

	}
}
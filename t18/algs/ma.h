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

#include "code/ma.h"
#include "_base.h"

namespace t18 {
	namespace algs {

	//don't care much about performance now. Correctness and readability is the main priority.

	//code namespace contains definition of types that implements only code of an algorithm.
	//Every type in that namespace MUST NOT have any non-static members
	namespace code {

		//this class describes:
		// - required parameters of the MA algo (appends it to the MA)
		// - the run-time state
		// - the run-time temporary data storage
		template<typename AlgCT>
		struct tMA_meta : public lenBased_meta<AlgCT> {
			typedef lenBased_meta<AlgCT> base_class_t;
			
			//////////////////////////////////////////////////////////////////////////

			//let's describe which parameters are required by the algo
			using typename base_class_t::prm_len_ht;
			using typename base_class_t::prm_len_t;
			using typename base_class_t::prm_len_descr;

			//and descr map
			typedef decltype(hana::make_map(prm_len_descr())) algPrmsDescr_t;
			
			typedef utils::dataMapFromDescrMap_t<algPrmsDescr_t> algPrmsMap_t;
			//algPrmsMap_t is a type that should be given to the algo as the params
			// It should be returned by prms2hmap()

			//////////////////////////////////////////////////////////////////////////
			//also specify additional internal/derived params to make runtime algo parameter set (nothing here)
			typedef algPrmsDescr_t algFullPrmsDescr_t;
			typedef algPrmsMap_t algFullPrmsMap_t;
			//algFullPrmsMap_t is a type that stores parameters as well as some internal data.
			// It should be returned by validatePrms()

			//////////////////////////////////////////////////////////////////////////
			static algPrmsMap_t prms2hmap(prm_len_t len)noexcept {
				T18_ASSERT(len > 0);
				return hana::make_map(
					hana::make_pair(prm_len_ht(), len)
				);
			}

			//this function checks that passed parameters are suitable for the algo
			//i.e. that params have all the necessary keys and its values are convertible to the necessary types
			template<typename HMT>
			static decltype(auto) validatePrms(HMT&& prms) {
				static_assert(utils::couldBeDataMap_v<::std::remove_reference_t<HMT>>, "");

				//testing that the prms is suitable for the alg
				utils::static_assert_hmap_conforms_descr<algFullPrmsDescr_t>(prms);

				//finally checking real parameters values.
				if (prms[prm_len_ht()] < 1) {
					T18_ASSERT(!"Invalid len parameter!");
					throw ::std::runtime_error("Invalid len parameter!");
				}

				return ::std::forward<HMT>(prms);
			}
		};

		typedef tMA_meta<MA> MA_meta;

		//the class describes a simple caller of MA algorithm.
		//Caller class isolates the code of algorithm from the knowledge of any parameter/state/tstor storage.
		template<typename MetaBaseT>
		struct tMA_call_base : public MetaBaseT {
			typedef MetaBaseT meta_t;

			using meta_t::minSrcHist;

			template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
			static size_t minSrcHist(const CallerT& C) noexcept {
				return meta_t::minSrcHist(C.getPrms());
			}

		protected:
			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			static auto _getLenPrm(const HMT& hmPrms)noexcept {
				return utils::hmap_get<typename meta_t::prm_len_descr>(hmPrms);
			}
		};

		struct MA_call : public tMA_call_base<MA_meta> {
			typedef tMA_call_base<MA_meta> base_class_t;

			//defining timeseries mapping (using standard defs)
			typedef adpt_src_ht adpt_src_ht;
			typedef adpt_dest_ht adpt_dest_ht;
			typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

			template<typename CallerT, typename SubstHMT = adptDefSubstMap_t>
			static void call(CallerT&& C, const bool = true) noexcept {
				constexpr auto substMap = SubstHMT();
				base_class_t::ma(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()]), base_class_t::_getLenPrm(C.getPrms()));
			}
		};
	}

	template<bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
	class tMA : public tAlg2ts_select<isCont, tMA<isCont, DVT, SVT, ContTplT>, code::MA_call, DVT, SVT, ContTplT> {
	public:
		typedef tAlg2ts_select<isCont, tMA<isCont, DVT, SVT, ContTplT>, code::MA_call, DVT, SVT, ContTplT> base_class_t;
		typedef typename base_class_t::prm_len_t prm_len_t;

	public:
		template<typename... Args>
		tMA(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

		template<typename D, typename S>
		tMA(D&& d, S&& s, prm_len_t len) : base_class_t(::std::forward<D>(d), ::std::forward<S>(s), base_class_t::prms2hmap(len)) {}
	};

	typedef tMA<false, real_t, real_t> MA;
	typedef tMA<true, real_t, real_t> MA_c;

	typedef MA MovMean;
} }


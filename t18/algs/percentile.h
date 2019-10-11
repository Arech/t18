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
#include "code/percentile.h"

namespace t18 {
	namespace algs {

		namespace code {

			struct Percentile_meta : public lenBased_meta<Percentile> {
				typedef lenBased_meta<Percentile> base_class_t;
				//using typename base_class_t::alg_class_t;

				//let's describe which parameters are required by the algo
				using typename base_class_t::prm_len_ht;
				using typename base_class_t::prm_len_t;
				using typename base_class_t::prm_len_descr;

				typedef common_meta::prm_percV_ht prm_percV_ht;
				typedef typename base_class_t::prm_percV_t prm_percV_t;
				typedef decltype(hana::make_pair(prm_percV_ht(), hana::type_c<prm_percV_t>)) prm_percV_descr;

				typedef decltype(hana::make_map(prm_len_descr(), prm_percV_descr())) algPrmsDescr_t;

				typedef utils::dataMapFromDescrMap_t<algPrmsDescr_t> algPrmsMap_t;
				//algPrmsMap_t is a type that should be given to the algo as the params
				// It should be returned by prms2hmap()

				//additional optional parameter that may change how the percV is treated.
				//if set, it inverts percV to 1-percV
				typedef bool oprm_invPercV_t;
				typedef decltype("bInvPercV"_s) oprm_invPercV_ht;

				//////////////////////////////////////////////////////////////////////////
				//also specify additional internal/derived params to make runtime algo parameter set (nothing here)
				typedef algPrmsDescr_t algFullPrmsDescr_t;
				typedef algPrmsMap_t algFullPrmsMap_t;
				//algFullPrmsMap_t is a type that stores parameters as well as some internal data.
				// It should be returned by validatePrms()

				//////////////////////////////////////////////////////////////////////////
				static algPrmsMap_t prms2hmap(prm_len_t len, prm_percV_t percV)noexcept {
					if (percV > 1) percV /= prm_percV_t(100);
					T18_ASSERT(len > 0 && percV >= prm_percV_t(0) && percV <= prm_percV_t(1));
					return hana::make_map(
						hana::make_pair(prm_len_ht(), len)
						, hana::make_pair(prm_percV_ht(), percV)
					);
				}

				//this function checks that passed parameters are suitable for the algo
				//i.e. that params have all the necessary keys and its values are convertible to the necessary types
				template<typename HMT>
				static decltype(auto) validatePrms(HMT&& prms) {
					static_assert(utils::couldBeDataMap_v<::std::remove_reference_t<HMT>>, "");

					//testing that the prms is suitable for the alg
					utils::static_assert_hmap_conforms_descr<algPrmsDescr_t>(prms);

					//finally checking real parameters values.
					const auto len = prms[prm_len_ht()];
					if (len < 1) {
						T18_ASSERT(!"Invalid len parameter!");
						throw ::std::runtime_error("Invalid len parameter!");
					}
					auto percV = static_cast<prm_percV_t>(prms[prm_percV_ht()]);
					percV = percV > prm_percV_t(1) ? percV / prm_percV_t(100) : percV;
					if (percV < prm_percV_t(0) || percV > prm_percV_t(1)) {
						T18_ASSERT(!"Invalid percV parameter!");
						throw ::std::runtime_error("Invalid percV parameter!");
					}

					//checking for percV modificator
					_fixPercV(prms, percV);

					return utils::setMapKey(prms, prm_percV_ht(), percV);
				}

				//////////////////////////////////////////////////////////////////////////
				//We have a tempStor, however it requires a knowledge of source data type, so we'll delay it spawning
				//untill it's known. Here we just mark its existence
				// Further more, each alg may have several different tmp storages that could be shared across algs
				// (but now it's required that these storages had identical properties)
				typedef common_meta::tstor_lenBased_ht tstor_lenBased_ht;
				typedef decltype(hana::make_basic_tuple(tstor_lenBased_ht())) algTStorDescr_t;

			private:
				template<typename HMT>
				static ::std::enable_if_t<utils::hasKey_v<HMT, oprm_invPercV_ht>> _fixPercV(HMT&& prms, prm_percV_t& percV) noexcept {
					if (static_cast<oprm_invPercV_t>(prms[oprm_invPercV_ht()])) {
						percV = prm_percV_t(1) - percV;
					}
				}
				template<typename HMT>
				static ::std::enable_if_t<!utils::hasKey_v<HMT, oprm_invPercV_ht>> _fixPercV(HMT&&, prm_percV_t&) noexcept {}

			protected:
				//this template helps to define the real type of temporarily storage required
				template<typename HST, typename VT, typename = ::std::enable_if_t<::std::is_same_v<HST, tstor_lenBased_ht>>>
				using TStor_tpl = base_class_t::template TStor_tpl<VT>;
				//should be defined in _*call class using CallerT only
			};


			struct Percentile_call : public tElementile_call_base<Percentile_meta> {
				typedef tElementile_call_base<Percentile_meta> base_class_t;
				using typename base_class_t::tstor_lenBased_ht;

				//defining timeseries mapping (using standard defs)
				typedef adpt_src_ht adpt_src_ht;
				typedef adpt_dest_ht adpt_dest_ht;

				typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

				template<typename CallerT, typename SubstHMT = adptDefSubstMap_t>
				static void call(CallerT&& C, const bool = true) noexcept {
					constexpr auto substMap = SubstHMT();
					auto& tstor = C.getTStor(tstor_lenBased_ht());
					const auto& prms = C.getPrms();
					T18_ASSERT(tstor._len() == _getLenPrm(prms) || !"TStor has invalid length!");
					base_class_t::percentile(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()]), tstor, _getPercVPrm(prms));
				}

			private:
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getPercVPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename base_class_t::prm_percV_descr>(hmPrms);
				}
			};

		}

		template<bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
		class tPercentile : public tAlg2ts_select<isCont, tPercentile<isCont, DVT, SVT, ContTplT>, code::Percentile_call, DVT, SVT, ContTplT> {
		public:
			typedef tAlg2ts_select<isCont, tPercentile<isCont, DVT, SVT, ContTplT>, code::Percentile_call, DVT, SVT, ContTplT> base_class_t;

		public:
			template<typename... Args>
			tPercentile(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}
		};

		typedef tPercentile<false> Percentile;
		typedef tPercentile<true> Percentile_c;

		/*template<typename DVT, typename SVT>
		class tPercentile : public tAlg<memb::simpleAdpt<DVT, SVT>, code::Percentile_call> {
		public:
			typedef tAlg<memb::simpleAdpt<DVT, SVT>, code::Percentile_call> base_class_t;
			typedef tPercentile<DVT, SVT> self_t;

			typedef typename base_class_t::srcAdpt_t srcAdpt_t;
			typedef typename base_class_t::destAdpt_t destAdpt_t;

			using typename base_class_t::prm_len_t;
			using typename base_class_t::prm_percV_t;

		public:

			template<typename D, typename S, typename HMT, typename = ::std::enable_if_t<
				hana::is_a<hana::map_tag, HMT> && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t>
				&& ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tPercentile(D&& d, S&& s, HMT&& prms) : base_class_t(pass2simpleAdpt(d, s), ::std::forward<HMT>(prms), *this)
			{}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tPercentile(D&& d, S&& s, prm_len_t len, prm_percV_t percV)
				: base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len, percV), *this)
			{}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tPercentile(D&& d, S&& s, prm_len_t len, int percV)
				: base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len, prm_percV_t(percV) / prm_percV_t(100)), *this)
			{}
		};

		typedef tPercentile<real_t, real_t> Percentile;

		template<int PercV>
		class PercentileN : public tPercentile<real_t, real_t> {
		public:
			typedef tPercentile<real_t, real_t> base_class_t;
			using typename base_class_t::prm_len_t;
		public:
			PercentileN(RealNCont_t& dest, const RealNCont_t& src, prm_len_t len) : base_class_t(dest, src, len, PercV) {}
		};*/

} }
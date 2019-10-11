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

#include "_base.h"
#include "code/elementile.h"

namespace t18 {
	namespace algs {
		//don't care much about performance now. Correctness and readability is the main priority.

		namespace code {
			
			struct Elementile_meta : public lenBased_meta<Elementile> {
				typedef lenBased_meta<Elementile> base_class_t;
				//using typename base_class_t::alg_class_t;

				//let's describe which parameters are required by the algo
				using typename base_class_t::prm_len_ht;
				using typename base_class_t::prm_len_t;
				using typename base_class_t::prm_len_descr;

				typedef decltype("rank"_s) prm_rank_ht;
				typedef typename base_class_t::prm_rank_t prm_rank_t;
				typedef decltype(hana::make_pair(prm_rank_ht(), hana::type_c<prm_rank_t>)) prm_rank_descr;

				typedef decltype(hana::make_map(prm_len_descr(), prm_rank_descr())) algPrmsDescr_t;

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
				static algPrmsMap_t prms2hmap(prm_len_t len, prm_rank_t elmIdx)noexcept {
					T18_ASSERT(len > 0 && elmIdx >= 0 && elmIdx < len);
					return hana::make_map(
						hana::make_pair(prm_len_ht(), len)
						, hana::make_pair(prm_rank_ht(), elmIdx)
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
					const auto len = prms[prm_len_ht()];
					if (len < 1) {
						T18_ASSERT(!"Invalid len parameter!");
						throw ::std::runtime_error("Invalid len parameter!");
					}
					const auto elmIdx = prms[prm_rank_ht()];
					if (elmIdx < 0 || elmIdx >= len) {
						T18_ASSERT(!"Invalid rank parameter!");
						throw ::std::runtime_error("Invalid rank parameter!");
					}

					return ::std::forward<HMT>(prms);
				}

				//////////////////////////////////////////////////////////////////////////
				//we do have a tempStor, however it requires a knowledge of source data type, so we'll delay it spawning
				//untill it's known. Here we just mark its existence
				// Further more, each alg may have several different tmp storages that could be shared across algs
				// (but now it's required that these storages had identical properties)
				typedef common_meta::tstor_lenBased_ht tstor_lenBased_ht;
				typedef decltype(hana::make_basic_tuple(tstor_lenBased_ht())) algTStorDescr_t;

			protected:
				//this template helps to define the real type of temporarily storage required
				template<typename HST, typename VT, typename = ::std::enable_if_t<::std::is_same_v<HST, tstor_lenBased_ht>>>
				using TStor_tpl = base_class_t::template TStor_tpl<VT>;
				//should be defined in _*call class using CallerT only
			};

			template<typename MetaT>
			struct tElementile_call_base : public MetaT {
				typedef MetaT meta_t;
				using typename meta_t::tstor_lenBased_ht;

				//////////////////////////////////////////////////////////////////////////
				//support for temp storage
				template<typename HST, typename CallerT, typename = ::std::enable_if_t<::std::is_same_v<HST, tstor_lenBased_ht>>>
				using TStor_tpl = typename meta_t::template TStor_tpl<HST, typename CallerT::src_value_t>;

				template<typename HST, typename CallerT>
				static void initTStor(const CallerT& C, TStor_tpl<HST, CallerT>& tstor, const HST&) {
					tstor.init(_getLenPrm(C.getPrms()));
				}

				using meta_t::minSrcHist;

				template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
				static size_t minSrcHist(const CallerT& C) noexcept {
					const auto& prms = C.getPrms();
					T18_ASSERT(C.getTStor(tstor_lenBased_ht())._len() == _getLenPrm(prms) || !"TStor has invalid length!");
					return meta_t::minSrcHist(prms);
				}

			private:
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getLenPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename meta_t::prm_len_descr>(hmPrms);
				}
			};


			struct Elementile_call : public tElementile_call_base<Elementile_meta> {
				typedef tElementile_call_base<Elementile_meta> base_class_t;
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
					base_class_t::elementile(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()]), tstor, _getRankPrm(prms));
				}
				
			private:
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static typename base_class_t::prm_rank_t _getRankPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename base_class_t::prm_rank_descr>(hmPrms);
				}
			};
		}


		template<bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
		class tElementile : public tAlg2ts_select<isCont, tElementile<isCont, DVT, SVT, ContTplT>, code::Elementile_call, DVT, SVT, ContTplT> {
		public:
			typedef tAlg2ts_select<isCont, tElementile<isCont, DVT, SVT, ContTplT>, code::Elementile_call, DVT, SVT, ContTplT> base_class_t;

		public:
			template<typename... Args>
			tElementile(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}
		};

		typedef tElementile<false> Elementile;
		typedef tElementile<true> Elementile_c;


		/*template<typename DVT, typename SVT>
		class tElementile : public tAlg<memb::simpleAdpt<DVT, SVT>, code::Elementile_call> {
		public:
			typedef tAlg<memb::simpleAdpt<DVT, SVT>, code::Elementile_call> base_class_t;
			typedef tElementile<DVT, SVT> self_t;

			typedef typename base_class_t::srcAdpt_t srcAdpt_t;
			typedef typename base_class_t::destAdpt_t destAdpt_t;

			using typename base_class_t::prm_len_t;
			using typename base_class_t::prm_rank_t;

		public:

			template<typename D, typename S, typename HMT, typename = ::std::enable_if_t<
				hana::is_a<hana::map_tag, HMT> && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t>
				&& ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tElementile(D&& d, S&& s, HMT&& prms) : base_class_t(pass2simpleAdpt(d, s), ::std::forward<HMT>(prms), *this)
			{}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tElementile(D&& d, S&& s, prm_len_t len, prm_rank_t elmIdx)
				: base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len, elmIdx), *this)
			{}
		};

		typedef tElementile<real_t, real_t> Elementile;
		
		class ElementileMedian : public tElementile<real_t, real_t> {
		public:
			typedef tElementile<real_t, real_t> base_class_t;
		public:
			ElementileMedian(RealNCont_t& dest, const RealNCont_t& src, size_t len) : base_class_t(dest, src, len, len / 2) {
				if (1 != len % 2) {
					T18_ASSERT(!"Length must be odd here!");
					throw ::std::runtime_error("Length must be odd here!");
				}
			}
		};

		template<size_t elmIdx>
		class ElementileN : public tElementile<real_t, real_t> {
		public:
			typedef tElementile<real_t, real_t> base_class_t;
		public:
			ElementileN(RealNCont_t& dest, const RealNCont_t& src, size_t len) : base_class_t(dest, src, len, elmIdx) {}
		};*/

	}
}
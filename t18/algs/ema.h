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

#include "code/ema.h"

namespace t18 {
	namespace algs {

		namespace code {

			struct EMA_common_meta {
				typedef common_meta::prm_gamma_ht prm_gamma_ht;
				typedef decltype("omgamma"_s) iprm_omgamma_ht;//has the same type as prm_gamma_t

			protected:
				//TODO: should be refactored into more generic form

				//must place enable_if in func parameter to make overloaded function signatures differ
				// Default template args aren't a part of function signature, and it's illegal to declare function overloads
				// with same signature. Also can't place it as return type, because decltype(auto) won't work there.
				template<typename ACT, typename LenPrmHT, typename HMT
					, bool c = hana::value(hana::contains(::std::remove_reference_t<HMT>(), prm_gamma_ht()))>
				static decltype(auto) _checkGammaPrmAvail(HMT&& p, ::std::enable_if_t<c>* = nullptr)noexcept {
					typedef typename ACT::prm_gamma_t prm_gamma_t;
					return hana::insert(hana::erase_key(::std::forward<HMT>(p), iprm_omgamma_ht())
						, hana::make_pair(iprm_omgamma_ht(), prm_gamma_t(1) - static_cast<prm_gamma_t>(p[prm_gamma_ht()]))
					);
				}
				template<typename ACT, typename LenPrmHT, typename HMT
					, bool c = hana::value(hana::contains(::std::remove_reference_t<HMT>(), prm_gamma_ht()))>
				static decltype(auto) _checkGammaPrmAvail(HMT&& p, ::std::enable_if_t<!c>* = nullptr)noexcept
				{
					//may fail here if the "len" parameter is also absent
					const auto v = p[LenPrmHT()];
					T18_ASSERT(v > 0);
					const auto g = ACT::makeGamma(static_cast<typename ACT::prm_len_t>(v));
					return hana::insert(
						hana::insert(hana::erase_key(::std::forward<HMT>(p), iprm_omgamma_ht())
							, hana::make_pair(prm_gamma_ht(), g)
						), hana::make_pair(iprm_omgamma_ht(), decltype(g)(1) - g)
					);
				}
			};


			//this class describes:
			// - required parameters of the MA algo (appends it to the MA)
			// - the run-time state
			// - the run-time temporary data storage
			struct EMA_meta : public lenBased_meta<EMA>, public EMA_common_meta {
				typedef EMA_common_meta base_meta_t;
				typedef lenBased_meta<EMA> base_class_t;
				
				//////////////////////////////////////////////////////////////////////////

				//let's describe which parameters are required by the algo
				//it has two params - len (required for proper initialization) & updateable on len parameter gamma
				using typename base_class_t::prm_len_ht;
				using typename base_class_t::prm_len_t;
				using typename base_class_t::prm_len_descr;

				using typename base_meta_t::prm_gamma_ht;
				typedef typename base_class_t::prm_gamma_t prm_gamma_t;
				typedef utils::Descr_t<prm_gamma_ht, prm_gamma_t> prm_gamma_descr;

				//make description of data required for the algo
				typedef utils::makeMap_t<prm_len_descr, prm_gamma_descr> algPrmsDescr_t;
				typedef utils::dataMapFromDescrMap_t<algPrmsDescr_t> algPrmsMap_t;
				//algPrmsMap_t is a type that should be given to the algo as the params
				// It should be returned by prms2hmap()

				//////////////////////////////////////////////////////////////////////////
				//also specify additional internal/derived params to make runtime algo parameter set
				using typename base_meta_t::iprm_omgamma_ht;
				typedef utils::Descr_t<iprm_omgamma_ht, prm_gamma_t> iprm_omgamma_descr;

				typedef utils::makeMap_t<iprm_omgamma_descr> algIPrmsDescr_t;

				typedef utils::unionMaps_t<algPrmsDescr_t, algIPrmsDescr_t> algFullPrmsDescr_t;
				typedef utils::dataMapFromDescrMap_t<algFullPrmsDescr_t> algFullPrmsMap_t;
				//algFullPrmsMap_t is a type that stores parameters as well as some internal data.
				// It should be returned by validatePrms()

				//////////////////////////////////////////////////////////////////////////
				static algPrmsMap_t prms2hmap(prm_len_t len)noexcept {
					T18_ASSERT(len > 0);
					return hana::make_map(
						hana::make_pair(prm_len_ht(), len)
						, hana::make_pair(prm_gamma_ht(), base_class_t::makeGamma(len))
					);
				}
				static algPrmsMap_t prms2hmap(prm_len_t len, prm_gamma_t g){
					T18_ASSERT(len > 0);
					//#TODO: validateLen here and everywhere where it's used
					base_class_t::validateGamma(g);
					return hana::make_map(
						hana::make_pair(prm_len_ht(), len)
						, hana::make_pair(prm_gamma_ht(), g)
					);
				}
				//////////////////////////////////////////////////////////////////////////
				//this function checks that passed parameters are suitable for the algo
				//i.e. that params have all the necessary keys and its values are convertible to the necessary types
				template<typename HMT>
				static auto validatePrms(HMT&& prms) {
					static_assert(utils::couldBeDataMap_v<::std::remove_reference_t<HMT>>, "");

					//if there are no 'gamma' parameter - make it out of 'len' parameter
					// and also add the omgamma parameter
					auto np = base_meta_t::template _checkGammaPrmAvail<base_class_t, prm_len_ht>(::std::forward<HMT>(prms));

					//testing that the prms is suitable for the alg
					utils::static_assert_hmap_conforms_descr<algFullPrmsDescr_t>(np);

					//finally checking real parameters values.
					if (np[prm_len_ht()] < 1) {
						T18_ASSERT(!"Invalid len parameter!");
						throw ::std::runtime_error("Invalid len parameter!");
					}
					base_class_t::validateGamma(np[prm_gamma_ht()]);

					return np;
				}
			};

			template<typename AlgCT>
			struct tEMAsi_meta : public AlgCT, public EMA_common_meta {
				//alg_class_t is a helper type that helps to distinguish one algorithm from another or to instantiate proper alg
				typedef AlgCT base_class_t;
				using typename base_class_t::prm_len_t;

				typedef EMA_common_meta base_meta_t;

				//it has only one parameter - gamma
				typedef typename base_meta_t::prm_gamma_ht prm_gamma_ht;
				typedef typename base_class_t::prm_gamma_t prm_gamma_t;
				typedef decltype(hana::make_pair(prm_gamma_ht(), hana::type_c<prm_gamma_t>)) prm_gamma_descr;

				typedef decltype(hana::make_map(prm_gamma_descr())) algPrmsDescr_t;

				//also define prm_len_ht as could be used to parse/validate prms to get gamma parameter
				typedef common_meta::prm_len_ht prm_len_ht;

				typedef utils::dataMapFromDescrMap_t<algPrmsDescr_t> algPrmsMap_t;
				//algPrmsMap_t is a type that should be given to the algo as the params
				// It should be returned by prms2hmap()

				//////////////////////////////////////////////////////////////////////////
				//also specify additional internal/derived params to make runtime algo parameter set
				using typename base_meta_t::iprm_omgamma_ht;
				typedef utils::Descr_t<iprm_omgamma_ht, prm_gamma_t> iprm_omgamma_descr;

				typedef utils::makeMap_t<iprm_omgamma_descr> algIPrmsDescr_t;

				typedef utils::unionMaps_t<algPrmsDescr_t, algIPrmsDescr_t> algFullPrmsDescr_t;
				typedef utils::dataMapFromDescrMap_t<algFullPrmsDescr_t> algFullPrmsMap_t;
				//algFullPrmsMap_t is a type that stores parameters as well as some internal data.
				// It should be returned by validatePrms()

				//////////////////////////////////////////////////////////////////////////

				static algPrmsMap_t prms2hmap(prm_len_t len)noexcept {
					T18_ASSERT(len > 0);
					return hana::make_map(
						hana::make_pair(prm_gamma_ht(), base_class_t::makeGamma(len))
					);
				}
				static algPrmsMap_t prms2hmap(prm_gamma_t g) {
					base_class_t::validateGamma(g);
					return hana::make_map(
						hana::make_pair(prm_gamma_ht(), g)
					);
				}

				//this function checks that passed parameters are suitable for the algo
				//i.e. that params have all the necessary keys and its values are convertible to the necessary types
				template<typename HMT>
				static auto validatePrms(HMT&& prms) {
					static_assert(utils::couldBeDataMap_v<::std::remove_reference_t<HMT>>, "");

					//if there are no 'gamma' parameter - make it out of 'len' parameter
					auto np = base_meta_t::template _checkGammaPrmAvail<base_class_t, prm_len_ht>(::std::forward<HMT>(prms));

					//testing that the prms is suitable for the alg
					utils::static_assert_hmap_conforms_descr<algFullPrmsDescr_t>(np);

					//finally checking real parameters values.
					base_class_t::validateGamma(np[prm_gamma_ht()]);

					return np;
				}

				//minSrcHist variant to accept map of params
				using base_class_t::minSrcHist;
				template<typename HMT, typename = ::std::enable_if_t < hana::is_a < hana::map_tag, HMT > >>
				static size_t minSrcHist(HMT&& prms) noexcept {
					return base_class_t::minSrcHist(0);
				}

				//////////////////////////////////////////////////////////////////////////
				// state and tstor is empty (unused)
				typedef void algState_t;

				typedef void algTStorDescr_t;
			};

			typedef tEMAsi_meta<EMAsi> EMAsi_meta;

			template<bool bAmiMetastockInit>
			using EMA_meta_select = ::std::conditional_t<bAmiMetastockInit, EMA_meta, EMAsi_meta>;


			//the class describes a simple caller of MA algorithm.
			//Caller class isolates the code of algorithm from the knowledge of any parameter/state/tstor storage.
			template<bool bAmiMetastockInit>
			struct tEMA_call : public EMA_meta_select<bAmiMetastockInit> {
				typedef EMA_meta_select<bAmiMetastockInit> meta_t;

				//defining timeseries mapping (using standard defs)
				typedef adpt_src_ht adpt_src_ht;
				typedef adpt_dest_ht adpt_dest_ht;

				typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

				template<typename CallerT, typename SubstHMT = adptDefSubstMap_t, bool b = bAmiMetastockInit>
				static ::std::enable_if_t<b> call(CallerT&& C, const bool = true) noexcept {
					constexpr auto substMap = SubstHMT();
					const auto& prms = C.getPrms();
					meta_t::ema(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()])
						, _getLenPrm(prms), _getGammaPrm(prms), _getOmgammaIPrm(prms));
				}

				template<typename CallerT, typename SubstHMT = adptDefSubstMap_t, bool b = bAmiMetastockInit>
				static ::std::enable_if_t<!b> call(CallerT&& C, const bool = true) noexcept {
					constexpr auto substMap = SubstHMT();
					const auto& prms = C.getPrms();
					meta_t::ema(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()])
						, 0, _getGammaPrm(prms), _getOmgammaIPrm(prms));
				}

				using meta_t::minSrcHist;

				template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
				static  size_t minSrcHist(const CallerT& C) noexcept {
					return meta_t::minSrcHist(C.getPrms());
				}

			private:
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getLenPrm(const HMT& hmPrms)noexcept {
					static_assert(meta_t::bAmiMetastockInit && bAmiMetastockInit, "");
					return utils::hmap_get<typename meta_t::prm_len_descr>(hmPrms);
				}
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getGammaPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename meta_t::prm_gamma_descr>(hmPrms);
				}
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static auto _getOmgammaIPrm(const HMT& hmPrms)noexcept {
					return utils::hmap_get<typename meta_t::iprm_omgamma_descr>(hmPrms);
				}
			};

			typedef tEMA_call<true> EMA_call;
			typedef tEMA_call<false> EMAsi_call;
		}
		
		template<bool _bAmiMetastockInit, bool isCont, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
		class tEMA : public tAlg2ts_select<isCont, tEMA<_bAmiMetastockInit, isCont, DVT, SVT, ContTplT>, code::tEMA_call<_bAmiMetastockInit>, DVT, SVT, ContTplT> {
		public:
			typedef tAlg2ts_select<isCont, tEMA<_bAmiMetastockInit, isCont, DVT, SVT, ContTplT>, code::tEMA_call<_bAmiMetastockInit>, DVT, SVT, ContTplT> base_class_t;
			typedef typename base_class_t::prm_len_t prm_len_t;

		public:
			template<typename... Args>
			tEMA(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

			template<typename D, typename S>
			tEMA(D&& d, S&& s, prm_len_t len) : base_class_t(::std::forward<D>(d), ::std::forward<S>(s), base_class_t::prms2hmap(len)) {}
		};

		typedef tEMA<true, false, real_t, real_t> EMA;
		typedef tEMA<false, false, real_t, real_t> EMAsi;

		typedef tEMA<true, true, real_t, real_t> EMA_c;
		typedef tEMA<false, true, real_t, real_t> EMAsi_c;

		/*template<typename DVT, typename SVT, bool _bAmiMetastockInit = true>
		class tEMA : public tAlg<memb::simpleAdpt<DVT, SVT>, code::tEMA_call<_bAmiMetastockInit>> {
		public:
			typedef tAlg<memb::simpleAdpt<DVT, SVT>, code::tEMA_call<_bAmiMetastockInit>> base_class_t;
			typedef tEMA<DVT, SVT, _bAmiMetastockInit> self_t;

			typedef typename base_class_t::srcAdpt_t srcAdpt_t;
			typedef typename base_class_t::destAdpt_t destAdpt_t;

			typedef typename base_class_t::prm_len_t prm_len_t;
			typedef typename base_class_t::prm_gamma_t prm_gamma_t;

		public:
			template<typename D, typename S, typename HMT, typename = ::std::enable_if_t<
				hana::is_a<hana::map_tag, HMT> && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t>
				&& ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tEMA(D&& d, S&& s, HMT&& prms) : base_class_t(pass2simpleAdpt(d, s), ::std::forward<HMT>(prms), *this) {}

			template<typename D, typename S, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tEMA(D&& d, S&& s, prm_len_t len) : base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len), *this) {}

			template<typename D, typename S, bool b = _bAmiMetastockInit, typename = ::std::enable_if_t<
				b && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tEMA(D&& d, S&& s, prm_len_t len, prm_gamma_t gamma)
				: base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(len, gamma), *this) {}

			template<typename D, typename S, bool b = _bAmiMetastockInit, typename = ::std::enable_if_t<
				!b && ::std::is_same_v<::std::remove_reference_t<D>, destAdpt_t> && ::std::is_same_v<::std::remove_reference_t<S>, srcAdpt_t>
			>>
			tEMA(D&& d, S&& s, prm_gamma_t gamma) : base_class_t(pass2simpleAdpt(d, s), base_class_t::prms2hmap(gamma), *this) {}
		};

		typedef tEMA<real_t, real_t, true> EMA;
		typedef tEMA<real_t, real_t, false> EMAsi;*/

	}
}
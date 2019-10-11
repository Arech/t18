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

namespace t18 {
namespace algs {

	namespace code {

		//LTFPercentile is an algorithm that returns the specified percentile of the lower timeframe timeseries corresponding
		//to N bars of current timeframe. For example, one may calculate 10%-percentile of a M1 timeseries for a time span of 3 M15 bar
		// (45minutes).

		struct LTFPercentile_meta : public Percentile_meta {};


		struct LTFPercentile_call : public LTFPercentile_meta {
			typedef LTFPercentile_meta base_class_t;
			using typename base_class_t::tstor_lenBased_ht;

			//////////////////////////////////////////////////////////////////////////
			//support for temp storage
			template<typename HST, typename CallerT, typename = ::std::enable_if_t<::std::is_same_v<HST, tstor_lenBased_ht>>>
			using TStor_tpl = typename base_class_t::template TStor_tpl<HST, typename CallerT::src_value_t>;

			template<typename HST, typename CallerT>
			static void initTStor(const CallerT& C, TStor_tpl<HST, CallerT>& tstor, const HST&) {
				//will reinit to the correct length (which may be variable) later during the call
				tstor.init(_getLenPrm(C.getPrms()));
			}

			//hide base definition of minSrcHist
			//using base_class_t::minSrcHist;
			//redefining minSrcHist() to return actual history required for the alg at this moment of time
			template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
			static size_t minSrcHist(const CallerT& C) noexcept {
				const auto& prms = C.getPrms();
				//T18_ASSERT(C.getTStor(tstor_lenBased_ht())._len() == _getLenPrm(prms) || !"TStor has invalid length!");
				//just passing prms and finally returning back value of length parameter. 
				return base_class_t::minSrcHist(prms);
			}

			//////////////////////////////////////////////////////////////////////////

			//defining timeseries mapping (using standard defs)
			typedef adpt_src_ht adpt_src_ht;
			typedef adpt_dest_ht adpt_dest_ht;

			typedef makeAdptDefSubstMap_t<adpt_src_ht, adpt_dest_ht> adptDefSubstMap_t;

			template<typename CallerT, typename SubstHMT = adptDefSubstMap_t>
			static void call(CallerT&& C, const bool = true) noexcept {
				//we must resize tmp store first
				const auto ush = C.getNumOfUnseenBars();
				if (LIKELY(ush > 0)) {
					auto& tstor = C.getTStor(tstor_lenBased_ht());
					tstor.reinit(ush);
					constexpr auto substMap = SubstHMT();
					base_class_t::percentile(C.getTs(substMap[adpt_dest_ht()]), C.getTs(substMap[adpt_src_ht()]), tstor, _getPercVPrm(C.getPrms()));
				}
			}

		private:
			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			static auto _getPercVPrm(const HMT& hmPrms)noexcept {
				return utils::hmap_get<typename base_class_t::prm_percV_descr>(hmPrms);
			}

			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			static auto _getLenPrm(const HMT& hmPrms)noexcept {
				return utils::hmap_get<typename base_class_t::prm_len_descr>(hmPrms);
			}
		};


	}

	//////////////////////////////////////////////////////////////////////////
	//allows to execute a callback that inspects contents of timeseries of a lower timeframe and calculates some function on it.
	// Note that this algorithm code MUST always be called; even if the src TS doesn't have enought bars.
	template<bool isCont, typename CodeCallT, typename DVT = real_t, typename SVT = real_t, template<class> class ContTplT = TsCont_t>
	class tInspectLowerTF 
		: public tAlg2ts_select<isCont, tInspectLowerTF<isCont, CodeCallT, DVT, SVT, ContTplT>, CodeCallT, DVT, SVT, ContTplT> {
	public:
		typedef tAlg2ts_select<isCont, tInspectLowerTF<isCont, CodeCallT, DVT, SVT, ContTplT>, CodeCallT, DVT, SVT, ContTplT> base_class_t;
		
		using typename base_class_t::algPrmsMap_t;
		using typename base_class_t::self_ref_t;		

	protected:
		const size_t& m_srcTotalBarsRef;
		//size_t m_lastTotalBars = 0;
		::boost::circular_buffer<size_t> m_srcBars;

	public:
		template<typename srcTsHStrT, typename D, typename SrcTsStorT, typename PrmsT
			, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, srcTsHStrT>>
		>
		tInspectLowerTF(D&& d, const SrcTsStorT& sLowTf, srcTsHStrT srcTsName, PrmsT&& prms)
			: base_class_t(::std::forward<D>(d), sLowTf.getTs(srcTsName), ::std::forward<PrmsT>(prms))
			, m_srcTotalBarsRef(sLowTf.TotalBars())
			//, m_srcBars(base_class_t::_getLenPrm(base_class_t::getPrms()), 0)
			, m_srcBars(base_class_t::minSrcHist())
		{
			static_assert(::std::is_reference_v<decltype(sLowTf.TotalBars())>,"");

			m_srcBars.push_front(0);
			//if (base_class_t::_getLenPrm(base_class_t::getPrms()) != 1) {
			if (base_class_t::minSrcHist() != 1) {
				STDCOUTL("tInspectLowerTF: length==1 did NOT tested for correctness!");
			}
		}

		template<typename srcTsHStrT, typename D, typename SrcTsStorT, typename PrmsT
			, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, srcTsHStrT>>
		>
		tInspectLowerTF(D&& d, const SrcTsStorT& sLowTf, PrmsT&& prms)
			: tInspectLowerTF(::std::forward<D>(d), sLowTf, srcTsHStrT(), ::std::forward<PrmsT>(prms))
		{}

		self_ref_t operator()(bool bClose) noexcept {
			T18_ASSERT(m_srcTotalBarsRef > 0);
			base_class_t::operator()(bClose);
			if (bClose) {
				m_srcBars.push_front(m_srcTotalBarsRef);
			}
			return base_class_t::get_self();
		}

		size_t getNumOfUnseenBars()const noexcept {
			return m_srcBars.full() ? m_srcTotalBarsRef - m_srcBars.back() : 0;
		}

	};

	typedef tInspectLowerTF<false, code::LTFPercentile_call> LTFPercentile;
	typedef tInspectLowerTF<true, code::LTFPercentile_call> LTFPercentile_c;

}
}
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

#include "MaCross.h"
#include "../debug/tsqDump.h"

namespace t18 {
	namespace ts {
		namespace hana = ::boost::hana;
		using namespace hana::literals;

		//MaCrossO is a small modification of MaCross TS that performs all actions on main TF bar opening only
		// Such TS is much easier to implement in a common trading terminal (such as AmiBroker) to compare the output
		
		struct MaCrossOCompiletimePrms : public MaCrossCompiletimePrms {
			//changing the timeseries used to feed the maSlow and maFast
			typedef open_ht maSlowSrcTs_ht;
			typedef open_ht maFastSrcTs_ht;
		};

		template<typename CtPrmsT = MaCrossOCompiletimePrms>
		class MaCrossO final : public _MaCross<MaCrossO<CtPrmsT>, CtPrmsT> {
			typedef _MaCross<MaCrossO<CtPrmsT>, CtPrmsT> base_class_t;

		public:
			using typename base_class_t::self_t;
			using base_class_t::get_self;

		protected:
			using base_class_t::m_TF;
			using base_class_t::m_tsCont;
			using base_class_t::m_minBars;
			using typename base_class_t::myBar;
			using typename base_class_t::maFast_ht;
			using typename base_class_t::maSlow_ht;

			debug::tsqDump m_dumper;

		public:
			template<typename MktT, typename RtPrmsT>
			MaCrossO(MktT& m, exec::tradingInterface& t, RtPrmsT& prms)
				: base_class_t(m, t, prms), m_dumper(prms.pDbgFile)
			{
				static_assert(::std::is_same_v<open_ht, typename base_class_t::maSlowSrcTs_ht>, "WTF?");

				//don't need to use base TF at all, so releasing the callback
				base_class_t::m_hOnBaseTF1.deregister();

				//rewiring main tf to the new function
				base_class_t::m_hOnMainTF1 = m_TF.registerOnNewBarOpen(::std::bind(&self_t::onMainTFOpen, &get_self(), ::std::placeholders::_1));
			}

			void onMainTFOpen(const tsq_data& dto) {
				//if (m_minBars > m_TF.TotalBars()) return;//we must buildup some source data history first
				//disabling to check correctness of vars calculation

				//this means that m1 event handler never run, therefore m_tsCont doesn't have a corresponding bar to update
				//we must add it first
				m_tsCont.storeBar(myBar({ 0,0 }).to_hmap());
				//we must update the last bar of m_tsCont to reflect real end of day data.
				get_self().updateTimeseries();

				get_self().dumpVars(dto);

				//if (m_tsCont.size() >= 2) {
				if (m_TF.TotalBars() >= 2 + m_minBars) {
					get_self().makeTradeDecisions();
				}
			}

			void dumpVars(mxTimestamp dt) noexcept {
				m_dumper.dump(dt, m_tsCont.get(maSlow_ht(), 0), 0);
				m_dumper.dump(dt, m_tsCont.get(maFast_ht(), 0), 1);
			}

		};


		template<typename MaCrossT = MaCrossO<MaCrossCompiletimePrms>>
		struct MaCrossOSetup : public MaCrossSetup<MaCrossT> {
		private:
			typedef MaCrossSetup<MaCrossT> base_class_t;
		public:
			const char* pDbgFile = nullptr;

			/*template<typename ExecT>
			void setup(ExecT& exe) {
				base_class_t::setup(exe);

			}*/

		};
	}
}
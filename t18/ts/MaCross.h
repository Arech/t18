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
#include "../tfConverter/dailyhm.h"
#include "../algs/ma.h"

namespace t18 { namespace ts {
	namespace hana = ::boost::hana;
	using namespace hana::literals;


	//MaCross is a simple trade system that places a long order when daily MA(fast) becomes greater than daily MA(slow),
	//using a 1% stoploss and 3% takeprofit. To make sure, that trading is possible (an exchange is working), the TS calculates
	// all the necessary statistics (MAs) and places orders at max 30 minutes before the end of a day.

	//Compile-time objects and types of this struct is used to parametrize the template of TS type
	struct MaCrossCompiletimePrms {
		//sample parameters
		static constexpr size_t maSlow = 30;
		static constexpr int minutesBeforeEod = 30;

		static constexpr int stopLossPerc = 1;
		static constexpr int takeProfitPerc = 3;

		static constexpr int maxSkippedDaysNotifs = 15;

		//define a timeseries used to feed the maSlow and maFast
		typedef close_ht maSlowSrcTs_ht;
		typedef close_ht maFastSrcTs_ht;

		typedef algs::MA algMaSlow_t;
		typedef algs::MA algMaFast_t;

		//////////////////////////////////////////////////////////////////////////
		//we must describe a TickerStor type the strategy require
		typedef decltype("base"_s) baseTf_hst;
		typedef decltype("d"_s) mainTf_hst;

		typedef timeseries::Timeframe<tfConverter::baseOhlc> baseTf_t;
		typedef timeseries::Timeframe<tfConverter::dailyhmOhlc> mainTf_t;
		typedef tickerServer<true, utils::makeMap_t<
			utils::Descr_t<baseTf_hst, baseTf_t>
			, utils::Descr_t<mainTf_hst, mainTf_t>
			>> TheTicker_t;

		typedef utils::make_set_t<TheTicker_t> TickersSet_t;
	};


	//trading system might have parameters that are already known in compile time. Pass them here
	template<typename FinalPolymorphChild, typename CompiletimePrmsT>
	class _MaCross : public _base_poly<FinalPolymorphChild> {
	private:
		typedef _base_poly<FinalPolymorphChild> base_class_t;

	public:
		typedef typename base_class_t::self_t self_t;
		typedef CompiletimePrmsT ct_prms_t;
		static constexpr int mainTfId = 60 * 24;

		typedef typename ct_prms_t::TheTicker_t ticker_t;
		typedef typename ct_prms_t::mainTf_t mainTf_t;

	protected:
		//lets define types to store and retrieve runtime data

		typedef typename ct_prms_t::maSlowSrcTs_ht maSlowSrcTs_ht;
		typedef typename ct_prms_t::maFastSrcTs_ht maFastSrcTs_ht;

		typedef decltype("S"_s) maSlow_ht;
		typedef decltype("F"_s) maFast_ht;

		typedef decltype(hana::make_map(
			hana::make_pair(maSlow_ht(), hana::type_c<real_t>)
			, hana::make_pair(maFast_ht(), hana::type_c<real_t>)
		)) tsData_ht;

		typedef timeseries::TsStor<tsData_ht> TsCont_t;

		struct myBar {
			real_t s, f;

			decltype(auto) to_hmap() {
				return hana::make_map(
					hana::make_pair(maSlow_ht(), s)
					, hana::make_pair(maFast_ht(), f)
				);
			}
		};

		typedef typename ct_prms_t::algMaSlow_t algMaSlow_t;
		typedef typename ct_prms_t::algMaFast_t algMaFast_t;

	public:
		using base_class_t::get_self;

	protected:
		//using base_class_t::m_Market;
		using base_class_t::m_Trading;

		ticker_t& m_Ticker;
		mainTf_t& m_TF;
		utils::regHandle m_hOnMainTF1, m_hOnBaseTF1;//for the automatic handle release on object destruction

		TsCont_t m_tsCont;//this is where we would store the calculated MA's

		algMaSlow_t m_maSlowCalc;
		algMaFast_t m_maFastCalc;

		const size_t m_minBars;
		const mxTime m_EarliestDecisionMoment;

		int m_skippedDaysNotifs = 0;

		mxDate m_dayBarAdded;//will help us add only one (daily) bar into m_tsCont while handling M1 events
		bool m_bTradedThisDay = false;

		const real_t m_SLPerc;//stop loss percent 
		const real_t m_TPPerc;

	public:
		//better make it as a static member function to prevent code dilution amongst many places
		template<typename RtPrmsT>
		static size_t minBarsRequired(const RtPrmsT& prms) noexcept{
			return ::std::max(decltype(self_t::m_maSlowCalc)::minSrcHist(ct_prms_t::maSlow), decltype(self_t::m_maFastCalc)::minSrcHist(prms.maFast));
		}

		size_t minBarsRequired()const noexcept {
			return ::std::max(m_maFastCalc.minSrcHist(), m_maSlowCalc.minSrcHist());
		}

		template<typename MktT, typename RtPrmsT>
		_MaCross(MktT& m, exec::tradingInterface& t, RtPrmsT& prms)
			: base_class_t(t)
			, m_Ticker(m.template getTicker<ticker_t>(prms.pTickerName))
			, m_TF(m_Ticker.template getTf<typename ct_prms_t::mainTf_hst>())
			, m_hOnMainTF1(m_TF.registerOnNewBarClose(::std::bind(&self_t::onMainTFClose, &get_self(), ::std::placeholders::_1)))
			//, m_hOnBaseTF1(m_Ticker.registerOnNewBarOpen(::std::bind(&self_t::onBaseTFOpen, &get_self(), ::std::placeholders::_1)))
			, m_hOnBaseTF1(m_Ticker.template getTf<typename ct_prms_t::baseTf_hst>()
				.registerOnNewBarOpen(::std::bind(&self_t::onBaseTFOpen, &get_self(), ::std::placeholders::_1)))
			, m_tsCont(2)//we don't need large history for MAs
			, m_maSlowCalc(m_tsCont._getTs<maSlow_ht>(), m_TF.template getTs<maSlowSrcTs_ht>(), ct_prms_t::maSlow)
			, m_maFastCalc(m_tsCont._getTs<maFast_ht>(), m_TF.template getTs<maFastSrcTs_ht>(), prms.maFast)
			, m_minBars(minBarsRequired())
			, m_EarliestDecisionMoment(m_Ticker.getEndOfSession() - mxTime(tag_milTime(), ct_prms_t::minutesBeforeEod * 100))
			, m_SLPerc(real_t(ct_prms_t::stopLossPerc) / 100)
			, m_TPPerc(real_t(ct_prms_t::takeProfitPerc) / 100)
		{}

		~_MaCross() {}

		//this callback will fire on every M1 bar start
		//Note, that we're listening on ticker object, so it schedules callbacks before timeframes processes new data.
		// It doesn't matter for this setup, however, if we're to use M1 data, we'd better listen on M1 timeframe object
		void onBaseTFOpen(const tsq_data& dto) {
			//The following assert MUST never fire.
			T18_COMP_SILENCE_FLOAT_CMP_UNSAFE
			T18_ASSERT(dto == m_Ticker.getLastQuote());
			T18_COMP_POP

			if (m_minBars > m_TF.TotalBars()) return;//we must buildup some source data history first
			//note that despite handling M1 bar open event here, we were acting on the daily timeframe data m_TF
			
			if (dto.Time() >= m_EarliestDecisionMoment && !m_bTradedThisDay) {
				// we should add/update timeseries used in making trading decision and make a trade if necessary
				if (m_dayBarAdded != dto.Date()) {
					m_dayBarAdded = dto.Date();
					// first we add a placeholder bar into container and then performing an update.
					// It's the best strategy, because on a sufficiently liquid market there can be at max 5 onBaseTFOpen()
					// callbacks before the end of the day (while using default "act at max 5 minutes before EOD" strategy)
					m_tsCont.storeBar(myBar({ 0,0 }).to_hmap());
				}
				get_self().updateTimeseries(false);

				if (m_tsCont.size()>=2) {
					m_bTradedThisDay = get_self().makeTradeDecisions();
				}
			}
		}

		//this callback will fire on every Daily (main/work TF) bar completion
		void onMainTFClose(const tsohlcv& bar) {
			//T18_ASSERT(bar.c == m_Ticker.getLastKnownQuote().q && bar.DT() <= m_Ticker.getLastKnownQuote().DT());
			//this assert actually may and SHOULD fire in some perfectly normal cases!
			
			if (m_minBars > m_TF.TotalBars()) return;//we must buildup some source data history first

			if (m_dayBarAdded != bar.Date()) {
				//this means that m1 event handler never run, therefore m_tsCont doesn't have a corresponding bar to update
				//we must add it first
				m_tsCont.storeBar(myBar({ 0,0 }).to_hmap());

				if (m_minBars != m_TF.TotalBars()) {
					if (ct_prms_t::maxSkippedDaysNotifs > m_skippedDaysNotifs) {
						//notifying user (there'll be a lot of notifications actually!)
						STDCOUTL("Hey! Daily bar " << bar.Date().to_string() << " had no M1 bars near its ending!");
						if (ct_prms_t::maxSkippedDaysNotifs == ++m_skippedDaysNotifs) {
							STDCOUTL("...silencing notifications from now...");
						}
					}
				}else T18_ASSERT(m_dayBarAdded.empty());//first time here
			}

			//we must update the last bar of m_tsCont to reflect real end of day data.
			get_self().updateTimeseries();

			//preparing for the next day
			m_bTradedThisDay = false;
		}

		void updateTimeseries(const bool bOnClose = true) {
			//it's extremely important to make sure both of these statements are true
			//T18_ASSERT(m_TF.size() >= m_minBars && m_TF.TotalBars() - m_minBars + 1 == m_tsCont.TotalBars());
			// not so for some derived classes

			m_maSlowCalc(bOnClose);
			m_maFastCalc(bOnClose);
		}

		//return whether a trade has been made and no more trades should be done today
		bool makeTradeDecisions() {
			T18_ASSERT(m_tsCont.size() >= 2);

			const auto& slow = m_tsCont.getTs<maSlow_ht>();
			const auto& fast = m_tsCont.getTs<maFast_ht>();

			const bool bBuy = slow[1] < slow[0] && fast[0]>slow[0] && fast[1]<slow[1];
			if (bBuy) {
				m_Trading.newMarketLong(m_Ticker, 1
					, PriceRel(m_Ticker.getTickerBase(), m_SLPerc), PriceRel(m_Ticker.getTickerBase(), m_TPPerc));
			}

			return bBuy;
		}
	};

	//class to instantiate original MaCross trading strategy
	template<typename CompiletimePrmsT>
	class MaCross final : public _MaCross<MaCross<CompiletimePrmsT>, CompiletimePrmsT> {
		typedef _MaCross<MaCross<CompiletimePrmsT>, CompiletimePrmsT> base_class_t;
	public:
		template<typename... Args>
		MaCross(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}
	};

	//this struct fields should be used to populate const objects of TS
	//baseTfMins is just a sample parameter showing that setup class might be templated too
	template<typename MaCrossT = MaCross<MaCrossCompiletimePrms>>
	struct MaCrossSetup {
		typedef MaCrossT ts_t;
		typedef typename ts_t::ct_prms_t ct_prms_t;

		typedef typename ct_prms_t::TheTicker_t TheTicker_t;
		typedef typename ct_prms_t::baseTf_hst baseTf_hst;
		typedef typename ct_prms_t::mainTf_hst mainTf_hst;


		//we must define here or at a derived class the exact type of trade system we'd use
		//it'll be instantiated later by internal framework code
		static_assert(::std::is_base_of_v<_MaCross<MaCrossT, ct_prms_t>, MaCrossT>, "MaCrossT must be derived from _MaCross");

		//sample parameter
		size_t maFast=0;
		const char* pTickerName=nullptr;
		real_t tickerMinPriceDelta = real_t(0);
		unsigned tickerLotSize = 0;

		//ExecT is a type that defines an object implementing interface to manipulate all market data
		// (usually it is MarketDataStorServ type)
		//purpose of the setup() function is to prepare/setup trade system execution environment and return a set
		// of run-time prms used to construct (generally populate const fields of) TS object
		template<typename ExecT>
		void setup(ExecT& exe) {
			//we must define here which tickers and timeframes our TS will use
			//first decide what are the runtime parameters.
			maFast = 10;
			//ticker name also specifies the ticker csv data file
			if (pTickerName) {
				T18_ASSERT(tickerLotSize > 0 && tickerMinPriceDelta > 0);
			} else {
				T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
				T18_ASSERT(tickerLotSize == 0 && tickerMinPriceDelta == real_t(0));
				T18_COMP_POP;
				pTickerName = "GAZP_170901_190407-tst";
				tickerMinPriceDelta = real_t(.01);
				tickerLotSize = 1u;
			}

			_do_setup(exe);
		}

	protected:
		template<typename ExecT>
		void _do_setup(ExecT& exe) {
			//btw, name of ticker in case of history backtesting also defines a name of csv file with quotation data.
			// newTicker() arguments are exactly the arguments of ticker() constructor with a first argument stripped.
			// we will use M1 timeframe only to track stoplosses and for that we don't need M1 history at all.
			// So we're going to convert M1 directly to Daily and use D timeframe only (it'll also be the base TF for the ticker)
			// First, find out what's the max data history required for the TS
			size_t maxHist = ts_t::minBarsRequired(*this);

			T18_ASSERT(tickerMinPriceDelta > 0);
			//we need M1 only to have a callback on NewBarOpen
			auto& tickr = exe.template newTicker<TheTicker_t>(pTickerName, tickerMinPriceDelta, baseTf_hst(), 1u, 1);
			tickr.template createTf<mainTf_hst>(maxHist, ts_t::mainTfId);

			tickr.setEndOfSession(mxTime(tag_milTime(), 185000));
			tickr.setLotSize(tickerLotSize);
		}
	};
	
} }

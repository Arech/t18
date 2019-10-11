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

#include <functional>
#include "../utils/regHandle.h"
#include "../date_time.h"
#include "../market/ticker.h"
#include "../timefilter.h"

namespace t18 {
	
	class trade;//fwd declaration
	class tradeEx;

	//we MUST specify a ticker to use for price level definition to make possible to open/sl/tp using non base ticker
	struct PriceRel {
		typedef tickerBase tickerBase_t;

	protected:
		const tickerBase_t* m_pTicker{ nullptr };
		const real_t m_rv{ 0 };

	public:
		PriceRel() {}
		PriceRel(const tickerBase_t& Tickr, real_t val) : m_pTicker(&Tickr), m_rv(val) {
			if (UNLIKELY(val < 0 || val >= 1)) {
				T18_ASSERT(!"Relative value must be within [0,1) range");
				throw ::std::runtime_error("Relative value must be within [0,1) range");
			}
		}

		bool isValid()const noexcept { return static_cast<bool>(m_pTicker); }
		const auto& getTicker()const noexcept {
			T18_ASSERT(isValid());
			return *m_pTicker;
		}
		auto getRv()const noexcept {
			T18_ASSERT(isValid());
			return m_rv;
		}
	};

	struct PriceAbs {
		typedef tickerBase tickerBase_t;

	protected:
		//valid price level MUST have ticker specified
		const tickerBase_t* m_pTicker{ nullptr };
		//zero in a price level is OK. It may be updated later
		real_t m_lvl{ 0 };

	public:
		PriceAbs() {}
		PriceAbs(const tickerBase_t& Tickr, real_t val) : m_pTicker(&Tickr), m_lvl(val) {
			if (UNLIKELY(val < 0)) {
				T18_ASSERT(!"Absolute value can't be <0");
				throw ::std::runtime_error("Absolute value can't be <0");
			}
		}

		bool isValid()const noexcept { return static_cast<bool>(m_pTicker); }
		bool isSet()const noexcept { return static_cast<bool>(m_pTicker) && m_lvl > 0; }

		const auto& getTicker()const noexcept {
			T18_ASSERT(isValid());
			return *m_pTicker;
		}
		auto getLvl()const noexcept {
			T18_ASSERT(isValid());
			return m_lvl;
		}
		//it's ok to set zero here to disable level
		void updateLvl(real_t val) {
			T18_ASSERT(isValid());
			if (UNLIKELY(val < 0)) {
				T18_ASSERT(!"Absolute value can't be <0");
				throw ::std::runtime_error("Absolute value can't be <0");
			}
			m_lvl = val;
		}
		void clean() { return updateLvl(0); }

		static PriceAbs from(const PriceRel& PR, bool bHigher, real_t basePrice)noexcept {
			T18_ASSERT(PR.isValid());
			return PriceAbs(PR.getTicker(), basePrice * (real_t(1) + PR.getRv()*(2 * static_cast<real_t>(bHigher) - real_t(1))));
		}
	};

	namespace exec {

		typedef size_t tradeId_t;

		struct OrderData {
			typedef tickerBase tickerBase_t;

		//protected:
		public:
			// A ticker on which a trade is executed. MAY BE different from a ticker in m_prOpen!!
			const tickerBase_t* m_pTickr = nullptr;
			// ticker and price level to monitor to execute a trade. If the ticker is differs from m_pTickr
			// than when price condition on the ticker is met, a trade will be executed on m_pTickr using current MARKET price!
			PriceAbs m_prOpen;

			PriceAbs m_prSl;
			PriceAbs m_prTp;

			//an intraday time period during which the order is considered to be active
			TimeFilter m_timeActive;
			//#todo should the m_timeActive be set need some kind of unconditional SL to prevent heavy losses


			volume_t m_nShares;

			bool m_bLong;

		private:
			T18_ATTR_NORETURN static void _throwInvalidRelPrice() {
				T18_ASSERT(!"relative price specification MUST have the same ticker as prOpen");
				throw ::std::runtime_error("relative price specification MUST have the same ticker as prOpen");
			}

		public:
			OrderData() {}

			OrderData(bool bLong, const tickerBase_t& _Ticker, const PriceAbs& prOpen, volume_t nShares
				, const PriceAbs& sl = PriceAbs(), const PriceAbs& tp = PriceAbs(), const TimeFilter& tflt = TimeFilter()) noexcept
			{
				T18_ASSERT(prOpen.isSet());
				set(bLong, _Ticker, prOpen, nShares, sl, tp, tflt);
			}
			OrderData(bool bLong, const tickerBase_t& _Ticker, const PriceAbs& prOpen, volume_t nShares, const PriceRel& slr) {
				T18_ASSERT(prOpen.isSet());
				if (UNLIKELY(slr.getTicker() != prOpen.getTicker())) _throwInvalidRelPrice();
				set(bLong, _Ticker, prOpen, nShares, PriceAbs::from(slr, !bLong, prOpen.getLvl()), PriceAbs());
			}

			OrderData(bool bLong, const tickerBase_t& _Ticker, const PriceAbs& prOpen, volume_t nShares
				, const PriceRel& slr, const PriceRel& tpr)
			{
				T18_ASSERT(prOpen.isSet());
				if (UNLIKELY(slr.getTicker() != prOpen.getTicker() || tpr.getTicker() != prOpen.getTicker())) _throwInvalidRelPrice();
				set(bLong, _Ticker, prOpen, nShares, PriceAbs::from(slr, !bLong, prOpen.getLvl()), PriceAbs::from(tpr, bLong, prOpen.getLvl()));
			}

			OrderData(bool bLong, const tickerBase_t& _Ticker, const PriceAbs& prOpen, volume_t nShares, const PriceAbs& sl, const PriceRel& tpr) {
				T18_ASSERT(prOpen.isSet());
				if (UNLIKELY(tpr.getTicker() != prOpen.getTicker())) _throwInvalidRelPrice();
				set(bLong, _Ticker, prOpen, nShares, sl, PriceAbs::from(tpr, bLong, prOpen.getLvl()));
			}


			bool is_clean()const { return m_pTickr == nullptr; }
			void clean() { m_pTickr = nullptr; }

			/*const tickerBase_t& Ticker()const { T18_ASSERT(!is_clean());			return *m_pTickr; }
			real_t prOpen()const { T18_ASSERT(!is_clean());			return m_prOpen; }
			auto nShares()const { T18_ASSERT(!is_clean());		return m_nShares; }

			real_t prSl()const { T18_ASSERT(!is_clean());			return m_prSl; }
			real_t prTp()const { T18_ASSERT(!is_clean());			return m_prTp; }

			bool bLong()const { T18_ASSERT(!is_clean());			return m_bLong; }*/

			void set(bool bLong, const tickerBase_t& _Ticker, const PriceAbs& prOpen, volume_t nShares
				, const PriceAbs& sl = PriceAbs(), const PriceAbs& tp = PriceAbs(), const TimeFilter& tflt = TimeFilter()) noexcept
			{
				T18_ASSERT(is_clean());
				m_pTickr = &_Ticker;

				T18_ASSERT(prOpen.isSet());
				m_prOpen = prOpen;

				T18_ASSERT(nShares > 0);
				m_nShares = nShares;

				m_timeActive = tflt;

				T18_ASSERT(!sl.isSet() || (bLong ? sl.getLvl() < prOpen.getLvl() : sl.getLvl() > prOpen.getLvl()));
				T18_ASSERT(!tp.isSet() || (bLong ? tp.getLvl() > prOpen.getLvl() : tp.getLvl() < prOpen.getLvl()));
				
				m_prSl = sl;
				m_prTp = tp;
				m_bLong = bLong;
			}

			/*void set(bool bLong, const tickerBase_t& _Ticker, real_t prOpen, volume_t nShares, PriceRel slr) noexcept {
				set(bLong, _Ticker, prOpen, nShares, PriceAbs(prOpen * (bLong ? (1 - slr.rv) : (1 + slr.rv))), PriceAbs(0));
			}

			void set(bool bLong, const tickerBase_t& _Ticker, real_t prOpen, volume_t nShares, PriceRel slr, PriceRel tpr) noexcept {
				set(bLong, _Ticker, prOpen, nShares, PriceAbs(prOpen * (bLong ? (1 - slr.rv) : (1 + slr.rv)))
					, PriceAbs(prOpen * (bLong ? (1 + tpr.rv) : (1 - tpr.rv))));
			}

			void set(bool bLong, const tickerBase_t& _Ticker, real_t prOpen, volume_t nShares, PriceAbs sl, PriceRel tpr) noexcept {
				set(bLong, _Ticker, prOpen, nShares, sl, PriceAbs(prOpen * (bLong ? (1 + tpr.rv) : (1 - tpr.rv))));
			}*/

			void set(OrderData&& od)noexcept {
				T18_ASSERT(!od.is_clean());
				if (LIKELY(this != &od)) {
					*this = ::std::move(od);
				}
			}

			template<typename TrdIntf>
			decltype(auto) makeMarketTrade(TrdIntf& iTrd) const {
				if (UNLIKELY(is_clean())) {
					T18_ASSERT(!"WTF? order is empty, can't emit");
					throw ::std::runtime_error("WTF? order is empty, can't emit");
				}
				T18_ASSERT(m_prOpen.isSet());
				T18_ASSERT(m_timeActive.shouldAllow(m_prOpen.getTicker().getBestBid().Time()));//bid/ask doesn't matter here
				return iTrd.newMarketTrade(m_bLong, *m_pTickr, m_nShares, m_prSl, m_prTp);
			}
		};


		//defines an interface for handling trade orders
		class iExecTrade {
		//protected:
		public:
			virtual ~iExecTrade() {}

			//we shouldn't use trade or tradeEx classes here to make variable trade class inheritance possible
			/*typedef trade trade_t;
			typedef tradeEx tradeEx_t;*/


			typedef tradeId_t tradeId_t;

			typedef tickerBase tickerBase_t;

			virtual void _execTradeOpen(tradeId_t) = 0;
			virtual void _execTradeClose(tradeId_t) = 0;
			//should this func be called, the passed trade must be canceled and totally removed from market
			//no callbacks on associated trade object must be called!
			virtual void _execTradeAbort(tradeId_t) = 0;

			virtual void _execTradeSetStopLoss(tradeId_t) = 0;
			virtual void _execTradeSetTakeProfit(tradeId_t) = 0;

			typedef typename def_call_wrapper_t::template call_tpl<void(mxTimestamp)> openedTradesTimedCB_t;
			//typedef ::std::function<void(const dt_data&)> openedTradesTimedCB_t;
			
			virtual utils::regHandle _execScheduleTimedCallbackIfOpenedTrades(mxTime t, const tickerBase_t* pTickr, openedTradesTimedCB_t&& f) = 0;

			// #todo need a way to cancel the order!
			// #warning stop orders are currently dropped at the end of each day. This is in-line with some broker's behaviour,
			//but may need to be fixed for others
			virtual void _execOrderStop(OrderData&&) = 0;

			virtual size_t _execOrderStopCount() const = 0;

			//#WARNING: StopLoss orders MUST be left intact! This routine must remove ONLY orders set by _execOrderStop()
			virtual void _execOrderStopDropAll() = 0;

			//Ok to return slightly worse estimate than real value (backtester uses the worst last bar values for estimation of opened trades)
			virtual money_t _execGetEquity()const = 0;
			virtual money_t _execPortfolioAmount()const = 0;
		};

	}
}

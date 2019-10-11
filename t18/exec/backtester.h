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

//#include "../feeder/singleFile.h"
#include "tradingInterface.h"
#include "../market/MarketDataStorServ.h"

namespace t18 { namespace exec {

	//this class performs execution/evaluation of trading strategy over a history 
	//#TODO #WARNING At this moment it's possible that a trade that have a stoploss/tp defined over another ticker
	//would be closed with a timestamp of current bar, but using a close price of previous bar, should the stoploss/tp
	// ticker be updated before the trade ticker update. To mitigate that issue we had to aggregate updates in a block,
	// propagate updates to tickers/timeframes and only after that execute stoploss/tp checks. 
	// Similar issues with scheduling new stop orders; they can be opened with a slighly wrong price if the opening price
	// is given using another ticker
	template<typename TickersSetHST>
	class backtester 
		: public tradingInterface
		, public MarketDataStorServ<TickersSetHST>
		, protected iExecTrade
	{
	public:
		typedef backtester<TickersSetHST> self_t;
		T18_TYPEDEFS_SELF()
		
		typedef tradingInterface base_class_trading_intf_t;
		using base_class_trading_intf_t::tickerBase_t;
		using base_class_trading_intf_t::trade_t;
		using base_class_trading_intf_t::tradeEx_t;
		using base_class_trading_intf_t::tradeId_t;


		typedef tickerBase_t::bestPriceInfo_t bestPriceInfo_t;

		typedef MarketDataStorServ<TickersSetHST> base_class_data_stor_t;
		//using typename base_class_data_stor_t::ticker_t;
		//using typename base_class_data_stor_t::tickerServ_t;

		typedef iExecTrade base_exec_trade_t;
		using typename base_exec_trade_t::openedTradesTimedCB_t;

		//static_assert(::std::is_same_v<ticker_t, base_exec_trade_t::ticker_t>, "");

	protected:
		struct openedTradesCBInfo {
			openedTradesTimedCB_t m_CB;
			const tickerBase_t*const m_pTickr;
			const mxTime m_timeToCall;
			mxDate m_curTradingDay;
			mxDate m_lastDateCalled;

			openedTradesCBInfo(openedTradesTimedCB_t&& f, const tickerBase_t* pTickr, mxTime t)
				: m_CB(::std::move(f)), m_pTickr(pTickr), m_timeToCall(t) {}
		};

		typedef ::std::list<openedTradesCBInfo> openedTradesTimedCBStor_t;

		struct ordersStopData {
			::std::vector<OrderData> m_odSet;
			size_t m_total = 0;

			void dropAll()noexcept {
				if (m_total > 0) {
					T18_ASSERT(m_odSet.size() >= m_total);
					for (auto& e : m_odSet) {
						e.clean();
					}
					m_total = 0;
				}
			}

			size_t count()const { return m_total; }

			void drop_idx(size_t i){
				if (UNLIKELY(i >= m_odSet.size() || m_total <= 0)) {
					T18_ASSERT(i < m_odSet.size());
					T18_ASSERT(m_total > 0);
					throw ::std::runtime_error("Invalid index!");
				}
				m_odSet[i].clean();
				--m_total;
			}

			void addNew(OrderData&& od) {
				T18_ASSERT(!od.is_clean());
				if (UNLIKELY(m_total==m_odSet.size())) {
					//creating new slot
					m_odSet.push_back(::std::move(od));
				} else {
					//using existing slot
					T18_ASSERT(m_total < m_odSet.size());
					for (auto&e : m_odSet) {
						if (e.is_clean()) {
							e.set(::std::move(od));
							break;
						}
					}
				}
				++m_total;
			}
		};

	protected:
		openedTradesTimedCBStor_t m_openedTradesTimedCBs;	
		ordersStopData m_ordersStop;

	protected:
		money_t m_curPortfolioAmount = money_t(1e6);
		bool m_bVerboseTrading = true;

		//////////////////////////////////////////////////////////////////////////
		//iExecTrade implementation
		//////////////////////////////////////////////////////////////////////////
		
	private:
		//assert triggering here is a sign that this object might have already been destroyed!
		void _deregister(typename openedTradesTimedCBStor_t::iterator it) {
			T18_ASSERT(m_openedTradesTimedCBs.size());
			m_openedTradesTimedCBs.erase(it);
		}
		template<typename CBStorT>
		auto _makeHandle(CBStorT& stor) {
			return utils::regHandle(::std::bind(static_cast<void(self_t::*)(typename CBStorT::iterator)>(&self_t::_deregister), this, --stor.end()));
		}

	protected:
		friend trade_t;

		virtual void _execTradeOpen(tradeId_t tid) override {
			auto& trd = getTradeEx(tid);
			typedef ::std::decay_t<decltype(trd)> trd_t;

			T18_ASSERT(trd.isOpenedByMarket() || !"non-market trades not supported yet");//just a check
			T18_ASSERT(trd.getState() == TradeState::PendingOpen);
			//consequense of isOpenedByMarket()
			T18_ASSERT((trd.isLong() && trd.plannedOpen() == trd.Ticker().getBestAsk())
				|| (trd.isShort() && trd.plannedOpen() == trd.Ticker().getBestBid()));

			//just marking the trade as opened
			const auto& openedAt = trd.plannedOpen();
			const auto v = trd.plannedVolume();

			trd._cb_dealDone(typename trd_t::deals_t(openedAt.TS(), openedAt.q, v, trd.isLong()));
			T18_ASSERT(TradeState::inMarket == trd.getState());

			//updating portfolio estimate first
			//#TODO: For short trades balance estimate may be invalid
			//see also _execTradeClose() and _execGetEquity()
			m_curPortfolioAmount -= openedAt.q*v*trd.Ticker().getLotSize();

			if(m_bVerboseTrading)STDCOUTL("Enter:" << trd.tradeOpenInfo());
			if (trd.isShort()) {
				STDCOUTL("#TODO: For short trades balance estimate may be invalid");
			}

			base_class_trading_intf_t::_cb_tradeOpenResult(trd, false);
		}
		virtual void _execTradeClose(tradeId_t tid) override {
			auto& trd = getTradeEx(tid);
			typedef ::std::decay_t<decltype(trd)> trd_t;

			T18_ASSERT(trd.getState() == TradeState::PendingClose);
			T18_ASSERT(trd.isClosedByMarket() || !"non-market closing are not supported yet");
			//consequense of isClosedByMarket()
			T18_ASSERT((trd.isLong() && trd.plannedClose() == trd.Ticker().getBestBid())
				|| (trd.isShort() && trd.plannedClose() == trd.Ticker().getBestAsk()));

			//closing
			const auto& closedAt = trd.plannedClose();
			const auto v = trd.volumeInMarket();
			trd._cb_dealDone(typename trd_t::deals_t(closedAt.TS(), closedAt.q, v, !trd.isLong()));
			T18_ASSERT(TradeState::Closed == trd.getState());

			//updating portfolio estimate first
			m_curPortfolioAmount += trd.tradeProfit() + trd.plannedOpen().q*v*trd.Ticker().getLotSize();
			//m_curPortfolioAmount += trd.isLong() ? closedAt.q*v : trd.tsqStart().q*trd.VolShares() + trd.tradeProfit();

			base_class_trading_intf_t::_cb_tradeCloseResult(trd, false);

			if (m_bVerboseTrading)STDCOUTL("Leave:" << trd.tradeCloseInfo() << ", eq=" 
				<< base_class_trading_intf_t::moneyVal2str(_execGetEquity()));
		}
		virtual void _execTradeAbort(tradeId_t tid) override {
			if (m_bVerboseTrading) STDCOUTL("Aborting everything about trade#" << tid);
			auto& trd = getTradeEx(tid);
			base_class_trading_intf_t::_cb_tradeCloseResult(trd, true);
		};

		virtual void _execTradeSetStopLoss(tradeId_t tid) override {
			if (m_bVerboseTrading)STDCOUTL("setting stop loss for trade#" << tid);
		}
		virtual void _execTradeSetTakeProfit(tradeId_t tid) override {
			if (m_bVerboseTrading)STDCOUTL("setting take profit for trade#" << tid);
		}
		virtual utils::regHandle _execScheduleTimedCallbackIfOpenedTrades(mxTime t, const tickerBase_t* pTickr, openedTradesTimedCB_t&& f) override {
			m_openedTradesTimedCBs.emplace_back(::std::move(f), pTickr, t);
			return _makeHandle(m_openedTradesTimedCBs);
		}

		//#todo need a way to cancel the order!
		virtual void _execOrderStop(OrderData&& od)override {
			m_ordersStop.addNew(::std::move(od));
		}

		virtual size_t _execOrderStopCount() const noexcept override {
			return m_ordersStop.count();
		}

		virtual void _execOrderStopDropAll() noexcept override {
			m_ordersStop.dropAll();
		}

		virtual money_t _execGetEquity()const override {
			auto r = m_curPortfolioAmount;
			base_class_trading_intf_t::forEachOpenedTrade([&r](const trade_t& trd)noexcept {
				T18_ASSERT(!trd.is_Closed() && !trd.is_Failed());

				//#TODO: should include comissions here!
				const auto v = trd.volumeInMarket();
				const auto& tickr = trd.Ticker();
				const auto lotSize = tickr.getLotSize();
				if (trd.isLong()) {
					r += v * tickr.getBestBid().q*lotSize;
				} else {
					const auto prOpen = trd.plannedOpen().q;
					r += v *(prOpen * 2 - tickr.getBestAsk().q)*lotSize;
				}
			});
			return r;
		}
		virtual money_t _execPortfolioAmount()const override {
			return m_curPortfolioAmount;
		}

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

	protected:
		static void _setBestGuessBidAsk(tickerBase_t& Tickr, const tsq_data& tsq)noexcept {
			Tickr.setBestBidAsk(bestPriceInfo_t(tsq.TS(), tsq.q, ::std::numeric_limits<volume_t>::epsilon())
				, bestPriceInfo_t(tsq.TS(), tsq.q + Tickr.getMinPriceDelta(), ::std::numeric_limits<volume_t>::epsilon()));
		}
		template<typename BarT>
		static void _setBestGuessBidAsk(tickerBase_t& Tickr, const BarT& bar)noexcept {
			//setting spread to the whole bar span
			const auto tx = bar.TS().next();
			T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
			Tickr.setBestBidAsk(bestPriceInfo_t(tx, bar.l, bar.v)
				, bestPriceInfo_t(tx, bar.h == bar.l ? bar.h + Tickr.getMinPriceDelta() : bar.h, bar.v));
			T18_COMP_POP;
		}

		void _handleNewBarOpen(tickerBase_t& Tickr, const tsq_data& tso) {
			//T18_ASSERT(dto == Tickr.getLastKnownQuote());
			//this assert is wrong, because callback is called before last quote update
			
			//new bar mustn't be added to the base yet as well as any other timeframe object bc of how handling of timed-callbacks work
			T18_ASSERT(Tickr.notFilledYet() || tso.TS() > Tickr.getLastSeenTS());

			bool bBidAskSet = false;

			if (base_class_trading_intf_t::openedTradesCount() > 0) {
				//we should check if we must call to scheduled callback
				if (m_openedTradesTimedCBs.size()) {
					_doScheduledTimedCallbacksIfOpenTrades(Tickr, tso);
				}

				//best guess
				_setBestGuessBidAsk(Tickr, tso);
				bBidAskSet = true;

				base_class_trading_intf_t::forEachOpenedTradeEx([&Tickr, lq = tso](tradeEx_t& t) {
					T18_ASSERT(!t.is_Closed() && !t.is_Failed());

					const bool bLong = t.isLong();
					bool bClosed = false;
					const auto& sl = t.StopLoss();
					if (sl.isSet() && sl.getTicker() == Tickr) {
						const auto lvl = sl.getLvl();
						if ((bLong && lq.q <= lvl) || (!bLong && lq.q >= lvl)) {
							t.closeByMarket(TradeCloseReason::StopLoss);
							bClosed = true;
						}
					}

					if (!bClosed) {
						const auto& tp = t.TakeProfit();
						if (tp.isSet() && tp.getTicker() == Tickr) {
							const auto lvl = tp.getLvl();
							if ((bLong && lq.q >= lvl) || (!bLong && lq.q <= lvl)) {
								//#todo: should take profit be implemented using limit orders?
								t.closeByMarket(TradeCloseReason::TakeProfit);
							}
						}
					}
				});
			}

			//checking whether we should launch some OrderStop
			if (m_ordersStop.count() > 0) {
				//we shall allow to last stop orders for more than a day, bc some brokers allows that.
				if (!bBidAskSet) {
					_setBestGuessBidAsk(Tickr, tso);
				}

				const auto lqv = tso.q;
				size_t i = 0;
				for (auto& od : m_ordersStop.m_odSet) {
					if (!od.is_clean() && Tickr == od.m_prOpen.getTicker() && od.m_timeActive.shouldAllow(tso.Time())) {
						T18_ASSERT(od.m_prOpen.isSet());
						T18_ASSERT(m_ordersStop.count() > 0);
						//do check
						const auto openLvl = od.m_prOpen.getLvl();
						if ((od.m_bLong && lqv > openLvl) || (!od.m_bLong && lqv < openLvl)) {
							//opening a trade and cleaning the od
							od.makeMarketTrade(*this);

							m_ordersStop.drop_idx(i);
							T18_ASSERT(od.is_clean());
							//if (0 == m_ordersStop.count()) break;
						}
					}
					++i;
				}
			}
		}

		template<typename BarT>
		void _handleNewBarClose(tickerBase_t& Tickr, const BarT& bar) {
			//T18_ASSERT(tsq_data::asLastQuote(bar) == Tickr.getLastKnownQuote());
			//this assert is wrong, because callback is called before last quote update

			//new bar mustn't be added to the base as well as any other timeframe object bc of how we handle stop orders (they might
			// be triggered in the middle of a bar)
			T18_ASSERT(Tickr.notFilledYet() || bar.TS() >= Tickr.getLastSeenTS());
			//assuming that anywhere below Tickr.getBaseTf().TotalBars() > 0, just because there shouldn't be any TS that
			//enters market without any real market data available
			
			bool bBidAskSet = false;

			if (base_class_trading_intf_t::openedTradesCount() > 0) {

				//best guess
				_setBestGuessBidAsk(Tickr, bar);
				bBidAskSet = true;

				base_class_trading_intf_t::forEachOpenedTradeEx([&Tickr, &bar](tradeEx_t& t) {
					T18_ASSERT(!t.is_Closed() && !t.is_Failed());

					const auto& sl = t.StopLoss();
					const auto& tp = t.TakeProfit();

					const bool slSuits = sl.isSet() && Tickr == sl.getTicker();
					const bool tpSuits = tp.isSet() && Tickr == tp.getTicker();

					if (slSuits || tpSuits) {
						const bool bLong = t.isLong();
						const real_t slLvl = slSuits ? sl.getLvl() : real_t(0);
						const real_t tpLvl = tpSuits ? tp.getLvl() : real_t(0);

						//#TODO need a better solution for the condition
						if (UNLIKELY(slSuits && tpSuits && slLvl >= bar.l && slLvl <= bar.h && tpLvl >= bar.l && tpLvl <= bar.h))
						{
							T18_ASSERT(!"Cant resolve stoploss and take profit at that bar!");
							throw ::std::runtime_error("Cant resolve stoploss and take profit for bar = "s + bar.TS().to_string());
						}

						bool bClosed = false;
						if (slSuits) {
							T18_ASSERT(!((bLong && bar.o() <= slLvl) || (!bLong && bar.o() >= slLvl)));//must be checked earlier

							if ((bLong && bar.l <= slLvl) || (!bLong && bar.h >= slLvl)) {
								t.closeByMarket(TradeCloseReason::StopLoss);
								bClosed = true;
							}
						}
						if (!bClosed && tpSuits) {
							T18_ASSERT(!((bLong && bar.o() >= tpLvl) || (!bLong && bar.o() <= tpLvl)));//must be checked earlier

							if ((bLong && bar.h >= tpLvl) || (!bLong && bar.l <= tpLvl)) {
								//#todo: should take profit be implemented using limit orders?
								t.closeByMarket(TradeCloseReason::TakeProfit);
							}
						}
					}
				});
			}

			//checking whether we should launch some OrderStop
			if (m_ordersStop.count() > 0) {
				size_t i = 0;
				for (auto& od : m_ordersStop.m_odSet) {
					if (!od.is_clean() && Tickr == od.m_prOpen.getTicker() && od.m_timeActive.shouldAllow(bar.Time())) {
						T18_ASSERT(od.m_prOpen.isSet());
						T18_ASSERT(m_ordersStop.count() > 0);
						const auto openLvl = od.m_prOpen.getLvl();
						const auto bLong = od.m_bLong;
						//do check
						if ((bLong && bar.h > openLvl) || (!bLong && bar.l < openLvl)) {
							T18_ASSERT(openLvl <= bar.h && openLvl >= bar.l);
							//must check, that SL/TP won't be triggered by the same bar, else it's unresolvable
							const auto& sl = od.m_prSl;
							if (UNLIKELY(sl.isSet() && sl.getTicker()==Tickr && ((bLong && sl.getLvl() > bar.l) || (!bLong && sl.getLvl() < bar.h)))) {
								T18_ASSERT(!"Cant resolve SL for the OrderStop");
								throw ::std::runtime_error("Cant resolve SL for the OrderStop at bar "s + bar.TS().to_string());
							}
							const auto& tp = od.m_prTp;
							if (UNLIKELY(tp.isSet() && tp.getTicker()==Tickr && ((bLong && tp.getLvl() < bar.h) || (!bLong && tp.getLvl() > bar.l)))) {
								T18_ASSERT(!"Cant resolve TP for the OrderStop");
								throw ::std::runtime_error("Cant resolve TP for the OrderStop at bar "s + bar.TS().to_string());
							}

							//opening a trade and cleaning the od
							if (!bBidAskSet) {
								//best guess
								_setBestGuessBidAsk(Tickr, bar);
								bBidAskSet = true;
							}
							od.makeMarketTrade(*this);

							m_ordersStop.drop_idx(i);
							T18_ASSERT(od.is_clean());
							//if (0 == m_ordersStop.count()) break;
						}
					}
					++i;
				}
			}
		}

		void _handleNotifyDateTime(tickerBase_t& Tickr, mxTimestamp curTs) {
			if (base_class_trading_intf_t::openedTradesCount() > 0 && m_openedTradesTimedCBs.size()) {
				//we should check if we must call to scheduled callback
				_doScheduledTimedCallbacksIfOpenTrades(Tickr, tsq_data(curTs, Tickr.getLastQuote().q));
			}
		}

		static constexpr decltype(::std::declval<mxTime>().SecondOfDay()) _ScheduleTimedCallbacksIfOpenTrades_maxSecondsNoWarn = 60;

		void _doScheduledTimedCallbacksIfOpenTrades(tickerBase_t& Tickr, const tsq_data& curTsq) {
			T18_ASSERT((base_class_trading_intf_t::openedTradesCount() > 0 && m_openedTradesTimedCBs.size()) || !"WTF!? Mustn't be here!");

			const auto curTsDate = curTsq.Date();
			const auto curTsTime = curTsq.Time();
			const auto& lastTsq = Tickr.getLastQuote();
			const auto lastTsDate = lastTsq.Date();

			for (auto& cbData : m_openedTradesTimedCBs) {
				//we must call the callback cbData IFF it's time AND there's at least one opened trade for ticker, mentioned in cbData
				if (cbData.m_lastDateCalled != curTsDate) { // operator< would be better here, but we would need to change empty()
				//	handling in it, which does not seem like a good idea
					//we must issue a callback at some moment today
					const bool sameTickerAsCbRequire = (!cbData.m_pTickr || *cbData.m_pTickr == Tickr);
					if (
						( (cbData.m_timeToCall <= curTsTime && (cbData.m_curTradingDay == curTsDate || sameTickerAsCbRequire))
							|| (!cbData.m_curTradingDay.empty() && cbData.m_curTradingDay < curTsDate && cbData.m_lastDateCalled!= cbData.m_curTradingDay) )
						&& (!cbData.m_pTickr || base_class_trading_intf_t::anyOpenedTrades4Ticker(*cbData.m_pTickr))
						)
					{
						//either it's time to call and the necessary ticker is trading now or was traded today
						//or we've missed a call yesterday/other day before

						//we must check if we could be too late to call (i.e. difference between scheduled time and current
						//last bar is too huge
						
						bool bWarn;
						const mxTimestamp* pTS;
						if (lastTsDate != curTsDate) {
							T18_ASSERT(lastTsDate < curTsDate);
							pTS = &lastTsq.TS();

							//changing the bid/ask spread to pretend now it is a previous date
							_setBestGuessBidAsk(Tickr, lastTsq);

							//we don't have a good date/time arithmetics at this point so just skip to warning.
							//#todo good check here!
							bWarn = true;
						} else {
							_setBestGuessBidAsk(Tickr, curTsq);
							pTS = &curTsq;
							//#TODO must rely on timestamp from lastQuote here and some predefined threshold. No need to rely on baseTf here
							bWarn = curTsq.SecondOfDay() > lastTsq.SecondOfDay() + _ScheduleTimedCallbacksIfOpenTrades_maxSecondsNoWarn;
						}
						if (bWarn) {
							STDCOUTL("* WARNING! TimedCallbackIfOpenedTrades('" << Tickr.Name()
								<< "'): Last TS = " << lastTsq.TS().to_string() << ", cur TS = " << curTsq.TS().to_string());
						}
						cbData.m_lastDateCalled = pTS->Date();
						//cbData.m_curTradingDay.clear();//why should we clear this mark here?

						//cbData.m_CB(curDtq);
						//it's a backtester, not the real trading, and we may safely pretend that current timestamp is 
						// lastDt instead of curDtq, because every framework's object state now corresponds to that timestamp
						// and for real trading we will use different scheduling approach based on timing/cron

						cbData.m_CB(*pTS);


					}// else {
						//No opened trades for required ticker, or it's too early to call. Just mark cur day to check if we late later
						//we set m_curTradingDay only if the ticker required by the callback is being traded today
						//if(sameTickerAsCbRequire) cbData.m_curTradingDay = curTsDate;
					//}
					//should be updated in any case
					if (sameTickerAsCbRequire) cbData.m_curTradingDay = curTsDate;
				}
			}
		}

	public:
		virtual ~backtester() override {}
		backtester() : base_class_trading_intf_t(this), base_class_data_stor_t() {}

		using base_class_data_stor_t::tickersCount;

		template<typename TsSetupT, typename FeederT>
		void run(TsSetupT&& so, FeederT&& feed) {//&& is just universal refs here
			typedef typename ::std::remove_reference<TsSetupT>::type ts_setup_t;

			//first ensure we're not reusing this object as it'll require some cleanup code I don't want to write now.
			if (tickersCount() > 0 || tradesCount() > 0) {
				T18_ASSERT(!"backtester object reusing is not supported");
				throw ::std::logic_error("backtester object reusing is not supported");
			}

			//performing the setup and obtaining runtime config
			so.setup(*static_cast<base_class_data_stor_t*>(this));

			//some checks
			if (!tickersCount()) {
				T18_ASSERT(!"At least one ticker must be added!");
				throw ::std::logic_error("At least one ticker must be added!");
			}

			//installing ticker event handlers that are needed to track stoploss/takeprofits
			//They'll be fired BEFORE any related TS event handlers fire
			::std::vector<utils::regHandle> hStor;
			hStor.reserve(3 * tickersCount());

			base_class_data_stor_t::forEachTicker([&hStor, ths = this](auto& t) {
				typedef ::std::decay_t<decltype(t)> ticker_t;

				if (t.countOfOnNewBarClose() || t.countOfOnNewBarOpen() || t.countOfOnNotifyDateTime()) {
					//there must be no callbacks registered, as they will run actually before our callbacks, and
					//that will spoil all trade handling.
					T18_ASSERT(!"WTF? Don't register callbacks on Ticker object during TS setup!");
					throw ::std::logic_error("WTF? Don't register callbacks on Ticker object during TS setup!");
				}

				hStor.push_back(t.registerOnNewBarOpen(::std::bind(&self_t::_handleNewBarOpen
					, ths, ::std::ref(t.getTickerBase()), ::std::placeholders::_1)));
				hStor.push_back(t.registerOnNewBarClose(::std::bind(&self_t::_handleNewBarClose<typename ticker_t::bar_t>
					, ths, ::std::ref(t.getTickerBase()), ::std::placeholders::_1)));
				hStor.push_back(t.registerOnNotifyDateTime(::std::bind(&self_t::_handleNotifyDateTime
					, ths, ::std::ref(t.getTickerBase()), ::std::placeholders::_1)));
			});

			//creating TS object in heap as it might be big
			//during construction TS must register on necessary timeframe events to receive updates
			auto pTS = ::std::make_unique<typename ts_setup_t::ts_t>(
				*static_cast<typename base_class_data_stor_t::MarketDataStor_t *>(this)
				, *static_cast<exec::tradingInterface*>(this)
				, so);

			//feeding data into the TS 
			feed(*static_cast<base_class_data_stor_t*>(this));

			//done here. TS object will be destroyed, however, the TsSetupT& so won't
		}

		void verboseTrading(bool b)noexcept { m_bVerboseTrading = b; }
		self_ref_t silence()noexcept { 
			m_bVerboseTrading = false;
			return *this;
		}

		void SetInitDeposit(real_t v) {
			if (UNLIKELY(v<=0)) {
				T18_ASSERT(!"Invalid initial deposit!");
				throw ::std::logic_error("Invalid initial deposit!");
			}
			m_curPortfolioAmount = v;
		}
	};

} }

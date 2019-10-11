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

#include <vector>
#include <list>
#include "trade.h"
#include "../utils/myFile.h"

namespace t18 {

	namespace exec {

		//maintains the list of all trades as well as trade open/close status. Provides trading interface for trade systems
		class tradingInterface {
			typedef tradingInterface self_t;
		public:
			typedef iExecTrade exec_t;
			typedef trade trade_t;
			typedef tickerBase tickerBase_t;

			typedef def_call_wrapper_t call_wrapper_t;
			typedef typename call_wrapper_t::template call_tpl<void(trade_t& trd)> onTradeStatusCB_t;

		protected:
			typedef tradeEx tradeEx_t;
			typedef typename tradeEx_t::tradeId_t tradeId_t;

			typedef ::std::vector<tradeEx_t> tradeList_t;
			typedef ::std::list<tradeId_t> tradeOpenList_t;
			typedef ::std::vector<tradeEx_t*> tradeOpenListHlpr_t;

			//structure to store data necessary for implementation of onTrade*() events
			template<typename CBT>
			struct tradeCBInfo {
				CBT m_CB;
				const tickerBase_t* m_pTickr = nullptr;

				tradeCBInfo(CBT&& f, const tickerBase_t* pT = nullptr) : m_CB(::std::move(f)), m_pTickr(pT) {}
			};

			typedef ::std::list<tradeCBInfo<onTradeStatusCB_t>> onTradeStatusCBInfoStor_t;

		protected:
			tradeList_t m_trades;
			tradeOpenList_t m_OpenedTrades;
			exec_t* m_pTrdExec;

			tradeOpenListHlpr_t m_OpenedTradesHlpr;

			onTradeStatusCBInfoStor_t m_onTradeStatusCBs;

			unsigned m_maxOpenedPositions = ::std::numeric_limits<unsigned>::max();

		public:
			tradingInterface(exec_t* pE) :m_trades(), m_OpenedTrades(), m_pTrdExec(pE) {}

			void setMaxOpenedPositions(unsigned n)noexcept {
				T18_ASSERT(n > 0);
				m_maxOpenedPositions = n;
			}

			//////////////////////////////////////////////////////////////////////////
			//#TODO implement limit orders. Currently only market orders works correctly

			template<class... TradeTArgs>
			trade_t* newMarketTrade(bool bLong, const tickerBase_t& Tickr, TradeTArgs&&... ttargs) {
				if (openedTradesCount() >= m_maxOpenedPositions) return nullptr;

				const auto tid = m_trades.size();
				m_OpenedTrades.push_back(tid);

				//creating tradeEx_t object
				m_trades.emplace_back(m_pTrdExec, tid, trade_t::TSQ4market(bLong ? Tickr.getBestAsk() : Tickr.getBestBid())
					, bLong, Tickr,::std::forward<TradeTArgs>(ttargs)...);
				auto& trd = m_trades.back();

				//passing it to the real execution
				m_pTrdExec->_execTradeOpen(tid);

				//we must not call onStatus callback, it's the job of executor
				//_execOnTradeStatusCBs(trd);
				return &trd;
			}
			template<class... TradeTArgs>
			auto newMarketLong(TradeTArgs&&... ttargs) {
				return newMarketTrade(true, ::std::forward<TradeTArgs>(ttargs)...);
			}
			template<class... TradeTArgs>
			auto newMarketShort(TradeTArgs&&... ttargs) {
				return newMarketTrade(false, ::std::forward<TradeTArgs>(ttargs)...);
			}

			size_t openedTradesCount()const noexcept { return m_OpenedTrades.size(); }
		private:
			void _removeTradeFromOpenedList(const trade_t& trd)noexcept {
				T18_ASSERT(!trd.is_SomethingInMarket());

				auto it = ::std::find(m_OpenedTrades.begin(), m_OpenedTrades.end(), trd.TradeId());
				if (it == m_OpenedTrades.end()) {
					//#todo log!
					T18_ASSERT(!"_removeTradeFromOpenedList: No such trade to close!");
					//throw ::std::runtime_error("_markTradeClosed: No such trade to close!");
					//no point to throw here
				} else {
					m_OpenedTrades.erase(it);
				}
			}

			bool _isTradeInOpenedList(const trade_t& trd)const noexcept {
				auto it = ::std::find(m_OpenedTrades.begin(), m_OpenedTrades.end(), trd.TradeId());
				return it != m_OpenedTrades.end();
			}

		public:
			//////////////////////////////////////////////////////////////////////////

			template<class... OrderDataArgsT>
			void newOrderStop(bool bLong, const tickerBase_t& Tickr, const PriceAbs& prOpen, OrderDataArgsT&&... oargs) {
				if (UNLIKELY(!prOpen.isSet())) {
					T18_ASSERT(!"Invalid prOpen!");
					throw ::std::runtime_error("Invalid prOpen!");
				}

				const auto lq = prOpen.getTicker().getLastQuote().q;
				if (UNLIKELY((bLong && lq >= prOpen.getLvl()) || (!bLong && lq <= prOpen.getLvl()))) {
					T18_ASSERT(!"Invalid price level for stop order!");
					throw ::std::runtime_error("Invalid price level for stop order!");
				}

				return m_pTrdExec->_execOrderStop(OrderData(bLong, Tickr, prOpen, ::std::forward<OrderDataArgsT>(oargs)...));
			}
			template<class... OrderDataArgsT>
			auto newBuyStop(OrderDataArgsT&&... oargs) {
				return newOrderStop(true, ::std::forward<OrderDataArgsT>(oargs)...);
			}
			template<class... OrderDataArgsT>
			auto newSellStop(OrderDataArgsT&&... oargs) {
				return newOrderStop(false, ::std::forward<OrderDataArgsT>(oargs)...);
			}

			size_t orderStopCount()const { return m_pTrdExec->_execOrderStopCount(); }

			void orderStopDropAll() {
				m_pTrdExec->_execOrderStopDropAll();
			}

			//////////////////////////////////////////////////////////////////////////

			money_t GetEquity()const {
				return m_pTrdExec->_execGetEquity();
			}
			money_t GetPortfolioAmount()const {
				return m_pTrdExec->_execPortfolioAmount();
			}

			//////////////////////////////////////////////////////////////////////////

			//will call f(const dt_data& curDT) at around the time t of every trading day, if there are opened trades on ticker pTickr
			// Note: exact callback moment may depend on the base timeframe granularity
			// Note: should a trade be opened after callback time, the callback may fire immediately after! To silence callback
			// for this very day, need to implement distinct API to update cbData.m_lastDateCalled
			template<typename F>
			utils::regHandle scheduleTimedCallbackIfOpenedTrades(tag_milTime, time_ult t, const tickerBase_t* pTickr, F&& f) {
				return m_pTrdExec->_execScheduleTimedCallbackIfOpenedTrades(mxTime(tag_milTime(), t), pTickr
					, exec_t::openedTradesTimedCB_t(::std::forward<F>(f)));
			}
			template<typename F>
			utils::regHandle scheduleTimedCallbackIfOpenedTrades(mxTime t, const tickerBase_t* pTickr, F&& f) {
				return m_pTrdExec->_execScheduleTimedCallbackIfOpenedTrades(t, pTickr, exec_t::openedTradesTimedCB_t(::std::forward<F>(f)));
			}

			//////////////////////////////////////////////////////////////////////////

		protected:
			void _notifyTradeStatusChange(trade_t& trd)const noexcept {
				for (const auto& cbData : m_onTradeStatusCBs) {
					if ((cbData.m_pTickr == nullptr || *cbData.m_pTickr == trd.Ticker())) {
						cbData.m_CB(trd);
					}
				}
			}
		private:
			//assert triggering here is a sign that this object might have already been destroyed!
			void _deregister(typename onTradeStatusCBInfoStor_t::iterator it) {
				T18_ASSERT(m_onTradeStatusCBs.size());
				m_onTradeStatusCBs.erase(it);
			}
			template<typename CBStorT>
			auto _makeHandle(CBStorT& stor) {
				return utils::regHandle(::std::bind(static_cast<void(self_t::*)(typename CBStorT::iterator)>(&self_t::_deregister), this, --stor.end()));
			}

		public:
			decltype(auto) registerOnTradeStatus(const tickerBase_t* pTickr, onTradeStatusCB_t&& f) {
				m_onTradeStatusCBs.emplace_back(::std::move(f), pTickr);
				return _makeHandle(m_onTradeStatusCBs);
			}

			//////////////////////////////////////////////////////////////////////////
		protected:
			//callback from transaction API that confirms the trade was opened. Trade object must already been updated by caller
			//we just broadcasting the results to all who cares
			void _cb_tradeOpenResult(trade_t& trd, bool bFailed) noexcept{
				if (UNLIKELY(bFailed)) {
					T18_ASSERT(!trd.is_SomethingInMarket() && trd.getState() == TradeState::OpenFailed);
					//we must remove the trade from opened list
					_removeTradeFromOpenedList(trd);
				} else {
					T18_ASSERT(trd.is_SomethingInMarket());
					T18_ASSERT(_isTradeInOpenedList(trd));
				}
				_notifyTradeStatusChange(trd);
			}

			void _cb_tradeCloseResult(trade_t& trd, bool bFailed) noexcept{
				if (UNLIKELY(!(trd.is_Closed() || (bFailed && trd.is_Failed())))) {
					T18_ASSERT(!"Trade must be closed or failed here!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Tried to mark trade with invalid state as closed!");
					T18_COMP_POP;
				}
				T18_ASSERT(bFailed || _isTradeInOpenedList(trd));
				
				if (_isTradeInOpenedList(trd)) _removeTradeFromOpenedList(trd);

				_notifyTradeStatusChange(trd);
			}

			tradeEx_t& getTradeEx(size_t tid) {
				if (tid >= m_trades.size()) {
					T18_ASSERT(!"Invalid trade id specified!");
					throw ::std::runtime_error("Invalid trade id specified!");
				}
				auto& t = m_trades[tid];
				T18_ASSERT(t.TradeId() == tid);
				return t;
			}

		public:
		
			trade_t& getTrade(size_t tid) { return getTradeEx(tid); }

			const trade_t& getTrade(size_t tid)const {
				if (tid >= m_trades.size()) {
					T18_ASSERT(!"Invalid trade id specified!");
					throw ::std::runtime_error("Invalid trade id specified!");
				}
				const auto& t = m_trades[tid];
				T18_ASSERT(t.TradeId() == tid);
				return t;
			}

			size_t tradesCount()const noexcept { return m_trades.size(); }

			/////////////////////////////////////////////////////////////////////////////////
		protected:
			//during execution of _any_ forEach*() routine it is safe to close a trade, but NOT safe to open new trade

			template<typename TradeT, typename F>
			void _forEachTradeT(F&& f) {
				for (TradeT& t : m_trades) {
					::std::forward<F>(f)(t);
				}
			}
			template<typename TradeT, typename F>
			void _forEachTradeT(F&& f) const {
				for (const TradeT& t : m_trades) {
					::std::forward<F>(f)(t);
				}
			}

			template<typename TradeT, typename F>
			void _forEachTradeT(const tickerBase_t& Tickr, F&& f) {
				for (TradeT& t : m_trades) {
					if (Tickr == t.Ticker()) {
						::std::forward<F>(f)(t);
					}
				}
			}
			template<typename TradeT, typename F>
			void _forEachTradeT(const tickerBase_t& Tickr, F&& f) const{
				for (const TradeT& t : m_trades) {
					if (Tickr == t.Ticker()) {
						::std::forward<F>(f)(t);
					}
				}
			}


			template<typename TradeT, typename F>
			void _execOpenedTradesHlpr(F&& f) {
				for (auto t : m_OpenedTradesHlpr) {
					::std::forward<F>(f)(*static_cast<TradeT*>(t));
				}
				m_OpenedTradesHlpr.clear();
			}

			template<typename TradeT, typename F>
			void _forEachOpenedTradeT(F&& f) {
				//m_OpenedTrades container might be changed as a result of applying f, so we have to workaround it
				const auto otc = m_OpenedTrades.size();
				if (otc) {
					T18_ASSERT(!m_OpenedTradesHlpr.size());
					if (m_OpenedTradesHlpr.capacity() < otc) m_OpenedTradesHlpr.reserve(otc);

					for (auto tid : m_OpenedTrades) {
						auto& t = getTradeEx(tid);
						T18_ASSERT(!t.is_Closed() && !t.is_Failed());
						m_OpenedTradesHlpr.push_back(&t);
					}

					if (m_OpenedTradesHlpr.size()) {
						_execOpenedTradesHlpr<TradeT>(::std::forward<F>(f));
					}
				}
			}

			template<typename TradeT, typename F>
			void _forEachOpenedTradeT(const tickerBase_t& Tickr, F&& f) {
				//m_OpenedTrades container might be changed as a result of applying f, so we have to workaround it
				const auto otc = m_OpenedTrades.size();
				if (otc) {
					T18_ASSERT(!m_OpenedTradesHlpr.size());
					if (m_OpenedTradesHlpr.capacity() < otc) m_OpenedTradesHlpr.reserve(otc);

					for (auto tid : m_OpenedTrades) {
						auto& t = getTradeEx(tid);
						T18_ASSERT(!t.is_Closed() && !t.is_Failed());
						if (Tickr == t.Ticker()) {
							m_OpenedTradesHlpr.push_back(&t);
						}
					}

					if (m_OpenedTradesHlpr.size()) {
						_execOpenedTradesHlpr<TradeT>(::std::forward<F>(f));
					}
				}
			}

			template<typename F>
			void forEachTradeEx(F&& f) {
				_forEachTradeT<tradeEx_t>(::std::forward<F>(f));
			}
			template<typename F>
			void forEachTradeEx(F&& f) const {
				_forEachTradeT<tradeEx_t>(::std::forward<F>(f));
			}

			template<typename F>
			void forEachTradeEx(const tickerBase_t& Tickr, F&& f) {
				_forEachTradeT<tradeEx_t>(Tickr, ::std::forward<F>(f));
			}
			template<typename F>
			void forEachTradeEx(const tickerBase_t& Tickr, F&& f) const {
				_forEachTradeT<tradeEx_t>(Tickr, ::std::forward<F>(f));
			}

			template<typename F>
			void forEachOpenedTradeEx(F&& f) {
				_forEachOpenedTradeT<tradeEx_t>(::std::forward<F>(f));
			}

			template<typename F>
			void forEachOpenedTradeEx(const tickerBase_t& Tickr, F&& f) {
				_forEachOpenedTradeT<tradeEx_t>(Tickr, ::std::forward<F>(f));
			}

		public:

			template<typename F>
			void forEachTrade(F&& f) {
				_forEachTradeT<trade_t>(::std::forward<F>(f));
			}
			template<typename F>
			void forEachTrade(F&& f) const {
				_forEachTradeT<trade_t>(::std::forward<F>(f));
			}

			template<typename F>
			void forEachTrade(const tickerBase_t& Tickr, F&& f) {
				_forEachTradeT<trade_t>(Tickr, ::std::forward<F>(f));
			}
			template<typename F>
			void forEachTrade(const tickerBase_t& Tickr, F&& f) const {
				_forEachTradeT<trade_t>(Tickr, ::std::forward<F>(f));
			}

		
			template<typename F>
			void forEachOpenedTrade(F&& f) {
				_forEachOpenedTradeT<trade_t>(::std::forward<F>(f));
			}

			template<typename F>
			void forEachOpenedTrade(F&& f) const {
				for (auto tid : m_OpenedTrades) {
					const auto& t = getTrade(tid);
					T18_ASSERT(!t.is_Closed() && !t.is_Failed());
					::std::forward<F>(f)(t);
				}
			}

			template<typename F>
			void forEachOpenedTrade(const tickerBase_t& Tickr, F&& f) {
				_forEachOpenedTradeT<trade_t>(Tickr, ::std::forward<F>(f));
			}

			template<typename F>
			void forEachOpenedTrade(const tickerBase_t* pTickr, F&& f) {
				if (pTickr) {
					forEachOpenedTrade(*pTickr, ::std::forward<F>(f));
				} else {
					forEachOpenedTrade(::std::forward<F>(f));
				}
			}
		
			template<typename F>
			void forEachOpenedTrade(const tickerBase_t& Tickr, F&& f) const {
				for (auto tid : m_OpenedTrades) {
					const auto& t = getTrade(tid);
					T18_ASSERT(!t.is_Closed() && !t.is_Failed());
					if (Tickr == t.Ticker()) {
						::std::forward<F>(f)(t);
					}
				}
			}
			template<typename F>
			void forEachOpenedTrade(const tickerBase_t* pTickr, F&& f)const {
				if (pTickr) {
					forEachOpenedTrade(*pTickr, ::std::forward<F>(f));
				} else {
					forEachOpenedTrade(::std::forward<F>(f));
				}
			}

			bool anyOpenedTrades4Ticker(const tickerBase_t& Tickr)const noexcept {
				for (const auto tid : m_OpenedTrades) {
					const auto& t = getTrade(tid);
					T18_ASSERT(!t.is_Closed() && !t.is_Failed());
					if (Tickr == t.Ticker()) return true;
				}
				return false;
				/*return ::std::any_of(m_OpenedTrades.cbegin(), m_OpenedTrades.cend(), [&Tickr](const auto tid) {
					const auto& t = getTrade(tid);
					T18_ASSERT(!t.isClosed());
					return Tickr == t.Ticker();
				});*/
			}

			///////////////////////////////////////////////////////////////////////////
		public:
			//for backtesting use only!
			void bt_exportTradeList_AmibrokerCsv(const char* pszCsvFile)const {
				static const char*const pszAmiCsvHeader = "Symbol,Trade,Date,Price,Ex. date,Ex. Price,% chg,Profit,% Profit,Shares,Position value,Cum. Profit,# bars,Profit/bar,MAE,MFE,Scale In/Out\r\n";

				utils::myFile myF(pszCsvFile, "wb");
				fwrite(pszAmiCsvHeader, sizeof(*pszAmiCsvHeader), strlen(pszAmiCsvHeader), myF);

				real_t cumProf = 0;
				forEachTrade([&myF, &cumProf](const auto& t) {
					if (UNLIKELY(t.is_Failed())) {
						T18_ASSERT(!"bt_exportTradeList_AmibrokerCsv is for backtesting only and can't handle failed trades!");
						throw ::std::logic_error("bt_exportTradeList_AmibrokerCsv is for backtesting only and can't handle failed trades!");
					}

					const auto& tickr = t.Ticker();
					const tsq_data& lq = tickr.getLastQuote();
					const int prec = tickr.getPrecision();
					T18_ASSERT(prec >= 0 && prec <= 10);

					const tsq_data& tS = t.plannedOpen();
					const tsq_data& tE = t.is_Closed() ? t.plannedClose() : lq;
					const bool bLong = t.isLong();
					const real_t prfPerShare = bLong ? tE.q - tS.q : tS.q - tE.q;
					const money_t profit = prfPerShare*t.plannedVolume();
					const real_t percChg = prfPerShare*real_t(100) / tS.q;
					const auto teSec = tE.Second();

					cumProf += profit;

					fprintf_s(myF, "%s,%s,%02d.%02d.%04d %02d:%02d:%02d,%.*f,%02d.%02d.%04d %02d:%02d:%02d,%.*f"
						",%.6f%%,%.*f,%.6f%%,%.6f,%.6f,%.6f"
						",%d,%.*f,%.*f%%,%.*f%%,%s"
						"\r\n"
						, tickr.Name(), bLong ? "Long" : "Short"
						, tS.Day(), tS.Month(), tS.Year(), tS.Hour(), tS.Minute(), tS.Second()
						, prec, tS.q
						, tE.Day(), tE.Month(), tE.Year(), tE.Hour(), tE.Minute(), teSec > 0 ? teSec - 1 : 0 //we adjust closing bar time one second in future, so reverting it
						, prec, tE.q
						, percChg, prec, profit, percChg*t.plannedVolume(), t.plannedVolume(), t.plannedVolume()*tS.q, cumProf
						, 0 //#TODO number of bars!
						, prec, realNan_v  //prfPerShare/real_t(0) //#TODO number of bars!
						, prec, t.MAE()*real_t(100), prec, t.MFE()*real_t(100), "0/0"
					);
				});
			}

			//for backtesting use only!
			void bt_exportTradeList_LinesFmt(const tickerBase_t& Tickr, const char* pszCsvFile)const {
				utils::myFile myF(pszCsvFile, "wb");
				fprintf_s(myF, "%d\r\n", tradesCount());

				//general format is:
				//start datenum, start timenum, end datenum, end timenum, start price, end price, lineClass (int)

				forEachTrade(Tickr, [&myF](const auto& t) {
					if (UNLIKELY(t.is_Failed())) {
						T18_ASSERT(!"bt_exportTradeList_AmibrokerCsv is for backtesting only and can't handle failed trades!");
						throw ::std::logic_error("bt_exportTradeList_AmibrokerCsv is for backtesting only and can't handle failed trades!");
					}

					const auto& tickr = t.Ticker();
					const tsq_data& lq = tickr.getLastQuote();
					const int prec = tickr.getPrecision();
					T18_ASSERT(prec >= 0 && prec <= 10);
					const tsq_data& tS = t.plannedOpen();
					const tsq_data& tE = t.is_Closed() ? t.plannedClose() : lq;
					const bool bLong = t.isLong();
					const real_t prf = bLong ? tE.q - tS.q : tS.q - tE.q;

					fprintf_s(myF, "%u,%u,%u,%u,%.*f,%.*f,%d\r\n"
						,tS.Date().to_datenum(), tS.Time().to_timenum(), tE.Date().to_datenum(), tE.Time().to_timenum()
						, prec, tS.q
						, prec, tE.q
						, 2 * (bLong ? 0 : 1) + int(prf > 0)
					);
				});
			}
			void bt_exportTradeList_LinesFmt(const tickerBase_t& Tickr, const ::std::string& sCsvFile)const {
				bt_exportTradeList_LinesFmt(Tickr, sCsvFile.c_str());
			}

			static const char* moneyVal2str(money_t v)noexcept {
				static char buf[32];
				sprintf_s(buf, "%.2f", v);
				return buf;
			}

		};

}

}


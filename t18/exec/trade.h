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

#include <forward_list>

#include "../base.h"
#include "iExecTrade.h"
#include "../market/ticker.h"
#include "_tradeEnums.h"

namespace t18 {
	
	/////////////////////////////////////////////////////////////////////
	// Object of class trade is created to represent an ACTUALLY occurred trade. Therefore is MUST NOT be created
	// for trades that are only scheduled to happen iff some conditions are met (such as limit or stop-orders, that
	// won't be executed immediately)
	class trade {
	private:
		typedef trade self_t;
		typedef exec::iExecTrade exec_t;

	public:
		typedef typename exec_t::tickerBase_t tickerBase_t;
		typedef typename exec_t::tradeId_t tradeId_t;

		static constexpr tradeId_t invalid_tid = ::std::numeric_limits<tradeId_t>::max();
		//invalid_tid is used somewhere in a framework to mark some data as non existent.

		//data type to describe deals that happened during trade opening/closing. Long means the volume has been bought, short == sold
		typedef tsDirTick deals_t;

	protected:
		typedef tsTick priceAndVol_t;

		typedef ::std::forward_list<deals_t> dealsList_t;

	protected:
		//to compare floating point volume
		//_volZeroThreshold should be big enought to capture all rounding errors, but much smaller than smallest possible trade size
		static constexpr volume_t _volZeroThreshold = volume_t(1e-6);
		static bool isVolEqual(volume_t v1, volume_t v2)noexcept {
			return ::std::abs(v1 - v2) < _volZeroThreshold;
		}

		typedef ::std::uint32_t flags_t;
		static constexpr flags_t _bit_isLong = 1;
		static constexpr flags_t _bit_openByMarket = (_bit_isLong << 1);
		static constexpr flags_t _bit_closeByMarket = (_bit_openByMarket << 1);

		static constexpr flags_t _trade_lastUsedBit = _bit_closeByMarket;
		static_assert(_trade_lastUsedBit <= (1u << (::std::numeric_limits<flags_t>::digits - 1)), "");

		template<flags_t flg> void _flagSet()noexcept { m_flags |= flg; }
		template<flags_t flg> void _flagClear()noexcept { m_flags &= (~flg); }
		template<flags_t flgSet, flags_t flgClr> void _flagSetClear()noexcept { m_flags = (m_flags & (~flgClr)) | flgSet; }
		template<flags_t flg>  bool _isFlagSet()const noexcept { return (m_flags & flg); }

	protected:
		PriceAbs m_StopLoss;
		PriceAbs m_TakeProfit;

		exec_t* m_pExec{ nullptr };
		//const size_t m_tickerId;
		const tickerBase_t& m_Ticker;

		const tradeId_t m_tid;//unique trade id

		volume_t m_volInMarket{volume_t(0)}; //really bought/sold on the market - that's the value to be used to close the trade

		const priceAndVol_t m_tsqPlannedOpen;//requested price (for market orders that's the best price possible) and volume for trade
		tsq_data m_tsqPlannedClose; //helpful to estimate slippage and timing
		
		dealsList_t m_dealsOpen, m_dealsClose;//real TSQV of the trade, i.e. timestamp and a level of market entry/exit.
		
		//real_t m_prMFE = realNan_v, m_prMAE = realNan_v;//#TODO! just a placeholders now, calc later. Or don't calc at all ?
		
		flags_t m_flags{ 0 };

		TradeState m_State{ TradeState::_NotConstructed };
		TradeCloseReason m_CloseReason{ TradeCloseReason::_UNDEFINED };
		TradeFailureCode m_failureCode{ TradeFailureCode::_NoFailure };

	public:
		//two special function to distinguish really invalid tso from special case used to open market trades
		static constexpr tsq_data TSQ4market(const tsq_data& tsq) noexcept{
			//_ts should be the current datetime
			if (tsq.invalid()) {
				T18_ASSERT(!"WTF?");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid tsq!");
				T18_COMP_POP;
			}
			return tsq_data(tsq.TS(), -tsq.q);
		}

	private:
		//to make sure the trade object is always passed by reference
		//trade(const trade&) = delete;

		//////////////////////////////////////////////////////////////////////////
	public:
		//////////////////////////////////////////////////////////////////////////
		// getter methods
		//////////////////////////////////////////////////////////////////////////
		// can be called anytime		
		bool isOpenedByMarket()const noexcept { return _isFlagSet<_bit_openByMarket>(); }

		tradeId_t TradeId()const noexcept{ return m_tid; }
		const tickerBase_t& Ticker()const noexcept{ return m_Ticker; }
		auto getTickerId()const noexcept{ return m_Ticker.getTickerId(); }

		bool isLong()const { return _isFlagSet<_bit_isLong>(); }
		bool isShort()const { return !_isFlagSet<_bit_isLong>(); }

		const auto& StopLoss()const noexcept { return m_StopLoss; }
		const auto& TakeProfit()const noexcept { return m_TakeProfit; }

		const tsq_data& plannedOpen()const noexcept { return m_tsqPlannedOpen.TSQ(); }
		auto plannedVolume()const noexcept { return m_tsqPlannedOpen.v; }

		auto MFE()const noexcept { return realNan_v; }// m_prMFE / m_tsqOpen.q; }
		auto MAE()const noexcept { return realNan_v; }// m_prMAE / m_tsqOpen.q; }

		auto getFailureCode()const noexcept { return m_failureCode; }
		bool is_Failed()const noexcept { return m_failureCode != TradeFailureCode::_NoFailure; }

		//don't make decisions based on state, use appropriate helper functions
		auto getState()const noexcept { return m_State; }

		//returns true IFF the trade is in some terminal state (either closed or failed) and nothing could/should be done with it
		bool is_TradeFinished()const noexcept {
			const bool ret = m_pExec == nullptr;
			T18_ASSERT(ret || (m_State != TradeState::Closed && m_failureCode == TradeFailureCode::_NoFailure));
			return ret;
		}

		//true for any state possible since after PendingOpen finished (i.e. at least variables related to trade opening are valid)
		bool is_AfterOpenRequest()const noexcept {
			const auto s = m_State;
			return TradeState::_NotConstructed != s && TradeState::PendingOpen != s;
		}

		bool is_DuringPositionChange()const noexcept {
			const auto s = m_State;
			return !is_TradeFinished() && (TradeState::PendingOpen == s || TradeState::Opening == s || TradeState::PendingClose == s || TradeState::Closing == s);
		}
		bool is_DuringOpening()const noexcept {
			const auto s = m_State;
			return !is_TradeFinished() && (TradeState::PendingOpen == s || TradeState::Opening == s);
		}
		bool is_DuringClosing()const noexcept {
			const auto s = m_State;
			return !is_TradeFinished() && (TradeState::PendingClose == s || TradeState::Closing == s);
		}

		//returns if the trade in any *normal* possible state after it was opened ==== if it can be closed
		bool is_SomethingInMarket()const noexcept { 
			const auto s = m_State;
			const bool ret = !is_TradeFinished() && (TradeState::inMarket == s || TradeState::Opening == s || TradeState::Closing == s);
			T18_ASSERT(!ret || m_volInMarket > 0);
			//note that we're not using m_volInMarket here as a criterion, because should a fail happen, the trade object must preserve its state
			return ret;
		}
		//whether .close() method was called on the object (i.e. whether plannedClose is valid)
		bool is_AfterCloseRequest()const noexcept {
			const auto s = m_State;
			return TradeState::PendingClose == s || TradeState::Closing == s || TradeState::Closed == s || TradeState::CloseFailed == s;
		}

		bool is_Closed()const noexcept {
			return TradeState::Closed == m_State;
		}
		

		//////////////////////////////////////////////////////////////////////////
		//the following functions may be called IFF the trade is ok and not scheduled for closing
		void verifyState_OK_andNOT_AfterCloseRequest()const noexcept {
			if (UNLIKELY(is_TradeFinished() || is_AfterCloseRequest())) {
				T18_ASSERT(!"Invalid state is_inMarket for this call");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid state is_inMarket for this call = "s + describe(m_State));
				T18_COMP_POP;
			}
			T18_ASSERT(TradeState::OpenFailed != m_State);//should be caught earlier by the is_TradeFinished()
		}
		void setStopLoss(const PriceAbs& sla) {
			verifyState_OK_andNOT_AfterCloseRequest();
			if (sla.isSet()) {
				//testing relationship to cur market.
				const real_t v = sla.getLvl();
				const auto& t = sla.getTicker();
				if ((isLong() && t.getBestBid().q <= v) || (!isLong() && t.getBestAsk().q >= v)) {
					T18_ASSERT(!"Invalid stop loss level specified!");
					throw ::std::runtime_error("Invalid stop loss level specified!");
				}
			}
			m_StopLoss = sla;
			m_pExec->_execTradeSetStopLoss(TradeId());
		}
		void updateStopLossLvl(real_t v) {
			if (!m_StopLoss.isValid()) {
				T18_ASSERT(!"SL object to update doesn't have ticker set!");
				throw ::std::runtime_error("SL object to update doesn't have ticker set!");
			}
			return setStopLoss(PriceAbs(m_StopLoss.getTicker(), v));
		}
		void setTakeProfit(const PriceAbs& tp) {
			verifyState_OK_andNOT_AfterCloseRequest();
			if (tp.isSet()) {
				//testing relationship to cur market.
				const real_t v = tp.getLvl();
				const auto& t = tp.getTicker();
				if ((isLong() && t.getBestBid().q >= v) || (!isLong() && t.getBestAsk().q <= v)) {
					T18_ASSERT(!"Invalid takeprofit level specified!");
					throw ::std::runtime_error("Invalid takeprofit level specified!");
				}
			}
			m_TakeProfit = tp;
			m_pExec->_execTradeSetTakeProfit(TradeId());
		}
		void updateTakeProfitLvl(real_t v) {
			if (!m_TakeProfit.isValid()) {
				T18_ASSERT(!"TP object to update doesn't have ticker set!");
				throw ::std::runtime_error("TP object to update doesn't have ticker set!");
			}
			return setTakeProfit(PriceAbs(m_TakeProfit.getTicker(), v));
		}

		//////////////////////////////////////////////////////////////////////////
		//the following functions may be called IFF is_AfterOpened()
		void verifyStateIs_AfterOpenRequest()const noexcept {
			if (UNLIKELY(!is_AfterOpenRequest())) {
				T18_ASSERT(!"Not valid is_AfterOpenRequest state for this call");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Not valid is_AfterOpenRequest state for this call, state = "s + describe(m_State));
				T18_COMP_POP;
			}
		}
		/*const auto& tsqOpen()const noexcept {
			verifyStateIs_AfterOpened();
			T18_ASSERT(m_tsqOpen.valid());
			return m_tsqOpen;
		}*/
		auto volumeInMarket()const noexcept {
			verifyStateIs_AfterOpenRequest();
			//T18_ASSERT(m_volInMarket > 0);
			return m_volInMarket;
		}

		//////////////////////////////////////////////////////////////////////////
		//the following functions may be called IFF is_AfterCloseRequest()
		void verifyStateIs_AfterCloseRequest()const noexcept {
			if (UNLIKELY(!is_AfterCloseRequest())) {
				T18_ASSERT(!"Invalid state is_AfterCloseRequest for this call");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid state is_AfterCloseRequest for this call = "s + describe(m_State));
				T18_COMP_POP;
			}
		}

		bool isClosedByMarket()const noexcept { 
			verifyStateIs_AfterCloseRequest();
			return _isFlagSet<_bit_closeByMarket>();
		}
		auto getCloseReason()const noexcept {
			verifyStateIs_AfterCloseRequest();
			T18_ASSERT(TradeCloseReason::_UNDEFINED != m_CloseReason);
			return m_CloseReason;
		}
		const auto& plannedClose()const noexcept {
			verifyStateIs_AfterCloseRequest();
			T18_ASSERT(m_tsqPlannedClose.valid());
			return m_tsqPlannedClose;
		}

		//////////////////////////////////////////////////////////////////////////
		//on successful close
		void verifyState_OK_and_Closed()const noexcept {
			if (UNLIKELY(is_Failed() || !is_Closed())) {
				T18_ASSERT(!"Invalid state is_Closed for this call");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid state is_Closed for this call = "s + describe(m_State));
				T18_COMP_POP;
			}
		}

		/*const tsq_data& tsqClose()const noexcept {
			verifyStateIs_Closed();
			T18_ASSERT(m_tsqClose.valid());
			return m_tsqClose;
		}*/

		money_t tradeProfit()const {
			verifyState_OK_and_Closed();

			money_t opnMoney{ 0 }, clsMoney{ 0 };

			//#TODO: should include comissions here
			for (const auto& e : m_dealsOpen) opnMoney += static_cast<money_t>(e.q) * e.volume();
			for (const auto& e : m_dealsClose) clsMoney += static_cast<money_t>(e.q) * e.volume();

			return (clsMoney - opnMoney)*static_cast<money_t>(2 * isLong() - 1)*m_Ticker.getLotSize();
		}

		//////////////////////////////////////////////////////////////////////////
	protected:
		// A trade SHOULD be created with a real current timestamp, therefore, knowing less than a caller, we
		// should allow him to pass the real timestamp instead of using lastKnownQuote's timestamp here (which may be late even
		// during backtesting (if the trading decision was made using another ticker).
		// So, trdOpen SHOULD be tsq_data, not the plain real_t
		trade(exec_t* pE, tradeId_t tid, const tsq_data& trdOpen, const bool bLong, const tickerBase_t& _Ticker, volume_t nShares
			, const PriceAbs& sl = PriceAbs(), const PriceAbs& tp = PriceAbs()) noexcept
			: m_StopLoss(sl), m_TakeProfit(tp), m_pExec(pE), m_Ticker(_Ticker), m_tid(tid)
			, m_tsqPlannedOpen(trdOpen.TS(), ::std::abs(trdOpen.q), nShares)
		{
			T18_ASSERT(tid != invalid_tid);//should never happen, but let it be
			
			if (bLong) _flagSet<_bit_isLong>();
			if (trdOpen.q < 0) _flagSet<_bit_openByMarket>();

			if (UNLIKELY(!pE || m_tsqPlannedOpen.invalid())) {
				T18_ASSERT(!"invalid trade open data!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("invalid trade open data!");
				T18_COMP_POP;
			}

			if (isOpenedByMarket()) {
				T18_ASSERT(m_tsqPlannedOpen.TSQ() == (bLong ? m_Ticker.getBestAsk() : m_Ticker.getBestBid()));
				//nothing to here yet
			} else {
				//we can't make any check on m_tsqPlannedOpen.q (probably except to check whether it fits to max/min price, or not)

				T18_ASSERT(!"Non-market orders are NOT yet implemented");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Non-market orders are NOT yet implemented");
				T18_COMP_POP;
			}

			const auto prOpen = m_tsqPlannedOpen.q;
			if (UNLIKELY(m_StopLoss.isSet() && ((bLong && prOpen <= m_StopLoss.getLvl()) || (!bLong && prOpen >= m_StopLoss.getLvl())))) {
				T18_ASSERT(!"Invalid stoploss value!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid stoploss value!");
				T18_COMP_POP;
			}
			if (UNLIKELY(m_TakeProfit.isSet() && ((bLong && prOpen >= m_TakeProfit.getLvl()) || (!bLong && prOpen <= m_TakeProfit.getLvl())))) {
				T18_ASSERT(!"Invalid takeprofit value!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid takeprofit value!");
				T18_COMP_POP;
			}


			if (UNLIKELY(m_StopLoss.isSet() && m_TakeProfit.isSet() 
				&& ((bLong && m_TakeProfit.getLvl() <= m_StopLoss.getLvl())
					|| (!bLong && m_TakeProfit.getLvl() >= m_StopLoss.getLvl()))))
			{
				T18_ASSERT(!"Invalid stoploss+takeprofit!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid stoploss+takeprofit!");
				T18_COMP_POP;
			}
			
			m_State = TradeState::PendingOpen;

			//must NOT call this function because superclass might have not finished their initialization/construction.
			// it's CALLER responsibility to pass this object to execution
			//m_pExec->_execTradeOpen(*this);
		}

		trade(exec_t* pE, tradeId_t tid, const tsq_data& trdOpen, bool bLong, const tickerBase_t& _Ticker, volume_t nShares
			, const PriceRel& slr) noexcept
			: trade(pE, tid, trdOpen, bLong, _Ticker, nShares, PriceAbs::from(slr, !bLong, ::std::abs(trdOpen.q)), PriceAbs())
		{}

		trade(exec_t* pE, tradeId_t tid, const tsq_data& trdOpen, bool bLong, const tickerBase_t& _Ticker, volume_t nShares
			, const PriceRel& slr, const PriceRel& tpr)noexcept
			: trade(pE, tid, trdOpen, bLong, _Ticker, nShares
				, PriceAbs::from(slr, !bLong, ::std::abs(trdOpen.q)), PriceAbs::from(tpr, bLong, ::std::abs(trdOpen.q)))
		{}

		trade(exec_t* pE, tradeId_t tid, const tsq_data& trdOpen, bool bLong, const tickerBase_t& _Ticker, volume_t nShares
			, const PriceAbs& sl, const PriceRel& tpr)noexcept
			: trade(pE, tid, trdOpen, bLong, _Ticker, nShares, sl, PriceAbs::from(tpr, bLong, ::std::abs(trdOpen.q)))
		{}

	protected:
		void _failed(TradeFailureCode fc)noexcept {
			T18_ASSERT(TradeFailureCode::_NoFailure != fc);
			//#todo log!
			STDCOUTL("Trade "<< m_tid << " failed with code " << describe(fc));

			if (m_failureCode != TradeFailureCode::_NoFailure) {
				STDCOUTL("## previous fail code = " << describe(m_failureCode) << " preserved");
			}else m_failureCode = fc;

			if (TradeCloseReason::_UNDEFINED == m_CloseReason) m_CloseReason = TradeCloseReason::ABORTED;

			//executing trade abortion if possible
			if (m_pExec) {
				m_pExec->_execTradeAbort(m_tid);
				m_pExec = nullptr;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//callbacks
		//_cb_dealDone() get called by execution engine to pass information that some part of the trade was processed by an exchange/market.
		// #Note that if the trade was failed/aborted earlier, the callback handler can't issue new abortion command. Therefore,
		// execution handle MUST check whether the trade was aborted/failed BEFORE calling the callbacks. If it found the trade was
		// aborted, it must close all pending orders and exit market
		void _cb_dealDone(const deals_t& deal) noexcept{
			if (UNLIKELY(deal.invalid() || deal.vol() <= _volZeroThreshold)) {
				T18_ASSERT(!"WTF! Must never be here!");
				//dont throw, just return ignoring the data
				return;
			}

			TradeFailureCode newFC = TradeFailureCode::_NoFailure;

			//expected to be called during trade opening or closing phase
			const bool bOpening = is_DuringOpening();
			const bool bClosing = is_DuringClosing();
			if (LIKELY((bOpening || bClosing) && !is_Failed())) {
				const bool thisLong = isLong();
				//appending deal to the list
				if (bOpening) {
					m_dealsOpen.insert_after(m_dealsOpen.before_begin(), deal);
					m_State = TradeState::Opening;
				} else {
					T18_ASSERT(bClosing);//sanity check should the base IF be changed
					m_dealsClose.insert_after(m_dealsClose.before_begin(), deal);
					m_State = TradeState::Closing;
				}
				if (UNLIKELY(bClosing ^ (thisLong ^ deal.isLong()))) {
					T18_ASSERT(!"WTF!? Got invalid deal direction!");
					newFC = bOpening ? TradeFailureCode::invalidDealDirectionDuringOpening : TradeFailureCode::invalidDealDirectionDuringClosing;
				}

				if (LIKELY(TradeFailureCode::_NoFailure == newFC)) {
					//examining content and change the state appropriately
					const volume_t v = deal.vol() * (2 * deal.isLong() - 1);
					m_volInMarket += v;

					if (isVolEqual(m_volInMarket, volume_t(0))) {
						m_volInMarket = volume_t(0);
						if (bClosing) {
							m_State = TradeState::Closed;
							m_pExec = nullptr;
						} else newFC = TradeFailureCode::invalidVolumeOnOpening;
					} else if (isVolEqual(m_volInMarket, plannedVolume())) {
						m_volInMarket = plannedVolume();
						if (bOpening) {
							m_State = TradeState::inMarket;
						} else newFC = TradeFailureCode::invalidVolumeOnClosing;
					} else if (m_volInMarket < 0) {
						newFC = TradeFailureCode::totalVolumeLessZero;
					} else if (m_volInMarket > plannedVolume()) {
						newFC = TradeFailureCode::exceededVolume;
					}
				}
			} else {
				//#todo log!
				T18_ASSERT(!"invalid trade state");
				//won't throw, because here we don't want the system to die
				newFC = TradeFailureCode::unexpectedDeal;
			}
			if (TradeFailureCode::_NoFailure != newFC) _failed(newFC);
		}
		//should any deal related to this trade fail, executor MUST call this cb
		void _cb_dealFailed() {
			TradeFailureCode newFC = TradeFailureCode::_NoFailure;

			const bool bOpening = is_DuringOpening();
			const bool bClosing = is_DuringClosing();
			if (LIKELY(bOpening || bClosing)) {
				m_State = bOpening ? TradeState::OpenFailed : TradeState::CloseFailed;
				newFC = TradeFailureCode::dealFailed;
			} else {
				//#todo log!
				T18_ASSERT(!"invalid trade state");
				//won't throw, because here we don't want the system to die
				newFC = TradeFailureCode::unexpectedDeal;
			}
			T18_ASSERT(TradeFailureCode::_NoFailure != newFC);
			_failed(newFC);
		}

	public:
		void closeByMarket(TradeCloseReason cr /*= TradeCloseReason::Normal*/) noexcept {
			if (cr == TradeCloseReason::_UNDEFINED) {
				T18_ASSERT(!"Invalid close reason");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::logic_error("Invalid close reason");
				T18_COMP_POP;
			}

			if (!is_SomethingInMarket()) {
				T18_ASSERT(!"Invalid state!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Trade can't be closed with state = "s + describe(m_State));
				T18_COMP_POP;
			}

			T18_ASSERT(m_tsqPlannedOpen.valid() && m_volInMarket > 0);
			T18_ASSERT(m_tsqPlannedClose.invalid());

			m_tsqPlannedClose = isLong() ? m_Ticker.getBestBid() : m_Ticker.getBestAsk();
			m_CloseReason = cr;
			m_State = TradeState::PendingClose;
			_flagSet<_bit_closeByMarket>();

			m_pExec->_execTradeClose(TradeId());
		}
		
		//////////////////////////////////////////////////////////////////////////

		/*::std::string describe()const {
			return is_Failed() ? describeFailed() : describeValid();
		}

		::std::string describeValid()const {
			if (UNLIKELY(is_Failed())) {
				T18_ASSERT(!"mustn't call this func!");
				throw ::std::logic_error("mustn't call this func!");
			}

			char _buf[1024];
			//long/short tid state
			sprintf(_buf, "%s%d ")
		}

		::std::string describeFailed()const {
			if (UNLIKELY(!is_Failed())) {
				T18_ASSERT(!"mustn't call this func!");
				throw ::std::logic_error("mustn't call this func!");
			}
		}*/


		//#todo optimize it!
		::std::string tradeOpenInfo()const {
			if (UNLIKELY(is_Failed())) {
				T18_ASSERT(!"Mustn't call the function!");
				return "tradeOpenInfo - TradeFailed! Code TODO!"s;
			}
			return (isLong() ? "L"s : "S"s) + ::std::to_string(m_tid) + " " + describe(m_State) + " "
				+ m_tsqPlannedOpen.to_string() 
				/*+ " shares=" + ::std::to_string(m_volShares)
				+ (m_StopLoss.isSet() ? " SL="s + ::std::to_string(m_StopLoss.getLvl()) 
					+ (m_StopLoss.getTicker() != m_Ticker ? "@"s + m_StopLoss.getTicker().Name() : ""s) : ""s)
				+ (m_TakeProfit.isSet() ? " TP="s + ::std::to_string(m_TakeProfit.getLvl())
					+ (m_TakeProfit.getTicker() != m_Ticker ? "@"s + m_TakeProfit.getTicker().Name() : ""s) : ""s)*/;
		}

		::std::string tradeCloseInfo()const {
			if (UNLIKELY(is_Failed())) {
				T18_ASSERT(!"Mustn't call the function!");
				return "tradeCloseInfo - TradeFailed! Code TODO!"s;
			}
			if (UNLIKELY(!is_TradeFinished())) {
				T18_ASSERT(!"Mustn't call the function!");
				return "tradeCloseInfo - trade NOT finished yet! Code TODO!"s;
			}
			T18_ASSERT(is_Closed());

			return (isLong() ? "L"s : "S"s) + ::std::to_string(m_tid) + " " + m_tsqPlannedClose.to_string()
				+ " reason:" + describe(m_CloseReason)
				+ ", result=" + ::std::to_string(tradeProfit());
		}

	};

	//tradeEx just provides a write API for some internals/protected parts of trade class
	class tradeEx : public trade {
	private:
		typedef tradeEx self_t;
		typedef trade base_class_t;

	public:
		template<class... Args>
		tradeEx(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

		using base_class_t::_cb_dealDone;
		using base_class_t::_cb_dealFailed;
	};

}

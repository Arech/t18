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

#include "ticker.h"
#include "TickerUpdatesHandler.h"

namespace t18 {

	//provides interface to update data objects
	// #WARNING The order of callback execution for timeframes is UNDEFINED, because hana::map offer no guarantees on folding order
	// isBacktesting is a special parameter that should be set only for (surprise!) backtesting.
	//	it updates current bid/ask data using incoming quotes and changes how quotes with incoherent time are handled
	//	(ignored with logging for real work and asserts/thrown for backtesting)
	template<bool isBacktesting, typename hdmTimeframesT>
	class tickerServer 
		: public ticker<hdmTimeframesT>
		, public TickerUpdatesHandler
	{
	private:
		typedef ticker<hdmTimeframesT> base_class_t;
		typedef TickerUpdatesHandler base_class_updh_t;
		typedef tickerServer<isBacktesting,hdmTimeframesT> self_t;

	public:
		using typename base_class_t::hdmTimeframes_t;
		using typename base_class_t::bar_t;
		
		using base_class_t::getLastSeenTS;
		using base_class_t::_updateLastQuote;

		using base_class_t::setBestBidAsk;
		using base_class_t::getMinPriceDelta;

		using base_class_t::setTf;
		using base_class_t::createTf;
		using base_class_t::forEachTF;
		
	public:

		template<class... Args>
		tickerServer(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

		bool operator==(const self_t& r)const noexcept { return this == &r; }
		bool operator!=(const self_t& r)const noexcept { return this != &r; }

	public:

		void setEndOfSession(mxTime t) {
			if (t.empty()) {
				T18_ASSERT(!"Invalid session time!");
				throw ::std::runtime_error("Invalid session time!");
			}
			if (!this->m_endOfSession.empty()) {//just for bogus case. probably should be removed
				T18_ASSERT(!"Session time had already been set!");
				throw ::std::runtime_error("Session time had already been set!");
			}
			this->m_endOfSession = t;
		}
		
		void setPrecision(int p) {
			if (p < 0 || p>8) {
				T18_ASSERT(!"WTF? Wrong precision");
				throw ::std::runtime_error("WTF? Wrong precision");
			}
			this->m_precision = p;
		}

		void setLotSize(unsigned ls) {
			if (ls<=0) {
				T18_ASSERT(!"WTF? Wrong long size");
				throw ::std::runtime_error("WTF? long size");
			}
			this->m_lotSize = ls;
		}

	private:
		template<bool b = isBacktesting, typename = ::std::enable_if_t<b>>
		void _setBestGuessBidAsk(const tsq_data& tso)noexcept {
			setBestBidAsk(typename base_class_t::bestPriceInfo_t(tso.TS(), tso.q, ::std::numeric_limits<volume_t>::epsilon())
				, typename base_class_t::bestPriceInfo_t(tso.TS(), tso.q + getMinPriceDelta(), ::std::numeric_limits<volume_t>::epsilon()));
		}

	public:
		// #todo make the next functions invisible to trading systems

		//////////////////////////////////////////////////////////////////////////
		// Note that callbacks on ticker object runs BEFORE the update of lastQuote and before the data is passed to timeframe objects.
		// These callbacks are for internal use only
		//////////////////////////////////////////////////////////////////////////
		void _notifyDateTime(mxTimestamp ts)noexcept {
			//just check and forward to baseTF
			if (UNLIKELY(getLastSeenTS() > ts)) {//must be strict here
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at _notifyDateTime!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at _notifyDateTime!");
					T18_COMP_POP;
				}else{
					//#todo write log!
					STDCOUTL("Got timestamp from the past at _notifyDateTime");
					return;
				}
			}
			base_class_updh_t::_onNotifyDateTime(ts);

			forEachTF([ts](auto& tf)noexcept {
				tf._notifyDateTime(ts);
			});
		}
		void _notifyDateTime_post(mxTimestamp ts)noexcept {
			forEachTF([ts](auto& tf)noexcept {
				tf._notifyDateTime_post(ts);
			});
		}

		void _newBarOpen_pre(mxTimestamp ts)noexcept {
			if (UNLIKELY(getLastSeenTS() >= ts)) {//must be non strict here
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at _newBarOpen_pre!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at _newBarOpen_pre!");
					T18_COMP_POP;
				} else {
					//#todo write log!
					STDCOUTL("Got timestamp from the past at _newBarOpen_pre");
					return;
				}
			}
			forEachTF([ts](auto& tf)noexcept {
				tf._newBarOpen_pre(ts);
			});
		}

		// #todo must use some type, taken from the bar_t:: here.
		void _newBarOpen(const tsq_data& tso) noexcept {
			//updating last quote and forwarding
			if (UNLIKELY(getLastSeenTS() >= tso.TS())) {//must be non strict to catch bar open with same time
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at _newBarOpen!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at _newBarOpen!");
					T18_COMP_POP;
				} else {
					//#todo write log!
					STDCOUTL("Got timestamp from the past at _newBarOpen");
					return;
				}
			}
			base_class_updh_t::_onNewBarOpen(tso);
			_updateLastQuote(tso);

			if constexpr(isBacktesting)_setBestGuessBidAsk(tso);

			forEachTF([&tso](auto& tf)noexcept {
				tf._newBarOpen(tso);
			});
		}
		void _newBarOpen_post() noexcept {
			forEachTF([](auto& tf) noexcept {
				tf._newBarOpen_post();
			});
		}

		/*void _newBarClose(const bar_t& bar) {
			//updating last quote and forwarding
			if (!m_LastKnownQuote.TS().empty() && m_LastKnownQuote.TS() > bar.TS()) { //must be strict, as bars with same DT is ok here
				T18_ASSERT(!"Got timestamp from the past at newBarClose!");
				throw ::std::runtime_error("Got timestamp from the past at newBarClose!");
			}
			base_class_updh_t::_onNewBarClose(bar);
			//m_LastKnownQuote.q = bar.c;
			m_LastKnownQuote.lastQuote(bar);
			this->_updateLastSeenBarTS(bar.TS());

			//m_pBaseTf->_newBarClose(bar);
			forEachTF([&bar](auto& tf) {
				tf._newBarClose(bar);
			});
		}
		void _newBarClose_post() {
			//m_pBaseTf->_newBarClose_post();
			forEachTF([](auto& tf) {
				tf._newBarClose_post();
			});
		}*/

		void _newBarAggregate(const bar_t& bar)noexcept {
			//updating last quote and forwarding
			if (UNLIKELY(getLastSeenTS() > bar.TS())) { //must be strict, as bars with same DT is ok here
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at newBarAggregate!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at newBarAggregate!");
					T18_COMP_POP;
				} else {
					//#todo write log!
					STDCOUTL("Got timestamp from the past at newBarAggregate");
					return;
				}
			}
			base_class_updh_t::_onNewBarClose(bar);

			const auto lq{ tsq_data::asLastQuote(bar) };
			_updateLastQuote(lq);
			if constexpr(isBacktesting)_setBestGuessBidAsk(lq);

			forEachTF([&bar](auto& tf) noexcept {
				tf._newBarAggregate(bar);
			});
		}

		//////////////////////////////////////////////////////////////////////////

		void _newTick_pre(mxTimestamp ts)noexcept {
			if (UNLIKELY(getLastSeenTS() > ts)) {//must be strict here, as ticks with the same time are normal
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at _newTick_pre!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at _newTick_pre!");
					T18_COMP_POP;
				} else {
					//#todo write log!
					STDCOUTL("Got timestamp from the past at _newTick_pre");
					return;
				}
			}
			forEachTF([ts](auto& tf)noexcept {
				tf._newTick_pre(ts);
			});
		}
		void _newTick(const tsTick& tst)noexcept {
			//updating last quote and forwarding
			if (UNLIKELY(getLastSeenTS() > tst.TS())) {//must be strict as ticks with same time entirely normal
				if constexpr(isBacktesting) {
					T18_ASSERT(!"Got timestamp from the past at _newTick!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error("Got timestamp from the past at _newTick!");
					T18_COMP_POP;
				} else {
					//#todo write log!
					STDCOUTL("Got timestamp from the past at _newTick");
					return;
				}
			}

			const auto& tso = tst.TSQ();
			base_class_updh_t::_onNewBarOpen( tso );//essentially onNewTick(), as tick's volume doesn't contain any necessary info
			// for ticker subscribers
			_updateLastQuote(tso);
			if constexpr(isBacktesting)_setBestGuessBidAsk(tso);

			forEachTF([&tst](auto& tf)noexcept {
				tf._newTick(tst);
			});
		}
		void _newTick_post() noexcept {
			forEachTF([](auto& tf) noexcept {
				tf._newTick_post();
			});
		}
	};

}

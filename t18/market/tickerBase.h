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

#include "../base.h"
#include "TickerId.h"

namespace t18 {
	using namespace ::std::literals;

	//////////////////////////////////////////////////////////////////////////
	// base for ticker class; gathers all common ticker data in a template-less place to be able it to pass everywhere easily.
	class tickerBase {
	private:
		typedef tickerBase self_t;

	public:
		typedef tag_Ticker_t tag_t;

		//#/todo probably this type should be defined from bar_t::  --- no, it should not.
		// We need a notion of lastQuote (timestamp + last price) and a notion of lastDeal, which is
		// derived from lastQuote and may have also a deal volume and some other variable stuff (such as 
		// an open interest value for futures). So lastQuote doesn't depend on any parametrization of ticker
		// and also must be moved to tickerBase
		//typedef tsq_data lastKnownQuote_t;

		typedef BestPriceInfo bestPriceInfo_t;

	protected:
		//lastKnownQuote_t m_LastKnownQuote;
		bestPriceInfo_t m_bestBid, m_bestAsk;

		// m_Name and m_tickerId must be unique among tickers are used to address particular ticker object 
		// m_tickerId is just better suited for coding, m_Name - for human.
		//const ::std::string m_Name;
		const char*const m_Name;
		const TickerId m_tickerId;

		// It's the latest quote seen by ticker.
		// Used to ensure correct quoting order and for proper ScheduledTimedCallbacksIfOpenTrades() timing
		tsq_data m_lastSeenQuote;

		//aka TickSize. MUST be set on construction
		real_t m_minPriceDelta;

		mxTime m_endOfSession;
		int m_precision{ 6 };//count of decimal digits in price data. Used for exporting data

		//must be set before use
		unsigned m_lotSize{ 0 };

	protected:
		//not for direct instantiation

		//__tickerId is never passed by user. it's defined by framework
		tickerBase(TickerId __tickerId, const char* name, real_t minPriceD) : m_Name(name), m_tickerId(__tickerId)
			, m_lastSeenQuote(mxTimestamp(tag_mxTimestamp()), ::std::numeric_limits<real_t>::epsilon())
			, m_minPriceDelta(minPriceD)
		{
			if (!name) {
				T18_ASSERT(!"name must be set!");
				throw ::std::logic_error("name must be set!");
			}
			if (minPriceD <= 0) {
				T18_ASSERT(!"Invalid minPriceDelta passed!");
				throw ::std::logic_error("Invalid minPriceDelta passed!");
			}
		}

		template<typename T, typename = ::std::enable_if_t<::std::is_same_v<tsq_data, ::std::decay_t<T>>>>
		void _updateLastQuote(T&& tsq)noexcept {
			T18_ASSERT(tsq.TS() > m_lastSeenQuote.TS());//must be release-checked earlier
			m_lastSeenQuote = ::std::forward<T>(tsq);
		}

	public:

		const auto& getLastSeenTS()const noexcept { return m_lastSeenQuote.TS(); }
		const auto& getLastQuote()const noexcept { return m_lastSeenQuote; }

		const auto& getBestBid()const noexcept { return m_bestBid; }
		const auto& getBestAsk()const noexcept { return m_bestAsk; }
		
		template<typename TB, typename TA, typename
			= ::std::enable_if_t<::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TB>>
			&& ::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TA>>>
		>
		void setBestBidAsk(TB&& bid, TA&& ask)noexcept {
			//generally, the data passed here must already been tested for correctness multiple times, so we don't need a 
			//release run-time inconsistence handler here. Debug only assert would be fine
			T18_ASSERT(bid.valid() && ask.valid());
			T18_ASSERT(m_bestBid.TS().empty() || (m_bestBid.TS() <= bid.TS() && m_bestBid.TS() <= ask.TS()));
			T18_ASSERT(m_bestAsk.TS().empty() || (m_bestAsk.TS() <= bid.TS() && m_bestAsk.TS() <= ask.TS()));
			// non strict inequality here because the function may be called to update intra bar quote

			if ( (ask.q - bid.q) + ::std::max(ask.q, bid.q) * (::std::numeric_limits<::std::decay_t<decltype(bid.q)>>::epsilon() * 2)
				< m_minPriceDelta)
			{
				T18_ASSERT(!"Invalid bid/ask price relation!");
				//#TODO write log!
				//bid.q = ask.q - m_minPriceDelta;
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("Invalid bid/ask price relation!");
				T18_COMP_POP;
			}

			m_bestBid = ::std::forward<TB>(bid);
			m_bestAsk = ::std::forward<TA>(ask);
		}

		bool notFilledYet()const noexcept { return getBestBid().TS().empty()/* || getBestAsk().TS().empty()*/; }
		bool seenAtLeastOneQuote()const noexcept { return !notFilledYet(); }

		//we definitely need a way to compare for equality of two ticker objects
		constexpr bool operator==(const self_t& r)const noexcept { return this == &r; }
		constexpr bool operator!=(const self_t& r)const noexcept { return this != &r; }

		const char* Name()const noexcept { return m_Name; }
		auto getTickerId()const noexcept { return m_tickerId; }

		auto getMinPriceDelta()const noexcept {
			T18_ASSERT(m_minPriceDelta > 0 || !"minPriceDelta not set!");
			return m_minPriceDelta;
		}

		mxTime getEndOfSession()const {
			if (m_endOfSession.empty()) {
				T18_ASSERT(!"session time not set!");
				throw ::std::runtime_error("session time not set!");
			}
			return m_endOfSession;
		}

		int getPrecision()const noexcept { return m_precision; }

		volume_t getLotSize()const noexcept {
			T18_ASSERT(m_lotSize > 0 || !"Lot size not set!");
			return static_cast<volume_t>(m_lotSize);
		}
	};

}

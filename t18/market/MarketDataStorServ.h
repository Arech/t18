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

#include "../tags.h"
#include "MarketDataStor.h"

namespace t18 {

	template<typename TickersSetHST>
	class MarketDataStorServ : public MarketDataStor<TickersSetHST>{
	private:
		typedef MarketDataStor<TickersSetHST> base_class_t;
		typedef MarketDataStorServ<TickersSetHST> self_t;

	public:
		typedef base_class_t MarketDataStor_t;
		typedef tag_MarketDataStorServ_t tag_t;

		typedef tickerBase::bestPriceInfo_t bestPriceInfo_t;

		using typename base_class_t::TickersTuple_t;

	protected:
		using base_class_t::m_TickerStor;

		T18_DEBUG_ONLY(mxTimestamp m_latestTS);

	public:

		template<class... Args>
		MarketDataStorServ(Args&&... a) : base_class_t(::std::forward<Args>(a)...)
		{
			T18_DEBUG_ONLY(m_latestTS = mxTimestamp(1901, 1, 1, 1, 1, 1));
		}


		//never pass tickerId as Args, it won't work, just omit it
		template<typename TickerT, class... Args>
		auto& newTicker(Args&&... a) {
			constexpr auto tIdx = utils::index_of_existing<TickersTuple_t>(hana::type_c<TickerT>);
			auto& cont = hana::at(m_TickerStor, tIdx);
			TickerId tiid(tIdx, cont.size());

			cont.emplace_back(::std::make_unique<TickerT>(tiid, ::std::forward<Args>(a)...));
			return *cont[tiid.getIdx()];
		}

		//////////////////////////////////////////////////////////////////////////
	public:
		//interface to update ticker data
		//at first glance it seems that notifyDateTime should broadcast the dt to every ticker.
		//However it's possible that due to a network delay some ticker will receive an update later that might be treated
		// as an "update from the past". So it's better to require a ticker here too
		void notifyDateTime(TickerId tiid, mxTimestamp ts) {
			auto l = [ts, ths = this](auto& Tickr) {
				ths->notifyDateTime(Tickr, ts);
			};
			if (UNLIKELY(tiid.isEveryone())) {
				this->forEachTicker(l);
			} else {
				this->template exec4Ticker<false>(tiid, l);
			}
		}
		
		template<typename TickerT, typename = ::std::enable_if<::std::is_same_v<tag_Ticker_t, typename TickerT::tag_t>>>
		void notifyDateTime(TickerT& Tickr, mxTimestamp ts) {
			T18_ASSERT(m_latestTS <= ts);
			T18_DEBUG_ONLY(m_latestTS = ts);

			Tickr._notifyDateTime(ts);
			Tickr._notifyDateTime_post(ts);
		}

		//////////////////////////////////////////////////////////////////////////
		void newBarOpen(TickerId tiid, const tsq_data& tso) { 
			this->exec4Ticker<false>(tiid, [&tso, ths = this](auto& Tickr) {
				ths->newBarOpen(Tickr, tso);
			});
		}
		template<typename TickerT, typename = ::std::enable_if<::std::is_same_v<tag_Ticker_t, typename TickerT::tag_t>>>
		void newBarOpen(TickerT& Tickr, const tsq_data& tso) {
			T18_ASSERT(m_latestTS <= tso.TS());
			T18_DEBUG_ONLY(m_latestTS = tso.TS());

			Tickr._newBarOpen_pre(tso);
			Tickr._newBarOpen(tso);
			Tickr._newBarOpen_post();
		}

		//////////////////////////////////////////////////////////////////////////
		template<typename BarT>
		void newBarAggregate(TickerId tiid, const BarT& bar) {
			this->exec4Ticker<false>(tiid, [&bar, ths = this](auto& Tickr) {
				ths->newBarAggregate(Tickr, bar);
			});
		}
		template<typename TickerT, typename = ::std::enable_if<::std::is_same_v<tag_Ticker_t, typename TickerT::tag_t>>>
		void newBarAggregate(TickerT& Tickr, typename TickerT::bar_t const& bar) {
			T18_ASSERT(m_latestTS <= bar.TS());
			T18_DEBUG_ONLY(m_latestTS = bar.TS());

			Tickr._newBarAggregate(bar);
		}
		//////////////////////////////////////////////////////////////////////////
		void newTick(TickerId tiid, const tsTick& tst) {
			this->exec4Ticker<false>(tiid, [&tst, ths = this](auto& Tickr) {
				ths->newTick(Tickr, tst);
			});
		}
		template<typename TickerT, typename = ::std::enable_if<::std::is_same_v<tag_Ticker_t, typename TickerT::tag_t>>>
		void newTick(TickerT& Tickr, const tsTick& tst) {
			T18_ASSERT(m_latestTS <= tst.TS());
			T18_DEBUG_ONLY(m_latestTS = tst.TS());

			Tickr._newTick_pre(tst);
			Tickr._newTick(tst);
			Tickr._newTick_post();
		}

		//////////////////////////////////////////////////////////////////////////
		
		template<typename TB, typename TA, typename
			= ::std::enable_if_t<::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TB>>
			&& ::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TA>>>
		>
		void setBestBidAsk(TickerId tiid, TB&& bid, TA&& ask) {
			this->exec4Ticker<false>(tiid, [ths = this, &b=bid, &a=ask](auto& Tickr) {
				ths->setBestBidAsk(Tickr, ::std::forward<TB>(b), ::std::forward<TA>(a));
			});
		}
		
		template<typename TickerT, typename TB, typename TA, typename = ::std::enable_if<
			::std::is_same_v<tag_Ticker_t, typename TickerT::tag_t> && ::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TB>>
			&& ::std::is_same_v<bestPriceInfo_t, ::std::decay_t<TA>>
		>>
		void setBestBidAsk(TickerT& Tickr, TB&& bid, TA&& ask) {
			//generally, bid/ask prices comes from another source than tick/ohlc flow, so we probably shouldn't rely on their time
			//T18_ASSERT(m_latestTS <= earliest(bid,ask));
			//T18_DEBUG_ONLY(m_latestTS = v.TS());

			Tickr.setBestBidAsk(::std::forward<TB>(bid), ::std::forward<TA>(ask));
		}

		///////////////////////////////////////////////////////////////////////////////
		/*template<typename F>
		void forEachTickerServ(F&& f) {
			for (auto& t : m_TickerStor) {
				T18_ASSERT(t);
				::std::forward<F>(f)(*t.get());
			}
		}
		template<typename F>
		void forEachTickerServ(F&& f) const{
			for (const auto& t : m_TickerStor) {
				T18_ASSERT(t);
				::std::forward<F>(f)(*t.get());
			}
		}*/
	};

}

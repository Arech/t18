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

#include "updatesHandler.h"
#include "TimestampStor.h"
#include "../tfConverter/tfConvBase.h"
#include "../timefilter.h"

namespace t18 { namespace timeseries {
	namespace hana = ::boost::hana;
	using namespace hana::literals;
	using namespace ::std::literals;

	template<typename BarT> // = tsohlcv>
	struct dummyTf {
		typedef BarT bar_t;

		static void _notifyDateTime(mxTimestamp)noexcept {}
		static void _notifyDateTime_post(mxTimestamp)noexcept {}

		static void _newBarOpen_pre(mxTimestamp)noexcept {}
		// #todo must use some type, taken from the bar_t:: here.
		static void _newBarOpen(const tsq_data&) noexcept {}
		static void _newBarOpen_post() noexcept {}

		static void _newBarAggregate(const bar_t&) noexcept {}
		static void _newBarAggregate_post() noexcept {}
	};

	//////////////////////////////////////////////////////////////////////////
	//#todo we probably need a special handling for the case when we don't need to store any data in timeframe,
	//(for example, when we need to use M1 callbacks, filtered by time, but don't need M1 historical data itself)
	template<typename BarT> // = tsohlcv>
	class timeframeStor 
		: public TimestampStor<typename BarT::metaDescr_t>
		, public TFUpdatesHandler
	{
	public:
		typedef BarT bar_t;

		typedef typename bar_t::metaDescr_t metaDataDescr_t;
		static constexpr auto metaDataKeys = hana::keys(metaDataDescr_t());
		static_assert(hana::any_of(metaDataKeys, utils::is_hana_string<open_ht>), "There must be a key for open");
		static_assert(hana::any_of(metaDataKeys, utils::is_hana_string<high_ht>), "There must be a key for high");
		static_assert(hana::any_of(metaDataKeys, utils::is_hana_string<low_ht>), "There must be a key for low");
		static_assert(hana::any_of(metaDataKeys, utils::is_hana_string<close_ht>), "There must be a key for close");
		static_assert(hana::any_of(metaDataKeys, utils::is_hana_string<volume_ht>), "There must be a key for vol");

	private:
		typedef timeseries::TimestampStor<metaDataDescr_t> base_class_t;
		typedef timeseries::TFUpdatesHandler base_class_updH_t;

	protected:
		using updateDataMap_t = utils::dataMapFromDescrMap_t<typename bar_t::metaDescrUpdate_t>;

	protected:
		//::std::shared_ptr<const TimeFilter> m_pTimeFltr;
		TimeFilter m_timeFltr;

		const bar_t* m_pCurBar = nullptr;

		//this var helps to keep track of _newBarOpen/Close sequence calls. If you're going to use newTick*() functions
		// (and it's better to use them over newBar*() functions family), m_openCloseState is just a debugging measure, which
		// is quite redundant for release build.
		T18_DEBUG_ONLY(int m_openCloseState = 0);

		T18_DEBUG_ONLY(mxTimestamp m_lastTimeFilterTime);
		bool m_lastTimeFilterReject = false;

		//bool m_curBarJustClosed = false;

	protected:
		//should only serve as a base class
		timeframeStor(size_t barsHistory) : base_class_t(barsHistory), base_class_updH_t() {
			T18_DEBUG_ONLY(m_lastTimeFilterTime = mxTimestamp(1901,1,1,1,1,1));
		}

	public:

		//void setTimeFilter(::std::shared_ptr<TimeFilter>& pTFlt)noexcept {
		void updateTimeFilter(const TimeFilter& TFlt)noexcept {
			m_timeFltr = TFlt;
		}
		const auto& getTimeFilter()const noexcept { return m_timeFltr; }

		///////////////////////////////////////////////////////////////////
		using base_class_t::timestamp;
		using base_class_t::lastTimestamp;
		using base_class_t::get;

		auto open(size_t N)const noexcept { return get(open_ht(), N); }
		auto high(size_t N)const noexcept { return get(high_ht(), N); }
		auto low(size_t N)const noexcept { return get(low_ht(), N); }
		auto close(size_t N)const noexcept { return get(close_ht(), N); }
		auto vol(size_t N)const noexcept { return get(volume_ht(), N); }

		auto lastOpen()const noexcept { return open(0); }
		auto lastHigh()const noexcept { return high(0); }
		auto lastLow()const noexcept { return low(0); }
		auto lastClose()const noexcept { return close(0); }
		auto lastVol()const noexcept { return vol(0); }

		//////////////////////////////////////////////////////////////////////////
		// #TODO generalize to use bar_t the following functions

		::std::string bar2string(size_t N)const {
			//this shouldn't be hard to do, but have not time for it now
			static_assert(::std::is_same_v<bar_t, tsohlcv>, "Not implemented yet");
			return tsohlcv::bar2string(timestamp(N), open(N), high(N), low(N), close(N), vol(N));
		}

		tsohlcv bar(size_t N) const noexcept {
			//this shouldn't be hard to do, but have not time for it now
			static_assert(::std::is_same_v<bar_t, tsohlcv>, "Not implemented yet");
			return tsohlcv(timestamp(N), open(N), high(N), low(N), close(N), vol(N));
		}

		bar_t lastBar() const noexcept { return bar(0); }
		
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////// 

	protected:
		void _updateLastBar(updateDataMap_t&& v) noexcept {
			hana::for_each(v, [&contMap = base_class_t::m_ContMap](auto&& x) noexcept {
				contMap[hana::first(x)][0] = ::std::move(hana::second(x));
			});
		}
		void _updateLastBar(const updateDataMap_t& v) noexcept {
			hana::for_each(v, [&contMap = base_class_t::m_ContMap](const auto& x) noexcept {
				contMap[hana::first(x)][0] = hana::second(x);
			});
		}

		//////////////////////////////////////////////////////////////////////////

		void _verifyWasOpenedAndClose() noexcept {
			if (UNLIKELY(1 != m_openCloseState)) {
				T18_ASSERT(!"expecting _newBarOpen() was called here");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("WTF?! expecting _newBarOpen() was called here.");
				T18_COMP_POP;
			}
			--m_openCloseState;
		}
		void _verifyWasClosed()const noexcept {
			if (UNLIKELY(0 != m_openCloseState)) {
				T18_ASSERT(!"expecting cur bar closed here");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("WTF?! expecting cur bar closed here.");
				T18_COMP_POP;
			}
		}

		//All error/sanity checking code must reside in a caller

		//checks that date and time are most recent
		//#TODO need a better handler for inconsistent data. It should not throw at all!
		void _checkTS(mxTimestamp ts, const char* src, const bool bAllowSameDt) const noexcept {
			if (UNLIKELY(ts.empty())) {
				T18_ASSERT(!"WTF? timestamp is empty!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error(::std::string(src) + ": WTF? timestamp is empty!");
				T18_COMP_POP;
			}
			if (LIKELY(base_class_t::size() > 0)) {
				auto lts = lastTimestamp();
				if (UNLIKELY((bAllowSameDt && ts < lts) || (!bAllowSameDt && ts <= lts))) {
					T18_ASSERT(!"Received a timestamp from the past!");
					T18_COMP_SILENCE_THROWING_NOEXCEPT;
					throw ::std::runtime_error(::std::string(src) + ": got timestamp (" + ts.to_string()
						+ ") that is prior to the prev stored timestamp (" + lts.to_string() + ")");
					T18_COMP_POP;
				}
			}
		}

		void _checkBar(const bar_t& bar, const char* src, const bool bAllowSameDt, const bool bNoVolumeCheck = false) const noexcept {
			_checkTS(bar, src, bAllowSameDt);
			if (UNLIKELY((!bNoVolumeCheck && bar.invalid()) || (bNoVolumeCheck && bar.ohlc_invalid()))) {
				T18_ASSERT(!"Invalid bar!");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error(::std::string(src) + ": invalid data passed! "
					+ bar.ohlcv_to_string());
				T18_COMP_POP;
			}
		}

		template<bool bFromNotify, typename TfConvT>
		void _checkShouldCloseCurBar(mxTimestamp ts, TfConvT& tfConv) noexcept {
			const bar_t*const pBtc = tfConv.shouldCloseCurBar(ts);
			if (pBtc) {
				T18_DEBUG_ONLY( if (bFromNotify)_verifyWasClosed(); )

				//current bar must already be the same as pBtc, because we're handling each onClose and updating the state
				T18_ASSERT(*pBtc == this->lastBar());

				base_class_updH_t::_onNewBarClose(*pBtc);
			}
		}

		//////////////////////////////////////////////////////////////////////////
	public:
		// #todo hide this (updating) interface from trade system code.
		//protected://only MarketDataStorServ interface is public
		//	friend ::t18::tickerServer;

		// Here's a general scheme of how these notify*()/newBarOpen()/aggregate functions work.
		// aggregate() does NOT emits any events at all. It just updates current bar internal representation.
		// newBarOpen() may fire onNewBarOpen IFF the incoming timestamp doesn't fit to current timeframe period
		// notifyDateTime() may file onNewBarClose IFF the timestamp is past current timeframe period
		// 
		// In order to ensure correct sequence of execution of timeframe callbacks, there're special *_pre() and *_post()
		// functions added. *_pre() function receives the current time and uses it to decide whether it must close
		// current bar. Then the main function is called; it does all the job but doesn't call callbacks.
		// After that, the corresponding _post() function is executed; it in turn executes any callbacks registered
		// on the main function.
		// The described sequence ensures that all callbacks in different timeframes of different tickers are called
		// in the correct order.
		// 
		// newTick*() function family is similar to newBar*() functions, but does all their work using a single function set.
		// newTick*() and newBar*() should never be used in conjuntion
		// (i.e. either the timeframe object is fed by newBar*(), or by newTick*(), but not by both).
		// newTick_pre(timestamp) checks whether the timestamp of new tick belongs to a new bar and if it does, emits onNewBarClose()
		//		event. While the execution flow passes through all timeframes, it may trigger all the necessary onNewBarClose() events
		// newTick(tsTick) performs either creation of new or an update of the current bar with the new tick data.
		// newTick_post() emits onNewBarOpen() event IFF a new bar was created during previous step.
		//
		// _notifyDateTime() could be called anytime with a current timestamp and is used to detect current bar ending
		// Consider the case: you have a basic timeframe M1 and a derived TF M15. A market has very low
		// liquidity and there might be huge (more than a minute) time gaps between deals. Say, you've
		// got last bar in 14:58 and now it's 15:01 and there're still no M1 bars coming. But you must at least close
		// the M15 bar that spans between 14:45 and 15:00. Thats what that function is for.
		// 
		// BTW: use _notifyDateTime() with care or better do not use at all, and always take a network delays into account.
		// If your terminal will get a delayed packet with, for example,
		// 15:00 M1 bar later and you'll try to call _newBarOpen() - something bad may happen.
		// Also, a problem will arise if the _newBarOpen() was called without accompanying _newBarAggregate()
		// before the _notifyDateTime() is called
		// #todo: deal with it.
		// Generally, the safest use of this function is to mark end of trading session after some significant delay that 
		// allows any network packets either to reach a terminal, or to be lost forever.
		
	protected:
		template<typename TfConvT>
		void _notifyDateTime(mxTimestamp ts, TfConvT& tfConv) noexcept {
			_checkTS(ts, "_notifyDateTime", false);
			_checkShouldCloseCurBar<true>(ts, tfConv);
		}

	public:
		void _notifyDateTime_post(mxTimestamp ts) noexcept {
			base_class_updH_t::_onNotifyDateTime(ts);
		}
	
	protected:
		template<typename TfConvT>
		void _newBarOpen_pre(mxTimestamp ts, TfConvT& tfConv) noexcept {
			T18_ASSERT(m_lastTimeFilterTime <= ts);
			T18_DEBUG_ONLY(m_lastTimeFilterTime = ts);
			m_lastTimeFilterReject = m_timeFltr.shouldReject(ts.Time());
			if (m_lastTimeFilterReject) return;

			T18_DEBUG_ONLY(_verifyWasClosed();)
			_checkTS(ts, "_newBarOpen_pre", false);

			//first we must check if the prev bar must have already been completed here
			// #WARNING ! listening to onBarClose() and acting/trading in a event handler might lead to assertions failure assotiated with
			// ticker.lastKnownQuote, because a bar to be closed belongs to a previous TFbar, but lastKnownQuote was already refreshed to new data
			_checkShouldCloseCurBar<false>(ts, tfConv);
		}

		// #todo must use some type, taken from the bar_t:: here.
		template<typename TfConvT>
		void _newBarOpen(const tsq_data& tso, TfConvT& tfConv) noexcept {
			T18_ASSERT(tso.TS() == m_lastTimeFilterTime);
			if (m_lastTimeFilterReject) return;

			T18_DEBUG_ONLY(_verifyWasClosed();)
			T18_DEBUG_ONLY(++m_openCloseState;)

			T18_DEBUG_ONLY(_checkTS(tso, "_newBarOpen", false));

			if (UNLIKELY(tso.q <= 0)) {
				T18_ASSERT(!"Invalid Open");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("_newBarOpen: invalid open = "s + tso.to_string());
				T18_COMP_POP;
			}
			//#TODO need a better handler for inconsistent data. It should not throw at all!		

			//then checking if the new bar opening really occurs
			m_pCurBar = tfConv.isReallyNewBarOpen(tso);
			if (m_pCurBar) {
				//this is a start of a bar of ours timeframe
				T18_DEBUG_ONLY(_checkBar(*m_pCurBar, "_newBarOpen", false, true));

				//inserting new bar into TsCont
				base_class_t::storeBar(m_pCurBar->to_hmap());
			}
		}

	public:
		void _newBarOpen_post() noexcept {
			if (m_lastTimeFilterReject) return;

			//T18_ASSERT(1 == m_openCloseState);

			if (m_pCurBar) {
				//notifying subscribers about new bar start
				base_class_updH_t::_onNewBarOpen(m_pCurBar->TSQ());
				m_pCurBar = nullptr;
			}
		}

	protected:
		template<typename TfConvT>
		void _newBarAggregate(const bar_t& bar, TfConvT& tfConv) noexcept {
			T18_ASSERT(bar.TS() == m_lastTimeFilterTime);
			if (m_lastTimeFilterReject) return;

			T18_DEBUG_ONLY(_verifyWasOpenedAndClose();)

			//check for consistence first
			_checkBar(bar, "_newBarAggregate", true);
			//#TODO need a better handler for inconsistent data. It should not throw at all!

			m_pCurBar = tfConv.aggregate(bar);
			T18_ASSERT(m_pCurBar);
			T18_DEBUG_ONLY(_checkBar(*m_pCurBar, "_newBarAggregate", true));
			
			//we MUST update cur bar representation

			T18_COMP_SILENCE_FLOAT_CMP_UNSAFE
			//T18_ASSERT(lastDate() == m_pCurBar->d && lastTime() == m_pCurBar->t && lastOpen() == m_pCurBar->o);
			T18_ASSERT(lastTimestamp() == m_pCurBar->TS() && lastOpen() == m_pCurBar->o());
			T18_COMP_POP

			_updateLastBar(m_pCurBar->data2update_to_hmap());
		}

	public:
		/*void _newBarAggregate_post() {
			if (m_lastTimeFilterReject) return;

			T18_ASSERT(0 == m_openCloseState);

			if (m_curBarJustClosed) {
				//notifying subscribers about new bar end
				T18_ASSERT(m_pCurBar);
				base_class_updH_t::_onNewBarClose(*m_pCurBar);
				m_curBarJustClosed = false;
			}
			m_pCurBar = nullptr;
		}*/

		//////////////////////////////////////////////////////////////////////////

		// newTick*() function family is similar to newBar*() functions, but does all their work using a single function set.
		// newTick*() and newBar*() should never be used in conjunction
		// (i.e. either the timeframe object is fed by newBar*(), or by newTick*(), but not by both).
		// newTick_pre(timestamp) checks whether the timestamp of new tick belongs to a new bar and if it does, emits onNewBarClose()
		//		event. While the execution flow passes through all timeframes, it may trigger all the necessary onNewBarClose() events
		// newTick(tsTick) performs either creation of new or an update of the current bar with the new tick data.
		// newTick_post() emits onNewBarOpen() event IFF a new bar was created during previous step.
	protected:

		//very similar to _newBarOpen_pre()
		template<typename TfConvT>
		void _newTick_pre(mxTimestamp ts, TfConvT& tfConv) noexcept {
			T18_ASSERT(m_lastTimeFilterTime <= ts);
			T18_DEBUG_ONLY(m_lastTimeFilterTime = ts);
			m_lastTimeFilterReject = m_timeFltr.shouldReject(ts.Time());
			if (m_lastTimeFilterReject) return;

			_checkTS(ts, "_newTick_pre", true);
			
			//first we must check if the prev bar must have already been completed here
			// #WARNING ! listening to onBarClose() and acting/trading in a event handler might lead to assertions failure assotiated with
			// ticker.lastKnownQuote, because a bar to be closed belongs to a previous TFbar, but lastKnownQuote was already refreshed to new data
			_checkShouldCloseCurBar<false>(ts, tfConv);
		}

		// #todo must use some type, taken from the bar_t:: here ?
		template<typename TfConvT>
		void _newTick(const tsTick& tst, TfConvT& tfConv) noexcept {
			T18_ASSERT(tst.TS() == m_lastTimeFilterTime);
			if (m_lastTimeFilterReject) return;

			T18_DEBUG_ONLY(_checkTS(tst, "_newTick", true));

			if (UNLIKELY(tst.invalid())) {
				T18_ASSERT(!"Invalid tick data");
				T18_COMP_SILENCE_THROWING_NOEXCEPT;
				throw ::std::runtime_error("_newTick: invalid tick = "s + tst.to_string());
				T18_COMP_POP;
			}
			//#TODO need a better handler for inconsistent data. It should not throw at all!		

			//then checking if the new bar opening really occurs
			m_pCurBar = tfConv.isReallyNewBarOpen(tst);
			if (m_pCurBar) {
				//this is a start of a bar of ours timeframe
				T18_DEBUG_ONLY(_checkBar(*m_pCurBar, "_newTick", false));

				//inserting new bar into TsCont
				base_class_t::storeBar(m_pCurBar->to_hmap());
				//leaving m_pCurBar non null to run event callback during _post phase
			} else {
				//we must update check-update the cur bar
				const auto pCurBar = tfConv.aggregate(tst);
				T18_ASSERT(pCurBar);
				T18_DEBUG_ONLY(_checkBar(*pCurBar, "_newTick", true));
				//in any case we should update cur bar representation

				T18_ASSERT(lastTimestamp() == pCurBar->TS());

				_updateLastBar(pCurBar->data2update_to_hmap());
			}
		}

	public:
		void _newTick_post() noexcept {
			_newBarOpen_post();
		}


	};

} }

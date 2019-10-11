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

#include "timeframeStor.h"
#include "../tfConverter/tfConvBase.h"

//#TODO: some terminals might "redraw" some amount of previous bars due to network latency problems or something else.
//To deal with it, we probably should provide an API to reset and recalculate some amount of already stored data!?
//#TODO or at least to detect such events!

namespace t18 {

	//class tickerServer;//fwd declaration for friend spec.

	namespace timeseries {

		namespace hana = ::boost::hana;
		using namespace hana::literals;
		using namespace ::std::literals;
		
		//this class is designed to fill the internal timeframeStor storage with data
		template<typename TfConvT> // = tfConverter::tfConvBase<tsohlcv>>
		class Timeframe : public timeframeStor<typename TfConvT::bar_t>
		{
		private:
			typedef timeframeStor<typename TfConvT::bar_t> base_class_t;
			//typedef Timeframe<TfConvT> self_t;

		public:
			typedef TfConvT tfConv_t;
			using typename base_class_t::bar_t;

		protected:
			tfConv_t m_tfConv;

		public:
			Timeframe(size_t barsHistory, tfConv_t&& tfConv)
				: base_class_t(barsHistory), m_tfConv(::std::move(tfConv))
			{}

			template<class... TfConvArgsT>
			Timeframe(size_t barsHistory, TfConvArgsT&&... a) : base_class_t(barsHistory), m_tfConv(::std::forward<TfConvArgsT>(a)...)
			{}

			int TF()const noexcept { return m_tfConv.tf(); }
			//int BaseTF()const noexcept { return m_tfConv.baseTf(); }
			bool lastBarJustClosed()const noexcept { return m_tfConv.lastBarClosed(); }
			//bool lastBarJustOpened()const noexcept { return m_tfConv.lastBarJustOpened(); }
			
		public:
			// #todo hide this (updating) interface from trade system code.
		//protected://only MarketDataStorServ interface is public
		//	friend ::t18::tickerServer;
			
			void _notifyDateTime(mxTimestamp ts) noexcept {
				base_class_t::_notifyDateTime(ts, m_tfConv);
			}			

			////////////////////////////////////////////////////////////////////////////
			// we're expecting that for each bar the _newBarOpen() and _newBarAggregate() will be called exactly once
			// _newBarOpen must be called exactly one time for each bar as soon as that bar starts (caller knows bar open)
			void _newBarOpen_pre(mxTimestamp ts) noexcept {
				base_class_t::_newBarOpen_pre(ts, m_tfConv);
			}
			
			// #todo must use some type, taken from the bar_t:: here.
			void _newBarOpen(const tsq_data& tso) noexcept {
				base_class_t::_newBarOpen(tso, m_tfConv);
			}

			// _newBarAggregate() must be called exactly one time for each bar as soon as that bar ends (caller knows all bar data)
			// _newBarAggregate() only changes the state of the timeframe object, but must not call any callbacks
			void _newBarAggregate(const bar_t& bar) noexcept {
				base_class_t::_newBarAggregate(bar, m_tfConv);
			}


			void _newTick_pre(mxTimestamp ts) noexcept {
				base_class_t::_newTick_pre(ts, m_tfConv);
			}
			void _newTick(const tsTick& tst) noexcept {
				base_class_t::_newTick(tst, m_tfConv);
			}
		};

} }

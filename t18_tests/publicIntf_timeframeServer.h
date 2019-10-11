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

#include "../t18/timeseries/Timeframe.h"

namespace {
	using namespace t18;

	template<typename TfConvT> // = tfConverter::tfConvBase<tsohlcv>>
	class publicIntf_timeframeServer : public ::t18::timeseries::Timeframe<TfConvT> {
	private:
		typedef ::t18::timeseries::Timeframe<TfConvT> base_class_t;

	public:
		template<class...Args>
		publicIntf_timeframeServer(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

		using base_class_t::_getTs;

		void notifyDateTime(::t18::mxTimestamp dt) {
			base_class_t::_notifyDateTime(dt);
			base_class_t::_notifyDateTime_post(dt);
		}
		void notifyDateTime(::t18::mxDate d, ::t18::mxTime t) {
			notifyDateTime(::t18::mxTimestamp(d, t));
		}
		void notifyDateTime(milDate dMil, milTime tMil) { notifyDateTime(mxTimestamp(mxDate(dMil), mxTime(tMil))); }

		void newBarOpen(const ::t18::tsq_data& dto) {
			base_class_t::_newBarOpen_pre(dto);
			base_class_t::_newBarOpen(dto);
			base_class_t::_newBarOpen_post();
		}
		void newBarOpen(mxTimestamp ts, real_t o) { newBarOpen(tsq_data(ts, o)); }
		void newBarOpen(milDate dMil, milTime tMil, real_t o) { newBarOpen(tsq_data(mxTimestamp(mxDate(dMil), mxTime(tMil)), o)); }
		void newBarOpen(mxDate d, mxTime t, real_t o) { newBarOpen(tsq_data(mxTimestamp(d, t), o)); }

		void newBarAggregate(const ::t18::tsohlcv& bar) {
			base_class_t::_newBarAggregate(bar);
			//base_class_t::_newBarClose_post();
		}
		void newBarAggregate(mxTimestamp ts, real_t o, real_t h, real_t l, real_t c, real_t v) {
			newBarAggregate(tsohlcv(ts, o, h, l, c, v));
		}
		void newBarAggregate(milDate dMil, milTime tMil, real_t o, real_t h, real_t l, real_t c, real_t v) {
			newBarAggregate(tsohlcv( mxTimestamp(mxDate(dMil), mxTime(tMil)), o, h, l, c, v));
		}
		void newBarAggregate(mxDate d, mxTime t, real_t o, real_t h, real_t l, real_t c, real_t v) {
			newBarAggregate(tsohlcv( mxTimestamp(d, t), o, h, l, c, v));
		}

		template<typename TickT>
		void newTick(const TickT& tst) {
			base_class_t::_newTick_pre(tst);
			base_class_t::_newTick(tst);
			base_class_t::_newTick_post();
		}
	};

}
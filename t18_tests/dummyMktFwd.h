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

#include "../t18/tags.h"

//use only with ticker or timeframe objects
template<typename FwdToT>
struct dummyMktFwd : public ::t18::feeder::dummyMkt {
	FwdToT& b;

	dummyMktFwd(FwdToT& _b) : ::t18::feeder::dummyMkt(), b(_b) {}

	template<typename TickerT>
	void newBarOpen(TickerT&, const tsq_data& tso) {
		b.newBarOpen(tso);
	}

	template<typename TickerT, typename BarT>
	void newBarAggregate(TickerT&, BarT const& bar) {
		b.newBarAggregate(bar);
	}

	template<typename T>
	void notifyDateTime(T&&, mxTimestamp ts) {
		b.notifyDateTime(ts);
	}

	template<typename T, typename TickT>
	void newTick(T&&, const TickT& tick) {
		b.newTick(tick);
	}
};

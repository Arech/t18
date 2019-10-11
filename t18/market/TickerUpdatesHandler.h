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

#include "../timeseries/updatesHandler.h"

namespace t18 {

	//designed to handle callbacks, originated in Ticker objects, i.e. onNewBarOpen, onNotifyDateTime and onAggregate
	//Note that onTick event is actually reduced to onNewBarOpen(), so use onNewBarOpen to get onTick notifications
	// (didn't find any case when onTick callback might need a volume field of tsTick data)
	// onAggregate is similar to onNewBarClose, but is called whenever any incoming bar closes instead of a bar of concrete timeframe,
	// so use _onNewBarClose() as onAggregate() event
	class TickerUpdatesHandler : public timeseries::TFUpdatesHandler {
	private:
		typedef TickerUpdatesHandler self_t;
		typedef timeseries::TFUpdatesHandler base_class_t;

	protected:
		TickerUpdatesHandler()noexcept : base_class_t(){}
	};


}

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
#include "stdafx.h"

#include "publicIntf_timeframeServer.h"
namespace hana = ::boost::hana;
using namespace hana::literals;
using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestDtohlcvServer, Basic) {
	using namespace t18;
	//publicIntf_timeframeServer ts(size_t(3), ::std::make_unique<tfConverter::tfConvBase<>>(1));
	publicIntf_timeframeServer<tfConverter::tfConvBase<tsohlcv>> ts(size_t(3), 1);

	bool bOnNewBarCalled(false);

	auto h1 = ts.registerOnNewBarClose([&](const tsohlcv&) {
		ASSERT_FALSE(bOnNewBarCalled);
		bOnNewBarCalled = true;
		STDCOUTL("onNewBarClose callback executed");
	});

	auto h2 = ts.registerOnNewBarClose([&](const tsohlcv&) {
		ASSERT_TRUE(bOnNewBarCalled) << "Invalid order!";
		STDCOUTL("onNewBarClose callback2 executed");
	});

	mxTimestamp tx(milDate(20180903), milTime(154201));

	ts.newBarOpen(tx, 3.5);
	ts.newBarAggregate(tx, 3.5, 4.5, 2.5, 4, 1);
	ts.notifyDateTime(tx.plusYear());//to make onClose event, that runs either with newBarOpen_pre, or notifyDateTime

	ASSERT_TRUE(bOnNewBarCalled);

	ASSERT_EQ(ts.get("ts"_s, 0), tx.TFLowerBoundary(ts.TF()));
	ASSERT_EQ(ts.get("open"_s, 0), 3.5);
	ASSERT_EQ(ts.get("high"_s, 0), 4.5);
	ASSERT_EQ(ts.get("low"_s, 0), 2.5);
	ASSERT_EQ(ts.get("close"_s, 0), 4);
	ASSERT_EQ(ts.get("vol"_s, 0), 1);
}
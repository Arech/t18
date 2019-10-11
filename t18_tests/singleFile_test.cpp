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
#include "stdafx.h"

#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"
//#include "../t18/tfConverter/dailyhm.h"
#include "publicIntf_timeframeServer.h"
#include "dummyMktFwd.h"

using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestSingleFile, Basic) {
	using namespace t18;
	//publicIntf_timeframeServer ts(size_t(3), ::std::make_unique<tfConverter::tfConvBase<>>(1));
	publicIntf_timeframeServer<tfConverter::tfConvBase<tsohlcv>> ts(size_t(3), 1);

	real_t lastOpen(0);
	auto h1 = ts.registerOnNewBarOpen([&](const tsq_data& dto) {
		lastOpen = dto.q;
	});
	size_t tb = 0;
	auto h2 = ts.registerOnNewBarClose([&](const tsohlcv& b) {
		ASSERT_EQ(ts.get("open"_s, 0), lastOpen);
		ASSERT_EQ(ts.TotalBars(), ++tb);
		ASSERT_EQ(ts.bar(0), b);
	});

	typedef feeder::adapters::csv_tsohlcv adapt_t;
	typedef feeder::singleFile<adapt_t> feeder_t;	
	feeder_t::processFile(dummyMktFwd<decltype(ts)>(ts), 0, TESTS_TESTDATA_DIR "dtohlcv.csv", adapt_t());

	ASSERT_EQ(tb, 5);
	ASSERT_EQ(ts.TotalBars(), 5);

	ASSERT_EQ(ts.get("open"_s, 0), lastOpen);
	ASSERT_EQ(ts.get("open"_s, 0), 173.52);
	ASSERT_EQ(ts.get("high"_s, 1), 173.53);
	ASSERT_EQ(ts.get("vol"_s, 2), 46330);
}

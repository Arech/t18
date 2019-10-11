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
#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"
#include "../t18/tfConverter/dailyhm.h"
#include "dummyMktFwd.h"
#include "../t18/utils/scope_exit.h"

using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestTfConvDailyhm, Basic) {
	using namespace t18;
	//publicIntf_timeframeServer tsBase(size_t(3), ::std::make_unique<tfConverter::tfConvBase<>>(1));
	publicIntf_timeframeServer<tfConverter::tfConvBase<tsohlcv>> tsBase(size_t(3), 1);
	//publicIntf_timeframeServer tsM2(size_t(3), ::std::make_unique<tfConverter::dailyhm<>>(1, 2));
	publicIntf_timeframeServer<tfConverter::dailyhm<tsohlcv>> tsM2(size_t(3), /*1,*/ 2);

	auto h1 = tsBase.registerOnNewBarOpen([&](const tsq_data& dto) {
		tsM2.newBarOpen(dto);
	});
	auto h2 = tsBase.registerOnNewBarClose([&](const tsohlcv& bar) {
		tsM2.newBarAggregate(bar);
	});
	auto h3 = tsBase.registerOnNotifyDateTime([&](mxTimestamp dt) {
		tsM2.notifyDateTime(dt);
	});

	//feeder::csvDtohlcv(tsBase, TESTS_TESTDATA_DIR "dtohlcv.csv");
	typedef feeder::adapters::csv_tsohlcv adapt_t;
	typedef feeder::singleFile<adapt_t> feeder_t;
	feeder_t::processFile(dummyMktFwd<decltype(tsBase)>(tsBase), 0, TESTS_TESTDATA_DIR "dtohlcv.csv", adapt_t());

	ASSERT_EQ(tsBase.TotalBars(), 5);
	ASSERT_EQ(tsBase.size(), 3);

	ASSERT_EQ(tsBase.bar(0), tsohlcv(tag_milDT(), 20170103, 100400, 173.5200000, 173.8400000, 173.0000000, 173.5900000, 248160));
	ASSERT_EQ(tsBase.bar(1), tsohlcv(tag_milDT(), 20170103, 100300, 173.3000000, 173.5300000, 173.3000000, 173.5200000, 96690));
	ASSERT_EQ(tsBase.bar(2), tsohlcv(tag_milDT(), 20170103, 100200, 173.2600000, 173.4000000, 173.2500000, 173.2500000, 46330));

	ASSERT_EQ(tsM2.TotalBars(), 3);
	ASSERT_EQ(tsM2.size(), 3);

	ASSERT_EQ(tsM2.bar(0), tsohlcv(tag_milDT(), 20170103, 100400, 173.5200000, 173.8400000, 173.0000000, 173.5900000, 248160));
	ASSERT_EQ(tsM2.bar(1)
		, tsohlcv(tag_milDT(), 20170103, 100200, 173.2600000, 173.5300000, 173.2500000, 173.5200000, 96690 + 46330)
	) << "bar1 = " << tsM2.bar(1).to_string();

	ASSERT_EQ(tsM2.bar(2)
		, tsohlcv(tag_milDT(), 20170103, 100000, 173.4100000, 173.5000000, 173.1100000, 173.2600000, 100970 + 148360)
	) << "bar2 = " << tsM2.bar(2).to_string();
}

void convertAndCheck(int destTf, const char* pszSrcFile, const char* pszTestFile);
void convertAndCheck(int destTf, const char* pszSrcFile, const char* pszTestFile) {
	using namespace t18;

	typedef feeder::adapters::csv_tsohlcv adapt_t;

	//publicIntf_timeframeServer tsDest(1u, ::std::make_unique<tfConverter::dailyhm<>>(srcTf, destTf));
	publicIntf_timeframeServer<tfConverter::dailyhm<tsohlcv>> tsDest(1u, /*srcTf,*/ destTf);

	///////////////////////////////////
	//preparing pszDestFile for reading & comparing 

	FILE* pF = nullptr;
	utils::scope_exit on_exit([&pF]() {
		if (pF) {
			fclose(pF);
			pF = nullptr;
		}
	});
	auto ec = fopen_s(&pF, pszTestFile, "rb");
	if (ec) {
		char buf[128];
		strerror_s(buf, ec);
		throw T18_FILESYSTEM_NAMESPACE::filesystem_error(
			"Failed to open file, reason: "s + buf
			, T18_FILESYSTEM_NAMESPACE::path(pszTestFile)
			, ::std::error_code(ec, ::std::system_category())
		);
	}
	T18_ASSERT(pF);

	adapt_t adpt;

	auto hndl = tsDest.registerOnNewBarClose([&tsDest, &adpt, pF, destTf](const tsohlcv& bar) {
		tsohlcv correctBar;
		int n;
		ASSERT_TRUE(adpt.readNext(pF, correctBar, n));

		ASSERT_EQ(bar, tsohlcv(correctBar.TFLowerBoundary(destTf), correctBar)) << "Bar comparison #" << tsDest.TotalBars()
			<< " failed.\nMade " << bar.to_string() << "\nwhile expect " << correctBar.to_string();
	});

	typedef feeder::singleFile<adapt_t> feeder_t;
	feeder_t::processFile(dummyMktFwd<decltype(tsDest)>(tsDest), 0, pszSrcFile, adapt_t());
}

#if defined(T18_TESTS_DO_SLOW_TESTS) && T18_TESTS_DO_SLOW_TESTS
TEST(TestTfConvDailyhm, M1toM30) {
	convertAndCheck(/*1, */30, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn.csv", TESTS_TESTDATA_DIR "GAZP_060101_170101-trn-M30.csv");
}

TEST(TestTfConvDailyhm, M1toM15) {
	convertAndCheck(/*1,*/ 15, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn.csv", TESTS_TESTDATA_DIR "GAZP_060101_170101-trn-M15.csv");
}

TEST(TestTfConvDailyhm, M1toM2) {
	convertAndCheck(/*1,*/ 2, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn.csv", TESTS_TESTDATA_DIR "GAZP_060101_170101-trn-M2.csv");
}
#endif

TEST(TestTfConvDailyhm, M1toD) {
	convertAndCheck(/*1,*/24 * 60, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn.csv", TESTS_TESTDATA_DIR "GAZP_060101_170101-trn-D.csv");
}

void ticksConvertAndCheck(int destTf, const char* pszSrcFile, const char* pszTestFile);
void ticksConvertAndCheck(int destTf, const char* pszSrcFile, const char* pszTestFile) {
	using namespace t18;

	typedef feeder::adapters::csv_tsohlcv adaptOhlc_t;
	typedef feeder::adapters::csv_tsTick adaptTicks_t;

	publicIntf_timeframeServer<tfConverter::dailyhm<tsohlcv>> tsDest(1u, destTf);

	///////////////////////////////////
	//preparing pszDestFile for reading & comparing 

	FILE* pF = nullptr;
	utils::scope_exit on_exit([&pF]() {
		if (pF) {
			fclose(pF);
			pF = nullptr;
		}
	});
	auto ec = fopen_s(&pF, pszTestFile, "rb");
	if (ec) {
		char buf[128];
		strerror_s(buf, ec);
		throw T18_FILESYSTEM_NAMESPACE::filesystem_error(
			"Failed to open file, reason: "s + buf
			, T18_FILESYSTEM_NAMESPACE::path(pszTestFile)
			, ::std::error_code(ec, ::std::system_category())
		);
	}
	T18_ASSERT(pF);

	adaptOhlc_t adpt;
	tsDest.updateTimeFilter(TimeFilter(mxTime(tag_milTime(), 100000)));

	auto hndl = tsDest.registerOnNewBarClose([&tsDest, &adpt, pF, destTf](const tsohlcv& bar) {
		tsohlcv correctBar;
		int n;
		ASSERT_TRUE(adpt.readNext(pF, correctBar, n));

		ASSERT_EQ(bar, tsohlcv(correctBar.TFLowerBoundary(destTf), correctBar)) << "Bar comparison #" << tsDest.TotalBars()
			<< " failed.\nMade " << bar.to_string() << "\nwhile expect " << correctBar.to_string();
	});

	typedef feeder::singleFile<adaptTicks_t> feeder_t;
	feeder_t::processFile(dummyMktFwd<decltype(tsDest)>(tsDest), 0, pszSrcFile, adaptTicks_t());
}

#if defined(T18_TESTS_DO_SLOW_TESTS) && T18_TESTS_DO_SLOW_TESTS
TEST(TestTfConvDailyhm, TicksToM1) {
	ticksConvertAndCheck(1, TESTS_TESTDATA_DIR "trades_GAZP@TQBR_2019_01_03_2019_01_28.csv", TESTS_TESTDATA_DIR "GAZP_190103_190128-NO_AO.csv");
}

TEST(TestTfConvDailyhm, TicksToM5) {
	ticksConvertAndCheck(5, TESTS_TESTDATA_DIR "trades_GAZP@TQBR_2019_01_03_2019_01_28.csv", TESTS_TESTDATA_DIR "GAZP_190103_190128-NO_AO-M5.csv");
}

TEST(TestTfConvDailyhm, TicksToM60) {
	ticksConvertAndCheck(60, TESTS_TESTDATA_DIR "trades_GAZP@TQBR_2019_01_03_2019_01_28.csv", TESTS_TESTDATA_DIR "GAZP_190103_190128-NO_AO-M60.csv");
}
#endif

TEST(TestTfConvDailyhm, TicksToD) {
	ticksConvertAndCheck(24 * 60, TESTS_TESTDATA_DIR "trades_GAZP@TQBR_2019_01_03_2019_01_28.csv", TESTS_TESTDATA_DIR "GAZP_190103_190128-NO_AO-D.csv");
}
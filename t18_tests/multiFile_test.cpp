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

#include "../t18/feeder/multiFile.h"
//#include "../t18/tfConverter/dailyhm.h"
#include "publicIntf_timeframeServer.h"
#include "../t18/market/MarketDataStorServ.h"

using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

template<typename TickersSetHST>
class Mdss_tester : public MarketDataStorServ<TickersSetHST>{
public:
	typedef MarketDataStorServ<TickersSetHST> base_class_t;


public:
	template<class... Args>
	Mdss_tester(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

};

TEST(TestMultiFile, Basic) {
	using namespace t18;

	typedef decltype("tf"_s) tfid_hst;
	typedef utils::makeMap_t<utils::Descr_t<tfid_hst, timeseries::Timeframe<tfConverter::baseOhlc>>> TickerTimeframes_hdm;
	typedef tickerServer<true, TickerTimeframes_hdm> Ticker_t;
	
	Mdss_tester<utils::make_set_t<Ticker_t>> mkt;

	auto& tickr1 = mkt.newTicker<Ticker_t>("dtohlcv", real_t(.01), tfid_hst(), 3u, 1);
	auto& tf = tickr1.getTf<tfid_hst>();

	real_t lastOpen(0);
	auto h1 = tf.registerOnNewBarOpen([&](const tsq_data& dto) {
		lastOpen = dto.q;
	});
	size_t tb = 0;
	auto h2 = tf.registerOnNewBarClose([&](const tsohlcv& b) {
		ASSERT_EQ(tf.get("open"_s, 0), lastOpen);
		ASSERT_EQ(tf.TotalBars(), ++tb);
		ASSERT_EQ(tf.bar(0), b);
	});

	feeder::multiFile<> feed(TESTS_TESTDATA_DIR);
	feed(mkt);

	ASSERT_EQ(tb, 5);
	ASSERT_EQ(tf.TotalBars(), 5);

	ASSERT_EQ(tf.get("open"_s, 0), lastOpen);
	ASSERT_EQ(tf.get("open"_s, 0), 173.52);
	ASSERT_EQ(tf.get("high"_s, 1), 173.53);
	ASSERT_EQ(tf.get("vol"_s, 2), 46330);
}

TEST(TestMultiFile, SequenceSame) {
	using namespace t18;

	typedef decltype("tf"_s) tfid_hst;
	typedef utils::makeMap_t<utils::Descr_t<tfid_hst, timeseries::Timeframe<tfConverter::baseOhlc>>> TickerTimeframes_hdm;
	typedef tickerServer<true, TickerTimeframes_hdm> Ticker_t;

	Mdss_tester<utils::make_set_t<Ticker_t>> mkt;

	auto& tickr1 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_same_1", real_t(.01), tfid_hst(), 3u, 1);
	auto& tickr2 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_same_2", real_t(.01), tfid_hst(), 3u, 1);
	auto& tickr3 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_same_3", real_t(.01), tfid_hst(), 3u, 1);
	
	mxTimestamp prevDt(tag_milDT(), 20000101, 0);
	int rc = 0;
	auto seqChecker = [&](const tsq_data& dto) {
		ASSERT_LE(prevDt, dto.TS());
		prevDt = dto.TS();
		++rc;
	};
	
	auto h1 = tickr1.registerOnNewBarOpen(seqChecker);
	auto h2 = tickr2.registerOnNewBarOpen(seqChecker);
	auto h3 = tickr3.registerOnNewBarOpen(seqChecker);
	
	ASSERT_EQ(tickr1.getTf<tfid_hst>().TotalBars(), 0);
	ASSERT_EQ(tickr2.getTf<tfid_hst>().TotalBars(), 0);
	ASSERT_EQ(tickr3.getTf<tfid_hst>().TotalBars(), 0);
	ASSERT_EQ(rc, 0);

	feeder::multiFile<> feed(TESTS_TESTDATA_DIR);
	feed(mkt);

	ASSERT_EQ(tickr1.getTf<tfid_hst>().TotalBars(), 5);
	ASSERT_EQ(tickr2.getTf<tfid_hst>().TotalBars(), 5);
	ASSERT_EQ(tickr3.getTf<tfid_hst>().TotalBars(), 5);
	ASSERT_EQ(rc, 15);
}

TEST(TestMultiFile, SequenceDiff) {
	using namespace t18;

	typedef decltype("tf"_s) tfid_hst;
	typedef utils::makeMap_t<utils::Descr_t<tfid_hst, timeseries::Timeframe<tfConverter::baseOhlc>>> TickerTimeframes_hdm;
	typedef tickerServer<true, TickerTimeframes_hdm> Ticker_t;

	Mdss_tester<utils::make_set_t<Ticker_t>> mkt;

	auto& tickr1 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_diff_1", real_t(.01), tfid_hst(), 3u, 1);
	auto& tickr2 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_diff_2", real_t(.01), tfid_hst(), 3u, 1);
	auto& tickr3 = mkt.newTicker<Ticker_t>("dtohlcv_csvLib_seq_diff_3", real_t(.01), tfid_hst(), 3u, 1);

	mxTimestamp prevDt(tag_milDT(), 20000101, 0);
	int rc = 0;
	auto seqChecker = [&](const tsq_data& dto) {
		ASSERT_LE(prevDt, dto.TS());
		prevDt = dto.TS();
		++rc;
	};

	auto h1 = tickr1.registerOnNewBarOpen(seqChecker);
	auto h2 = tickr2.registerOnNewBarOpen(seqChecker);
	auto h3 = tickr3.registerOnNewBarOpen(seqChecker);

	auto& b1 = tickr1.getTf<tfid_hst>();
	auto& b2 = tickr2.getTf<tfid_hst>();
	auto& b3 = tickr3.getTf<tfid_hst>();

	ASSERT_EQ(b1.TotalBars(), 0);
	ASSERT_EQ(b2.TotalBars(), 0);
	ASSERT_EQ(b3.TotalBars(), 0);
	ASSERT_EQ(rc, 0);

	feeder::multiFile<> feed(TESTS_TESTDATA_DIR);
	feed(mkt);

	ASSERT_EQ(b1.TotalBars(), 5);
	ASSERT_EQ(b2.TotalBars(), 3);
	ASSERT_EQ(b3.TotalBars(), 2);
	ASSERT_EQ(rc, 5+3+2);

	ASSERT_EQ(b1.timestamp(0), mxTimestamp(tag_milDT(), 20170103, 100400));
	ASSERT_EQ(b1.timestamp(1), mxTimestamp(tag_milDT(), 20170103, 100300));
	ASSERT_EQ(b1.timestamp(2), mxTimestamp(tag_milDT(), 20170103, 100200));

	ASSERT_EQ(b1.vol(0), 11);
	ASSERT_EQ(b1.vol(1), 22);
	ASSERT_EQ(b1.vol(2), 33);


	ASSERT_EQ(b2.timestamp(0), mxTimestamp(tag_milDT(), 20170105, 100200));
	ASSERT_EQ(b2.timestamp(1), mxTimestamp(tag_milDT(), 20170103, 100200));
	ASSERT_EQ(b2.timestamp(2), mxTimestamp(tag_milDT(), 20170102, 100000));

	ASSERT_EQ(b2.vol(0), 46330);
	ASSERT_EQ(b2.vol(1), 2);
	ASSERT_EQ(b2.vol(2), 148360);


	ASSERT_EQ(b3.timestamp(0), mxTimestamp(tag_milDT(), 20170104, 100100));
	ASSERT_EQ(b3.timestamp(1), mxTimestamp(tag_milDT(), 20170103, 100000));

	ASSERT_EQ(b3.vol(0), 100970);
	ASSERT_EQ(b3.vol(1), 1);
}

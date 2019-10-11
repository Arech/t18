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

#include "../t18/exec/backtester.h"
#include "../t18/ts/MaCross.h"
#include "../t18/ts/MaCrossO.h"
#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"
#include "../t18/exec/assocTradeInfo.h"

using namespace t18;
using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestBacktester, Basic) {
	typedef decltype("tf"_s) tfid_hst;
	typedef utils::makeMap_t<utils::Descr_t<tfid_hst, timeseries::Timeframe<tfConverter::baseOhlc>>> TickerTimeframes_hdm;
	typedef tickerServer<true, TickerTimeframes_hdm> Ticker_t;
	typedef typename Ticker_t::bestPriceInfo_t bestPriceInfo_t;

	exec::backtester<utils::make_set_t<Ticker_t>> h;
	auto& tickr = h.newTicker<Ticker_t>("test", real_t(1), tfid_hst(), size_t(1), 1);
	tickr.setLotSize(10);

	mxTimestamp tx(tag_milDT(), 20180101, 192000);
	h.setBestBidAsk(tickr, bestPriceInfo_t(tx, 1, 1), bestPriceInfo_t(tx, 2, 1));
	
	auto pTrd = h.newMarketLong(tickr, 1);
	ASSERT_EQ(pTrd->getState(), TradeState::inMarket);
	ASSERT_TRUE(pTrd->isLong());
	ASSERT_TRUE(!pTrd->is_Closed());
	ASSERT_EQ(pTrd->plannedOpen().q, real_t(2));
	ASSERT_EQ(pTrd->plannedVolume(), real_t(1));
	ASSERT_EQ(pTrd->volumeInMarket(), real_t(1));
	
	pTrd->closeByMarket(TradeCloseReason::Normal);
	ASSERT_EQ(pTrd->getState(), TradeState::Closed);
	ASSERT_EQ(pTrd->getCloseReason(), TradeCloseReason::Normal);
	ASSERT_TRUE(pTrd->isLong());
	ASSERT_TRUE(pTrd->is_Closed());
	ASSERT_EQ(pTrd->plannedClose().q, real_t(1));
	ASSERT_EQ(pTrd->volumeInMarket(), real_t(0));
	ASSERT_EQ(pTrd->tradeProfit(), real_t(-10));
}


struct TrdInfo {
	int i;

	TrdInfo() : i(1){
		STDCOUTL("TrdInfo DEFAULT constructor, i=" << i);
	}
	TrdInfo(int _i) :i(_i) {
		STDCOUTL("TrdInfo constructor with i=" << i);
	}
	//had to define copy constructor because of non-trivial destructor
	TrdInfo(const TrdInfo& a) { i = a.i; }
	TrdInfo& operator=(const TrdInfo& a) { i = a.i; return *this; }

	~TrdInfo() {
		STDCOUTL("TrdInfo destructor, i=" << i);
		i = -1;
	}
};

TEST(TestBacktester, AssocTradeInfo) {
	typedef decltype("tf"_s) tfid_hst;
	typedef utils::makeMap_t<utils::Descr_t<tfid_hst, timeseries::Timeframe<tfConverter::baseOhlc>>> TickerTimeframes_hdm;
	typedef tickerServer<true, TickerTimeframes_hdm> Ticker_t;
	typedef typename Ticker_t::bestPriceInfo_t bestPriceInfo_t;

	exec::backtester<utils::make_set_t<Ticker_t>> h;
	auto& tickr = h.newTicker<Ticker_t>("test", real_t(1), tfid_hst(), size_t(1), 1);
	tickr.setLotSize(10);

	mxTimestamp tx(tag_milDT(), 20180101, 192000);
	h.setBestBidAsk(tickr, bestPriceInfo_t(tx, 1, 1), bestPriceInfo_t(tx, 2, 1));
	auto pTrd = h.newMarketLong(tickr, 1);

	assocTradeInfo<TrdInfo> assoc(1);
	ASSERT_TRUE(0== assoc.size());

	auto& ndef = assoc.store(*pTrd, TrdInfo(2));
	ASSERT_EQ(1, assoc.size());
	ASSERT_EQ(2, ndef.i);
	
	assoc.free(*pTrd);
	ASSERT_EQ(1, assoc.size());
	ASSERT_EQ(-1, ndef.i);

	auto& def = assoc.store(*pTrd);
	ASSERT_EQ(1, assoc.size());
	ASSERT_EQ(1, def.i);

	const auto& cAssoc = assoc;
	auto& d = cAssoc.get(*pTrd);
	ASSERT_EQ(1, d.i);

	assoc.free(*pTrd);
	ASSERT_EQ(1, assoc.size());
	ASSERT_EQ(-1, def.i);

	//lastPrice = tsq_data(tag_milDT(), 20180101, 192001, 1);
	//h.updateLastKnownQuote(tickr, lastPrice);
	pTrd->closeByMarket(TradeCloseReason::Normal);
}

TEST(TestBacktester, MaCross) {
	ts::MaCrossSetup<> so;

	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;

	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	feeder::singleFile<feeder::adapters::csv_tsohlcv> feed(TESTS_TESTDATA_DIR);

	bt.run(so, feed);
}

TEST(TestBacktester, MaCrossO) {
	ts::MaCrossOSetup<ts::MaCrossO<>> so;

	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;
	
	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	feeder::singleFile<feeder::adapters::csv_tsohlcv> feed(TESTS_TESTDATA_DIR);
	
	bt.run(so, feed);

	bt.bt_exportTradeList_AmibrokerCsv(TESTS_TESTDATA_DIR "gen_MaCrossO_tradelistAmi.csv");
	bt.exec4Ticker<true>(so.pTickerName, [&](auto& t) {
		bt.bt_exportTradeList_LinesFmt(t, ::std::string(TESTS_TESTDATA_DIR) + "gen_MaCrossO_" +t.Name()+ "_tradelistLines.csv");
	});
}

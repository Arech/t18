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

#include "publicIntf_tickerServer.h"
#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"
#include "../t18/tfConverter/dailyhm.h"
#include "dummyMktFwd.h"

using namespace std::literals;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestTicker, Basic) {
	using namespace t18;

	typedef decltype("base"_s) btf_ht;
	typedef decltype("m2"_s) htf_ht;

	typedef timeseries::Timeframe<tfConverter::baseOhlc> tfBase_t;
	typedef timeseries::Timeframe<tfConverter::dailyhmOhlc> tfHigher_t;
	typedef publicIntf_tickerServer<decltype(hana::make_map(utils::Descr_v<btf_ht, tfBase_t>, utils::Descr_v<htf_ht, tfHigher_t>))> piticker_t;

	piticker_t tServ(TickerId::forEveryone(), "test", real_t(.01), "base"_s, 3u, 1);

	tServ.createTf("m2"_s, 3u,/* 1,*/ 2);

	typedef feeder::adapters::csv_tsohlcv adapt_t;
	typedef feeder::singleFile<adapt_t> feeder_t;
	feeder_t::processFile(dummyMktFwd<decltype(tServ)>(tServ), 0, TESTS_TESTDATA_DIR "dtohlcv.csv", adapt_t());

	auto& tsBase = tServ.getTf("base"_s);
	ASSERT_EQ(tsBase.TotalBars(), 5);
	ASSERT_EQ(tsBase.size(), 3);

	ASSERT_EQ(tsBase.bar(0), tsohlcv(tag_milDT(), 20170103, 100400, 173.5200000, 173.8400000, 173.0000000, 173.5900000, 248160));
	ASSERT_EQ(tsBase.bar(1), tsohlcv(tag_milDT(), 20170103, 100300, 173.3000000, 173.5300000, 173.3000000, 173.5200000, 96690));
	ASSERT_EQ(tsBase.bar(2), tsohlcv(tag_milDT(), 20170103, 100200, 173.2600000, 173.4000000, 173.2500000, 173.2500000, 46330));

	auto& tsM2 = tServ.getTf("m2"_s);

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

TEST(TestTicker, Sequences) {
	using namespace t18;

	//#WARNING This test relies on specific unfolding of hana::map when ticker is calling timeframes and therefore their callbacks
	//However, this order IS NOT GUARANTEED

	typedef decltype("base"_s) btf_ht;
	typedef decltype("m5"_s) htf_ht;
	typedef decltype("D"_s) dtf_ht;

	typedef timeseries::Timeframe<tfConverter::baseOhlc> tfBase_t;
	typedef timeseries::Timeframe<tfConverter::dailyhmOhlc> tfHigher_t;
	typedef publicIntf_tickerServer<decltype(hana::make_map(utils::Descr_v<btf_ht, tfBase_t>
		, utils::Descr_v<htf_ht, tfHigher_t>, utils::Descr_v<dtf_ht, tfHigher_t>))> piticker_t;

	const int m1Tf = 1;
	const int m5Tf = 5;
	const int dailyTf = mxTime::minutesInDay;

	piticker_t tServ(TickerId::forEveryone(), "test", real_t(.01), "base"_s, 1u, m1Tf);

	auto& m1 = tServ.getTf("base"_s);
	auto& m5 = tServ.createTf("m5"_s, 1u, m5Tf);
	auto& daily = tServ.createTf("D"_s, 1u, dailyTf);

	int tv = 0;
	int odv, om1v, om5v, osv, cdv, cm1v, cm5v, csv;

	tsq_data m1Dto, m5Dto, daDto, tsDto;
	tsohlcv m1b, m5b, dab, tsb;

	auto h1 = daily.registerOnNewBarClose([&tv, &cdv, &dab](const tsohlcv& bar) {
		STDCOUTL("da.OnNewBarClose " << bar.to_string());
		if (cdv<0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(cdv, tv++);
		ASSERT_EQ(bar, dab);
	});
	auto h2 = daily.registerOnNewBarOpen([&tv, &odv, &daDto](const tsq_data& dto) {
		STDCOUTL(" da.OnNewBarOpen " << dto.to_string());
		if (odv < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(odv, tv++);
		ASSERT_EQ(dto, daDto);
	});

	auto h3 = m5.registerOnNewBarClose([&tv, &cm5v, &m5b](const tsohlcv& bar) {
		STDCOUTL("m5.OnNewBarClose " << bar.to_string());
		if (cm5v < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(cm5v, tv++);
		ASSERT_EQ(bar, m5b);
	});
	auto h4 = m5.registerOnNewBarOpen([&tv, &om5v, &m5Dto](const tsq_data& dto) {
		STDCOUTL(" m5.OnNewBarOpen " << dto.to_string());
		if (om5v < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(om5v, tv++);
		ASSERT_EQ(dto, m5Dto);
	});

	auto h5 = m1.registerOnNewBarClose([&tv, &cm1v, &m1b](const tsohlcv& bar) {
		STDCOUTL("m1.OnNewBarClose " << bar.to_string());
		if (cm1v < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(cm1v, tv++);
		ASSERT_EQ(bar, m1b);
	});
	auto h6 = m1.registerOnNewBarOpen([&tv, &om1v,&m1Dto](const tsq_data& dto) {
		STDCOUTL(" m1.OnNewBarOpen " << dto.to_string());
		if (om1v < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(om1v, tv++);
		ASSERT_EQ(dto, m1Dto);
	});

	auto h7 = tServ.registerOnNewBarClose([&tv, &csv, &tsb](const tsohlcv& bar) {
		STDCOUTL("tS.OnNewBarClose " << bar.to_string());
		if (csv < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(csv, tv++);
		ASSERT_EQ(bar, tsb);
	});
	auto h8 = tServ.registerOnNewBarOpen([&tv, &osv, &tsDto](const tsq_data& dto) {
		STDCOUTL(" tS.OnNewBarOpen " << dto.to_string());
		if (osv < 0) {
			GTEST_FAIL() << "Mustn't be here!";
		} else ASSERT_EQ(osv, tv++);
		ASSERT_EQ(dto, tsDto);
	});

	tsohlcv br(tag_milDT(), 20170103, 100200, 173.5200000, 173.8400000, 173.0000000, 173.5900000, 1);
	tsohlcv br2(tag_milDT(), 20170103, 100300, 172.00000, 173.800000, 172.0000000, 173.00000, 3);
	tsohlcv br3(tag_milDT(), 20170103, 100500, 173.100000, 173.9400000, 173.0000000, 173.2300000, 5);

	tsq_data dto = br;
	STDCOUTL("bar = " << br.to_string());

	osv = 0; om1v = 1; om5v = 2; odv = 3;
	csv = -1; cm1v = -1; cm5v = -1; cdv = -1;
	tsDto = dto; m1Dto = dto; m5Dto = tsq_data(dto.TFLowerBoundary(m5Tf),dto.q); daDto = tsq_data(dto.TFLowerBoundary(dailyTf), dto.q);
	tServ.newBarOpen(dto);
	
	osv = -1; om1v = -1; om5v = -1; odv = -1;
	csv = 4; cm1v = 5; cm5v = -1; cdv = -1;
	tsb = br; m1b = br;
	tServ.newBarAggregate(br);
	tServ._newBarOpen_pre(br2);//to fire onClose callbacks
	ASSERT_EQ(br, m1.lastBar());
	ASSERT_EQ(tsohlcv(br.TFLowerBoundary(m5Tf), br), m5.lastBar());
	ASSERT_EQ(tsohlcv(br.TFLowerBoundary(dailyTf), br), daily.lastBar());
	
	dto = br2;
	STDCOUTL("bar = " << br2.to_string()); tv = 0;
	osv = 0; om1v = 1; om5v = -1; odv = -1;
	csv = -1; cm1v = -1; cm5v = -1; cdv = -1;
	tsDto = dto; m1Dto = dto; m5Dto = tsq_data(dto.TFLowerBoundary(m5Tf), dto.q); daDto = tsq_data(dto.TFLowerBoundary(dailyTf), dto.q);
	tServ._newBarOpen_and_post(dto);//finalizing newBarOpen

	osv = -1; om1v = -1; om5v = -1; odv = -1;
	csv = 2; cm1v = 3; cm5v = 4; cdv = -1;
	tsb = br2; m1b = br2;
	tServ.newBarAggregate(br2);
	
	STDCOUTL("--- now making m5 to close");
	auto brt = br;	brt.aggregate(br2);
	m5b = tsohlcv(m5Dto.TS(), brt);

	dto = br3;
	tsDto = dto; m1Dto = dto; m5Dto = tsq_data(dto.TFLowerBoundary(m5Tf), dto.q); daDto = tsq_data(dto.TFLowerBoundary(dailyTf), dto.q);
	tServ._newBarOpen_pre(br3);
		
	ASSERT_EQ(br2, m1.lastBar());
	ASSERT_EQ(tsohlcv(brt.TFLowerBoundary(m5Tf), brt), m5.lastBar());
	ASSERT_EQ(tsohlcv(brt.TFLowerBoundary(dailyTf), brt), daily.lastBar());
		
	STDCOUTL("bar = " << br3.to_string()); tv = 0;
	osv = 0; om1v = 1; om5v = 2; odv = -1;
	csv = -1; cm1v = -1; cm5v = -1; cdv = -1;
	tServ._newBarOpen_and_post(dto);

	osv = -1; om1v = -1; om5v = -1; odv = -1;
	csv = 3; cm1v = 4; cm5v = 5; cdv = 6;
	tsb = br3; m1b = br3;
	tServ.newBarAggregate(br3);

	STDCOUTL("--- now forcing M5 & daily TF bars to close");
	brt.aggregate(br3);
	m5b = tsohlcv(m5Dto.TS(), br3);
	dab = tsohlcv(daDto.TS(), brt);

	br = tsohlcv(tag_milDT(), 20170204, 110300, 173.2200000, 173.8400000, 173.0000000, 173.800000, 7);
	tServ._newBarOpen_pre(br);

	ASSERT_EQ(br3, m1.lastBar());
	ASSERT_EQ(tsohlcv(br3.TFLowerBoundary(m5Tf), br3), m5.lastBar());
	ASSERT_EQ(tsohlcv(brt.TFLowerBoundary(dailyTf), brt), daily.lastBar());

	dto = br;
	STDCOUTL("bar = " << br.to_string()); tv = 0;
	osv = 0; om1v = 1; om5v = 2; odv = 3;
	csv = -1; cm1v = -1; cm5v = -1; cdv = -1;
	tsDto = dto; m1Dto = dto; m5Dto = tsq_data(dto.TFLowerBoundary(m5Tf), dto.q); daDto = tsq_data(dto.TFLowerBoundary(dailyTf), dto.q);
	tServ._newBarOpen_and_post(dto);
	
	osv = -1; om1v = -1; om5v = -1; odv = -1;
	csv = 4; cm1v = 5; cm5v = 6; cdv = 7;
	tsb = br; m1b = br;
	tServ.newBarAggregate(br);

	m5b = tsohlcv(m5Dto.TS(), br);
	dab = tsohlcv(daDto.TS(), br);

	tServ.notifyDateTime(br.plusYear());
	ASSERT_EQ(br, m1.lastBar());
	ASSERT_EQ(tsohlcv(br.TFLowerBoundary(m5Tf), br), m5.lastBar());
	ASSERT_EQ(tsohlcv(br.TFLowerBoundary(dailyTf), br), daily.lastBar());

}

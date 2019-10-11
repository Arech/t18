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

#include "../t18/algs/AlgsMap.h"

#include "algsRunner.h"

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

using namespace t18;
using namespace std::literals;

TEST(AlgsTests, Open2real_t) {
	using namespace hana::literals;
	
	algsRunner::run(AlgsMap
		, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn-D.csv"
		, TESTS_TESTDATA_DIR "test_"
		, hana::make_map(
			hana::make_pair("MA"_s, algPrms(Prm("len"_s, 11)))
			, hana::make_pair("MA"_s, algPrms(Prm("len"_s, 30)))
			, hana::make_pair("EMA"_s, algPrms(Prm("len"_s, 10)))
			, hana::make_pair("EMA"_s, algPrms(Prm("len"_s, 30)))
			, hana::make_pair("EMAsi"_s, algPrms(Prm("len"_s, 8)))
			, hana::make_pair("EMAsi"_s, algPrms(Prm("len"_s, 37)))
			, hana::make_pair("DEMA"_s, algPrms(Prm("len"_s, 20)))
			, hana::make_pair("TEMA"_s, algPrms(Prm("len"_s, 20)))
			, hana::make_pair("MovMin"_s, algPrms(Prm("len"_s, 20)))
			, hana::make_pair("MovMin"_s, algPrms(Prm("len"_s, 1)))
			, hana::make_pair("MovMin"_s, algPrms(Prm("len"_s, 11)))
			, hana::make_pair("MovMax"_s, algPrms(Prm("len"_s, 20)))
			, hana::make_pair("Elementile"_s, algPrms(Prm("len"_s, 101), Prm("rank"_s, 10)))
			, hana::make_pair("Elementile"_s, algPrms(Prm("len"_s, 11), Prm("rank"_s, 9)))
			, hana::make_pair("Percentile"_s, algPrms(Prm("len"_s, 11), Prm("percV"_s, 15)))
			, hana::make_pair("Percentile"_s, algPrms(Prm("len"_s, 27), Prm("percV"_s, 81)))

			, hana::make_pair("PercentRank"_s, algPrms(Prm("len"_s, 24)))
			, hana::make_pair("PercentRank"_s, algPrms(Prm("len"_s, 197)))
			, hana::make_pair("PercentRank"_s, algPrms(Prm("len"_s, 500)))
		)
		, [](auto&& prms, auto&& algT) 
	{
		typedef typename decltype(+algT)::type AlgT;
		const auto minHist = AlgT::minSrcHist(prms);
		//return ::std::make_unique<publicIntf_timeframeServer>(minHist, ::std::make_unique<tfConverter::tfConvBase<>>(1));
		return ::std::make_unique<publicIntf_timeframeServer< tfConverter::tfConvBase<tsohlcv> >>(minHist, 1);
	}
		, [](auto& ts, auto&& prms, auto&& algT)
	{
		typedef typename decltype(+algT)::type AlgT;
		return ::std::make_unique<AlgT>(ts._getTs("close"_s), ts.getTs("open"_s), prms);
	}
	);
}

#include "publicIntf_tickerServer.h"
#include "../t18/tfConverter/dailyhm.h"

class DualTsSubs {
public:
	typedef decltype("base"_s) btf_ht;
	typedef decltype("higher"_s) htf_ht;

	typedef timeseries::Timeframe<tfConverter::baseOhlc> tfBase_t;
	typedef timeseries::Timeframe<tfConverter::dailyhmOhlc> tfHigher_t;

	publicIntf_tickerServer<decltype(hana::make_map(utils::Descr_v<btf_ht, tfBase_t>, utils::Descr_v<htf_ht, tfHigher_t>))> m_ticker;
	tfBase_t& m_baseTf;
	tfHigher_t& m_higherTf;


public:
	template<typename HMT>
	DualTsSubs(real_t minD, int highTf, HMT&& prms) 
		: m_ticker(TickerId::forEveryone() //used as dummy tickerId here.
			, "DualTsSubs", minD, hana::make_map(
				hana::make_pair(btf_ht(), hana::make_tuple(size_t(ceil(real_t(highTf) / real_t(1))*prms["len"_s]), 1 /*, baseTF*/))
				, hana::make_pair(htf_ht(), hana::make_tuple(1u, /*baseTF,*/ highTf)) 
			)
		)
		, m_baseTf(m_ticker.getTf<btf_ht>())
		//, m_higherTf(m_ticker.newTf(1u, ::std::make_unique<tfConverter::dailyhm<>>(baseTF, highTf)))
		//, m_higherTf(m_ticker.newTf(1u, tfConverter::dailyhm<tsohlcv>(baseTF, highTf)))
		, m_higherTf(m_ticker.getTf<htf_ht>())
	{}

	template<typename T>
	decltype(auto) registerOnNewBarOpen(T&& t) {
		return m_higherTf.registerOnNewBarOpen(::std::forward<T>(t));
	}
	template<typename T>
	decltype(auto) registerOnNewBarClose(T&& t) {
		return m_higherTf.registerOnNewBarClose(::std::forward<T>(t));
	}

	//auto lastDate()const { return m_higherTf.lastDate(); }
	//auto lastTime()const { return m_higherTf.lastTime(); }
	auto lastTimestamp()const { return m_higherTf.lastTimestamp(); }
	auto lastClose()const { return m_higherTf.lastClose(); }
	auto TotalBars()const { return m_higherTf.TotalBars(); }
	auto TF()const { return m_higherTf.TF(); }

	//////////////////////////////////////////////////////////////////////////
	void notifyDateTime(const ::t18::mxTimestamp& dt) {
		m_ticker.notifyDateTime(dt);
	}
	void notifyDateTime(::t18::mxDate d, ::t18::mxTime t) {
		notifyDateTime(::t18::mxTimestamp(d, t));
	}
	void notifyDateTime(milDate dMil, milTime tMil) { notifyDateTime(mxTimestamp(dMil, tMil)); }

	void newBarOpen(const ::t18::tsq_data& dto) {
		m_ticker.newBarOpen(dto);
	}
	void newBarOpen(milDate dMil, milTime tMil, real_t o) { newBarOpen(tsq_data(dMil, tMil, o)); }
	void newBarOpen(mxDate d, mxTime t, real_t o) { newBarOpen(tsq_data(d, t, o)); }

	void newBarAggregate(const ::t18::tsohlcv& bar) {
		m_ticker.newBarAggregate(bar);
	}
	void newBarAggregate(milDate dMil, milTime tMil, real_t o, real_t h, real_t l, real_t c, volume_t v) {
		newBarAggregate(tsohlcv(dMil, tMil, o, h, l, c, v));
	}
	void newBarAggregate(mxDate d, mxTime t, real_t o, real_t h, real_t l, real_t c, volume_t v) {
		newBarAggregate(tsohlcv( d, t, o, h, l, c, v));
	}
};

TEST(AlgsTests, LTFPercentileTest) {
	using namespace hana::literals;
	algsRunner::run(AlgsMap
		, TESTS_TESTDATA_DIR "GAZP_060101_170101-trn.csv"
		, TESTS_TESTDATA_DIR "test_"
		, hana::make_map(
			hana::make_pair("LTFPercentile"_s, algPrms(Prm("len"_s, 1), Prm("percV"_s, .01), Prm("bInvPercV"_s, true)))
			, hana::make_pair("LTFPercentile"_s, algPrms(Prm("len"_s, 1), Prm("percV"_s, 0.05), Prm("bInvPercV"_s, true)))
		)
		, [](auto&& prms, auto&&)
	{
		//typedef typename decltype(+algT)::type AlgT;
		//const auto minHist = AlgT::minSrcHist(prms);
		//return ::std::make_unique<publicIntf_timeframeServer>(minHist, ::std::make_unique<tfConverter::tfConvBase<>>(1));
		return ::std::make_unique<DualTsSubs>(real_t(.01), mxTime::minutesInDay, prms);
	}
		, [](auto& ts, auto&& prms, auto&& algT)
	{
		typedef typename decltype(+algT)::type AlgT;
		//return ::std::make_unique<AlgT>(ts.m_higherTf._getTs("close"_s), ts.m_baseTf, "open"_s, prms);

		typedef decltype(ts.m_higherTf.getTs("close"_s)) orig_t;
		return ::std::make_unique<AlgT>( const_cast<::std::decay_t<orig_t>&>(ts.m_higherTf.getTs("close"_s)), ts.m_baseTf, "high"_s, prms);
	}
	);
}
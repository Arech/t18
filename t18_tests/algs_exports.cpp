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

#if defined(T18_TESTS_DO_EXPORTS) && T18_TESTS_DO_EXPORTS

#include "../t18/exec/backtester.h"
#include "../t18/ts/MaCrossO.h"
#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"

using namespace t18;
using namespace std::literals;

template<typename MaCrossT = ts::MaCrossO<ts::MaCrossCompiletimePrms>>
struct MaCrossAlgExporterSetup : public ts::MaCrossOSetup<MaCrossT> {
private:
	typedef ts::MaCrossOSetup<MaCrossT> base_class_t;
public:
	template<typename ExecT>
	void setup(ExecT& exe) {
		if (!base_class_t::maFast) base_class_t::maFast = 10;
		//redefining the ticker for faster execution
		base_class_t::pTickerName = "GAZP_060101_170101-trn-D";
		base_class_t::tickerLotSize = 1u;
		base_class_t::tickerMinPriceDelta = real_t(.01);
		base_class_t::_do_setup(exe);
	}
};

struct ModifMaCrossO : public ts::MaCrossOCompiletimePrms {
	static constexpr int stopLossPerc = 10;
	static constexpr int takeProfitPerc = 10;
};

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

//////////////////////////////////////////////////////////////////////////
TEST(AlgsExports, MA) {
	MaCrossAlgExporterSetup<ts::MaCrossO<>> so;
	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;

	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	auto feed{ feeder::make_singleFile(feeder::adapters::csv_tsohlcv{}, TESTS_TESTDATA_DIR) };

	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpMA.csv";
	bt.silence().run(so, feed);
}
//////////////////////////////////////////////////////////////////////////
#include "../t18/algs/ema.h"
struct MaCrossO_Ema : public ModifMaCrossO {
	typedef algs::EMA algMaSlow_t;
	typedef algs::EMAsi algMaFast_t;
};

TEST(AlgsExports, EMA) {
	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_Ema>> so;
	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;
	
	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	auto feed{ feeder::make_singleFile(feeder::adapters::csv_tsohlcv{}, TESTS_TESTDATA_DIR) };

	so.maFast = 8;
	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpEMA.csv";
	bt.silence().run(so, feed);
}
//////////////////////////////////////////////////////////////////////////
#include "../t18/algs/dema.h"
#include "../t18/algs/tema.h"
struct MaCrossO_DTEma : public ModifMaCrossO {
	typedef algs::DEMA algMaSlow_t;
	typedef algs::TEMA algMaFast_t;
};

TEST(AlgsExports, DEMATEMA) {
	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_DTEma>> so;
	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;

	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	auto feed{ feeder::make_singleFile(feeder::adapters::csv_tsohlcv{}, TESTS_TESTDATA_DIR) };

	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpDEMATEMA.csv";
	bt.silence().run(so, feed);
}
//////////////////////////////////////////////////////////////////////////
#include "../t18/algs/BoostAcc.h"
struct MaCrossO_MinMax : public ModifMaCrossO {
	typedef algs::MovMax algMaSlow_t;
	typedef algs::MovMin algMaFast_t;
};
TEST(AlgsExports, MinMax) {
	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_MinMax>> so;
	exec::backtester<typename decltype(so)::ct_prms_t::TickersSet_t> bt;
	
	//feeder::theCsvDtohlcv feed(TESTS_TESTDATA_DIR);
	auto feed{ feeder::make_singleFile(feeder::adapters::csv_tsohlcv{}, TESTS_TESTDATA_DIR) };

	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpMinMax.csv";
	bt.silence().run(so, feed);
}
//////////////////////////////////////////////////////////////////////////
/*
#include "../t18/algs/elementile.h"
struct MaCrossO_ElmMed : public ModifMaCrossO {
	typedef algs::ElementileMedian algMaSlow_t;
	typedef algs::ElementileMedian algMaFast_t;
	static constexpr size_t maSlow = 31;
};
TEST(AlgsExports, ElementileMedian) {
	exec::backtester bt(TESTS_SRC_TICKERS_DIR);

	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_ElmMed>> so;
	feeder::CsvDtohlcv feed;
	so.maFast = 11;
	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpElementileMedian.csv";
	bt.silence().run(so, feed);
}
//////////////////////////////////////////////////////////////////////////
struct MaCrossO_ElmN : public ModifMaCrossO {
	typedef algs::ElementileN<10> algMaSlow_t;//10th percentile for len 101
	typedef algs::ElementileN<9> algMaFast_t;//90th percentile for len 11
	static constexpr size_t maSlow = 101;
};
TEST(AlgsExports, ElementileN) {
	exec::backtester bt(TESTS_SRC_TICKERS_DIR);

	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_ElmN>> so;
	feeder::CsvDtohlcv feed;
	so.maFast = 11;
	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpElementileN.csv";
	bt.silence().run(so, feed);
}

//////////////////////////////////////////////////////////////////////////
#include "../t18/algs/percentile.h"
struct MaCrossO_Perc : public ModifMaCrossO {
	typedef algs::PercentileN<50> algMaSlow_t;//median
	typedef algs::PercentileN<15> algMaFast_t;
	static constexpr size_t maSlow = 31;
};
TEST(AlgsExports, Percentiles) {
	exec::backtester bt(TESTS_SRC_TICKERS_DIR);

	MaCrossAlgExporterSetup<ts::MaCrossO<MaCrossO_Perc>> so;
	feeder::CsvDtohlcv feed;
	so.maFast = 11;
	so.pDbgFile = TESTS_TESTDATA_DIR "gen_MaCrossO_dumpPercentile.csv";
	bt.silence().run(so, feed);
}
*/
#endif // T18_TESTS_DO_EXPORTS


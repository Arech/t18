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

#include "publicIntf_timeframeServer.h"
#include "../t18/feeder/adapters/csv.h"
#include "../t18/feeder/singleFile.h"
#include "../t18/feeder/memory.h"

struct algsRunner {
	//#WARNING the code relies on specific order of folding parameter maps, which is not guaranteed!!!

	template<typename AlgsMapT, typename AlgsHMT, typename FmakeTs, typename FmakeAlg>
	static void run(const AlgsMapT& algsMap, const char* tickerData, const char* checkFilesPfx, const AlgsHMT& AlgsNPrms
		, FmakeTs&& mkTs, FmakeAlg&& mkAlg, const real_t tol = real_t(1e-6))
	{
		using namespace ::t18;
		
		//feeder::memory<tsohlcv> f((feeder::theCsvDtohlcv(tickerData)));

		auto f{ feeder::memory<tsohlcv>::make<feeder::singleFile, feeder::adapters::csv_tsohlcv>(
			hana::make_tuple(tickerData), hana::make_tuple()
			) };

		hana::for_each(AlgsNPrms, [/*&algsMap, */&f, checkFilesPfx, &mkTs, &mkAlg, &tol](auto&& pr) {
			const auto alName = hana::first(pr);
			typedef typename decltype(+algsMap[alName])::type AlgT;

			auto pTs = mkTs(hana::second(pr), hana::type_c<AlgT>);

			//going to use close as destination
			::std::unique_ptr<AlgT> pAlg = mkAlg(*pTs.get(), hana::second(pr), hana::type_c<AlgT>);

			utils::myFile hF;
			::std::string sfx, fname;

			sfx = alName.c_str();
			sfx += prms_to_string<typename AlgT::algPrmsDescr_t>(hana::second(pr));

			fname = checkFilesPfx;
			fname += sfx;
			fname += ".csv";

			try {
				hF.open(fname.c_str(), "r");
			} catch (const ::std::exception&) {}

			if (!hF.isOpened()) {
				STDCOUTL("No datafile for " << sfx << ", trying to use actual params...");

				sfx = alName.c_str();
				sfx += prms_to_string<typename AlgT::algPrmsDescr_t>(pAlg->getPrms());

				fname = checkFilesPfx;
				fname += sfx;
				fname += ".csv";
				try {
					hF.open(fname.c_str(), "r");
				} catch (const ::std::exception& e) {
					STDCOUTL("Also no datafile for " << sfx << ", failing..");
					FAIL() << e.what();
				}
			}

			STDCOUT("Testing " << sfx << " ... ");
			bool bGotSrcData = false;
			real_t maxErr = ::std::numeric_limits<real_t>::lowest();

			auto rh2 = pTs->registerOnNewBarOpen([&algObj = *pAlg](const tsq_data&) {
				algObj.notifyNewBarOpened();
			});

			auto rh = pTs->registerOnNewBarClose([&algObj = *pAlg, &ts = *pTs, &sfx, pF = hF.get(), &bGotSrcData, &tol, &maxErr](const tsohlcv&) {
				algObj(true);

				tsq_data dtq;
				int nRead;
				if (!feeder::adapters::csv_tsq::readNext(pF, dtq, nRead)) {
					T18_ASSERT(!"Failed to read line!");
					throw ::std::runtime_error("Failed to read line!");
				}

				ASSERT_EQ(ts.lastTimestamp(), dtq.TS().TFLowerBoundary(ts.TF()));

				real_t q = ts.lastClose();
				if (isnan(dtq.q)) {
					ASSERT_TRUE(!bGotSrcData || isnan(q));
				} else {
					bGotSrcData = true;
					//constexpr real_t tol = 1e-6;
					const auto er = ::std::abs(q - dtq.q) / ::std::min(::std::abs(q), ::std::abs(dtq.q));
					maxErr = ::std::max(maxErr, er);
					if (er >= tol) {
						char buf[400];
						sprintf_s(buf, "Algorithm %s: Bar #%lu DTQ=%s is invalid.\nCalculated value = %.8f\n"
							"  Expected value = %.8f\n abs difference is %.8f\n rel difference is %.8f (tolerance %.8f)\n"
							, sfx.c_str(), ts.TotalBars(), dtq.to_string().c_str(), q, dtq.q, ::std::abs(q - dtq.q), er, tol);
						throw ::std::runtime_error(::std::string(buf));
					}
				}
			});

			try {
				f(*pTs);
				STDCOUTL("ok! Biggest rel error = " << maxErr);
			} catch (const ::std::exception& e) {
				FAIL() << e.what();
			}
		});
	}

};
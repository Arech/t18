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

/*
Basically the idea behind feeder::adapters concept is simple - adapter is a class that 
knows exactly how to extract all the necessary data from the data source and populate data structure such tsohlcv or tsTick.
The adapter class has fixed interface.
*/

#include "../../base.h"
#include "../../utils/myFile.h"
#include "../../market/tickerBase.h"

namespace t18 {
	namespace feeder {
		namespace adapters {

			namespace _i {

				struct _csv_adapters_base {
				public:
					static size_t capacity(const char* fname) noexcept{
						utils::myFile myF(fname, "r");
						size_t r = 0;
						int ch;
						while (EOF != (ch = getc(myF)))
							if ('\n' == ch)
								++r;
						return r;
					}

					static void _setBestGuessBidAsk(tickerBase& Tickr, const tsq_data& tso) {
						Tickr.setBestBidAsk(tickerBase::bestPriceInfo_t(tso.TS(), tso.q, ::std::numeric_limits<volume_t>::epsilon())
							, tickerBase::bestPriceInfo_t(tso.TS(), tso.q + Tickr.getMinPriceDelta(), ::std::numeric_limits<volume_t>::epsilon()));
					}
				};

				template<typename T> struct tsohlcv;
				template<> struct tsohlcv<float> {
					static constexpr auto fmt = "%lu,%lu,%f,%f,%f,%f,%lf"_s;
				};
				template<> struct tsohlcv<double> {
					static constexpr auto fmt = "%lu,%lu,%lf,%lf,%lf,%lf,%lf"_s;
				};
			}

			//all the public field here are mandatory for each adapter.
			//Adapter class may have non static member variables and functions and always called in a non-static manner
			struct csv_tsohlcv : public _i::_csv_adapters_base {
				typedef tsohlcv value_t;
			private:
				typedef _i::_csv_adapters_base base_class_t;

			protected:
				static constexpr auto csvFormat = _i::tsohlcv<decltype(tsohlcv::h)>::fmt;
				static constexpr int numOfCsvElements = 7;

			public:
				//csv_tsohlcv() : base_class_t() {}

				//"static" as well as "noexcept" is not mandatory here
				static bool readNext(FILE* hF, value_t& val, int& nRead)noexcept {
					date_ult d;
					time_ult t;

					nRead = fscanf_s(hF, csvFormat.c_str(), &d, &t, &val.o(), &val.h, &val.l, &val.c, &val.v);
					const bool bRead = (nRead == numOfCsvElements);
					if (LIKELY(bRead)) {
						val.TS() = mxTimestamp(tag_milDT(), d, t);
					}
					return bRead;
				}

				template<typename MdssT, typename TickerServT>
				static void updateMkt(MdssT& Mkt, TickerServT& Tickr, const value_t& val)noexcept {
					//don't need this asserts here
					//static_assert(utils::has_tag_t_v<tag_MarketDataStorServ_t, MdssT>, "");
					//static_assert(utils::is_tag_of_v<tag_Ticker_t, TickerServT>, "");
					//base_class_t::_updateMkt(Mkt, val);

					Mkt.newBarOpen(Tickr, val.TSQ());
					Mkt.newBarAggregate(Tickr, val);
				}
			};

			//////////////////////////////////////////////////////////////////////////

			namespace _i {
				template<typename T> struct tsDirDeal;
				template<> struct tsDirDeal<float> {
					static constexpr auto fmt = "%lu,%lu,%f,%lf,%lu,%ld"_s;
				};
				template<> struct tsDirDeal<double> {
					static constexpr auto fmt = "%lu,%lu,%lf,%lf,%lu,%ld"_s;
				};

				/*template<typename T> struct tsDirDeal;
				template<> struct tsDirDeal<float> {
					static constexpr auto fmt = "%lu,%lu,%f,%f,%f,%f,%lf"_s;
				};
				template<> struct tsDirDeal<double> {
					static constexpr auto fmt = "%lu,%lu,%lf,%lf,%lf,%lf,%lf"_s;
				};*/
			}

			struct csv_tsTick : public _i::_csv_adapters_base {
				typedef tsTick value_t;
			private:
				typedef _i::_csv_adapters_base base_class_t;

			protected:
				static constexpr auto csvFormat = _i::tsDirDeal<decltype(tsTick::q)>::fmt;
				static constexpr int numOfCsvElements = 6;

			public:
				//csv_tsTick() : base_class_t() {}

				//"static" as well as "noexcept" is not mandatory here
				static bool readNext(FILE* hF, value_t& val, int& nRead)noexcept {
					date_ult d;
					time_ult t;

					::std::uint32_t dealNum;
					::std::int32_t bIsLong;

					nRead = fscanf_s(hF, csvFormat.c_str(), &d, &t, &val.q, &val.v, &dealNum, &bIsLong);
					const bool bRead = (nRead == numOfCsvElements);
					if (LIKELY(bRead)) {
						val.TS() = mxTimestamp(tag_milDT(), d, t);
					}
					return bRead;
				}

				template<typename MdssT, typename TickerServT>
				static void updateMkt(MdssT& Mkt, TickerServT& Tickr, const value_t& val) noexcept{
					//don't need this asserts here
					//static_assert(utils::has_tag_t_v<tag_MarketDataStorServ_t, MdssT>, "");
					//static_assert(utils::is_tag_of_v<tag_Ticker_t, TickerServT>, "");
					//base_class_t::_updateMkt(Mkt, val);

					Mkt.newTick(Tickr, val);
				}
			};

			//////////////////////////////////////////////////////////////////////////


			namespace _i {
				template<typename T> struct tsq;
				template<> struct tsq<float> {
					static constexpr auto fmt = "%lu,%lu,%f"_s;
				};
				template<> struct tsq<double> {
					static constexpr auto fmt = "%lu,%lu,%lf"_s;
				};
			}

			//#WARNING csv_tsq is not a full-featured adapter. It's kind-a helper that only reads data
			struct csv_tsq {
				typedef tsq_data value_t;

			protected:
				static constexpr auto csvFormat = _i::tsq<decltype(tsq_data::q)>::fmt;
				static constexpr int numOfCsvElements = 3;

			public:
				//"static" as well as "noexcept" is not mandatory here
				static bool readNext(FILE* hF, value_t& val, int& nRead)noexcept {
					date_ult d;
					time_ult t;

					nRead = fscanf_s(hF, csvFormat.c_str(), &d, &t, &val.q);
					const bool bRead = (nRead == numOfCsvElements);
					if (LIKELY(bRead)) {
						val.TS() = mxTimestamp(tag_milDT(), d, t);
					}
					return bRead;
				}
			};

		}
	}
}

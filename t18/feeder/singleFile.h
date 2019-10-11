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

//#include "tickerUpdater.h"

#include "../tags.h"

namespace t18 {
	namespace feeder {

		struct dummyMkt {
			template<typename TickerT>
			static void notifyDateTime(TickerT&&, mxTimestamp) {}
		};

		template<typename AdaptT>
		struct singleFile {
		public:
			typedef AdaptT adapter_t;
			typedef typename adapter_t::value_t value_t;
			
		protected:
			adapter_t m_adapter;
			const char* m_fname = nullptr;

		public:
			singleFile(const char* f) :m_fname(f) {}

			template<typename Adp, typename = ::std::enable_if_t<::std::is_same_v<adapter_t, ::std::decay_t<Adp>>>>
			singleFile(Adp&& a, const char* f) : m_adapter(::std::forward<Adp>(a)), m_fname(f) {}

			//////////////////////////////////////////////////////////////////////////

			//to be called by backtester
			template<typename MdssT>
			::std::enable_if_t<utils::has_tag_t_v<tag_MarketDataStorServ_t, MdssT>> operator()(MdssT& mkt)
			{
				if (mkt.tickersCount() != 1) {
					T18_ASSERT(!"singleFile class supports feeding into only 1 ticker");
					throw ::std::logic_error("singleFile class supports feeding into only 1 ticker");
				}
				mkt.forEachTicker([&mkt, _fn = m_fname, &adp = m_adapter](auto& tickr) {
					typedef ::std::decay_t<decltype(tickr)> ticker_t;
					static_assert(utils::has_tag_t_v<tag_Ticker_t, ticker_t>, "");

					::std::string fn(::std::string(_fn) + "/" + tickr.Name() + ".csv");
					processFile(mkt, tickr, fn.c_str(), adp);
				});
			}

			size_t capacity()const {
				return m_adapter.capacity(m_fname);
			}

			void processByAdapter() {
				processFile(dummyMkt(), 0, m_fname, m_adapter);
			}

			template<typename MdssT, typename TickerServT, typename AdptT, typename = ::std::enable_if_t<::std::is_same_v<::std::decay_t<AdptT>, adapter_t>>>
			static void processFile(MdssT&& mkt, TickerServT&& tickr, const char* fname, AdptT&& adpt) {
				utils::myFile myF(fname, "r");

				size_t nLines = 0;
				value_t val;
				int nRead;

				while (adpt.readNext(myF, val, nRead)) {
					++nLines;
					adpt.updateMkt(mkt, tickr, val);
				}
				if (feof(myF)) {
					if (EOF != nRead) {
						T18_ASSERT(!"Invalid end of file!");
						throw ::std::runtime_error("Invalid end of file, after "s + ::std::to_string(nLines) + " read "
							+ ::std::to_string(nRead) + " !=0 elements");
					}
				} else {
					T18_ASSERT(!"Failed to read csv line");
					throw ::std::runtime_error("Failed to read csv line #"s + ::std::to_string(nLines + 1) + ". Read only "
						+ ::std::to_string(nRead) + " elements");
				}

				//doing notify to make sure that all higher-level timeframes are closed 
				mkt.notifyDateTime(tickr, val.TS().plusYear());
			}

		};

		template<typename AdaptT>
		inline decltype(auto) make_singleFile(AdaptT&& a, const char* f) {
			return singleFile<AdaptT>(::std::forward<AdaptT>(a), f);
		}
	}
}

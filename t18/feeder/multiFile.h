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

// this is a bit proper implementation of feeder concept, that represents more proper way to extract tsohlcv data
// from a (csv-files based) library

#include "../base.h"
#include "../utils/myFile.h"
//#include "../market/MarketDataStorServ.h"
#include "../market/tickerBase.h"
#include "adapters/csv.h"

namespace t18 {
	namespace feeder {

		//to minimize memory usage this class read files and passes the data to backtester live
		//#TODO However for the optimization or any mass testing purposes the data should be cached at memory first.
		//template parameter Ticker2AdapterHMT is a type of hana::map that maps ticker type (or void as default)
		// to specific feeder::adapter that is used to populate that ticker.
		template<typename Ticker2AdapterHMT = utils::makeMap_t<decltype(hana::make_pair(hana::type_c<void>, hana::type_c<adapters::csv_tsohlcv>))>>
		class multiFile {
			static_assert(hana::is_a<hana::map_tag, Ticker2AdapterHMT>, "");
			static_assert(hana::contains(Ticker2AdapterHMT(), hana::type_c<void>), "Ticker2AdapterHMT must contain hana::type_c<void> key as a default key");

			typedef multiFile<Ticker2AdapterHMT> self_t;

			template<typename T>
			static constexpr auto _makeTicker2AdapterMap(T const& map) {
				return hana::fold_left(map, hana::make_map(), [](auto&& tsmap, auto&& pair) {
					typedef decltype(+hana::first(pair)) key_t;
					static_assert(hana::is_a<hana::type_tag, key_t>, "Key must be a hana::type_c instance");

					typedef typename key_t::type tickerOrVoid_t;
					static_assert(::std::is_void_v<tickerOrVoid_t> || utils::has_tag_t_v<tag_Ticker_t, tickerOrVoid_t>, "");

					auto t = hana::second(pair);//auto drops references and top level cv-qualifiers
					typedef decltype(+t) val_t;
					static_assert(hana::is_a<hana::type_tag, val_t>, "Value must be a hana::type_c instance");
					

					return hana::insert(::std::forward<decltype(tsmap)>(tsmap)
						, hana::make_pair(hana::first(pair), typename val_t::type ()));
				});
			}

		public:
			typedef Ticker2AdapterHMT ticker2adapter_descr_hmt;
			typedef decltype(_makeTicker2AdapterMap(ticker2adapter_descr_hmt())) ticker2adapter_hmt;

		protected:
			const char* m_libPath = nullptr;
			ticker2adapter_hmt m_adapterMap;

		protected:
			template<typename T>
			auto& _getAdapter()noexcept {
				return hana::find(m_adapterMap, hana::type_c<T>).value_or(m_adapterMap[hana::type_c<void>]);
			}

		public:
			~multiFile()noexcept {}
			multiFile()noexcept {}
			multiFile(const char* pLib)noexcept : m_libPath(pLib) {
				T18_ASSERT(pLib);
			}
			template<typename HMT, typename = ::std::enable_if_t<::std::is_same_v<ticker2adapter_hmt, ::std::decay_t<HMT>>>>
			multiFile(const char* pLib, HMT&& hmt)noexcept : m_libPath(pLib), m_adapterMap(::std::forward<HMT>(hmt)){
				T18_ASSERT(pLib);
			}

			void setLibPath(const char* pLib) noexcept {
				T18_ASSERT(pLib);
				m_libPath = pLib;
			}

			//////////////////////////////////////////////////////////////////////////
		protected:

			inline static constexpr mxTimestamp invalidTs = mxTimestamp::max();
			
			//BarT is taken from corresponding feeder::adapter type, because the bar_t member of ticker is taken from the 
			//first timeframe and it may differ from the data read from file (timeframe could be operating on tsohlcv, while file
			// could contain simple tsTick-s)
			// tickerCtx exists for every ticker
			template<typename BarT>
			struct tickerCtx {
				typedef BarT bar_t;

				utils::myFile hF;

				bar_t bar = bar_t(invalidTs);
				size_t nLinesRead = 0;

				//TickerId tiid;

				//tickerCtx()noexcept : tiid(TickerId::forEveryone()) {}
			};

			//the data type to help synchronize time flow of different files
			struct readingCtx {
				mxTimestamp earliestTs;
				TickerId tiidOfEarliestTs;

				readingCtx()noexcept : earliestTs(invalidTs), tiidOfEarliestTs(TickerId::forEveryone()) {}
				void reset()noexcept {
					earliestTs = invalidTs;
					//tiidOfEarliestDt = invalidTiid;
				}
				constexpr bool isValid()const noexcept {
					//T18_ASSERT(!( (earliestDt == invalidDt) ^ (tiidOfEarliestDt==invalidTiid)));
					return earliestTs != invalidTs;
				}
			};

			template<typename AdptT>
			static void sReadBar(AdptT&& adpt, tickerCtx<typename ::std::decay_t<AdptT>::value_t>& ctx, readingCtx& rctx, tickerBase& Tickr) {
				T18_ASSERT(ctx.hF.isOpened());// && Tickr.getTickerId() == ctx.tiid);
				
				int nRead;

				++ctx.nLinesRead;
				if (adpt.readNext(ctx.hF, ctx.bar, nRead)) {
					//setting the bid/ask spread to bar opening data (we can't make a better guess here) in advance
					//this would make more realistic trade entry points should we trade using different tickers
					adpt._setBestGuessBidAsk(Tickr, ctx.bar.TSQ());

					if (ctx.bar.TS() < rctx.earliestTs) {
						rctx.earliestTs = ctx.bar.TS();
						rctx.tiidOfEarliestTs = Tickr.getTickerId();
					}
				} else {
					bool isEOF = feof(ctx.hF);
					ctx.hF.close();
					if (LIKELY(isEOF)) {
						if (UNLIKELY(EOF != nRead)) {
							T18_ASSERT(!"Invalid end of file!");
							throw ::std::runtime_error("Invalid end of csv for ticker "s + Tickr.Name()
								+ " at line#" + ::std::to_string(ctx.nLinesRead) + " read "
								+ ::std::to_string(nRead) + " !=0 elements");
						}
					} else {
						T18_ASSERT(!"Failed to read csv line");
						throw ::std::runtime_error("Failed to read csv for ticker "s + Tickr.Name()
							+ " line #"s + ::std::to_string(ctx.nLinesRead) + ". Read only "
							+ ::std::to_string(nRead) + "!=tsohlcv::numOfCsvElements elements");
					}
				}
			}

		private:
			template<typename BarT>
			using singleTickerTypeCtx_tpl = ::std::vector<tickerCtx<BarT>>;

			template<typename HTT, typename = ::std::enable_if_t<utils::isDescrTuple_v<HTT>>>
			static constexpr auto makeCtxStor(HTT const& tup) {
				return hana::transform(tup, [](auto&& e) {
					typedef typename decltype(+e)::type ticker_t;
					static_assert(::std::is_same_v<typename ticker_t::tag_t, tag_Ticker_t>, "TickersSetHST must contain set of Ticker types");

					typedef ::std::decay_t<decltype(
						hana::find(::std::declval<ticker2adapter_hmt>(), hana::type_c<ticker_t>)
						.value_or(hana::at_key(::std::declval<ticker2adapter_hmt>(), hana::type_c<void>))
						)> adpt_t;

					//return ::std::declval<singleTickerTypeCtx_tpl<typename ticker_t::bar_t>>();
					return ::std::declval<singleTickerTypeCtx_tpl<typename adpt_t::value_t>>();
				});
			}
			
			template<typename AdptT> //, typename TickerBaseT, typename = ::std::enable_if_t<utils::is_tag_of_v<tag_Ticker_t, TickerBaseT>>>
			static void _sInitCtxs(AdptT&& adpt, singleTickerTypeCtx_tpl<typename ::std::decay_t<AdptT>::value_t>& vCtx
				, readingCtx& rctx, const ::std::string& libPath, tickerBase& tickr)
			{
				auto tickerIdx = tickr.getTickerId().getIdx();
				T18_ASSERT(tickerIdx < vCtx.size());

				auto& ctx = vCtx[tickerIdx];
				//ctx.tiid = tickr.getTickerId();

				const ::std::string fname(libPath + tickr.Name() + ".csv");
				if (!ctx.hF.open(fname.c_str(), "r")) {
					T18_ASSERT(!"Failed to open csv file");
					throw ::std::runtime_error("Failed to open csv file, path="s + fname);
				}

				sReadBar(adpt, ctx, rctx, tickr);
				if (UNLIKELY(ctx.bar.TS() == invalidTs)) {
					T18_ASSERT(!"Failed to read a first bar from csv file");
					throw ::std::runtime_error("Failed to read first bar, path="s + fname);
				}
			}

			template<typename BarT>
			static void _sUpdateCtxs(singleTickerTypeCtx_tpl<BarT>& vCtx, readingCtx& rctx, const tickerBase& tickr) {
				const auto tiid = tickr.getTickerId();
				const auto& ctxBarDt = vCtx[tiid.getIdx()].bar.TS();
				if (ctxBarDt < rctx.earliestTs) {
					rctx.earliestTs = ctxBarDt;
					rctx.tiidOfEarliestTs = tiid;
				}
			}

		public:
			//called by the backtester to start data feeding process
			template<typename MdssT, typename = ::std::enable_if_t<utils::is_tag_of_v<tag_MarketDataStorServ_t, MdssT>>>
			void operator()(MdssT& m) {
				typedef decltype(makeCtxStor(typename MdssT::TickersTuple_t())) CtxStor_t;

				CtxStor_t ctxStor;
				//0.resizing ctx's to correct size
				const auto tickersCnt = m.tickersCountByType();
				static_assert(hana::size(ctxStor) == hana::size(tickersCnt), "");

				hana::for_each(hana::to_tuple(hana::range_c<size_t, 0, MdssT::TickersTupleSize_v>)
					, [&ctxStor, &tickersCnt](auto&& typeIdIdx)
				{
					//resizing
					auto& vCtx = hana::at(ctxStor, typeIdIdx);
					auto cnt = hana::at(tickersCnt, typeIdIdx);
					vCtx.resize(cnt);
					//we mustn't assume any method of generation of getTickerId's here, so context initialization should be done in
					//another, MdssT controlled loop (well, we still use some assumptions here&there, but let's not multiply them)
				});

				//1. opening files & initializing contexts
				readingCtx rctx;
				const ::std::string libPath(::std::string(m_libPath) + "/");
				m.forEachTickerIndexed([&ctxStor, &rctx, &libPath, ths=this](auto& tickr, auto&& typeIdIdx) {
					//typedef ::std::decay_t<decltype(tickr)> ticker_t;
					T18_ASSERT(tickr.getTickerId().getTypeId() == typeIdIdx);

					typedef ::std::decay_t<decltype(tickr)> ticker_t;

					//almost template-less function
					self_t::_sInitCtxs(ths->template _getAdapter<ticker_t>(), hana::at(ctxStor, typeIdIdx), rctx, libPath, tickr.getTickerBase());
				});


				//2. reading/populating market - feeding the earliest bar to the market, reread a new bar and repeat
				while (rctx.isValid()) {
					T18_ASSERT(!rctx.tiidOfEarliestTs.isEveryone());

					m.template exec4TickerIndexed<false>(rctx.tiidOfEarliestTs, [&rctx, &ctxStor, &m, ths=this](auto& tickr, auto&& typeIdIdx) {
						auto &vCtx = hana::at(ctxStor, typeIdIdx);
						auto& ctxToUse = vCtx[tickr.getTickerId().getIdx()];
						T18_ASSERT(invalidTs != ctxToUse.bar.TS());

						typedef ::std::decay_t<decltype(tickr)> ticker_t;
						auto& adpt = ths->template _getAdapter<ticker_t>();

						//m.newBarOpen(tickr, tsq_data::asBarOpen(ctxToUse.bar));
						//m.newBarClose(tickr, ctxToUse.bar);
						adpt.updateMkt(m, tickr, ctxToUse.bar);

						//now we must find the earliest already read bar and then read new bar into ctxToUse
						ctxToUse.bar.TS() = invalidTs;
						rctx.reset();

						m.forEachTickerIndexed([&ctxStor, &rctx](auto& tickr2, auto&& typeIdIdx2) {
							_sUpdateCtxs(hana::at(ctxStor, typeIdIdx2), rctx, tickr2.getTickerBase());							
						});				

						//new we must read a new bar into the pCtx
						sReadBar(adpt, ctxToUse, rctx, tickr.getTickerBase());
					});					
				}

				//doing notify to make sure that all higher-level timeframes are closed 
				m.notifyDateTime(TickerId::forEveryone(), invalidTs);
			}

		};

	}
}

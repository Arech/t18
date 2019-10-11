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

#include <vector>
#include "../base.h"
#include "../utils/myFile.h"

#include "tickerUpdater.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//#TODO: Whole feeders concept must be refactored
// feeder object must also accept (set of) ticker objects and populate that tickers
// backtester class should know nothing about tickers data storage
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 

namespace t18 {
	namespace feeder {

		namespace _i {
			template<typename ThisT, typename baseAdptT>
			struct adpt_hlpr : public baseAdptT {
				using typename baseAdptT::value_t;

			protected:
				ThisT* pT = nullptr;

			public:
				template<typename...ArgsT>
				adpt_hlpr(ThisT* p, ArgsT&&... a) : baseAdptT(::std::forward<ArgsT>(a)...), pT(p) {}

				template<typename MdssT, typename TickerServT>
				void updateMkt(MdssT& , TickerServT& , const value_t& val) const {
					pT->m_data.push_back(val);
				}
			};

			template<template<class> typename SrcFeederTpl, typename FeederAdptT>
			struct type_hlpr {
				template<typename AdptT>
				using src_feeder_tpl = SrcFeederTpl<AdptT>;

				typedef FeederAdptT feeder_adpt_t;
			};
		}


		template<typename ValT>
		class memory {
		public:
			typedef ValT value_type;
			typedef memory<value_type> self_t;

		protected:
			::std::vector<value_type> m_data;

			template<typename,typename> friend struct _i::adpt_hlpr;

		public:
			memory(){}

			//feederArgsTuple is a hana::tuple of arguments to the feeder constructor excluding adapter argument
			//adptArgsTuple is a hana::tuple of arguments to the adapter constructor
			template<typename TypeHlprT, typename FeederArgsHTupleT, typename AdptArgsHTupleT>
			memory(FeederArgsHTupleT&& feederArgsTuple, AdptArgsHTupleT&& adptArgsTuple, TypeHlprT&&) {
				static_assert(hana::is_a<hana::tuple_tag, FeederArgsHTupleT>, "");
				static_assert(hana::is_a<hana::tuple_tag, AdptArgsHTupleT>, "");

				typedef _i::adpt_hlpr<self_t, typename TypeHlprT::feeder_adpt_t> adpt_hlpr_t;
				typedef typename TypeHlprT::template src_feeder_tpl<adpt_hlpr_t> feeder_t;

				populateFrom(hana::unpack(
					//creating arguments for the feeder constructor by prepending feederArgsTuple with adapter object
					hana::prepend(
						::std::forward<FeederArgsHTupleT>(feederArgsTuple)
						, hana::unpack(
							hana::prepend(::std::forward<AdptArgsHTupleT>(adptArgsTuple), this)
							//creating the adapter by forwarding {this,adptArgsTuple...} to the adapter constructor
							, [](auto&&...a) { return adpt_hlpr_t(std::forward<decltype(a)>(a)...); }
						)
					), //creating feeder object by forwarding {constructed_adapter, feederArgsTuple} tuple to feeder constructor
					[](auto&&...a) {return feeder_t(std::forward<decltype(a)>(a)...); }
				));
			}

			template< template<class> typename SrcFeederTpl, typename FeederAdptT, typename FeederArgsHTupleT, typename AdptArgsHTupleT>
			static self_t make(FeederArgsHTupleT&& feederArgsTuple, AdptArgsHTupleT&& adptArgsTuple) {
				return self_t(::std::forward<FeederArgsHTupleT>(feederArgsTuple), ::std::forward<AdptArgsHTupleT>(adptArgsTuple)
					, _i::type_hlpr<SrcFeederTpl, FeederAdptT>{});
			}


			template<typename SrcFeederT>
			void populateFrom(SrcFeederT&& f) {
				m_data.clear();
				const auto sz = f.capacity();
				m_data.reserve( sz );

				f.processByAdapter();

				T18_ASSERT(sz == m_data.size() || sz - 1 == m_data.size());
			}

			void clear(){ m_data.clear(); }
			bool empty()const { return m_data.empty(); }

			template<typename SrvT>
			void feed(SrvT&& srv)const {
				T18_ASSERT(!empty());
				
				for (const auto& e : m_data) {
					srv.newBarOpen(e.TSQ());
					srv.newBarAggregate(e);
				}
				//doing notify to make sure that all higher-level timeframes are closed 
				srv.notifyDateTime(m_data.back().plusYear());
			}

			template<typename SrvT>
			void operator()(SrvT& srv) const {
				feed(srv);
			}

			template<typename MktT, typename TServT>
			void operator()(MktT& m, TServT& t)const {
				feed(tickerUpdater<MktT, TServT>(m, t));
			}
			/*template<typename MktT>
			void operator()(MktT& m, size_t tiid) const {
				feed(tickerUpdater<MktT, TServT>(m, m.getTickerServ(tiid)));
			}*/
		};

	}
}
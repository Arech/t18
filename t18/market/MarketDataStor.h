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

#include "tickerServer.h"

namespace t18 {

	//this class manages all the run-time market data (tickers for now) required for a TS to work and provides access interface to them
	// TickersSetHST is a hana::set of different ticker types (each of which could be instantiated multiple times)
	template<typename TickersSetHST>
	class MarketDataStor {
	private:
		static_assert(utils::isHSetT_v<TickersSetHST>, "TickersSetHST must be a hana::set consisting of different hana::type_c<TickerT>");
		typedef MarketDataStor<TickersSetHST> self_t;

	public:
		typedef TickersSetHST TickersSet_hst;

		typedef decltype(hana::to_tuple(TickersSet_hst())) TickersTuple_t;
		static constexpr auto TickersTupleSize_v = hana::size(TickersTuple_t());

	protected:
		// #TODO: forward_list with inplace construction is better suited here. It'd be almost the same as vector+ptr,
		//but without added burden
		template<typename T>
		using TickersCont_tpl = ::std::vector<::std::unique_ptr<T>>;

	private:
		template<typename HTT, typename = ::std::enable_if_t<utils::isDescrTuple_v<HTT>>>
		static constexpr auto makeTickersStor(HTT const& tup) {
			return hana::transform(tup, [](auto&& e) {
				typedef typename decltype(+e)::type ticker_t;
				static_assert(::std::is_same_v<typename ticker_t::tag_t, tag_Ticker_t>, "TickersSetHST must contain set of Ticker types");
				return ::std::declval<TickersCont_tpl<ticker_t>>();
			});
		}

	protected:
		typedef decltype(makeTickersStor(TickersTuple_t())) TickersStor_t;
		
	protected:
		TickersStor_t m_TickerStor;

	public:
		MarketDataStor() : m_TickerStor() {}

		//////////////////////////////////////////////////////////////////////////
		template<typename TickerT, bool bChecked = true>
		auto& getTicker(TickerId tiid) {
			constexpr auto tIdx = utils::index_of_existing<TickersTuple_t>(hana::type_c<TickerT>);
			if (bChecked && tiid.getTypeId() != tIdx) {
				//should never happen but still must check
				T18_ASSERT(!"Type index in tiid mismatches real type index!");
				throw ::std::runtime_error("Type index in tiid mismatches real type index!");
			}
			auto& cont = hana::at(m_TickerStor, tIdx);
			const auto idx = tiid.getIdx();
			if (bChecked && idx >= cont.size()) {
				//should never happen but still must check
				T18_ASSERT(!"Index in tiid is too big for container found!");
				throw ::std::runtime_error("Index in tiid is too big for container found!");
			}
			auto& ptr = cont[idx];
			if (bChecked && !ptr) {
				//should never happen but still must check
				T18_ASSERT(!"Ticker found by tiid has not been created yet!");
				throw ::std::runtime_error("Ticker found by tiid has not been created yet!");
			}
			auto& t = *ptr;
			if (bChecked && t.getTickerId() != tiid) {
				//should never happen but still must check
				T18_ASSERT(!"Ticker found by tiid has wrong tickerId!");
				throw ::std::runtime_error("Ticker found by tiid has wrong tickerId!");
			}
			return t;
		}
		template<typename TickerT, bool bChecked = true>
		const auto& getTicker(TickerId tiid) const {
			return const_cast<self_t*>(this)->getTicker<TickerT, bChecked>(tiid);
		}

		template<typename TickerT, bool bChecked = true>
		auto& getTicker(const ::std::string& name) {
			constexpr auto tIdx = utils::index_of_existing<TickersTuple_t>(hana::type_c<TickerT>);
			auto& cont = hana::at(m_TickerStor, tIdx);
			TickerT* pTicker = nullptr;
			for (auto& ptr : cont) {
				if (bChecked && !ptr) {
					T18_ASSERT(!"WTF? Empty ticker slot found!");
					throw ::std::runtime_error("WTF? Empty ticker slot found!");
				}
				if (name == ptr->Name()) {
					pTicker = ptr.get();
					break;
				}
			}
			if (bChecked && !pTicker) {
				T18_ASSERT(!"Failed to find ticker by type and name!");
				throw ::std::runtime_error("Failed to find ticker by type and name="s + name);
			}
			return *pTicker;
		}
		template<typename TickerT, bool bChecked = true>
		const auto& getTicker(const ::std::string& name) const {
			return const_cast<self_t*>(this)->getTicker<TickerT, bChecked>(name);
		}

		//////////////////////////////////////////////////////////////////////////
		template<bool bChecked, typename F>
		void exec4Ticker(const ::std::string& name, F&& f) {
			hana::any_of(hana::to_tuple(hana::range_c<size_t, 0, TickersTupleSize_v>)
				, [&name, &f, &stor = m_TickerStor](auto&& x)
			{
				auto& cont = hana::at(stor, x);
				typedef typename ::std::decay_t<decltype(cont)>::value_type::element_type ticker_t;

				ticker_t* pTicker = nullptr;
				for (auto& ptr : cont) {
					if (bChecked && !ptr) {
						T18_ASSERT(!"WTF? Empty ticker slot found!");
						throw ::std::runtime_error("WTF? Empty ticker slot found!");
					}
					if (name == ptr->Name()) {
						pTicker = ptr.get();
						::std::forward<F>(f)(*pTicker);
						break;
					}
				}
				return pTicker != nullptr;
			});
		}
		template<bool bChecked, typename F>
		const auto& exec4Ticker(const ::std::string& name, F&& f) const {
			return const_cast<self_t*>(this)->exec4Ticker<bChecked>(name, ::std::forward<F>(f));
		}

	protected:
		template<bool bChecked, typename F>
		void _exec4TickerIndexed(TickerId tiid, F&& f) {
			const auto typeId = tiid.getTypeId();
			hana::any_of(hana::to_tuple(hana::range_c<size_t, 0, TickersTupleSize_v>)
				, [typeId, tiid, &f, &stor = m_TickerStor](auto&& typeIdIdx)
			{
				const bool found = typeId == typeIdIdx;
				if (found) {
					auto& cont = hana::at(stor, typeIdIdx);

					//typedef typename ::std::decay_t<decltype(cont)>::value_type::element_type ticker_t;

					const auto idx = tiid.getIdx();
					if (bChecked && idx >= cont.size()) {
						//should never happen but still must check
						T18_ASSERT(!"Index in tiid is too big for container found!");
						throw ::std::runtime_error("Index in tiid is too big for container found!");
					}
					auto& ptr = cont[idx];
					if (bChecked && !ptr) {
						T18_ASSERT(!"WTF? Ticker slot addressed by tiid is empty!");
						throw ::std::runtime_error("WTF? Empty ticker slot found!");
					}
					//auto pT = hana::if_(hana::bool_c<isConst>, static_cast<const ticker_t*>(ptr.get()), static_cast<ticker_t*>(ptr.get()));
					//cant use references here, because hana turns them into r-values

					auto& t = *ptr;
					if (bChecked && t.getTickerId() != tiid) {
						//should never happen but still must check
						T18_ASSERT(!"Ticker found by tiid has wrong tickerId!");
						throw ::std::runtime_error("Ticker found by tiid has wrong tickerId!");
					}
					::std::forward<F>(f)(t, typeIdIdx);
				}
				return found;
			});
		}

	public:
		template<bool bChecked, typename F>
		void exec4Ticker(TickerId tiid, F&& f) {
			_exec4TickerIndexed<bChecked>(tiid, [&f](auto& tickr, auto&&) {
				::std::forward<F>(f)(tickr);
			});
		}
		template<bool bChecked, typename F>
		void exec4Ticker(TickerId tiid, F&& f) const {
			const_cast<self_t*>(this)->_exec4TickerIndexed<bChecked>(tiid, [&f](const auto& tickr, auto&&) {
				::std::forward<F>(f)(tickr);
			});
		}

		template<bool bChecked, typename F>
		void exec4TickerIndexed(TickerId tiid, F&& f) {
			_exec4TickerIndexed<bChecked>(tiid, ::std::forward<F>(f));
		}
		template<bool bChecked, typename F>
		const auto& exec4TickerIndexed(TickerId tiid, F&& f) const {
			const_cast<self_t*>(this)->_exec4TickerIndexed<bChecked>(tiid, [&f](const auto& tickr, auto&& typeIdIdx) {//need lambda to propagate constness of tickr
				::std::forward<F>(f)(tickr, typeIdIdx);
			});
		}

		//////////////////////////////////////////////////////////////////////////

		size_t tickersCount()const noexcept { 
			/*return hana::sum<size_t>(hana::transform(m_TickerStor, [](const auto& v)-> size_t {
				return v.size();
			}));*/
			return hana::sum<size_t>(tickersCountByType());
		}

		auto tickersCountByType()const noexcept {
			return hana::transform(m_TickerStor, [](const auto& v)-> size_t {
				return v.size();
			});
		}

		constexpr auto tickerTypesCount()const noexcept{
			return hana::size(m_TickerStor);
		}

	protected:
		template<typename F>
		void _forEachTickerIndexed(F&& f) {
			hana::for_each(hana::to_tuple(hana::range_c<size_t, 0, TickersTupleSize_v>)
				, [&f, &stor = m_TickerStor](auto&& typeIdIdx)
			{
				auto& cont = hana::at(stor, typeIdIdx);
				//typedef typename ::std::decay_t<decltype(cont)>::value_type::element_type ticker_t;
				for (auto& ptr : cont) {
					if (!ptr) {
						T18_ASSERT(!"WTF? Ticker slot addressed by tiid is empty!");
						throw ::std::runtime_error("WTF? Empty ticker slot found!");
					}
					//auto pT = hana::if_(hana::bool_c<isConst>, static_cast<const ticker_t*>(ptr.get()), static_cast<ticker_t*>(ptr.get()));
					//cant use references here, because hana turns them into r-values
					::std::forward<F>(f)(*ptr.get(), typeIdIdx);
				}
			});
		}
		
	public:
		template<typename F>
		void forEachTicker(F&& f) {
			_forEachTickerIndexed([&f](auto& tickr, auto&&) {
				::std::forward<F>(f)(tickr);
			});
		}
		template<typename F>
		void forEachTicker(F&& f) const {
			//const_cast<self_t*>(this)->_forEachTicker<true>(::std::forward<F>(f));
			const_cast<self_t*>(this)->_forEachTickerIndexed([&f](const auto& tickr, auto&&) {
				::std::forward<F>(f)(tickr);
			});
		}

		template<typename F>
		void forEachTickerIndexed(F&& f) {
			_forEachTickerIndexed(::std::forward<F>(f));
		}
		template<typename F>
		void forEachTickerIndexed(F&& f) const {
			const_cast<self_t*>(this)->_forEachTickerIndexed([&f](const auto& tickr, auto&& typeIdIdx) {//need lambda to propagate constness of tickr
				::std::forward<F>(f)(tickr, typeIdIdx);
			});
		}
	};


}

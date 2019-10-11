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

//#include <forward_list>
#include <vector>
#include "trade.h"

namespace t18 {

	//#WARNING this is a very quick&dirty solution suitable for a small amount of simultaneously opened trades.
	
	template<typename DataT>
	class assocTradeInfo {
	public:
		typedef trade trade_t;
		typedef typename trade_t::tradeId_t tradeId_t;
		static constexpr tradeId_t invalid_tid = trade_t::invalid_tid;

		typedef DataT data_t;
		
	protected:
		struct fullData {
		protected:
			//data_t d;
			alignas(data_t) char d[sizeof(data_t)];

		public:
			tradeId_t tid;

		public:
			//we must clear the data on destruction. Note that defining the destructor disables implicit copy constructor
			~fullData() {
				clear();
			}

			fullData(const fullData& a) {
				tid = invalid_tid;
				if (!a.is_clear()) {
					get_data() = a.get_data();
				}
			}
			
			fullData(data_t&& _d, tradeId_t _t) : tid(_t){
				T18_ASSERT(_t != invalid_tid);
				get_data() = ::std::move(_d);
			}
			fullData(tradeId_t _t) : tid(_t) {
				T18_ASSERT(_t != invalid_tid);
				new (&get_data()) data_t();
			}

			void check_available()const {
				if (UNLIKELY(is_clear())) {
					T18_ASSERT(!"WTF? No associated data at this moment!");
					throw ::std::runtime_error("WTF? No associated data at this moment!");
				}
			}
			
			data_t& get_data() {
				check_available();
				return *(static_cast<data_t*>(static_cast<void*>(&d)));
			}
			const data_t& get_data()const noexcept {
				check_available();
				return *(static_cast<const data_t*>(static_cast<const void*>(&d)));
			}

			void clear() {
				if (!is_clear()) {
					get_data().~data_t();
					tid = invalid_tid;
				}
			}

			bool is_clear()const noexcept {
				return tid == invalid_tid;
			}

			void set(data_t&& _d, tradeId_t _t) {
				T18_ASSERT(tid == invalid_tid);
				T18_ASSERT(_t != invalid_tid);
				//d = ::std::move(_d);
				tid = _t;
				get_data() = ::std::move(_d);
			}
			void set(tradeId_t _t) {
				T18_ASSERT(tid == invalid_tid);
				T18_ASSERT(_t != invalid_tid);
				tid = _t;
				new (&get_data()) data_t();
			}
		};

		//typedef DataContainerT DataCont_t;
		typedef ::std::vector<fullData> DataCont_t;
		typedef typename DataCont_t::iterator iterator_t;
		//typedef typename DataCont_t::value_type value_type;

	protected:
		DataCont_t m_Cont;

	public:
		assocTradeInfo() {}
		assocTradeInfo(size_t initCapacity) {
			m_Cont.reserve(initCapacity);
		}

		assocTradeInfo(const assocTradeInfo&) = delete;
		assocTradeInfo(assocTradeInfo&&) = delete;

		size_t size()const noexcept { return m_Cont.size(); }

		data_t& store(const trade_t& trd, data_t&& d) {
			T18_ASSERT(::std::none_of(m_Cont.begin(), m_Cont.end(), [t = trd.TradeId()](const auto& e) {
				return t == e.tid;
			}));

			auto it = _first_free();
			if (it == m_Cont.end()) {
				m_Cont.emplace_back(::std::move(d), trd.TradeId());
				it = --m_Cont.end();
			} else {
				it->set(::std::move(d), trd.TradeId());
			}
			return it->get_data();
		}

		data_t& store(const trade_t& trd) {
			T18_ASSERT(::std::none_of(m_Cont.begin(), m_Cont.end(), [t = trd.TradeId()](const auto& e) {
				return t == e.tid;
			}));

			auto it = _first_free();
			if (it == m_Cont.end()) {
				m_Cont.emplace_back(trd.TradeId());
				it = --m_Cont.end();
			} else {
				it->set(trd.TradeId());
			}
			return it->get_data();
		}

		void free(const trade_t& trd) {
			auto& ti = _find(trd.TradeId());
			ti.clear();
		}

		data_t& get(const trade_t& trd){
			auto& ti = _find(trd.TradeId());
			return ti.get_data();
		}
		const data_t& get(const trade_t& trd)const {
			auto& ti = _find(trd.TradeId());
			return ti.get_data();
		}

	protected:
		iterator_t _first_free()noexcept {
			iterator_t r = m_Cont.begin();
			const iterator_t e = m_Cont.end();
			for (; r != e; ++r) {
				if (r->is_clear()) {
					return r;
				}
			}
			return e;
		}

		auto& _find(tradeId_t tid) {
			auto r = m_Cont.begin();
			const auto e = m_Cont.end();
			for (; r != e; ++r) {
				if (r->tid == tid) {
					return *r;
				}
			}
			T18_ASSERT(!"There's no such trade id in assocTradeInfo!");
			throw ::std::runtime_error("There's no such trade id in assocTradeInfo!");
		}

		const auto& _find(tradeId_t tid) const {
			auto r = m_Cont.begin();
			const auto e = m_Cont.end();
			for (; r != e; ++r) {
				if (r->tid == tid) {
					return *r;
				}
			}
			T18_ASSERT(!"There's no such trade id in assocTradeInfo!");
			throw ::std::runtime_error("There's no such trade id in assocTradeInfo!");
		}


	};

}


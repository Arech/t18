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

#include <functional>
#include <list>
#include <memory>
#include <type_traits>

#include "../base.h"
#include "../utils/regHandle.h"

namespace t18 {
	namespace timeseries {

		//handle common for every source events, such as onNewBarOpen and onNotifyDateTime
		class _baseHandler {
		private:
			typedef _baseHandler self_t;

		public:
			typedef def_call_wrapper_t call_wrapper_t;
			typedef typename call_wrapper_t::template call_tpl<void(const tsq_data& tso)> onNewBarOpenCB_t;
			typedef typename call_wrapper_t::template call_tpl<void(mxTimestamp ts)> onNotifyDateTimeCB_t;

		protected:
			typedef ::std::list<onNewBarOpenCB_t> onNewBarOpenCBStor_t;
			typedef ::std::list<onNotifyDateTimeCB_t> onNotifyDateTimeCBStor_t;

		protected:
			//#TODO refactor all this shit into a single hana::map
			onNewBarOpenCBStor_t m_onNewBarOpenCBs;
			onNotifyDateTimeCBStor_t m_onNotifyDateTimeCBs;

		protected:
			_baseHandler()noexcept{}

			void _onNewBarOpen(const tsq_data& tso)const noexcept {
				for (const auto& f : m_onNewBarOpenCBs) {
					f(tso);
				}
			}
			void _onNotifyDateTime(mxTimestamp ts)const noexcept {
				for (const auto& f : m_onNotifyDateTimeCBs) {
					f(ts);
				}
			}

		private:
			void _deregister(typename onNewBarOpenCBStor_t::iterator it) noexcept {
				T18_ASSERT(m_onNewBarOpenCBs.size());
				m_onNewBarOpenCBs.erase(it);
			}
			void _deregister(typename onNotifyDateTimeCBStor_t::iterator it) noexcept {
				T18_ASSERT(m_onNotifyDateTimeCBs.size());
				m_onNotifyDateTimeCBs.erase(it);
			}

			template<typename CBStorT>
			auto _makeHandle(CBStorT& stor) {
				return utils::regHandle(::std::bind(static_cast<void(self_t::*)(typename CBStorT::iterator)>(&self_t::_deregister)
					, this, --stor.end()));
			}

		public:
			/////////////////////////////////////////////////////////////////////////////////
			//WARNING! Read the warning above the regHandle class declaration!
			/////////////////////////////////////////////////////////////////////////////////
			decltype(auto) registerOnNewBarOpen(onNewBarOpenCB_t&& f) {
				//template<typename F> decltype(auto) registerOnNewBarOpen(F&& f) {
				//m_onNewBarOpenCBs.push_back(call_wrapper_t::wrap<F>(::std::forward<F>(f)));// ::std::move(f));
				m_onNewBarOpenCBs.push_back(::std::move(f));
				return _makeHandle(m_onNewBarOpenCBs);
			}
			decltype(auto) registerOnNotifyDateTime(onNotifyDateTimeCB_t&& f) {
				//template<typename F> decltype(auto) registerOnNotifyDateTime(F&& f) {
				//m_onNotifyDateTimeCBs.push_back(call_wrapper_t::wrap<F>(::std::forward<F>(f)));// ::std::move(f));
				m_onNotifyDateTimeCBs.push_back(::std::move(f));
				return _makeHandle(m_onNotifyDateTimeCBs);
			}
			
			size_t countOfOnNewBarOpen()const noexcept { return m_onNewBarOpenCBs.size(); }
			size_t countOfOnNotifyDateTime()const noexcept { return m_onNotifyDateTimeCBs.size(); }
		};

		//designed to handle callbacks, originated in Timeframe objects, i.e. onNewBarOpen, onNotifyDateTime and onNewBarClose
		class TFUpdatesHandler : public _baseHandler {
		private:
			typedef TFUpdatesHandler self_t;
			typedef _baseHandler base_class_t;

		public:
			using base_class_t::call_wrapper_t;
			typedef typename call_wrapper_t::template call_tpl<void(const tsohlcv& bar)> onNewBarCloseCB_t;
			
		private:
			typedef ::std::list<onNewBarCloseCB_t> onNewBarCloseCBStor_t;
			
		private:
			//#TODO refactor all this shit into a single hana::map
			onNewBarCloseCBStor_t m_onNewBarCloseCBs;

		protected:
			TFUpdatesHandler() noexcept : base_class_t() {}

			void _onNewBarClose(const tsohlcv& bar)const noexcept {
				for (const auto& f : m_onNewBarCloseCBs) {
					f(bar);
				}
			}

		private:
			//assert triggering here is a sign that this object might have already been destroyed!
			void _deregister(typename onNewBarCloseCBStor_t::iterator it) noexcept {
				T18_ASSERT(m_onNewBarCloseCBs.size());
				m_onNewBarCloseCBs.erase(it);
			}
			
			template<typename CBStorT>
			auto _makeHandle(CBStorT& stor) {
				return utils::regHandle(::std::bind(static_cast<void(self_t::*)(typename CBStorT::iterator)>(&self_t::_deregister)
					, this, --stor.end()));
			}

		public:
			/////////////////////////////////////////////////////////////////////////////////
			//WARNING! Read the warning above the regHandle class declaration!
			/////////////////////////////////////////////////////////////////////////////////
			
			// Note that extra care must be taken, when making trading decisions during handling of onNewBarClose()
			// By the nature of quotes transfer process an actual bar closing might occur either far after the real time of
			// bar closing, or even if the timing is correct, you may find yourself in a position when you can't issue trading orders (EOD!)!
			// In general, the taken approach on issuing NewBarClose() event (when newBarOpen() is arrived while previous
			// bar wasn't completed yet) seems correct and coherent to how quoting data actually gets transferred to a trading app.
			// We can't just issue onNewBarClose() purely on timely basis (using onNotifyDateTime()), because some older
			// bars just might be delayed by network and arrive a bit later.
			
			decltype(auto) registerOnNewBarClose(onNewBarCloseCB_t&& f) {
			//template<typename F> decltype(auto) registerOnNewBarClose(F&& f) {
				//m_onNewBarCloseCBs.push_back(call_wrapper_t::wrap<F>(::std::forward<F>(f)));// ::std::move(f));
				m_onNewBarCloseCBs.push_back(::std::move(f));
				return _makeHandle(m_onNewBarCloseCBs);
			}

			size_t countOfOnNewBarClose()const noexcept { return m_onNewBarCloseCBs.size(); }
		};

	}
}


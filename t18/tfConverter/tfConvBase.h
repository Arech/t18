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

#include "_base.h"

//doesn't convert anything

namespace t18 {
	namespace tfConverter {
		using namespace ::std::literals;
		
		//The tfConvBase is a time-based converter

		//this converter must never be used with tsTicks. It's designed for backtesting purposes only with
		//tsohlcv-like data, that should NOT be converted to different timeframe. Pass minutes id of a timeframe that's fed to
		// converter to a constructor (and make sure there're no finer-grained data in bar-inflow)
		template<typename BarT> // = tsohlcv>
		class tfConvBase : public _base<> {
			typedef _base<> base_class_t;
		protected:
			using typename base_class_t::flags_t;
		protected:
			using base_class_t::_bit_lastBarClosed;
			using base_class_t::_bit_barHasData;
		private:
			using base_class_t::_base_lastUsedBit;
		protected:
			using base_class_t::m_flags;
		protected:
			using base_class_t::_flagSet;
			using base_class_t::_flagClear;
			using base_class_t::_flagSetClear;
			using base_class_t::_isFlagSet;
		public:
			using base_class_t::lastBarClosed;
			using base_class_t::_setLastBarClosed_clearBarHasData;
			using base_class_t::_setLastBarClosed;
			using base_class_t::_clearLastBarClosed;
			using base_class_t::_setFlag_BarHasData;
			using base_class_t::_clearFlag_BarHasData;
			using base_class_t::_BarHasData;
			using base_class_t::_verifyLastBarClosed;
			using base_class_t::_verifyLastBarOpened;
			using base_class_t::_verifyFlagSet_BarHasData;

		public:
			typedef BarT bar_t;

		protected:
			bar_t m_lastBar;

			TimestampTFMinutes m_tfBndry;

			static constexpr flags_t _tfConvBase_lastUsedBit = _base_lastUsedBit;

			T18_DEBUG_ONLY(timestamp_diff_t m_dbgTfMks);

		public:
			explicit tfConvBase(int src) : base_class_t(), m_lastBar(typename bar_t::tag_Default_t()), m_tfBndry(src) {
				_setLastBarClosed_clearBarHasData();

				T18_DEBUG_ONLY(m_dbgTfMks = _mxTimestamp<false>(0, 0
					, m_tfBndry.tf() >= 24 * 60 ? m_tfBndry.tf() / (24 * 60) : 0
					, m_tfBndry.tf() >= 60 && m_tfBndry.tf() < 24 * 60 ? m_tfBndry.tf() / 60 : 0
					, m_tfBndry.tf() < 60 ? m_tfBndry.tf() : m_tfBndry.tf() % 60, 0)._yearTime());
			}

			int tf()const { return m_tfBndry.tf(); }

		protected:
			void _doClose() {
				_verifyFlagSet_BarHasData();

				T18_ASSERT(m_lastBar.valid());

				_setLastBarClosed_clearBarHasData();
			}

			bool _belongsToNewBar(mxTimestamp ts)const noexcept {
				T18_ASSERT(!ts.empty());
				T18_ASSERT(m_lastBar.TS().empty() || m_lastBar.TS() <= ts);
				//it's a new bar if either of the following is true:
				// - current day is not the same as previous day
				// - current minute differs more than tfMins from previously stored
				// 
				// lasy OR (||) helps to deal with possibly uninitialized m_tfBndry
				//return m_lastBar.TS().empty() || ts.Date() != m_lastBar.Date() || m_tfBndry.upperBoundTimestamp() <= ts;
				const bool ret = m_tfBndry.notInialized() || m_tfBndry.upperBoundTimestamp() <= ts;
				//checking that if ts is NOT next bar, than it MUST be inside of current bar
				// (i.e. it must not differ from the current period upper bound in more than timeframe minutes)
				T18_ASSERT(ret || m_tfBndry.upperBoundTimestamp().uglyDiffMks(ts) <= m_dbgTfMks);
				return ret;
			}

			void _verifyBelongsToCurBar(mxTimestamp ts)const {
				if (UNLIKELY(_belongsToNewBar(ts))) {
					T18_ASSERT(!"WTF? Bar to close must be inside current tf bar!");
					throw ::std::runtime_error("WTF? Bar to close must be inside current tf bar!");
				}
			}

		public:
			//this function is used to detect bar ending
			//returns inner representation of a bar to close which is valid until another call to any of this->function
			//#TODO in general case it should be not the timestamp of a new <bar>, but the whole new <bar> itself to
			//make possible more complex timeframe conversions, based on something else besides timestamp
			// shouldCloseCurBar() MUST NOT change internal bar representation, but may and should change the bar aggregation state
			const bar_t* shouldCloseCurBar(mxTimestamp ts) {
				const bar_t* pRet;
				// !lastBarClosed() seems to be wrong here. _bit_barHasData must be used instead, but for this particular converter
				//they both means the same, so leave as is
				if (!lastBarClosed() && _belongsToNewBar(ts)) {
					_doClose();
					pRet = &m_lastBar;
				} else pRet = nullptr;
				return pRet;
			}

			// Determines if the passed bar opening belongs to a new time period, i.e. a new bar is being created
			// If so, returns a pointer to inner bar representation which is valid until another call to any of this->function
			const bar_t* isReallyNewBarOpen(const tsq_data& tso) {
				_verifyLastBarClosed();//we don't do tf conversion, therefore it must be closed here

				T18_ASSERT(!_BarHasData());//no need to release-check, bc for this class it's the same as !LastBarClosed

				T18_ASSERT(tso.valid());
				T18_ASSERT(m_lastBar.TS().empty() || m_lastBar.TS() < tso.TS());

				m_lastBar.openedFrom(tso, m_tfBndry);
				//_clearLastBarClosed();
				//_setLastBarJustOpened();
				_flagClear<(_bit_lastBarClosed | _bit_barHasData)>();

				return &m_lastBar;
			}

			//////////////////////////////////////////////////////////////////////////

		protected:
			template<typename T, typename = ::std::enable_if_t<::std::is_same_v<T, bar_t> || ::std::is_same_v<T, tsTick>>>
			const bar_t* _aggregate(const T& curBar) {
				//just some sanity checks here
				T18_ASSERT(curBar.valid());
				T18_ASSERT((!_BarHasData() && m_lastBar.ohlc_valid()) || (_BarHasData() && m_lastBar.valid()));
				T18_ASSERT(curBar.TS() >= m_lastBar.TS());

				_verifyLastBarOpened();//it must be opened
				_verifyBelongsToCurBar(curBar);//this bar can't belong to a new bar of current TF
				
				//aggregating values
				m_lastBar.aggregate(curBar);
				_setFlag_BarHasData();
				//this->_clearLastBarJustOpened();

				return &m_lastBar;
			}

		public:
			//aggregate() always returns a non-null value, but we're using a pointer instead of reference to block possible data copying
			//should the caller forget to declare as reference a variable that receives returned value
			template<typename T>
			::std::enable_if_t<::std::is_base_of_v<bar_t, T>, const bar_t*> aggregate(const T& curBar) {
				return _aggregate(static_cast<const bar_t&>(curBar));
			}
		};

		typedef tfConvBase<tsohlcv> baseOhlc;

	}
}


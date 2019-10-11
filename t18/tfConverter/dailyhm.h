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

#include "../base.h"
#include "tfConvBase.h"

//converts timeframe from M1 to any Mx, where x<=60 and 0 == 60 % x, as well as to H1=M60 and to daily=M1440
//#WARNING there're unsolved issues with this converter and lastKnownQuote that might arise when a bar gets closed
//via isReallyNewBarOpen().
namespace t18 { namespace tfConverter {

	template<typename BarT> // = tsohlcv>
	class dailyhm : public tfConvBase<BarT>{
	private:
		typedef tfConvBase<BarT> base_class_t;

	public:
		using typename base_class_t::bar_t;

	protected:
		//example how to add more bits to the flag variable
		/*static_assert(base_class_t::_tfConvBase_lastUsedBit <= (::std::numeric_limits<::std::uint32_t>::max() >> 1)
			, "Choose a bigger type for flags variable");
		static constexpr ::std::uint32_t _bit_barHasData = (base_class_t::_tfConvBase_lastUsedBit << 1);
		static constexpr ::std::uint32_t _dailyhm_lastUsedBit = _bit_barHasData;*/
			
	protected:
		using base_class_t::_bit_lastBarClosed;
		using base_class_t::_bit_barHasData;

		using base_class_t::m_lastBar;
		using base_class_t::m_tfBndry;

	protected:
		using base_class_t::lastBarClosed;
		using base_class_t::_verifyLastBarClosed;
		using base_class_t::_verifyLastBarOpened;
		//using base_class_t::lastBarJustOpened;

		using base_class_t::_setFlag_BarHasData;
		using base_class_t::_clearFlag_BarHasData;
		using base_class_t::_BarHasData;

		using base_class_t::_verifyFlagSet_BarHasData;
		using base_class_t::_doClose;

		using base_class_t::_belongsToNewBar;
		using base_class_t::_verifyBelongsToCurBar;

	public:
		using base_class_t::tf;

	public:
		dailyhm(int dest) : base_class_t(dest) {}

	public:
		template<typename T, typename=::std::enable_if_t<::std::is_same_v<T,tsq_data> || ::std::is_same_v<T, tsTick>>>
		const bar_t* _isReallyNewBarOpen(const T& tso) {
			//just some sanity checks
			T18_ASSERT(tso.valid());
			const bar_t* pRet;
			if (_belongsToNewBar(tso)) {
				T18_ASSERT(lastBarClosed() && !_BarHasData());

				m_lastBar.openedFrom(tso, m_tfBndry);
				//this->_clearLastBarClosed();

				if constexpr(::std::is_same_v<T, tsTick>) {
					//volume field has already been set from tso
					//this->_setFlag_BarHasData();
					this->template _flagSetClear<_bit_barHasData, _bit_lastBarClosed>();
				} else {
					//only the price field has been set, but the whole m_lastBar is still not a valid bar
					this->template _flagClear<(_bit_lastBarClosed | _bit_barHasData)>();
					//	_clearFlag_BarHasData();
				}

				//we must round m_nextBarMinuteOfDay to the next timeframe minute boundary
				/*const auto tmod = m_lastBar.MinuteOfDay();
				const auto ctf = m_tfBndry.tf();
				m_nextBarMinuteOfDay = ctf + tmod - (tmod % ctf);
				T18_ASSERT(0 == m_nextBarMinuteOfDay % ctf);
				m_baseTfLastBarMinuteOfDay = m_nextBarMinuteOfDay - 111; //this->_baseTf;
				T18_ASSERT(m_baseTfLastBarMinuteOfDay < mxTime::minutesInDay);
				//m_nextBarMinuteOfDay = m_nextBarMinuteOfDay % mxTime::minutesInDay;
				// m_nextBarMinuteOfDay could be > mxTime::minutesInDay, because in that case d!=openD condition will help
				//setting to default values*/
				
				pRet = &m_lastBar;
			} else {
				T18_ASSERT(!lastBarClosed() && _BarHasData());
				pRet = nullptr;
			}
			return pRet;
		}

		template<typename T>
		::std::enable_if_t<::std::is_base_of_v<tsq_data, T> && !::std::is_base_of_v<tsTick, T>, const bar_t*> isReallyNewBarOpen(const T& tso) {
			return _isReallyNewBarOpen(static_cast<const tsq_data&>(tso));
		}
		template<typename T>
		::std::enable_if_t<::std::is_base_of_v<tsTick, T>, const bar_t*> isReallyNewBarOpen(const T& tst) {
			return _isReallyNewBarOpen(static_cast<const tsTick&>(tst));
		}

		//////////////////////////////////////////////////////////////////////////
		/*bool isReallyNewBarClose(const bar_t& curBar, const bar_t* & pBtc) {
			pBtc = aggregate(curBar);

			const auto bClose = m_baseTfLastBarMinuteOfDay == curBar.MinuteOfDay();
			if (bClose) {
				//we had to close it
				_doClose();
			}
			return bClose;
		}*/

		/*//////////////////////////////////////////////////////////////////////////
		//aggregate() MUST always be called only after isReallyNewBarOpen
		template<typename T, typename = ::std::enable_if_t<::std::is_same_v<T, bar_t> || ::std::is_same_v<T, tsTick>>>
		const bar_t* _aggregate(const T& curBar) {
			//just some sanity checks here
			T18_ASSERT(curBar.valid());
			T18_ASSERT((lastBarJustOpened() && m_lastBar.ohlc_valid()) || (!lastBarJustOpened() && m_lastBar.valid()));
			T18_ASSERT(curBar.TS() >= m_lastBar.TS());

			_verifyLastBarOpened();//it must be opened
			_verifyBelongsToCurBar(curBar);//this bar can't belong to a new bar of current TF

			//aggregating values
			m_lastBar.aggregate(curBar);
			_setFlag_BarHasData();
			this->_clearLastBarJustOpened();

			return &m_lastBar;
		}

		template<typename T>
		::std::enable_if_t<::std::is_base_of_v<bar_t, T>, const bar_t*> aggregate(const T& curBar) {
			return _aggregate(static_cast<const bar_t&>(curBar));
		}*/

		template<typename T>
		::std::enable_if_t<::std::is_base_of_v<tsTick, T>, const bar_t*> aggregate(const T& curBar) {
			return base_class_t::_aggregate(static_cast<const tsTick&>(curBar));
		}
		template<typename T>
		::std::enable_if_t<::std::is_base_of_v<bar_t, T>, const bar_t*> aggregate(const T& curBar) {
			return base_class_t::_aggregate(static_cast<const bar_t&>(curBar));
		}
	};

	typedef dailyhm<tsohlcv> dailyhmOhlc;

} }
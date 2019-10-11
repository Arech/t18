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

#include <stdexcept>
#include "../base.h"

namespace t18 { namespace tfConverter {

	template<bool bDirected = false, typename FlagsT = ::std::uint32_t>
	class _base {
	public:
		//bar_t is a data type of the RESULT of conversion
		typedef void bar_t;//overrides MUST redefine bar_t to a correct type

		static constexpr bool isDirectedConverted = bDirected;

	protected:
		typedef FlagsT flags_t;

		static constexpr flags_t _bit_lastBarClosed = 1;
		static constexpr flags_t _bit_barHasData = (_bit_lastBarClosed << 1);
		//note that these two flags are generally mean different things. _bit_lastBarClosed means that the last bar have already been
		//closed (because required time period has passed, for example), and _bit_barHasData means that a new
		// data belonging to a new bar has already been aggregated into some internal bar representation.
		// Therefore it is totally valid state when _bit_lastBarClosed is set and _bit_barHasData is cleared

		//static constexpr flags_t _bit_lastBarJustOpened = _bit_barHasData << 1;
		static constexpr flags_t _base_lastUsedBit = _bit_barHasData;

	protected:
		flags_t m_flags{ 0 };

	protected:
		~_base()noexcept {}
		_base()noexcept {}

		template<flags_t flg> void _flagSet()noexcept { m_flags |= flg; }
		template<flags_t flg> void _flagClear()noexcept { m_flags &= (~flg); }
		template<flags_t flgSet, flags_t flgClr> void _flagSetClear()noexcept { m_flags = (m_flags & (~flgClr)) | flgSet; }
		template<flags_t flg>  bool _isFlagSet()const noexcept { return (m_flags & flg); }

	public:
		bool lastBarClosed()const noexcept { return _isFlagSet<_bit_lastBarClosed>(); }

	protected:
		void _setLastBarClosed_clearBarHasData()noexcept {
			_flagSetClear<_bit_lastBarClosed, _bit_barHasData>();
		}
		void _setLastBarClosed()noexcept { _flagSet<_bit_lastBarClosed>(); }

		void _clearLastBarClosed()noexcept { _flagClear<_bit_lastBarClosed>(); }

		void _setFlag_BarHasData()noexcept { _flagSet<_bit_barHasData>(); }
		void _clearFlag_BarHasData()noexcept { _flagClear<_bit_barHasData>(); }
		bool _BarHasData()const noexcept { return _isFlagSet<_bit_barHasData>(); }

		void _verifyLastBarClosed()const {
			if (UNLIKELY(!lastBarClosed())) {
				T18_ASSERT(!"Last bar must be closed here!");
				throw ::std::runtime_error("WTF? Last bar must be closed here!");
			}
		}
		void _verifyLastBarOpened()const {
			if (UNLIKELY(lastBarClosed())) {
				T18_ASSERT(!"Last bar must be opened here!");
				throw ::std::runtime_error("WTF? Last bar must be opened here!");
			}
		}

		void _verifyFlagSet_BarHasData()const {
			if (UNLIKELY(!_BarHasData())) {
				T18_ASSERT(!"Hey, newBarOpen() was called without accompanying aggregate()");
				throw ::std::runtime_error("Hey, newBarOpen() was called without accompanying aggregate()");
			}
		}

	public:
		//init() is a part of tfConverter's API.
		//Default implementation does nothing. Overrides may change function signature
		// type T is converter dependent
		/*template<typename T>
		void init(const T& lastKnownQuote)noexcept {}*/

	};

}
}

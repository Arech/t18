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

#include "tsTick.h"

namespace t18 {
	//the directed tick also holds information about deal initiator - either a buyer, or a seller
	//note: it seems a good idea to implement direction storage as a sign of volume field. However in that case
	// it would be wrong/impossible to cast tsDirTick to base class. And generally we aren't going to store too much
	// tsDirTick objects that the compression would matter (mass storaging, like timeframe class anyway uses array-style storage)
	struct tsDirTick : public tsTick {
	private:
		typedef tsTick base_class_t;

	public:
		typedef decltype(hana::make_map(
			hana::make_pair(timestamp_ht(), hana::type_c<mxTimestamp>)
			, hana::make_pair(quote_ht(), hana::type_c<real_t>)//the deal price
			, hana::make_pair(volume_ht(), hana::type_c<volume_t>)
			, hana::make_pair(bLong_ht(), hana::type_c<bool>)
		)) metaDescr_t;

	public:
		bool bLong;

		auto& bLng()noexcept { return bLong; }
		const auto& bLng()const noexcept { return bLong; }
		
		auto isLong()const noexcept { return bLong; }
		auto isShort()const noexcept { return !bLong; }

	public:

		tsDirTick()noexcept {}
		constexpr tsDirTick(mxTimestamp _ts, real_t _pr, volume_t _v, bool b) noexcept : base_class_t(_ts, _pr, _v), bLong(b) {}
		tsDirTick(tag_milDT, date_ult _d, time_ult _t, real_t _pr, volume_t _v, bool b) noexcept
			: base_class_t(tag_milDT(), _d, _t, _pr, _v), bLong(b)
		{}
		tsDirTick(tag_milDT, date_ult _d, time_ult _t, time_ult _mks, real_t _pr, volume_t _v, bool b) noexcept
			: base_class_t(tag_milDT(), _d, _t, _mks, _pr, _v), bLong(b)
		{}
		tsDirTick(milDate _d, milTime _t, real_t _pr, volume_t _v, bool b) noexcept : base_class_t(_d, _t, _pr, _v), bLong(b) {}
		tsDirTick(mxDate _d, mxTime _t, real_t _pr, volume_t _v, bool b) noexcept : base_class_t(_d, _t, _pr, _v), bLong(b) {}

		constexpr tsTick& TICK()noexcept { return *static_cast<tsTick*>(this); }
		constexpr const tsTick& TICK()const noexcept { return *static_cast<const tsTick*>(this); }

		//valid() and invalid() are inherited completely

		constexpr bool operator==(const tsDirTick& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TICK() == r.TICK() && bLong == r.bLong;
		}
		constexpr bool operator!=(const tsDirTick& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TICK() != r.TICK() || bLong != r.bLong;
		}

		void aggregate(const tsDirTick& from)noexcept {
			T18_ASSERT(valid());
			T18_ASSERT(from.valid());
			T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
			T18_ASSERT(q == from.q && isLong() == from.isLong());
			T18_COMP_POP;

			//T18_ASSERT(TS() <= from.TS());//this assert may fail in some valid situations

			v += from.v;
		}

		::std::string to_string()const {
			T18_ASSERT(valid());
			return dirTick2string(TS(), q, v, bLong);
		}

		static ::std::string dirTick2string(mxTimestamp _ts, real_t _pr, volume_t _v, bool b) {
			return base_class_t::tick2string(_ts, _pr, _v) + "," + (b ? "L" : "S");
		}

		constexpr decltype(auto) to_hmap()const noexcept {
			T18_ASSERT(valid());
			return hana::make_map(
				hana::make_pair(timestamp_ht(), TS())
				, hana::make_pair(quote_ht(), q)
				, hana::make_pair(volume_ht(), v)
				, hana::make_pair(bLong_ht(), bLong)
			);
		}

	};

}


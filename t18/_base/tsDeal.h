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

#include "tsDirTick.h"

namespace t18 {

	typedef decltype("num"_s) dealnum_ht;
	//seems to be enough
	typedef ::std::uint64_t dealnum_t;

	struct tsDeal : public tsDirTick {
	private:
		typedef tsDirTick base_class_t;

	public:
		typedef decltype(hana::make_map(
			hana::make_pair(timestamp_ht(), hana::type_c<mxTimestamp>)
			, hana::make_pair(quote_ht(), hana::type_c<real_t>)//the deal price
			, hana::make_pair(volume_ht(), hana::type_c<volume_t>)
			, hana::make_pair(dealnum_ht(), hana::type_c<dealnum_t>)
			, hana::make_pair(bLong_ht(), hana::type_c<bool>)
		)) metaDescr_t;

	public:
		dealnum_t dealNum;

		auto& Num()noexcept { return dealNum; }
		const auto& Num()const noexcept { return dealNum; }

	public:
		using base_class_t::bLong;

		tsDeal()noexcept {}
		constexpr tsDeal(mxTimestamp _ts, real_t _pr, volume_t _v, bool b, dealnum_t dn) noexcept
			: base_class_t(_ts, _pr, _v, b), dealNum(dn)
		{}
		tsDeal(tag_milDT, date_ult _d, time_ult _t, real_t _pr, volume_t _v, bool b, dealnum_t dn) noexcept
			: base_class_t(tag_milDT(), _d, _t, _pr, _v, b), dealNum(dn)
		{}
		tsDeal(tag_milDT, date_ult _d, time_ult _t, time_ult _mks, real_t _pr, volume_t _v, bool b, dealnum_t dn) noexcept
			: base_class_t(tag_milDT(), _d, _t, _mks, _pr, _v, b), dealNum(dn)
		{}
		tsDeal(milDate _d, milTime _t, real_t _pr, volume_t _v, bool b, dealnum_t dn) noexcept
			: base_class_t(_d, _t, _pr, _v, b), dealNum(dn)
		{}
		tsDeal(mxDate _d, mxTime _t, real_t _pr, volume_t _v, bool b, dealnum_t dn) noexcept
			: base_class_t(_d, _t, _pr, _v, b), dealNum(dn)
		{}

		constexpr tsDirTick& DIRTICK()noexcept { return *static_cast<tsDirTick*>(this); }
		constexpr const tsDirTick& DIRTICK()const noexcept { return *static_cast<const tsDirTick*>(this); }

		//valid() and invalid() are inherited completely

		constexpr bool operator==(const tsDeal& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return DIRTICK() == r.DIRTICK() && dealNum == r.dealNum;
		}
		constexpr bool operator!=(const tsDeal& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return DIRTICK() != r.DIRTICK() || dealNum != r.dealNum;
		}

		::std::string to_string()const {
			T18_ASSERT(valid());
			return deal2string(TS(), q, v, bLong, dealNum);
		}

		static ::std::string deal2string(mxTimestamp _ts, real_t _pr, volume_t _v, bool b, dealnum_t dn) {
			return base_class_t::dirTick2string(_ts, _pr, _v, b) + "," + ::std::to_string(dn);
		}

		constexpr decltype(auto) to_hmap()const noexcept {
			T18_ASSERT(valid());
			return hana::make_map(
				hana::make_pair(timestamp_ht(), TS())
				, hana::make_pair(quote_ht(), q)
				, hana::make_pair(volume_ht(), v)
				, hana::make_pair(dealnum_ht(), dealNum)
				, hana::make_pair(bLong_ht(), bLong)
			);
		}

	};


}
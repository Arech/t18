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

#include "tsq_data.h"

namespace t18 {

	//tsTick describes a single deal happened on a market (not knowing, who was the deal initiator - buyer or seller)
	struct tsTick : public tsq_data {
	private:
		typedef tsq_data base_class_t;

		//changing visibility
		using base_class_t::asLastQuote;
		//using base_class_t::makeLastQuote;
		using base_class_t::to_hmap;
		using base_class_t::valid;
		using base_class_t::invalid;

	public:
		typedef decltype(hana::make_map(
			hana::make_pair(timestamp_ht(), hana::type_c<mxTimestamp>)
			, hana::make_pair(quote_ht(), hana::type_c<real_t>)//the deal price
			, hana::make_pair(volume_ht(), hana::type_c<volume_t>)
		)) metaDescr_t;

	public:
		volume_t v;

		auto& vol()noexcept { return v; }
		const auto& vol()const noexcept { return v; }
		auto volume()const noexcept { return v; }

	public:
		tsTick()noexcept {}
		constexpr tsTick(mxTimestamp _ts, real_t _pr, volume_t _v) noexcept : base_class_t(_ts, _pr), v(_v) {}
		tsTick(tag_milDT, date_ult _d, time_ult _t, real_t _pr, volume_t _v) noexcept : base_class_t(tag_milDT(), _d, _t, _pr), v(_v) {}
		tsTick(tag_milDT, date_ult _d, time_ult _t, time_ult _mks, real_t _pr, volume_t _v) noexcept
			: base_class_t(tag_milDT(), _d, _t, _mks, _pr), v(_v)
		{}
		tsTick(milDate _d, milTime _t, real_t _pr, volume_t _v) noexcept : base_class_t(_d, _t, _pr), v(_v) {}
		tsTick(mxDate _d, mxTime _t, real_t _pr, volume_t _v) noexcept : base_class_t(_d, _t, _pr), v(_v) {}

		constexpr tsq_data& TSQ()noexcept { return *static_cast<tsq_data*>(this); }
		constexpr const tsq_data& TSQ()const noexcept { return *static_cast<const tsq_data*>(this); }

		constexpr bool valid() const noexcept { return base_class_t::valid() && v > 0; }
		constexpr bool invalid() const noexcept { return base_class_t::invalid() || v <= 0; }

		T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
		constexpr bool operator==(const tsTick& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TSQ() == r.TSQ() && v == r.v;
		}
		constexpr bool operator!=(const tsTick& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TSQ() != r.TSQ() || v != r.v;
		}
		T18_COMP_POP;


		void aggregate(const tsTick& from)noexcept {
			T18_ASSERT(valid());
			T18_ASSERT(from.valid());
			T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
			T18_ASSERT(q == from.q);
			T18_COMP_POP;

			//T18_ASSERT(TS() <= from.TS());//this assert may fail in some valid situations

			v += from.v;
		}


		::std::string to_string()const {
			T18_ASSERT(valid());
			return tick2string(TS(), q, v);
		}

		static ::std::string tick2string(mxTimestamp _ts, real_t _pr, volume_t _v) {
			return base_class_t::tsq2string(_ts, _pr) + "," + ::std::to_string(_v);
		}

		constexpr decltype(auto) to_hmap()const noexcept {
			T18_ASSERT(valid());
			return hana::make_map(
				hana::make_pair(timestamp_ht(), TS())
				, hana::make_pair(quote_ht(), q)
				, hana::make_pair(volume_ht(), v)
			);
		}
	};

	//for bid/ask prices & related info
	typedef tsTick BestPriceInfo;

}

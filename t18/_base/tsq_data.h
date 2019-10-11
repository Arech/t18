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

namespace t18 {

	struct tsq_data : public mxTimestamp {
	public:
		typedef decltype(hana::make_map(
			hana::make_pair(timestamp_ht(), hana::type_c<mxTimestamp>)
			, hana::make_pair(quote_ht(), hana::type_c<real_t>)
		)) metaDescr_t;

	public:
		//epsilon to safely compare price data. Value MUST be smaller than any possible valid price. Choose carefully
		static constexpr real_t _priceCmpEps = sizeof(real_t) > sizeof(int) ? 1e-16 : 1e-8;

	public:
		real_t q;

		bool samePriceEps(const real_t o, const real_t eps = _priceCmpEps)const noexcept {
			return ::std::abs(q - o) < eps;
		}

	public:
		constexpr tsq_data(mxTimestamp _ts, real_t _q) noexcept : mxTimestamp(_ts), q(_q) {}
		explicit constexpr tsq_data() noexcept : mxTimestamp(), q(0) {}

		tsq_data(tag_milDT, date_ult _d, time_ult _t, real_t _q) noexcept : mxTimestamp(tag_milDT(), _d, _t), q(_q) {}
		tsq_data(tag_milDT, date_ult _d, time_ult _t, time_ult _mks, real_t _q) noexcept
			: mxTimestamp(tag_milDT(), _d, _t, _mks), q(_q) {}

		tsq_data(milDate _d, milTime _t, real_t _q) noexcept : mxTimestamp(_d, _t), q(_q) {}
		tsq_data(mxDate _d, mxTime _t, real_t _q) noexcept : mxTimestamp(_d, _t), q(_q) {}

		constexpr mxTimestamp& TS()noexcept { return *static_cast<mxTimestamp*>(this); }
		constexpr const mxTimestamp& TS()const noexcept { return *static_cast<const mxTimestamp*>(this); }

		//using template to deal with (undefined at this moment) tsohlcv
		template<typename T, typename = ::std::enable_if_t<::std::is_same_v<T, tsohlcv>>>
		static constexpr tsq_data asLastQuote(const T& bar)noexcept {
			T18_ASSERT(bar.valid());
			return tsq_data(bar.TS().next(), bar.c);
		}

	protected:
		using mxTimestamp::empty;
		using mxTimestamp::clear;

	public:

		constexpr bool valid() const noexcept {
			return !mxTimestamp::empty() && q > 0;
		}
		constexpr bool invalid() const noexcept {
			return mxTimestamp::empty() || q <= 0;
		}

		T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;
		constexpr bool operator==(const tsq_data& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TS() == r.TS() && q == r.q;
		}
		constexpr bool operator!=(const tsq_data& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TS() != r.TS() || q != r.q;
		}
		T18_COMP_POP;

		::std::string to_string()const {
			T18_ASSERT(valid());
			return tsq2string(TS(), q);
		}

		static ::std::string tsq2string(mxTimestamp _ts, real_t _q) {
			return _ts.to_string() + "," + ::std::to_string(_q);
		}

		constexpr decltype(auto) to_hmap()const noexcept {
			T18_ASSERT(valid());
			return hana::make_map(
				hana::make_pair(timestamp_ht(), TS())
				, hana::make_pair(quote_ht(), q)
			);
		}
	};

}
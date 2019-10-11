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

#include "tsq_data.h"

namespace t18 {

	//"default" is a dedicated hlcv state that marks that only d,t and o data MIGHT be correct.
	//For the "outside" users of hlcv (except framework parts like timeframe converters), the "default"
	// means that only d,t and o data fields ARE correct.
	struct tag_ohlcvDefault {};

	struct hlcv {
		typedef decltype(hana::make_map(
			hana::make_pair(high_ht(), hana::type_c<real_t>)
			, hana::make_pair(low_ht(), hana::type_c<real_t>)
			, hana::make_pair(close_ht(), hana::type_c<real_t>)
			, hana::make_pair(volume_ht(), hana::type_c<volume_t>)
		)) metaDescrUpdate_t;

		static constexpr real_t _LowInitializer = ::std::numeric_limits<real_t>::max();
		static constexpr real_t _HighInitializer = ::std::numeric_limits<real_t>::lowest();

		real_t h, l, c;
		volume_t v;

	protected:
		hlcv()noexcept {}
		constexpr hlcv(real_t _h, real_t _l, real_t _c, volume_t _v) noexcept : h(_h), l(_l), c(_c), v(_v) {}
		constexpr hlcv(tag_ohlcvDefault)noexcept : h(_HighInitializer), l(_LowInitializer), c(0), v(0) {}

		/*constexpr void _set_default_hlcv()noexcept {
		h = _HighInitializer;
		l = _LowInitializer;
		c = 0;
		v = 0;
		}*/

	public:

		//checks everything
		constexpr bool valid() const noexcept { return h > 0 && l > 0 && l < _LowInitializer && c <= h && c >= l && v > 0; }
		constexpr bool invalid() const noexcept { return h <= 0 || l <= 0 || c < 0 || c<l || c>h || v <= 0; }

		//checks everything but volume could be zero
		constexpr bool hlc_valid() const noexcept { return h > 0 && l > 0 && l < _LowInitializer && c <= h && c >= l && v >= 0; }
		constexpr bool hlc_invalid() const noexcept { return h <= 0 || l <= 0 || c < 0 || c<l || c>h || v < 0; }

		T18_COMP_SILENCE_FLOAT_CMP_UNSAFE;

		/*constexpr bool isDefaulted()const noexcept {
		return h == _HighInitializer T18_DEBUG_ONLY(&& l==_LowInitializer);
		}*/
		//#NOTE: floating point equality comparison here!
		constexpr bool operator==(const hlcv& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return h == r.h && l == r.l && c == r.c && v == r.v;
		}
		constexpr bool operator!=(const hlcv& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return h != r.h || l != r.l || c != r.c || v != r.v;
		}
		T18_COMP_POP;

		::std::string to_string()const {
			T18_ASSERT(valid());
			return hlcv2string(h, l, c, v);
		}

		static ::std::string hlcv2string(real_t _h, real_t _l, real_t _c, volume_t _v) {
			return ::std::to_string(_h) + "," + ::std::to_string(_l)
				+ "," + ::std::to_string(_c) + "," + ::std::to_string(_v);
		}

		constexpr decltype(auto) data2update_to_hmap()const noexcept {
			T18_ASSERT(valid());
			return hana::make_map(
				hana::make_pair(high_ht(), h)
				, hana::make_pair(low_ht(), l)
				, hana::make_pair(close_ht(), c)
				, hana::make_pair(volume_ht(), v)
			);
		}
	};

	//////////////////////////////////////////////////////////////////////////

	struct tsohlcv : public tsq_data, public hlcv {
		typedef decltype(hana::make_map(
			hana::make_pair(timestamp_ht(), hana::type_c<mxTimestamp>)
			, hana::make_pair(open_ht(), hana::type_c<real_t>)
			, hana::make_pair(high_ht(), hana::type_c<real_t>)
			, hana::make_pair(low_ht(), hana::type_c<real_t>)
			, hana::make_pair(close_ht(), hana::type_c<real_t>)
			, hana::make_pair(volume_ht(), hana::type_c<volume_t>)
		)) metaDescr_t;

		typedef tag_ohlcvDefault tag_Default_t;

	private:
		typedef tsq_data base_tsq_t;
		typedef hlcv base_hlcv_t;

		//changing visibility
		using base_tsq_t::asLastQuote;
		using base_tsq_t::to_hmap;
		using base_tsq_t::valid;
		using base_tsq_t::invalid;

		using base_hlcv_t::valid;
		using base_hlcv_t::invalid;
		using base_hlcv_t::hlc_valid;
		using base_hlcv_t::hlc_invalid;

		//changing the scope of q variable to rename it to open
		using base_tsq_t::q;

	public:
		//short form returns reference, long form - value.
		auto& o()noexcept { return base_tsq_t::q; }
		const auto& o()const noexcept { return base_tsq_t::q; }

		auto open()const noexcept { return o(); }

		//////////////////////////////////////////////////////////////////////////
		constexpr tsohlcv(mxTimestamp _ts, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) noexcept
			: base_tsq_t(_ts, _o), base_hlcv_t(_h, _l, _c, _v)
		{}
		tsohlcv(tag_milDT, date_ult _d, time_ult _t, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) noexcept
			: base_tsq_t(tag_milDT(), _d, _t, _o), base_hlcv_t(_h, _l, _c, _v)
		{}
		tsohlcv(tag_milDT, date_ult _d, time_ult _t, time_ult _mks, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) noexcept
			: base_tsq_t(tag_milDT(), _d, _t, _mks, _o), base_hlcv_t(_h, _l, _c, _v)
		{}

		tsohlcv(milDate _d, milTime _t, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) noexcept
			: base_tsq_t(_d, _t, _o), base_hlcv_t(_h, _l, _c, _v)
		{}
		tsohlcv(mxDate _d, mxTime _t, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) noexcept
			: base_tsq_t(_d, _t, _o), base_hlcv_t(_h, _l, _c, _v)
		{}

		tsohlcv(mxTimestamp _ts) noexcept : base_tsq_t(_ts, 0), base_hlcv_t() {}

		tsohlcv(mxTimestamp _ts, const tsohlcv& _bar) noexcept : base_tsq_t(_ts, _bar.o()), base_hlcv_t(_bar.HLCV()) {}
		//tsohlcv(mxTimestamp _ts, real_t _o, const hlcv& _hlcv) noexcept : base_tsq_t(_ts, _o), base_hlcv_t(_hlcv) {}
		//tsohlcv(const tsq_data& _tsq, const hlcv& _hlcv) noexcept : base_tsq_t(_tsq), base_hlcv_t(_hlcv) {}

		tsohlcv()noexcept : base_tsq_t(), base_hlcv_t() {}
		constexpr tsohlcv(tag_Default_t) noexcept : base_tsq_t(), base_hlcv_t(tag_Default_t()) {}

	public:
		constexpr tsq_data& TSQ()noexcept { return *static_cast<tsq_data*>(this); }
		constexpr const tsq_data& TSQ()const noexcept { return *static_cast<const tsq_data*>(this); }
		constexpr hlcv& HLCV()noexcept { return *static_cast<hlcv*>(this); }
		constexpr const hlcv& HLCV()const noexcept { return *static_cast<const hlcv*>(this); }

		//checks everything
		constexpr bool valid() const noexcept { return base_tsq_t::valid() && base_hlcv_t::valid() && o() <= h && o() >= l; }
		constexpr bool invalid() const noexcept { return base_tsq_t::invalid() || base_hlcv_t::invalid() || o() > h || o() < l; }

		//checks everything but volume
		constexpr bool ohlc_valid() const noexcept { return base_tsq_t::valid() && base_hlcv_t::hlc_valid() && o() <= h && o() >= l; }
		constexpr bool ohlc_invalid() const noexcept { return base_tsq_t::invalid() || base_hlcv_t::hlc_invalid() || o() > h || o() < l; }

		/*constexpr void reset(mxTimestamp _ts, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v)noexcept {
		TS() = _ts;
		o = _o;
		h = _h;
		l = _l;
		c = _c;
		v = _v;
		}*/

		template<typename TsTransformT /*= TimestampTFMinutes*/>
		constexpr void openedFrom(const tsq_data& dto, TsTransformT& tstrans)noexcept {
			T18_ASSERT(dto.valid());
			TS() = tstrans.adjust(dto.TS());
			const auto nq = dto.q;
			o() = nq; h = nq; l = nq; c = nq;
			v = 0;
		}

		template<typename TsTransformT /*= TimestampTFMinutes*/>
		constexpr void openedFrom(const tsTick& tst, TsTransformT& tstrans)noexcept {
			T18_ASSERT(tst.valid());
			TS() = tstrans.adjust(tst.TS());
			const auto nq = tst.q;
			o() = nq; h = nq; l = nq; c = nq;
			v = tst.v;
		}

		void aggregate(const tsohlcv& from)noexcept {
			T18_ASSERT(from.valid());
			T18_ASSERT(ohlc_valid());
			h = ::std::max(h, from.h);
			l = ::std::min(l, from.l);
			c = from.c;
			v += from.v;
		}
		void aggregate(const tsTick& from)noexcept {
			T18_ASSERT(from.valid());
			T18_ASSERT(valid());//more strict because openedFrom(tsTick) sets the volume field
			const auto nq = from.q;
			h = ::std::max(h, nq);
			l = ::std::min(l, nq);
			c = nq;
			v += from.v;
		}


		constexpr bool operator==(const tsohlcv& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TSQ() == r.TSQ() && HLCV() == r.HLCV();
		}
		constexpr bool operator!=(const tsohlcv& r)const noexcept {
			T18_ASSERT(valid() && r.valid());
			return TSQ() != r.TSQ() || HLCV() != r.HLCV();
		}

		::std::string to_string()const {
			T18_ASSERT(valid());
			return bar2string(TS(), o(), h, l, c, v);
		}

		::std::string ohlcv_to_string()const {
			T18_ASSERT(valid());
			return ::std::to_string(o()) + "," + HLCV().to_string();
		}

		static ::std::string bar2string(mxTimestamp _ts, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) {
			return base_tsq_t::tsq2string(_ts, _o) + "," + base_hlcv_t::hlcv2string(_h, _l, _c, _v);
		}

		static ::std::string bar2string(mxDate _d, mxTime _t, real_t _o, real_t _h, real_t _l, real_t _c, volume_t _v) {
			return bar2string(mxTimestamp(_d, _t), _o, _h, _l, _c, _v);
		}

		constexpr decltype(auto) to_hmap()const noexcept {
			T18_ASSERT(ohlc_valid());//volume could be zero
			return hana::make_map(
				hana::make_pair(timestamp_ht(), TS())
				, hana::make_pair(open_ht(), o())
				, hana::make_pair(high_ht(), h)
				, hana::make_pair(low_ht(), l)
				, hana::make_pair(close_ht(), c)
				, hana::make_pair(volume_ht(), v)
			);
		}
	};

}



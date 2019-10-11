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

#include <ctime>
#include "_dt_impl.h"

namespace t18 {

	template<bool bDoRTChecks = true>
	struct _mxTimestamp {
	public:
		typedef _dt_impl::timestampBits bits;
		typedef bits::Offs<tag_mxTimestamp, !bDoRTChecks> impl;

	protected:
		timestamp_ult val;

	public:
		constexpr _mxTimestamp() noexcept : val(impl::emptyValue) {}

		//special constructor that helps to drop .isEmpty checks. Use with care
		constexpr _mxTimestamp(tag_mxTimestamp) noexcept : val(0) {
			static_assert(0 != impl::emptyValue, "");
		}
		explicit constexpr _mxTimestamp(timestamp_ult v) noexcept : val(v) {}
		constexpr _mxTimestamp(int y, int m, int d, int h, int mn, int s, int mks)noexcept
			: val(impl::to_mxtimestamp(y, m, d, h, mn, s, mks)) {}
		constexpr _mxTimestamp(int y, int m, int d, int h, int mn, int s)noexcept
			: val(impl::to_mxtimestamp(y, m, d, h, mn, s)) {}

		_mxTimestamp(tag_milDT, date_ult milD, time_ult milT)noexcept {
			int y, m, d, h, mn, s;
			milDate::untie_mildate(milD, y, m, d);
			milTime::untie_miltime(milT, h, mn, s);
			val = impl::to_mxtimestamp(y, m, d, h, mn, s);
		}
		_mxTimestamp(tag_milDT, date_ult milD, time_ult milT, time_ult mks)noexcept {
			int y, m, d, h, mn, s;
			milDate::untie_mildate(milD, y, m, d);
			milTime::untie_miltime(milT, h, mn, s);
			val = impl::to_mxtimestamp(y, m, d, h, mn, s, static_cast<int>(mks));
		}
		_mxTimestamp(milDate milD, milTime milT)noexcept : _mxTimestamp(tag_milDT(), milD._get(), milT._get()) {}
		_mxTimestamp(milDate milD, milTime milT, time_ult mks)noexcept : _mxTimestamp(tag_milDT(), milD._get(), milT._get(), mks) {}

		constexpr _mxTimestamp(tag_mxDT, date_ult mxD, time_ult mxT)noexcept : val(impl::to_mxtimestamp(mxD, mxT)) {}
		constexpr _mxTimestamp(mxDate mxD, mxTime mxT)noexcept : val(impl::to_mxtimestamp(mxD._get(), mxT._get())) {}

		void set(int y, int m, int d, int h = 0, int mn = 0, int s = 0, int mks = 0)noexcept {
			val = impl::to_mxtimestamp(y, m, d, h, mn, s, mks);
		}

		constexpr bool empty()const noexcept { return impl::isEmpty(val); }
		void clear()noexcept { val = impl::emptyValue; }

		constexpr timestamp_ult _get()const noexcept {
			T18_ASSERT(!empty());
			return val;
		}
		//for special use only
		timestamp_ult& _getref()noexcept {
			return val;
		}

		static constexpr _mxTimestamp max()noexcept {
			return _mxTimestamp(impl::maxValue);
		}

		constexpr bool operator==(_mxTimestamp r)const {
			T18_ASSERT(!empty() && !r.empty());
			return val == r.val;
		}
		constexpr bool operator!=(_mxTimestamp r)const {
			T18_ASSERT(!empty() && !r.empty());
			return val != r.val;
		}

		friend constexpr bool operator<(_mxTimestamp l, _mxTimestamp r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val < r.val;
		}
		friend constexpr bool operator>(_mxTimestamp l, _mxTimestamp r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val > r.val;
		}
		friend constexpr bool operator<=(_mxTimestamp l, _mxTimestamp r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val <= r.val;
		}
		friend constexpr bool operator>=(_mxTimestamp l, _mxTimestamp r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val >= r.val;
		}

		constexpr int Microsecond() const noexcept {
			T18_ASSERT(!empty());
			return impl::Microsecond(val);
		}
		constexpr int Second() const noexcept {
			T18_ASSERT(!empty());
			return impl::Second(val);
		}
		constexpr int Minute() const noexcept {
			T18_ASSERT(!empty());
			return impl::Minute(val);
		}
		constexpr int Hour() const noexcept {
			T18_ASSERT(!empty());
			return impl::Hour(val);
		}
		constexpr int Day() const noexcept {
			T18_ASSERT(!empty());
			return impl::Day(val);
		}
		constexpr int Month() const noexcept {
			T18_ASSERT(!empty());
			return impl::Month(val);
		}
		constexpr int Year() const noexcept {
			T18_ASSERT(!empty());
			return impl::Year(val);
		}

		/* could be made more efficient
		*void untie(int& y, int& m, int& d, int& h, int& mn, int& s, int& mks)const noexcept {
		y = Year();
		m = Month();
		d = Day();
		h =
		}*/

		constexpr mxDate Date()const noexcept {
			T18_ASSERT(!empty());
			return mxDate(tag_mxDate(), impl::mxdate(val));
		}
		constexpr mxTime Time()const noexcept {
			T18_ASSERT(!empty());
			return mxTime(tag_mxTime(), impl::mxtime(val));
		}

		constexpr int MinuteOfDay()const noexcept {
			T18_ASSERT(!empty());
			return impl::Hour(val) * 60 + impl::Minute(val);
		}
		constexpr int SecondOfDay()const noexcept {
			T18_ASSERT(!empty());
			return impl::Hour(val) * (60 * 60) + impl::Minute(val) * 60 + impl::Second(val);
		}

		//////////////////////////////////////////////////////////////////////////

		void set_time(int h, int m, int s, int mks)noexcept { val = impl::set_time(val, h, m, s, mks); }
		void set_StartOfDay()noexcept { val = impl::set_time(val, 0u, 0); }

		//////////////////////////////////////////////////////////////////////////
		static _mxTimestamp now()noexcept {
			const auto t = ::std::time(nullptr);
			if (LIKELY(static_cast<::std::time_t>(-1) != t)) {
				::std::tm tms;
				if (LIKELY(0 == ::localtime_s(&tms, &t))) {
					return _mxTimestamp(pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday
						, pTM->hour, pTM->tm_min, pTM->tm_sec);
				}
			}
			return _mxTimestamp();
		}

		//////////////////////////////////////////////////////////////////////////
		friend constexpr _mxTimestamp latest(_mxTimestamp t1, _mxTimestamp t2)noexcept {
			T18_ASSERT(!t1.empty() && !t2.empty());
			return t1.val > t2.val ? t1 : t2;
		}
		friend constexpr _mxTimestamp earliest(_mxTimestamp t1, _mxTimestamp t2)noexcept {
			T18_ASSERT(!t1.empty() && !t2.empty());
			return t1.val < t2.val ? t1 : t2;
		}

		//////////////////////////////////////////////////////////////////////////
		constexpr _mxTimestamp TFLowerBoundary(int mTf)const noexcept {
			return _mxTimestamp(impl::TFLowerBoundary(mTf, val));
		}

		//////////////////////////////////////////////////////////////////////////

		constexpr _mxTimestamp next()const noexcept {
			T18_ASSERT(!empty());
			return _mxTimestamp(impl::next(val));
		}

		constexpr _mxTimestamp nextMinuteStart()const noexcept {
			T18_ASSERT(!empty());
			return _mxTimestamp(impl::nextMinuteStart(val));
		}
		constexpr _mxTimestamp nextDayStart()const noexcept {
			T18_ASSERT(!empty());
			return _mxTimestamp(impl::nextDayStart(val));
		}

		constexpr _mxTimestamp plusYear()const noexcept {
			T18_ASSERT(Year() < bits::maxYear);
			return _mxTimestamp(impl::plusYear(val));
		}

		//shouldn't make operator-/+/etc to underscore that it's just a surrogate
		constexpr timestamp_diff_t uglyDiffMks(_mxTimestamp ts2)const {
			return impl::uglyDiffMks(val, ts2.val);
		}
		constexpr timestamp_diff_t _yearTime(bool bLeapYear = false)const noexcept {
			return impl::_yearTime(val, bLeapYear);
		}

		constexpr bool isStartOfDay()const noexcept {
			return 0 == Microsecond() && 0 == Time()._get();
		}

		::std::string to_string()const {
			T18_ASSERT(!empty());
			char buf[256];
			if (0 == val) {
				buf[0] = '0';
				buf[1] = 0;
			} else {				
				const auto mks = Microsecond();
				const auto dat = Date().to_mil()._get();
				const auto tme = Time().to_mil()._get();
				if (mks > 0) {
					sprintf_s(buf, "%u,%u.%06d", dat, tme, mks);
				} else sprintf_s(buf, "%u,%u", dat, tme);
			}
			return ::std::string(buf);
		}

		void sprintf_time(char* dest, const size_t maxDestLen)const noexcept {
			::sprintf_s(dest, maxDestLen, "%u.%06d", Time().to_timenum(), Microsecond());
		}
		bool sscanf_time(const char* pTime) {
			T18_ASSERT(!empty());
			T18_ASSERT(pTime);

			int mks, h, m, s;
			if (*pTime == 0) {
				mks = 0;				h = 0;				m = 0;				s = 0;
			} else {
				time_ult tn;
				const auto n = sscanf_s(pTime, "%u.%d", &tn, &mks);
				if (n != 2 || !milTime::untie_miltime_s(tn, h, m, s)) return false;
			}
			set_time(h, m, s, mks);
			return true;
		}

		void sprintf_full(char*dest, const size_t ml)const noexcept {
			::sprintf_s(dest, ml, "%I64x", val);
		}
		bool sscanf_full(const char* pStr)noexcept {
			T18_ASSERT(!empty());
			T18_ASSERT(pStr);

			if (*pStr == 0) {
				//no update by intent!
				//val = 0;
			} else {
				timestamp_ult v;
				const auto n = sscanf_s(pStr, "%I64x", &v);
				if (n != 1) return false;
				val = v;
			}
			return true;
		}
	};

	typedef _mxTimestamp<true> mxTimestamp;

	//////////////////////////////////////////////////////////////////////////

	//struct to hold current timeframe in minutes as well as the timestamp of next period boundary
	struct TimestampTFMinutes {
	protected:
		typedef mxTimestamp::impl impl;

	protected:
		mxTimestamp m_upperBnd;
		int m_tfMins;

	public:
		explicit TimestampTFMinutes(int m) :m_upperBnd(), m_tfMins(m) {
			if (!isGoodTfMinutes(m)) {
				T18_ASSERT(!"Invalid timeframe passed!");
				throw ::std::logic_error("Invalid timeframe passed!");
			}
		}

		static constexpr bool isGoodTfMinutes(int m)noexcept {
			return impl::isGoodTfMinutes(m);
		}

		auto tf()const noexcept { return m_tfMins; }
		mxTimestamp upperBoundTimestamp()const noexcept {
			T18_ASSERT(!m_upperBnd.empty());
			return m_upperBnd;
		}

		decltype(auto) adjust(mxTimestamp ts)noexcept {
			return mxTimestamp(impl::TFBoundaries(m_tfMins, ts._get(), m_upperBnd._getref()));
		}

		bool notInialized()const noexcept {
			return m_upperBnd.empty();
		}
	};

}

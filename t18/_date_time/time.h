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

#include "_dt_impl.h"

namespace t18 {

	struct milTime {
	protected:
		time_ult val;

	public:
		explicit constexpr milTime(time_ult _v)noexcept : val(_v) {}
		constexpr milTime(int h, int m, int s)noexcept : val(to_miltime(h, m, s)) {}
		constexpr auto _get()const noexcept { return val; }

		::std::string to_string()const { return ::std::to_string(val); }

		constexpr bool operator==(milTime r)const noexcept { return val == r.val; }
		constexpr bool operator!=(milTime r)const noexcept { return val != r.val; }
		friend bool operator==(mxTime l, milTime r)noexcept;
		friend bool operator==(milTime l, mxTime r)noexcept;
		friend bool operator!=(mxTime l, milTime r)noexcept;
		friend bool operator!=(milTime l, mxTime r)noexcept;

		static constexpr time_ult to_miltime(int h, int m, int s)noexcept {
			T18_ASSERT(s >= 0 && s <= 59);
			T18_ASSERT(m >= 0 && m <= 59);
			T18_ASSERT(h >= 0 && h <= 23);
			return static_cast<time_ult>(s) + static_cast<time_ult>(m) * static_cast<time_ult>(100)
				+ static_cast<time_ult>(h) * static_cast<time_ult>(10000);
		}

		static void untie_miltime(time_ult _milT, int& h, int& m, int& s)noexcept {
			const auto milT = static_cast<::std::make_signed_t<time_ult>>(_milT);
			T18_ASSERT(milT >= 0);
			s = milT % 100;
			T18_ASSERT(s >= 0 && s <= 59);
			const auto dr = ::std::div(milT, 10000);
			m = dr.rem / 100;
			T18_ASSERT(m >= 0 && m <= 59);
			h = dr.quot;
			T18_ASSERT(h >= 0 && h <= 23);
		}
		static bool untie_miltime_s(time_ult _milT, int& h, int& m, int& s)noexcept {
			const auto milT = static_cast<::std::make_signed_t<time_ult>>(_milT);
			if (milT < 0) return false;
			s = milT % 100;
			if (s < 0 || s > 59) return false;
			const auto dr = ::std::div(milT, 10000);
			m = dr.rem / 100;
			if (m < 0 || m > 59) return false;
			h = dr.quot;
			return (h >= 0 && h <= 23);
		}
	};
	static_assert(sizeof(time_ult) == sizeof(milTime), "");

	struct mxTime {
	public:
		typedef _dt_impl::timestampBits::Offs<tag_mxTime> impl;
		static constexpr int minutesInDay = 24 * 60;
		static constexpr int secondsInDay = 24 * 60 * 60;

	protected:
		time_ult val;

	public:
		constexpr mxTime()noexcept : val(impl::emptyValue) {}

		//special constructor that helps to drop .isEmpty checks.
		//Also makes a perfectly valid 00:00:00 time
		constexpr mxTime(tag_mxTime) noexcept : val(0) {
			static_assert(0 != impl::emptyValue, "");
		}

		mxTime(tag_milTime, time_ult _v) noexcept : val(miltime2mxtime(_v)) {}
		constexpr mxTime(tag_mxTime, time_ult _v)noexcept : val(_v) {}
		constexpr mxTime(int h, int m, int s)noexcept : val(impl::to_mxtime(h, m, s)) {}

		explicit mxTime(milTime _v)noexcept : mxTime(tag_milTime(), _v._get()) {}

		constexpr bool empty()const noexcept { return impl::isEmpty(val); }
		void clear()noexcept { val = impl::emptyValue; }

		constexpr time_ult _get()const noexcept {
			T18_ASSERT(!empty());
			return val;
		}

		constexpr bool operator==(mxTime  r)const noexcept {
			T18_ASSERT(!empty() && !r.empty());
			return val == r.val;
		}
		constexpr bool operator!=(mxTime  r)const noexcept {
			T18_ASSERT(!empty() && !r.empty());
			return val != r.val;
		}
		friend bool operator==(mxTime l, milTime r)noexcept;
		friend bool operator==(milTime l, mxTime r)noexcept;
		friend bool operator!=(mxTime l, milTime r)noexcept;
		friend bool operator!=(milTime l, mxTime r)noexcept;

		friend constexpr bool operator<(mxTime l, mxTime r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val < r.val;
		}
		friend constexpr bool operator>(mxTime l, mxTime r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val > r.val;
		}
		friend constexpr bool operator<=(mxTime l, mxTime r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val <= r.val;
		}
		friend constexpr bool operator>=(mxTime l, mxTime r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val >= r.val;
		}

		//#TODO: BAD ARITHMETIC!
		//#TODO change to ugly() function
		friend mxTime operator+(mxTime l, mxTime r)noexcept {
			auto sm = l.SecondOfDay() + r.SecondOfDay();
			T18_ASSERT(sm < secondsInDay);
			return mxTime(tag_mxTime(), SecondOfDay2mxtime(sm));
		}
		friend mxTime operator-(mxTime l, mxTime r)noexcept {
			auto lsod = l.SecondOfDay(), rsod = r.SecondOfDay();
			T18_ASSERT(lsod >= rsod);
			return mxTime(tag_mxTime(), SecondOfDay2mxtime(lsod - rsod));
		}

		//friend mxTime operator+(const mxTime&l, int r)noexcept { return l + mxTime(tag_mxTime(), SecondOfDay2mxtime(r)); }
		//friend mxTime operator-(const mxTime&l, int r)noexcept { return l - mxTime(tag_mxTime(), SecondOfDay2mxtime(r)); }

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

		/* could be made more efficient
		void untie(int& h, int& m, int& s)const noexcept {
		h = Hour();
		m = Minute();
		s = Second();
		}*/

		constexpr int MinuteOfDay() const noexcept {
			T18_ASSERT(!empty());
			return MinuteOfDay(val);
		}
		constexpr int SecondOfDay() const noexcept {
			T18_ASSERT(!empty());
			return SecondOfDay(val);
		}

		constexpr milTime to_mil()const noexcept {
			T18_ASSERT(!empty());
			return milTime(mxtime2miltime(val));
		}

		//should be optimized
		//constexpr mxTime newDropSeconds()const noexcept { return mxTime(Hour(), Minute(), 0); }
		//constexpr milTime to_milDroppedSeconds()const noexcept { return milTime(Hour(), Minute(), 0); }

		// time coded as follows: 	10000 * hour + 100 * minute + second, so 12:37 : 15 becomes 123715
		//same, as miltime
		constexpr time_ult to_timenum()const noexcept {
			T18_ASSERT(!empty());
			return mxtime2miltime(val);
		}

		::std::string to_string()const { return to_mil().to_string(); }

		/////////////////////////////////////////////////////////////////////////////

		static time_ult miltime2mxtime(time_ult _milT)noexcept {
			/*const auto milT = static_cast<::std::make_signed_t<time_ult>>(_milT);
			T18_ASSERT(milT >= 0);
			const int s = milT % 100;
			const auto dr = ::std::div(milT, 10000);
			const int m = dr.rem / 100;
			return impl::to_mxtime(dr.quot, m, s);*/
			int h, m, s;
			milTime::untie_miltime(_milT, h, m, s);
			return impl::to_mxtime(h, m, s);
		}

		static constexpr time_ult mxtime2miltime(time_ult mxT) noexcept {
			return static_cast<time_ult>(impl::Second(mxT)) + static_cast<time_ult>(100) * static_cast<time_ult>(impl::Minute(mxT))
				+ static_cast<time_ult>(10000) * static_cast<time_ult>(impl::Hour(mxT));
		}

		static constexpr int MinuteOfDay(time_ult mxT) noexcept {
			return impl::Hour(mxT) * 60 + impl::Minute(mxT);
		}
		static constexpr int SecondOfDay(time_ult mxT) noexcept {
			return impl::Hour(mxT) * (60 * 60) + impl::Minute(mxT) * 60 + impl::Second(mxT);
		}

		static time_ult SecondOfDay2mxtime(int sod)noexcept {
			T18_ASSERT(sod >= 0);
			const int s = sod % 60;
			const auto dr = ::std::div(sod, 60 * 60);
			const int m = dr.rem / 60;
			return impl::to_mxtime(dr.quot, m, s);
		}
	};
	static_assert(sizeof(time_ult) == sizeof(mxTime), "");

	inline bool operator==(mxTime l, milTime r)noexcept { return l == mxTime(r); }
	inline bool operator==(milTime l, mxTime r)noexcept { return mxTime(l) == r; }
	inline bool operator!=(mxTime l, milTime r)noexcept { return !(l == r); }
	inline bool operator!=(milTime l, mxTime r)noexcept { return !(l == r); }

}

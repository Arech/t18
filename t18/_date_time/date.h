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

#include "_dt_impl.h"

namespace t18 {

	struct milDate {
	protected:
		date_ult val;

	public:
		explicit constexpr milDate(date_ult _v)noexcept : val(_v) {}
		constexpr auto _get()const noexcept { return val; }

		::std::string to_string() const { return ::std::to_string(val); }

		constexpr bool operator==(milDate r)const noexcept { return val == r.val; }
		constexpr bool operator!=(milDate r)const noexcept { return val != r.val; }
		friend bool operator==(mxDate l, milDate r)noexcept;
		friend bool operator==(milDate l, mxDate r)noexcept;
		friend bool operator!=(mxDate l, milDate r)noexcept;
		friend bool operator!=(milDate l, mxDate r)noexcept;

		static void untie_mildate(date_ult _milV, int& y, int& m, int& d)noexcept {
			const auto milV = static_cast<::std::make_signed_t<date_ult>>(_milV);
			T18_ASSERT(milV > 0);
			d = milV % 100;
			T18_ASSERT(d > 0 && d <= 31);
			const auto dr = ::std::div(milV, 10000);
			m = dr.rem / 100;
			T18_ASSERT(m > 0 && m <= 12);
			y = dr.quot;
			T18_ASSERT(y > 1900 && y < 2100);
		}
	};
	static_assert(sizeof(date_ult) == sizeof(milDate), "");


	//converts date in military format (yyyymmdd) which is easy to read in decimal to hex-military format
	//which is easy&fast to process for computer. Spec is, in general, the same  (yyyymmdd), however, the parts are
	// just shifted to byte boundaries: dd now sits in the first byte, mm in the second, while yyyy resides in the last two
	struct mxDate {
	public:
		typedef _dt_impl::timestampBits::Offs<tag_mxDate> impl;

	protected:
		date_ult val;

	public:
		constexpr mxDate() noexcept : val(impl::emptyValue) {}

		mxDate(tag_milDate, date_ult _v)noexcept : val(mildate2mxdate(_v)) {}
		constexpr mxDate(tag_mxDate, date_ult _v)noexcept : val(_v) {}

		constexpr mxDate(int y, int m, int d)noexcept : val(impl::to_mxdate(y, m, d)) {}

		explicit mxDate(milDate _v)noexcept : mxDate(tag_milDate(), _v._get()) {}

		constexpr bool empty()const noexcept { return impl::isEmpty(val); }
		void clear()noexcept { val = impl::emptyValue; }

		constexpr date_ult _get()const noexcept {
			T18_ASSERT(!empty());
			return val;
		}

		static constexpr mxDate max()noexcept {
			return mxDate(_dt_impl::timestampBits::maxYear, 12, 31);
		}

		//let's allow left operand to be empty.
		constexpr bool operator==(mxDate r)const noexcept {
			T18_ASSERT(/*!empty() &&*/ !r.empty());
			return val == r.val;
		}
		constexpr bool operator!=(mxDate r)const noexcept {
			T18_ASSERT(/*!empty() &&*/ !r.empty());
			return val != r.val;
		}
		friend bool operator==(mxDate l, milDate r)noexcept;
		friend bool operator==(milDate  l, mxDate r)noexcept;
		friend bool operator!=(mxDate l, milDate r)noexcept;
		friend bool operator!=(milDate  l, mxDate r)noexcept;

		friend constexpr bool operator<(mxDate l, mxDate r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val < r.val;
		}
		friend constexpr bool operator>(mxDate l, mxDate r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val > r.val;
		}
		friend constexpr bool operator<=(mxDate l, mxDate r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val <= r.val;
		}
		friend constexpr bool operator>=(mxDate l, mxDate r)noexcept {
			T18_ASSERT(!l.empty() && !r.empty());
			return l.val >= r.val;
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
		void untie(int& y, int& m, int& d)const noexcept {
		y = Year();
		m = Month();
		d = Day();
		}*/

		static mxDate now()noexcept {
			const auto t = ::std::time(nullptr);
			if (LIKELY(static_cast<::std::time_t>(-1) != t)) {
				::std::tm tms;
				if (LIKELY(0 == ::localtime_s(&tms, &t))) {
					return mxDate(tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday);
				}
			}
			return mxDate();
		}

		constexpr milDate to_mil()const noexcept {
			T18_ASSERT(!empty());
			return milDate(mxdate2mildate(val));
		}

		constexpr mxDate plusYear()const noexcept {
			T18_ASSERT(!empty());
			return mxDate(tag_mxDate(), val + static_cast<date_ult>(1 << impl::ofsYear));
		}

		//dates coded as follows: 
		//10000 * (year - 1900) + 100 * month + day, so 2001 - 12 - 31 becomes 1011231 and 1995 - 12 - 31 becomes 951231
		date_ult to_datenum()const noexcept {
			return static_cast<date_ult>(10000)*static_cast<date_ult>(Year() - 1900)
				+ static_cast<date_ult>(100)*static_cast<date_ult>(Month()) + static_cast<date_ult>(Day());
		}

		::std::string to_string()const { return to_mil().to_string(); }

		static date_ult mildate2mxdate(date_ult _milV)noexcept {
			int y, m, d;
			milDate::untie_mildate(_milV, y, m, d);
			return impl::to_mxdate(y, m, d);
		}
		static constexpr date_ult mxdate2mildate(date_ult mxD) noexcept {
			return static_cast<date_ult>(impl::Day(mxD)) + static_cast<date_ult>(100) * static_cast<date_ult>(impl::Month(mxD))
				+ static_cast<date_ult>(10000) * static_cast<date_ult>(impl::Year(mxD));
		}
	};
	static_assert(sizeof(date_ult) == sizeof(mxDate), "");

	inline bool operator==(mxDate l, milDate r)noexcept { return l == mxDate(r); }
	inline bool operator==(milDate l, mxDate r)noexcept { return mxDate(l) == r; }
	inline bool operator!=(mxDate l, milDate r)noexcept { return !(l == r); }
	inline bool operator!=(milDate l, mxDate r)noexcept { return !(l == r); }

}

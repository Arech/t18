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

#include "_uglyTime.h"

namespace t18 {
	namespace _dt_impl {

		//describes bit fields width and offsets in timestamp_ult
		struct timestampBits {
			static constexpr unsigned bitsYear = 12;
			static constexpr int maxYear = (1 << bitsYear) - 1; //4095 for bitsYear = 12. Should be enought and ymdhm still fits in 32 bits

			static constexpr unsigned bitsMonth = 4;
			static constexpr unsigned bitsDay = 5;

			static constexpr unsigned bitsHour = 5;
			static constexpr unsigned bitsMinute = 6;

			static constexpr unsigned bitsSecond = 6;
			static constexpr unsigned bitsMicrosecond = 20;

			static_assert(!(::std::is_signed_v<timestamp_ult> || ::std::is_signed_v<time_ult> || ::std::is_signed_v<date_ult>), "");

			// strictly less to reserve the highest bit to mark "empty" value
			static_assert(bitsMicrosecond + bitsSecond + bitsMinute + bitsHour + bitsDay + bitsMonth + bitsYear < sizeof(timestamp_ult) * 8, "");
			static_assert(bitsSecond + bitsMinute + bitsHour < sizeof(time_ult) * 8, "");
			static_assert(bitsDay + bitsMonth + bitsYear < sizeof(date_ult) * 8, "");

			//////////////////////////////////////////////////////////////////////////
			//finally describe an offsets in d/t related underlying types
			template <typename WhereT, bool bSkipSomeRtChecks = false> struct Offs;
		};

		template <bool bSkipSomeRtChecks> struct timestampBits::Offs<tag_mxTimestamp, bSkipSomeRtChecks> {
			static constexpr unsigned ofsMicrosecond = 0;
			static constexpr unsigned ofsSecond = ofsMicrosecond + bitsMicrosecond;
			static constexpr unsigned ofsMinute = ofsSecond + bitsSecond;
			static constexpr unsigned ofsHour = ofsMinute + bitsMinute;
			static constexpr unsigned ofsDay = ofsHour + bitsHour;
			static constexpr unsigned ofsMonth = ofsDay + bitsDay;
			static constexpr unsigned ofsYear = ofsMonth + bitsMonth;

			static constexpr timestamp_ult maskMicrosecond = ((timestamp_ult(1) << bitsMicrosecond) - 1) << ofsMicrosecond;
			static constexpr timestamp_ult maskSecond = ((timestamp_ult(1) << bitsSecond) - 1) << ofsSecond;
			static constexpr timestamp_ult maskMinute = ((timestamp_ult(1) << bitsMinute) - 1) << ofsMinute;
			static constexpr timestamp_ult maskHour = ((timestamp_ult(1) << bitsHour) - 1) << ofsHour;
			static constexpr timestamp_ult maskDay = ((timestamp_ult(1) << bitsDay) - 1) << ofsDay;
			static constexpr timestamp_ult maskMonth = ((timestamp_ult(1) << bitsMonth) - 1) << ofsMonth;
			static constexpr timestamp_ult maskYear = ((timestamp_ult(1) << bitsYear) - 1) << ofsYear;

			static constexpr timestamp_ult maskmxDate = maskYear | maskMonth | maskDay;
			static constexpr timestamp_ult maskmxTime = maskHour | maskMinute | maskSecond;

			static constexpr timestamp_ult maxValue = ((static_cast<timestamp_ult>(999999) << ofsMicrosecond)
				| (static_cast<timestamp_ult>(59) << ofsSecond)
				| (static_cast<timestamp_ult>(59) << ofsMinute)
				| (static_cast<timestamp_ult>(23) << ofsHour)
				| (static_cast<timestamp_ult>(31) << ofsDay)
				| (static_cast<timestamp_ult>(12) << ofsMonth)
				| (static_cast<timestamp_ult>(maxYear) << ofsYear));

			static constexpr timestamp_ult emptyValue = timestamp_ult(1) << (8 * sizeof(timestamp_ult) - 1);
			static constexpr bool isEmpty(timestamp_ult v)noexcept {
				return static_cast<bool>(v & emptyValue);
			}

			static constexpr int Microsecond(timestamp_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskMicrosecond) >> ofsMicrosecond);
				T18_ASSERT(v >= 0 && v <= 999999);
				return v;
			}
			static constexpr int Second(timestamp_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskSecond) >> ofsSecond);
				T18_ASSERT(v >= 0 && v <= 59);
				return v;
			}
			static constexpr int Minute(timestamp_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskMinute) >> ofsMinute);
				T18_ASSERT(v >= 0 && v <= 59);
				return v;
			}
			static constexpr int Hour(timestamp_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskHour) >> ofsHour);
				T18_ASSERT(v >= 0 && v <= 23);
				return v;
			}
			static constexpr int Day(timestamp_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskDay) >> ofsDay);
				T18_ASSERT(v > 0 && v <= 31);
				return v;
			}
			static constexpr int Month(timestamp_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskMonth) >> ofsMonth);
				T18_ASSERT(bSkipSomeRtChecks || (v > 0 && v <= 12));
				return v;
			}
			static constexpr int Year(timestamp_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskYear) >> ofsYear);
				T18_ASSERT(bSkipSomeRtChecks || (v > 1900 && v < 2100));//just a sanity check
				return v;
			}

			//returns mks passed since year start.
			static constexpr timestamp_diff_t _yearTime(timestamp_ult mxT, bool isLeap)noexcept {
				auto m = Month(mxT);
				return ((m == 0 ? 0 : _uglyTime::days[12 * int(isLeap) + m - 1])
					+ static_cast<timestamp_diff_t>((mxT & maskDay) >> ofsDay)
					)*_uglyTime::mksInDay
					+ static_cast<timestamp_diff_t>((mxT & maskHour) >> ofsHour)*_uglyTime::mksInHour
					+ static_cast<timestamp_diff_t>((mxT & maskMinute) >> ofsMinute)*_uglyTime::mksInMin
					+ static_cast<timestamp_diff_t>((mxT & maskSecond) >> ofsSecond)*_uglyTime::mksInSec
					+ static_cast<timestamp_diff_t>((mxT & maskMicrosecond) >> ofsMicrosecond);
			}

		public:
			//returns the difference (mxT1-mxT2) between two timestamps.
			//WARNING: the timestamps MUST NOT differ more than a year
			static constexpr timestamp_diff_t uglyDiffMks(timestamp_ult mxT1, timestamp_ult mxT2) {
				const auto y1 = Year(mxT1);
				const auto y2 = Year(mxT2);
				const auto yd = y1 - y2;
				if (yd > 1 || yd < -1) {
					T18_ASSERT(!"Cant handle too big time differences");
					throw ::std::runtime_error("Cant handle too big time differences");
				}
				const bool b1leap = _uglyTime::isLeapYear(y1);
				const bool b2leap = _uglyTime::isLeapYear(y2);
				const timestamp_diff_t td = _yearTime(mxT1, b1leap) - _yearTime(mxT2, b2leap);
				if (y1 == y2) {
					return td;
				} else {
					return td + _uglyTime::mksInDay*(y1 > y2 ? (b2leap ? 366 : 365) : (b1leap ? -366 : -365));
				}
			}

			static constexpr date_ult mxdate(timestamp_ult mxD) noexcept {
				/*auto v = (mxD & maskmxDate) >> ofsDay;
				T18_ASSERT(v <= ::std::numeric_limits<date_ult>::max());
				return static_cast<date_ult>(v & ((timestamp_ult(1) << sizeof(date_ult)*8) - 1));*/
				return static_cast<date_ult>((mxD & maskmxDate) >> ofsDay);
			}
			static constexpr time_ult mxtime(timestamp_ult mxD) noexcept {
				return static_cast<time_ult>((mxD & maskmxTime) >> ofsSecond);
			}

			static constexpr timestamp_ult to_mxtimestamp(int y, int m, int d, int h, int mn, int s, int mks)noexcept {
				T18_ASSERT(mks >= 0 && mks <= 999999);
				T18_ASSERT(s >= 0 && s <= 59);
				T18_ASSERT(mn >= 0 && mn <= 59);
				T18_ASSERT(h >= 0 && h <= 23);
				T18_ASSERT((bSkipSomeRtChecks && y >= 0) || (y > 1900 && y <= maxYear));
				T18_ASSERT((bSkipSomeRtChecks && m >= 0) || (m > 0 && m <= 12));
				T18_ASSERT((bSkipSomeRtChecks && d >= 0) || (d > 0 && d <= 31));
				T18_ASSERT(y == 0 || m == 0 || d == 0 || (_uglyTime::daysInMonth(m, _uglyTime::isLeapYear(y)) >= d));
				return (static_cast<timestamp_ult>(mks) << ofsMicrosecond)
					| (static_cast<timestamp_ult>(s) << ofsSecond)
					| (static_cast<timestamp_ult>(mn) << ofsMinute)
					| (static_cast<timestamp_ult>(h) << ofsHour)
					| (static_cast<timestamp_ult>(d) << ofsDay)
					| (static_cast<timestamp_ult>(m) << ofsMonth)
					| (static_cast<timestamp_ult>(y) << ofsYear);
			}
			static constexpr timestamp_ult to_mxtimestamp(int y, int m, int d, int h, int mn, int s)noexcept {
				T18_ASSERT(s >= 0 && s <= 59);
				T18_ASSERT(mn >= 0 && mn <= 59);
				T18_ASSERT(h >= 0 && h <= 23);
				T18_ASSERT((bSkipSomeRtChecks && y >= 0) || (y > 1900 && y <= maxYear));
				T18_ASSERT((bSkipSomeRtChecks && m >= 0) || (m > 0 && m <= 12));
				T18_ASSERT((bSkipSomeRtChecks && d >= 0) || (d > 0 && d <= 31));
				T18_ASSERT(y == 0 || m == 0 || d == 0 || (_uglyTime::daysInMonth(m, _uglyTime::isLeapYear(y)) >= d));
				/*T18_ASSERT(y == 0 || m == 0 || d == 0
				|| ((_uglyTime::days[12 * _uglyTime::isLeapYear(y) + m] - _uglyTime::days[12 * _uglyTime::isLeapYear(y) + m - 1]) >= d)
				);*/
				return (static_cast<timestamp_ult>(s) << ofsSecond)
					| (static_cast<timestamp_ult>(mn) << ofsMinute)
					| (static_cast<timestamp_ult>(h) << ofsHour)
					| (static_cast<timestamp_ult>(d) << ofsDay)
					| (static_cast<timestamp_ult>(m) << ofsMonth)
					| (static_cast<timestamp_ult>(y) << ofsYear);
			}

			/*static constexpr timestamp_ult max()noexcept {
			return to_mxtimestamp(maxYear, 12, 31, 23, 59, 59, 999999);
			}*/

			static constexpr timestamp_ult to_mxtimestamp(date_ult mxD, time_ult mxT)noexcept {
				return (timestamp_ult(mxD) << ofsDay) | (timestamp_ult(mxT) << ofsSecond);
			}

			static constexpr timestamp_ult set_time(timestamp_ult ts, time_ult mxT, int mks)noexcept {
				T18_ASSERT(mks >= 0 && mks < 1000000);
				return (ts & maskmxDate) | (timestamp_ult(mxT) << ofsSecond) | (static_cast<timestamp_ult>(mks) << ofsMicrosecond);
			}
			static constexpr timestamp_ult set_time(timestamp_ult ts, int h, int mn, int s, int mks)noexcept {
				T18_ASSERT(s >= 0 && s <= 59);
				T18_ASSERT(mn >= 0 && mn <= 59);
				T18_ASSERT(h >= 0 && h <= 23);
				T18_ASSERT(mks >= 0 && mks < 1000000);
				return (ts & maskmxDate) | (static_cast<timestamp_ult>(mks) << ofsMicrosecond)
					| (static_cast<timestamp_ult>(s) << ofsSecond)
					| (static_cast<timestamp_ult>(mn) << ofsMinute)
					| (static_cast<timestamp_ult>(h) << ofsHour);
			}

			//////////////////////////////////////////////////////////////////////////
			//valOnlyDays is an ordinary timestamp_ult >> ofsDay
			static constexpr timestamp_ult _nextDay(timestamp_ult valOnlyDays)noexcept {
				T18_ASSERT(valOnlyDays < (maxValue >> ofsDay));
				const timestamp_ult origVal = valOnlyDays << ofsDay;

				const int m = Month(origVal);
				const int d = Day(origVal);
				const int daysInThisMonth = _uglyTime::daysInMonth(m, _uglyTime::isLeapYear(Year(origVal)));

				if (LIKELY(d < daysInThisMonth)) {
					valOnlyDays = (valOnlyDays + 1) << ofsDay;
				} else {
					valOnlyDays >>= bitsDay;//got month into lowest bits of valOnlyDays
					if (LIKELY(m < 12)) {
						valOnlyDays = ((valOnlyDays + 1) << ofsMonth) | (timestamp_ult(1) << ofsDay);
					} else {
						valOnlyDays = (((valOnlyDays >> bitsMonth) + 1) << ofsYear) | ((timestamp_ult(1) << ofsMonth) | (timestamp_ult(1) << ofsDay));
					}
				}
				T18_ASSERT(valOnlyDays < maxValue);
				T18_ASSERT(valOnlyDays > origVal);
				return valOnlyDays;
			}
			static constexpr timestamp_ult nextDayStart(timestamp_ult val)noexcept {
				return _nextDay(val >> ofsDay);
			}

			//////////////////////////////////////////////////////////////////////////
			//valOnlyDays is an ordinary timestamp_ult >> ofsDay
			static constexpr timestamp_ult _prevDay(timestamp_ult valOnlyDays)noexcept {
				T18_ASSERT(valOnlyDays < (maxValue >> ofsDay));
				const timestamp_ult origVal = valOnlyDays << ofsDay;

				const int m = Month(origVal);
				const int d = Day(origVal);
				
				//const int daysInThisMonth = _uglyTime::daysInMonth(m, _uglyTime::isLeapYear(Year(origVal)));

				if (LIKELY(d > 1)) {
					valOnlyDays = (valOnlyDays - 1) << ofsDay;
				} else {
					valOnlyDays >>= bitsDay;//got month into lowest bits of valOnlyDays
					if (LIKELY(m > 1)) {
						const int daysInPrevMonth = _uglyTime::daysInMonth(m - 1, _uglyTime::isLeapYear(Year(origVal)));
						valOnlyDays = ((valOnlyDays - 1) << ofsMonth) | (timestamp_ult(daysInPrevMonth) << ofsDay);
					} else {
						valOnlyDays = (((valOnlyDays >> bitsMonth) - 1) << ofsYear) | ((timestamp_ult(12) << ofsMonth) | (timestamp_ult(31) << ofsDay));
					}
				}

				T18_ASSERT(valOnlyDays < origVal);
				return valOnlyDays;
			}
			static constexpr timestamp_ult prevDayStart(timestamp_ult val)noexcept {
				return _prevDay(val >> ofsDay);
			}

			//////////////////////////////////////////////////////////////////////////
			//valOnlyHours is an ordinary timestamp_ult >> ofsHour
			static constexpr timestamp_ult _nextHour(timestamp_ult valOnlyHours)noexcept {
				T18_ASSERT(valOnlyHours < (maxValue >> ofsHour));
				T18_DEBUG_ONLY(timestamp_ult origVal = valOnlyHours << ofsHour);

				timestamp_ult k = valOnlyHours & (maskHour >> ofsHour);
				if (LIKELY(k < 23)) {
					valOnlyHours = (valOnlyHours + 1) << ofsHour;
				} else {
					valOnlyHours = _nextDay(valOnlyHours >> bitsHour);
				}

				T18_ASSERT(valOnlyHours > origVal);
				return valOnlyHours;
			}
			static constexpr timestamp_ult nextHourStart(timestamp_ult val)noexcept {
				return _nextHour(val >> ofsHour);
			}

			//valOnlyMinutes is an ordinary timestamp_ult >> ofsMinute
			static constexpr timestamp_ult _nextMinute(timestamp_ult valOnlyMinutes)noexcept {
				T18_ASSERT(valOnlyMinutes < (maxValue >> ofsMinute));
				T18_DEBUG_ONLY(timestamp_ult origVal = valOnlyMinutes << ofsMinute);

				timestamp_ult k = valOnlyMinutes & (maskMinute >> ofsMinute);
				if (LIKELY(k < 59)) {
					valOnlyMinutes = (valOnlyMinutes + 1) << ofsMinute;
				} else {
					valOnlyMinutes = _nextHour(valOnlyMinutes >> bitsMinute);
				}

				T18_ASSERT(valOnlyMinutes > origVal);
				return valOnlyMinutes;
			}
			static constexpr timestamp_ult nextMinuteStart(timestamp_ult val)noexcept {
				return _nextMinute(val >> ofsMinute);
			}

			static constexpr timestamp_ult next(timestamp_ult val) noexcept {
				static_assert(ofsMicrosecond == 0, "");
				T18_DEBUG_ONLY(auto origVal = val);

				timestamp_ult k = val & (maskMicrosecond >> ofsMicrosecond);
				if (LIKELY(k < 999999)) {
					val = (val + 1) << ofsMicrosecond;
				} else {
					val >>= bitsMicrosecond;//got sec
					k = val & (maskSecond >> ofsSecond);
					if (LIKELY(k < 59)) {
						val = (val + 1) << ofsSecond;
					} else {
						val = _nextMinute(val >> bitsSecond);
					}
				}
				T18_ASSERT(val > origVal);
				return val;
			}

			static constexpr timestamp_ult plusYear(timestamp_ult val) noexcept {
				return val + (timestamp_ult(1) << ofsYear);
			}

			//////////////////////////////////////////////////////////////////////////
			// easiest way (and it doesn't seems now like we'd ever need another way) to encode timeframe convertors
			// is to stick to minute-based timeframes that evenly devide an hour or a day.

			typedef decltype(hana::make_set(
				hana::int_c<1>, hana::int_c<2>, hana::int_c<3>, hana::int_c<4>, hana::int_c<5>, hana::int_c<6>
				, hana::int_c<10>, hana::int_c<12>, hana::int_c<15>, hana::int_c<20>, hana::int_c<30>, hana::int_c<60>
				, hana::int_c<2 * 60>, hana::int_c<3 * 60>, hana::int_c<4 * 60>, hana::int_c<6 * 60>, hana::int_c<8 * 60>, hana::int_c<24 * 60>
			)) _supportedTFMinutes_t;
			//not intended to be used frequently in release build
			static constexpr bool isGoodTfMinutes(int m)noexcept {
				return hana::contains(_supportedTFMinutes_t(), m);
			}

			//for time ts and timeframe mTf return timeframe boundary whick includes ts: lower boundary as return and upper in tsUpperBnd
			static constexpr timestamp_ult TFBoundaries(int mTf, timestamp_ult ts, timestamp_ult& tsUpperBnd)noexcept {
				T18_ASSERT(isGoodTfMinutes(mTf));
				T18_DEBUG_ONLY(timestamp_ult origTs = ts);

				timestamp_ult tempTS = ts & (maskmxDate | maskHour);

				if (mTf < 60) {
					//adjusting to nearest previous suitable minute
					int m = Minute(ts);
					int rem = m % mTf;
					if (rem > 0) {
						m -= rem;
						T18_ASSERT(m >= 0);
					}
					ts = tempTS | (static_cast<timestamp_ult>(m) << ofsMinute);

					//making upper boundary
					m += mTf;
					if (m >= 60) {
						//need to update hour+minute
						tempTS = nextHourStart(tempTS);
						m = m - 60;
						T18_ASSERT(m >= 0);
					} else {
						//just substituting minutes, i.e. do nothing here
					}
					tsUpperBnd = tempTS | (static_cast<timestamp_ult>(m) << ofsMinute);
				} else {
					const int hTF = mTf / 60;//it's OK to do integer devision here as long as isGoodTfMinutes or _supportedTFMinutes_t
					int h = Hour(tempTS);
					int rem = h % hTF;
					if (rem>0) {
						h -= rem;
						T18_ASSERT(h >= 0);
					}
					tempTS &= maskmxDate;//leaving only date info
					ts = tempTS | (static_cast<timestamp_ult>(h) << ofsHour);

					h += hTF;
					//are formed using whole hours
					if (h >= 24) {
						//updating day
						h = h - 24;
						tempTS = nextDayStart(tempTS);
					} else {
						//just substituting hours, i.e. do nothing here
					}
					tsUpperBnd = tempTS | (static_cast<timestamp_ult>(h) << ofsHour);
				}
				T18_ASSERT(ts < tsUpperBnd && origTs < tsUpperBnd && origTs >= ts);
				return ts;
			}

			//returns current TF lower (earliest) boundary
			static constexpr timestamp_ult TFLowerBoundary(int mTf, timestamp_ult ts)noexcept {
				T18_ASSERT(isGoodTfMinutes(mTf));
				T18_DEBUG_ONLY(timestamp_ult origTs = ts);

				timestamp_ult tempTS = ts & (maskmxDate | maskHour);

				if (mTf < 60) {
					//adjusting to nearest previous suitable minute
					int m = Minute(ts);
					int rem = m % mTf;
					if (rem > 0) {
						m -= rem;
						T18_ASSERT(m >= 0);
					}
					ts = tempTS | (static_cast<timestamp_ult>(m) << ofsMinute);
				} else {
					const int hTF = mTf / 60;//it's OK to do integer devision here as long as isGoodTfMinutes or _supportedTFMinutes_t
					int h = Hour(tempTS);
					int rem = h % hTF;
					if (rem > 0) {
						h -= rem;
						T18_ASSERT(h >= 0);
					}
					tempTS &= maskmxDate;//leaving only date info
					ts = tempTS | (static_cast<timestamp_ult>(h) << ofsHour);
				}
				T18_ASSERT(ts <= origTs);
				return ts;
			}
		};

		template <> struct timestampBits::Offs<tag_mxDate> {
			static constexpr unsigned ofsDay = 0;
			static constexpr unsigned ofsMonth = ofsDay + bitsDay;
			static constexpr unsigned ofsYear = ofsMonth + bitsMonth;

			static constexpr date_ult maskDay = ((date_ult(1) << bitsDay) - 1) << ofsDay;
			static constexpr date_ult maskMonth = ((date_ult(1) << bitsMonth) - 1) << ofsMonth;
			static constexpr date_ult maskYear = ((date_ult(1) << bitsYear) - 1) << ofsYear;

			static constexpr date_ult emptyValue = date_ult(1) << (8 * sizeof(date_ult) - 1);
			static constexpr bool isEmpty(date_ult v)noexcept {
				return static_cast<bool>(v & emptyValue);
			}

			static constexpr int Day(date_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskDay) >> ofsDay);
				T18_ASSERT(v > 0 && v <= 31);
				return v;
			}
			static constexpr int Month(date_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskMonth) >> ofsMonth);
				T18_ASSERT(v > 0 && v <= 12);
				return v;
			}
			static constexpr int Year(date_ult mxD) noexcept {
				auto v = static_cast<int>((mxD & maskYear) >> ofsYear);
				T18_ASSERT(v > 1900 && v < 2100);//just a sanity check
				return v;
			}

			static constexpr date_ult to_mxdate(int y, int m, int d)noexcept {
				T18_ASSERT(y > 1900 && y <= maxYear);
				T18_ASSERT(m > 0 && m <= 12);
				T18_ASSERT(d > 0 && d <= 31);
				return (static_cast<date_ult>(d) << ofsDay)
					| (static_cast<date_ult>(m) << ofsMonth)
					| (static_cast<date_ult>(y) << ofsYear);
			}
		};

		template <> struct timestampBits::Offs<tag_mxTime> {
			static constexpr unsigned ofsSecond = 0;
			static constexpr unsigned ofsMinute = ofsSecond + bitsSecond;
			static constexpr unsigned ofsHour = ofsMinute + bitsMinute;

			static constexpr time_ult maskSecond = ((time_ult(1) << bitsSecond) - 1) << ofsSecond;
			static constexpr time_ult maskMinute = ((time_ult(1) << bitsMinute) - 1) << ofsMinute;
			static constexpr time_ult maskHour = ((time_ult(1) << bitsHour) - 1) << ofsHour;

			static constexpr time_ult emptyValue = time_ult(1) << (8 * sizeof(time_ult) - 1);
			static constexpr bool isEmpty(time_ult v)noexcept {
				return static_cast<bool>(v & emptyValue);
			}

			static constexpr time_ult to_mxtime(int h, int m, int s)noexcept {
				T18_ASSERT(s >= 0 && s <= 59);
				T18_ASSERT(m >= 0 && m <= 59);
				T18_ASSERT(h >= 0 && h <= 23);
				return (static_cast<time_ult>(s) << ofsSecond)
					| (static_cast<time_ult>(m) << ofsMinute)
					| (static_cast<time_ult>(h) << ofsHour);
			}

			static constexpr int Second(time_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskSecond) >> ofsSecond);
				T18_ASSERT(v >= 0 && v <= 59);
				return v;
			}
			static constexpr int Minute(time_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskMinute) >> ofsMinute);
				T18_ASSERT(v >= 0 && v <= 59);
				return v;
			}
			static constexpr int Hour(time_ult mxT) noexcept {
				auto v = static_cast<int>((mxT & maskHour) >> ofsHour);
				T18_ASSERT(v >= 0 && v <= 23);
				return v;
			}
		};

	}
}


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
#include "stdafx.h"

#include "../t18/date_time.h"
#include "../t18/utils/hana.h"
#include "../t18/utils/obj_traits.h"

using namespace ::std::literals;
using namespace t18;

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestDateTime, MxTime) {
	printf("Empty: %x\n", _dt_impl::timestampBits::Offs<tag_mxTime>::emptyValue);

	mxTime t;
	ASSERT_TRUE(t.empty());

	mxTime t0(tag_milTime(), 0);
	ASSERT_FALSE(t0.empty());
	ASSERT_TRUE(0 == t0.Hour());
	ASSERT_TRUE(0 == t0.Minute());
	ASSERT_TRUE(0 == t0.Second());

	mxTime t1(tag_milTime(), 235959);
	ASSERT_FALSE(t1.empty());
	ASSERT_EQ(23, t1.Hour());
	ASSERT_EQ(59, t1.Minute());
	ASSERT_EQ(59, t1.Second());

	mxTime t2(tag_milTime(), 183712);
	ASSERT_FALSE(t2.empty());
	ASSERT_EQ(18, t2.Hour());
	ASSERT_EQ(37, t2.Minute());
	ASSERT_EQ(12, t2.Second());

	mxTime t3(18,37,12);
	ASSERT_FALSE(t3.empty());
	ASSERT_EQ(18, t3.Hour());
	ASSERT_EQ(37, t3.Minute());
	ASSERT_EQ(12, t3.Second());
}

TEST(TestDateTime, MxDate) {
	printf("Empty: %x\n", _dt_impl::timestampBits::Offs<tag_mxDate>::emptyValue);

	mxDate d;
	ASSERT_TRUE(d.empty());

	mxDate t0(tag_milDate(), 19010101);
	ASSERT_FALSE(t0.empty());
	ASSERT_EQ(1901, t0.Year());
	ASSERT_EQ(1, t0.Month());
	ASSERT_EQ(1, t0.Day());

	/*mxDate t1(tag_milDate(), _dt_impl::timestampBits::maxYear*10000 + 12*100 +31);
	ASSERT_FALSE(t1.empty());
	ASSERT_EQ(_dt_impl::timestampBits::maxYear, t1.Year());
	ASSERT_EQ(12, t1.Month());
	ASSERT_EQ(31, t1.Day());*/

	mxDate t2(tag_milDate(), 20910315);
	ASSERT_FALSE(t2.empty());
	ASSERT_EQ(2091, t2.Year());
	ASSERT_EQ(3, t2.Month());
	ASSERT_EQ(15, t2.Day());

	mxDate t3(2091,3,15);
	ASSERT_FALSE(t3.empty());
	ASSERT_EQ(2091, t3.Year());
	ASSERT_EQ(3, t3.Month());
	ASSERT_EQ(15, t3.Day());
}

TEST(TestDateTime, MxTimestamp) {
	printf("Empty: %llx\n", _dt_impl::timestampBits::Offs<tag_mxTimestamp>::emptyValue);

	mxTimestamp e;
	ASSERT_TRUE(e.empty());

	mxTimestamp t1(tag_milDT(), 20130912, 234123);
	ASSERT_FALSE(t1.empty());
	ASSERT_EQ(mxDate(tag_milDate(), 20130912), t1.Date());
	ASSERT_EQ(mxTime(tag_milTime(), 234123), t1.Time());
	ASSERT_TRUE(0 == t1.Microsecond());

	mxTimestamp t2(2013, 11, 2, 2, 42, 0, 12);
	ASSERT_FALSE(t2.empty());
	ASSERT_EQ(mxDate(tag_milDate(), 20131102), t2.Date());
	ASSERT_EQ(mxTime(tag_milTime(), 24200), t2.Time());
	ASSERT_EQ(12, t2.Microsecond());
	ASSERT_EQ(t2.next(), mxTimestamp(2013, 11, 2, 2, 42, 0, 13));

	ASSERT_EQ(mxTimestamp(2013, 11, 2, 2, 42, 0, 999999).next(), mxTimestamp(2013, 11, 2, 2, 42, 1, 0));
	ASSERT_EQ(mxTimestamp(2013, 11, 2, 2, 42, 59, 999999).next(), mxTimestamp(2013, 11, 2, 2, 43, 0, 0));
	ASSERT_EQ(mxTimestamp(2013, 11, 2, 2, 59, 59, 999999).next(), mxTimestamp(2013, 11, 2, 3, 0, 0, 0));
	ASSERT_EQ(mxTimestamp(2013, 11, 2, 23, 59, 59, 999999).next(), mxTimestamp(2013, 11, 3, 0, 0, 0, 0));
}

TEST(TestDateTime, MxTimestamp_uglyDiff) {
	typedef _mxTimestamp<false> testMxTs;//this will remove some assertions, that otherwise would be triggered by the normal test flow
	testMxTs t1(2018, 11, 20, 13, 50, 40, 500000);

	//half second less
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 20, 13, 50, 40, 0)), testMxTs(0, 0, 0, 0, 0, 0,500000)._yearTime());

	//half second more
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 20, 13, 50, 41, 0)), -testMxTs(0, 0, 0, 0, 0, 0, 500000)._yearTime());

	//minute and half second less
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 20, 13, 49, 40, 0)), testMxTs(0, 0, 0, 0, 1, 0, 500000)._yearTime());

	//minute more
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 20, 13, 51, 40, 500000)), -testMxTs(0, 0, 0, 0, 1, 0, 0)._yearTime());

	//day and half second less
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 19, 13, 50, 40, 0)), testMxTs(0, 0, 1, 0, 0, 0, 500000)._yearTime());

	//day more
	ASSERT_EQ(t1.uglyDiffMks(testMxTs(2018, 11, 21, 13, 50, 40, 500000)), -testMxTs(0, 0, 1, 0, 0, 0, 0)._yearTime());

	namespace hana = ::boost::hana;
	constexpr auto dNonLeap = hana::tuple_c<int, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31>;
	constexpr auto dLeap = hana::tuple_c<int, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31>;
	hana::for_each(hana::to_tuple(hana::range_c<int, 1, 12>), [&dNonLeap, &dLeap](auto&& m)noexcept {
		typedef ::std::decay_t<decltype(m)> m_t;
		static_assert(_uglyTime::daysInMonth(m.value, false) == hana::at(dNonLeap, m_t() - hana::int_c<1>), "");
		static_assert(_uglyTime::daysInMonth(m.value, true) == hana::at(dLeap, m_t() - hana::int_c<1>), "");
	});	

	for (int y = 2000; y <= 2050; ++y) {
		const bool yLeap = _uglyTime::isLeapYear(y);
		for (int m = 1; m <= 12; ++m) {
			for (int d = 1; d <= 28; d+=5) {
				t1 = testMxTs(y, m, d, 13, 50, 40, 500000);
				if (m > 1) {
					//1 day and half second less
					ASSERT_EQ(t1.uglyDiffMks(testMxTs(y, m - 1, _uglyTime::daysInMonth(m - 1, yLeap), 13, 50, 40, 0))
						, testMxTs(0, 0, d, 0, 0, 0, 500000)._yearTime());
				} else {
					//1 day and half second less
					ASSERT_EQ(t1.uglyDiffMks(testMxTs(y - 1, 12, _uglyTime::daysInMonth(12, _uglyTime::isLeapYear(y - 1)), 13, 50, 40, 0))
						, testMxTs(0, 0, d, 0, 0, 0, 500000)._yearTime());
				}

				t1 = testMxTs(y, m, _uglyTime::daysInMonth(m, yLeap), 13, 50, 40, 500000);
				if (m > 11) {
					//1 day more
					ASSERT_EQ(t1.uglyDiffMks(testMxTs(y + 1, 1, d, 13, 50, 40, 500000)), -testMxTs(0, 0, d, 0, 0, 0, 0)._yearTime());
				} else {
					//1 day more
					ASSERT_EQ(t1.uglyDiffMks(testMxTs(y, m + 1, d, 13, 50, 40, 500000)), -testMxTs(0, 0, d, 0, 0, 0, 0)._yearTime());
				}
			}
		}
	}
}
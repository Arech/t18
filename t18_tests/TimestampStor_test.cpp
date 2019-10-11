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

#include "../t18/timeseries/TimestampStor.h"
namespace hana = ::boost::hana;
using namespace hana::literals;
using namespace std::literals;

typedef decltype(hana::make_map(
	hana::make_pair(t18::timestamp_ht(), hana::type_c<t18::mxTimestamp>)
)) ts_ht;

class TimestampStorWrap : public t18::timeseries::TimestampStor<ts_ht> {
	typedef t18::timeseries::TimestampStor<ts_ht> base_class_t;
public:
	TimestampStorWrap(size_t N) : base_class_t(N) {}
	void storeBar(t18::milDate d, t18::milTime t) {
		base_class_t::storeBar(hana::make_map(
			hana::make_pair(t18::timestamp_ht(), t18::mxTimestamp(t18::tag_milDT(), d._get(), t._get()))
		));
	}
};

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestDTStor, DTFuncs) {
	using namespace t18;
	milDate md(20170305);
	ASSERT_EQ(md, mxDate(md).to_mil());

	milTime mt(144729);
	ASSERT_EQ(mt, mxTime(mt).to_mil());
}

TEST(TestDTStor, Creation) {
	using namespace t18;
	TimestampStorWrap ts(5);

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 0);

	ts.storeBar(milDate(20180101), milTime(144000));

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 1);
	ASSERT_EQ(ts.get(t18::timestamp_ht(), 0), mxTimestamp(tag_milDT(), 20180101, 144000));

	ts.storeBar(milDate(20190101), milTime(154000));

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 2);

	ASSERT_EQ(ts.timestamp(0).Date().to_mil()._get(), 20190101);
	ASSERT_EQ(ts.timestamp(0).Time().to_mil()._get(), 154000);
	ASSERT_EQ(ts.timestamp(1).Date(), milDate(20180101));
	ASSERT_EQ(ts.timestamp(1).Time(), milTime(144000));
}

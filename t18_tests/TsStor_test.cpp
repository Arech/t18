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


#include "../t18/timeseries/TsStor.h"
namespace hana = ::boost::hana;
using namespace hana::literals;
using namespace std::literals;
//#include "../t18/utils/name_of_type.h"

#include "CopyMoveTestClass.h"

template<typename HanaMapT>
class TsStorWrap : public t18::timeseries::TsStor<HanaMapT>{
	typedef t18::timeseries::TsStor<HanaMapT> base_class_t;
public:
	TsStorWrap(size_t N) : base_class_t(N) {}
	using base_class_t::storeBar;
};

T18_COMP_SILENCE_REQ_GLOBAL_CONSTR

TEST(TestTsStor, Creation) {
	static constexpr auto hsTs = "ts"_s;
	static constexpr auto hsOpen = "open"_s;

	TsStorWrap<decltype(hana::make_map(
		hana::make_pair(hsTs, hana::type_c<int>)
		, hana::make_pair(hsOpen, hana::type_c<double>)
	))> ts(5);

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 0);
	/*ASSERT_EQ(ts.BarsCount(), 0);
	ASSERT_EQ(ts.TF(), 1);*/

	ts.storeBar(hana::make_map(
		hana::make_pair(hsTs, 1)
		, hana::make_pair(hsOpen, 2.5)
	));

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 1);
	//ASSERT_EQ(ts.BarsCount(), 1);

	ASSERT_EQ(ts.get(hsTs, 0), 1);
	ASSERT_EQ(ts.get(hsOpen, 0), 2.5);

	ts.storeBar(hana::make_map(
		hana::make_pair(hsTs, 2)
		, hana::make_pair(hsOpen, 4.5)
	));

	ASSERT_EQ(ts.capacity(), 5);
	ASSERT_EQ(ts.size(), 2);
	//ASSERT_EQ(ts.BarsCount(), 2);

	ASSERT_EQ(ts.get(hsTs, 1), 1);
	ASSERT_EQ(ts.get(hsOpen, 1), 2.5);
	ASSERT_EQ(ts.get(hsTs, 0), 2);
	ASSERT_EQ(ts.get(hsOpen, 0), 4.5);

	/*
	typedef ::std::remove_reference<decltype(ts.get(hsTs, 0))>::type tsDct_t;
	typedef ::std::remove_reference<decltype(ts.get(hsOpen, 0))>::type openDct_t;

	auto tsDctN = utils::name_of_type(tsDct_t());
	auto openDctN = utils::name_of_type(openDct_t());
	STDCOUTL("tsDctN = "<< tsDctN<<", openDctN="<< openDctN);

	ASSERT_EQ(tsDctN, "const int"s);
	ASSERT_EQ(openDctN, "const double"s);
	*/
}

TEST(TestTsStor, MoveSymantics) {
	TsStorWrap<decltype(hana::make_map(
		hana::make_pair("CopyMoveTestClass"_s, hana::type_c<CopyMoveTestClass>)
	))> ts(5);

	{
		STDCOUTL("Copying");
		CopyMoveTestClass v(100);

		ts.storeBar(hana::make_map(
			hana::make_pair("CopyMoveTestClass"_s, v)
		));
		STDCOUTL("Done copying");
	}
	const auto& ev = ts.get("CopyMoveTestClass"_s, 0);
	ASSERT_EQ(ev.vtc, 100);
	ASSERT_GT(ev.cc, 0);

	{
		STDCOUTL("Moving");
		CopyMoveTestClass v(200);

		ts.storeBar(hana::make_map(
			hana::make_pair("CopyMoveTestClass"_s, ::std::move(v))
		));
		STDCOUTL("Done moving");
	}
	const auto& ev2 = ts.get("CopyMoveTestClass"_s, 0);
	ASSERT_EQ(ev2.vtc, 200);
	ASSERT_EQ(ev2.cc, 0);
	ASSERT_GT(ev2.mc, 0);

	ASSERT_EQ(ts.get("CopyMoveTestClass"_s, 1).vtc, 100);

	STDCOUTL("Finished");
}

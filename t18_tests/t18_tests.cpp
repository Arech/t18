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
// trd_tests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <locale.h>

#include "../t18/base.h"
#include "../t18/utils/name_of_type.h"

int __cdecl main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);

	//if (!setlocale(LC_ALL, "Russian")) STDCOUTL("Error: Failed to setlocale(LC_ALL, \"Russian\").\nYou will not be able to read cyrillic output...");
	STDCOUTL(std::endl << "*** Using real_t == " << utils::name_of_type<::t18::real_t>() << " ***" << std::endl);

	int r = RUN_ALL_TESTS();
	//::std::cin.ignore();
	return r;
}

#ifdef BOOST_ENABLE_ASSERT_HANDLER

namespace boost {
	T18_COMP_SILENCE_MISSING_NORETURN
	void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line) {
		using namespace ::std::literals;
		STDCOUTL("*** Boost assertion failed @" << file << ", func=" << function << "@" << line << ", expr = (" << expr << ")");
		if (msg) STDCOUTL("Msg = " << msg);
		T18_ASSERT(!"Boost assertion failed!");
		::std::terminate();
	}

	void assertion_failed(char const * expr, char const * function, char const * file, long line) {
		assertion_failed_msg(expr, nullptr, function, file, line);
	}
	T18_COMP_POP

} // namespace boost

#endif


/*

//namespace hana = ::boost::hana;
using namespace t18;
using namespace hana::literals;
using namespace std::literals;

template<typename T>
using TickersCont_tpl = ::std::vector<T>;

/ *
template<typename HST, typename = ::std::enable_if_t<utils::isHSetT_v<HST>>>
static constexpr auto makeTickersStor(HST const& s) {
	return hana::to_map( hana::zip_with(
		hana::make_pair
		, hana::to_tuple(hana::range_c<size_t, 0, hana::size(HST())>)
		, hana::transform(hana::to_tuple(s), hana::template_<TickersCont_tpl>)
	));
}* /
template<typename HST, typename = ::std::enable_if_t<utils::isHSetT_v<HST>>>
static constexpr auto makeTickersStor(HST const& s) {
	return hana::transform(hana::to_tuple(s), [](auto&& e) {
		typedef typename decltype(+e)::type ticker_t;
		return ::std::declval<TickersCont_tpl<ticker_t>>();
	});
}

void tst(){
	typedef utils::make_set_t<int, float, double> types_hst;

	typedef decltype(makeTickersStor(types_hst())) TickersStor_t;
	
	utils::static_assert_same<TickersStor_t, void>();
	
	/ *TickersStor_t stor{ makeTickersStor(types_hst()) };
	size_t idx = 1;

	constexpr auto l = [](auto&& x) {
		printf("%d\n", x);
	};

	auto v = hana::any_of(hana::to_tuple(hana::range_c<size_t, 0, 3>), [&](auto&& x) -> decltype(auto) {
		bool b = idx == x;
		if (b) {
			//typedef decltype(x) x_t;
			//utils::static_assert_same<x_t, void>();
			l(x);
		}
		return b;
	});* /
}
*/



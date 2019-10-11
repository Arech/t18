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

//provides compile-time correspondence between int and algorithm type

#include "_all.h"

namespace t18 {
	
	constexpr auto AlgsMap = hana::make_map(
		hana::make_pair("MA"_s, hana::type_c<algs::MA>)
		, hana::make_pair("EMA"_s, hana::type_c<algs::EMA>)
		, hana::make_pair("EMAsi"_s, hana::type_c<algs::EMAsi>)
		, hana::make_pair("DEMA"_s, hana::type_c<algs::DEMA>)
		, hana::make_pair("TEMA"_s, hana::type_c<algs::TEMA>)
		, hana::make_pair("MovMin"_s, hana::type_c<algs::MovMin>)
		, hana::make_pair("MovMax"_s, hana::type_c<algs::MovMax>)
		, hana::make_pair("Elementile"_s, hana::type_c<algs::Elementile>)
		, hana::make_pair("Percentile"_s, hana::type_c<algs::Percentile>)
		, hana::make_pair("PercentRank"_s, hana::type_c<algs::PercentRank>)
		, hana::make_pair("LTFPercentile"_s, hana::type_c<algs::LTFPercentile>)
	);

	template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
	using alg_type_by_name_t = typename decltype(+AlgsMap[HST()])::type;


	constexpr auto AlgsMap_c = hana::make_map(
		hana::make_pair("MA"_s, hana::type_c<algs::MA_c>)
		, hana::make_pair("EMA"_s, hana::type_c<algs::EMA_c>)
		, hana::make_pair("EMAsi"_s, hana::type_c<algs::EMAsi_c>)
		, hana::make_pair("DEMA"_s, hana::type_c<algs::DEMA_c>)
		, hana::make_pair("TEMA"_s, hana::type_c<algs::TEMA_c>)
		, hana::make_pair("MovMin"_s, hana::type_c<algs::MovMin_c>)
		, hana::make_pair("MovMax"_s, hana::type_c<algs::MovMax_c>)
		, hana::make_pair("Elementile"_s, hana::type_c<algs::Elementile_c>)
		, hana::make_pair("Percentile"_s, hana::type_c<algs::Percentile_c>)
		, hana::make_pair("PercentRank"_s, hana::type_c<algs::PercentRank_c>)
		, hana::make_pair("LTFPercentile"_s, hana::type_c<algs::LTFPercentile_c>)
	);

	template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
	using alg_type_by_name_c_t = typename decltype(+AlgsMap_c[HST()])::type;
}
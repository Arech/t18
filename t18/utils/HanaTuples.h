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

namespace utils {
	namespace hana = ::boost::hana;
	using namespace hana::literals;

	template<typename... T>
	using make_descr_tuple_t = decltype(hana::make_basic_tuple( hana::type_c<T>... ));

	template<typename HTT>
	constexpr bool isGenHTuple(const HTT&)noexcept {
		return hana::is_a<hana::basic_tuple_tag, HTT> || hana::is_a<hana::tuple_tag, HTT>;
	}
	template<typename HTT>
	constexpr bool isGenHTuple_v = isGenHTuple(HTT());


	template<typename HTT>
	constexpr bool isDescrTuple(const HTT& t)noexcept {
		return isGenHTuple(t) && hana::all_of(t, is_htype_tag);
	}
	template<typename HTT>
	constexpr bool isDescrTuple_v = isDescrTuple(HTT());


	// creates a tuple object holding actual default constructed objects instead of their types as values
	template<typename HTT, typename = ::std::enable_if_t<isDescrTuple_v<HTT>> >
	static constexpr auto dataTupleFromDescrTuple(const HTT& tup) {
		return hana::fold_left(tup, hana::make_basic_tuple(), [](auto dataTup, auto elm)noexcept {
			//"auto elm" drops references and top level cv-qualifiers
			typedef decltype(elm) elm_t;
			return hana::insert(dataTup, typename elm_t::type());
		});
	}

	template<typename HTT>
	using dataTupleFromDescrTuple_t = decltype(dataTupleFromDescrTuple(HTT()));

	//////////////////////////////////////////////////////////////////////////
	template<typename HTT, typename = ::std::enable_if_t<isDescrTuple_v<HTT>> >
	static constexpr auto dataPtrTupleFromDescrTuple(const HTT& tup) {
		return hana::fold_left(tup, hana::make_basic_tuple(), [](auto dataTup, auto elm)noexcept {
			//"auto elm" drops references and top level cv-qualifiers
			typedef ::std::add_pointer_t<typename decltype(elm)::type> pElm_t;
			pElm_t p = nullptr;
			return hana::insert(dataTup, hana::length(dataTup), p);
		});
	}

	template<typename HTT>
	using dataPtrTupleFromDescrTuple_t = decltype(dataPtrTupleFromDescrTuple(HTT()));
}


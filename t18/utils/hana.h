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

#ifndef T18_LEAVE_ALONE_BOOST_HANA_CONFIG_ENABLE_STRING_UDL

#ifndef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#define BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#endif

#endif

#include <boost/hana.hpp>
//related #includes defined after namespace def

// Generally, utils are too generic for a single project things, therefore
// there's no need to put'em into a project specific namespace.
// Without it is's far more easier to use them across different projects
namespace utils {

	namespace hana = ::boost::hana;
	using namespace hana::literals;

	//////////////////////////////////////////////////////////////////////////

	template<typename HTag, typename T>
	static constexpr void static_assert_htag()noexcept {
		static_assert(hana::is_a<HTag, T>, "Wrong hana tag passed!");
	}

	//////////////////////////////////////////////////////////////////////////

	//thanks to 
	//https://stackoverflow.com/questions/33979592/boost-hana-get-index-of-first-matching
	template <typename Iterable, typename T>
	constexpr auto index_of(Iterable const& iterable, T const& element) {
		auto size = decltype(hana::size(iterable)){};
		auto dropped = decltype(hana::size(
			hana::drop_while(iterable, hana::not_equal.to(element))
		)){};
		return size - dropped;
	}
	//  Louis Dionne: hana::drop_while will create a new sequence at runtime if you don't enclose it in decltype.
	//  This might not be a problem for your current use case, but consider what happens if your iterable contains std::strings,
	//  for example. Then, running hana::drop_while will create copies of those strings, which is completely useless.
	//  Enclosing the whole thing in decltype ensures that no runtime work is done. 
	template <typename Iterable, typename T>
	constexpr auto index_of_existing(T const& element) {
		auto size = decltype(hana::size(Iterable())){};
		auto dropped = decltype(hana::size(
			hana::drop_while(Iterable(), hana::not_equal.to(element))
		)){};		
		static_assert(size == hana::size_c<0>  || dropped > hana::size_c<0>, "element not found!");//will fail here if run-time computation required
		return size - dropped;
	}
	template <typename Iterable, typename T>
	constexpr auto index_of_existing(Iterable const&, T const& element) {
		return index_of_existing<Iterable>(element);
	}

	//////////////////////////////////////////////////////////////////////////
	
	struct is_htype_tag_t {
		template<typename T>
		constexpr decltype(auto) operator()(T const&) const{
			return hana::is_a<hana::type_tag, T>;
		}
	};
	constexpr is_htype_tag_t is_htype_tag{};

	struct is_hstring_tag_t {
		template<typename T>
		constexpr decltype(auto) operator()(T const&) const {
			return hana::is_a<hana::string_tag, T>;
		}
	};
	constexpr is_hstring_tag_t is_hstring_tag{};

	template<typename HST>
	struct is_hana_string_t {
		template<typename T>
		constexpr decltype(auto) operator()(T const& v) const {
			return v==HST();
		}
	};
	template<typename HST>
	constexpr is_hana_string_t<HST> is_hana_string{};

	struct is_hmap_tag_t {
		template<typename T>
		constexpr decltype(auto) operator()(T const&) const {
			return hana::is_a<hana::map_tag, T>;
		}
	};
	constexpr is_hmap_tag_t is_hmap_tag{};

	//////////////////////////////////////////////////////////////////////////

	template<typename...PTs>
	using makeMap_t = decltype(hana::make_map(PTs()...));

	template <typename HMT1, typename HMT2>
	constexpr bool noSameKeys_v = (hana::make_set() == hana::intersection(
		hana::to_set(hana::keys(HMT1())), hana::to_set(hana::keys(HMT2()))
	));

	template <typename HMT, typename HST>
	inline constexpr auto findKey(HMT&& m, HST&& k)noexcept {
		return hana::find(hana::keys(m), k);
	}

	template <typename HMT, typename HST>
	constexpr auto findKey_v = findKey(::std::remove_reference_t<HMT>(), ::std::remove_reference_t<HST>());

	//why not hana::contains() ?
	template <typename HMT, typename HST>
	inline constexpr bool hasKey(HMT&& m, HST&& k)noexcept {
		return (hana::just(k) == findKey(m, k));
	}

	template <typename HMT, typename HST>
	constexpr bool hasKey_v = hasKey(::std::remove_reference_t<HMT>(), ::std::remove_reference_t<HST>());

	template <typename HMT, typename HST>
	inline constexpr bool noKey(HMT&& m, HST&& k)noexcept {
		return (hana::nothing == findKey(m, k));
	}

	template <typename HMT, typename HST>
	constexpr bool noKey_v = noKey(::std::remove_reference_t<HMT>(), ::std::remove_reference_t<HST>());


	template <typename HMT1, typename HMT2>
	using unionMaps_t = decltype(hana::union_(HMT1(),HMT2()));
	//#todo: write generic function
	template <typename HMT1, typename HMT2, typename HMT3>
	using unionMaps3_t = decltype(hana::union_(HMT3(), hana::union_(HMT1(), HMT2())));

	//////////////////////////////////////////////////////////////////////////
	template <typename HMT, typename HST, typename T>
	inline constexpr auto setMapKey(HMT&& m, HST&& k, T&& v)noexcept {
		return hana::insert(
			hana::erase_key(::std::forward<HMT>(m), k), hana::make_pair(k, v)
		);
	}


	//////////////////////////////////////////////////////////////////////////

	// Using hana::type<T> as a parameter type wouldn't work, since it may be
	// a dependent type, in which case template argument deduction will fail.
	// Using hana::basic_type<T> is fine, since this one is guaranteed to be
	// a non dependent base of hana::type<T>.
	// NOTE: if you're going to use that function, include name_of_type.h first or #include <boost/core/demangle.hpp>
	template <typename T>
	inline std::string const& name_of_hana_type(hana::basic_type<T>) {
		static std::string name = core::demangle(typeid(T).name());
		return name;
	}
}

#include "HanaDescrMaps.h"
#include "HanaDataMaps.h"
#include "HanaTuples.h"
#include "HanaSets.h"


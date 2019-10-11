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
#include "HanaDescrMaps.h"

namespace utils {


	//values of data map is simple (non-hana) types and I don't know how to check for it, so just check keys
	template<typename HMT>
	constexpr ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>, bool> couldBeDataMap(const HMT& m)noexcept {
		return hana::all_of(hana::keys(m), is_hstring_tag);
	}
	template<typename HMT>
	constexpr ::std::enable_if_t<!hana::is_a<hana::map_tag, HMT>, bool> couldBeDataMap(const HMT&)noexcept {
		return false;
	}
	template<typename HMT>
	constexpr bool couldBeDataMap_v = couldBeDataMap(HMT());


	// creates a map object holding the same keys as passed map, but actual default constructed objects instead of their types as values
	template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
	static constexpr auto dataMapFromDescrMap(const HMT& map) {
		return hana::fold_left(map, hana::make_map(), [](auto dataMap, auto pair)noexcept {
			auto t = hana::second(pair);//auto drops references and top level cv-qualifiers
			typedef decltype(t) secondP_t;
			static_assert(hana::is_a<hana::type_tag, secondP_t>, "Map values must be hana::type_c!");
			static_assert(hana::is_a < hana::string_tag, ::std::remove_reference_t<decltype(hana::first(pair))> >
				, "Map keys must be hana::string-s");
			return hana::insert(dataMap, hana::make_pair(hana::first(pair), typename secondP_t::type()));
		});
	}

	template<typename HMT>
	using dataMapFromDescrMap_t = decltype(dataMapFromDescrMap(HMT()));

	template<typename HMDataT, typename HMT>
	static inline constexpr void static_assert_hmap_conforms_dataMap(const HMT& hm)noexcept {
		static_assert(couldBeDataMap_v<HMDataT>, "");
		typedef decltype(hana::keys(hm)) keysToTest_t;
		hana::for_each(HMDataT(), [](auto pr)noexcept {
			auto kv = hana::first(pr);
			//typedef ::std::remove_reference_t<decltype(kv)> key_t;
			static_assert(hana::find(keysToTest_t(), kv) == hana::just(kv), "Necessary key was not found");

			typedef ::std::remove_reference_t<decltype(hana::second(pr))> expectVal_t;
			typedef ::std::remove_reference_t<decltype(hana::at_key(HMT(), kv))> realVal_t;
			static_assert(::std::is_convertible_v<realVal_t, expectVal_t>, "Value is not convertible to required type");
		});
	}

	//////////////////////////////////////////////////////////////////////////
	//hmap_get set of functions queries the hana::map by a key end ensures that it'll return the exact type as expected
	template<typename T, typename HST, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::string_tag, HST>>>
	inline static decltype(auto) hmap_get(const HMT& hm, HST s) noexcept {
		static_assert(::std::is_same_v<T, ::std::decay_t<decltype(hm[s])>>, "Type mismatch!");
		return hm[s];
	}
	template<typename T, typename HST, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::string_tag, HST>>>
	inline static decltype(auto) hmap_get(HMT& hm, HST s) noexcept {
		static_assert(::std::is_same_v<T, ::std::decay_t<decltype(hm[s])>>, "Type mismatch!");
		return hm[s];
	}

	template<typename T, typename HST, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::string_tag, HST>>>
	inline static decltype(auto) hmap_get(const HMT& hm) noexcept {
		static_assert(::std::is_same_v<T, ::std::decay_t<decltype(hm[HST()])>>, "Type mismatch!");
		return hm[HST()];
	}
	template<typename T, typename HST, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::string_tag, HST>>>
	inline static decltype(auto) hmap_get(HMT& hm) noexcept {
		static_assert(::std::is_same_v<T, ::std::decay_t<decltype(hm[HST()])>>, "Type mismatch!");
		return hm[HST()];
	}
	//PDescr should be something like decltype(hana::make_pair("string"_s, hana::type_c<int>))
	template<typename PDescr, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::pair_tag, PDescr>>>
	inline static decltype(auto) hmap_get(const HMT& hm) noexcept {
		typedef ::std::remove_reference_t<decltype(hana::first(PDescr()))> key_ht;
		static_assert(hana::is_a<hana::string_tag, key_ht>, "Key must be hana string!");
		typedef ::std::remove_reference_t<decltype(hana::second(PDescr()))> val_ht;
		//utils::static_assert_htag<hana::type_tag, val_ht>();
		static_assert(hana::is_a<hana::type_tag, val_ht>, "Not a hana::type!");
		typedef typename val_ht::type val_t;

		//static_assert(::std::is_same_v<val_t, ::std::remove_reference_t<decltype(hm[key_ht()])>>, "Types mismatch!");
		//static_assert_same<val_t, ::std::remove_reference_t<decltype(hm[key_ht()])>>();
		// actually, with remove_reference_t it'll fire on const maps and that is not what we want
		static_assert(::std::is_same_v<val_t, ::std::decay_t<decltype(hm[key_ht()])>>, "Types mismatch!");
		return hm[key_ht()];
	}
	template<typename PDescr, typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT> && hana::is_a<hana::pair_tag, PDescr>>>
	inline static decltype(auto) hmap_get(HMT& hm) noexcept {
		typedef ::std::remove_reference_t<decltype(hana::first(PDescr()))> key_ht;
		static_assert(hana::is_a<hana::string_tag, key_ht>, "Key must be hana string!");
		typedef ::std::remove_reference_t<decltype(hana::second(PDescr()))> val_t;
		static_assert(::std::is_same_v<val_t, ::std::decay_t<decltype(hm[key_ht()])>>, "Type mismatch!");
		return hm[key_ht()];
	}

	//////////////////////////////////////////////////////////////////////////

}

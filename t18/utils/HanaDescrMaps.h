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

	//////////////////////////////////////////////////////////////////////////

	template<typename NameHST, typename TypeT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, NameHST>>>
	constexpr auto Descr_v = hana::make_pair(NameHST(), hana::type_c<TypeT>);

	template<typename NameHST, typename TypeT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, NameHST>>>
	using Descr_t = decltype(Descr_v<NameHST, TypeT>);

	//////////////////////////////////////////////////////////////////////////

	template<typename HMT>
	constexpr bool isDescrMap(const HMT& m)noexcept {
		return hana::is_a<hana::map_tag, HMT>
			&& hana::all_of(hana::values(m), is_htype_tag)
			&& hana::all_of(hana::keys(m), is_hstring_tag);
	}
	template<typename HMT>
	constexpr bool isDescrMap_v = isDescrMap(HMT());

	


	namespace _i {
		template<typename HMT1, typename HMT2>
		inline constexpr decltype(auto) _check_descrMaps_intersection_has_same_vals(HMT1&& p1, HMT2&& p2) {
			static_assert(isDescrMap_v<HMT1> && isDescrMap_v<HMT2>, "");
			return hana::fold_left(hana::intersection(
				hana::to_set(hana::keys(p1))
				, hana::to_set(hana::keys(p2))
			), hana::make_set(), [](auto s, auto k) noexcept {
				static_assert(::std::is_same_v<decltype(+hana::at_key(p1, k))
					, decltype(+hana::at_key(p2, k))>
					, "Values MUST have the same types for the same keys!");
				return s;
			});
		}
	}

	template<typename HMT1, typename HMT2>
	constexpr bool isDescrMapIntersectionHasSameVals_v = ::std::is_same_v<decltype(hana::make_set())
		, decltype(_i::_check_descrMaps_intersection_has_same_vals(HMT1(), HMT2())) >;


	//merging descrMaps assuring that same keys contain same values
	template<typename HMT_Tuple>
	static constexpr auto descrMap_merge(HMT_Tuple&& t) {
		static_assert(hana::is_a<hana::basic_tuple_tag, HMT_Tuple>, "");
		return hana::fold_left(t, hana::make_map(), [](auto map, auto tElm) {
			static_assert(hana::size(map) == hana::size_c<0> || isDescrMap(map), "");
			return hana::fold_left(tElm, map, [&map](auto mp, auto pair)noexcept {
				auto fv = hana::find(map, hana::first(pair));
				static_assert(hana::nothing == fv || fv == hana::just(hana::second(pair)), "");
				return hana::insert(mp, pair);
			});
		});
	}
	template<typename... HMTs>
	using descrMap_merge_t = decltype(descrMap_merge(hana::make_basic_tuple(HMTs()...)));


	template<typename HMDescrT, typename HMT>
	static inline constexpr void static_assert_hmap_conforms_descr(const HMT& hm)noexcept {
		static_assert(isDescrMap_v<HMDescrT>, "");
		typedef decltype(hana::keys(hm)) keysToTest_t;
		hana::for_each(HMDescrT(), [](auto pr)noexcept {
			auto kv = hana::first(pr);
			//typedef ::std::remove_reference_t<decltype(kv)> key_t;
			static_assert(hana::find(keysToTest_t(), kv) == hana::just(kv), "Necessary key was not found");

			typedef typename ::std::remove_reference_t<decltype(hana::second(pr))>::type expectVal_t;
			typedef ::std::remove_reference_t<decltype(hana::at_key(HMT(), kv))> realVal_t;
			static_assert(::std::is_convertible_v<realVal_t, expectVal_t>, "Value is not convertible to required type");
		});
	}
}
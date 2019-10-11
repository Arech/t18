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

namespace utils {

	/*template<typename T> static constexpr void static_show_type() {
		static_assert(false, "you've requested type of T, see below");
	};*/

	//static_assert_* family functions should only be used to debug parameter types used
	template<typename T1, typename T2>
	static constexpr void static_assert_same()noexcept {
		static_assert(::std::is_same_v<T1, T2>, "Types mismatch!");
	}
	template<typename T1, typename T2>
	static constexpr int static_assert_not_same()noexcept {
		static_assert(!::std::is_same_v<T1, T2>, "Types must be different!");
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename T> 
	inline constexpr T& pointer2ref(T* p)noexcept { T18_ASSERT(p);		return *p; }
	
	template<typename T>
	inline constexpr const T& pointer2ref(const T* p)noexcept { T18_ASSERT(p);		return *p; }

	template<typename T>
	inline constexpr T& pointer2ref(T& r)noexcept { return r; }

	template<typename T>
	inline constexpr const T& pointer2ref(const T& r)noexcept { return r; }

	//////////////////////////////////////////////////////////////////////////

	// primary template handles types that have no nested ::tag_t member:
	template< class, class = ::std::void_t<> >
	struct has_type_tag_t : ::std::false_type { };

	// specialization recognizes types that do have a nested ::tag_t member:
	template< class T >
	struct has_type_tag_t<T, ::std::void_t<typename T::tag_t>> : ::std::true_type {};

	template< class T >
	constexpr bool has_type_tag_t_v = has_type_tag_t<T>::value;

	template<typename TagT, typename T>
	using is_tag_of = ::std::is_same<TagT, typename T::tag_t>;

	template<typename TagT, typename T>
	constexpr bool is_tag_of_v = is_tag_of<TagT, T>::value;

	//template<typename TagT, typename T>
	//using has_tag_t = ::std::conditional<has_type_tag_t_v<T>, is_tag_of<TagT,T>, ::std::false_type>;
	template<typename TagT, typename T, typename = ::std::void_t<>>
	struct has_tag_t : ::std::false_type {};

	template<typename TagT, typename T>
	struct has_tag_t<TagT, T, ::std::void_t<typename T::tag_t>> : is_tag_of<TagT, T> {};

	template<typename TagT, typename T>
	constexpr bool has_tag_t_v = has_tag_t<TagT, T>::value;
}
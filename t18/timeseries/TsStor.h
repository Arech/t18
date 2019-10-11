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

#include "../base.h"

//Этот класс описывает хранилище данных тикера в памяти
namespace t18 {
namespace timeseries {
	namespace hana = ::boost::hana;
	using namespace hana::literals;

	//#TODO the type of container that is used to store data must be specified directly at HanaMapT
	template<typename HanaMapT>
	class TsStor {
	private:
		typedef TsStor<HanaMapT> self_t;

	public:
		typedef HanaMapT HanaMap_ht;
		static_assert(utils::isDescrMap_v<HanaMapT>, "Not a description map!");

	private:
		//type deducing function that builds a hana::map to connect compile-time strings (keys, names of timeseries) with a 
		//run-time containers that holds ts data.
		template<typename T>
		static constexpr auto makeTsStorMap(T const& map) {
			return hana::fold_left(map, hana::make_map(), [](auto tsmap, auto pair) {
				auto t = hana::second(pair);//auto drops references and top level cv-qualifiers
				return hana::insert(tsmap, hana::make_pair(hana::first(pair), TsCont_t<typename decltype(t)::type>()));
			});
		}

	public:
		using TsStor_ht = decltype(makeTsStorMap(HanaMapT()));

		using TsData_ht = utils::dataMapFromDescrMap_t<HanaMapT>;

	protected:
	//private://we don't want derived classes to mess with the container//nope, we may need it sometimes
		TsStor_ht m_ContMap;

		size_t m_TotalBars = 0;

	public:

		TsStor(size_t N) {
			//initializing containers
			hana::for_each(m_ContMap, [N](auto& x) {
				hana::second(x).set_capacity(N);
			});
		}
		
		//as we fill all containers simultaneously, we may just query any of them
		size_t size()const noexcept { return m_ContMap[hana::at_c<0>(hana::keys(HanaMapT()))].size(); }
		size_t capacity()const noexcept { return m_ContMap[hana::at_c<0>(hana::keys(HanaMapT()))].capacity(); }

		//must return reference to help easy algs creation. See algs::inspectLowerTF
		const size_t& TotalBars()const noexcept { return m_TotalBars; }

		size_t BarIndex(size_t N)const noexcept {
			T18_ASSERT(N < m_TotalBars);
			return m_TotalBars - N - 1;
		}

		void storeBar(TsData_ht&& v) noexcept {
			auto& contMap = m_ContMap;
			hana::for_each(v, [&contMap](auto&& x)noexcept {
				contMap[hana::first(x)].push_front(::std::move(hana::second(x)));
			});
			++m_TotalBars;
		}
		void storeBar(const TsData_ht& v) noexcept {
			auto& contMap = m_ContMap;
			hana::for_each(v, [&contMap](const auto& x)noexcept {
				contMap[hana::first(x)].push_front(hana::second(x));
			});
			++m_TotalBars;
		}


		void updateLastBar(TsData_ht&& v) noexcept {
			auto& contMap = m_ContMap;
			hana::for_each(v, [&contMap](auto&& x)noexcept {
				contMap[hana::first(x)][0] = ::std::move(hana::second(x));
			});
		}
		void updateLastBar(const TsData_ht& v)noexcept {
			auto& contMap = m_ContMap;
			hana::for_each(v, [&contMap](const auto& x)noexcept {
				contMap[hana::first(x)][0] = hana::second(x);
			});
		}

		//getters returning non const are intentionally prefixed with underscore. Use with care.
		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		auto& _get(size_t N)noexcept { return m_ContMap[HStrT()][N]; }

		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		auto& _get(HStrT, size_t N) noexcept { return _get<HStrT>(N); }

		//DONT add new data into container via _getTs(), or you'll spoil m_TotalBars
		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		auto& _getTs()noexcept { return m_ContMap[HStrT()]; }

		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		auto& _getTs(HStrT)noexcept { return _getTs<HStrT>(); }


		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		const auto& get(size_t N)const noexcept { return m_ContMap[HStrT()][N]; }

		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		const auto& get(HStrT, size_t N)const noexcept { return get<HStrT>(N); }


		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		const auto& getTs()const noexcept { return m_ContMap[HStrT()]; }

		template<typename HStrT, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HStrT>>>
		const auto& getTs(HStrT)const noexcept { return getTs<HStrT>(); }
	};


}
}

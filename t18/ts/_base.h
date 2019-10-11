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

#include "../market/MarketDataStorServ.h"
#include "../exec/tradingInterface.h"

//base class for each Trade System, defines TS outer interface

namespace t18 { namespace ts {

	class _base {
	public:
		typedef exec::tradingInterface tradingIntf_t;
		typedef typename tradingIntf_t::trade_t trade_t;
		//typedef typename tradingIntf_t::ticker_t ticker_t;

		//typedef MarketDataStor MktDataStor_t;
		//static_assert(::std::is_same_v<typename MktDataStor_t::ticker_t, ticker_t>, "");

	protected:
		//MktDataStor_t& m_Market;
		tradingIntf_t& m_Trading;

		_base(tradingIntf_t& t) : m_Trading(t) {}
	};

	template<typename FinalPolymorphChild>
	class _base_poly : public _base {
	private:
		typedef _base base_class_t;

	public:
		typedef FinalPolymorphChild self_t;
		T18_METHODS_SELF_CHECKED((::std::is_base_of_v<_base_poly, self_t>),"FinalPolymorphChild must be derived from ts::_base_poly");
		
	public:
		template<typename... Args>
		_base_poly(Args&&... a) : base_class_t(::std::forward<Args>(a)...) {}

	};

	//////////////////////////////////////////////////////////////////////////
	// Чтобы было удобнее наследовать код ТС нужен метод добавления и своих тайпдефов. Проще всего это сделать, если
	// договориться, что CtPrmsT может иметь тип add2Td_t, в котором перечисляются дополнения к тайпдефам. Тогда
	// можно наследованием добавить их к базовым *_TypedefsBase
	// Если внутри add2Td_t содержится определение типа Add2TickersSet_t, то это определение объединяется с
	// базовым TickersSet_t в новый TickersSet_t

	namespace _i {
		template< class, class = ::std::void_t<> >
		struct has_type_add2Td_t : ::std::false_type { };

		// specialization recognizes types that do have a nested ::tag_t member:
		template< class T >
		struct has_type_add2Td_t<T, ::std::void_t<typename T::add2Td_t>> : ::std::true_type {};

		template< class T >
		static constexpr auto has_type_add2Td_t_v = has_type_add2Td_t<T>::value;


		template <class T>
		struct inhAddTd_T : public T::add2Td_t {};
		struct inhDummy {};

		template <class T>
		using inhChooser = ::std::conditional_t<has_type_add2Td_t_v<T>, inhAddTd_T<T>, inhDummy>;


		template< class, class = ::std::void_t<> >
		struct has_type_Add2TickersSet_t : ::std::false_type { };

		// specialization recognizes types that do have a nested ::tag_t member:
		template< class T >
		struct has_type_Add2TickersSet_t<T, ::std::void_t<typename T::Add2TickersSet_t>> : ::std::true_type {};

		template< class T >
		static constexpr auto has_type_Add2TickersSet_t_v = has_type_Add2TickersSet_t<T>::value;

		template<class BaseCT, class AddTCT>
		struct mergeTickersSetsImpl {
			typedef decltype(hana::union_(typename AddTCT::Add2TickersSet_t(), typename BaseCT::TickersSet_t())) type;
		};
		template<class BaseCT>
		struct mergeTickersSetsImpl<BaseCT, void> {
			typedef typename BaseCT::TickersSet_t type;
		};

		template<class AddTCT, class BaseCT>
		using mergeTickersSets_t = typename mergeTickersSetsImpl<BaseCT, ::std::conditional_t<
			has_type_Add2TickersSet_t_v<AddTCT>, AddTCT, void>
		>::type;
	}

} }

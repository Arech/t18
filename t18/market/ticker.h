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

//#include <forward_list>
#include "../timeseries/Timeframe.h"
#include "tickerBase.h"

namespace t18 {
	using namespace ::std::literals;

	//////////////////////////////////////////////////////////////////////////
	//maintains the data objects and provides access interface to them
	//hdmTimeframesT is hana description map (hana::map that maps hana::string to hana::type_c) of timeframes classes
	// #WARNING The order of callback execution for timeframes is UNDEFINED, because hana::map offer no guarantees on folding order
	template<typename hdmTimeframesT>
	class ticker : public tickerBase {
	private:
		typedef tickerBase base_class_t;

	public:
		typedef base_class_t tickerBase_t;


		typedef hdmTimeframesT hdmTimeframes_t;
		static_assert(utils::isDescrMap_v<hdmTimeframesT>, "");
		
		//static_assert(utils::hasKey_v<hdmTimeframes_t, baseTf_hst>, "There must be a baseTf_hst key addressing the most discrete timeframe");

		//because every TF must have the same definition of bar_t, we may extract any definition (and check later if
		//it's the same for every TF.
		// Note, that hana::keys() as well as hana::values() return data in UNSPECIFIED order.
		typedef typename decltype(+hana::at_c<0>(hana::values(hdmTimeframes_t())))::type::bar_t bar_t;

	private:
		typedef ticker<hdmTimeframes_t> self_t;

	private:
		// if we statically allocate timeframe objects in ticker class, than we have to require timeframe classes to be
		// either default-constructible and then initializable (but there's no way to reinit const variables),
		// or to construct them right from the constructor of ticker class. Both options are not good, so we had to
		// allocate timeframe objects from the heap somewhere during trade system setup phase.
		// #TODOLater shouldn't we parametrize the deleter parameter of unique_ptr<> ?
		
		template<typename TfT>
		using TfStorage_tpl = ::std::unique_ptr<TfT>;

		template<typename T>
		static constexpr auto makeTfsStorMap(T const& map) {
			return hana::fold_left(map, hana::make_map(), [](auto&& tfmap, auto pair) {
				auto& t = hana::second(pair);//auto drops references and top level cv-qualifiers
				typedef typename decltype(+t)::type Tf_t;
				static_assert(::std::is_same_v<bar_t, typename Tf_t::bar_t>, "Every TF of the Ticker must have the same definition of bar_t");
				//typedef ::std::decay_t<decltype(tfmap)> curMap_t;
				return hana::insert(::std::move(tfmap), hana::make_pair(hana::first(pair), TfStorage_tpl<Tf_t>()));
			});
		}

	protected:
		typedef decltype(makeTfsStorMap(hdmTimeframes_t())) timeframesDm_t;

	protected:
		timeframesDm_t m_tfsMap;
		
	protected:
		//class is not intended to be instantiated directly
		ticker(TickerId __tickerId, const char* name, real_t minD) : base_class_t(__tickerId, name, minD), m_tfsMap() {}

		//for the convenience variant to create a single timeframe object right during ticker's construction
		template<typename TfIdT, typename TfT>
		ticker(TickerId __tickerId, const char* name, real_t minD, TfIdT&& id, TfStorage_tpl<TfT>&& pTf)
			: base_class_t(__tickerId, name, minD), m_tfsMap()
		{
			static_assert(hana::is_a<hana::string_tag, ::std::decay_t<TfIdT>>, "Must be hana::string");
			setTf(::std::forward<TfIdT>(id), ::std::move(pTf));
		}

		template<typename TfIdT, typename... TfArgsT>
		ticker(TickerId __tickerId, const char* name, real_t minD, TfIdT&& id, TfArgsT&&... a)
			: base_class_t(__tickerId, name, minD), m_tfsMap()
		{
			static_assert(hana::is_a<hana::string_tag, ::std::decay_t<TfIdT>>, "Must be hana::string");
			createTf(::std::forward<TfIdT>(id), ::std::forward<TfArgsT>(a)...);
		}

		template<typename HDMT, typename = ::std::enable_if_t<utils::couldBeDataMap_v<HDMT>>>
		ticker(TickerId __tickerId, const char* name, real_t minD, HDMT&& creationPrms)
			: base_class_t(__tickerId, name, minD), m_tfsMap()
		{
			hana::for_each(::std::forward<HDMT>(creationPrms), [t=this](auto&& pr) {
				auto id = hana::first(pr);
				typedef decltype(id) tfId_t;
				static_assert(utils::hasKey_v<hdmTimeframes_t, tfId_t>, "The key passed doesn't exist in the timeframes description");
				t->createTf(id, hana::second(pr));
			});
		}

		//////////////////////////////////////////////////////////////////////////
		//TfT is a timeframe class and TfIdT is a hana::string identifier
		template<typename TfIdT, typename TfT>
		auto& setTf(TfIdT&& id, TfStorage_tpl<TfT>&& pTf) {
			typedef TfStorage_tpl<TfT> tfStor_t;
			T18_ASSERT(pTf.get() || !"Invalid pointer passed");
			tfStor_t& tfPtr = utils::hmap_get<tfStor_t>(m_tfsMap, id);
			if (tfPtr.get()) {
				T18_ASSERT(!"Timeframe object has already been set!");
				throw ::std::logic_error("Tried to replace already set timeframe '"s + id.c_str() + "', ticker = " + m_Name);
			}
			tfPtr = ::std::move(pTf);
			T18_ASSERT(utils::hmap_get<tfStor_t>(m_tfsMap, id).get() && !pTf.get());
			return *tfPtr.get();
		}
		template<typename TfIdT, typename TfT>
		auto& setTf(TfStorage_tpl<TfT>&& pTf) {
			return setTf(TfIdT(), ::std::move(pTf));
		}

		template<typename TfIdT, typename... TfArgsT>
		auto& createTf(TfIdT&& id, TfArgsT&&... a) {
			static_assert(hana::is_a<hana::string_tag, ::std::decay_t<TfIdT>>, "Must be hana::string");

			auto& tfPtr = m_tfsMap[id];
			if (tfPtr.get()) {
				T18_ASSERT(!"Timeframe object has already been set!");
				throw ::std::logic_error("Tried to replace already set timeframe '"s + id.c_str() + "', ticker = " + m_Name);
			}
			typedef typename ::std::decay_t<decltype(tfPtr)>::element_type Tf_t;
			tfPtr.reset( new Tf_t(::std::forward<TfArgsT>(a)...) );
			T18_ASSERT(utils::hmap_get<::std::decay_t<decltype(tfPtr)>>(m_tfsMap, id).get());
			return *tfPtr.get();
		}
		template<typename TfIdT, typename... TfArgsT>
		auto& createTf(TfArgsT&&... a) {
			return createTf(TfIdT(), ::std::forward<TfArgsT>(a)...);
		}

		template<typename TfIdT, typename HGT, typename = ::std::enable_if_t<utils::isGenHTuple_v<::std::decay_t<HGT>>>>
		void createTf(TfIdT&& id, HGT&& tupl) {
			/*hana::fuse([&id, t = this](auto&... a) {
				t->createTf(::std::forward<TfIdT>(id), a...);
			})(::std::forward<HGT>(tupl));*/
			hana::unpack(::std::forward<HGT>(tupl), [&id, t = this](auto&...a) {
				t->createTf(::std::forward<TfIdT>(id), a...);
			});
		}

	public:
		tickerBase_t& getTickerBase()noexcept { return *this; }
		const tickerBase_t& getTickerBase()const noexcept { return *this; }

		template<typename F>
		void forEachTF(F&& f)const noexcept {
			hana::for_each(m_tfsMap, [&f](const auto& pr) noexcept {
				const auto& p = hana::second(pr);
				T18_ASSERT(p.get());
				typedef typename ::std::decay_t<decltype(p)>::element_type Tf_t;
				::std::forward<F>(f)(* static_cast<const Tf_t*>(p.get()));
			});
		}
		template<typename F>
		void forEachTF(F&& f) noexcept {
			hana::for_each(m_tfsMap, [&f](const auto& pr)noexcept {
				const auto& p = hana::second(pr);
				//if it dies here, you might forgot to create timeframe object
				T18_ASSERT(p.get());
				::std::forward<F>(f)(*p.get());
			});
		}

		template<typename TfIdT>
		auto& getTf(TfIdT&& id)noexcept {
			static_assert(utils::hasKey_v<hdmTimeframes_t, TfIdT>, "Timeframe key must be present in timeframes map");
			const auto& p = m_tfsMap[id];
			T18_ASSERT(p.get());
			return *p.get();
		}
		template<typename TfIdT>
		auto& getTf()noexcept { return getTf(TfIdT()); }

		template<typename TfIdT>
		const auto& getTf(TfIdT&& id)const noexcept {
			const auto& p = m_tfsMap[id];
			T18_ASSERT(p.get());
			return *p.get();
		}
		template<typename TfIdT>
		const auto& getTf()const noexcept { return getTf(TfIdT()); }

		//we definitely need a way to compare for equality of two ticker objects
		constexpr bool operator==(const self_t& r)const noexcept { return this == &r; }
		constexpr bool operator!=(const self_t& r)const noexcept { return this != &r; }
		/*template <typename T, typename = ::std::enable_if_t<!::std::is_same_v<T, self_t> && ::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
		constexpr bool operator==(const T& r)const noexcept { return false; }
		template <typename T, typename = ::std::enable_if_t<!::std::is_same_v<T, self_t> && ::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
		constexpr bool operator!=(const T& r)const noexcept { return true; }*/

		//auto& getBaseTf()noexcept { return getTf<baseTf_hst>(); }
		//const auto& getBaseTf() const noexcept { return getTf<baseTf_hst>(); }
	};

	template <typename T, typename = ::std::enable_if_t<::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
	inline constexpr bool operator==(const T& l, const typename T::tickerBase_t &r)noexcept { return l.getTickerBase() == r; }

	template <typename T, typename = ::std::enable_if_t<::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
	inline constexpr bool operator==(const typename T::tickerBase_t &l, const T& r)noexcept { return l == r.getTickerBase(); }

	template <typename T, typename = ::std::enable_if_t<::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
	inline constexpr bool operator!=(const T& l, const typename T::tickerBase_t &r)noexcept { return !(l == r); }

	template <typename T, typename = ::std::enable_if_t<::std::is_same_v<tag_Ticker_t, typename T::tag_t>>>
	inline constexpr bool operator!=(const typename T::tickerBase_t &l, const T& r)noexcept { return !(l == r); }
}

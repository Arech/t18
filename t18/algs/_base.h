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

#include "../base.h"
#include "code/_base.h"

namespace t18 {

	template<typename PrmHST, typename PrmV, typename = ::std::enable_if_t<::boost::hana::is_a<::boost::hana::string_tag, PrmHST>>>
	inline constexpr decltype(auto) Prm(PrmHST&& n, PrmV&& v) noexcept {
		return ::boost::hana::make_pair(::std::forward<PrmHST>(n), ::std::forward<PrmV>(v));
	}

	inline decltype(auto) PrmLen(algs::code::common_meta::prm_len_t l)noexcept {
		return Prm(algs::code::common_meta::prm_len_ht(), l);
	}
	inline decltype(auto) PrmLen(int l)noexcept {
		return Prm(algs::code::common_meta::prm_len_ht(), static_cast<algs::code::common_meta::prm_len_t>(l));
	}

	inline decltype(auto) PrmPercV(algs::code::common_meta::prm_percV_t v)noexcept {
		return Prm(algs::code::common_meta::prm_percV_ht(), v > 1 ? v / 100 : v);
	}
	inline decltype(auto) PrmPercV(int v)noexcept {
		return Prm(algs::code::common_meta::prm_percV_ht(), static_cast<algs::code::common_meta::prm_percV_t>(v) / 100);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename... Prms>
	inline constexpr decltype(auto) algPrms(Prms... p) noexcept {
		return ::boost::hana::make_map(::std::forward<Prms>(p)...);
	}

	//#WARNING specific order of folding is not guaranteed!!!
	//#todo need order-guaranteed folding
	template<typename PrmsDescrHMT, typename PrmsHMT, typename = ::std::enable_if_t < hana::is_a<hana::map_tag, PrmsHMT> && utils::isDescrMap_v<PrmsDescrHMT> >>
	static inline ::std::string prms_to_string(PrmsHMT&& m) {
		::std::string s;
		s.reserve(hana::value(hana::length(PrmsDescrHMT())) * 32);//#todo may be do something better?
		hana::for_each(PrmsDescrHMT(), [&m, &s](auto pr) {
			constexpr auto k = hana::first(pr);
			s += "_";
			s += k.c_str();
			
			auto v = hana::find(m, k);
			if (hana::is_just(v)) {
				s += ::std::to_string(v.value_or(0));
			} else s += "UNK";

		});
		return s;
	}

	//////////////////////////////////////////////////////////////////////////

	namespace algs {

		//����������:
		//1. ���� ����� ����������� ������ ��������������� ���������, ������� ����� �������� ��� ��� ����� ����������
		//		��� � ���� ������� ���������
		//2. ������ ��������� (� ������� ������) ����� ����������� ������/������ �� ���������� ���� �����. ��������,
		//		���������� ��� ������ ���-�� ��������������
		//3. ���������� ��� ���������� ������ ���� ������ �� ��������� ���������� ���������� (����� ����� ����
		//		�������� ��� � ������� �����������) � �� ��������� ��������� ���������. ��������, ���
		//		������ ����/���� ��������� ��������� ��������� � ����������� ���� ���������� ��������� � �� ������
		//		��������� �� � ��� ������. � ������ �� ���������� ���� ������ ���������, ����������� ������ � �����������
		//		������������ ���������� ��������� � ����� ����� ������� ����������� ��� ��������� ���������, - ����� � �����
		//		������������ ����� "���������", ������� � ������ ������ ������ �������� ��������� ��������������� ����������.
		//		��������������, ������ ���� ����� ��������� ���-���� ������:
		//			- ����� ���������� (���������� ���������� ������ ���� ������� �� ���������). � ����������� ������� (������?)
		//				"����������" ��������� ������ ���������� ������������� ������������ ��������� � ���������������.
		//			- ��������������� ��������� (��� ���������� ��� ���� ������� �� ���� � ������ ������������ ���������� ����).
		//				��� ��� ����� ��������� ���������, ���� ��������� ����� �������������� ����������� ����������� ���������
		//			- ��������� ��������� - ������� �� ���������, ���������� � ������� ��� ���������� � ���������� ��, �������
		//				��������� ��������� ����� ����������� ������, ��� ��������� ��� ������� ����������
		//4. ��� ������ ������������ ������������ ����� ���������� ������ ���� ����������� �������� ����� ����� ����� �����
		//		������� �� �������� ������������� ������� ���� � ����� ��� ���� ���������� ����� ���������� � ���������� ��
		//		(������������ ��� ���������� ������ ����������) ������ ���� ������ ��� � ��������� �����������.
		//		������ ���� �����������:
		//		- ����������� �������� ��� � �������-����� (������ ���������� �������, ��� ������������ ��������� ����������� ����������)
		//		- ����� ����������� ����������� �������� � ���-����� (������ �������������� �������, ��� ������ �������� �����������)
		//
		//����������:
		//- ������ ���� ������ ����� �����, ���������� ������ ����������� ������-�������, ����������� ������
		//		- � ����� ������ ������ ���� ��� ������������� ������� ��������� (���� ma()) ��� ������ �� ������
		//			���� ���, ��� ��� ���� � ����������� ����-������������ �����������,
		//		- ��� � ����� ������� calc(), ���������� �� ����� ������ ��������� ������ � ���� ������� ������������
		//			�������, ������������ ������� ����������, �����.��������� � ���������. ��� ����, ����� �� ���������
		//			��������, ������������ ������������� ���������, ����� ������� ������������ ��������� ����������/���������/��������
		//			calc() ������ ��������� ������ ��, ��� ����� ��������� ��.
		//		--- ������, �������� ���� �������� ������ �� calc() �������, � ����. �������, ������� ������ �����, ��� ���������
		//			�������� ������ ��������. ��� ���� ������� ����� ��������������� � ���� ������, ��� � ������ ����� ���-���� ������
		//			��� ����. ������� ������� ����� ������, � ��� ������-������� - �����.
		//		- ����� ������� ������ ���� 2 ������ ����� ������ �������. 1� ����� ������ ������ ��������� ���� � ������ ������,
		//			� ������, ��������� �� �������, ��� ����� ������������� ��������� ������� �������������� ������ ��� ���������
		//			���������� (����, �� �������)
		//			
		//////////////////////////////////////////////////////////////////////////
		//
		// ��� ��� ����:
		// - ����� �����������:
		//		- ����� ��������������� ����� � ����� /code � namespace code. ���� ����� ���������� ��������� ��������, ����������
		//			������������ ���� ������, � �����, ������� ��� ����� ������� ������ (�������). �� ��������� ����������.
		//		- �� ���� ����������� ����� _meta (���� � namespace code, �� ��� � ���� �����), ������� ��������� �������������
		//			������� ��������� ��������� (�������� � ��� ������ � ������ algPrmsDescr_t � algPrmsMap_t), �����������
		//			��������� ��������� (� ���� �������� algState_t -
		//			#todo ������� ����� �������� �� ���� ��. ������) � ��������� ��������� (� ���� ������� TStor_tpl,
		//			���������� �� ���� ������ ������� ���������, � �������� ���������, ������������ � ����� algTStorDescr_t
		//			��� �� � ���� ������ ������������ ������� ����������� ��������� ���������� � ����� prms2hmap() � ���������
		//			���������� ���������� validatePrms(), ������� ����� � ��� ������������� ������ ���������� ��������� �������� �
		//			����������, ���������� �� �����, � ��� �� minSrcHist, ���������� ��� �� ����� ���������� ������ ���������� ����.
		//			���� _���� ������������ ��������� ��� ��������� ���������/������ ���������, ������� ����� ������� �� ����, ���
		//			��� ����� ��������.
		//		- �� _���� ����������� _call, ������� ��� ����� ���������� � ���, ��� ����� �������� �������� � �� ��� ���������
		//			�������� ��������� ���������. ���� ����� ���������� ����� ��������, ��� �������� �������� �������� � ���� �����
		//			����������� ����������� ����� ��������� �� �������� ���, ������� ���� ������������ ��� ������� ���������.
		//			��� �� ���������� ������� minSrcHist � call, ��������� ������ �� ��������� "����������". ��� ������� ���������
		//			�������� ��������� ���������.
		//		- �� _call ��� ����� ������������� �����, ����������� ������� ��������� ��������� (��� ������������� � ��, ��������)
		//		
		// - �����������/������/��������
		//		- minSrcHist ��������� � ����� ������� ����� ����������� ������� ������� ������� ������, ����������� ���������
		//			(��� �������������, ��������� ��������� � ������� ������, � �� size_t, ����� ��.����� ���������)
		//			����� �������, ��� minSrcHist ��� �� ����� ����� ������� ����, �� ������� ���������� �������� ��������� ���������,
		//			������� � ������ ��������� ����������, �� minSrcHist ������ ��������� ������� ���������� ����.
		//		- ��������� � ����� ���������� �������� � memb::adapterStor � ���������� ����� ���������.
		//			��������� ����� ���� ������ ������, ���� �� default constructible (������� ������
		//			������ ���� ������������ ���������, - ��� ���� ��������������� �� � ������ ��� �������� getTs)
		//		- � ��������� ��������� ���� �����, �������������� ������ ���� ���/������ ���������. ��. stateStorSelector.
		//			��� ����, ������������� ��� ��������� ������, ��� ������ ������� ��������� � _meta
		//			#todo ����� �������� ��� ����������� ������������� ���������� ���� � �����������
		//		- � ��������� ��������� �������� ���� ����� (��. TStorSelector � makeTStor_t). � ����� �������, ������ � _����
		//			algTStorDescr_t ����� ����� �������� ��������� ��������� ��������, ������� ����� �������������� �� ����������
		//			���������� ��� �������, ��� ��� �������� ���������� ����. ������ ����������� ���� ��������� ������� ��� �� ������-
		//			-�������� ���������, ��� � �� ���� ������� ����-�����. �� ��� ������� ��������� ������ ������ �������� �� ���
		//			"src", � ������, �� ������� src_value_t, ������� ������ ���� ���������� �������� � ����������� ��������.
		//			#todo ��� ��������� ������/�������� ��� ��� ������������� ������������� ������ ��������� - ���� �� �������.
		//			��� �� �� �������, ��� ����, ���� ��������� (��� ���������, ������), ������ ����������������� ������ �����,
		//			�������� �� ��.���������.
		//		- ������� ����� "����������" ���� tAlg ������ �� ��� "����� �������" tAdptPrmsState_base, ����� � �������
		//			����������� ���� ��������� ��������� ������ ��� ���� �� ���������� ���� ������ ���������, � � �������
		//			��� ������������� - ���� �� ������������������� �������� ��������� � ���������.
		//		- #todo ����������� ���������� ���������� ���������� ������ �� �������������� - ��� ���������� ����������
		//			� ������ ����������� ���������.
		//		- #todo �������� ����� ������� �������� ��������� ������ ���������� - ������ ��� ��� �����. ���� ������ ��������,
		//			��� ��������� ����� ����������� ������� ���������� ��� ���������, - ��� �� ����� ���� ���������
		//		
		
		typedef decltype("src"_s) adpt_src_ht;
		typedef decltype("dest"_s) adpt_dest_ht;

		template<typename HST, typename = ::std::enable_if_t<::boost::hana::is_a<::boost::hana::string_tag, HST>>>
		constexpr auto adptDefSubst_v = hana::make_pair(HST(), HST());

		template<typename... HSTs>
		using makeAdptDefSubstMap_t = decltype(hana::make_map(adptDefSubst_v<HSTs>...));

		//////////////////////////////////////////////////////////////////////////

		namespace code {
			template <typename BaseAlgT>
			struct lenBased_meta : public BaseAlgT {
				//alg_class_t is a helper type that helps to distinguish one algorithm from another or to instantiate proper alg
				typedef BaseAlgT base_class_t;

				typedef common_meta::prm_len_ht prm_len_ht;
				typedef typename base_class_t::prm_len_t prm_len_t;
				typedef decltype(hana::make_pair(prm_len_ht(), hana::type_c<prm_len_t>)) prm_len_descr;

				//minSrcHist variant to accept map of params
				using base_class_t::minSrcHist;
				template<typename HMT, typename = ::std::enable_if_t < hana::is_a < hana::map_tag, HMT > >>
				static size_t minSrcHist(HMT&& prms) noexcept {
					const auto v = prms[prm_len_ht()];
					T18_ASSERT(v > 0);
					return base_class_t::minSrcHist(static_cast<prm_len_t>(v));
				}

				//////////////////////////////////////////////////////////////////////////
				// placeholders for empty/unused state and tstor types
				typedef void algState_t;
				typedef void algTStorDescr_t;
			};

			template <typename BaseAlgT>
			struct noPrmsStateTStor_meta : public BaseAlgT {
				typedef BaseAlgT base_class_t;

				//////////////////////////////////////////////////////////////////////////
				// params description
				typedef utils::makeMap_t<> algPrmsDescr_t;

				typedef utils::dataMapFromDescrMap_t<algPrmsDescr_t> algPrmsMap_t;
				//algPrmsMap_t is a type that should be given to the algo as the params

				//////////////////////////////////////////////////////////////////////////
				//also specify additional internal/derived params to make runtime algo parameter set (nothing here)
				typedef algPrmsDescr_t algFullPrmsDescr_t;
				typedef algPrmsMap_t algFullPrmsMap_t;
				//algFullPrmsMap_t is a type that stores parameters as well as some internal data.

				//this function checks that passed parameters are suitable for the algo
				//i.e. that params have all the necessary keys and its values are convertible to the necessary types
				template<typename HMT>
				static decltype(auto) validatePrms(HMT&& prms) {
					static_assert(utils::couldBeDataMap_v<::std::remove_reference_t<HMT>>, "");
					//testing that the prms is suitable for the alg
					utils::static_assert_hmap_conforms_descr<algFullPrmsDescr_t>(prms);
					return ::std::forward<HMT>(prms);
				}

				//////////////////////////////////////////////////////////////////////////

				//minSrcHist variant to accept map of params
				using base_class_t::minSrcHist;
				template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
				static constexpr auto minSrcHist(HMT&&) noexcept {
					return base_class_t::minSrcHist();
				}
				template<typename CallerT, typename = ::std::enable_if_t<!hana::is_a<hana::map_tag, CallerT>>>
				static constexpr auto minSrcHist(const CallerT&) noexcept {
					return base_class_t::minSrcHist();
				}

				//////////////////////////////////////////////////////////////////////////
				// placeholders for empty/unused state and tstor types
				typedef void algState_t;
				typedef void algTStorDescr_t;
			};
		}


		//this namespace contains types that contain member variable and getters only. No other code is allowed here!
		namespace memb {

			//Adapter types could be complete types or references to complete types
			//AdptHMT is a hana::map that maps hana::string (name) to hana::type<adpt type> - i.e. descrMap
			template<typename AdptHMT>
			class adapterStor {
			public:
				static_assert(utils::isDescrMap_v<AdptHMT>, "");
				typedef AdptHMT adptDescrMap_t;
				typedef utils::dataMapFromDescrMap_t<AdptHMT> adptDataMap_t;

				//////////////////////////////////////////////////////////////////////////
				//the following is a quick and dirty fix for the requirement of tstor machinery to define src_value_t typedef
				
				typedef ::std::remove_pointer_t<typename decltype(+hana::at_key(adptDescrMap_t(), adpt_src_ht()))::type> srcAdpt_t;
				typedef typename srcAdpt_t::value_type src_value_t;
				//end of fix
				//////////////////////////////////////////////////////////////////////////

			protected:
				adptDataMap_t m_Adpts;

			public:
				template<typename HMT, typename = ::std::enable_if_t<::std::is_same_v<::std::remove_reference_t<HMT>, adptDataMap_t>>>
				adapterStor(HMT&& adpts) : m_Adpts(::std::forward<HMT>(adpts)) {}

				template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
				auto& getTs(const HST& k)noexcept { return utils::pointer2ref(m_Adpts[k]); }

				template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
				auto& getTs()noexcept { return getTs(HST()); }

				template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
				const auto& getTs(const HST& k)const noexcept { return utils::pointer2ref(m_Adpts[k]); }

				template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
				const auto& getTs()const noexcept { return getTs(HST()); }
			};

			//////////////////////////////////////////////////////////////////////////
			namespace _i {
				template<typename StateT>
				struct stateStor {
				protected:
					StateT m_state;
				public:
					StateT& getState()noexcept { return m_state; }
				};

				struct stateStorDummy {};
			}

			template<typename StateCandT>
			using stateStorSelector = ::std::conditional_t<::std::is_void_v<StateCandT>, _i::stateStorDummy, _i::stateStor<StateCandT>>;

			//////////////////////////////////////////////////////////////////////////
			namespace _i {
				template<typename AlgT, typename CallerT, typename HMT, typename HST>
				inline constexpr decltype(auto) assertUniqueAppendMap(HMT&& map, const HST& keyVal
					, ::std::enable_if_t<hana::value(hana::contains(::std::remove_reference_t<HMT>(), HST()))> * = nullptr)noexcept
				{
					//the key already exists in the map, we must make sure that it's value are totally the same as we were
					//going to set.
					static_assert(hana::is_a<hana::pair_tag, decltype(map[keyVal])>, "");
					static_assert(::std::is_same_v <
						typename AlgT::template TStor_tpl<HST, CallerT>
						, typename decltype(+hana::second(map[keyVal]))::type
					>, "TStor description requires to hold different temporarily storages under the same key, thats wrong!");
					return ::std::forward<HMT>(map);
				}
				template<typename AlgT, typename CallerT, typename HMT, typename HST>
				inline constexpr decltype(auto) assertUniqueAppendMap(HMT&& map, const HST& keyVal
					, ::std::enable_if_t<!hana::value(hana::contains(::std::remove_reference_t<HMT>(), HST()))> * = nullptr)noexcept
				{
					//no key in the map, so just add it
					return hana::insert(::std::forward<HMT>(map), hana::make_pair(keyVal
						, hana::make_pair(hana::type_c<AlgT>, hana::type_c<typename AlgT::template TStor_tpl<HST, CallerT>>)
					));
				}

				template<typename AlgT, typename CallerT, typename HMT>
				inline constexpr decltype(auto) doTStorMap(HMT&& map
					, ::std::enable_if_t<!::std::is_void_v<typename AlgT::algTStorDescr_t>>* = nullptr)noexcept
				{
					typedef typename AlgT::algTStorDescr_t algTStorDescr_t;
					static_assert(hana::is_a<hana::basic_tuple_tag, algTStorDescr_t>, "algTStorDescr_t must be a hana::basic_tuple");
					return hana::fold_left(algTStorDescr_t(), ::std::forward<HMT>(map), [](auto&& gmap, auto keyVal) {
						typedef decltype(keyVal) key_ht;
						static_assert(hana::is_a < hana::string_tag, key_ht>, "each algTStorDescr_t tuple element must be a hana::string");
						return assertUniqueAppendMap<AlgT, CallerT>(::std::move(gmap), keyVal);
					});
				}

				template<typename AlgT, typename CallerT, typename HMT>
				inline constexpr decltype(auto) doTStorMap(HMT&& map
					, ::std::enable_if_t<::std::is_void_v<typename AlgT::algTStorDescr_t>>* = nullptr)noexcept
				{
					return ::std::forward<HMT>(map);
				}

				//this function makes a temp store map for the tuple of algorithms
				//TStorDataMap is hana::map with keys made of hana strings and values of hana::pair's. The
				// first element of the pair is a type of TStor type required for the second element - Algorithm.
				// If two algorithm marks same TStor key, it ensures that the type of the TStor produced are also the same
				template<typename AlgsTupleT, typename CallerT>
				inline constexpr decltype(auto) makeTStorDataMap() {
					static_assert(hana::is_a<hana::basic_tuple_tag, AlgsTupleT>, "");
					return hana::fold_left(AlgsTupleT(), hana::make_map(), [](auto&& map, auto alg) {
						typedef typename decltype(alg)::type AlgT;
						return doTStorMap<AlgT, CallerT>(::std::move(map));
					});
				}
			}

			template<typename AlgsTupleT, typename CallerT>
			using makeTStor_t = decltype(_i::makeTStorDataMap<AlgsTupleT, CallerT>());

			namespace _i {
				template<typename TStorDataMapT>
				struct TStor {
				private:
					//makes map: key_ht -> TStor out of TStorDataMapT
					static decltype(auto) TStorDataMap2TStorageT()noexcept {
						static_assert(hana::is_a<hana::map_tag, TStorDataMapT>, "");
						return hana::fold_left(TStorDataMapT(), hana::make_map(), [](auto&& map, auto pr) {
							typedef typename decltype(+hana::second(hana::second(pr)))::type tstor_t;
							return hana::insert(::std::move(map), hana::make_pair(
								hana::first(pr), tstor_t()
							));
						});
					}
					typedef decltype(TStorDataMap2TStorageT()) TStorage_t;

				protected:
					TStorage_t m_tStor;

					template<typename CallerT>
					void _initTStorMap(CallerT& C) {
						hana::for_each(TStorDataMapT(), [&C, &tStor = m_tStor](auto&& pr) {
							typedef typename decltype(+hana::first(hana::second(pr)))::type alg_t;
							const auto& k = hana::first(pr);
							alg_t::initTStor(C, tStor[k], k);
						});
					}

				public:
					template<typename CallerT>
					TStor(CallerT& C) {
						_initTStorMap(C);
					}

					template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
					auto& getTStor(HST&& k)noexcept {
						return m_tStor[k];
					}
					template<typename HST, typename = ::std::enable_if_t<hana::is_a<hana::string_tag, HST>>>
					const auto& getTStor(HST&& k)const noexcept {
						return m_tStor[k];
					}
				};

				struct TStorDummy {
					template<typename T>
					TStorDummy(T&)noexcept {}
				};
			}

			template<typename TStorDataMapCandT>
			using TStorSelector = ::std::conditional_t<::std::is_same_v<decltype(hana::make_map()), TStorDataMapCandT>
				, _i::TStorDummy, _i::TStor<TStorDataMapCandT>
			>;
		}

		template<typename AdptBaseT, typename MetaCallerT>
		class tAdptPrmsState_base
			: public AdptBaseT
			, public memb::stateStorSelector<typename MetaCallerT::algState_t>
			, public MetaCallerT
		{
		public:
			typedef AdptBaseT base_class_adpt_t;
			using typename base_class_adpt_t::adptDataMap_t;
			
			typedef MetaCallerT base_code_t;
			typedef typename base_code_t::algFullPrmsMap_t algFullPrmsMap_t;
			typedef typename base_code_t::algFullPrmsDescr_t algFullPrmsDescr_t;

			static_assert(utils::isDescrMap_v<algFullPrmsDescr_t>, "");

		protected:
			algFullPrmsMap_t m_prms;

		protected:

			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			static decltype(auto) _constructPrms(HMT&& prms) {
				auto validatedPrms = base_code_t::validatePrms(::std::forward<HMT>(prms));
				return hana::fold_left(algFullPrmsDescr_t(), hana::make_map(), [&validatedPrms](auto map, auto pr)noexcept {
					auto kv = hana::first(pr);
					typedef typename ::std::remove_reference_t<decltype(::boost::hana::second(pr))>::type expectVal_t;
					return hana::insert(map, hana::make_pair(kv, static_cast<expectVal_t>(validatedPrms[kv])));
				});
			}

		public:
			template<typename AdptHMT, typename PrmsHMT, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<AdptHMT>, adptDataMap_t> && hana::is_a<hana::map_tag, PrmsHMT>
			>>
			tAdptPrmsState_base(AdptHMT&& adpts, PrmsHMT&& prms)
				: base_class_adpt_t(::std::forward<AdptHMT>(adpts))
				, m_prms(_constructPrms(::std::forward<PrmsHMT>(prms)))
			{}

			const algFullPrmsMap_t& getPrms()const noexcept {
				return m_prms;
			}
		};

		//#todo: initialization on tstore expects CallerT object to have a src_value_t typedef.
		//it's used to determine correct tstor type and binded to "src" timeseries
		// That mechanism mustn't use src_value_t, because now it doesn't allow to easily change tstor type
		// There must be a way to change that binding to any timeseries...

		//#todo MetaCallerT must be basic_tuple with alg types in it
		//memb::TStorSelector is ready for that, but other bases - don't
		template<typename FinalPolymorphChild, typename AdptBaseT, typename MetaCallerT>
		class tAlg
			: public tAdptPrmsState_base<AdptBaseT, MetaCallerT>
			, public memb::TStorSelector<memb::makeTStor_t<decltype(hana::make_basic_tuple(hana::type_c<MetaCallerT>)), tAdptPrmsState_base<AdptBaseT, MetaCallerT>>>
		{
		public:
			typedef FinalPolymorphChild self_t;
			T18_METHODS_SELF_CHECKED((::std::is_base_of_v<tAlg<FinalPolymorphChild, AdptBaseT, MetaCallerT>, self_t>)
				, "FinalPolymorphChild must be derived from algs::memb::adapterStor");

			typedef tAdptPrmsState_base<AdptBaseT, MetaCallerT> base_class_t;
			typedef memb::TStorSelector<memb::makeTStor_t<decltype(hana::make_basic_tuple(hana::type_c<MetaCallerT>)), tAdptPrmsState_base<AdptBaseT, MetaCallerT>>> base_tstor_t;

			using typename base_class_t::adptDataMap_t;

		public:
			template<typename AdptHMT, typename PrmsHMT, typename = ::std::enable_if_t<
				::std::is_same_v<::std::remove_reference_t<AdptHMT>, adptDataMap_t> && hana::is_a<hana::map_tag, PrmsHMT>
			>>
			tAlg(AdptHMT&& adpts, PrmsHMT&& prms)
				: base_class_t(::std::forward<AdptHMT>(adpts), ::std::forward<PrmsHMT>(prms))
				, base_tstor_t(get_self())
			{}

			//using base_class_t::minDestHist;
			using base_class_t::minSrcHist;
			auto minSrcHist() const noexcept {
				return base_class_t::minSrcHist(get_self());
			}

			//bool parameter determines whether the function is called on m_dest bar close. If true,
			//alg must update internal state accordingly, else the state must left intact.
			// See DEMA/TEMA for example, MA/EMA doesn't need/use this parameter
			self_ref_t operator()(bool bClose) noexcept {
				base_class_t::call(get_self(), bClose);
				return get_self();
			}

			//redefine in a derived container-based class to insert new bar into container
			self_ref_t notifyNewBarOpened() noexcept { return get_self(); }

			decltype(auto) operator[](size_t N)const noexcept {
				return get_self().getTs(typename base_class_t::adpt_dest_ht())[N];
			}
			decltype(auto) operator[](size_t N) noexcept {
				return get_self().getTs(typename base_class_t::adpt_dest_ht())[N];
			}
		};

		//////////////////////////////////////////////////////////////////////////
		template<typename FinalPolymorphChild, typename MetaCallerT
			, typename DVT /*= real_t*/, typename SVT /*= real_t*/, template<class> class ContTplT /*= TsCont_t*/>
		class tAlg2ts : public tAlg<FinalPolymorphChild, memb::adapterStor<utils::makeMap_t<
			utils::Descr_t<typename MetaCallerT::adpt_src_ht, const ContTplT<SVT>*const>
			, utils::Descr_t<typename MetaCallerT::adpt_dest_ht, ContTplT<DVT>*const>
			>>, MetaCallerT>
		{
		public:
			typedef tAlg<FinalPolymorphChild, memb::adapterStor<utils::makeMap_t<
				utils::Descr_t<typename MetaCallerT::adpt_src_ht, const ContTplT<SVT>*const>
				, utils::Descr_t<typename MetaCallerT::adpt_dest_ht, ContTplT<DVT>*const>
				>>, MetaCallerT> base_class_t;

			template <typename VT>
			using ContTpl_t = ContTplT<VT>;
		
			using typename base_class_t::adpt_src_ht;
			using typename base_class_t::adpt_dest_ht;

		public:
			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			tAlg2ts(ContTplT<DVT>& d, const ContTplT<SVT>& s, HMT&& prms)
				: base_class_t(hana::make_map(hana::make_pair(adpt_src_ht(), &s), hana::make_pair(adpt_dest_ht(), &d))
					, ::std::forward<HMT>(prms))
			{}
		};

		//note that for every class derived from this class notifyNewBarOpened() function MUST be called in order to
		//update/prepare container for a new bar!
		template<typename FinalPolymorphChild, typename MetaCallerT
			, typename DVT /*= real_t*/, typename SVT /*= real_t*/, template<class> class ContTplT /*= TsCont_t*/>
		class tAlg2ts_c : public tAlg<FinalPolymorphChild, memb::adapterStor<utils::makeMap_t<
			utils::Descr_t<typename MetaCallerT::adpt_src_ht, const ContTplT<SVT>*const>
			, utils::Descr_t<typename MetaCallerT::adpt_dest_ht, ContTplT<DVT>>
			>>, MetaCallerT>
		{
		public:
			typedef tAlg<FinalPolymorphChild, memb::adapterStor<utils::makeMap_t<
				utils::Descr_t<typename MetaCallerT::adpt_src_ht, const ContTplT<SVT>*const>
				, utils::Descr_t<typename MetaCallerT::adpt_dest_ht, ContTplT<DVT>>
				>>, MetaCallerT> base_class_t;

			using typename base_class_t::self_ref_t;
			using base_class_t::get_self;

			template <typename VT>
			using ContTpl_t = ContTplT<VT>;

			using typename base_class_t::adpt_src_ht;
			using typename base_class_t::adpt_dest_ht;

		public:
			template<typename HMT, typename = ::std::enable_if_t<hana::is_a<hana::map_tag, HMT>>>
			tAlg2ts_c(size_t nDestCapacity, const ContTplT<SVT>& s, HMT&& prms)
				: base_class_t(hana::make_map(
					hana::make_pair(adpt_src_ht(), &s)
					, hana::make_pair(adpt_dest_ht(), ContTplT<DVT>(::std::max(nDestCapacity, base_class_t::minDestHist())))
				), ::std::forward<HMT>(prms))
			{}

			self_ref_t notifyNewBarOpened()noexcept {
				base_class_t::getTs(adpt_dest_ht()).push_front(tNaN<DVT>);
				return get_self();
			}

		};

		//note that for every class derived from tAlg2ts_select<true, ...>, notifyNewBarOpened() function MUST be called in order to
		//update/prepare container for a new bar!
		template<bool bDestIsContainer, typename FinalPolymorphChild, typename MetaCallerT
			, typename DVT /*= real_t*/, typename SVT /*= real_t*/, template<class> class ContTplT /*= TsCont_t*/>
		using tAlg2ts_select = ::std::conditional_t<bDestIsContainer
			, tAlg2ts_c<FinalPolymorphChild, MetaCallerT, DVT, SVT, ContTplT>
			, tAlg2ts<FinalPolymorphChild, MetaCallerT, DVT, SVT, ContTplT>>;

	}
}

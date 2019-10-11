/*
This file is a part of NNTL project (https://github.com/Arech/nntl)

Copyright (c) 2015-2019, Arech (aradvert@gmail.com; https://github.com/Arech)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of NNTL nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

//////////////////////////////////////////////////////////////////////////
// forwarder is a very fast (about x7), but restrictive analog of ::std::function.
// cmcforwarder is even faster (about x7+ depending of functor size), but less versatile than the forwarder analog of ::std::function.
//		forwarder requires a functor to obey TriviallyCopyable concept
//		cmcforwarder requires a functor to obey only is_trivially_copy_constructible
//			or is_trivially_move_constructible concepts (depending on how the functor is passed to assign())
// Code was mostly taken from: https://github.com/user1095108/generic/blob/master/forwarder.hpp @2017.08.11
// Author (https://github.com/user1095108) didn't leave a licensing information. I contacted him @2017.08.11 via email
// and he gave me permission to use his code.

//namespace nntl {
	namespace utils {

		namespace {

			constexpr auto const default_noexcept =
#if defined(__cpp_exceptions) && __cpp_exceptions
				false;
#else
				true;
#endif // __cpp_exceptions

			constexpr auto const forwarder_storage_default_size = 4 * sizeof(void*);
		}

		namespace _impl {

			template <::std::size_t N>
			struct _forwarder_store {
			public:
				typedef ::std::aligned_storage_t<N> storage_t;
				static constexpr size_t store_size = sizeof(storage_t);
				static constexpr size_t size = N;

			protected:
				storage_t store_;
			};
		}

		//////////////////////////////////////////////////////////////////////////
		template <typename F, ::std::size_t N = forwarder_storage_default_size, bool NE = default_noexcept	>
		class cmcforwarder;

		template <typename R, typename ...A, ::std::size_t N, bool NE>
		class cmcforwarder<R(A...), N, NE> : public _impl::_forwarder_store<N>
		{
			typedef _impl::_forwarder_store<N> base_class_t;
			using base_class_t::store_;

		protected:
			R(*stub_)(void*, A&&...) noexcept(NE) {};

			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator==(cmcforwarder<T(U...), M> const&, ::std::nullptr_t) noexcept;
			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator==(::std::nullptr_t, cmcforwarder<T(U...), M> const&) noexcept;

			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator!=(cmcforwarder<T(U...), M> const&, ::std::nullptr_t) noexcept;
			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator!=(::std::nullptr_t, cmcforwarder<T(U...), M> const&) noexcept;

		public:
			using result_type = R;

			//enum : ::std::size_t { size = N };

		public:
			cmcforwarder() = default;
			//to prevent copying
// 			cmcforwarder(cmcforwarder const&) = delete;
// 			cmcforwarder(cmcforwarder&&) = delete;
// 			cmcforwarder& operator=(cmcforwarder const&) = delete;
// 			cmcforwarder& operator=(cmcforwarder&&) = delete;
			cmcforwarder(cmcforwarder const&) = default;
			cmcforwarder(cmcforwarder&&) = default;
			cmcforwarder& operator=(cmcforwarder const&) = default;
			cmcforwarder& operator=(cmcforwarder&&) = default;

			template <typename F, typename = ::std::enable_if_t<!::std::is_same<::std::decay_t<F>, cmcforwarder>::value>>
			cmcforwarder(F&& f) noexcept {
				assign(::std::forward<F>(f));
			}

			template <typename F, typename = ::std::enable_if_t<!::std::is_same<::std::decay_t<F>, cmcforwarder>::value>>
			cmcforwarder& operator=(F&& f) noexcept {
				assign(::std::forward<F>(f));
				return *this;
			}

			explicit operator bool() const noexcept { return stub_; }

			R operator()(A... args) const
				noexcept(noexcept(stub_(const_cast<void*>(static_cast<void const*>(&store_)), ::std::forward<A>(args)...)))
			{
				//assert(stub_);
				return stub_(const_cast<void*>(static_cast<void const*>(&store_)), ::std::forward<A>(args)... );
			}

			void assign(::std::nullptr_t) noexcept {
				reset();
			}

			template <typename F>
			void assign(F&& f) noexcept {
				typedef ::std::decay_t<F> functor_type;

				static_assert(sizeof(functor_type) <= sizeof(store_), "functor too large");
				static_assert(::std::is_trivially_destructible<functor_type>::value, "functor not trivially destructible");

				static_assert(::std::conditional_t<
					::std::is_lvalue_reference<F>::value
					, ::std::is_trivially_copy_constructible<functor_type>
					, ::std::is_trivially_move_constructible<functor_type>
				>::value, "functor not trivially copy/move constructible");

				::new (static_cast<void*>(&store_)) functor_type(::std::forward<F>(f));

				stub_ = [](void* const ptr, A&&... args) noexcept(noexcept(NE)) -> R
				{
/*#if __cplusplus <= 201402L
					return (*static_cast<functor_type*>(ptr))(
						::std::forward<A>(args)...);
#else*/
					return ::std::invoke(*static_cast<functor_type*>(ptr),
						::std::forward<A>(args)...);
//#endif // __cplusplus
				};
			}

			void reset() noexcept { stub_ = nullptr; }

			template <typename T>
			auto target() noexcept {
				//#todo for C++17 must change to ::std::launder(reinterpret_cast<T*>(&store_))
				return reinterpret_cast<T*>(&store_);
			}

			template <typename T>
			auto target() const noexcept {
				//#todo for C++17 must change to ::std::launder(reinterpret_cast<T const*>(&store_))
				return reinterpret_cast<T const*>(&store_);
			}
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator==(cmcforwarder<R(A...), N> const& f, ::std::nullptr_t const) noexcept {
			return nullptr == f.stub_;
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator==(::std::nullptr_t const, cmcforwarder<R(A...), N> const& f) noexcept {
			return nullptr == f.stub_;
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator!=(cmcforwarder<R(A...), N> const& f, ::std::nullptr_t const) noexcept {
			return !operator==(nullptr, f);
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator!=(::std::nullptr_t const, cmcforwarder<R(A...), N> const& f) noexcept {
			return !operator==(nullptr, f);
		};

		//////////////////////////////////////////////////////////////////////////

		template <typename F, ::std::size_t N = forwarder_storage_default_size, bool NE = default_noexcept	>
		class forwarder;

		template <typename R, typename ...A, ::std::size_t N, bool NE>
		class forwarder<R(A...), N, NE> : public cmcforwarder<R(A...), N, NE>
		{
			typedef cmcforwarder<R(A...), N, NE> base_class_t;
			using base_class_t::store_;

		public:
			forwarder() = default;
			forwarder(forwarder const&) = default;
			forwarder(forwarder&&) = default;
			forwarder& operator=(forwarder const&) = default;
			forwarder& operator=(forwarder&&) = default;

			template <typename F, typename = ::std::enable_if_t < !::std::is_same<::std::decay_t<F>, forwarder>::value > >
			forwarder(F&& f) noexcept {
				assign(::std::forward<F>(f));
			}

			template <typename F, typename = ::std::enable_if_t < !::std::is_same<::std::decay_t<F>, forwarder>::value > >
			forwarder& operator=(F&& f) noexcept {
				assign(::std::forward<F>(f));
				return *this;
			}
			
			template <typename F>
			void assign(F&& f) noexcept {
				typedef ::std::decay_t<F> functor_type;

				static_assert(sizeof(functor_type) <= sizeof(store_), "functor too large");
				static_assert(::std::is_trivially_copyable<functor_type>::value, "functor not trivially copyable");
				static_assert(::std::is_trivially_destructible<functor_type>::value, "functor not trivially destructible");

				::new (static_cast<void*>(&store_)) functor_type(::std::forward<F>(f));

				stub_ = [](void* const ptr, A&&... args) noexcept(noexcept(NE)) -> R
				{
#if __cplusplus <= 201402L
					return (*static_cast<functor_type*>(ptr))(
						::std::forward<A>(args)...);
#else
					return ::std::invoke(*static_cast<functor_type*>(ptr),
						::std::forward<A>(args)...);
#endif // __cplusplus
				};
			}
			
			void swap(forwarder& other) noexcept {
				::std::swap(*this, other);
			}

			void swap(forwarder&& other) noexcept {
				::std::swap(*this, ::std::move(other));
			}
		};

		//////////////////////////////////////////////////////////////////////////
		/*template <typename F, ::std::size_t N = forwarder_storage_default_size, bool NE = default_noexcept	>
		class forwarder;

		template <typename R, typename ...A, ::std::size_t N, bool NE>
		class forwarder<R(A...), N, NE> : public _impl::_forwarder_store<N>
		{
			R(*stub_)(void*, A&&...) noexcept(NE) {};

			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator==(forwarder<T(U...), M> const&, ::std::nullptr_t) noexcept;
			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator==(::std::nullptr_t, forwarder<T(U...), M> const&) noexcept;

			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator!=(forwarder<T(U...), M> const&, ::std::nullptr_t) noexcept;
			template<typename T, typename ...U, ::std::size_t M>
			friend bool operator!=(::std::nullptr_t, forwarder<T(U...), M> const&) noexcept;

		public:
			using result_type = R;

			enum : ::std::size_t { size = N };

		public:
			forwarder() = default;
			forwarder(forwarder const&) = default;
			forwarder(forwarder&&) = default;

			template <typename F, typename = ::std::enable_if_t < !::std::is_same<::std::decay_t<F>, forwarder>::value > >
			forwarder(F&& f) noexcept {
				assign(::std::forward<F>(f));
			}

			forwarder& operator=(forwarder const&) = default;
			forwarder& operator=(forwarder&&) = default;

			template <typename F, typename = ::std::enable_if_t < !::std::is_same<::std::decay_t<F>, forwarder>::value > >
			forwarder& operator=(F&& f) noexcept {
				assign(::std::forward<F>(f));
				return *this;
			}

			explicit operator bool() const noexcept { return stub_; }

			R operator()(A... args) const
				noexcept(noexcept(stub_(const_cast<void*>(static_cast<void const*>(&store_)), ::std::forward<A>(args)...)))
			{
				//assert(stub_);
				return stub_(const_cast<void*>(static_cast<void const*>(&store_)),
					::std::forward<A>(args)...
				);
			}

			void assign(::std::nullptr_t) noexcept {
				reset();
			}

			template <typename F>
			void assign(F&& f) noexcept {
				typedef ::std::decay_t<F> functor_type;

				static_assert(sizeof(functor_type) <= sizeof(store_), "functor too large");
				static_assert(::std::is_trivially_copyable<functor_type>::value, "functor not trivially copyable");
				static_assert(::std::is_trivially_destructible<functor_type>::value, "functor not trivially destructible");

				::new (static_cast<void*>(&store_)) functor_type(::std::forward<F>(f));

				stub_ = [](void* const ptr, A&&... args) noexcept(noexcept(NE)) -> R
				{
#if __cplusplus <= 201402L
					return (*static_cast<functor_type*>(ptr))(
						::std::forward<A>(args)...);
#else
					return ::std::invoke(*static_cast<functor_type*>(ptr),
						::std::forward<A>(args)...);
#endif // __cplusplus
				};
			}

			void reset() noexcept { stub_ = nullptr; }

			void swap(forwarder& other) noexcept {
				::std::swap(*this, other);
			}

			void swap(forwarder&& other) noexcept {
				::std::swap(*this, ::std::move(other));
			}

			template <typename T>
			auto target() noexcept {
				return reinterpret_cast<T*>(&store_);
			}

			template <typename T>
			auto target() const noexcept {
				return reinterpret_cast<T const*>(&store_);
			}
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator==(forwarder<R(A...), N> const& f, ::std::nullptr_t const) noexcept {
			return nullptr == f.stub_;
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator==(::std::nullptr_t const, forwarder<R(A...), N> const& f) noexcept {
			return nullptr == f.stub_;
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator!=(forwarder<R(A...), N> const& f, ::std::nullptr_t const) noexcept {
			return !operator==(nullptr, f);
		};

		template<typename R, typename ...A, ::std::size_t N>
		bool operator!=(::std::nullptr_t const, forwarder<R(A...), N> const& f) noexcept {
			return !operator==(nullptr, f);
		};*/

		template<size_t N>
		struct cmcforwarderWrap {
			template<typename F>
			static auto wrap(F&& f
				, ::std::enable_if_t<(sizeof(::std::decay_t<F>) <= _impl::_forwarder_store<N>::size)>* = nullptr)noexcept
				-> decltype(::std::forward<F>(f))//without this specification "auto" will remove lreference from forwarded type
			{
				return ::std::forward<F>(f);
			}

			/*template<typename F>
			//nntl_static_warning("Too big object passed! Had to pass it by ref. Increase N?")
			static auto wrap(F&& f
				, ::std::enable_if_t<(sizeof(::std::decay_t<F>) > _impl::_forwarder_store<N>::size)>* = nullptr)noexcept
				-> decltype(::std::ref(f))//shouldn't trust auto
			{
				return ::std::ref(f);
			}*/
		};

		template<::std::size_t N>
		struct forwarderWrap {
			//typedef utils::forwarder<void(int64_t)> call_through_t;

			template<typename F>
			static auto wrap(F&& f, ::std::enable_if_t<::std::is_trivially_copyable<::std::decay_t<F>>::value
				&& (sizeof(::std::decay_t<F>) <= _impl::_forwarder_store<N>::size)>* = nullptr)noexcept
				-> decltype(::std::forward<F>(f))//without this specification "auto" will remove lreference from forwarded type
			{
				return ::std::forward<F>(f);
			}

			template<typename F>
			//nntl_static_warning("Too big object passed! Had to pass it by ref. Increase N?")
			static auto wrap(F&& f, ::std::enable_if_t<::std::is_trivially_copyable<::std::decay_t<F>>::value
				&& (sizeof(::std::decay_t<F>) > _impl::_forwarder_store<N>::size)>* = nullptr)noexcept
				-> decltype(::std::ref(f))//shouldn't trust auto
			{
				return ::std::ref(f);
			}

			template<typename F>
			static auto wrap(F&& f, ::std::enable_if_t<!::std::is_trivially_copyable<::std::decay_t<F>>::value>* = nullptr)noexcept
				-> decltype(::std::ref(f))//shouldn't trust auto
			{
				return ::std::ref(f);
			}
		};
	}
//}
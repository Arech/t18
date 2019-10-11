/*
This file is a part of NNTL project (https://github.com/Arech/nntl)

Copyright (c) 2015-2019, Arech (al.rech@gmail.com; https://github.com/Arech)
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

#include "forwarder.h"

//namespace nntl {

	namespace utils {
		//////////////////////////////////////////////////////////////////////////
		// wrappers to use with calls. See use-case in ::nntl::threads::*

		//for full-featured machinery like ::std::function
		//template<typename BT>
		template<template <class> class CTpl>
		struct simpleWrapper {
			template<typename FuncSigT>
			using call_tpl = CTpl<FuncSigT>;
			//typedef BT call_through_t;

			template<typename F>
			static auto wrap(F&& f)noexcept
				-> decltype(::std::forward<F>(f))//without this specification "auto" will remove lreference from forwarded type
			{
				return ::std::forward<F>(f);
			}
		};

		template<::std::size_t N = forwarder_storage_default_size>
		struct forwarderWrapper : forwarderWrap<N> {
			template<typename FuncSigT>
			using call_tpl = forwarder<FuncSigT, N>;
		};

		template<::std::size_t N = forwarder_storage_default_size>
		struct cmcforwarderWrapper : cmcforwarderWrap<N> {
			template<typename FuncSigT>
			using call_tpl = cmcforwarder<FuncSigT, N>;
		};

	}
//}
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

#define T18_ATTR_NORETURN [[noreturn]]
#define T18_ATTR_NODISCARD [[nodiscard]]

//thanks to https://stackoverflow.com/questions/14335494/how-to-deal-with-exit-time-destructor-warning-in-clang
#define T18_DEFINE_STATIC_LOCAL(type, name, arguments) static type& name = *new type arguments

//////////////////////////////////////////////////////////////////////////

#define T18_COMP_SILENCE_FLOAT_CMP_UNSAFE _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wfloat-equal\"")

#define T18_COMP_SILENCE_C17EXT _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wc++17-extensions\"")

#define T18_COMP_SILENCE_STRING_CONV _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wstring-conversion\"")

#define T18_COMP_SILENCE_EXIT_DESTRUCTOR _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"")

#define T18_COMP_SILENCE_COVERED_SWITCH _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wcovered-switch-default\"")

#define T18_COMP_SILENCE_MISSING_NORETURN _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wmissing-noreturn\"")

#define T18_COMP_SILENCE_THROWING_NOEXCEPT _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wexceptions\"")

#define T18_COMP_SILENCE_ZERO_AS_NULLPTR _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wzero-as-null-pointer-constant\"")

#define T18_COMP_SILENCE_OLD_STYLE_CAST _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wold-style-cast\"")

#define T18_COMP_SILENCE_DROP_CONST_QUAL _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wcast-qual\"")

#define T18_COMP_SILENCE_SIGN_CONVERSION _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")

#define T18_COMP_SILENCE_DEPRECATED _Pragma("clang diagnostic push") \
     _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

#define T18_COMP_POP _Pragma("clang diagnostic pop")

#define T18_COMP_SILENCE_REQ_GLOBAL_CONSTR _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"")

#define STRINGIZE(t) #t
#define _T18_COMP_PRAGMA_PACK(n) STRINGIZE(pack(push, n))

#define T18_COMP_PRAGMA_PACK(n) _Pragma(_T18_COMP_PRAGMA_PACK(n))
#define T18_COMP_PRAGMA_PACK_POP _Pragma("pack(pop)")

#define T18_COMP_PRAGMA_PACK_POP_ASSERT_SIZE(type,size) _Pragma("pack(pop)") \
    static_assert(sizeof(type)==size,"WTF? size doesn't match!");


//////////////////////////////////////////////////////////////////////////

//thanks to https://stackoverflow.com/a/43870188/1974258
#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
//http://llvm.org/docs/BranchWeightMetadata.html#builtin-expect
#define LIKELY_CASE(var, likelyCaseNum) __builtin_expect((var), (likelyCaseNum))

#define T18_UNREF(a) ((void)(a))

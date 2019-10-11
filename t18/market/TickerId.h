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

#include "../tags.h"

namespace t18 {
	using namespace ::std::literals;

	struct TickerId {
		::std::uint64_t tiid;

		constexpr TickerId(::std::uint32_t typeId, ::std::uint32_t idx) noexcept : tiid(makeTiid(typeId, idx)) {}

		//need this trick to support 32bit builds
		template<typename T, typename = ::std::enable_if_t<::std::is_same_v<T,size_t> && sizeof(T) != sizeof(::std::uint32_t)>>
		constexpr TickerId(size_t typeId, T idx)noexcept
			: tiid(makeTiid(static_cast<::std::uint32_t>(typeId), static_cast<::std::uint32_t>(idx)))
		{
			T18_ASSERT(typeId < ::std::numeric_limits<::std::uint32_t>::max() && idx < ::std::numeric_limits<::std::uint32_t>::max());
		}

	private:
		static constexpr auto everyone = ::std::numeric_limits<decltype(tiid)>::max();
		constexpr TickerId(tag_Ticker_t)noexcept : tiid(everyone) {}

	public:
		constexpr bool isEveryone()const noexcept { return tiid == everyone; }

		static constexpr TickerId forEveryone()noexcept {
			return TickerId(tag_Ticker_t());
		}

		constexpr bool operator==(const TickerId& r)const noexcept { return tiid == r.tiid; }
		constexpr bool operator!=(const TickerId& r)const noexcept { return tiid != r.tiid; }

		static constexpr ::std::uint64_t makeTiid(::std::uint32_t typeId, ::std::uint32_t idx)noexcept {
			return (static_cast<::std::uint64_t>(typeId) << 32) | idx;
		}

		constexpr size_t getTypeId()const noexcept {
			return static_cast<size_t>(tiid >> 32);
		}
		constexpr size_t getIdx()const noexcept {
			return static_cast<size_t>(tiid & 0xFFFFFFFF);
		}
	};


}

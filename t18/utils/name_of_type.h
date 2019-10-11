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

#include <string>
#include <boost/core/demangle.hpp>

namespace utils {

	template <typename T>
	std::string const& name_of_type() {
		//static std::string name = boost::core::demangle(typeid(T).name());
		T18_DEFINE_STATIC_LOCAL(std::string, name, (boost::core::demangle(typeid(T).name())));
		return name;
	}
	template <typename T>
	std::string const& name_of_type(T&&) {
		//static std::string name = boost::core::demangle(typeid(::std::remove_reference_t<T>).name());
		T18_DEFINE_STATIC_LOCAL(std::string, name, (boost::core::demangle(typeid(::std::remove_reference_t<T>).name())));
		return name;
	}
}
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

struct CopyMoveTestClass {
	int cc = 0, mc = 0, oc = 0, om = 0, vtc = 0;

	CopyMoveTestClass() {}
	CopyMoveTestClass(int v) :vtc(v) {}
	void reset(int v) {
		cc = 0; mc = 0; oc = 0; om = 0; vtc = v;
	}
	void copyFrom(const CopyMoveTestClass& v) {
		if (this != &v) {
			cc = v.cc;
			mc = v.mc;
			oc = v.oc;
			om = v.om;
			vtc = v.vtc;
		}
	}
	void moveFrom(CopyMoveTestClass&& v) {
		copyFrom(v);
		/*if (this != &v) {
		v.cc = -100;			v.mc = -100;			v.oc = -100;			v.om = -100;			v.vtc = -100;
		}*/
	}

	CopyMoveTestClass(const CopyMoveTestClass& v) {
		copyFrom(v);
		++cc;
	}
	CopyMoveTestClass& operator=(const CopyMoveTestClass& v) {
		copyFrom(v);
		++oc;
		return *this;
	}

	CopyMoveTestClass(CopyMoveTestClass&& v) {
		moveFrom(::std::move(v));
		++mc;
	}
	CopyMoveTestClass& operator=(CopyMoveTestClass&& v) {
		moveFrom(::std::move(v));
		++om;
		return *this;
	}
	void report()const noexcept {
		STDCOUTL("constructors: copy=" << cc << ", move=" << mc << ". operator=: copy=" << oc << ", move=" << om << ". VTC=" << vtc);
	}
	~CopyMoveTestClass() {
		report();
	}
};


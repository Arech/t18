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

namespace t18 {
	//////////////////////////////////////////////////////////////////////////
	//this class helps to decide whether the incoming bar/deal must be added to the storage or it should be
	//rejected because of inappropriate time
	struct TimeFilter {
		mxTime m_rejectEverthingEarlierThan;
		mxTime m_rejectEverthingLaterOrEqualThan;

		TimeFilter()noexcept {}
		TimeFilter(mxTime reet) noexcept : m_rejectEverthingEarlierThan(reet) {}
		TimeFilter(mxTime reet, mxTime reloet) noexcept : m_rejectEverthingEarlierThan(reet), m_rejectEverthingLaterOrEqualThan(reloet) {}
		TimeFilter(int, mxTime reloet) noexcept : m_rejectEverthingLaterOrEqualThan(reloet) {}

		bool shouldReject(mxTime t)const noexcept {
			return (!m_rejectEverthingEarlierThan.empty() && t < m_rejectEverthingEarlierThan)
				|| (!m_rejectEverthingLaterOrEqualThan.empty() && t >= m_rejectEverthingLaterOrEqualThan);
		}

		bool shouldAllow(mxTime t)const noexcept {
			return !shouldReject(t);
		}

		bool active()const noexcept {
			return !(m_rejectEverthingEarlierThan.empty() && m_rejectEverthingLaterOrEqualThan.empty());
		}
	};
}

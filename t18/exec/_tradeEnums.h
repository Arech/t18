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

	enum class TradeState : char {
		_NotConstructed,
		PendingOpen,//order has been sent, but not evaluated yet
		Opening,//at least some of requested amount has been satifsfied
		OpenFailed,

		inMarket,//all requested amount has been satisfied. Trade totally "in market"

		PendingClose,
		Closing,
		CloseFailed,
		Closed
	};

	//inline const ::std::string& to_string(const TradeState& cr) {
	inline const char* describe(TradeState cr) {
		switch (cr) {
		case TradeState::_NotConstructed:
			return "NotConstructed!";

		case TradeState::PendingOpen:
			return "PendingOpen";

		case TradeState::Opening:
			return "Opening";

		case TradeState::OpenFailed:
			return "OpenFailed";

		case TradeState::inMarket:
			return "inMarket";

		case TradeState::PendingClose:
			return "PendingClose";

		case TradeState::Closing:
			return "Closing";

		case TradeState::CloseFailed:
			return "CloseFailed";

		case TradeState::Closed:
			return "Closed";

			T18_COMP_SILENCE_COVERED_SWITCH;
		default:
			T18_COMP_POP;

			T18_ASSERT(!"Unknown TradeState enum value!");
			throw ::std::runtime_error("Unknown TradeState enum value!");
		}
	}

	//////////////////////////////////////////////////////////////////////////

	enum class TradeFailureCode : char {
		_NoFailure
		, extraDealDuringOpening//total volume exceeded max expected
		, extraDealDuringClosing//total volume exceeded max expected
		, unexpectedDeal//got deal callback during state that mustn't issue position change orders
		, invalidDealDirectionDuringOpening
		, invalidDealDirectionDuringClosing
		, invalidVolumeOnOpening
		, invalidVolumeOnClosing
		, totalVolumeLessZero
		, exceededVolume
		, dealFailed
	};

	inline const char* describe(TradeFailureCode fc) {
		switch (fc) {
		case TradeFailureCode::_NoFailure:
			return "NoFailure";

		case TradeFailureCode::extraDealDuringOpening:
			return "extraDealDuringOpening";

		case TradeFailureCode::extraDealDuringClosing:
			return "extraDealDuringClosing";

		case TradeFailureCode::unexpectedDeal:
			return "unexpectedDeal";

		case TradeFailureCode::invalidDealDirectionDuringOpening:
			return "invalidDealDirectionDuringOpening";

		case TradeFailureCode::invalidDealDirectionDuringClosing:
			return "invalidDealDirectionDuringClosing";

		case TradeFailureCode::invalidVolumeOnOpening:
			return "invalidVolumeOnOpening";

		case TradeFailureCode::invalidVolumeOnClosing:
			return "invalidVolumeOnClosing";

		case TradeFailureCode::totalVolumeLessZero:
			return "totalVolumeLessZero";

		case TradeFailureCode::exceededVolume:
			return "exceededVolume";

		case TradeFailureCode::dealFailed:
			return "dealFailed";

			T18_COMP_SILENCE_COVERED_SWITCH;
		default:
			T18_COMP_POP;

			T18_ASSERT(!"Unknown TradeFailureCode enum value!");
			throw ::std::runtime_error("Unknown TradeFailureCode enum value!");
		}
	}

	//////////////////////////////////////////////////////////////////////////

	enum class TradeCloseReason : char {
		_UNDEFINED,
		Normal,
		StopLoss,
		TakeProfit,
		ABORTED
	};
	inline const char* describe(TradeCloseReason cr) {
		switch (cr) {
		case TradeCloseReason::_UNDEFINED:
			return "UNDEFINED!";

		case TradeCloseReason::Normal:
			return "Normal";

		case TradeCloseReason::StopLoss:
			return "StopLoss";

		case TradeCloseReason::TakeProfit:
			return "TakeProfit";

		case TradeCloseReason::ABORTED:
			return "ABORTED!";

			T18_COMP_SILENCE_COVERED_SWITCH;
		default:
			T18_COMP_POP;

			T18_ASSERT(!"Unknown TradeCloseReason enum value!");
			throw ::std::runtime_error("Unknown TradeCloseReason enum value!");
		}
	}

}

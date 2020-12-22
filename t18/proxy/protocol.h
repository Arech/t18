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

#include "../base.h"

namespace t18 {
	namespace proxy {

		static constexpr ::std::uint16_t defaultServerTcpPort{ 8945 };
		static constexpr size_t defaultMaxDealsToSendOnAllTrades = 512;

		//server's send a heartbeat packet as the first packet on connection event to pass the most important isQuikConnected flag
		//and test connection
		enum class ProtoCli2Srv : ::std::uint8_t {
			heartbeat //#todo to ensure the connection is alive, when there's nothing more to send client will send heartbeat packets
			//server must return whether it is connected to quik and whether quik is connected to broker's server.
			//#todo <what else?>

			//, init //client sends init packet with the ip:port opened to listen server's connection & session key/id to
			// authenticate server connection
			// server must return whether it is connected to quik and whether quik is connected to broker's server.
			//#todo <what else?>

			, getClassesList //gets result of quik.getClassesList()
			, listAllTickers //gets results of quik.getClassSecurities() for each quik.getClassesList()

			, queryTickerInfo //pass class&securityids of tickers to get info on them
			//For example : "QJSIM(SBER,GAZP)" returns prxyTickerInfo structure on each of tickers specified in request

			, subscribeAllTrades //pass class&securityids of tickers to subscribe and time of earliest known deal in current session
			// (or nothing to fetch all deals since session start).
			// For example: "QJSIM(SBER|,GAZP|100511.000003)" requests GAZP ticker since 10:05:11.000003 and all deals
			//		for SBER since session start.
			//server responds with stream of deals for each ticker after (not including) specified time.
			// after that, server sends 
			


			//#todo
			//, querySubscribeAllTrades //asks server to return info which tickers are subscribed

			//#todo
			//, cancelAllTrades //pass class&securityids of tickers to unsubscribe, or null to cancel all
			//server responds with a list of unsubscribed tickers and timestamp of a last deal on each ticker
		};

		constexpr inline bool packetHaveOnlyHeader(const ProtoCli2Srv c2s)noexcept {
			return c2s == ProtoCli2Srv::heartbeat || c2s == ProtoCli2Srv::getClassesList || c2s == ProtoCli2Srv::listAllTickers;
		}

		T18_COMP_PRAGMA_PACK(4);
		//generally if it's possible, it's better not to use flexible array members (such as char data[]).
		// for us now it's entirely no burden to not use them. The only trick is to NEVER use sizeof(Cli2Srv_PacketHdr)
		// when designating the amount of bytes to read from the stream.
		// Also, we need to 
		struct Cli2Srv_PacketHdr {
			::std::uint16_t wTotalPacketLen;
			::std::uint8_t bPacketType;//enum ProtoCli2Srv
			::std::uint8_t _padding;
		};
		//always use this constant instead of sizeof(Cli2Srv_PacketHdr)
		static constexpr size_t Cli2Srv_PacketHdr_Size = 4;
		T18_COMP_PRAGMA_PACK_POP_ASSERT_SIZE(Cli2Srv_PacketHdr, Cli2Srv_PacketHdr_Size);

		static constexpr size_t maxPossible_Cli2Srv_Packet_Payload_Len = ::std::numeric_limits<decltype(Cli2Srv_PacketHdr::wTotalPacketLen)>::max();

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		
		enum class ProtoSrv2Cli : ::std::uint8_t {
			heartbeat
			//to ensure the connection is alive, when there's nothing more to send server will send heartbeat packets

			//#todo
			//, subscribeAllTradesState //returns the subscription state of tickers on querySubscribeAllTrades.

			, queryTickerInfoResult //result of querying ticker info. One packet for each ticker
			//bit 2 of Srv2Cli_PacketHdr::bFlags contains the success flag for the ticker.
			// If it is 1, then info retrieval succeeded and the payload contains prxyTickerInfo structure.
			// If is 0, then there was some error for the ticker@class written in payload

			, subscribeAllTradesResult //returns the result of subscription for each ticker, specified in subscribeAllTrades,
			//before any trades/deal data will be sent.
			//bit 2 of Srv2Cli_PacketHdr::bFlags contains result of subscribeAllTrades for the ticker.
			// If it is 1, then subscribe succeeded and the payload contains prxyTickerInfo structure.
			// If is 0, then there was some error for the ticker@class written in payload

			, getClassesListResult
			, listAllTickersResult

			//#todo
			, AllTrades
			//#todo
			, requestFailed //sends error string to describe error

		};

		constexpr inline bool packetHaveOnlyHeader(const ProtoSrv2Cli s2c)noexcept {
			return s2c == ProtoSrv2Cli::heartbeat;
		}

		//////////////////////////////////////////////////////////////////////////
		typedef ::std::uint8_t Srv2Cli_PacketHdr_flags_t;

		T18_COMP_PRAGMA_PACK(4);
		struct Srv2Cli_PacketHdr {
			::std::uint16_t wTotalPacketLen;
			::std::uint8_t bPacketType;
			Srv2Cli_PacketHdr_flags_t bFlags; //the last bit is reserved to contain isQuikConnected flag. The others depend on packet type
		};
		//always use this constant instead of sizeof(Srv2Cli_PacketHdr)
		static constexpr size_t Srv2Cli_PacketHdr_Size = 4;
		T18_COMP_PRAGMA_PACK_POP_ASSERT_SIZE(Srv2Cli_PacketHdr, Srv2Cli_PacketHdr_Size);

		/*T18_COMP_PRAGMA_PACK(1);
		struct Srv2Cli_Packet {
			Srv2Cli_PacketHdr hdr;
			char serialized[];
		};
		T18_COMP_PRAGMA_PACK_POP;*/

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		
		typedef ::std::uint8_t intTickerId_t;
		typedef ::std::uint32_t volume_lots_t;

		T18_COMP_PRAGMA_PACK(4);
		struct prxyTsDeal {
			mxTimestamp ts;
			real_t pr;
			volume_lots_t volLots;
			dealnum_t dealNum;

			::std::uint8_t bLong;
			intTickerId_t tid;

			::std::uint8_t _padding[2];

			prxyTsDeal()noexcept{}
			prxyTsDeal(mxTimestamp _t, real_t _p, volume_lots_t _v, dealnum_t _n, bool bL, intTickerId_t _id) noexcept
				: ts(_t), pr(_p), volLots(_v), dealNum(_n), bLong(static_cast<decltype(bLong)>(bL)), tid(_id)
			{}

			const char* _to_string() const{
				static char _b[128];
				sprintf_s(_b, "%s %0.4f %u (%u) %s", ts.to_string().c_str(), pr, volLots, dealNum, bLong ? "BUY" : "SELL");
				return static_cast<const char*>(_b);
			}

			tsDirTick to_tsDirTick(const volume_lots_t lotSize)const noexcept {
				T18_ASSERT(lotSize > 0);
				return tsDirTick(ts, pr, static_cast<volume_t>(volLots*lotSize), static_cast<bool>(bLong));
			}
			tsTick to_tsTick(const volume_lots_t lotSize)const noexcept {
				T18_ASSERT(lotSize > 0);
				return tsTick(ts, pr, static_cast<volume_t>(volLots*lotSize));
			}
		};
		static constexpr size_t prxyTsDeal_Size = 8 + (sizeof(real_t) > sizeof(int) ? 8 : 4) + 4 + 4 + 1 + 1 + 2;
		T18_COMP_PRAGMA_PACK_POP_ASSERT_SIZE(prxyTsDeal, prxyTsDeal_Size);

		T18_COMP_PRAGMA_PACK(4);
		struct prxyTickerInfo{
			real_t minStepSize;
			int precision;
			volume_lots_t lotSize;

			intTickerId_t tid;

			//right after the tid member there's a null-terminated string with ticker@class code for the ticker
			//char szTickerName[];
			// commented out to get rid of c99 warning. Just never use sizeof(prxyTickerInfo), use prxyTickerInfo_Size instead
			
			bool isValid()const noexcept {
				return minStepSize > 0;//that is enought;
			}

			char* _ofsPastEOS()noexcept {
				return reinterpret_cast<char*>(this) + offsetof(prxyTickerInfo, tid) + 1;
			}
			const char* _ofsPastEOS()const noexcept {
				return reinterpret_cast<const char*>(this) + offsetof(prxyTickerInfo, tid) + 1;
			}

			::std::string to_string() const {
				char b[256];
				sprintf_s(b, "(%u) lots=%u, prec=%d, stepsize=%.6f", static_cast<unsigned>(tid), lotSize, precision, minStepSize);
				return ::std::string(b);
			}

			static prxyTickerInfo createInvalid()noexcept { return{ 0,0,0,0 }; }
			void reset()noexcept {
				minStepSize = real_t(0);
				precision = 0;
				lotSize = 0;
				tid = 0;
			}
		};
		static constexpr size_t prxyTickerInfo_Size = (sizeof(real_t) > sizeof(int) ? 8 : 4) + 4 + 4 + 1;
		T18_COMP_PRAGMA_PACK_POP_ASSERT_SIZE(prxyTickerInfo, prxyTickerInfo_Size + 3);//+3 for padding

	}
}

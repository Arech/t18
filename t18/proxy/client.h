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

#include <functional>
#include <iostream>
#include <string>
#include <deque>
#include <cstring>

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

#include "protocol.h"

//#WARNING, #include <spdlog/spdlog.h> must be done with all proper settings earlier before including current file
//#note, that only _mt sinks/loggers must be used if a separate thread started to call io_context::run()
//
//this file must additionally be included to output some asio shit...
#include <spdlog/fmt/bundled/ostream.h>

//https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/example/cpp11/timeouts/async_tcp_client.cpp
namespace t18 {
	namespace proxy {
		namespace asio = ::boost::asio;
		using asio::ip::tcp;
		//using asio::ip::udp;

		template<typename HandlerT>
		class client {
		protected:
			//typedef ::std::vector<::std::uint8_t> data_packet_raw_t;
			typedef ::std::vector<char> data_packet_raw_t;

		public:
			static constexpr long timeoutConnectMs = HandlerT::timeoutConnectMs;
			static constexpr long timeoutReadMs = HandlerT::timeoutReadMs;
			static constexpr long timeoutWriteMs = HandlerT::timeoutWriteMs;
			static constexpr long heartbeatPeriodMs = HandlerT::heartbeatPeriodMs;

		protected:
			bool m_stopped{ false };
			tcp::resolver::results_type m_endpoints;
			tcp::socket m_socket;
			
			data_packet_raw_t m_readBuf;

			asio::steady_timer m_readDeadline;
			asio::steady_timer m_writeDeadline;
			asio::steady_timer m_heartbeatTmr;
			
			::std::deque<data_packet_raw_t> m_writeQueue;

			HandlerT& m_handler;

			Srv2Cli_PacketHdr m_srvHdr;

			bool m_bWriteInProgress{ false };
			bool m_bReadInProgress{ false };

		public:
			client(HandlerT& h, asio::io_context& io_context) : m_socket(io_context), m_readDeadline(io_context)
				, m_writeDeadline(io_context), m_heartbeatTmr(io_context), m_handler(h)
			{
				_deactivateTimer(m_writeDeadline);
				_deactivateTimer(m_readDeadline);
				_deactivateTimer(m_heartbeatTmr);
			}

			// Called by the user of the client class to initiate the connection process.
			// The endpoints will have been obtained using a tcp::resolver.
			void start(tcp::resolver::results_type&& endpoints) {
				// Start the connect actor.
				m_endpoints = ::std::move(endpoints);

				// Start the deadline actor. You will note that we're not setting any
				// particular deadline here. Instead, the connect and input actors will
				// update the deadline prior to each asynchronous operation.
				_check_deadline(m_readDeadline, "Read");
				_check_deadline(m_writeDeadline, "Write");

				_start_connect(m_endpoints.begin());
			}

		protected:
			static void _deactivateTimer(asio::steady_timer& tmr)noexcept {
				tmr.expires_at(asio::steady_timer::time_point::max());
			}
			static bool _isTimerActive(const asio::steady_timer& tmr)noexcept {
				return tmr.expiry() < asio::steady_timer::time_point::max();
			}

			void _check_deadline(asio::steady_timer& tmr, const char*const dname) {
				tmr.async_wait([this, &tmr, dname](const ::boost::system::error_code& /*error*/) {
					if (UNLIKELY(m_stopped)) return;

					// Check whether the deadline has passed. We compare the deadline against
					// the current time since a new asynchronous operation may have moved the
					// deadline before this actor had a chance to run.
					if (UNLIKELY(tmr.expiry() <= asio::steady_timer::clock_type::now())) {
						m_handler.getLog().error("{} deadline has passed!", dname);
						// The deadline has passed. The socket is closed so that any outstanding
						// asynchronous operations are cancelled.
						//m_socket.close();
						stop();

						// There is no longer an active deadline. The expiry is set to the
						// maximum time point so that the actor takes no action until a new
						// deadline is set.
						//tmr.expires_at(asio::steady_timer::time_point::max());
					} else _check_deadline(tmr, dname);
				});
			}

			void _start_connect(tcp::resolver::results_type::iterator endpoint_iter) {
				if (endpoint_iter != m_endpoints.end()) {
					m_handler.getLog().debug("Trying endpoint {}...", endpoint_iter->endpoint() );

					// Set a deadline for the connect operation.
					m_readDeadline.expires_after(::std::chrono::milliseconds(timeoutConnectMs));

					// Start the asynchronous connect operation.
					m_socket.async_connect(endpoint_iter->endpoint()
						, [this, endpoint_iter](const ::boost::system::error_code& error)mutable
					{
						if (UNLIKELY(m_stopped)) return;

						//no need to do anything with m_readDeadline, b/c we going to call either _start_connect() or _read_responce()
						//later and they both resets the timer.

						// The async_connect() function automatically opens the socket at the start
						// of the asynchronous operation. If the socket is closed at this time then
						// the timeout handler must have run first.
						if (!m_socket.is_open()) {
							m_handler.getLog().error("Connect timed out");

							// Try the next available endpoint.
							_start_connect(++endpoint_iter);
						} else if (error) {// Check if the connect operation failed before the deadline expired.
							m_handler.getLog().error("Connection error: {}", error.message());

							// We need to close the socket used in the previous connection attempt before starting a new one.
							m_socket.close();
							// Try the next available endpoint.
							_start_connect(++endpoint_iter);
						} else {// Otherwise we have successfully established a connection.
							m_handler.getLog().info("Connected to {}", endpoint_iter->endpoint());
							m_handler.hndConnectionState(true);

							// Start the input actor.
							_read_responce();
							// Start the heartbeat actor.
							_reset_heartbeat();
							_check_heartbeat();
						}
					});
						//::std::bind(&client::_handle_connect, this, ::std::placeholders::_1, endpoint_iter));
				} else {
					m_handler.getLog().error("There are no more endpoints to try. Connection failed. Shutting down the client.");
					stop();
				}
			}

		public:
			// This function terminates all the actors to shut down the connection. It
			// may be called by the user of the client class, or by the class itself in
			// response to graceful termination or an unrecoverable error.
			void stop() {
				if (!m_stopped) {
					m_stopped = true;

					m_handler.getLog().info("Stopping the client");
					m_handler.hndConnectionState(false);

					::boost::system::error_code ignored_error;
					m_socket.close(ignored_error);
					m_readDeadline.cancel();
					m_writeDeadline.cancel();
					m_heartbeatTmr.cancel();
				}
			}
			
			void send_heartbeat() {
				// deactivating heartbeat timer (it'll be reactivated in _handle_write)
				_deactivateTimer(m_heartbeatTmr);
				// and sending a heartbeat

				//m_handler.getLog().trace("sending heartbeat...");

				m_writeQueue.emplace_back(Cli2Srv_PacketHdr_Size);
				auto& s = m_writeQueue.back();

				Cli2Srv_PacketHdr* pHdr = reinterpret_cast<Cli2Srv_PacketHdr*>(s.data());
				pHdr->bPacketType = static_cast<decltype(pHdr->bPacketType)>(ProtoCli2Srv::heartbeat);
				pHdr->wTotalPacketLen = Cli2Srv_PacketHdr_Size;

				_write_request();
			}

			void send_packet(const ProtoCli2Srv pid, const char*const str, const size_t slen) {
				const auto totalLen = Cli2Srv_PacketHdr_Size + slen;
				m_writeQueue.emplace_back(totalLen);
				auto& s = m_writeQueue.back();
				T18_ASSERT(s.size() >= totalLen);

				Cli2Srv_PacketHdr* pHdr = reinterpret_cast<Cli2Srv_PacketHdr*>(s.data());
				pHdr->bPacketType = static_cast<decltype(pHdr->bPacketType)>(pid);
				T18_ASSERT(::std::numeric_limits<decltype(pHdr->wTotalPacketLen)>::max() >= totalLen);
				pHdr->wTotalPacketLen = static_cast<decltype(pHdr->wTotalPacketLen)>(totalLen);

				if (str && slen > 0) {
					T18_ASSERT(!packetHaveOnlyHeader(pid));
					::std::memcpy(s.data() + Cli2Srv_PacketHdr_Size, str, slen);
				} else {
					T18_ASSERT(packetHaveOnlyHeader(pid));
					T18_ASSERT(slen == 0 && !str);
				}

				_write_request();
			}
			void send_packet(const ProtoCli2Srv pid, const ::std::string& str) {
				send_packet(pid, str.empty() ? nullptr : str.data(), str.empty() ? 0 : str.length());
			}
			/*void send_subscribeAllTrades(const ::std::string& str) {
				send_packet(ProtoCli2Srv::subscribeAllTrades, str);
			}*/

		protected:
			void _reset_heartbeat()noexcept {
				m_heartbeatTmr.expires_after(::std::chrono::milliseconds(heartbeatPeriodMs));
			}

			bool _skip_heartbeat()const noexcept {
				//we should NOT do the heartbeat if the writing timer is active (i.e. the write operation already in progress
				//and it's enough to test the connection), or if the output queue is not empty (i.e. there's
				// a pending write operation that will fire just soon)
				// we shouldn't check the reading timer, because it is always active listening for servers output
				return _isTimerActive(m_writeDeadline) || !m_writeQueue.empty();
			}

			void _check_heartbeat() {
				m_heartbeatTmr.async_wait([this](const ::boost::system::error_code& error) {
					if (UNLIKELY(m_stopped)) return;

					const bool bNotNow = static_cast<bool>(error) || m_heartbeatTmr.expiry() > asio::steady_timer::clock_type::now();
					if (bNotNow || _skip_heartbeat()) {
						if(!bNotNow) m_handler.getLog().debug("-- something in progress, skipping heartbeat");
						//an operation is in progress. No need to send a heartbeat. Just resetting
						if (error != boost::asio::error::operation_aborted)_reset_heartbeat();
					} else {
						// no operation in progress and too much time has passed.
						send_heartbeat();
					}
					//resuming the waiting
					_check_heartbeat();
				});
			}

			void _write_request() {
				//MUST do this check because communicating party may induce new write requests while some older
				//requests are in process
				if (m_bWriteInProgress || m_stopped) return;
				m_bWriteInProgress = true;

				T18_ASSERT(!m_writeQueue.empty());

				m_writeDeadline.expires_after(std::chrono::milliseconds(timeoutWriteMs));
				asio::async_write(m_socket, asio::buffer(m_writeQueue.front())
					, [this](const ::boost::system::error_code& error, ::std::size_t /*n*/)
				{
					if (UNLIKELY(m_stopped)) return;
					m_bWriteInProgress = false;
					if (UNLIKELY(error)) {
						m_handler.getLog().error("Error in handle_write: {}", error.message());
						stop();
					} else {
						m_writeQueue.pop_front();

						if (m_writeQueue.empty()) {
							_deactivateTimer(m_writeDeadline);
							_reset_heartbeat();
						} else _write_request();
					}
				});
			}

			decltype(auto) _read_hdr_buffer() noexcept {
				return asio::buffer(&m_srvHdr, Srv2Cli_PacketHdr_Size);
			}

			void _log_read_failed(const ::boost::system::error_code& error, const ::std::size_t n
				, const ::std::size_t expected, const char* const nme)const
			{
				//#log

				::std::string s;
				s.reserve(256);
				s += "read ";
				s += nme;
				s += " failed, reason: ";
				if (error) {
					s += error.message();
				} else {
					s += "incoherent read count (";
					s += ::std::to_string(n); s += " instead of "; s += ::std::to_string(expected);
					s += ")";
				}
				//m_handler.hndMessage(s);
				m_handler.getLog().critical(s);
			}

			void _read_responce() {
				T18_ASSERT(!m_bReadInProgress);
				m_bReadInProgress = true;

				// Set a deadline for the read operation.
				m_readDeadline.expires_after(std::chrono::milliseconds(timeoutReadMs));

				// Start an asynchronous operation to read a newline-delimited message.
				//#TODO IMPORTANT: NO other read operation MUST be issued before completion handler fires
				asio::async_read(m_socket, _read_hdr_buffer()
					, [this](const ::boost::system::error_code& error, const ::std::size_t n)
				{
					if (UNLIKELY(m_stopped)) return;
					
					if (UNLIKELY(error || (n != Srv2Cli_PacketHdr_Size))) {
						//#log
						_log_read_failed(error, n, Srv2Cli_PacketHdr_Size, "hdr");
						stop();
					} else {
						T18_ASSERT(m_srvHdr.wTotalPacketLen >= Srv2Cli_PacketHdr_Size);
						m_handler.hndQuikConnectionState(static_cast<bool>(m_srvHdr.bFlags & 0x80));

						const Srv2Cli_PacketHdr_flags_t packetFlags = (m_srvHdr.bFlags & 0x7f);
						const auto packetType = static_cast<ProtoSrv2Cli>(m_srvHdr.bPacketType);

						if (ProtoSrv2Cli::heartbeat == packetType) {
							T18_ASSERT(m_srvHdr.wTotalPacketLen == Srv2Cli_PacketHdr_Size || !"Invalid heartbeat packet size!");

							//resetting a heartbeat timer
							//_reset_heartbeat();//NO, we should NOT reset heartbeat or a server-side reading will timeout!
							//repeat wait for incoming data
							m_bReadInProgress = false;
							_read_responce();
						} else {
							// we must read the packet entirely and parse it
							m_readDeadline.expires_after(::std::chrono::milliseconds(timeoutReadMs));
							const auto expectedLen = (static_cast<::std::size_t>(m_srvHdr.wTotalPacketLen) - Srv2Cli_PacketHdr_Size);
							m_readBuf.clear();
							m_readBuf.reserve(expectedLen + 1);//we may need +1 to parse strings
							
							asio::async_read(m_socket, asio::dynamic_buffer(m_readBuf, expectedLen)
								, [this, expectedLen, packetType, packetFlags]
								(const ::boost::system::error_code& error2, const ::std::size_t n2)
							{
								// Check if the session was stopped while the operation was pending.
								if (UNLIKELY(m_stopped)) return;

								if (UNLIKELY(error2 || (n2 != expectedLen))) {
									_log_read_failed(error2, n2, expectedLen, ("body("s
										+ ::std::to_string(static_cast<int>(packetType)) + ")").c_str());
									stop();
								} else {
									//despite the recursive call to _read_responce() below, we'd better to deactivate readDeadline, because
									// it might take a lot of time for handler to process the request.
									_deactivateTimer(m_readDeadline);
									//resetting a heartbeat timer
									//_reset_heartbeat();//NO, we should NOT reset heartbeat or a server-side reading will timeout!
									m_bReadInProgress = false;

									_deliver(packetType, n2, packetFlags);

									//repeat wait for incoming data
									_read_responce();
								}
							});
						}
					}
				});
			}

			void _deliver(const ProtoSrv2Cli packetType, const size_t packetLen, Srv2Cli_PacketHdr_flags_t packetFlags) {
				T18_ASSERT(!packetHaveOnlyHeader(packetType));//leave here to ensure coherent handling

				const char*const pData = m_readBuf.data();

				switch (packetType) {
				case ProtoSrv2Cli::requestFailed:
					//#todo
					m_readBuf.emplace_back(0);
					m_handler.hndRequestFailed(pData);
					break;

				case ProtoSrv2Cli::getClassesListResult:
					//#todo
					m_readBuf.emplace_back(0);
					m_handler.getLog().info("getClassesList: {}", pData);
					break;

				case ProtoSrv2Cli::listAllTickersResult:
					//#todo
					m_readBuf.emplace_back(0);
					m_handler.getLog().info("listAllTickers: {}", pData);
					break;

				case ProtoSrv2Cli::queryTickerInfoResult:
				case ProtoSrv2Cli::subscribeAllTradesResult:
					m_readBuf.emplace_back(0);
					{
						const bool bAllTrades = (ProtoSrv2Cli::subscribeAllTradesResult == packetType);
						const char*const reqName = bAllTrades ? "subscribeAllTradesResult" : "queryTickerInfoResult";

						const char* pTickerName{nullptr};
						const char* pClassName;
						const prxyTickerInfo* pPTI;
						if (packetFlags) {
							pPTI = reinterpret_cast<const prxyTickerInfo*>(pData);
							pTickerName = pPTI->_ofsPastEOS();
							const auto l = ::std::strlen(pTickerName);
							if (LIKELY( l + prxyTickerInfo_Size == packetLen)) {
								//safe to const_cast here. We could either const_cast here, or cast to const while calling to
								// hndSubscribeAllTradesResult(). The first option seems more reasonable.
								pTickerName = _parseTickerClass(const_cast<char*>(pTickerName), l, &pClassName);
							}else{
								m_handler.getLog().error("Invalid {}(success) packet: len={}, while expected={}"
									, reqName, packetLen, l);
								packetFlags = 0;
							}
						} 
						if (!packetFlags) {
							pPTI = nullptr;
							const auto l = ::std::strlen(pData);
							if (LIKELY(l == packetLen)) {
								//safe to const_cast here. We could either const_cast here, or cast to const while calling to
								// hndSubscribeAllTradesResult(). The first option seems more reasonable.
								pTickerName = _parseTickerClass(const_cast<char*>(pData), l, &pClassName);
							} else {
								m_handler.getLog().error("Invalid {}(failure) packet: len={}, while expected={}"
									, reqName, packetLen, l);
							}
						}
						if (LIKELY(pTickerName)) {
							/*if (bAllTrades) {
								m_handler.hndSubscribeAllTradesResult(pPTI, pTickerName, pClassName);
							}else m_handler.hndQueryTickerInfoResult(pPTI, pTickerName, pClassName);*/
							typedef void(HandlerT::*pF_t)(const ::t18::proxy::prxyTickerInfo*, const char*, const char*);
							pF_t fPtr{ bAllTrades ? &HandlerT::hndSubscribeAllTradesResult : &HandlerT::hndQueryTickerInfoResult };
							(m_handler.*fPtr)(pPTI, pTickerName, pClassName);
						} else {
							m_handler.getLog().error("Invalid {} packet, failed to parse ticker@class", reqName);
						}
					}
					break;

				case ProtoSrv2Cli::AllTrades:
					{
						const size_t cnt = packetLen / prxyTsDeal_Size;
						if (LIKELY((prxyTsDeal_Size* cnt) == packetLen)) {
							m_handler.hndAllTrades(reinterpret_cast<const prxyTsDeal*>(pData), cnt);
						} else {
							//#log
							m_handler.getLog().error("Invalid AllTrades packet: len={}, while prxyTsDeal_Size={}"
								, packetLen, prxyTsDeal_Size);
						}
					}
					break;

					//////////////////////////////////////////////////////////////////////////
					//////////////////////////////////////////////////////////////////////////
				case ProtoSrv2Cli::heartbeat:
					T18_ASSERT(!"Must NEVER be here!");
					throw ::std::logic_error("Invalid packet type for _deliver");

				/*default:
					m_handler.getLog().warn("Can't handle packet of type={}, len={}", static_cast<char>(packetType), packetLen);
					break;*/
				}
			}

			static const char* _parseTickerClass(char* pBeg, const size_t maxLen, const char** ppClass)noexcept {
				T18_ASSERT(pBeg && maxLen);
				const auto pEnd = pBeg + maxLen;
				char* pCls = ::std::strchr(pBeg, '@');
				if (!pCls || pCls >= pEnd) return nullptr;

				*pCls = 0;
				++pCls;
				if (pCls >= pEnd || (pCls + ::std::strlen(pCls)) > pEnd)return nullptr;
				*ppClass = pCls;
				return pBeg;
			}
		};
		
		//////////////////////////////////////////////////////////////////////////

		template<typename HandlerT>
		class QCli {
		protected:
			typedef client<HandlerT> client_t;

		protected:
			asio::io_context m_io_context;
			client_t m_cli;

		public:
			~QCli() { stop(); }

			QCli(HandlerT& h, const char* szHostIP, ::std::uint16_t uiPort) : m_cli(h, m_io_context) {
				tcp::resolver r(m_io_context);
				char buf[32];
				sprintf_s(buf, "%u", static_cast<unsigned>(uiPort));
				m_cli.start(r.resolve(szHostIP, buf));
			}

			bool stopped()const { return m_io_context.stopped(); }
			
			void stop() {
				if (!stopped()) {
					m_io_context.stop();
				}
				m_cli.stop();
			}

			auto* getCtx()noexcept { return &m_io_context; }

			template<typename Rep, typename Period>
			::std::size_t run_for(const ::std::chrono::duration< Rep, Period > & rel_time) {
				return m_io_context.run_for(rel_time);
			}

			void run() { m_io_context.run(); }

			void run_from_stop() {
				m_io_context.restart();
				m_io_context.run();
			}
			
			//DON'T pass local variables here, as they will be destroyed to the moment real handler will run
			/*void post_packet(const ProtoCli2Srv pid, const char*const str, const size_t slen) {
				asio::post(m_io_context, [this, pid, str, slen]() {
					m_cli.send_packet(pid, str, slen);
				});
			}
			void post_packet(const ProtoCli2Srv pid, const ::std::string& str) {
				post_packet(pid, str.data(), str.length());
			}
			void post_packet(const ProtoCli2Srv pid, const char*const str) {
				post_packet(pid, str, str ? ::std::strlen(str) : 0);
			}*/
			void post_packet(const ProtoCli2Srv pid, ::std::string&& str) {
				asio::post(m_io_context, [this, pid, s = ::std::move(str)]() {
					m_cli.send_packet(pid, s);
				});
			}
			/*void post_packet(const ProtoCli2Srv pid, const ::std::string& str) {
				post_packet(pid, str.data(), str.length());
			}*/
			void post_packet(const ProtoCli2Srv pid, const char*const str) {
				post_packet(pid, str ? ::std::string(str) : ::std::string());
			}

			template<size_t dl>
			static int makeAllTradesRequest(OUT char(&pReq)[dl], const char*const pTicker, const char*const pClass, mxTimestamp lastDeal) {
				T18_ASSERT(pTicker && pClass && !lastDeal.empty());
				return sprintf_s(pReq, "%s(%s|%I64x)", pClass, pTicker, lastDeal._get());
			}

			// the required buffer len for AllTradesRequest minus the length of strings with ticker&class
			// 3 delimiters (|)
			// 16 chars for i64 in hex form
			// and 1 for null termination
			static constexpr int sAllTradesRequest_addedBufLen = 3 + 8 * 2 + 1;

		};

		template<typename HandlerT>
		class QCliWThread : public QCli<HandlerT> {
			typedef QCli<HandlerT> base_class_t;

		protected:
			::std::thread m_workerThread;

		public:
			~QCliWThread() { stop(); }
			QCliWThread(HandlerT& h, const char* szHostIP, ::std::uint16_t uiPort)
				: base_class_t(h,szHostIP, uiPort), m_workerThread([this]() { base_class_t::run(); })
			{}

		protected:
			using base_class_t::run;
			using base_class_t::run_from_stop;
			using base_class_t::run_for;
			//should you cast this to base_class_t and then call run*() on it - you would better know what you are doing

		public:
			void stop() {
				base_class_t::stop();
				if (m_workerThread.joinable()) m_workerThread.join();
			}

		};
	}
}

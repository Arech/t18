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

#include <atomic>
#include <thread>

//btw, there is a #include <boost/smart_ptr/detail/spinlock.hpp> which offer spinlock implementation, however
//it may call sleep/yield even during lock() phase, but it may be undesireable in some cases

namespace utils {

	struct spinlock {
	protected:
		::std::atomic_flag m_lock= ATOMIC_FLAG_INIT;

	public:
		spinlock()noexcept {}

		spinlock(const spinlock&) = delete;
		spinlock& operator=(const spinlock&) = delete;

		//#consider not sure we need atomic_thread_fence() there

		void lock()noexcept {
			while (m_lock.test_and_set(::std::memory_order_acquire)) {
				//asm("pause");
				//the funny thing is despite recommendation https://stackoverflow.com/questions/26583433/c11-implementation-of-spinlock-using-atomic#comment47890032_26583433
				// using the pause instruction slows down the tight spinlock usage cycle 
			}
			//::std::atomic_thread_fence(::std::memory_order_acquire);
		}
		bool try_lock() noexcept {
			return !m_lock.test_and_set(::std::memory_order_acquire);
		}

		void lock_yield()noexcept {
			while (m_lock.test_and_set(::std::memory_order_acquire)) {
				::std::this_thread::yield();
			}
			//::std::atomic_thread_fence(::std::memory_order_acquire);
		}
		void unlock()noexcept {
			//::std::atomic_thread_fence(::std::memory_order_release);
			m_lock.clear(::std::memory_order_release);
		}
	};

	struct spinlock_guard {
	protected:
		spinlock& m_l;

	public:
		spinlock_guard(spinlock& l)noexcept : m_l(l){
			l.lock();
		}

		spinlock_guard(const spinlock_guard&) = delete;
		spinlock_guard& operator=(const spinlock_guard&) = delete;

		~spinlock_guard()noexcept {
			m_l.unlock();
		}
	};

	struct spinlock_guard_ex {
	protected:
		spinlock& m_l;
		bool m_locked;

	public:
		spinlock_guard_ex(spinlock& l)noexcept : m_l(l), m_locked(true) {
			l.lock();
		}

		spinlock_guard_ex(const spinlock_guard_ex&) = delete;
		spinlock_guard_ex& operator=(const spinlock_guard_ex&) = delete;

		~spinlock_guard_ex()noexcept {
			unlock();
		}

		void lock()noexcept {
			if (!m_locked) {
				m_locked = true;
				m_l.lock();
			}
		}
		void unlock()noexcept {
			if (m_locked) {
				m_l.unlock();
				m_locked = false;
			}
		}
	};


	/*
	 *#include "../t18/utils/spinlock.h"

volatile int value = 0;
utils::spinlock l;

int loop(bool inc, int limit) {
	std::cout << "Started " << inc << " " << limit << std::endl;
	for (int i = 0; i < limit; ++i) {
		utils::spinlock_guard guard(l);
		if (inc) {
			++value;
		} else {
			--value;
		}
	}
	return 0;
}


TEST(TestBasic, sl) {
	auto f = std::async(std::launch::async, std::bind(loop, true, 20000000));
	loop(false, 10000000);
	f.wait();
	std::cout << value << std::endl;
}
	 **/
}
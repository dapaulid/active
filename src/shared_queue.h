//------------------------------------------------------------------------------
/**
 * @license
 * Copyright (c) Daniel Pauli <dapaulid@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
//------------------------------------------------------------------------------
#pragma once

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
//
#include <queue>
#include <mutex>
#include <condition_variable>

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
template<typename T>
class shared_queue {

// public member functions
public:
	//--------------------------------------------------------------------------
	//
	void put(T&& a_item) {
		// critical section
		std::unique_lock<std::mutex> lock(m_mutex);
		// add item to queue
		m_items.push(std::move(a_item));
		// end critical section
		lock.unlock();
		// wakeup waiting thread
		m_cond.notify_one();
	}

	//--------------------------------------------------------------------------
	//
	T get() {
		// critical section
		std::unique_lock<std::mutex> lock(m_mutex);
		// wait until not empty
		// lock is atomically released while doing so
		while (m_items.empty()) {
			m_cond.wait(lock);
		}
		// take item from queue
		T item = std::move(m_items.front());
		m_items.pop();
		return item;
	}	

// protected members
protected:
	std::queue<T> m_items;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

//------------------------------------------------------------------------------
// end of file

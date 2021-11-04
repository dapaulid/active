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
#include <type_traits>
#include <utility>

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class call_queue {
// public member functions
 public:
	//--------------------------------------------------------------------------
	//
	template<typename T>
	void put(T/*&&*/ a_item) {
		void* addr = (unsigned char*)&m_data + m_uWrite;
		::new (addr) T(/*std::forward<T>*/(a_item));
		m_uWrite += sizeof(T);
	}

	//--------------------------------------------------------------------------
	//
	template<typename T>
	T get() {
		T* ptr = reinterpret_cast<T*>((unsigned char*)&m_data + m_uRead);
		ptr->~T();
		T item = *ptr;
		m_uRead += sizeof(T);
		return item; // std::move(*ptr);
	}

// protected member functions
 protected:
	void* alloc() {
		return &m_data;
	}

// private members
 private:
	typename std::aligned_storage<16>::type m_data;
	size_t m_uRead = 0;
	size_t m_uWrite = 0;
};

//------------------------------------------------------------------------------
// end of file

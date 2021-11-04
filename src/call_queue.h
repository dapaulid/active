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
#include <memory>
#include <tuple>
#include <assert.h>

#include <iostream>

// NOTE: the "0," avoids the "ISO C++ forbids zero-size array" pendantic warning
#define REMO_FOREACH_ARG(args, what) { int dummy[] = { 0,(what(args),0)... }; (void)dummy; }

class call_queue;
//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class BaseFunc {
 public:
	explicit BaseFunc(call_queue* a_queue): m_queue(a_queue) {}
	virtual ~BaseFunc() {}

	virtual void dequeue() = 0;
 protected:
	call_queue* m_queue = nullptr;
};

// fire-and-forget style function (no return value, no wait)
template< class Object, class... Args >
class EventFunc: public BaseFunc {
 public:
	using inner_func = void (Object::*)(Args...);
	using ArgsTuple = std::tuple<Args...>;

	EventFunc(call_queue* a_queue, Object* a_obj, inner_func a_func):
		BaseFunc(a_queue), m_obj(a_obj), m_func(a_func) {}

	void operator()(Args... args) {
		// TODO vs std::make_tuple, std::tie, std::forward_as_tuple?
		m_queue->enqueue(this, ArgsTuple(args...));
	}

	void dequeue() override {
		ArgsTuple args = m_queue->get<ArgsTuple>();
		// std::apply needs instance as first argument, prepend it
		auto full_args = std::tuple_cat(std::make_tuple(m_obj), args);
		std::apply(m_func, full_args);
		
	}
 private:
	Object* m_obj;
	inner_func m_func;
};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class call_queue {
// public member functions
 public:
	// a_capacity must be a power of 2
	explicit call_queue(size_t a_capacity = 256) {
		m_capacity = a_capacity;
		m_mod = a_capacity - 1;
		m_data = new unsigned char[m_capacity];
		m_uSpace = m_capacity;
	}

	~call_queue() {
		delete [] m_data;
	}

	template<class ArgsTuple>
	void enqueue(BaseFunc* a_func, const ArgsTuple& a_args) {
		put(a_func);
		put(a_args);
	}

	void process() {
		BaseFunc* func = get<BaseFunc*>();
		func->dequeue();
	}

	//--------------------------------------------------------------------------
	//
	template<typename T>
	bool put(const T& a_item) {
		void* addr = advance(sizeof(T), alignof(T));
		if (addr) {
			// placement new: copy-construct value onto buffer memory
			::new (addr) T(a_item);
			// success
			return true;
		} else {
			// TODO optional blocking
			return false;
		}
	}

	//--------------------------------------------------------------------------
	//
	template<typename T>
	T get() {
		void* addr = retreat(sizeof(T), alignof(T));
		// TODO blocking/exception
		assert(addr);
		// copy item from queue
		T* ptr = reinterpret_cast<T*>(addr);
		T item(*ptr);
		// "placement delete"
		ptr->~T();
		// NOTE: For MSVC (cl.exe) 19.16.27041, this needs optimization /02
		// to perform copy elision / return value optimization
		// BUT ONLY if there's no second return (?)
		return item;
	}

// protected member functions
 protected:
	void* advance(size_t a_size, size_t a_alignment) {
		// get current write pointer
		void* ptr = &m_data[m_uWrite];
		// align it
		if (std::align(a_alignment, a_size, ptr, m_uSpace)) {
			// increase write offset (modulo buffer size)
			m_uWrite = (m_uWrite + a_size) & m_mod;
			// decrease free buffer size
			m_uSpace -= a_size;
			std::cout << "advance by " << a_size << std::endl;
			// return pointer to allocated memory
			return ptr;
		} else {
			// not enough space
			return nullptr;
		}
	}

	void* retreat(size_t a_size, size_t a_alignment) {
		// get current read pointer
		void* ptr = &m_data[m_uRead];
		// align it
		if (std::align(a_alignment, a_size, ptr, m_uSpace)) {
			// increase read offset (modulo buffer size)
			m_uRead = (m_uRead + a_size) & m_mod;
			// increase free buffer size
			m_uSpace += a_size;
			std::cout << "retreat by " << a_size << std::endl;
			// return pointer to allocated memory
			return ptr;
		} else {
			// not enough data
			return nullptr;
		}
	}


// private members
 private:
	unsigned char* m_data = nullptr;
	size_t m_capacity = 0;
	size_t m_uRead = 0;
	size_t m_uWrite = 0;
	size_t m_uSpace = 0;
	size_t m_mod = 0;
};

//------------------------------------------------------------------------------
// end of file

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
// project
#include "shared_queue.h"
//
// C++ 
#include <functional>
#include <future>

//------------------------------------------------------------------------------
//
namespace active {

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class command {
public:
	virtual ~command() {}
	virtual void execute() = 0;
};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class object {
public:
	void action() {
		// TODO use unique ptr?
		command* cmd = m_queue.get();
		cmd->execute();
	}
	void enqueue(command* a_cmd) {
		m_queue.put(std::move(a_cmd));
	}
public:
	shared_queue<command*> m_queue;
};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
template< class Object, class Ret, class... Args >
class outer_func {
public:

	using inner_func = Ret (Object::*)(Args...);

	outer_func(Object* a_obj, inner_func a_func) {
		m_obj = a_obj;
		m_func = a_func;
	}

	Ret operator()(Args... args) {
		return call_async(...args).get();
	}

	std::future<Ret> call_async(Args... args) {
		call_command* call = new call_command();
		call->func = std::bind(m_func, m_obj, args...);
		std::future<Ret> res = call->promise.get_future();
		m_obj->enqueue(std::move(call));
		return res;
	}

protected:
	// helper class for pending call
	class call_command: public command {
	public:
		virtual void execute() override {
			promise.set_value(func());
			delete this;
		}
		std::function<Ret()> func;
		std::promise<Ret> promise;
	};

protected:
	Object* m_obj;
	inner_func m_func;

};


// helper class for pending call
template<class Ret>
class deferred_call: public command {
public:
	virtual void execute() override {
		func();
	}
	void complete(Ret&& result) {
		promise.set_value(result);
		delete this;
	}
	std::function<void()> func;
	std::promise<Ret> promise;
};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
template< class Object, class Ret, class... Args >
class defer_func {
public:

	using inner_func = void (Object::*)(deferred_call<Ret>*, Args...);

	defer_func(Object* a_obj, inner_func a_func) {
		m_obj = a_obj;
		m_func = a_func;
	}

	Ret operator()(Args... args) {
		return call_async(...args).get();
	}

	std::future<Ret> call_async(Args... args) {
		deferred_call<Ret>* call = new deferred_call<Ret>();
		call->func = std::bind(m_func, m_obj, call, args...);
		std::future<Ret> res = call->promise.get_future();
		m_obj->enqueue(std::move(call));
		return res;
	}


protected:
	Object* m_obj;
	inner_func m_func;

};

//------------------------------------------------------------------------------
//
} // end namespace active

//------------------------------------------------------------------------------
// end of file

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
#include <thread>
#include <functional>
#include <future>

//------------------------------------------------------------------------------
//
namespace active {

template<typename F, typename R>
void set_promise(std::promise<R> & p, F && f) //handle non-void here
{
    p.set_value(f()); 
}

template<typename F>
void set_promise(std::promise<void> & p, F && f)  //handle void here
{
    f();
    p.set_value(); 
}

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

	// TODO perfect forwarding??? issues with string conversion
	Ret operator()(Args... args) {
		return call_async(std::forward<Args>(args)...).get();
	}

	// TODO perfect forwarding??? issues with string conversion
	std::future<Ret> call_async(Args... args) {
		// create pending call 
		call_command* call = new call_command(
			std::bind(m_func, m_obj, std::forward<Args>(args)...)
		);
		// get future so that handler can wait for completion
		std::future<Ret> res = call->get_future();
		// add it to our queue for execution
		m_obj->enqueue(call);
		// done
		return res;
	}

protected:
	// helper class for pending call
	class call_command: public command {
	public:
		call_command(std::function<Ret()>&& a_func):
			m_func(a_func) {}
		virtual void execute() override {
			set_promise(m_promise, m_func);
			delete this;
		}
		std::future<Ret> get_future() {
			return m_promise.get_future();
		}
	private:
		std::function<Ret()> m_func;
		std::promise<Ret> m_promise;
	};

protected:
	Object* m_obj;
	inner_func m_func;

};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class command {
public:
	virtual ~command() {}
	virtual void execute() = 0;
};

#include <iostream>

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
class object {
protected:
	class finalizer;
public:
	
	object(finalizer& a_finalizer = finalizer()) {
		std::cout << "hello from object" << std::endl;
		// this will call actually run the thread as soon as the
		// constructor completes
		a_finalizer.set_object(this);
	}
	~object() {
		shutdown();
		m_thread.join();
	}

	void enqueue(command* a_cmd) {
		m_queue.put(std::move(a_cmd));
	}

protected:
	virtual void i_startup() {
		m_is_active = true;
	}

	virtual void i_shutdown() {
		m_is_active = false;
	}

protected:

	class finalizer {
	public:
		~finalizer() {
			std::cout << "hello from finalizer" << std::endl;
			m_obj->run();
		}
		void set_object(object* a_obj) {
			m_obj = a_obj;
		}
	private:
		object* m_obj = nullptr;
	};

private:
	void action() {
		do {
			command* cmd = m_queue.get();
			cmd->execute();
		} while (m_is_active);
	}

	void run() {
		m_thread = std::thread(&object::action, this);
		startup();
	}

	outer_func<object, void> startup { this, &object::i_startup };
	outer_func<object, void> shutdown { this, &object::i_shutdown };

protected:
	shared_queue<command*> m_queue;
	std::thread m_thread;
	bool m_is_active = false;
};


//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
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

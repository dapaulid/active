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

/**
 * This code requires C++17 for the following reasons:
 * 
 * - use of std::apply()
 * 
 */

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

template<class... Args>
struct pack {
};

template<class Object, class Ret, class... Args>
Object get_object(Ret (Object::*)(Args...));

template<class Object, class Ret, class... Args>
Ret get_ret(Ret (Object::*)(Args...));

template<class Object, class Ret, class... Args>
pack<Args...> get_args(Ret (Object::*)(Args...));

//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------
//
class object;

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
template<class F>
class callback {
public:
	callback(object* a_obj, F& a_func):
		m_obj(a_obj), m_func(a_func) {}

	template<class Ret>
	void operator()(Ret result) {
		m_func(result);
	}

	object* get_obj() { return m_obj; }
private:
	object* m_obj;
	F m_func;
};

//------------------------------------------------------------------------------
// class definition
//------------------------------------------------------------------------------
//
template< class Object, class Ret, class... Args >
class outer_func {
public:

	//using Object = decltype(get_object(inner_func()));
	//using Ret = decltype(get_ret(inner_func()));
	//using ArgsPack = decltype(get_args(inner_func()));

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
			m_func, std::make_tuple(m_obj, args...)
		);
		// get future so that handler can wait for completion
		std::future<Ret> res = call->get_future();
		// add it to our queue for execution
		m_obj->enqueue(call);
		// done
		return res;
	}

	// TODO perfect forwarding??? issues with string conversion
	template<typename F>
	void call_async(Args... args, callback<F> a_callback) {
		// create pending call 
		command* call = new callback_command<F>(
			m_func, std::make_tuple(m_obj, args...), a_callback
		);
		// add it to our queue for execution
		m_obj->enqueue(call);
	}	

protected:
	// helper class for pending call
	class call_command: public command {
	public:
		call_command(inner_func a_func, std::tuple<Object*, Args...> a_args):
			m_func(a_func), m_args(a_args) {}
		virtual void execute() override {
			i_execute(m_promise);
			delete this;
		}
		std::future<Ret> get_future() {
			return m_promise.get_future();
		}
	private:
		// helpers to differentiate between void and non-void promises
		template<typename non_void>
		void i_execute(std::promise<non_void>&) {
			Ret ret = std::apply(m_func, m_args);
			m_promise.set_value(ret);
		}
		void i_execute(std::promise<void>&) {
			std::apply(m_func, m_args);
			m_promise.set_value();
		}		
	private:
		inner_func m_func;	
		std::tuple<Object*, Args...> m_args;
		std::promise<Ret> m_promise;
	};

	// helper class for pending call that expects a callback
	template<typename F>
	class callback_command: public command {
	public:
		callback_command(inner_func a_func, std::tuple<Object*, Args...> a_args, callback<F> a_callback):
			m_func(a_func), m_args(a_args), m_callback(a_callback) {}
		virtual void execute() override {
			Ret ret = std::apply(m_func, m_args);
			// return to sender
			// TODO make sure object still exists!
			m_callback.get_obj()->enqueue(new reply_command<F>(ret, m_callback));
			delete this;
		}

	private:
		inner_func m_func;	
		std::tuple<Object*, Args...> m_args;
		callback<F> m_callback;
	};

	// helper class for invoking a callback on the originating object
	template<typename F>
	class reply_command: public command {
	public:
		reply_command(Ret a_result, callback<F> a_callback):
			m_result(a_result), m_callback(a_callback) {}
		virtual void execute() override {
			m_callback(m_result);
			delete this;
		}

	private:
		Ret m_result;	
		callback<F> m_callback;
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

	template<class F>
	callback<F> cb(F& a_func) {
		return callback<F>(this, a_func);
	}

protected:

	class finalizer {
	public:
		~finalizer() {
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
		startup(); // TODO call non-blocking?
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

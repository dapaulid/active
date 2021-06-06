#include <functional>
#include <future>

#include "queue.h"

class Command {
public:
	virtual ~Command() {}
	virtual void execute() = 0;
};

class ActiveObject {
public:
	void action() {
		// TODO use unique ptr?
		Command* cmd = m_queue.get();
		cmd->execute();
	}
	void enqueue(Command* a_cmd) {
		m_queue.put(std::move(a_cmd));
	}
public:
	Queue<Command*> m_queue;
};


template< class Object, class Ret, class... Args >
class OuterFunc {
public:

	using InnerFunc = Ret (Object::*)(Args...);

	OuterFunc(Object* a_obj, InnerFunc a_func) {
		m_obj = a_obj;
		m_func = a_func;
	}

	Ret operator()(Args... args) {
		return call_async(...args).get();
	}

	std::future<Ret> call_async(Args... args) {
		Call* call = new Call();
		call->func = std::bind(m_func, m_obj, args...);
		std::future<Ret> res = call->promise.get_future();
		m_obj->enqueue(std::move(call));
		return res;
	}

protected:
	// helper class for pending call
	class Call: public Command {
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
	InnerFunc m_func;

};


// helper class for pending call
template<class Ret>
class DeferredCall: public Command {
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

template< class Object, class Ret, class... Args >
class DeferFunc {
public:

	using InnerFunc = void (Object::*)(DeferredCall<Ret>*, Args...);

	DeferFunc(Object* a_obj, InnerFunc a_func) {
		m_obj = a_obj;
		m_func = a_func;
	}

	Ret operator()(Args... args) {
		return call_async(...args).get();
	}

	std::future<Ret> call_async(Args... args) {
		DeferredCall<Ret>* call = new DeferredCall<Ret>();
		call->func = std::bind(m_func, m_obj, call, args...);
		std::future<Ret> res = call->promise.get_future();
		m_obj->enqueue(std::move(call));
		return res;
	}


protected:
	Object* m_obj;
	InnerFunc m_func;

};
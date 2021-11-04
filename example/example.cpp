#include <iostream>
#include <sstream>

#include "../src/active.h"
#include "../src/call_queue.h"

// helper class to synchronize output to stdout
struct sync_cout: public std::ostringstream {
	sync_cout() {
		// start message with thread id
		*this << "[T:" << std::this_thread::get_id() << "] ";
	}
    ~sync_cout() { 
		// output buffered message
		std::cout << str(); 
		std::cout.flush();
	}
};

class MyActiveObject: public active::object {
public:
	MyActiveObject(finalizer& a_finalizer = finalizer())
		: active::object(a_finalizer)
	{
		sync_cout() << "hello from MyActiveObject" << std::endl;
	}

// outer functions (called by other threads)
public:
	active::outer_func<MyActiveObject, int, int, const char*> fritzli{this, &MyActiveObject::i_fritzli};
	active::defer_func<MyActiveObject, int, int, const char*> hansli{this, &MyActiveObject::i_hansli};

// inner functions (called by our own thread)
protected:
	int i_fritzli(int a, const char* b)
	{
		sync_cout() << "fritzli called with " << a << ", " << b << std::endl;
		return a+1;
	}

	void i_hansli(active::deferred_call<int>* call, int a, const char* b)
	{
		sync_cout() << "hansli called with " << a << ", " << b << std::endl;
		// no need to provide result upon return, we could pass
		// the call object along and completing it at some later point
		call->complete(777);
	}

};

class Main: public active::object {
	using super = active::object;

public:
	Main(finalizer& a_finalizer = finalizer())
		: super(a_finalizer)
	{
		sync_cout() << "hello from Main" << std::endl;
	}
	virtual ~Main() {
		shutdown();
	}

protected:
	virtual void i_startup() override {

		// call base
		super::i_startup();

		// create object with thread that handles our calls
		MyActiveObject obj;

		// synchronous call
		int res = obj.fritzli(111, "hello world");
		sync_cout() << "fritzli returned " << res << std::endl;

		// asynchronous calls using futures
		std::future<int> fritzli_res = obj.fritzli.call_async(222, "gugus");
		std::future<int> hansli_res = obj.hansli.call_async(333, "dada");
		sync_cout() << "fritzli result: " << fritzli_res.get() << std::endl;
		sync_cout() << "hansli result: " << hansli_res.get() << std::endl;

		// asynchronous call using callback
		int local_var = 101;
		obj.fritzli.call_async(500, "gugus2", cb([local_var](int result){
			sync_cout() << "local_foo: " << local_var << std::endl;
			sync_cout() << "fritzli2 result: " << result << std::endl;
		}));

		sync_cout() << "main done " << std::endl;
	}
};

struct Hansli {
	Hansli() {
		ctor_cnt++;
	}
	Hansli(const Hansli& other) {
		ctor_cnt++;
	}

	~Hansli() {
		dtor_cnt++;
	}
	static size_t ctor_cnt;
	static size_t dtor_cnt;
};
size_t Hansli::ctor_cnt = 0;
size_t Hansli::dtor_cnt = 0;


int main() {
	
	//Main main;
	call_queue q;
	int foo = 1234;
	q.put(foo);
	q.put((double)0.5);
	q.put(Hansli());
	sync_cout() << q.get<int>() << ' ' << q.get<double>() << std::endl;

	q.get<Hansli>();
	sync_cout() << Hansli::ctor_cnt << ' ' << Hansli::dtor_cnt << std::endl;


	return 0;
}

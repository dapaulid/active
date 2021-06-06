#include <iostream>
#include <sstream>

#include "../src/active.h"

// helper class to synchronize output to stdout
struct sync_cout: public std::ostringstream {
    ~sync_cout() { std::cout << str(); }
};

class MyActiveObject: public active::object {

// outer functions (called by other threads)
public:
	active::outer_func<MyActiveObject, int, int, const char*> fritzli{this, &MyActiveObject::i_fritzli};
	active::defer_func<MyActiveObject, int, int, const char*> hansli{this, &MyActiveObject::i_hansli};

// inner functions (called by our own thread)
protected:
	int i_fritzli(int a, const char* b)
	{
		sync_cout() << "fritzli called with " << a << ", " << b << std::endl;
		return 666;
	}

	void i_hansli(active::deferred_call<int>* call, int a, const char* b)
	{
		sync_cout() << "hansli called with " << a << ", " << b << std::endl;
		// no need to provide result upon return, we could pass
		// the call object along and completing it at some later point
		call->complete(777);
	}

};

int main() {
	
	MyActiveObject obj;
	std::stringstream msg;

	// synchronous call
	int res = obj.fritzli(111, "hello world");
	sync_cout() << "fritzli returned " << res << std::endl;

	// asynchronous calls
	std::future<int> fritzli_res = obj.fritzli.call_async(222, "gugus");
	std::future<int> hansli_res = obj.hansli.call_async(333, "dada");
	sync_cout() << "fritzli result: " << fritzli_res.get() << std::endl;
	sync_cout() << "hansli result: " << hansli_res.get() << std::endl;
	return 0;
}

#include <iostream>

#include "../src/active.h"


class MyActiveObject: public active::object {

public:
	active::outer_func<MyActiveObject, int, int, const char*> fritzli{this, &MyActiveObject::i_fritzli};

	active::defer_func<MyActiveObject, int, int, const char*> hansli{this, &MyActiveObject::i_hansli};

protected:
	int i_fritzli(int a, const char* b)
	{
		std::cout << "fritzli called with " << a << ", " << b << std::endl;
		return 666;
	}

	void i_hansli(active::deferred_call<int>* call, int a, const char* b)
	{
		std::cout << "hansli called with " << a << ", " << b << std::endl;
		call->complete(777);
	}

};

int main() {
	
	MyActiveObject obj;
	auto result = obj.fritzli.call_async(222, "gugus");
	obj.action();
	std::cout << "result " << result.get() << std::endl;

	auto result2 = obj.hansli.call_async(333, "dada");
	obj.action();
	std::cout << "result " << result2.get() << std::endl;
	return 0;
}

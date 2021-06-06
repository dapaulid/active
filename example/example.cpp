#include <iostream>

#include "../src/active.h"


class MyActiveObject: public ActiveObject {

public:
	OuterFunc<MyActiveObject, int, int, const char*> fritzli{this, &MyActiveObject::i_fritzli};

	DeferFunc<MyActiveObject, int, int, const char*> hansli{this, &MyActiveObject::i_hansli};

protected:
	int i_fritzli(int a, const char* b)
	{
		printf("fritzli called with %d, %s\n", a, b);
		return 666;
	}

	void i_hansli(DeferredCall<int>* call, int a, const char* b)
	{
		printf("hansli called with %d, %s\n", a, b);
		//printf("result: %d\n", result);
		call->complete(777);
	}

};

int main() {
	
	MyActiveObject obj;
	auto result = obj.fritzli.call_async(222, "gugus");
	obj.action();
	printf("result: %d\n", result.get());

	auto result2 = obj.hansli.call_async(333, "dada");
	obj.action();
	printf("result: %d\n", result2.get());	
	return 0;
}

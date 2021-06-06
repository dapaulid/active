#include <vector>

template<typename T>
class Queue {
public:
	void put(T&& a_item) {
		// should do an "emplace"
		// see https://stackoverflow.com/questions/4303513/push-back-vs-emplace-back
		m_items.push_back(std::move(a_item));
	}
	T get() {
		T item = std::move(m_items[0]);
		m_items.pop_back();
		return item;
	}	
protected:
	std::vector<T> m_items;
};

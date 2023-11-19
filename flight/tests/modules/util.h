#ifndef _FLIGHT_TESTS_MODULES_UTIL
#define _FLIGHT_TESTS_MODULES_UTIL


#include <cstdlib>
namespace sty {
namespace flight {
namespace tests {

template<typename T>
class MockObjIter { // TODO: Implement An Iter Framework
public:
	MockObjIter(size_t freq,
				size_t cap,
				const std::function<T(void)>& obj_generator): 
		_freq(freq),
		_cap(cap),
		_size(0),
		_obj_generator(obj_generator) {}
	virtual ~MockObjIter() {}
	size_t size() {
		return _size;
	}
	bool has_next() {
	    bool ret = _size - _cap;
		if (!ret) {
		    return false;
		}
		_size++;
		return true;
	}
    const T& next() {
	    return _obj_generator();
	}
	bool reset() {
	    _size = 0;
		return true;
	}
private:
	size_t _freq;
	size_t _cap;
	size_t _size = 0;
	std::function<T(void)> _obj_generator;
}; // MockObjIter

// Generators
MagSensor MagSensorGen(void);

} // tests 
} // flight
} // sty

#endif // _FLIGHT_TESTS_MODULES_UTIL

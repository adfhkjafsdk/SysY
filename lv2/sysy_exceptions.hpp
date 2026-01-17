#ifndef _SYSY_EXCEPTION_HPP_
#define _SYSY_EXCEPTION_HPP_

#include <exception>
#include <stdexcept>

class sysy_error: public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};
class syntax_error: public sysy_error {
public:
	using sysy_error::sysy_error;
};

#endif
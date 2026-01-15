#ifndef _SYSY_DEBUG_HPP_
#define _SYSY_DEBUG_HPP_

struct NoErr {
	template <typename T>
	const NoErr &operator<< (const T &) const { return *this; }
} noerr;

#ifdef NDEBUG
#  define derr noerr
#else 
#  define derr std::cerr
#endif

#endif
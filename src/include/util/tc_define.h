#ifndef _TC_DEFINE_
#define _TC_DEFINE_


#include <sstream>


#define __FILE__LINE__FUNCTION__ (string("[") +string(__FILE__) + string(":")+  TC_Common::tostr<int>(__LINE__) + string(",")+string(__FUNCTION__)+string("]"))

#endif
#ifndef _TC_TYPE_
#define _TC_TYPE_

#include <functional>
#include <sstream>
#include <util/tc_define.h>

#define LOGF_INFO(x) info(__FILE__LINE__FUNCTION__+(x))
#define LOGF_DEBUG(x) debug(__FILE__LINE__FUNCTION__+(x))
#define LOGF_ERROR(x) error(__FILE__LINE__FUNCTION__+(x))

//上面只能在框架tc_epoll_server.cpp中用

#endif
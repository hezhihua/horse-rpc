/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */
#include "util/tc_epoller.h"

#include <unistd.h>


namespace horsedb
{

TC_Epoller::NotifyInfo::NotifyInfo() : _ep(NULL)
{
}

TC_Epoller::NotifyInfo::~NotifyInfo()
{
    _notify.close();
    _notifyClient.close(); 
}

void TC_Epoller::NotifyInfo::init(TC_Epoller *ep)
{
    _ep = ep;
    int fd[2];
    TC_Socket::createPipe(fd, false);
    _notify.init(fd[0], true);
    _notifyClient.init(fd[1], true);
    _notifyClient.setKeepAlive();
    _notifyClient.setTcpNoDelay();         
}

void TC_Epoller::NotifyInfo::add(uint64_t data)
{
    _data = data;
    _ep->add(_notify.getfd(), data, EPOLLIN | EPOLLOUT);
}

void TC_Epoller::NotifyInfo::notify()
{
    _ep->mod(_notify.getfd(), _data, EPOLLIN | EPOLLOUT);
}

void TC_Epoller::NotifyInfo::release()
{
    _ep->del(_notify.getfd(), 0, EPOLLIN | EPOLLOUT);
    _notify.close();
    _notifyClient.close(); 
}

int TC_Epoller::NotifyInfo::notifyFd()
{
    return _notify.getfd();
}

//////////////////////////////////////////////////////////////////////

TC_Epoller::TC_Epoller()
{

	_iEpollfd = -1;

	_pevs     = nullptr;
	_max_connections = 1024;
}

TC_Epoller::~TC_Epoller()
{
	if(_pevs != nullptr)
	{
		delete[] _pevs;
		_pevs = nullptr;
	}


	if (_iEpollfd > 0)
		::close(_iEpollfd);


}


void TC_Epoller::ctrl(SOCKET_TYPE fd, uint64_t data, uint32_t events, int op)
{
	struct epoll_event ev;
	ev.data.u64 = data;


    ev.events   = events | EPOLLET;


	epoll_ctl(_iEpollfd, op, fd, &ev);
}


void TC_Epoller::create(int size)
{

	_iEpollfd = epoll_create(size);

    if (nullptr != _pevs)
    {
        delete[] _pevs;
    }

    _max_connections = 1024;

    _pevs = new epoll_event[_max_connections];
}

void TC_Epoller::close()
{
    ::close(_iEpollfd);

    _iEpollfd = 0;
}

void TC_Epoller::add(SOCKET_TYPE fd, uint64_t data, int32_t event)
{

    ctrl(fd, data, event, EPOLL_CTL_ADD);

}

void TC_Epoller::mod(SOCKET_TYPE fd, uint64_t data, int32_t event)
{

    ctrl(fd, data, event, EPOLL_CTL_MOD);

}

void TC_Epoller::del(SOCKET_TYPE fd, uint64_t data, int32_t event)
{

    ctrl(fd, data, event, EPOLL_CTL_DEL);

}

epoll_event& TC_Epoller::get(int i) 
{ 
	assert(_pevs != 0); 
	return _pevs[i]; 
}

int TC_Epoller::wait(int millsecond)
{
  
retry:    


	int ret;

	ret = epoll_wait(_iEpollfd, _pevs, _max_connections, millsecond);



	if(ret < 0 && errno == EINTR)
	{
		goto retry;
	}

	return ret;

}

bool TC_Epoller::readEvent(const epoll_event &ev)
{

    if (ev.events & EPOLLIN)

    {
        return true;
    }

    return false;
}

bool TC_Epoller::writeEvent(const epoll_event &ev)
{

    if (ev.events & EPOLLOUT)              

    {
        return true;
    }

    return false;
}

bool TC_Epoller::errorEvent(const epoll_event &ev)
{

    if (ev.events & EPOLLERR || ev.events & EPOLLHUP)
    {
        return true;
    }

    return false;
}

uint32_t TC_Epoller::getU32(const epoll_event &ev, bool high)
{
    uint32_t u32 = 0;
    if(high)
    {

        u32 = ev.data.u64 >> 32;
     
    }
    else
    {
   
        u32 = ev.data.u32;

    }

    return u32;
}

uint64_t TC_Epoller::getU64(const epoll_event &ev)
{
    uint64_t data;

    data = ev.data.u64;

    return data;
}

}



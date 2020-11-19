﻿/**
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

#include "client/CommunicatorEpoll.h"
#include "client/Communicator.h"
#include "logger/logger.h"

using namespace std;

namespace horsedb
{

CommunicatorEpoll::CommunicatorEpoll(Communicator * pCommunicator,size_t netThreadSeq)
: _communicator(pCommunicator)
, _terminate(false)
, _nextTime(0)
, _nextStatTime(0)
, _objectProxyFactory(NULL)
, _netThreadSeq(netThreadSeq)
, _noSendQueueLimit(1000)
, _timeoutCheckInterval(100)
{
    _ep.create(1024);

    _terminateFDInfo.notify.init(&_ep);
    _terminateFDInfo.iType = FDInfo::ET_C_TERMINATE;
    _terminateFDInfo.notify.add((uint64_t)&_terminateFDInfo);

    //ObjectProxyFactory 对象
    _objectProxyFactory = new ObjectProxyFactory(this);

    //节点队列未发送请求的大小限制
    _noSendQueueLimit = TC_Common::strto<size_t>(pCommunicator->getProperty("nosendqueuelimit", "100000"));
    if(_noSendQueueLimit < 1000)
    {
        _noSendQueueLimit = 1000;
    }


    //检查超时请求的时间间隔，单位:ms
    _timeoutCheckInterval = TC_Common::strto<int64_t>(pCommunicator->getProperty("timeoutcheckinterval", "100"));
    if(_timeoutCheckInterval < 1)
    {
        _timeoutCheckInterval = 1;
    }

	for(size_t i = 0;i < MAX_CLIENT_NOTIFYEVENT_NUM;++i)
	{
		_notify[i] = NULL;
	}

}

CommunicatorEpoll::~CommunicatorEpoll()
{
    for(size_t i = 0;i < MAX_CLIENT_NOTIFYEVENT_NUM;++i)
    {
        if(_notify[i])
        {
            delete _notify[i];
        }
        _notify[i] = NULL;
    }
	
    if(_objectProxyFactory)
    {
        delete _objectProxyFactory;
        _objectProxyFactory = NULL;
    }
}

void CommunicatorEpoll::terminate()
{
    _terminate = true;
    //通知epoll响应
    _terminateFDInfo.notify.notify();
}

ObjectProxy * CommunicatorEpoll::getObjectProxy(const string & sObjectProxyName,const string& setName)
{
    return _objectProxyFactory->getObjectProxy(sObjectProxyName,setName);
}

void CommunicatorEpoll::addFd(int fd, FDInfo * info, uint32_t events)
{
    _ep.add(fd,(uint64_t)info,events);
}

void CommunicatorEpoll::modFd(int fd,FDInfo * info, uint32_t events)
{
    _ep.mod(fd, (uint64_t)info, events);
}

void CommunicatorEpoll::delFd(int fd, FDInfo * info, uint32_t events)
{
    _ep.del(fd,(uint64_t)info,events);
}

void CommunicatorEpoll::notify(size_t iSeq,ReqInfoQueue * msgQueue)
{
    assert(iSeq < MAX_CLIENT_NOTIFYEVENT_NUM);

    if(_notify[iSeq] == NULL)
    {
        _notify[iSeq] = new FDInfo();
        _notify[iSeq]->iType = FDInfo::ET_C_NOTIFY;
        _notify[iSeq]->p     =(void*)msgQueue;
        _notify[iSeq]->iSeq  = iSeq;
        _notify[iSeq]->notify.init(&_ep);
        _notify[iSeq]->notify.add((uint64_t)_notify[iSeq]);
    }

    _notify[iSeq]->notify.notify();
}

void CommunicatorEpoll::notifyDel(size_t iSeq)
{
    assert(iSeq < MAX_CLIENT_NOTIFYEVENT_NUM);
    if(_notify[iSeq] && NULL != _notify[iSeq]->p)
    {
        _notify[iSeq]->notify.notify();
    }
}


void CommunicatorEpoll::handleInputImp(Transceiver * pTransceiver)
{
    TLOGTARS("handleInputImp"<< endl);
    //检查连接是否有错误
    if(pTransceiver->isConnecting())
    {
        int iVal = 0;
        SOCKET_LEN_TYPE iLen = static_cast<SOCKET_LEN_TYPE>(sizeof(int));
        if (::getsockopt(pTransceiver->fd(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&iVal), &iLen) == -1 || iVal)
        {
            pTransceiver->close();
            pTransceiver->getAdapterProxy()->addConnExc(true);
            TLOGERROR("[CommunicatorEpoll::handleInputImp] connect error "
                    << pTransceiver->getAdapterProxy()->endpoint().desc()
                    << "," << pTransceiver->getAdapterProxy()->getObjProxy()->name()
                    << ",_connExcCnt=" << pTransceiver->getAdapterProxy()->ConnExcCnt()
                    << "," << strerror(iVal) << endl);
            return;
        }

        pTransceiver->setConnected();
    }

	pTransceiver->doResponse();
}

void CommunicatorEpoll::handleOutputImp(Transceiver * pTransceiver)
{
    TLOGTARS("handleOutputImp"<< endl);
    //检查连接是否有错误
    if(pTransceiver->isConnecting())
    {
        int iVal = 0;
        SOCKET_LEN_TYPE iLen = static_cast<SOCKET_LEN_TYPE>(sizeof(int));
        if (::getsockopt(pTransceiver->fd(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&iVal), &iLen) == -1 || iVal)
        {
            pTransceiver->close();
            pTransceiver->getAdapterProxy()->addConnExc(true);
            TLOGERROR("[CommunicatorEpoll::handleOutputImp] connect error "
                    << pTransceiver->getAdapterProxy()->endpoint().desc()
                    << "," << pTransceiver->getAdapterProxy()->getObjProxy()->name()
                    << ",_connExcCnt=" << pTransceiver->getAdapterProxy()->ConnExcCnt()
                    << "," << strerror(iVal) << endl);
            return;
        }

        pTransceiver->setConnected();
    }

    pTransceiver->doRequest();
}

void CommunicatorEpoll::handle(FDInfo * pFDInfo, const epoll_event &ev)
{
    try
    {
        assert(pFDInfo != NULL);
        if(FDInfo::ET_C_TERMINATE == pFDInfo->iType)
        {
            //结束通知过来
            return;
        }
        else if(FDInfo::ET_C_NOTIFY == pFDInfo->iType)
        {
            //队列有消息通知过来
            ReqInfoQueue * pInfoQueue=(ReqInfoQueue*)pFDInfo->p;
            ReqMessage * msg = NULL;

	        size_t maxProcessCount = 0;

            try
            {
                while(pInfoQueue->pop_front(msg))
                {
                    //线程退出了
                    if(ReqMessage::THREAD_EXIT == msg->eType)
                    {
                        assert(pInfoQueue->empty());

						size_t iSeq = pFDInfo->iSeq;

						_notify[iSeq]->notify.release();

                        _notify[iSeq]->p = NULL;

                        delete _notify[iSeq];

                        _notify[iSeq] = NULL;

						//delete msg
						delete msg;

						//delete queue
						delete pInfoQueue;

                        return;
                    }

                    try
                    {
                        msg->pObjectProxy->invoke(msg);
                    }
                    catch(exception & e)
                    {
                        TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
                    }
                    catch(...)
                    {
                        TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
                    }

                    if(++maxProcessCount > 1000)
                    {
                        //避免包太多的时候, 循环占用网路线程, 导致连接都建立不上, 一个包都无法发送出去
                        pFDInfo->notify.notify();
                        break;
                    }
                }
            }
            catch(exception & e)
            {
                TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
            }
            catch(...)
            {
                TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
            }
        }
        else
        {

            Transceiver *pTransceiver = (Transceiver*)pFDInfo->p;

            //先收包
            if(TC_Epoller::readEvent(ev))
            {
                try
                {
                    handleInputImp(pTransceiver);
                }
                catch(exception & e)
                {
                    TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
                }
                catch(...)
                {
                    TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
                }
            }

            //发包
            if(TC_Epoller::writeEvent(ev))
            {
                try
                {
                    handleOutputImp(pTransceiver);
                }
                catch(exception & e)
                {
                    TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
                }
                catch(...)
                {
                    TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
                }
            }

            //连接出错 直接关闭连接
	        if(TC_Epoller::errorEvent(ev))
            {
                try
                {
                    pTransceiver->close();
                }
                catch(exception & e)
                {
                    TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
                }
                catch(...)
                {
                    TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
                }
            }
        }
    }
    catch(exception & e)
    {
        TLOGERROR("CommunicatorEpoll::handle exp:"<<e.what()<<" ,line:"<<__LINE__<<endl);
    }
    catch(...)
    {
        TLOGERROR("CommunicatorEpoll::handle|"<<__LINE__<<endl);
    }
}

void CommunicatorEpoll::doTimeout()
{
    int64_t iNow = TNOWMS;
    if(_nextTime > iNow)
    {
        return;
    }

    //每_timeoutCheckInterval检查一次
    _nextTime = iNow + _timeoutCheckInterval;

    for(size_t i = 0; i < _objectProxyFactory->getObjNum(); ++i)
    {
        _objectProxyFactory->getObjectProxy(i)->doTimeout();
    }
}

void CommunicatorEpoll::doStat()
{

}

void CommunicatorEpoll::pushAsyncThreadQueue(ReqMessage * msg)
{
    _communicator->pushAsyncThreadQueue(msg);
}

void CommunicatorEpoll::reConnect(int64_t ms, Transceiver*p)
{
	_reconnect[ms] = p;
}

string CommunicatorEpoll::getResourcesInfo()
{
    return "ResourcesInfo";

}

void CommunicatorEpoll::reConnect()
{
	int64_t iNow = TNOWMS;

	while(!_reconnect.empty())
	{
		auto it = _reconnect.begin();

		if(it->first > iNow)
		{
			return;
		}

		it->second->reconnect();

		_reconnect.erase(it++);
	}
}

void CommunicatorEpoll::run()
{
    // TLOGDEBUG("CommunicatorEpoll::run id:"<<syscall(SYS_gettid)<<endl);

    ServantProxyThreadData * pSptd = ServantProxyThreadData::getData();
    assert(pSptd != NULL);
    pSptd->_netThreadSeq = (int)_netThreadSeq;

    while (!_terminate)
    {
        try
        {
            //考虑到检测超时等的情况 这里就wait100ms吧
            int num = _ep.wait(100);
			if (_terminate) break;


            //先处理epoll的网络事件
            for (int i = 0; i < num; ++i)
            {
                const epoll_event& ev = _ep.get(i);

                uint64_t data = TC_Epoller::getU64(ev);

                if(data == 0) continue; //data非指针, 退出循环

//	            int64_t ms = TNOWMS;

                handle((FDInfo*)data, ev);
            }

            //处理超时请求
            doTimeout();

            //数据上报
            doStat();
	        reConnect();
        }
        catch (exception& e)
        {
            TLOGERROR("[CommunicatorEpoll:run exception:" << e.what() << "]" << endl);
        }
        catch (...)
        {
            TLOGERROR("[CommunicatorEpoll:run exception.]" << endl);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
}

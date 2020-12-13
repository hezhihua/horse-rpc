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


#include "util/tc_timeprovider.h"
#include "logger/logger.h"
#include "client/ServantHandle.h"

#include "client/AppProtocol.h"
#include "client/ServantProxy.h"
#include "client/protocol/BaseF.h"
#include "client/Servant.h"

namespace horsedb
{

/////////////////////////////////////////////////////////////////////////
//
ServantHandle::ServantHandle()
: _coroSched(NULL)
{
    
}

ServantHandle::~ServantHandle()
{
    

    if(_coroSched != NULL)
    {
        delete _coroSched;
        _coroSched = NULL;
    }
}

void ServantHandle::run()
{
	try
	{
		initialize();

	    if (!OpenCoroutine)
	    {
	        handleImp();
	    }
	    else
	    {        
	        //by goodenpei, 判断是否启用顺序模式
	        _bindAdapter->initThreadRecvQueue(getHandleIndex());
        
	        size_t iThreadNum = getEpollServer()->getLogicThreadNum();

	        size_t iCoroutineNum = (CoroutineMemSize > CoroutineStackSize) ? (CoroutineMemSize / (CoroutineStackSize * iThreadNum)) : 1;
	        if (iCoroutineNum < 1) iCoroutineNum = 1;

			startHandle();

			_coroSched = new CoroutineScheduler();
			_coroSched->init(iCoroutineNum, CoroutineStackSize);
			_coroSched->setHandle(this);

			_coroSched->createCoroutine(std::bind(&ServantHandle::handleRequest, this));

			ServantProxyThreadData *pSptd = ServantProxyThreadData::getData();

			assert(pSptd != NULL);

			pSptd->_sched = _coroSched;

			while (!getEpollServer()->isTerminate()) 
			{
				_coroSched->tars_run();
			}

			_coroSched->terminate();

			_coroSched->destroy();

			stopHandle();
		}
	}
	catch(exception &ex)
	{
		TLOGERROR("[ServantHandle::run exception error:" << ex.what() << "]" << endl);
		cerr << "ServantHandle::run exception error:" << ex.what() << endl;
	}
	catch(...)
	{
		TLOGERROR("[ServantHandle::run unknown exception error]" << endl);
		cerr << "ServantHandle::run unknown exception error]" << endl;
	}
}


void ServantHandle::handleRequest()
{
    bool bYield = false;
    while (!getEpollServer()->isTerminate())
    {
    	wait();

        //上报心跳
        heartbeat();

        //为了实现所有主逻辑的单线程化,在每次循环中给业务处理自有消息的机会
        handleAsyncResponse();
        handleCustomMessage(true);

        bYield = false;

	    shared_ptr<TC_EpollServer::RecvContext> data;
        try
        {
            bool bFlag = true;
            int	iLoop = 100;
            while (bFlag && iLoop > 0)
            {
                --iLoop;
                if ((_coroSched->getFreeSize() > 0) && popRecvQueue(data))
                {
                    bYield = true;

                    //上报心跳
                    heartbeat();

                    //为了实现所有主逻辑的单线程化,在每次循环中给业务处理自有消息的机会
                    handleAsyncResponse();

                    //数据已超载 overload
                    if (data->isOverload())
                    {
                        handleOverload(data);
                    }
                    //关闭连接的通知消息
                    else if (data->isClosed())
                    {
                        handleClose(data);
                    }
                    //数据在队列中已经超时了
                    else if ((TNOWMS - data->recvTimeStamp()) > (int64_t)_bindAdapter->getQueueTimeout())
                    {
                        handleTimeout(data);
                    }
                    else
                    {
                        uint32_t iRet = _coroSched->createCoroutine(std::bind(&ServantHandle::handleRecvData, this, data));
                        if (iRet == 0)
                        {
                            handleOverload(data);
                        }
                    }
                    handleCustomMessage(false);
                }
                else
                {
                    //_coroSched->yield();
                    bFlag = false;
                    bYield = false;
                }
            }

            if (iLoop == 0) bYield = false;
        }
        catch (exception& ex)
        {
            if (data)
            {
                close(data);
            }

            getEpollServer()->error("[Handle::handleImp] error:" + string(ex.what()));
        }
        catch (...)
        {
            if (data)
            {
                close(data);
            }

            getEpollServer()->error("[Handle::handleImp] unknown error");
        }
        if (!bYield)
        {
            _coroSched->yield();
        }
    }
}

void ServantHandle::handleRecvData(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    try
    {
        TarsCurrentPtr current = createCurrent(data);

        if (!current) 
        {
            return;
        }

        if (current->getBindAdapter()->isTarsProtocol())
        {
            handleTarsProtocol(current);
        }
        
    }
    catch(exception &ex)
    {
        TLOGERROR("[ServantHandle::handleRecvData exception:" << ex.what() << "]" << endl);
    }
    catch(...)
    {
        TLOGERROR("[ServantHandle::handleRecvData unknown exception error]" << endl);
    }
}

void ServantHandle::handleAsyncResponse()
{

}

void ServantHandle::handleCustomMessage(bool bExpectIdle)
{
   
}

bool ServantHandle::allFilterIsEmpty()
{
    
    return true;
}

void ServantHandle::initialize()
{
    cout << "ServantHandle::initialize: " << std::this_thread::get_id() << endl;
}

void ServantHandle::heartbeat()
{
    
}

TarsCurrentPtr ServantHandle::createCurrent(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TarsCurrentPtr current (new Current(this)) ;

    try
    {
        current->initialize(data);
    }
    catch (TarsDecodeException &ex)
    {
        TLOGERROR("[ServantHandle::handle request protocol decode error:" << ex.what() << "]" << endl);
        close(data);
        return NULL;
    }

    //只有TARS协议才处理
    if(current->getBindAdapter()->isTarsProtocol())
    {
        int64_t now = TNOWMS;

        //数据在队列中的时间超过了客户端等待的时间(TARS协议)
        if (current->_request.iTimeout > 0 && (now - data->recvTimeStamp()) > current->_request.iTimeout)
        {

            TLOGERROR("[ServantHandle::handle queue timeout:"
                         << current->_request.sServantName << ", func:"
                         << current->_request.sFuncName << ", recv time:"
                         << data->recvTimeStamp()  << ", queue timeout:"
                         << data->adapter()->getQueueTimeout() << ", timeout:"
                         << current->_request.iTimeout << ", now:"
                         << now << ", ip:" << data->ip() << ", port:" << data->port() << "]" << endl);

            current->sendResponse(TARSSERVERQUEUETIMEOUT);

            return NULL;
        }
    }

    return current;
}

TarsCurrentPtr ServantHandle::createCloseCurrent(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TarsCurrentPtr current (new Current(this));

    current->initializeClose(data);
    current->setReportStat(false);
    current->setCloseType(data->closeType());
    return current;
}

void ServantHandle::handleClose(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TLOGTARS("[ServantHandle::handleClose,adapter:" << data->adapter()->getName() << ",peer:" << data->ip() << ":" << data->port() << "]"<< endl);

    TarsCurrentPtr current = createCloseCurrent(data);

   
}

void ServantHandle::handleTimeout(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TarsCurrentPtr current = createCurrent(data);

    if (!current) return;



    TLOGERROR("[ServantHandle::handleTimeout adapter '"
                 << data->adapter()->getName()
                 << "', recvtime:" << data->recvTimeStamp() << "|"
                 << ", timeout:" << data->adapter()->getQueueTimeout()
                 << ", id:" << current->getRequestId() << "]" << endl);

    if (current->getBindAdapter()->isTarsProtocol())
    {
        current->sendResponse(TARSSERVERQUEUETIMEOUT);
    }
}

void ServantHandle::handleOverload(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TarsCurrentPtr current = createCurrent(data);

    if (!current) return;

    TLOGERROR("[ServantHandle::handleOverload adapter '"
                 << data->adapter()->getName()
                 << "',overload:-1,queue capacity:"
                 << data->adapter()->getQueueCapacity()
                 << ",id:" << current->getRequestId() << "]" << endl);

    if (current->getBindAdapter()->isTarsProtocol())
    {
        current->sendResponse(TARSSERVEROVERLOAD);
    }
}

void ServantHandle::handle(const shared_ptr<TC_EpollServer::RecvContext> &data)
{
    TarsCurrentPtr current = createCurrent(data);

    if (!current) return;

    if (current->getBindAdapter()->isTarsProtocol())
    {
        handleTarsProtocol(current);
    }

}




bool ServantHandle::processCookie(const TarsCurrentPtr &current, map<string, string> &cookie)
{
	const static string STATUS = "STATUS_";

	std::for_each(current->getRequestStatus().begin(), current->getRequestStatus().end(),[&](const map<string, string>::value_type& p){
		if(p.first.size() > STATUS.size() && strncmp(p.first.c_str(), STATUS.c_str(), STATUS.size()) == 0) {
			return;
		}
		cookie.insert(make_pair(p.first, p.second));
	});

	return !cookie.empty();
}


void ServantHandle::handleTarsProtocol(const TarsCurrentPtr &current)
{
    TLOGTARS("[ServantHandle::handleTarsProtocol current:"
                << current->getIp() << "|"
                << current->getPort() << "|"
                << current->getMessageType() << "|"
                << current->getServantName() << "|"
                << current->getFuncName() << "|"
                << current->getRequestId() << "|"
                << TC_Common::tostr(current->getRequestStatus()) << "]"<<endl);


    //处理cookie
    map<string, string> cookie;

    if (processCookie(current, cookie))
    {

        current->setCookie(cookie);
    }


    int ret = TARSSERVERUNKNOWNERR;

    string sResultDesc = "";

    vector<char> buffer;
    ResponsePacket response;
    try
    {
        //业务逻辑处理
        //ret = sit->second->dispatch(current, buffer);
        ServantPtr Servant;
        if (ServantManager::getInstance()->getServant(current->getServantName(),Servant))
        {
            ret = Servant->onDispatch(current, response.sBuffer);
        }
        
        
    }
    catch(TarsDecodeException &ex)
    {
        TLOGERROR("[ServantHandle::handleTarsProtocol " << ex.what() << "]" << endl);

        ret = TARSSERVERDECODEERR;

        sResultDesc = ex.what();
    }
    catch(TarsEncodeException &ex)
    {
        TLOGERROR("[ServantHandle::handleTarsProtocol " << ex.what() << "]" << endl);

        ret = TARSSERVERENCODEERR;

        sResultDesc = ex.what();
    }
    catch(exception &ex)
    {
        TLOGERROR("[ServantHandle::handleTarsProtocol " << ex.what() << "]" << endl);

        ret = TARSSERVERUNKNOWNERR;

        sResultDesc = ex.what();
    }
    catch(...)
    {
        TLOGERROR("[ServantHandle::handleTarsProtocol unknown error]" << endl);

        ret = TARSSERVERUNKNOWNERR;

        sResultDesc = "handleTarsProtocol unknown exception error";
    }

    //单向调用或者业务不需要同步返回
    if (current->isResponse())
    {
        current->sendResponse(ret, response, Current::TARS_STATUS(), sResultDesc);
    }

}


////////////////////////////////////////////////////////////////////////////
}

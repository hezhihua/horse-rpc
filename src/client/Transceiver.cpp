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

#include "client/Transceiver.h"
#include "client/AdapterProxy.h"

#include "logger/logger.h"
#include "util/tc_enum.h"

namespace horsedb
{

static const int BUFFER_SIZE = 16 * 1024;

///////////////////////////////////////////////////////////////////////
Transceiver::Transceiver(AdapterProxy * pAdapterProxy,const EndpointInfo &ep)
: _adapterProxy(pAdapterProxy)
, _ep(ep)
, _fd(-1)
, _connStatus(eUnconnected)
, _conTimeoutTime(0)
, _authState(AUTH_INIT)
, _sendBuffer(this)
, _recvBuffer(this)
{
    _fdInfo.iType = FDInfo::ET_C_NET;
    _fdInfo.p     = (void *)this;
    _fdInfo.fd    = -1;
}

Transceiver::~Transceiver()
{
    close();


}

void Transceiver::checkTimeout()
{
    if(eConnecting == _connStatus && TNOWMS > _conTimeoutTime)
    {
        //链接超时
        TLOGERROR("[Transceiver::checkTimeout ep:"<<_adapterProxy->endpoint().desc()<<" , connect timeout]"<<endl);
        _adapterProxy->setConTimeout(true);
        close();
    }
}

bool Transceiver::isSSL() const 
{ 
    return _adapterProxy->endpoint().type() == TC_Endpoint::SSL;
}

void Transceiver::reconnect()
{
    connect();
}

void Transceiver::connect()
{
    if(isValid())
    {
        return;
    }

    if(_connStatus == eConnecting || _connStatus == eConnected)
    {
        return;
    }

    int fd = -1;



    //每次连接前都重新解析一下地址, 避免dns变了!
    _ep.parseConnectAddress();

   
    {
        fd = NetworkUtil::createSocket(false, false, _ep.isConnectIPv6());

        _adapterProxy->getObjProxy()->getCommunicatorEpoll()->addFd(fd, &_fdInfo, EPOLLIN | EPOLLOUT);

        socklen_t len = _ep.isIPv6() ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
        bool bConnected = NetworkUtil::doConnect(fd, _ep.connectAddrPtr(), len);
        if(bConnected)
        {
            setConnected();
        }
        else
        {
            //cout<<"===eConnecting"<<endl;
            _connStatus     = Transceiver::eConnecting;
            _conTimeoutTime = TNOWMS + _adapterProxy->getConTimeout();
        }
    }

    _fd = fd;

    TLOGTARS("[Transceiver::connect obj:" << _adapterProxy->getObjProxy()->name()
        << ",connect:" << _ep.getConnectEndpoint()->toString() << ", fd:" << _fd << "]" << endl);

    // //设置网络qos的dscp标志
    // if(0 != _ep.qos())
    // {
    //     int iQos=_ep.qos();
    //     ::setsockopt(fd,SOL_IP,IP_TOS,&iQos,sizeof(iQos));
    // }

    //设置套接口选项
    vector<SocketOpt> &socketOpts = _adapterProxy->getObjProxy()->getSocketOpt();
    for(size_t i=0; i<socketOpts.size(); ++i)
    {
        if(setsockopt(_fd,socketOpts[i].level,socketOpts[i].optname, (const char*)socketOpts[i].optval,socketOpts[i].optlen) == -1)
        {
            TLOGERROR("[setsockopt error:" << TC_Exception::parseError(TC_Exception::getSystemCode()) 
                << ",objname:" << _adapterProxy->getObjProxy()->name() 
                << ",desc:" << _ep.getConnectEndpoint()->toString()
                << ",fd:" << _fd
                << ",level:" <<  socketOpts[i].level
                << ",optname:" << socketOpts[i].optname
                << ",optval:" << socketOpts[i].optval
                <<"    ]"<< endl);
        }
    }
}

void Transceiver::setConnected()
{
    _connStatus = eConnected;
    _adapterProxy->setConTimeout(false);
    _adapterProxy->addConnExc(false);

	TLOGTARS("[tcp setConnected, " << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << "]" << endl);

	
	onSetConnected();
	
}

void Transceiver::onSetConnected()
{
	onConnect();

	if(_adapterProxy->getObjProxy()->getPushCallback())
	{
		_adapterProxy->getObjProxy()->getPushCallback()->onConnect(*_ep.getConnectEndpoint());
	}

	_adapterProxy->onConnect();
}

void Transceiver::onConnect()
{


	//doAuthReq(); 要连接的服务连接后需要马上发送认证消息,这里不需要
}

void Transceiver::connectProxy()
{
	
}

int Transceiver::doCheckProxy(const char *buff, size_t length)
{

		return 0;

	
}

void Transceiver::doAuthReq()
{
    ObjectProxy* obj = _adapterProxy->getObjProxy();

    TLOGTARS("[Transceiver::doAuthReq obj:" << obj->name() << ", auth type:" << (AUTH_TYPE)_adapterProxy->endpoint().authType() << endl);

    if (_adapterProxy->endpoint().authType() ==0 )//AUTH_TYPENONE
    {
        _authState = AUTH_SUCC;
        TLOGTARS("[Transceiver::doAuthReq doInvoke obj:" << obj->name() << ", auth type:" << (AUTH_TYPE)_adapterProxy->endpoint().authType() << endl);
        _adapterProxy->doInvoke(true);
    }
    else
    {
        TLOGTARS("[Transceiver::doAuthReq doInvoke obj:" << obj->name() << ", auth type:" << (AUTH_TYPE)_adapterProxy->endpoint().authType() << endl);

    }
}

void Transceiver::finishInvoke(shared_ptr<ResponsePacket> &rsp)
{
	
	_adapterProxy->finishInvoke(rsp);
}


void Transceiver::close(bool destructor )
{
    if(!isValid()) return;




    _adapterProxy->getObjProxy()->getCommunicatorEpoll()->delFd(_fd,&_fdInfo, 0);

    TLOGTARS("[Transceiver::close fd:" << _fd << "]" << endl);

    NetworkUtil::closeSocketNoThrow(_fd);

    _connStatus = eUnconnected;

    _fd = -1;

	_sendBuffer.clearBuffers();

	_recvBuffer.clearBuffers();

    _authState = AUTH_INIT;

	//如果从析构函数调用close，则不走以下流程
    if (!destructor)
    {
        if (_adapterProxy->getObjProxy()->getPushCallback())
        {
            _adapterProxy->getObjProxy()->getPushCallback()->onClose(*_ep.getConnectEndpoint());
        }

        int second = _adapterProxy->getObjProxy()->reconnect();
        if (second > 0) 
        {
            int64_t nextTryConnectTime = TNOWMS + second * 1000;
            _adapterProxy->getObjProxy()->getCommunicatorEpoll()->reConnect(nextTryConnectTime, this);
            TLOGTARS("[trans close:" << _adapterProxy->getObjProxy()->name() << "," << _ep.getConnectEndpoint()->toString() << ", reconnect:" << second << "]" << endl);
        }
    }
}

int Transceiver::doRequest()
{
    if(!isValid()) return -1;

	//buf不为空,先发送buffer的内容
    while(!_sendBuffer.empty())
    {
    	auto data = _sendBuffer.getBufferPointer();
    	assert(data.first != NULL && data.second != 0);

        int iRet = this->send(data.first, (uint32_t) data.second, 0);

        if (iRet < 0)
        {
            return -2;
        }

	    _sendBuffer.moveHeader(iRet);
    }

	//取adapter里面积攒的数据
    if(_sendBuffer.empty()) {
        _adapterProxy->doInvoke(false);
    }

    return 0;
}

int Transceiver::sendRequest(const shared_ptr<TC_NetWorkBuffer::Buffer> &buff)
{
    //空数据 直接返回成功
    if(buff->empty()) {
	    return eRetOk;
    }

    // assert(_sendBuffer.empty());
    //buf不为空, 表示之前的数据还没发送完, 直接返回失败, 等buffer可写了,epoll会通知写事件
    if(!_sendBuffer.empty()) {
        //不应该运行到这里
        TLOGTARS("[Transceiver::sendRequest should not happened, not empty obj: " << _adapterProxy->getObjProxy()->name() << endl);
	    return eRetNotSend;
    }

    if(eConnected != _connStatus)
    {
        TLOGTARS("[Transceiver::sendRequest not connected: " << _adapterProxy->getObjProxy()->name() << endl);
        return eRetNotSend;
    }

	

    // if (_authState != AUTH_SUCC)
    // {

    //     TLOGTARS("[Transceiver::sendRequest need auth: " << _adapterProxy->getObjProxy()->name() << endl);
    //     return eRetNotSend; // 需要鉴权但还没通过，不能发送非认证消息
    // }


    _sendBuffer.addBuffer(buff);


    do
    {
        auto data = _sendBuffer.getBufferPointer();

        int iRet = this->send(data.first, (uint32_t) data.second, 0);
        if(iRet < 0)
        {
            if(!isValid()) 
            {
                _sendBuffer.clearBuffers();
                TLOGTARS("[Transceiver::sendRequest failed eRetError: data size:" << data.second << "]" << endl);
                return eRetError;
            } 
            else
            {
                TLOGTARS("[Transceiver::sendRequest failed eRetFull]" << endl);
                return eRetFull;
            }
        }

        _sendBuffer.moveHeader(iRet);
    }
    while(!_sendBuffer.empty());

    return eRetOk;
}

//////////////////////////////////////////////////////////
TcpTransceiver::TcpTransceiver(AdapterProxy * pAdapterProxy, const EndpointInfo &ep)
: Transceiver(pAdapterProxy, ep)
{
}


int TcpTransceiver::doResponse()
{
    if(!isValid()) return -1;

	int iRet = 0;

    int recvCount = 0;
	do
    {
	    char buff[BUFFER_SIZE] = {0x00};

	    if ((iRet = this->recv(buff, BUFFER_SIZE, 0)) > 0)
	    {
		    int check = doCheckProxy(buff, iRet);
            if(check != 0)
		    {
		    	return 0;
		    }

		    TC_NetWorkBuffer *rbuf = &_recvBuffer;

		    rbuf->addBuffer(buff, iRet);

		    ++recvCount;

		    try
		    {
			    PACKET_TYPE ret;

			    do
		        {
			        shared_ptr<ResponsePacket> rsp = std::make_shared<ResponsePacket>();

			        ret = _adapterProxy->getObjProxy()->getProxyProtocol().responseFunc(*rbuf, *rsp.get());

				    if (ret == PACKET_ERR) {
					    TLOGERROR( "[tcp doResponse," << _adapterProxy->getObjProxy()->name() << ", size:" << iRet << ", fd:" << _fd << "," << _ep.getConnectEndpoint()->toString() << ",tcp recv decode error" << endl);
					    close();
					    break;
				    }
				    else if (ret == PACKET_FULL) {
	                    finishInvoke(rsp);
				    }
					else if (ret == PACKET_FULL_CLOSE) {
						close();
	                    finishInvoke(rsp);
						break;
					}
				    else {
					    break;
				    }

			    }
			    while (ret == PACKET_FULL);

			    //接收的数据小于buffer大小, 内核会再次通知你
			    if(iRet < BUFFER_SIZE)
			    {
				    break;
			    }

			    //收包太多了, 中断一下, 释放线程给send等
			    if (recvCount >= 100 && isValid()) {
				    _adapterProxy->getObjProxy()->getCommunicatorEpoll()->modFd(_fd, &_fdInfo, EPOLLIN | EPOLLOUT);
				    break;
			    }
		    }
		    catch (exception & ex) {
			    TLOGERROR("[tcp doResponse," << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << ","
			                                      << _ep.getConnectEndpoint()->toString() << ",tcp recv decode error:" << ex.what() << endl);

			    close();
		    }
		    catch (...) {
			    TLOGERROR("[tcp doResponse," << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << ","
			                                      << _ep.getConnectEndpoint()->toString() << ",tcp recv decode error." << endl);

			    close();
		    }
	    }
    }
    while (iRet>0);

//    TLOGTARS("[tcp doResponse, " << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << ", all recvbuf:" << _recvBuffer.getBufferLength() << "]" << endl);

	return 0;
}

int TcpTransceiver::send(const void* buf, uint32_t len, uint32_t flag)
{
    //只有是连接状态才能收发数据
    if(eConnected != _connStatus)
        return -1;

	int iRet = ::send(_fd, (const char*)buf, len, flag);

	if (iRet < 0 && !TC_Socket::isPending())
    {
        TLOGTARS("[tcp send," << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << "," << _ep.getConnectEndpoint()->toString()
            << ",fail! errno:" << TC_Exception::getSystemCode() << "," 
            << TC_Exception::parseError(TC_Exception::getSystemCode()) << ",close]" << endl);

        close();

        return iRet;
    }


    TLOGTARS("[tcp send," << _adapterProxy->getObjProxy()->name() << ",fd:" << _fd << "," 
        << _ep.getConnectEndpoint()->toString() << ",len:" << iRet <<"]" << endl);

    return iRet;
}

int TcpTransceiver::recv(void* buf, uint32_t len, uint32_t flag)
{
    //只有是连接状态才能收发数据
    if(eConnected != _connStatus)
        return -1;

    int iRet = ::recv(_fd, (char*)buf, len, flag);

	if (iRet == 0 || (iRet < 0 && !TC_Socket::isPending()))
    {
        TLOGTARS("[tcp recv, " << _adapterProxy->getObjProxy()->name()
                << ",fd:" << _fd << ", " << _ep.getConnectEndpoint()->toString() <<",ret " << iRet
                << ", fail! errno:" << TC_Exception::getSystemCode() << "," << TC_Exception::parseError(TC_Exception::getSystemCode()) << ",close]" << endl);

        close();

        return 0;
    }


    TLOGTARS("[tcp recv," << _adapterProxy->getObjProxy()->name()
            << ",fd:" << _fd << "," << _ep.getConnectEndpoint()->toString() << ",ret:" << iRet << "]" << endl);

    return iRet;
}

/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
}

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

#include "client/EndpointManager.h"
#include "client/ObjectProxy.h"
#include "client/Communicator.h"
#include "logger/logger.h"

namespace horsedb
{
/////////////////////////////////////////////////////////////////////////////


EndpointManager::EndpointManager(ObjectProxy * pObjectProxy, Communicator* pComm, const string & sObjName, bool bFirstNetThread,const string& setName)
:_objectProxy(pObjectProxy)
,_communicator(pComm)
,_lastRoundPosition(0)
,_update(true)
{
    setNetThreadProcess(true);
    init(sObjName,"",setName);
}


EndpointManager::~EndpointManager()
{
    map<string,AdapterProxy*>::iterator iterAdapter;
    for(iterAdapter = _allProxys.begin();iterAdapter != _allProxys.end();iterAdapter++)
    {
        if(iterAdapter->second)
        {
            delete iterAdapter->second;
            iterAdapter->second = NULL;
        }
    }
}

bool EndpointManager::init(const string & sObjName,const string & sLocator,const string& setName)
{
    TLOGTARS("[QueryEpBase::init sObjName:" << sObjName << ",sLocator:" << sLocator << ",setName:" << setName << "]" << endl);


    setObjName(sObjName);

    return true;
}


void EndpointManager::setObjName(const string & sObjName)
{
    string::size_type pos = sObjName.find_first_of('@');

    string sEndpoints;
    string sInactiveEndpoints;

    if (pos != string::npos)
    {
        //[直接连接]指定服务的IP和端口列表
        _objName = sObjName.substr(0,pos);

        sEndpoints = sObjName.substr(pos + 1);

	    pos = _objName.find_first_of("#");

	    if(pos != string::npos)
	    {
		    _rootServant    = false;
		    _objName        = _objName.substr(0, pos);
	    }

	    _direct = true;

        _valid = true;
    }
    else
    {
        //[间接连接]通过registry查询服务端的IP和端口列表

        _direct = false;
        _valid = false;

        _objName = sObjName;

        

	    pos = _objName.find_first_of("#");
	    if(pos != string::npos)
	    {
		    _objName = _objName.substr(0, pos);
	    }


        
    }

    setEndpoints(sEndpoints, _activeEndpoints);
    

    if(_activeEndpoints.size() > 0)
    {
        _valid = true;
    }

     
}

void EndpointManager::setEndpoints(const string & sEndpoints, set<EndpointInfo> & setEndpoints)
{
    if(sEndpoints == "")
    {
        return ;
    }

    //bool         bSameWeightType  = true;
    //bool         bFirstWeightType = true;
    //unsigned int iWeightType      = 0;

     vector<string>  vEndpoints    = TC_Common::sepstr<string>(sEndpoints, ":");
    //vector<string>  vEndpoints = sepEndpoint(sEndpoints);

    for (size_t i = 0; i < vEndpoints.size(); ++i)
    {
        try
        {
            TC_Endpoint ep(vEndpoints[i]);

            EndpointInfo epi(ep.getHost(), ep.getPort(), ep.getType(), ep.getGrid(), "", ep.getQos(), ep.getWeight(), ep.getWeightType(), ep.getAuthType());

            setEndpoints.insert(epi);
        }
        catch (...)
        {
            TLOGERROR("[EndpointManager::setEndpoints parse error,objname:" << _objName << ",endpoint:" << vEndpoints[i] << "]" << endl);
        }
    }

    set<EndpointInfo> inactive;
    updateEndpoints(setEndpoints, inactive);

}


bool EndpointManager::getAdapterProxyByName(const string &name,AdapterProxy* &ap )
{
    auto iterAdapter = _regProxys.find(name);
    if(iterAdapter == _regProxys.end())
    {
        return false;
    }

    ap=iterAdapter->second;
    return true;
}

void EndpointManager::addEndpoints(const set<EndpointInfo> & active)
{
    auto iter = active.begin();
	for(;iter != active.end();++iter)
	{
		

		auto iterAdapter = _allProxys.find(iter->cmpDesc());
		if(iterAdapter == _allProxys.end())
		{
			AdapterProxy* ap = new AdapterProxy(_objectProxy, *iter, _communicator);

			auto result = _allProxys.insert(make_pair(iter->cmpDesc(),ap));
			assert(result.second);

			iterAdapter = result.first;

			_vAllProxys.push_back(ap);

            _activeProxys.push_back(iterAdapter->second);
		    _regProxys.insert(make_pair(iter->cmpDesc(),iterAdapter->second));
		}

	}

    //_vRegProxys 需要按顺序来 重排
	_vRegProxys.clear();
	auto iterAdapter = _regProxys.begin();
	for(;iterAdapter != _regProxys.end();++iterAdapter)
	{
		_vRegProxys.push_back(iterAdapter->second);
	}
}

void EndpointManager::updateEndpoints(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive)
{
	set<EndpointInfo>::const_iterator iter;
	map<string,AdapterProxy*>::iterator iterAdapter;
	pair<map<string,AdapterProxy*>::iterator,bool> result;

	_activeProxys.clear();
	_regProxys.clear();

	//更新active
	iter = active.begin();
	for(;iter != active.end();++iter)
	{
		

		iterAdapter = _allProxys.find(iter->cmpDesc());
		if(iterAdapter == _allProxys.end())
		{
			AdapterProxy* ap = new AdapterProxy(_objectProxy, *iter, _communicator);

			result = _allProxys.insert(make_pair(iter->cmpDesc(),ap));
			assert(result.second);

			iterAdapter = result.first;

			_vAllProxys.push_back(ap);
		}

		_activeProxys.push_back(iterAdapter->second);

		_regProxys.insert(make_pair(iter->cmpDesc(),iterAdapter->second));

		
	}

	//更新inactive
	iter = inactive.begin();
	for(;iter != inactive.end();++iter)
	{

		iterAdapter = _allProxys.find(iter->cmpDesc());
		if(iterAdapter == _allProxys.end())
		{
			AdapterProxy* ap = new AdapterProxy(_objectProxy, *iter, _communicator);

			result = _allProxys.insert(make_pair(iter->cmpDesc(),ap));
			assert(result.second);

			iterAdapter = result.first;

			_vAllProxys.push_back(ap);
		}

		//_regProxys.insert(make_pair(iter->cmpDesc(),iterAdapter->second));


	}

	//_vRegProxys 需要按顺序来 重排
	_vRegProxys.clear();
	iterAdapter = _regProxys.begin();
	for(;iterAdapter != _regProxys.end();++iterAdapter)
	{
		_vRegProxys.push_back(iterAdapter->second);
	}

	_update = true;
}



void EndpointManager::doNotify()
{
    _objectProxy->doInvoke();
}

bool EndpointManager::selectAdapterProxy(ReqMessage * msg,AdapterProxy * & pAdapterProxy)
{
    pAdapterProxy = NULL;

    //刷新主控
   // refreshReg(E_DEFAULT, "");

    //无效的数据 返回true
    if (!_valid) 
    {
        return true;
    }

    
    //普通轮询模式
    pAdapterProxy = getNextValidProxy();
    

    return false;
}

AdapterProxy * EndpointManager::getNextValidProxy()
{
    if (_activeProxys.empty())
    {
        TLOGERROR("[TAF][EndpointManager::getNextValidProxy activeEndpoints is empty][obj:"<<_objName<<"]" << endl);
        return NULL;
    }

    vector<AdapterProxy*> conn;

    for(size_t i=0;i<_activeProxys.size();i++)
    {
        ++_lastRoundPosition;
        if(_lastRoundPosition >= _activeProxys.size())
            _lastRoundPosition = 0;

	    if(_activeProxys[_lastRoundPosition]->checkActive(false))
        {
	        return _activeProxys[_lastRoundPosition];
        }

        if(!_activeProxys[_lastRoundPosition]->isConnTimeout() && !_activeProxys[_lastRoundPosition]->isConnExc()) {
	        conn.push_back(_activeProxys[_lastRoundPosition]);
        }
    }

    if(conn.size() > 0)
    {
        //都有问题, 随机选择一个没有connect超时或者链接异常的发送
        AdapterProxy * adapterProxy = conn[((uint32_t)rand() % conn.size())];

        //该proxy可能已经被屏蔽,需重新连一次
        adapterProxy->checkActive(true);
        return adapterProxy;
    }

    //所有adapter都有问题 选不到结点,随机找一个重试
    AdapterProxy * adapterProxy = _activeProxys[((uint32_t)rand() % _activeProxys.size())];

    //该proxy可能已经被屏蔽,需重新连一次
    adapterProxy->checkActive(true);

    return adapterProxy;
}

void EndpointManager::getEndpoints(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{

    TC_LockT<TC_SpinLock> lock(_mutex);

    activeEndPoint.insert(activeEndPoint.begin(),_activeEndpoints.begin(),_activeEndpoints.end());

    //inactiveEndPoint = _inactiveEndPoint;
    
}

void EndpointManager::notifyEndpoints(const set<EndpointInfo> & active,const set<EndpointInfo> & inactive,bool bNotify)
{
	updateEndpoints(active, inactive);

    //_objectProxy->onNotifyEndpoints(active, inactive);
}

/////////////////////////////////////////////////////////////////////////////
EndpointThread::EndpointThread(Communicator* pComm, const string & sObjName, GetEndpointType type, const string & sName, bool bFirstNetThread):EndpointManager(NULL,pComm,sObjName,bFirstNetThread)
 ,_type(type)
, _name(sName)
{
    init(sObjName,"","");
}

void EndpointThread::getEndpoints(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{


    {
        TC_LockT<TC_SpinLock> lock(_mutex);
        // TC_ThreadLock::Lock lock(_mutex);
    
        //refreshReg(_type,_name);
    
        activeEndPoint = _activeEndPoint;
        inactiveEndPoint = _inactiveEndPoint;
    }
}

void EndpointThread::getTCEndpoints(vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint)
{

    {
    
        TC_LockT<TC_SpinLock> lock(_mutex);
    
        //refreshReg(_type,_name);
    
        activeEndPoint = _activeTCEndPoint;
        inactiveEndPoint = _inactiveTCEndPoint;
    }
}

void EndpointThread::notifyEndpoints(const set<EndpointInfo> & active,const set<EndpointInfo> & inactive,bool bSync)
{
    if(!bSync)
    {
        TC_LockT<TC_SpinLock> lock(_mutex);

        update(active, inactive);
    }
    else
    {
        update(active, inactive);
    }
}

void EndpointThread::update(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive)
{
    _activeEndPoint.clear();
    _inactiveEndPoint.clear();

    _activeTCEndPoint.clear();
    _inactiveTCEndPoint.clear();

    set<EndpointInfo>::iterator iter= active.begin();
    for(;iter != active.end(); ++iter)
    {
//        TC_Endpoint ep = (iter->host(), iter->port(), 3000, iter->type(), iter->grid());

        _activeTCEndPoint.push_back(iter->getEndpoint());

        _activeEndPoint.push_back(*iter);
    }

    iter = inactive.begin();
    for(;iter != inactive.end(); ++iter)
    {
//        TC_Endpoint ep(iter->host(), iter->port(), 3000, iter->type(), iter->grid());

        _inactiveTCEndPoint.push_back(iter->getEndpoint());

        _inactiveEndPoint.push_back(*iter);
    }
}
/////////////////////////////////////////////////////////////////////////////
EndpointManagerThread::EndpointManagerThread(Communicator * pComm,const string & sObjName)
:_communicator(pComm)
,_objName(sObjName)
{
}
EndpointManagerThread::~EndpointManagerThread()
{
    map<string,EndpointThread*>::iterator iter;
    for(iter=_info.begin();iter != _info.end();iter++)
    {
        if(iter->second)
        {
            delete iter->second;
            iter->second = NULL;
        }
    }
}

void EndpointManagerThread::getEndpoint(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{
    EndpointThread * pThread  = getEndpointThread(E_DEFAULT,"");

    pThread->getEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getEndpointByAll(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{
    EndpointThread * pThread  = getEndpointThread(E_ALL,"");

    pThread->getEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getEndpointBySet(const string sName, vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{
    EndpointThread * pThread  = getEndpointThread(E_SET,sName);

    pThread->getEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getEndpointByStation(const string sName, vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint)
{

    EndpointThread * pThread  = getEndpointThread(E_STATION,sName);

    pThread->getEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getTCEndpoint(vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint)
{
    EndpointThread * pThread  = getEndpointThread(E_DEFAULT,"");

    pThread->getTCEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getTCEndpointByAll(vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint)
{

    EndpointThread * pThread  = getEndpointThread(E_ALL,"");

    pThread->getTCEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getTCEndpointBySet(const string sName, vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint)
{
    EndpointThread * pThread  = getEndpointThread(E_SET,sName);

    pThread->getTCEndpoints(activeEndPoint,inactiveEndPoint);
}

void EndpointManagerThread::getTCEndpointByStation(const string sName, vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint)
{

    EndpointThread * pThread  = getEndpointThread(E_STATION,sName);

    pThread->getTCEndpoints(activeEndPoint,inactiveEndPoint);
}

EndpointThread * EndpointManagerThread::getEndpointThread(GetEndpointType type,const string & sName)
{
    TC_LockT<TC_SpinLock> lock(_mutex);

    string sAllName = TC_Common::tostr((int)type) + ":" +  sName;

    map<string,EndpointThread*>::iterator iter;
    iter = _info.find(sAllName);
    if(iter != _info.end())
    {
        return iter->second;
    }

    EndpointThread * pThread = new EndpointThread(_communicator, _objName, type, sName);
    _info[sAllName] = pThread;

    return pThread;
}

}



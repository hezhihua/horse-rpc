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


#include "client/Communicator.h"
#include "logger/logger.h"

namespace horsedb
{

//////////////////////////////////////////////////////////////////////////////////////////////

string ClientConfig::LocalIp = "127.0.0.1";

string ClientConfig::ModuleName = "unknown";

set<string> ClientConfig::SetLocalIp;

bool ClientConfig::SetOpen = false;

string ClientConfig::SetDivision = "";

string ClientConfig::TarsVersion = string(TARS_VERSION);

//////////////////////////////////////////////////////////////////////////////////////////////

Communicator::Communicator()
: _initialized(false)
, _terminating(false)
, _clientThreadNum(1)

, _timeoutLogFlag(true)
, _minTimeout(100)

{

    memset(_communicatorEpoll,0,sizeof(_communicatorEpoll));
}

Communicator::Communicator( const string& domain/* = CONFIG_ROOT_PATH*/)
: _initialized(false)
, _terminating(false)
, _timeoutLogFlag(true)

{
    setProperty( domain);
}

Communicator::~Communicator()
{
    terminate();

   
}

bool Communicator::isTerminating()
{
    return _terminating;
}

map<string, string> Communicator::getServantProperty(const string &sObj)
{
	TC_LockT<TC_ThreadRecMutex> lock(*this);

	auto it = _objInfo.find(sObj);
	if(it != _objInfo.end())
	{
		return it->second;
	}

	return map<string, string>();
}

void Communicator::setServantProperty(const string &sObj, const string& name, const string& value)
{
	TC_LockT<TC_ThreadRecMutex> lock(*this);

	_objInfo[sObj][name] = value;
}

string Communicator::getServantProperty(const string &sObj, const string& name)
{
	TC_LockT<TC_ThreadRecMutex> lock(*this);

	auto it = _objInfo.find(sObj);
	if(it != _objInfo.end())
	{
		auto vit = it->second.find(name);

		if(vit != it->second.end())
		{
			return vit->second;
		}
	}

	return "";
}




void Communicator::setProperty( const string& domain/* = CONFIG_ROOT_PATH*/)
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);



    string defaultValue = "dft";
    if ((defaultValue == getProperty("enableset", defaultValue)) || (defaultValue == getProperty("setdivision", defaultValue)))
    {
        _properties["enableset"] = "n";
        _properties["setdivision"] ="NULL";
    }



//    initClientConfig();
}

void Communicator::initialize()
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);

    if (_initialized) return;

    _initialized = true;

	ClientConfig::TarsVersion   = TARS_VERSION;

    ClientConfig::SetOpen = TC_Common::lower(getProperty("enableset", "n")) == "y" ? true : false;

    if (ClientConfig::SetOpen)
    {
        ClientConfig::SetDivision = getProperty("setdivision");

        vector<string> vtSetDivisions = TC_Common::sepstr<string>(ClientConfig::SetDivision,".");

        string sWildCard = "*";

        if (vtSetDivisions.size()!=3 || vtSetDivisions[0]==sWildCard || vtSetDivisions[1]==sWildCard)
        {
            //set分组名不对时默认没有打开set分组
            ClientConfig::SetOpen = false;
            setProperty("enableset","n");
            TLOGERROR( "[set division name error:" << ClientConfig::SetDivision << ", client failed to open set]" << endl);
        }
    }

    ClientConfig::LocalIp = getProperty("localip", "");

    if (ClientConfig::SetLocalIp.empty())
    {
        vector<string> v = TC_Socket::getLocalHosts();
        for (size_t i = 0; i < v.size(); i++)
        {
            if (v[i] != "127.0.0.1" && ClientConfig::LocalIp.empty())
            {
                ClientConfig::LocalIp = v[i];
            }
            ClientConfig::SetLocalIp.insert(v[i]);
        }
    }

    //缺省采用进程名称
    string exe = "dfex-name";


	ClientConfig::ModuleName    = getProperty("modulename", exe);



	_servantProxyFactory = new ServantProxyFactory(this);

	_clientThreadNum = TC_Common::strto<size_t>(getProperty("netthread","1"));

	if(0 == _clientThreadNum)
	{
		_clientThreadNum = 1;
	}
	else if(MAX_CLIENT_THREAD_NUM < _clientThreadNum)
	{
		_clientThreadNum = MAX_CLIENT_THREAD_NUM;
	}

	//异步线程数
	_asyncThreadNum = TC_Common::strto<size_t>(getProperty("asyncthread", "3"));

	if(_asyncThreadNum == 0)
	{
		_asyncThreadNum = 3;
	}

	if(_asyncThreadNum > MAX_CLIENT_ASYNCTHREAD_NUM)
	{
		_asyncThreadNum = MAX_CLIENT_ASYNCTHREAD_NUM;
	}

	bool merge = TC_Common::strto<bool>(getProperty("mergenetasync", "0"));

	//异步队列的大小
	size_t iAsyncQueueCap = TC_Common::strto<size_t>(getProperty("asyncqueuecap", "100000"));
	if(iAsyncQueueCap < 10000)
	{
		iAsyncQueueCap = 10000;
	}

	//第一个通信器才去启动回调线程
	for (size_t i = 0; i < _asyncThreadNum; ++i) {
		_asyncThread.push_back(new AsyncProcThread(iAsyncQueueCap, merge));
	}



	for(size_t i = 0; i < _clientThreadNum; ++i)
	{
		_communicatorEpoll[i] = new CommunicatorEpoll(this, i);
		_communicatorEpoll[i]->start();
	}



	//初始化统计上报接口
	string statObj = getProperty("stat", "");

	string propertyObj = getProperty("property", "");



	_timeoutLogFlag = TC_Common::strto<bool>(getProperty("timeout-log-flag", "1"));

	_minTimeout = TC_Common::strto<int64_t>(getProperty("min-timeout", "100"));
	if(_minTimeout < 1)
		_minTimeout = 1;



}


void Communicator::setProperty(const map<string, string>& properties)
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);

    _properties = properties;
}

void Communicator::setProperty(const string& name, const string& value)
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);

    _properties[name] = value;
}

string Communicator::getProperty(const string& name, const string& dft/* = ""*/)
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);

    map<string, string>::iterator it = _properties.find(name);

    if (it != _properties.end())
    {
        return it->second;
    }
    return dft;
}



int Communicator::reloadProperty(string & sResult)
{
    for(size_t i = 0; i < _clientThreadNum; ++i)
    {
        _communicatorEpoll[i]->getObjectProxyFactory()->loadObjectLocator();
    }

    int iReportInterval = TC_Common::strto<int>(getProperty("report-interval", "60000"));

    int iReportTimeout = TC_Common::strto<int>(getProperty("report-timeout", "5000"));

    // int iSampleRate = TC_Common::strto<int>(getProperty("sample-rate", "1000"));

    // int iMaxSampleCount = TC_Common::strto<int>(getProperty("max-sample-count", "100"));

    int iMaxReportSize = TC_Common::strto<int>(getProperty("max-report-size", "1400"));

    string statObj = getProperty("stat", "");

    string propertyObj = getProperty("property", "");



    sResult = "locator=" + getProperty("locator", "") + "\r\n" +
        "stat=" + statObj + "\r\n" + "property=" + propertyObj + "\r\n" +
        "report-interval=" + TC_Common::tostr(iReportInterval) + "\r\n" +
        "report-timeout=" + TC_Common::tostr(iReportTimeout) + "\r\n";
        // "sample-rate=" + TC_Common::tostr(iSampleRate) + "\r\n" +
        // "max-sample-count=" + TC_Common::tostr(iMaxSampleCount) + "\r\n";

    return 0;
}



vector<TC_Endpoint> Communicator::getEndpoint4All(const string & objName)
{
    ServantProxy *pServantProxy = getServantProxy(objName);
    return pServantProxy->getEndpoint4All();
}

string Communicator::getResourcesInfo()
{
	ostringstream os;
	for (size_t i = 0; i < _clientThreadNum; ++i)
	{
		os << "OUT_LINE===" << endl;
		os << _communicatorEpoll[i]->getResourcesInfo();
	}
	return os.str();
}

void Communicator::terminate()
{
    {
        TC_LockT<TC_ThreadRecMutex> lock(*this);

        if (_terminating)
            return;

        _terminating = true;

        if(_initialized)
        {
            for(size_t i = 0; i < _clientThreadNum; ++i)
            {
                _communicatorEpoll[i]->terminate();
            }



            for(size_t i = 0;i < _asyncThreadNum; ++i)
            {
                if(_asyncThread[i])
                {
                    if (_asyncThread[i]->isAlive())
                    {
                        _asyncThread[i]->terminate();
                        _asyncThread[i]->getThreadControl().join();
                    }

                    delete _asyncThread[i];
                    _asyncThread[i] = NULL;
                }
            }
            _asyncThread.clear();
        }
    }

    //把锁释放掉, 再来等待线程停止, 避免死锁
    //因为通信器线程运行过程中, 有可能加上上面那把锁
    if (_initialized)
    {
        for (size_t i = 0; i < _clientThreadNum; ++i)
        {
            _communicatorEpoll[i]->getThreadControl().join();
            delete _communicatorEpoll[i];
            _communicatorEpoll[i] = NULL;
        }

     

        if(_servantProxyFactory)
        {
            delete _servantProxyFactory;
            _servantProxyFactory = NULL; 
        }
    }
}


void Communicator::pushAsyncThreadQueue(ReqMessage * msg)
{
	{
		TC_LockT<TC_SpinLock> lock(_callbackLock);

		auto it = _callback.find(msg->request.sServantName);
		if (it != _callback.end()) {
			ReqMessagePtr msgPtr (msg);
			it->second(msgPtr);
			return;
		}
	}

	//先不考虑每个线程队列数目不一致的情况
	_asyncThread[(_asyncSeq++) % _asyncThreadNum]->push_back(msg);
}



ServantProxy * Communicator::getServantProxy(const string& objectName,const string& setName)
{
    Communicator::initialize();

    return _servantProxyFactory->getServantProxy(objectName,setName);
}



ServantProxyFactory* Communicator::servantProxyFactory()
{
    return _servantProxyFactory;
}
///////////////////////////////////////////////////////////////
}

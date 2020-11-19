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

#ifndef __TARS_ENDPOINT_MANAGER_H_
#define __TARS_ENDPOINT_MANAGER_H_

#include "client/EndpointInfo.h"
//#include "client/EndpointF.h"

#include "client/AppProtocol.h"
#include "client/util/tc_spin_lock.h"
#include "client/Message.h"


#include "client/ServantProxy.h"
//#include "client/Global.h"
#include <set>

using namespace std;

namespace horsedb
{
////////////////////////////////////////////////////////////////////////
/*
 * 获取路由的方式
 */
enum  GetEndpointType
{
    E_DEFAULT = 0,
    E_ALL = 1,
    E_SET = 2,
    E_STATION = 3
};



////////////////////////////////////////////////////////////////////////
/*
 * 框架内部的路由管理的实现类
 */
class EndpointManager  :public ServantProxyCallback
{
public:
    static const size_t iMinWeightLimit = 10;
    static const size_t iMaxWeightLimit = 100;

public:
    /*
     * 构造函数
     */
    EndpointManager(ObjectProxy* pObjectProxy, Communicator* pComm, const string & sObjName, bool bFirstNetThread, const string& setName = "");

    /*
     * 析构函数
     */
    virtual ~EndpointManager();

    /*
     * 重写基类的实现
     */
    void notifyEndpoints(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive, bool bSync = false);

    /**
     * 更新
     * @param active
     * @param inactive
     */
	void updateEndpoints(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive);

	/*
     * 重写基类的实现
     */
    void doNotify();

    /**
     * 根据请求策略从可用的服务列表选择一个服务节点
     */
    bool selectAdapterProxy(ReqMessage * msg, AdapterProxy * & pAdapterProxy);

    /**
     * 获取所有的服务节点
     */
    const vector<AdapterProxy*> & getAdapters()
    {
        return _vAllProxys;
    }

    bool getAdapterProxyByName(const string &name,AdapterProxy* &ap );
    void addEndpoints(const set<EndpointInfo> & active);

    void setEndpoints(const string & sEndpoints, set<EndpointInfo> & setEndpoints);
    void setObjName(const string & sObjName);
    bool init(const string & sObjName,const string & sLocator,const string& setName);


    virtual int onDispatch(ReqMessagePtr msg) {return 0;}
    void getEndpoints(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);
    //void notifyEndpoints(const set<EndpointInfo> & active,const set<EndpointInfo> & inactive,bool bNotify);

private:

    /*
     * 轮询选取一个结点
     */
    AdapterProxy * getNextValidProxy();

    /*
     * 根据hash值选取一个结点
     */
    AdapterProxy* getHashProxy(int64_t hashCode,  bool bConsistentHash = false);


    /*
     * 根据hash值按取模方式，从正常节点中选取一个结点
     */
    AdapterProxy* getHashProxyForNormal(int64_t hashCode);


protected:

    /*
     * 通信器
     */
    Communicator *            _communicator;

    /*
     * 是否第一个客户端网络线程
     * 若是，会对ip列表信息的写缓存
     */
    bool                      _firstNetThread;

    /*
     * 是否主动请求ip列表信息的接口的请求
     * 比如按idc获取某个obj的ip列表信息
     */
    bool                      _interfaceReq;

    /*
     * 是否直连后端服务
     */
    bool                      _direct;

    /*
     * 请求的后端服务的Obj对象名称
     */
    string                    _objName;

    /*
     * 指定set调用的setid,默认为空
     * 如果有值，则说明是指定set调用
     */
    string                    _invokeSetId;

    /*
     * 框架的主控地址
     */
    string                    _locator;



    /*
     * 数据是否有效,初始化的时候是无效的数据
     * 只有请求过主控或者从文件缓存加载的数据才是有效数据
     */
    bool                      _valid;

    
    /*
     * 活跃节点列表
     */
    set<EndpointInfo>         _activeEndpoints;

    /*
     * 不活跃节点列表
     */
    set<EndpointInfo>         _inactiveEndpoints;

    /**
     * 是否是root servant
     */
	bool                      _rootServant;

private:

    TC_SpinLock             _mutex;
    /*
     * ObjectProxy
     */
    ObjectProxy *                 _objectProxy;

    /*
     * 活跃的结点
     */
    vector<AdapterProxy*>         _activeProxys;
    
    /*
     * 部署的结点 活跃的
     */
    map<string,AdapterProxy*>     _regProxys;
    vector<AdapterProxy*>         _vRegProxys;

    /*
     * 所有曾经create的结点
     */
    map<string,AdapterProxy*>     _allProxys;
    vector<AdapterProxy*>         _vAllProxys;

    /*
     * 轮训访问_activeProxys的偏移
     */
    size_t                        _lastRoundPosition;

    /*
     * 节点信息是否有更新
     */
    bool                          _update;

    /*
     * 是否是第一次建立权重信息
     */
    bool                          _first;

    /**
     * 上次重新建立权重信息表的时间
     */
    int64_t                          _lastBuildWeightTime;

    /**
     * 负载值更新频率,单位毫秒
     */
    int32_t                          _updateWeightInterval;

    /**
     * 静态时，对应的节点路由选择
     */
    size_t                          _lastSWeightPosition;

    /**
     * 静态权重对应的节点路由缓存
     */
    vector<size_t>                  _staticRouterCache;

    /*
     * 静态权重的活跃节点
     */
    vector<AdapterProxy*>          _activeWeightProxy;

    /*
     * hash静态权重的路由缓存
     */
    vector<size_t>                  _hashStaticRouterCache;

    /*
     * hash静态权重的缓存节点
     */
    vector<AdapterProxy*>         _lastHashStaticProxys;


};

////////////////////////////////////////////////////////////////////////
/*
 * 对外按类型获取路由的实现类
 */
class EndpointThread  : public EndpointManager
{
public:
    /*
     * 构造函数
     */
    EndpointThread(Communicator* pComm, const string & sObjName, GetEndpointType type, const string & sSetName, bool bFirstNetThread = false);

    /*
     * 析构函数
     */
    ~EndpointThread(){};

    /*
     * 用EndpointInfo存在可用与不可用的节点信息
     */
    void getEndpoints(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);

    /*
     * 用TC_Endpoint存在可用与不可用的节点信息
     */
    void getTCEndpoints(vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint);

    /*
     * 重写基类的实现
     */
    void notifyEndpoints(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive, bool bSync);

    /*
     * 重写基类的实现
     */
    void doNotify()
    {
    }

private:

    /*
     * 更新缓存的ip列表信息
     */
    void update(const set<EndpointInfo> & active, const set<EndpointInfo> & inactive);

private:

    /*
     * 类型
     */
    GetEndpointType          _type;

    /*
     * Obj名称
     */
    string                   _name;

    /*
     * 锁
     */
    // TC_ThreadLock            _mutex;
    TC_SpinLock             _mutex;


    /*
     * 活跃的结点
     */
    vector<EndpointInfo>     _activeEndPoint;
    vector<TC_Endpoint>      _activeTCEndPoint;

    /*
     * 不活跃的结点
     */
    vector<EndpointInfo>     _inactiveEndPoint;
    vector<TC_Endpoint>      _inactiveTCEndPoint;
    
};

////////////////////////////////////////////////////////////////////////
/*
 * 对外获取路由请求的封装类
 */
class EndpointManagerThread
{
public:
    /*
     * 构造函数
     */
    EndpointManagerThread(Communicator *pComm, const string &sObjName);

    /*
     * 析构函数
     */
    ~EndpointManagerThread();

    /*
     * 按idc获取可用与不可用的结点
     */
    void getEndpoint(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);

    /*
     * 获取所有可用与不可用的结点
     */
    void getEndpointByAll(vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);

    /*
     * 根据set获取可用与不可用的结点
     */
    void getEndpointBySet(const string sName, vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);

    /*
     * 根据地区获取可用与不可用的结点
     */
    void getEndpointByStation(const string sName, vector<EndpointInfo> &activeEndPoint, vector<EndpointInfo> &inactiveEndPoint);

    /*
     * 按idc获取可用与不可用的结点
     */
    void getTCEndpoint( vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint);

    /*
     * 获取所有可用与不可用的结点
     */
    void getTCEndpointByAll(vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint);

    /*
     * 根据set获取可用与不可用的结点
     */
    void getTCEndpointBySet(const string sName, vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint);

    /*
     * 根据地区获取可用与不可用的结点
     */
    void getTCEndpointByStation(const string sName, vector<TC_Endpoint> &activeEndPoint, vector<TC_Endpoint> &inactiveEndPoint);

protected:

    /*
     * 根据type创建相应的EndpointThread
     */
    EndpointThread * getEndpointThread(GetEndpointType type, const string & sName);

private:
    /*
     * 通信器
     */
    Communicator *                 _communicator;

    /*
     * Obj名称
     */
    string                         _objName;

    /*
     * 锁
     */
    // TC_ThreadLock                  _mutex;
    TC_SpinLock                     _mutex;

    /*
     * 保存对象的map
     */
    map<string,EndpointThread*>    _info;
};

////////////////////////////////////////////////////////////////////////
}
#endif

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

#ifndef __TARS_COMMUNICATOR_H_
#define __TARS_COMMUNICATOR_H_

#include "util/tc_thread.h"

//#include "util/tc_singleton.h"
#include "client/Global.h"
#include "client/ServantProxy.h"
#include "client/ServantProxyFactory.h"
#include "client/ObjectProxyFactory.h"
#include "client/AsyncProcThread.h"
#include "client/CommunicatorEpoll.h"




//
//struct ssl_ctx_st;
//typedef struct ssl_ctx_st SSL_CTX;

namespace horsedb
{
////////////////////////////////////////////////////////////////////////
/**
 * 客户端配置
 */
struct ClientConfig
{
    /**
     * 客户端IP地址
     */
    static string          LocalIp;
    /**
     * 客户端模块名称
     */
    static string          ModuleName;
    /**
     * 客户端所有的IP地址
     */
    static set<string>     SetLocalIp;
   /**
   *客户端是否打开set分组
   */
   static bool             SetOpen;
   /**
   *客户端set分组
   */
   static string           SetDivision;

   /**
    * 客户端的版本号
    */
   static string           TarsVersion;
};

////////////////////////////////////////////////////////////////////////
/**
 * 通信器,用于创建和维护客户端proxy
 */
class  Communicator :  public TC_ThreadRecMutex
{
public:

	typedef std::function<void(ReqMessagePtr)> custom_callback;

   /**
    * 构造函数
    */
   Communicator();

    /**
     * 采用配置构造
     * @param conf
     * @param path
     */
    Communicator( const string& domain );

    /**
     * 析够
     * 析够时自动接收相关线程
     */
    ~Communicator();

public:
   /**
    * 生成代理
    * @param T
    * @param objectName
    * @param setName 指定set调用的setid
    * @return T
    */
   template<class T> T stringToProxy(const string& objectName, const string& setName = "")
   {
      T prx = NULL;

      stringToProxy<T>(objectName, prx, setName);
      return prx;

      //return stringToProxy<T>(objectName, prx, setName);
   }

   /**
    * 生成代理
    * @param T
    * @param objectName
    * @param setName 指定set调用的setid
    * @param proxy
    */
   	template<class T> void stringToProxy(const string& objectName, T& proxy, const string& setName = "")
   	{
        ServantProxy *pServantProxy = getServantProxy(objectName, setName);
        cout<<"after getServantProxy"<<endl;

        T prx((typename T::element_type *)(pServantProxy));//T prx(dynamic_cast<typename T::element_type *>(pServantProxy));
        proxy = prx;

        //return prx;
    }


    /*
     *获取客户端网络线程的个数
     */
    inline size_t getClientThreadNum()
    {
        return _clientThreadNum;
    }

    /*
     *获取客户端网络线程的对象
     */
    inline CommunicatorEpoll * getCommunicatorEpoll(size_t iNum)
    {
        assert(iNum<_clientThreadNum);
        return _communicatorEpoll[iNum];
    }

    /**
     * 获取属性
     * @param name
     * @param dft, 缺省值
     * @return string
     */
    string getProperty(const string& name, const string& dft = "");

    /**
     * 设置属性
     * @param properties
     */
    void setProperty(const map<string, string>& properties);

    /**
     * 设置某一个属性
     * @param name
     * @param value
     */
    void setProperty(const string& name, const string& value);

    /**
     * 设置属性
     * @param conf
     * @param path
     */
    void setProperty( const string& domain );

    /**
     * get servant property
     * @param sObj
     * @return
     */
	map<string, string> getServantProperty(const string &sObj);

	/**
	 * set servant property
	 * @param sObj
	 * @return
	 */
	void setServantProperty(const string &sObj, const string& name, const string& value);

	/**
	 * get servant property
	 * @param sObj
	 * @return
	 */
	string getServantProperty(const string &sObj, const string& name);

    int reloadProperty(string & sResult);

    /**
     * 获取obj对应可用ip port列表  如果启用分组,只返回同分组的服务端ip
     * @param sObjName
     * @return vector<TC_Endpoint>
     */
    vector<TC_Endpoint> getEndpoint(const string & objName);

   /**
    * 获取obj对应可用ip port列表 包括所有IDC的
    * @param sObjName
    * @return vector<TC_Endpoint>
    */
   vector<TC_Endpoint> getEndpoint4All(const string& objName);

    /**
     * 结束
     */
    void terminate();

    /**
     * 超时请求的回包回来后，是否打印超时的日志信息，AdapterProxy里用到
     */
    bool getTimeoutLogFlag() { return _timeoutLogFlag; }

    /**
     * 获取最小的超时时间
     */
    int64_t getMinTimeout() { return _minTimeout; }

    /**
     * get resource info
     * @return
     */
	string getResourcesInfo();

	/**
	 * 更新prx的端口
	 * @param prx
	 * @param active
	 * @param inactive
	 */
	void notifyUpdateEndpoints(const ServantPrx &prx, const set<EndpointInfo> & active,const set<EndpointInfo> & inactive);

protected:
    /**
     * 初始化
     */
    void initialize();

    /**
     * 是否析构中
     * @return bool
     */
    bool isTerminating();

    /**
     * 获取对象代理生成器
     * @return ServantProxyFactoryPtr
     */
    ServantProxyFactory * servantProxyFactory();

    /**
     * 获取通用对象
     * @param objectName
     * @param setName 指定set调用的setid
     * @return ServantPrx
     */
    ServantProxy * getServantProxy(const string& objectName,const string& setName="");

    /**
     * 数据加入到异步线程队列里面
     * @return
     */
    void pushAsyncThreadQueue(ReqMessage * msg);


	/**
     * 框架内部需要直接访问通信器的类
     */
    friend class AdapterProxy;

    friend class ServantProxy;

    friend class ObjectProxy;

    friend class ServantProxyFactory;

    friend class ObjectProxyFactory;

    friend class AsyncProcThread;

    friend class CommunicatorEpoll;

	friend class Transceiver;

protected:
    /**
     * 是否初始化
     */
    bool  _initialized;

    /**
     * 停止标志
     */
    bool  _terminating;

    /**
     * 客户端的属性配置
     */
    map<string, string>    _properties;

    /**
     * obj info
     */
    map<string, map<string, string>>   _objInfo;

	/**
     * ServantProxy代码的工厂类
     */
    ServantProxyFactory* _servantProxyFactory;

    /*
     * 网络线程数组
     */
    CommunicatorEpoll *    _communicatorEpoll[MAX_CLIENT_THREAD_NUM];

    /*
     * 网络线程数目
     */
    size_t                 _clientThreadNum;
  


    /*
     * 超时请求的回包回来后，是否打印超时的日志信息，AdapterProxy里用到
     */
    bool                   _timeoutLogFlag;

    /*
     * 最小的超时时间
     */
    int64_t                _minTimeout;



	/**
	 *
	 */
	TC_SpinLock                                    _callbackLock;
    /**
	 * callback
	 */
	unordered_map<string, custom_callback>         _callback;

    /*
     * 异步线程数组
     */
    //异步线程(跨通信器共享)
    vector<AsyncProcThread*> _asyncThread;//[MAX_THREAD_NUM];



    /*
     * 异步线程数目
     */
    size_t                 _asyncThreadNum;
    /*
     * 分发给异步线程的索引seq
     */
    size_t                 _asyncSeq = 0;



};
////////////////////////////////////////////////////////////////////////
}
#endif

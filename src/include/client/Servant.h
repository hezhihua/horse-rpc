#ifndef _SERVANT__
#define _SERVANT__

#include  "client/Current.h"
#include  "util/tc_singleton.h"
namespace horsedb {

class Servant {
    public:

    virtual int onDispatch(TarsCurrentPtr current, vector<char> &buffer) { return -1; }
};

typedef shared_ptr<Servant> ServantPtr;

class ServantManager:public TC_Singleton<ServantManager> {
    public:

    template<typename T> void add(const string &adapterName,const string &servantName) 
    {
        auto it=_mapServant.find(servantName);
        if ( it==_mapServant.end())
        {
            _mapServant[servantName]=  ServantPtr(new T());
        }

        _mapAdapter2Servant[adapterName]=servantName;

    }

    bool getServant(const string &name,ServantPtr &ptr)
    {
        auto it=_mapServant.find(name);
        if ( it!=_mapServant.end())
        {
            ptr=_mapServant[name];
            return false;
        }
        return false;
    }
    string getServantName(const string &adapterName)
    {
        
        return _mapAdapter2Servant[adapterName];
    }

    map<string,ServantPtr> _mapServant;
    map<string,string> _mapAdapter2Servant;
};

}



#endif



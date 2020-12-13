#ifndef __THREADSINGLETON_H__
#define __THREADSINGLETON_H__

#include "util/tc_singleton.h"

namespace horsedb{
    
template
<typename T,
 template <class> class CreatePolicy = horsedb::CreateUsingNew,
 template <class> class LifetimePolicy = horsedb::DefaultLifetime>
class ThreadSingleton
{
public:
    typedef T instance_type;
    typedef volatile T volatile_type;
    
    /**
     * @brief 获取实例
     *
     * @return T*
     */
    static T *getThreadInstance()
    {
        if( !_pInstance )
        {
            if( _destroyed )
            {
                LifetimePolicy<T>::deadReference();
                _destroyed = false;
            }
            
            _pInstance = CreatePolicy<T>::create();
            LifetimePolicy<T>::scheduleDestruction( ( T * )_pInstance, &destroySingleton );
        }
        
        return ( T * )_pInstance;
    }
    
    virtual ~ThreadSingleton() {};
    
protected:
    static void destroySingleton()
    {
        if( !_destroyed )
        {
            CreatePolicy<T>::destroy( ( T * )_pInstance );
            _pInstance = NULL;
            _destroyed = true;
        }
    }
    
protected:
    static __thread T *_pInstance;
    static __thread bool _destroyed;
    
protected:
    ThreadSingleton()
    {
    }
    ThreadSingleton( const ThreadSingleton & );
    ThreadSingleton &operator=( const ThreadSingleton & );
};

template <class T, template <class> class CreatePolicy, template <class> class LifetimePolicy>
__thread bool ThreadSingleton<T, CreatePolicy, LifetimePolicy>::_destroyed = false;

template <class T, template <class> class CreatePolicy, template <class> class LifetimePolicy>
__thread T *ThreadSingleton<T, CreatePolicy, LifetimePolicy>::_pInstance = NULL;


}



#endif
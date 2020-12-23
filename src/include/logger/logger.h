#ifndef _HORSEDB_LOGGER__
#define _HORSEDB_LOGGER__

#include <memory>
#include <string>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"

#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging
#include "spdlog/sinks/daily_file_sink.h" // support for rotating file logging

#include "spdlog/logger-inl.h"
#include "util/tc_define.h"
#include "util/tc_singleton.h"
using namespace std;


#define LOG_INFO(x)  Logger::getInstance()->getLogger()->info(__FILE__LINE__FUNCTION__+(x));
#define LOG_DEBUG(x) Logger::getInstance()->getLogger()->debug(__FILE__LINE__FUNCTION__+(x));
#define LOG_WARN(x) Logger::getInstance()->getLogger()->warn(__FILE__LINE__FUNCTION__+(x));
#define LOG_ERROR(x) Logger::getInstance()->getLogger()->error(__FILE__LINE__FUNCTION__+(x));

#define TLOGDEBUG(x)  do{stringstream ss; ss<<x;LOG_DEBUG(ss.str())}while(0);
#define TLOGINFO(x)  do{stringstream ss; ss<<x;LOG_INFO(ss.str())}while(0);
#define TLOGWARN(x)  do{stringstream ss; ss<<x;LOG_WARN(ss.str())}while(0);
#define TLOGERROR(x)  do{stringstream ss; ss<<x;LOG_ERROR(ss.str())}while(0);
#define TLOGTARS(x)  do{stringstream ss; ss<<x;LOG_INFO(string("[fram]") +ss.str())}while(0);

#define LOGRAFT_INFO(x)  Logger::getInstance()->getRaftLogger()->info(__FILE__LINE__FUNCTION__+(x));
#define LOGRAFT_DEBUG(x) Logger::getInstance()->getRaftLogger()->debug(__FILE__LINE__FUNCTION__+(x));
#define LOGRAFT_WARN(x) Logger::getInstance()->getRaftLogger()->warn(__FILE__LINE__FUNCTION__+(x));
#define LOGRAFT_ERROR(x) Logger::getInstance()->getRaftLogger()->error(__FILE__LINE__FUNCTION__+(x));

#define TLOGINFO_RAFT(x)  do{stringstream ss; ss<<x;LOGRAFT_INFO(ss.str())}while(0);
#define TLOGDEBUG_RAFT(x)  do{stringstream ss; ss<<x;LOGRAFT_DEBUG(ss.str())}while(0);
#define TLOGWARN_RAFT(x)  do{stringstream ss; ss<<x;LOGRAFT_WARN(ss.str())}while(0);
#define TLOGERROR_RAFT(x)  do{stringstream ss; ss<<x;LOGRAFT_ERROR(ss.str())}while(0);

namespace horsedb{

class Logger: public TC_Singleton<Logger>{

    public:

    void init(const string& sExeName,const string& sLogPathName,const string& sRaftLogPathName,const string& logLevel="info")
    {
        _sExeName=sExeName;
        _sLogPathName=sLogPathName;
        _sLogRaftPathName=sRaftLogPathName;
        _async_logger =  spdlog::rotating_logger_mt<spdlog::async_factory>(sExeName, sLogPathName, 1024 * 1024 * 5, 3);
        _async_raft_logger=  spdlog::rotating_logger_mt<spdlog::async_factory>(sExeName+"raft", sRaftLogPathName, 1024 * 1024 * 5, 3);
        if (logLevel=="debug")
        {
            spdlog::set_level(spdlog::level::debug);
        }
        else if (logLevel=="info")
        {
            spdlog::set_level(spdlog::level::info);
        }else
        {
            spdlog::set_level(spdlog::level::info);
        }
        
        spdlog::set_level(spdlog::level::debug);
	    //async_logger->flush_on(spdlog::level::info);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][t:%t][%L]%v");
        spdlog::flush_every(std::chrono::seconds(1));

    }

    const std::shared_ptr<spdlog::logger>& getLogger() const { return _async_logger;}
    const std::shared_ptr<spdlog::logger>& getRaftLogger() const { return _async_raft_logger;}


    
    std::shared_ptr<spdlog::logger> _async_logger;
    std::shared_ptr<spdlog::logger> _async_raft_logger;

    string _sExeName;
    string _sLogPathName;
    string _sLogRaftPathName;


};    




}


#endif



﻿
#include "client/util/tc_timer.h"


namespace horsedb
{

TC_Timer::~TC_Timer()
{
	stopTimer();
}

void TC_Timer::startTimer(int numThread)
{
	if (numThread <= 0)
		numThread = 1;

	_tpool.init(numThread + 1);
	_tpool.start();
	_tpool.exec(std::bind(&TC_Timer::run, this));
}

bool TC_Timer::isTerminate()
{
	return _terminate;
}

void TC_Timer::stopTimer()
{
	{
		std::unique_lock <std::mutex> lck(_mutex);
		_cond.notify_all();
	}

	if (!_terminate)
	{
		//modify by hzh：要设置为true才能stop，不然线程不会退出,stop会阻塞
		_terminate=true;
		_tpool.stop();
		
	}
	
	
}

void TC_Timer::erase(int64_t uniqId)
{
	std::lock_guard <std::mutex> lock(_mutex);

	auto it = _mapEvent.find(uniqId);

	if (it != _mapEvent.end()) {
		auto itEvent = _mapTimer.find(it->second->_fireMillseconds);

		if (itEvent != _mapTimer.end()) {
			itEvent->second.erase(uniqId);

			if (itEvent->second.empty()) {
				_mapTimer.erase(itEvent);
			}
		}
		_mapEvent.erase(it);
	}
}

int64_t TC_Timer::post(const shared_ptr<TC_Timer::Func> & event)
{
	std::unique_lock <std::mutex> lock(_mutex);

	int64_t uniqId = (int64_t)event.get();

	if(_mapEvent.find(uniqId) == _mapEvent.end()) {
		_mapEvent[uniqId] = event;
	}
    //cout << "post event! fire time:" << TC_Common::tm2str(event->_fireMillseconds/1000, "%Y-%m-%d %H:%M:%S")<<endl;
	_mapTimer[event->_fireMillseconds].insert(uniqId);
	_cond.notify_one();
	return uniqId;
}

void TC_Timer::fireEvent(const EVENT_SET & el)
{
	auto itList = el.begin();

	while (itList != el.end()) {
		shared_ptr<Func> func ;

		{
			std::lock_guard<std::mutex> lock(_mutex);

			auto it = _mapEvent.find(*itList);

			if (it != _mapEvent.end()) {

				func = it->second;

				_mapEvent.erase(it);
			}
		}

		if(func) {
			//执行具体事件对象
			_tpool.exec(func->_func);
		}

		++itList;
	}
}

void TC_Timer::run()
{
	while (!_terminate) 
    {
		try 
        {
			EVENT_SET el;

			{
				std::unique_lock <std::mutex> lock(_mutex);

				auto it = _mapTimer.begin();

				if (it != _mapTimer.end())
                {
					uint64_t ms = TC_TimeProvider::getInstance()->getNowMs();

                    //cout << "now ms:" << TC_Common::tm2str(ms / 1000, "%Y-%m-%d %H:%M:%S") << "| it first:" << TC_Common::tm2str(it->first / 1000,  "%Y-%m-%d %H:%M:%S") << endl;
					if (it->first <= ms) 
                    {
						//时间过了, 有事件需要触发了
						el.swap(it->second);
						_mapTimer.erase(it);
					}
					else 
                    {
						//时间还没到
						ms = it->first - ms;
						_cond.wait_for(lock, std::chrono::milliseconds(ms));
					}
				}
				else 
                {
					//没有任何事件, 等待一下
					_cond.wait_for(lock, std::chrono::milliseconds(1000));
				}
			}

			if (!el.empty()) 
            {
				//触发相关所有事件
				fireEvent(el);
			}

		}
		catch (exception & ex) {
			cerr << ex.what() << endl;
		}
	}
}
}



project(rpc)
cmake_minimum_required(VERSION 2.8)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -Wno-deprecated -Wall -pthread" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-builtin-macro-redefined -D__FILE__='\"$(notdir $(abspath $<))\"'")
message(STATUS "optional:-g -std=c++14 -Wno-deprecated -Wall -pthread") 

set(TARS_VERSION "2.4.8")
add_definitions(-DTARS_VERSION="${TARS_VERSION}")

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
#include(spdlogd)
include(cmake/thirdparty.cmake)

#target_link_libraries(horse-rpc ${SPDLOG_LIB})
#target_include_directories(horse-rpc PRIVATE ${SPDLOG_INCLUDE_DIR})


ENABLE_LANGUAGE( ASM)

   SET(RPC_SRCS  
	  src/util/tc_ex.cpp
	  src/util/tc_thread.cpp
	  src/util/tc_thread_cond.cpp
	  src/util/tc_thread_mutex.cpp
	  src/util/tc_common.cpp
	  src/util/tc_socket.cpp
	  src/util/tc_network_buffer.cpp
	  src/util/tc_epoller.cpp
	  src/util/tc_clientsocket.cpp
	  src/util/tc_timeprovider.cpp
	  src/util/tc_epoll_server.cpp
	  src/util/tc_http.cpp


	  src/client/util/tc_spin_lock.cpp
	  src/client/util/tc_timer.cpp
	  src/client/util/tc_thread_pool.cpp
	  src/client/util/tc_cron.cpp

	  src/client/make_x86_64_sysv_elf_gas.S
	  src/client/jump_x86_64_sysv_elf_gas.S
	  
	  src/client/CoroutineScheduler.cpp
	  src/client/Current.cpp
	  src/client/EndpointInfo.cpp
	  src/client/Transceiver.cpp
	  src/client/AdapterProxy.cpp
	  src/client/ObjectProxy.cpp
	  src/client/ObjectProxyFactory.cpp
	  src/client/ServantProxy.cpp
	  src/client/EndpointManager.cpp
	  src/client/Communicator.cpp
	  src/client/ServantHandle.cpp
	  
	  src/client/NetworkUtil.cpp
	  src/client/ServantProxyFactory.cpp
	  src/client/CommunicatorEpoll.cpp
	  src/client/AsyncProcThread.cpp
	  src/client/Global.cpp
	  src/client/AppProtocol.cpp
      )   





INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/include) 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party/spdlog/include)


#ADD_LIBRARY(horse-rpc SHARED ${RPC_SRCS} )
ADD_LIBRARY(horse-rpc STATIC ${RPC_SRCS})
# 指定静态库的输出名称
set_target_properties(horse-rpc PROPERTIES OUTPUT_NAME "horse-rpc")

set_target_properties(horse-rpc PROPERTIES CLEAN_DIRECT_OUTPUT 1)




SET (LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# 安装horse-rpc库和头文件
#SET (CMAKE_INSTALL_PREFIX  ${CMAKE_CURRENT_SOURCE_DIR}/rpc)
install(
	    TARGETS horse-rpc
        ARCHIVE DESTINATION lib
        )
install(DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/include DESTINATION ./ )
#install(DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/third-party/spdlog/include DESTINATION ./ )

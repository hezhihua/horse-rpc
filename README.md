# horse-rpc
horse-rpc是基于腾讯tars rpc框架基础上修改过来的  
1,去除了一些依赖类，比如用c++11的智能指针shared_ptr全部替换掉TC_AutoPtr  
2,去除了里面的多平台支持，只支持linux平台  
3,去除了框架里面的跟tars框架基础服务交互的代码  
4,日志库改用spdlog  

# 目录介绍
| 目录名称 | 功能 |
| ----- | ----- |
| src/util | 编写服务端用到的类 |
| client/util | 编写客户端远程调用用到的类 |

# 依赖环境
| 软件	 | 要求 |
| ----- | ----- |
| gcc版本 | 最好4.8或以上 |
| cmake版本 | 3.10及以上版本 |
| spdlog版本 | 1.8.1版本 |
# 编译和安装

1,git clone https://github.com/hezhihua/horse-rpc.git  --recursive  
2,cd horse-rpc  
3,mkdir build && cd build && cmake ..  
4,make  

# TODO   
1,tars2cpp根据tars协议文件生成的代码有些地方不满足需求,需要修改tars2cpp工具生成目标代码,避免每次手工修改  


# 学习和交流
QQ群:1124085420  

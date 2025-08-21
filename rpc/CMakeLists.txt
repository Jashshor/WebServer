cmake_minimum_required(VERSION 3.10)

# RPC模块
set(RPC_SOURCES
    RpcConfig.cpp
    RpcProtocol.cpp
    JsonProtocolHandler.cpp
    RpcServer.cpp
    RpcClient.cpp
)

set(RPC_HEADERS
    RpcConfig.h
    RpcProtocol.h
    JsonProtocolHandler.h
    RpcServer.h
    RpcClient.h
    RpcTestClient.h
)

# 创建RPC静态库
add_library(rpc_lib STATIC ${RPC_SOURCES})

# 设置包含目录
target_include_directories(rpc_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 链接依赖库
target_link_libraries(rpc_lib 
    libserver_base
    jsoncpp
    pthread
)

# 编译示例服务器
add_executable(echo_server examples/EchoServer.cpp)
target_link_libraries(echo_server 
    rpc_lib
    libserver_base
    jsoncpp
    pthread
)

# 编译测试客户端
add_executable(rpc_test_client examples/TestClient.cpp)
target_link_libraries(rpc_test_client 
    rpc_lib
    libserver_base
    jsoncpp
    pthread
)

# 设置编译选项
set_target_properties(rpc_lib PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(echo_server PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(rpc_test_client PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
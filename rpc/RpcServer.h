#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
#include "../EventLoop.h"
#include "../Server.h"
#include "RpcProtocol.h"
#include "RpcConfig.h"

// RPC方法处理函数类型
using RpcMethodHandler = std::function<std::string(const std::string&)>;

// RPC服务器类
class RpcServer {
public:
    RpcServer(EventLoop* loop, int port);
    ~RpcServer();
    
    // 启动RPC服务器
    void start();
    
    // 停止RPC服务器
    void stop();
    
    // 注册RPC方法
    void registerMethod(const std::string& methodName, RpcMethodHandler handler);
    
    // 取消注册RPC方法
    void unregisterMethod(const std::string& methodName);
    
    // 设置协议处理器
    void setProtocolHandler(std::unique_ptr<RpcProtocolHandler> handler);
    
    // 获取统计信息
    struct Statistics {
        uint64_t totalRequests = 0;
        uint64_t successRequests = 0;
        uint64_t errorRequests = 0;
        uint64_t timeoutRequests = 0;
        double avgResponseTime = 0.0;
    };
    
    Statistics getStatistics() const { return statistics_; }

private:
    EventLoop* loop_;
    std::unique_ptr<Server> server_;
    std::unique_ptr<RpcProtocolHandler> protocolHandler_;
    std::unordered_map<std::string, RpcMethodHandler> methods_;
    
    // 统计信息
    mutable Statistics statistics_;
    
    // 处理新连接
    void handleNewConnection(std::shared_ptr<Channel> channel);
    
    // 处理RPC请求
    void handleRpcRequest(std::shared_ptr<Channel> channel, const std::string& data);
    
    // 发送RPC响应
    void sendRpcResponse(std::shared_ptr<Channel> channel, const RpcResponse& response);
    
    // 处理方法调用
    RpcResponse processMethodCall(const RpcRequest& request);
    
    // 更新统计信息
    void updateStatistics(bool success, double responseTime);
};
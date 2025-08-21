#pragma once
#include <memory>
#include <functional>
#include <future>
#include <unordered_map>
#include <atomic>
#include "../EventLoop.h"
#include "../Channel.h"
#include "RpcProtocol.h"

// RPC调用回调函数类型
using RpcCallback = std::function<void(const RpcResponse&)>;

// RPC客户端类
class RpcClient {
public:
    RpcClient(EventLoop* loop, const std::string& serverHost, int serverPort);
    ~RpcClient();
    
    // 连接到服务器
    bool connect();
    
    // 断开连接
    void disconnect();
    
    // 同步调用RPC方法
    RpcResponse call(const std::string& method, const std::string& params, 
                     uint32_t timeout = 5000);
    
    // 异步调用RPC方法
    void asyncCall(const std::string& method, const std::string& params, 
                   RpcCallback callback, uint32_t timeout = 5000);
    
    // 发送通知（不需要响应）
    void notify(const std::string& method, const std::string& params);
    
    // 设置协议处理器
    void setProtocolHandler(std::unique_ptr<RpcProtocolHandler> handler);
    
    // 检查连接状态
    bool isConnected() const { return connected_; }
    
    // 获取统计信息
    struct Statistics {
        uint64_t totalCalls = 0;
        uint64_t successCalls = 0;
        uint64_t errorCalls = 0;
        uint64_t timeoutCalls = 0;
        double avgResponseTime = 0.0;
    };
    
    Statistics getStatistics() const { return statistics_; }

private:
    EventLoop* loop_;
    std::string serverHost_;
    int serverPort_;
    int sockfd_;
    bool connected_;
    
    std::shared_ptr<Channel> channel_;
    std::unique_ptr<RpcProtocolHandler> protocolHandler_;
    
    // 消息ID生成器
    std::atomic<uint32_t> messageIdGenerator_;
    
    // 待处理的请求
    struct PendingRequest {
        std::promise<RpcResponse> promise;
        RpcCallback callback;
        std::chrono::steady_clock::time_point startTime;
        uint32_t timeout;
        bool isAsync;
        
        PendingRequest() : isAsync(false) {}
    };
    
    std::unordered_map<uint32_t, std::unique_ptr<PendingRequest>> pendingRequests_;
    mutable std::mutex pendingMutex_;
    
    // 统计信息
    mutable Statistics statistics_;
    
    // 生成消息ID
    uint32_t generateMessageId();
    
    // 处理接收到的数据
    void handleRead();
    
    // 处理连接事件
    void handleConnection();
    
    // 处理错误
    void handleError();
    
    // 发送请求
    bool sendRequest(const RpcRequest& request);
    
    // 处理响应
    void handleResponse(const RpcResponse& response);
    
    // 处理超时
    void handleTimeout();
    
    // 更新统计信息
    void updateStatistics(bool success, double responseTime);
    
    // 清理超时请求
    void cleanupTimeoutRequests();
};
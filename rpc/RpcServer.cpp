#include "RpcServer.h"
#include "JsonProtocolHandler.h"
#include "../base/Logging.h"
#include <chrono>

RpcServer::RpcServer(EventLoop* loop, int port) 
    : loop_(loop), server_(std::make_unique<Server>(loop, 4, port)) {
    
    // 设置默认协议处理器
    protocolHandler_ = std::make_unique<JsonProtocolHandler>();
    
    LOG << "RpcServer created on port " << port;
}

RpcServer::~RpcServer() {
    stop();
}

void RpcServer::start() {
    LOG << "Starting RPC Server...";
    server_->start();
}

void RpcServer::stop() {
    LOG << "Stopping RPC Server...";
    // 服务器停止逻辑
}

void RpcServer::registerMethod(const std::string& methodName, RpcMethodHandler handler) {
    methods_[methodName] = handler;
    LOG << "Registered RPC method: " << methodName;
}

void RpcServer::unregisterMethod(const std::string& methodName) {
    auto it = methods_.find(methodName);
    if (it != methods_.end()) {
        methods_.erase(it);
        LOG << "Unregistered RPC method: " << methodName;
    }
}

void RpcServer::setProtocolHandler(std::unique_ptr<RpcProtocolHandler> handler) {
    protocolHandler_ = std::move(handler);
    LOG << "Protocol handler updated";
}

void RpcServer::handleNewConnection(std::shared_ptr<Channel> channel) {
    // 设置读取回调处理RPC请求
    channel->setReadHandler([this, channel]() {
        // 读取数据并处理RPC请求
        // 这里需要与现有的HttpData集成
    });
}

void RpcServer::handleRpcRequest(std::shared_ptr<Channel> channel, const std::string& data) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 解码请求
    auto request = protocolHandler_->decodeRequest(data);
    if (!request) {
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::PARSE_ERROR, "Failed to parse request");
        sendRpcResponse(channel, errorResponse);
        updateStatistics(false, 0.0);
        return;
    }
    
    // 处理方法调用
    RpcResponse response = processMethodCall(*request);
    
    // 发送响应
    sendRpcResponse(channel, response);
    
    // 更新统计信息
    auto endTime = std::chrono::steady_clock::now();
    double responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    updateStatistics(response.isSuccess(), responseTime);
}

void RpcServer::sendRpcResponse(std::shared_ptr<Channel> channel, const RpcResponse& response) {
    std::string encodedResponse = protocolHandler_->encodeResponse(response);
    
    // 通过channel发送响应
    // 这里需要与现有的网络层集成
    // channel->send(encodedResponse);
}

RpcResponse RpcServer::processMethodCall(const RpcRequest& request) {
    RpcResponse response;
    response.setMessageId(request.getMessageId());
    
    auto it = methods_.find(request.getMethod());
    if (it == methods_.end()) {
        response.setError(RpcErrorCode::METHOD_NOT_FOUND, 
                         "Method '" + request.getMethod() + "' not found");
        return response;
    }
    
    try {
        std::string result = it->second(request.getParams());
        response.setResult(result);
    } catch (const std::exception& e) {
        response.setError(RpcErrorCode::INTERNAL_ERROR, e.what());
    }
    
    return response;
}

void RpcServer::updateStatistics(bool success, double responseTime) {
    statistics_.totalRequests++;
    if (success) {
        statistics_.successRequests++;
    } else {
        statistics_.errorRequests++;
    }
    
    // 更新平均响应时间
    double totalTime = statistics_.avgResponseTime * (statistics_.totalRequests - 1) + responseTime;
    statistics_.avgResponseTime = totalTime / statistics_.totalRequests;
}
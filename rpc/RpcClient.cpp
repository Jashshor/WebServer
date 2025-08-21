#include "RpcClient.h"
#include "JsonProtocolHandler.h"
#include "../base/Logging.h"
#include "../Util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <mutex>

RpcClient::RpcClient(EventLoop* loop, const std::string& serverHost, int serverPort)
    : loop_(loop), serverHost_(serverHost), serverPort_(serverPort), 
      sockfd_(-1), connected_(false), messageIdGenerator_(1) {
    
    // 设置默认协议处理器
    protocolHandler_ = std::make_unique<JsonProtocolHandler>();
}

RpcClient::~RpcClient() {
    disconnect();
}

bool RpcClient::connect() {
    if (connected_) {
        return true;
    }
    
    // 创建socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        LOG << "Failed to create socket";
        return false;
    }
    
    // 设置服务器地址
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort_);
    if (inet_pton(AF_INET, serverHost_.c_str(), &serverAddr.sin_addr) <= 0) {
        LOG << "Invalid server address: " << serverHost_;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    // 连接到服务器
    if (::connect(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG << "Failed to connect to server " << serverHost_ << ":" << serverPort_;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    // 设置非阻塞模式
    setSocketNonBlocking(sockfd_);
    
    // 创建Channel并设置回调
    channel_ = std::make_shared<Channel>(loop_, sockfd_);
    channel_->setReadHandler([this]() { handleRead(); });
    channel_->setConnHandler([this]() { handleConnection(); });
    channel_->setErrorHandler([this]() { handleError(); });
    
    // 添加到事件循环
    loop_->addToPoller(channel_);
    
    connected_ = true;
    LOG << "Connected to RPC server " << serverHost_ << ":" << serverPort_;
    
    return true;
}

void RpcClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    connected_ = false;
    
    if (channel_) {
        loop_->removeFromPoller(channel_);
        channel_.reset();
    }
    
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    
    // 清理待处理的请求
    std::lock_guard<std::mutex> lock(pendingMutex_);
    for (auto& pair : pendingRequests_) {
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::NETWORK_ERROR, "Connection closed");
        
        if (pair.second->isAsync && pair.second->callback) {
            pair.second->callback(errorResponse);
        } else {
            pair.second->promise.set_value(errorResponse);
        }
    }
    pendingRequests_.clear();
    
    LOG << "Disconnected from RPC server";
}

RpcResponse RpcClient::call(const std::string& method, const std::string& params, uint32_t timeout) {
    if (!connected_ && !connect()) {
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::NETWORK_ERROR, "Not connected to server");
        return errorResponse;
    }
    
    RpcRequest request(method, params);
    request.setMessageId(generateMessageId());
    request.setTimeout(timeout);
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 创建待处理请求
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->startTime = startTime;
    pendingRequest->timeout = timeout;
    pendingRequest->isAsync = false;
    
    auto future = pendingRequest->promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequests_[request.getMessageId()] = std::move(pendingRequest);
    }
    
    // 发送请求
    if (!sendRequest(request)) {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequests_.erase(request.getMessageId());
        
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::NETWORK_ERROR, "Failed to send request");
        return errorResponse;
    }
    
    // 等待响应
    auto status = future.wait_for(std::chrono::milliseconds(timeout));
    if (status == std::future_status::timeout) {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequests_.erase(request.getMessageId());
        
        RpcResponse timeoutResponse;
        timeoutResponse.setError(RpcErrorCode::TIMEOUT_ERROR, "Request timeout");
        updateStatistics(false, timeout);
        return timeoutResponse;
    }
    
    RpcResponse response = future.get();
    
    // 更新统计信息
    auto endTime = std::chrono::steady_clock::now();
    double responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    updateStatistics(response.isSuccess(), responseTime);
    
    return response;
}

void RpcClient::asyncCall(const std::string& method, const std::string& params, 
                         RpcCallback callback, uint32_t timeout) {
    if (!connected_ && !connect()) {
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::NETWORK_ERROR, "Not connected to server");
        if (callback) callback(errorResponse);
        return;
    }
    
    RpcRequest request(method, params);
    request.setMessageId(generateMessageId());
    request.setTimeout(timeout);
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 创建待处理请求
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->startTime = startTime;
    pendingRequest->timeout = timeout;
    pendingRequest->isAsync = true;
    pendingRequest->callback = callback;
    
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequests_[request.getMessageId()] = std::move(pendingRequest);
    }
    
    // 发送请求
    if (!sendRequest(request)) {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequests_.erase(request.getMessageId());
        
        RpcResponse errorResponse;
        errorResponse.setError(RpcErrorCode::NETWORK_ERROR, "Failed to send request");
        if (callback) callback(errorResponse);
        return;
    }
    
    // 启动超时检查
    std::thread([this, messageId = request.getMessageId(), timeout]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        handleTimeout();
    }).detach();
}

void RpcClient::notify(const std::string& method, const std::string& params) {
    if (!connected_ && !connect()) {
        LOG << "Failed to send notification: not connected";
        return;
    }
    
    RpcRequest request(method, params);
    request.setMessageId(0);  // 通知消息ID为0
    
    sendRequest(request);
}

uint32_t RpcClient::generateMessageId() {
    return messageIdGenerator_.fetch_add(1);
}

bool RpcClient::sendRequest(const RpcRequest& request) {
    std::string encodedRequest = protocolHandler_->encodeRequest(request);
    
    ssize_t sent = writen(sockfd_, encodedRequest.c_str(), encodedRequest.length());
    return sent == static_cast<ssize_t>(encodedRequest.length());
}

void RpcClient::handleRead() {
    char buffer[8192];
    ssize_t n = readn(sockfd_, buffer, sizeof(buffer));
    
    if (n <= 0) {
        handleError();
        return;
    }
    
    std::string data(buffer, n);
    auto response = protocolHandler_->decodeResponse(data);
    
    if (response) {
        handleResponse(*response);
    }
}

void RpcClient::handleResponse(const RpcResponse& response) {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    
    auto it = pendingRequests_.find(response.getMessageId());
    if (it == pendingRequests_.end()) {
        return;  // 未找到对应的请求
    }
    
    auto& pendingRequest = it->second;
    
    // 计算响应时间
    auto endTime = std::chrono::steady_clock::now();
    double responseTime = std::chrono::duration<double, std::milli>(
        endTime - pendingRequest->startTime).count();
    
    if (pendingRequest->isAsync && pendingRequest->callback) {
        pendingRequest->callback(response);
    } else {
        pendingRequest->promise.set_value(response);
    }
    
    pendingRequests_.erase(it);
    updateStatistics(response.isSuccess(), responseTime);
}

void RpcClient::handleConnection() {
    // 连接状态处理
}

void RpcClient::handleError() {
    LOG << "RPC client connection error";
    disconnect();
}

void RpcClient::handleTimeout() {
    cleanupTimeoutRequests();
}

void RpcClient::updateStatistics(bool success, double responseTime) {
    statistics_.totalCalls++;
    if (success) {
        statistics_.successCalls++;
    } else {
        statistics_.errorCalls++;
    }
    
    // 更新平均响应时间
    double totalTime = statistics_.avgResponseTime * (statistics_.totalCalls - 1) + responseTime;
    statistics_.avgResponseTime = totalTime / statistics_.totalCalls;
}

void RpcClient::cleanupTimeoutRequests() {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = pendingRequests_.begin(); it != pendingRequests_.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - it->second->startTime).count();
        
        if (elapsed > it->second->timeout) {
            RpcResponse timeoutResponse;
            timeoutResponse.setMessageId(it->first);
            timeoutResponse.setError(RpcErrorCode::TIMEOUT_ERROR, "Request timeout");
            
            if (it->second->isAsync && it->second->callback) {
                it->second->callback(timeoutResponse);
            } else {
                it->second->promise.set_value(timeoutResponse);
            }
            
            it = pendingRequests_.erase(it);
            statistics_.timeoutCalls++;
        } else {
            ++it;
        }
    }
}

void RpcClient::setProtocolHandler(std::unique_ptr<RpcProtocolHandler> handler) {
    protocolHandler_ = std::move(handler);
}
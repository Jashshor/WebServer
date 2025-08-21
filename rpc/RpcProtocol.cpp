#include "RpcProtocol.h"
#include "JsonProtocolHandler.h"
#include <sstream>
#include "../base/Logging.h"

// RpcRequest实现
std::string RpcRequest::serialize() const {
    std::ostringstream oss;
    oss << "{"
        << "\"messageId\":" << messageId_ << ","
        << "\"method\":\"" << method_ << "\","
        << "\"params\":\"" << params_ << "\","
        << "\"timeout\":" << timeout_
        << "}";
    return oss.str();
}

bool RpcRequest::deserialize(const std::string& data) {
    // 简单的JSON解析实现
    size_t pos = 0;
    
    // 解析messageId
    pos = data.find("\"messageId\":");
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = data.find(",", pos);
        if (end != std::string::npos) {
            messageId_ = std::stoul(data.substr(pos, end - pos));
        }
    }
    
    // 解析method
    pos = data.find("\"method\":\"");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = data.find("\"", pos);
        if (end != std::string::npos) {
            method_ = data.substr(pos, end - pos);
        }
    }
    
    // 解析params
    pos = data.find("\"params\":\"");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = data.find("\"", pos);
        if (end != std::string::npos) {
            params_ = data.substr(pos, end - pos);
        }
    }
    
    // 解析timeout
    pos = data.find("\"timeout\":");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = data.find("}", pos);
        if (end != std::string::npos) {
            timeout_ = std::stoul(data.substr(pos, end - pos));
        }
    }
    
    return true;
}

// RpcResponse实现
std::string RpcResponse::serialize() const {
    std::ostringstream oss;
    oss << "{"
        << "\"messageId\":" << messageId_ << ","
        << "\"result\":\"" << result_ << "\","
        << "\"errorCode\":" << static_cast<int>(errorCode_) << ","
        << "\"errorMessage\":\"" << errorMessage_ << "\""
        << "}";
    return oss.str();
}

bool RpcResponse::deserialize(const std::string& data) {
    size_t pos = 0;
    
    // 解析messageId
    pos = data.find("\"messageId\":");
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = data.find(",", pos);
        if (end != std::string::npos) {
            messageId_ = std::stoul(data.substr(pos, end - pos));
        }
    }
    
    // 解析result
    pos = data.find("\"result\":\"");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = data.find("\"", pos);
        if (end != std::string::npos) {
            result_ = data.substr(pos, end - pos);
        }
    }
    
    // 解析errorCode
    pos = data.find("\"errorCode\":");
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = data.find(",", pos);
        if (end != std::string::npos) {
            errorCode_ = static_cast<RpcErrorCode>(std::stoi(data.substr(pos, end - pos)));
        }
    }
    
    // 解析errorMessage
    pos = data.find("\"errorMessage\":\"");
    if (pos != std::string::npos) {
        pos += 16;
        size_t end = data.find("\"", pos);
        if (end != std::string::npos) {
            errorMessage_ = data.substr(pos, end - pos);
        }
    }
    
    return true;
}

// RpcProtocolFactory实现
std::unique_ptr<RpcProtocolHandler> RpcProtocolFactory::createHandler(RpcProtocolType type) {
    switch (type) {
        case RpcProtocolType::JSON:
            return std::make_unique<JsonProtocolHandler>();
        case RpcProtocolType::PROTOBUF:
            // TODO: 实现Protobuf处理器
            LOG << "Protobuf protocol handler not implemented yet";
            return nullptr;
        case RpcProtocolType::MSGPACK:
            // TODO: 实现MessagePack处理器
            LOG << "MessagePack protocol handler not implemented yet";
            return nullptr;
        case RpcProtocolType::CUSTOM:
            // TODO: 支持自定义协议处理器
            LOG << "Custom protocol handler not implemented yet";
            return nullptr;
        default:
            LOG << "Unknown protocol type: " << static_cast<int>(type);
            return nullptr;
    }
}
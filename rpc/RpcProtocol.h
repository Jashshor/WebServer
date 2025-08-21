#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "../base/noncopyable.h"
#include "RpcConfig.h"

// RPC错误码定义
enum class RpcErrorCode : int32_t {
    SUCCESS = 0,
    INVALID_REQUEST = -1,
    METHOD_NOT_FOUND = -2,
    INVALID_PARAMS = -3,
    INTERNAL_ERROR = -4,
    PARSE_ERROR = -5,
    TIMEOUT_ERROR = -6,
    NETWORK_ERROR = -7,
    SERIALIZE_ERROR = -8,
    DESERIALIZE_ERROR = -9,
    CUSTOM_ERROR = -100
};

// RPC消息类型
enum class RpcMessageType : uint8_t {
    REQUEST = 1,
    RESPONSE = 2,
    NOTIFICATION = 3,
    HEARTBEAT = 4
};

// RPC消息头结构
struct RpcMessageHeader {
    uint32_t magic;          // 魔数，用于协议识别
    uint32_t version;        // 协议版本
    RpcMessageType type;     // 消息类型
    uint32_t messageId;      // 消息ID
    uint32_t bodyLength;     // 消息体长度
    uint32_t checksum;       // 校验和
    uint64_t timestamp;      // 时间戳
    
    RpcMessageHeader() 
        : magic(0x12345678), version(1), type(RpcMessageType::REQUEST),
          messageId(0), bodyLength(0), checksum(0), timestamp(0) {}
};

// RPC请求消息
class RpcRequest {
public:
    RpcRequest() = default;
    RpcRequest(const std::string& method, const std::string& params)
        : method_(method), params_(params) {}
    
    void setMessageId(uint32_t id) { messageId_ = id; }
    void setMethod(const std::string& method) { method_ = method; }
    void setParams(const std::string& params) { params_ = params; }
    void setTimeout(uint32_t timeout) { timeout_ = timeout; }
    
    uint32_t getMessageId() const { return messageId_; }
    const std::string& getMethod() const { return method_; }
    const std::string& getParams() const { return params_; }
    uint32_t getTimeout() const { return timeout_; }
    
    // 序列化和反序列化接口
    virtual std::string serialize() const;
    virtual bool deserialize(const std::string& data);

private:
    uint32_t messageId_ = 0;
    std::string method_;
    std::string params_;
    uint32_t timeout_ = 5000;  // 默认5秒超时
};

// RPC响应消息
class RpcResponse {
public:
    RpcResponse() = default;
    RpcResponse(uint32_t messageId, const std::string& result)
        : messageId_(messageId), result_(result), errorCode_(RpcErrorCode::SUCCESS) {}
    
    void setMessageId(uint32_t id) { messageId_ = id; }
    void setResult(const std::string& result) { result_ = result; }
    void setError(RpcErrorCode code, const std::string& message) {
        errorCode_ = code;
        errorMessage_ = message;
    }
    
    uint32_t getMessageId() const { return messageId_; }
    const std::string& getResult() const { return result_; }
    RpcErrorCode getErrorCode() const { return errorCode_; }
    const std::string& getErrorMessage() const { return errorMessage_; }
    
    bool isSuccess() const { return errorCode_ == RpcErrorCode::SUCCESS; }
    
    // 序列化和反序列化接口
    virtual std::string serialize() const;
    virtual bool deserialize(const std::string& data);

private:
    uint32_t messageId_ = 0;
    std::string result_;
    RpcErrorCode errorCode_ = RpcErrorCode::SUCCESS;
    std::string errorMessage_;
};

// RPC协议处理器基类
class RpcProtocolHandler : noncopyable {
public:
    virtual ~RpcProtocolHandler() = default;
    
    // 编码请求
    virtual std::string encodeRequest(const RpcRequest& request) = 0;
    
    // 编码响应
    virtual std::string encodeResponse(const RpcResponse& response) = 0;
    
    // 解码请求
    virtual std::unique_ptr<RpcRequest> decodeRequest(const std::string& data) = 0;
    
    // 解码响应
    virtual std::unique_ptr<RpcResponse> decodeResponse(const std::string& data) = 0;
    
    // 验证消息完整性
    virtual bool validateMessage(const std::string& data) = 0;
    
    // 计算校验和
    virtual uint32_t calculateChecksum(const std::string& data) = 0;
};

// 协议工厂类
class RpcProtocolFactory {
public:
    static std::unique_ptr<RpcProtocolHandler> createHandler(RpcProtocolType type);
};
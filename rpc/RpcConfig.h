#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "base/noncopyable.h"

// RPC协议类型枚举
enum class RpcProtocolType {
    JSON = 0,
    PROTOBUF = 1,
    MSGPACK = 2,
    CUSTOM = 3
};

// RPC序列化方式枚举
enum class RpcSerializeType {
    JSON_SERIALIZE = 0,
    BINARY_SERIALIZE = 1,
    XML_SERIALIZE = 2,
    CUSTOM_SERIALIZE = 3
};

// RPC传输层类型枚举
enum class RpcTransportType {
    TCP = 0,
    UDP = 1,
    HTTP = 2,
    WEBSOCKET = 3
};

// RPC配置类
class RpcConfig : noncopyable {
public:
    static RpcConfig& getInstance();
    
    // 加载配置文件
    bool loadConfig(const std::string& configFile);
    
    // 获取配置项
    RpcProtocolType getProtocolType() const { return protocolType_; }
    RpcSerializeType getSerializeType() const { return serializeType_; }
    RpcTransportType getTransportType() const { return transportType_; }
    
    int getPort() const { return port_; }
    int getThreadNum() const { return threadNum_; }
    int getTimeoutMs() const { return timeoutMs_; }
    int getMaxConnections() const { return maxConnections_; }
    
    const std::string& getLogLevel() const { return logLevel_; }
    const std::string& getLogPath() const { return logPath_; }
    
    // 自定义协议配置
    const std::unordered_map<std::string, std::string>& getCustomConfig() const {
        return customConfig_;
    }
    
    std::string getCustomConfig(const std::string& key) const;

private:
    RpcConfig() = default;
    
    RpcProtocolType protocolType_ = RpcProtocolType::JSON;
    RpcSerializeType serializeType_ = RpcSerializeType::JSON_SERIALIZE;
    RpcTransportType transportType_ = RpcTransportType::TCP;
    
    int port_ = 8080;
    int threadNum_ = 4;
    int timeoutMs_ = 5000;
    int maxConnections_ = 1000;
    
    std::string logLevel_ = "INFO";
    std::string logPath_ = "./logs/";
    
    std::unordered_map<std::string, std::string> customConfig_;
};
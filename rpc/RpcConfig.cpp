#include "RpcConfig.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "base/Logging.h"

RpcConfig& RpcConfig::getInstance() {
    static RpcConfig instance;
    return instance;
}

bool RpcConfig::loadConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        LOG << "Failed to open config file: " << configFile;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // 去除空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // 解析配置项
        if (key == "protocol_type") {
            if (value == "JSON") protocolType_ = RpcProtocolType::JSON;
            else if (value == "PROTOBUF") protocolType_ = RpcProtocolType::PROTOBUF;
            else if (value == "MSGPACK") protocolType_ = RpcProtocolType::MSGPACK;
            else if (value == "CUSTOM") protocolType_ = RpcProtocolType::CUSTOM;
        } else if (key == "serialize_type") {
            if (value == "JSON") serializeType_ = RpcSerializeType::JSON_SERIALIZE;
            else if (value == "BINARY") serializeType_ = RpcSerializeType::BINARY_SERIALIZE;
            else if (value == "XML") serializeType_ = RpcSerializeType::XML_SERIALIZE;
            else if (value == "CUSTOM") serializeType_ = RpcSerializeType::CUSTOM_SERIALIZE;
        } else if (key == "transport_type") {
            if (value == "TCP") transportType_ = RpcTransportType::TCP;
            else if (value == "UDP") transportType_ = RpcTransportType::UDP;
            else if (value == "HTTP") transportType_ = RpcTransportType::HTTP;
            else if (value == "WEBSOCKET") transportType_ = RpcTransportType::WEBSOCKET;
        } else if (key == "port") {
            port_ = std::stoi(value);
        } else if (key == "thread_num") {
            threadNum_ = std::stoi(value);
        } else if (key == "timeout_ms") {
            timeoutMs_ = std::stoi(value);
        } else if (key == "max_connections") {
            maxConnections_ = std::stoi(value);
        } else if (key == "log_level") {
            logLevel_ = value;
        } else if (key == "log_path") {
            logPath_ = value;
        } else {
            // 自定义配置项
            customConfig_[key] = value;
        }
    }
    
    LOG << "RPC Config loaded successfully";
    return true;
}

std::string RpcConfig::getCustomConfig(const std::string& key) const {
    auto it = customConfig_.find(key);
    return it != customConfig_.end() ? it->second : "";
}
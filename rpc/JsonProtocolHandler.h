#pragma once
#include "RpcProtocol.h"
#include <json/json.h>

class JsonProtocolHandler : public RpcProtocolHandler {
public:
    JsonProtocolHandler() = default;
    ~JsonProtocolHandler() override = default;
    
    std::string encodeRequest(const RpcRequest& request) override;
    std::string encodeResponse(const RpcResponse& response) override;
    std::unique_ptr<RpcRequest> decodeRequest(const std::string& data) override;
    std::unique_ptr<RpcResponse> decodeResponse(const std::string& data) override;
    bool validateMessage(const std::string& data) override;
    uint32_t calculateChecksum(const std::string& data) override;

private:
    Json::Value parseJson(const std::string& data);
    std::string jsonToString(const Json::Value& json);
    RpcMessageHeader parseHeader(const std::string& data);
    std::string createHeader(const RpcMessageHeader& header);
};
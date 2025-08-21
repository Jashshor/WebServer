#include "JsonProtocolHandler.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include "../base/Logging.h"

std::string JsonProtocolHandler::encodeRequest(const RpcRequest& request) {
    Json::Value root;
    root["jsonrpc"] = "2.0";
    root["method"] = request.getMethod();
    root["id"] = request.getMessageId();
    
    // 解析参数
    if (!request.getParams().empty()) {
        Json::Value params;
        Json::Reader reader;
        if (reader.parse(request.getParams(), params)) {
            root["params"] = params;
        } else {
            root["params"] = request.getParams();
        }
    }
    
    std::string body = jsonToString(root);
    
    // 创建消息头
    RpcMessageHeader header;
    header.type = RpcMessageType::REQUEST;
    header.messageId = request.getMessageId();
    header.bodyLength = body.length();
    header.checksum = calculateChecksum(body);
    header.timestamp = time(nullptr);
    
    return createHeader(header) + body;
}

std::string JsonProtocolHandler::encodeResponse(const RpcResponse& response) {
    Json::Value root;
    root["jsonrpc"] = "2.0";
    root["id"] = response.getMessageId();
    
    if (response.isSuccess()) {
        Json::Value result;
        Json::Reader reader;
        if (reader.parse(response.getResult(), result)) {
            root["result"] = result;
        } else {
            root["result"] = response.getResult();
        }
    } else {
        Json::Value error;
        error["code"] = static_cast<int>(response.getErrorCode());
        error["message"] = response.getErrorMessage();
        root["error"] = error;
    }
    
    std::string body = jsonToString(root);
    
    // 创建消息头
    RpcMessageHeader header;
    header.type = RpcMessageType::RESPONSE;
    header.messageId = response.getMessageId();
    header.bodyLength = body.length();
    header.checksum = calculateChecksum(body);
    header.timestamp = time(nullptr);
    
    return createHeader(header) + body;
}

std::unique_ptr<RpcRequest> JsonProtocolHandler::decodeRequest(const std::string& data) {
    if (!validateMessage(data)) {
        return nullptr;
    }
    
    // 解析消息头
    RpcMessageHeader header = parseHeader(data);
    if (header.type != RpcMessageType::REQUEST) {
        return nullptr;
    }
    
    // 提取消息体
    std::string body = data.substr(sizeof(RpcMessageHeader));
    Json::Value root = parseJson(body);
    
    if (root.isNull() || !root.isMember("method")) {
        return nullptr;
    }
    
    auto request = std::make_unique<RpcRequest>();
    request->setMessageId(header.messageId);
    request->setMethod(root["method"].asString());
    
    if (root.isMember("params")) {
        request->setParams(jsonToString(root["params"]));
    }
    
    return request;
}

std::unique_ptr<RpcResponse> JsonProtocolHandler::decodeResponse(const std::string& data) {
    if (!validateMessage(data)) {
        return nullptr;
    }
    
    // 解析消息头
    RpcMessageHeader header = parseHeader(data);
    if (header.type != RpcMessageType::RESPONSE) {
        return nullptr;
    }
    
    // 提取消息体
    std::string body = data.substr(sizeof(RpcMessageHeader));
    Json::Value root = parseJson(body);
    
    if (root.isNull()) {
        return nullptr;
    }
    
    auto response = std::make_unique<RpcResponse>();
    response->setMessageId(header.messageId);
    
    if (root.isMember("result")) {
        response->setResult(jsonToString(root["result"]));
    } else if (root.isMember("error")) {
        Json::Value error = root["error"];
        RpcErrorCode code = static_cast<RpcErrorCode>(error["code"].asInt());
        std::string message = error["message"].asString();
        response->setError(code, message);
    }
    
    return response;
}

bool JsonProtocolHandler::validateMessage(const std::string& data) {
    if (data.length() < sizeof(RpcMessageHeader)) {
        return false;
    }
    
    RpcMessageHeader header = parseHeader(data);
    
    // 验证魔数
    if (header.magic != 0x12345678) {
        return false;
    }
    
    // 验证消息长度
    if (data.length() != sizeof(RpcMessageHeader) + header.bodyLength) {
        return false;
    }
    
    // 验证校验和
    std::string body = data.substr(sizeof(RpcMessageHeader));
    if (header.checksum != calculateChecksum(body)) {
        return false;
    }
    
    return true;
}

uint32_t JsonProtocolHandler::calculateChecksum(const std::string& data) {
    uint32_t checksum = 0;
    for (char c : data) {
        checksum = checksum * 31 + static_cast<uint32_t>(c);
    }
    return checksum;
}

Json::Value JsonProtocolHandler::parseJson(const std::string& data) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(data, root)) {
        LOG << "Failed to parse JSON: " << data;
        return Json::Value();
    }
    return root;
}

std::string JsonProtocolHandler::jsonToString(const Json::Value& json) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

RpcMessageHeader JsonProtocolHandler::parseHeader(const std::string& data) {
    RpcMessageHeader header;
    if (data.length() >= sizeof(RpcMessageHeader)) {
        memcpy(&header, data.data(), sizeof(RpcMessageHeader));
    }
    return header;
}

std::string JsonProtocolHandler::createHeader(const RpcMessageHeader& header) {
    std::string result(sizeof(RpcMessageHeader), '\0');
    memcpy(&result[0], &header, sizeof(RpcMessageHeader));
    return result;
}
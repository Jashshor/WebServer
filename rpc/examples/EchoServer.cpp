#include "../RpcServer.h"
#include "../RpcConfig.h"
#include "../JsonProtocolHandler.h"
#include "../../EventLoop.h"
#include "../../base/Logging.h"
#include <json/json.h>
#include <iostream>
#include <thread>
#include <chrono>

class EchoServer {
public:
    EchoServer(int port) : loop_(), rpcServer_(&loop_, port) {
        // 加载配置
        RpcConfig::getInstance().loadConfig("../config/rpc_server.conf");
        
        // 设置协议处理器
        auto protocolHandler = std::make_unique<JsonProtocolHandler>();
        rpcServer_.setProtocolHandler(std::move(protocolHandler));
        
        // 注册RPC方法
        registerMethods();
    }
    
    void start() {
        LOG << "Starting Echo RPC Server on port " << RpcConfig::getInstance().getPort();
        rpcServer_.start();
        loop_.loop();
    }

private:
    EventLoop loop_;
    RpcServer rpcServer_;
    
    void registerMethods() {
        // 注册echo方法
        rpcServer_.registerMethod("echo", [this](const std::string& params) -> std::string {
            return handleEcho(params);
        });
        
        // 注册add方法
        rpcServer_.registerMethod("add", [this](const std::string& params) -> std::string {
            return handleAdd(params);
        });
        
        // 注册slow_operation方法（用于测试超时）
        rpcServer_.registerMethod("slow_operation", [this](const std::string& params) -> std::string {
            return handleSlowOperation(params);
        });
        
        // 注册process_data方法（用于测试大数据处理）
        rpcServer_.registerMethod("process_data", [this](const std::string& params) -> std::string {
            return handleProcessData(params);
        });
        
        // 注册get_server_info方法
        rpcServer_.registerMethod("get_server_info", [this](const std::string& params) -> std::string {
            return handleGetServerInfo(params);
        });
    }
    
    std::string handleEcho(const std::string& params) {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(params, root)) {
            return createErrorResponse(-3, "Invalid JSON parameters");
        }
        
        // 直接返回输入参数
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, root);
    }
    
    std::string handleAdd(const std::string& params) {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(params, root)) {
            return createErrorResponse(-3, "Invalid JSON parameters");
        }
        
        if (!root.isMember("a") || !root.isMember("b")) {
            return createErrorResponse(-3, "Missing parameters 'a' or 'b'");
        }
        
        if (!root["a"].isNumeric() || !root["b"].isNumeric()) {
            return createErrorResponse(-3, "Parameters 'a' and 'b' must be numbers");
        }
        
        double a = root["a"].asDouble();
        double b = root["b"].asDouble();
        double result = a + b;
        
        Json::Value response;
        response["result"] = result;
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, response);
    }
    
    std::string handleSlowOperation(const std::string& params) {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(params, root)) {
            return createErrorResponse(-3, "Invalid JSON parameters");
        }
        
        int delay = root.get("delay", 5000).asInt();
        
        // 模拟慢操作
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        
        Json::Value response;
        response["message"] = "Operation completed";
        response["delay"] = delay;
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, response);
    }
    
    std::string handleProcessData(const std::string& params) {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(params, root)) {
            return createErrorResponse(-3, "Invalid JSON parameters");
        }
        
        if (!root.isMember("data")) {
            return createErrorResponse(-3, "Missing parameter 'data'");
        }
        
        std::string data = root["data"].asString();
        
        // 模拟数据处理
        size_t dataSize = data.length();
        
        Json::Value response;
        response["processed"] = true;
        response["data_size"] = static_cast<int>(dataSize);
        response["checksum"] = std::hash<std::string>{}(data);
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, response);
    }
    
    std::string handleGetServerInfo(const std::string& params) {
        auto stats = rpcServer_.getStatistics();
        
        Json::Value response;
        response["server_name"] = "Echo RPC Server";
        response["version"] = "1.0.0";
        response["uptime"] = time(nullptr);
        response["statistics"]["total_requests"] = static_cast<int>(stats.totalRequests);
        response["statistics"]["success_requests"] = static_cast<int>(stats.successRequests);
        response["statistics"]["error_requests"] = static_cast<int>(stats.errorRequests);
        response["statistics"]["avg_response_time"] = stats.avgResponseTime;
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, response);
    }
    
    std::string createErrorResponse(int code, const std::string& message) {
        Json::Value error;
        error["code"] = code;
        error["message"] = message;
        
        Json::Value response;
        response["error"] = error;
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, response);
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    try {
        EchoServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
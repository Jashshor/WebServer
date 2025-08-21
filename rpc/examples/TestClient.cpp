#include "../RpcTestClient.h"
#include "../RpcClient.h"
#include "../JsonProtocolHandler.h"
#include "../../EventLoop.h"
#include "../../base/Logging.h"
#include <iostream>
#include <fstream>
#include <json/json.h>

class TestClientApp {
public:
    TestClientApp(const std::string& serverHost, int serverPort)
        : testClient_(serverHost, serverPort), 
          loop_(),
          rpcClient_(&loop_, serverHost, serverPort) {
        
        // 设置协议处理器
        auto protocolHandler = std::make_unique<JsonProtocolHandler>();
        rpcClient_.setProtocolHandler(std::move(protocolHandler));
    }
    
    void runBasicTests() {
        std::cout << "=== 运行基础功能测试 ===" << std::endl;
        
        // 添加基础测试用例
        testClient_.addTestCase(RpcTestCase("echo_test", "echo", 
            "{\"message\":\"Hello RPC!\"}", "{\"message\":\"Hello RPC!\"}", 5000, true));
        
        testClient_.addTestCase(RpcTestCase("add_test", "add", 
            "{\"a\":10,\"b\":20}", "{\"result\":30}", 3000, true));
        
        testClient_.addTestCase(RpcTestCase("invalid_method", "nonexistent", 
            "{}", "", 5000, false));
        
        // 运行测试
        TestResults results = testClient_.runAllTests();
        
        // 打印结果
        printResults(results);
    }
    
    void runConcurrencyTests() {
        std::cout << "\n=== 运行并发测试 ===" << std::endl;
        
        ConcurrencyTestConfig config(10, 100, 30);
        TestResults results = testClient_.runConcurrencyTest(config);
        
        printConcurrencyResults(results);
    }
    
    void runStressTests() {
        std::cout << "\n=== 运行压力测试 ===" << std::endl;
        
        TestResults results = testClient_.runStressTest(60, 50);
        
        printStressResults(results);
    }
    
    void runInteractiveMode() {
        std::cout << "\n=== 交互模式 ===" << std::endl;
        std::cout << "输入 'help' 查看命令，输入 'quit' 退出" << std::endl;
        
        std::string input;
        while (true) {
            std::cout << "rpc> ";
            std::getline(std::cin, input);
            
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "help") {
                printHelp();
            } else if (input.substr(0, 4) == "call") {
                handleCallCommand(input);
            } else if (input == "stats") {
                printClientStats();
            } else if (input == "connect") {
                connectToServer();
            } else if (input == "disconnect") {
                disconnectFromServer();
            } else {
                std::cout << "未知命令: " << input << std::endl;
            }
        }
    }

private:
    RpcTestClient testClient_;
    EventLoop loop_;
    RpcClient rpcClient_;
    
    void printResults(const TestResults& results) {
        std::cout << "\n=== 测试结果 ===" << std::endl;
        std::cout << "总测试数: " << results.totalTests << std::endl;
        std::cout << "通过: " << results.passedTests << std::endl;
        std::cout << "失败: " << results.failedTests << std::endl;
        std::cout << "成功率: " << (results.totalTests > 0 ? 
            (double)results.passedTests / results.totalTests * 100 : 0) << "%" << std::endl;
        std::cout << "平均响应时间: " << results.avgResponseTime << "ms" << std::endl;
        std::cout << "最小响应时间: " << results.minResponseTime << "ms" << std::endl;
        std::cout << "最大响应时间: " << results.maxResponseTime << "ms" << std::endl;
    }
    
    void printConcurrencyResults(const TestResults& results) {
        std::cout << "\n=== 并发测试结果 ===" << std::endl;
        std::cout << "总请求数: " << results.totalRequests << std::endl;
        std::cout << "成功请求: " << results.successRequests << std::endl;
        std::cout << "错误请求: " << results.errorRequests << std::endl;
        std::cout << "超时请求: " << results.timeoutRequests << std::endl;
        std::cout << "成功率: " << (results.totalRequests > 0 ? 
            (double)results.successRequests / results.totalRequests * 100 : 0) << "%" << std::endl;
        std::cout << "吞吐量: " << results.throughput << " 请求/秒" << std::endl;
        std::cout << "平均响应时间: " << results.avgResponseTime << "ms" << std::endl;
    }
    
    void printStressResults(const TestResults& results) {
        std::cout << "\n=== 压力测试结果 ===" << std::endl;
        std::cout << "测试持续时间: 60秒" << std::endl;
        std::cout << "最大并发数: 50" << std::endl;
        printConcurrencyResults(results);
    }
    
    void printHelp() {
        std::cout << "\n可用命令:" << std::endl;
        std::cout << "  help                    - 显示帮助信息" << std::endl;
        std::cout << "  call <method> <params>  - 调用RPC方法" << std::endl;
        std::cout << "  stats                   - 显示客户端统计信息" << std::endl;
        std::cout << "  connect                 - 连接到服务器" << std::endl;
        std::cout << "  disconnect              - 断开连接" << std::endl;
        std::cout << "  quit/exit               - 退出程序" << std::endl;
        std::cout << "\n示例:" << std::endl;
        std::cout << "  call echo {\"message\":\"test\"}" << std::endl;
        std::cout << "  call add {\"a\":1,\"b\":2}" << std::endl;
    }
    
    void handleCallCommand(const std::string& input) {
        // 解析命令: call <method> <params>
        std::istringstream iss(input);
        std::string cmd, method, params;
        iss >> cmd >> method;
        std::getline(iss, params);
        
        // 去除前导空格
        params.erase(0, params.find_first_not_of(" \t"));
        
        if (method.empty()) {
            std::cout << "用法: call <method> <params>" << std::endl;
            return;
        }
        
        if (!rpcClient_.isConnected()) {
            std::cout << "未连接到服务器，正在尝试连接..." << std::endl;
            if (!rpcClient_.connect()) {
                std::cout << "连接失败" << std::endl;
                return;
            }
        }
        
        std::cout << "调用方法: " << method << std::endl;
        std::cout << "参数: " << params << std::endl;
        
        auto startTime = std::chrono::steady_clock::now();
        RpcResponse response = rpcClient_.call(method, params);
        auto endTime = std::chrono::steady_clock::now();
        
        double responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        std::cout << "响应时间: " << responseTime << "ms" << std::endl;
        
        if (response.isSuccess()) {
            std::cout << "结果: " << response.getResult() << std::endl;
        } else {
            std::cout << "错误 [" << static_cast<int>(response.getErrorCode()) << "]: " 
                      << response.getErrorMessage() << std::endl;
        }
    }
    
    void printClientStats() {
        auto stats = rpcClient_.getStatistics();
        std::cout << "\n=== 客户端统计信息 ===" << std::endl;
        std::cout << "总调用数: " << stats.totalCalls << std::endl;
        std::cout << "成功调用: " << stats.successCalls << std::endl;
        std::cout << "错误调用: " << stats.errorCalls << std::endl;
        std::cout << "超时调用: " << stats.timeoutCalls << std::endl;
        std::cout << "平均响应时间: " << stats.avgResponseTime << "ms" << std::endl;
        std::cout << "连接状态: " << (rpcClient_.isConnected() ? "已连接" : "未连接") << std::endl;
    }
    
    void connectToServer() {
        if (rpcClient_.isConnected()) {
            std::cout << "已经连接到服务器" << std::endl;
            return;
        }
        
        std::cout << "正在连接到服务器..." << std::endl;
        if (rpcClient_.connect()) {
            std::cout << "连接成功" << std::endl;
        } else {
            std::cout << "连接失败" << std::endl;
        }
    }
    
    void disconnectFromServer() {
        if (!rpcClient_.isConnected()) {
            std::cout << "未连接到服务器" << std::endl;
            return;
        }
        
        rpcClient_.disconnect();
        std::cout << "已断开连接" << std::endl;
    }
};

void printUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -h, --help              显示帮助信息" << std::endl;
    std::cout << "  -s, --server <host>     服务器地址 (默认: localhost)" << std::endl;
    std::cout << "  -p, --port <port>       服务器端口 (默认: 8080)" << std::endl;
    std::cout << "  -t, --test <type>       测试类型 (basic|concurrency|stress|interactive)" << std::endl;
    std::cout << "  -c, --config <file>     测试配置文件" << std::endl;
    std::cout << "  -o, --output <file>     输出报告文件" << std::endl;
    std::cout << "  -v, --verbose           详细输出模式" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string serverHost = "localhost";
    int serverPort = 8080;
    std::string testType = "interactive";
    std::string configFile;
    std::string outputFile;
    bool verbose = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-s" || arg == "--server") {
            if (i + 1 < argc) {
                serverHost = argv[++i];
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                serverPort = std::atoi(argv[++i]);
            }
        } else if (arg == "-t" || arg == "--test") {
            if (i + 1 < argc) {
                testType = argv[++i];
            }
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            }
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }
    
    try {
        TestClientApp app(serverHost, serverPort);
        
        std::cout << "RPC测试客户端" << std::endl;
        std::cout << "服务器: " << serverHost << ":" << serverPort << std::endl;
        std::cout << "测试类型: " << testType << std::endl;
        
        if (testType == "basic") {
            app.runBasicTests();
        } else if (testType == "concurrency") {
            app.runConcurrencyTests();
        } else if (testType == "stress") {
            app.runStressTests();
        } else if (testType == "interactive") {
            app.runInteractiveMode();
        } else {
            std::cout << "未知的测试类型: " << testType << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include "RpcClient.h"

// 测试用例结构
struct RpcTestCase {
    std::string name;
    std::string method;
    std::string params;
    std::string expectedResult;
    uint32_t timeout;
    bool shouldSucceed;
    
    RpcTestCase(const std::string& n, const std::string& m, const std::string& p,
                const std::string& expected = "", uint32_t t = 5000, bool succeed = true)
        : name(n), method(m), params(p), expectedResult(expected), 
          timeout(t), shouldSucceed(succeed) {}
};

// 并发测试配置
struct ConcurrencyTestConfig {
    int threadCount = 10;
    int requestsPerThread = 100;
    int durationSeconds = 60;
    std::string method = "echo";
    std::string params = "{\"message\":\"test\"}";
    
    ConcurrencyTestConfig() = default;
    ConcurrencyTestConfig(int threads, int requests, int duration)
        : threadCount(threads), requestsPerThread(requests), durationSeconds(duration) {}
};

// 测试结果统计
struct TestResults {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    double totalTime = 0.0;
    double avgResponseTime = 0.0;
    double minResponseTime = std::numeric_limits<double>::max();
    double maxResponseTime = 0.0;
    
    // 并发测试结果
    int totalRequests = 0;
    int successRequests = 0;
    int errorRequests = 0;
    int timeoutRequests = 0;
    double throughput = 0.0;  // 请求/秒
    
    void addResult(bool passed, double responseTime) {
        totalTests++;
        if (passed) passedTests++;
        else failedTests++;
        
        totalTime += responseTime;
        avgResponseTime = totalTime / totalTests;
        minResponseTime = std::min(minResponseTime, responseTime);
        maxResponseTime = std::max(maxResponseTime, responseTime);
    }
    
    void addConcurrencyResult(bool success, double responseTime) {
        totalRequests++;
        if (success) successRequests++;
        else errorRequests++;
        
        totalTime += responseTime;
        avgResponseTime = totalTime / totalRequests;
        minResponseTime = std::min(minResponseTime, responseTime);
        maxResponseTime = std::max(maxResponseTime, responseTime);
    }
};

// RPC测试客户端
class RpcTestClient {
public:
    RpcTestClient(const std::string& serverHost, int serverPort);
    ~RpcTestClient();
    
    // 添加测试用例
    void addTestCase(const RpcTestCase& testCase);
    
    // 从配置文件加载测试用例
    bool loadTestCases(const std::string& configFile);
    
    // 运行所有测试用例
    TestResults runAllTests();
    
    // 运行单个测试用例
    bool runSingleTest(const RpcTestCase& testCase, double& responseTime);
    
    // 运行并发测试
    TestResults runConcurrencyTest(const ConcurrencyTestConfig& config);
    
    // 运行压力测试
    TestResults runStressTest(int duration, int maxConcurrency);
    
    // 生成测试报告
    void generateReport(const TestResults& results, const std::string& outputFile);
    
    // 设置详细输出模式
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    // 自动生成测试请求
    std::vector<RpcTestCase> generateRandomTests(int count);

private:
    std::string serverHost_;
    int serverPort_;
    std::vector<RpcTestCase> testCases_;
    bool verbose_;
    
    // 验证响应结果
    bool validateResponse(const RpcResponse& response, const RpcTestCase& testCase);
    
    // 打印测试结果
    void printTestResult(const RpcTestCase& testCase, bool passed, 
                        double responseTime, const std::string& error = "");
    
    // 并发测试工作线程
    void concurrencyWorker(const ConcurrencyTestConfig& config, 
                          std::atomic<int>& completedRequests,
                          std::atomic<int>& successCount,
                          std::atomic<int>& errorCount,
                          std::atomic<bool>& stopFlag,
                          std::vector<double>& responseTimes);
    
    // 生成随机字符串
    std::string generateRandomString(int length);
    
    // 生成随机参数
    std::string generateRandomParams();
};
# RPC系统接口变更说明文档

## 概述

本文档详细说明了在原有WebServer项目基础上添加RPC通信协议后的接口变更，确保系统的向后兼容性。

## 1. 新增模块结构

```
rpc/
├── RpcConfig.h/cpp          # RPC配置管理
├── RpcProtocol.h/cpp        # RPC协议定义
├── JsonProtocolHandler.h/cpp # JSON协议处理器
├── RpcServer.h/cpp          # RPC服务器
├── RpcClient.h/cpp          # RPC客户端
├── RpcTestClient.h          # 测试客户端工具
├── config/                  # 配置文件目录
│   ├── rpc_server.conf     # 服务器配置
│   └── test_cases.json     # 测试用例配置
├── docs/                    # 文档目录
│   ├── RPC_PROTOCOL_SPECIFICATION.md
│   └── INTERFACE_CHANGES.md
└── examples/                # 示例代码
    ├── EchoServer.cpp      # 示例RPC服务器
    └── TestClient.cpp      # 测试客户端
```

## 2. 核心类接口

### 2.1 RpcServer类

**新增接口**：
```cpp
class RpcServer {
public:
    RpcServer(EventLoop* loop, int port);
    void start();
    void stop();
    void registerMethod(const std::string& methodName, RpcMethodHandler handler);
    void unregisterMethod(const std::string& methodName);
    void setProtocolHandler(std::unique_ptr<RpcProtocolHandler> handler);
    Statistics getStatistics() const;
};
```

**与原系统集成**：
- 复用现有的`EventLoop`和`Server`类
- 保持原有的网络事件处理机制
- 扩展`Channel`类支持RPC消息处理

### 2.2 RpcClient类

**新增接口**：
```cpp
class RpcClient {
public:
    RpcClient(EventLoop* loop, const std::string& serverHost, int serverPort);
    bool connect();
    void disconnect();
    RpcResponse call(const std::string& method, const std::string& params, uint32_t timeout = 5000);
    void asyncCall(const std::string& method, const std::string& params, RpcCallback callback, uint32_t timeout = 5000);
    void notify(const std::string& method, const std::string& params);
    bool isConnected() const;
    Statistics getStatistics() const;
};
```

### 2.3 RpcConfig类

**配置管理接口**：
```cpp
class RpcConfig {
public:
    static RpcConfig& getInstance();
    bool loadConfig(const std::string& configFile);
    RpcProtocolType getProtocolType() const;
    RpcSerializeType getSerializeType() const;
    RpcTransportType getTransportType() const;
    // ... 其他配置获取方法
};
```

## 3. 原有系统兼容性

### 3.1 HTTP服务保持不变

原有的HTTP服务功能完全保持不变：
- `HttpData`类继续处理HTTP请求
- `MimeType`类继续提供MIME类型支持
- 静态文件服务功能不受影响

### 3.2 网络层复用

RPC系统复用原有的网络基础设施：
- `EventLoop`：事件循环机制
- `Channel`：I/O事件处理
- `Epoll`：I/O多路复用
- `EventLoopThreadPool`：线程池管理

### 3.3 日志系统集成

RPC系统使用原有的异步日志系统：
- 复用`AsyncLogging`类
- 使用相同的日志格式和配置
- 支持日志级别控制

## 4. 配置文件扩展

### 4.1 原有配置保持兼容

原有的服务器配置文件格式保持不变，新增RPC相关配置项：

```ini
# 原有HTTP配置（保持不变）
http_port=80
document_root=/var/www/html
max_connections=1000

# 新增RPC配置
rpc_enabled=true
rpc_port=8080
rpc_protocol=JSON
rpc_threads=4
```

### 4.2 配置优先级

1. 命令行参数 > 配置文件 > 默认值
2. RPC配置独立于HTTP配置
3. 支持运行时配置热更新（部分配置项）

## 5. 编译系统变更

### 5.1 CMakeLists.txt更新

在根目录的CMakeLists.txt中添加：
```cmake
# 添加RPC模块
add_subdirectory(rpc)

# 链接RPC库到主程序
target_link_libraries(webserver 
    libserver_base
    rpc_lib
    jsoncpp
)
```

### 5.2 依赖库

新增依赖：
- `jsoncpp`：JSON处理库
- 保持原有依赖不变

## 6. 使用示例

### 6.1 启用RPC功能的服务器

```cpp
#include "Server.h"
#include "rpc/RpcServer.h"

int main() {
    EventLoop loop;
    
    // 原有HTTP服务器
    Server httpServer(&loop, 4, 80);
    
    // 新增RPC服务器
    RpcServer rpcServer(&loop, 8080);
    rpcServer.registerMethod("echo", [](const std::string& params) {
        return params;  // 简单回显
    });
    
    // 启动服务
    httpServer.start();
    rpcServer.start();
    loop.loop();
    
    return 0;
}
```

### 6.2 RPC客户端调用

```cpp
#include "rpc/RpcClient.h"

int main() {
    EventLoop loop;
    RpcClient client(&loop, "localhost", 8080);
    
    if (client.connect()) {
        RpcResponse response = client.call("echo", "{\"message\":\"Hello RPC!\"}");
        if (response.isSuccess()) {
            std::cout << "结果: " << response.getResult() << std::endl;
        }
    }
    
    return 0;
}
```

## 7. 迁移指南

### 7.1 现有项目升级步骤

1. **更新构建系统**：
   ```bash
   # 安装jsoncpp依赖
   sudo apt-get install libjsoncpp-dev  # Ubuntu/Debian
   # 或
   brew install jsoncpp  # macOS
   ```

2. **更新CMakeLists.txt**：
   添加RPC模块和依赖库

3. **可选启用RPC**：
   ```cpp
   // 在main函数中可选择性启用RPC
   #ifdef ENABLE_RPC
   RpcServer rpcServer(&loop, rpcPort);
   // 注册RPC方法...
   rpcServer.start();
   #endif
   ```

4. **配置文件更新**：
   在现有配置文件中添加RPC相关配置项

### 7.2 向后兼容性保证

- **二进制兼容**：原有的HTTP功能不受影响
- **配置兼容**：原有配置文件继续有效
- **API兼容**：原有类和接口保持不变
- **行为兼容**：HTTP请求处理逻辑不变

## 8. 性能影响评估

### 8.1 内存使用

- RPC模块增加约2-5MB内存使用
- 每个RPC连接约占用8KB内存
- 协议处理器占用可忽略内存

### 8.2 CPU使用

- RPC请求处理增加约5-10%CPU使用
- JSON序列化/反序列化开销较小
- 网络I/O复用原有机制，开销最小

### 8.3 网络带宽

- RPC协议头部开销：32字节
- JSON格式相比二进制格式约增加20-30%数据量
- 支持压缩减少网络传输

## 9. 测试和验证

### 9.1 兼容性测试

```bash
# 运行原有HTTP功能测试
./run_http_tests.sh

# 运行RPC功能测试
./rpc_test_client -t basic

# 运行混合负载测试
./mixed_load_test.sh
```

### 9.2 性能基准测试

```bash
# HTTP性能基准
ab -n 10000 -c 100 http://localhost/

# RPC性能基准
./rpc_test_client -t stress

# 混合性能测试
./benchmark_mixed.sh
```

## 10. 故障排除

### 10.1 常见问题

1. **编译错误**：
   - 检查jsoncpp库是否正确安装
   - 确认CMakeLists.txt配置正确

2. **运行时错误**：
   - 检查端口是否被占用
   - 验证配置文件格式正确

3. **性能问题**：
   - 调整线程池大小
   - 优化序列化方式
   - 启用消息压缩

### 10.2 调试工具

- 使用`LOG`宏输出调试信息
- 利用RPC客户端的统计功能
- 监控系统资源使用情况

## 11. 未来扩展计划

### 11.1 协议支持

- Protocol Buffers支持
- MessagePack支持
- 自定义二进制协议

### 11.2 传输层扩展

- UDP传输支持
- WebSocket支持
- HTTP/2支持

### 11.3 高级功能

- 服务发现机制
- 负载均衡
- 熔断器模式
- 分布式追踪

---

本文档将随着RPC系统的发展持续更新，确保接口变更的透明性和系统的稳定性。
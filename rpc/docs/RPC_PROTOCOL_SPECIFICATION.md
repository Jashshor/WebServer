# RPC通信协议规范文档

## 1. 协议概述

本RPC框架支持多种协议格式和序列化方式，提供灵活的配置选项和扩展能力。

### 1.1 支持的协议类型
- **JSON**: 基于JSON-RPC 2.0标准
- **PROTOBUF**: Google Protocol Buffers（待实现）
- **MSGPACK**: MessagePack二进制序列化（待实现）
- **CUSTOM**: 自定义协议格式

### 1.2 支持的传输层
- **TCP**: 可靠的流式传输
- **UDP**: 无连接的数据报传输（待实现）
- **HTTP**: 基于HTTP的RPC调用（待实现）
- **WEBSOCKET**: WebSocket长连接（待实现）

## 2. 消息格式

### 2.1 消息结构
每个RPC消息由消息头和消息体组成：

```
+------------------+------------------+
|   Message Header |   Message Body   |
|    (32 bytes)    |   (variable)     |
+------------------+------------------+
```

### 2.2 消息头格式
```cpp
struct RpcMessageHeader {
    uint32_t magic;          // 魔数: 0x12345678
    uint32_t version;        // 协议版本: 1
    RpcMessageType type;     // 消息类型: 1字节
    uint32_t messageId;      // 消息ID
    uint32_t bodyLength;     // 消息体长度
    uint32_t checksum;       // 校验和
    uint64_t timestamp;      // 时间戳
};
```

### 2.3 消息类型
- `REQUEST (1)`: RPC请求
- `RESPONSE (2)`: RPC响应
- `NOTIFICATION (3)`: 通知消息（无需响应）
- `HEARTBEAT (4)`: 心跳消息

## 3. JSON协议格式

### 3.1 请求格式
```json
{
    "jsonrpc": "2.0",
    "method": "method_name",
    "params": {
        "param1": "value1",
        "param2": "value2"
    },
    "id": 123
}
```

### 3.2 响应格式
成功响应：
```json
{
    "jsonrpc": "2.0",
    "result": {
        "data": "response_data"
    },
    "id": 123
}
```

错误响应：
```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32601,
        "message": "Method not found"
    },
    "id": 123
}
```

## 4. 错误码规范

### 4.1 标准错误码
| 错误码 | 名称 | 描述 |
|--------|------|------|
| 0 | SUCCESS | 成功 |
| -1 | INVALID_REQUEST | 无效请求 |
| -2 | METHOD_NOT_FOUND | 方法未找到 |
| -3 | INVALID_PARAMS | 无效参数 |
| -4 | INTERNAL_ERROR | 内部错误 |
| -5 | PARSE_ERROR | 解析错误 |
| -6 | TIMEOUT_ERROR | 超时错误 |
| -7 | NETWORK_ERROR | 网络错误 |
| -8 | SERIALIZE_ERROR | 序列化错误 |
| -9 | DESERIALIZE_ERROR | 反序列化错误 |
| -100 | CUSTOM_ERROR | 自定义错误起始码 |

### 4.2 自定义错误码
应用可以定义-100以下的错误码用于业务逻辑错误。

## 5. 配置文件格式

### 5.1 服务器配置
```ini
# 协议配置
protocol_type=JSON
serialize_type=JSON
transport_type=TCP

# 服务器配置
port=8080
thread_num=4
timeout_ms=5000
max_connections=1000

# 性能配置
buffer_size=8192
max_message_size=1048576
keepalive_timeout=60
```

### 5.2 客户端配置
```ini
# 服务器地址
server_host=localhost
server_port=8080

# 连接配置
connect_timeout=5000
request_timeout=10000
max_retries=3

# 连接池配置
pool_size=10
max_idle_connections=5
```

## 6. 性能调优指南

### 6.1 服务器端优化
1. **线程池大小**: 根据CPU核心数设置，通常为核心数的2-4倍
2. **缓冲区大小**: 根据消息大小调整，避免频繁内存分配
3. **连接池**: 复用连接减少建立/断开开销
4. **消息压缩**: 对大消息启用压缩

### 6.2 客户端优化
1. **连接复用**: 使用连接池避免频繁连接
2. **异步调用**: 使用异步接口提高并发性能
3. **批量请求**: 将多个小请求合并为批量请求
4. **超时设置**: 合理设置超时时间避免资源浪费

### 6.3 网络优化
1. **TCP_NODELAY**: 禁用Nagle算法减少延迟
2. **SO_REUSEADDR**: 允许端口复用
3. **缓冲区大小**: 调整系统socket缓冲区大小

## 7. 扩展协议定义

### 7.1 自定义协议接口
```cpp
class CustomProtocolHandler : public RpcProtocolHandler {
public:
    std::string encodeRequest(const RpcRequest& request) override;
    std::string encodeResponse(const RpcResponse& response) override;
    std::unique_ptr<RpcRequest> decodeRequest(const std::string& data) override;
    std::unique_ptr<RpcResponse> decodeResponse(const std::string& data) override;
    bool validateMessage(const std::string& data) override;
    uint32_t calculateChecksum(const std::string& data) override;
};
```

### 7.2 协议注册
```cpp
// 注册自定义协议处理器
RpcProtocolFactory::registerHandler(RpcProtocolType::CUSTOM, 
    []() { return std::make_unique<CustomProtocolHandler>(); });
```

## 8. 安全考虑

### 8.1 消息验证
- 校验消息头魔数和版本
- 验证消息长度和校验和
- 限制消息大小防止DoS攻击

### 8.2 访问控制
- 实现方法级别的访问控制
- 支持认证和授权机制
- 记录访问日志用于审计

### 8.3 数据保护
- 支持TLS加密传输
- 敏感数据脱敏处理
- 防止SQL注入等攻击

## 9. 监控和诊断

### 9.1 性能指标
- QPS (每秒请求数)
- 平均响应时间
- 错误率
- 连接数

### 9.2 日志记录
- 请求/响应日志
- 错误日志
- 性能日志
- 审计日志

### 9.3 健康检查
- 服务可用性检查
- 依赖服务检查
- 资源使用情况监控
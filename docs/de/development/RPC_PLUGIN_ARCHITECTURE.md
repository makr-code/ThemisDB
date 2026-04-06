# RPC Plugin Architecture Requirements

## Overview

This document outlines the architecture and requirements for implementing a plugin-based RPC framework for ThemisDB. The RPC framework will enable distributed operations, particularly for the Stream Protocol network layer and other enterprise features.

**Status:** Planning Document  
**Target Version:** v1.4.0+  
**Estimated Effort:** 2-3 weeks  
**Priority:** Medium (Enterprise Feature)

---

## 1. Architecture Goals

### 1.1 Core Principles

- **Plugin-Based Design:** RPC framework should be loadable as a plugin, not core infrastructure
- **Enterprise Feature:** Licensed separately, not part of community edition
- **Protocol Agnostic:** Support multiple RPC protocols (gRPC, Thrift, custom)
- **Zero-Copy Where Possible:** Minimize data copying for large transfers
- **Async/Non-Blocking:** Support async I/O for high throughput
- **Backward Compatible:** Existing local implementations should work without RPC

### 1.2 Design Patterns

- **Strategy Pattern:** For protocol selection
- **Factory Pattern:** For connection/channel creation
- **Observer Pattern:** For connection lifecycle events
- **Plugin Pattern:** For dynamic loading and lifecycle management

---

## 2. Plugin System Architecture

### 2.1 Plugin Discovery and Loading

```cpp
// Plugin interface
class IRPCPlugin {
public:
    virtual ~IRPCPlugin() = default;
    
    // Lifecycle
    virtual bool initialize(const PluginConfig& config) = 0;
    virtual void shutdown() = 0;
    
    // Metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::vector<std::string> getDependencies() const = 0;
    
    // Capabilities
    virtual bool supportsProtocol(const std::string& protocol) const = 0;
    virtual std::unique_ptr<IRPCClient> createClient(const ClientConfig& config) = 0;
    virtual std::unique_ptr<IRPCServer> createServer(const ServerConfig& config) = 0;
};
```

### 2.2 Plugin Manager

```cpp
class PluginManager {
public:
    // Plugin lifecycle
    bool loadPlugin(const std::string& pluginPath);
    bool unloadPlugin(const std::string& pluginName);
    
    // Plugin registry
    IRPCPlugin* getPlugin(const std::string& name);
    std::vector<std::string> listPlugins() const;
    
    // Singleton access
    static PluginManager& instance();
    
private:
    std::map<std::string, std::unique_ptr<IRPCPlugin>> plugins_;
    std::mutex mutex_;
};
```

### 2.3 Plugin Configuration

```json
{
  "rpc_plugin": {
    "enabled": true,
    "plugin_path": "/opt/themisdb/plugins/librpc_plugin.so",
    "protocol": "grpc",
    "server": {
      "listen_address": "0.0.0.0:50051",
      "max_connections": 1000,
      "thread_pool_size": 16,
      "tls": {
        "enabled": true,
        "cert_file": "/etc/themisdb/server.crt",
        "key_file": "/etc/themisdb/server.key",
        "ca_file": "/etc/themisdb/ca.crt"
      }
    },
    "client": {
      "connection_timeout_ms": 5000,
      "retry_policy": {
        "max_attempts": 3,
        "initial_backoff_ms": 100,
        "max_backoff_ms": 10000,
        "backoff_multiplier": 2.0
      }
    }
  }
}
```

---

## 3. RPC Protocol Interfaces

### 3.1 Client Interface

```cpp
class IRPCClient {
public:
    virtual ~IRPCClient() = default;
    
    // Connection management
    virtual bool connect(const std::string& address) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    // Synchronous RPC
    virtual RPCResponse call(const std::string& method, 
                            const RPCRequest& request) = 0;
    
    // Asynchronous RPC
    virtual std::future<RPCResponse> callAsync(const std::string& method,
                                               const RPCRequest& request) = 0;
    
    // Streaming (bidirectional)
    virtual std::unique_ptr<IRPCStream> createStream(const std::string& method) = 0;
    
    // Health check
    virtual bool healthCheck() = 0;
};
```

### 3.2 Server Interface

```cpp
class IRPCServer {
public:
    virtual ~IRPCServer() = default;
    
    // Server lifecycle
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    
    // Service registration
    virtual void registerService(const std::string& name,
                                IRPCService* service) = 0;
    virtual void unregisterService(const std::string& name) = 0;
    
    // Interceptors
    virtual void addInterceptor(std::unique_ptr<IRPCInterceptor> interceptor) = 0;
};
```

### 3.3 Service Interface

```cpp
class IRPCService {
public:
    virtual ~IRPCService() = default;
    
    // Method handling
    virtual RPCResponse handleRequest(const std::string& method,
                                     const RPCRequest& request,
                                     RPCContext& context) = 0;
    
    // Service metadata
    virtual std::string getServiceName() const = 0;
    virtual std::vector<std::string> getSupportedMethods() const = 0;
};
```

---

## 4. Stream Protocol Integration

### 4.1 RPC-Enabled Stream Operations

```cpp
// Stream Protocol with RPC support
class StreamProtocol {
public:
    // Set RPC client for remote operations
    void setRPCClient(std::shared_ptr<IRPCClient> rpcClient);
    
    // Create chunk (local or remote)
    std::optional<StreamChunk> createChunk(const std::string& source_path,
                                          size_t offset,
                                          size_t chunk_size,
                                          bool compress = true);
    
    // Send chunk via RPC
    bool sendChunk(const StreamChunk& chunk,
                  const std::string& target_shard);
    
    // Write chunk (received via RPC)
    bool writeChunk(const StreamChunk& chunk,
                   const std::string& target_path);
    
    // Request retry for failed chunk
    bool requestRetry(const std::string& chunk_id,
                     const std::string& source_shard);
};
```

### 4.2 RPC Service Definition (gRPC Example)

```protobuf
syntax = "proto3";

package themisdb.rpc.stream;

service StreamService {
    // Send a single chunk
    rpc SendChunk(ChunkRequest) returns (ChunkResponse);
    
    // Stream chunks (bidirectional)
    rpc StreamChunks(stream ChunkRequest) returns (stream ChunkResponse);
    
    // Request chunk retry
    rpc RequestRetry(RetryRequest) returns (RetryResponse);
    
    // Verify chunk integrity
    rpc VerifyChunk(VerifyRequest) returns (VerifyResponse);
}

message ChunkRequest {
    string chunk_id = 1;
    string target_path = 2;
    bytes data = 3;
    uint32 crc32 = 4;
    bool compressed = 5;
    string compression_type = 6;
}

message ChunkResponse {
    bool success = 1;
    string error_message = 2;
    uint64 bytes_written = 3;
}

message RetryRequest {
    string chunk_id = 1;
    string source_shard = 2;
    uint32 attempt_number = 3;
}

message RetryResponse {
    bool retry_granted = 1;
    string error_message = 2;
}

message VerifyRequest {
    string chunk_id = 1;
    uint32 expected_crc32 = 2;
}

message VerifyResponse {
    bool verified = 1;
    uint32 actual_crc32 = 2;
}
```

---

## 5. Protocol Implementations

### 5.1 Supported Protocols

#### 5.1.1 gRPC (Recommended)

**Advantages:**
- Industry standard
- HTTP/2 based (multiplexing, flow control)
- Excellent tooling and cross-language support
- Built-in load balancing and health checking
- Supports streaming (unary, server-streaming, client-streaming, bidirectional)

**Dependencies:**
- grpc++ (C++ library)
- protobuf (serialization)

**Implementation Priority:** High

#### 5.1.2 Apache Thrift

**Advantages:**
- Mature and battle-tested
- Good performance
- Multiple transport options (binary, compact, JSON)

**Dependencies:**
- libthrift

**Implementation Priority:** Medium

#### 5.1.3 Custom Binary Protocol

**Advantages:**
- Full control over wire format
- Minimal dependencies
- Optimized for specific use cases

**Disadvantages:**
- More development and maintenance
- Limited tooling

**Implementation Priority:** Low

---

## 6. Security and Authentication

### 6.1 TLS/SSL Support

```cpp
struct TLSConfig {
    bool enabled = false;
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    bool verify_client = false;
    std::vector<std::string> allowed_ciphers;
};
```

### 6.2 Authentication Mechanisms

1. **Token-Based Authentication**
   - JWT tokens for stateless authentication
   - Token refresh mechanism
   - Configurable expiration

2. **Mutual TLS (mTLS)**
   - Client certificate verification
   - Certificate chain validation

3. **API Keys**
   - Simple key-based authentication
   - Key rotation support

### 6.3 Authorization

```cpp
class IRPCAuthorizationPolicy {
public:
    virtual ~IRPCAuthorizationPolicy() = default;
    
    // Check if user can perform action
    virtual bool authorize(const RPCContext& context,
                          const std::string& service,
                          const std::string& method) = 0;
};
```

---

## 7. Performance Considerations

### 7.1 Connection Pooling

```cpp
class ConnectionPool {
public:
    ConnectionPool(size_t poolSize);
    
    // Get connection from pool
    std::shared_ptr<IRPCClient> acquire(const std::string& address);
    
    // Return connection to pool
    void release(std::shared_ptr<IRPCClient> client);
    
    // Pool management
    void setMaxSize(size_t maxSize);
    void setIdleTimeout(std::chrono::seconds timeout);
    void clearIdleConnections();
};
```

### 7.2 Request Batching

```cpp
class RPCBatcher {
public:
    // Add request to batch
    void addRequest(const RPCRequest& request);
    
    // Send batch when ready
    std::vector<RPCResponse> flush();
    
    // Auto-flush configuration
    void setMaxBatchSize(size_t size);
    void setMaxWaitTime(std::chrono::milliseconds wait);
};
```

### 7.3 Zero-Copy Optimization

- Use memory-mapped files for large transfers
- Implement sendfile() support where available
- Direct buffer passing between network and storage layers

---

## 8. Error Handling and Resilience

### 8.1 Retry Policies

```cpp
struct RetryPolicy {
    uint32_t max_attempts = 3;
    std::chrono::milliseconds initial_backoff{100};
    std::chrono::milliseconds max_backoff{10000};
    double backoff_multiplier = 2.0;
    
    // Retryable error codes
    std::set<RPCErrorCode> retryable_errors = {
        RPCErrorCode::UNAVAILABLE,
        RPCErrorCode::DEADLINE_EXCEEDED,
        RPCErrorCode::RESOURCE_EXHAUSTED
    };
};
```

### 8.2 Circuit Breaker

```cpp
class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };
    
    // Check if call is allowed
    bool allowRequest();
    
    // Record result
    void recordSuccess();
    void recordFailure();
    
    // Configuration
    void setFailureThreshold(uint32_t threshold);
    void setResetTimeout(std::chrono::seconds timeout);
    void setHalfOpenRequests(uint32_t requests);
};
```

### 8.3 Timeout Management

```cpp
struct TimeoutConfig {
    std::chrono::milliseconds connection_timeout{5000};
    std::chrono::milliseconds request_timeout{30000};
    std::chrono::milliseconds idle_timeout{60000};
    std::chrono::milliseconds keepalive_interval{10000};
};
```

---

## 9. Monitoring and Observability

### 9.1 Metrics

```cpp
struct RPCMetrics {
    // Request metrics
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::atomic<uint64_t> retried_requests{0};
    
    // Latency (histogram)
    LatencyHistogram request_latency;
    
    // Throughput
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
    
    // Connection pool
    std::atomic<uint32_t> active_connections{0};
    std::atomic<uint32_t> idle_connections{0};
};
```

### 9.2 Logging

```cpp
class RPCLogger {
public:
    // Log levels
    void logRequest(const RPCRequest& request, const RPCContext& context);
    void logResponse(const RPCResponse& response, std::chrono::milliseconds latency);
    void logError(const std::string& error, const RPCContext& context);
    
    // Structured logging
    void setLogFormat(LogFormat format); // JSON, TEXT, BINARY
    void setLogLevel(LogLevel level);
};
```

### 9.3 Tracing (OpenTelemetry)

```cpp
class RPCTracer {
public:
    // Start trace span
    SpanContext startSpan(const std::string& operation);
    
    // Add span attributes
    void addAttribute(const std::string& key, const std::string& value);
    
    // End span
    void endSpan(const SpanContext& span);
    
    // Propagate context
    void injectContext(RPCRequest& request, const SpanContext& span);
    SpanContext extractContext(const RPCRequest& request);
};
```

---

## 10. Testing Strategy

### 10.1 Unit Tests

- Mock RPC clients and servers
- Test retry logic
- Test circuit breaker state transitions
- Test connection pool management

### 10.2 Integration Tests

- Test with real gRPC/Thrift implementations
- Test TLS/SSL connections
- Test authentication and authorization
- Test streaming operations

### 10.3 Performance Tests

- Benchmark throughput (requests/second, bytes/second)
- Measure latency (p50, p95, p99)
- Test under load (concurrent connections, large payloads)
- Test memory usage and connection pool behavior

### 10.4 Chaos Engineering

- Network partition simulation
- Packet loss and delays
- Server failures and recovery
- Timeout scenarios

---

## 11. Implementation Phases

### Phase 1: Foundation (Week 1)

**Deliverables:**
- Plugin architecture implementation
- Plugin manager and discovery
- Basic IRPCClient and IRPCServer interfaces
- Configuration system

**Effort:** 3-4 days

### Phase 2: gRPC Implementation (Week 2)

**Deliverables:**
- gRPC client implementation
- gRPC server implementation
- Stream Protocol RPC service
- Basic authentication (TLS + tokens)

**Effort:** 4-5 days

### Phase 3: Resilience & Production (Week 3)

**Deliverables:**
- Retry policies and circuit breakers
- Connection pooling
- Metrics and observability
- Comprehensive testing
- Documentation

**Effort:** 4-5 days

---

## 12. Dependencies

### 12.1 Required Libraries

```cmake
# gRPC
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

# OpenSSL (for TLS)
find_package(OpenSSL REQUIRED)

# Optional: OpenTelemetry
find_package(opentelemetry-cpp CONFIG)
```

### 12.2 Build Configuration

```cmake
option(THEMISDB_BUILD_RPC_PLUGIN "Build RPC plugin" OFF)
option(THEMISDB_RPC_USE_GRPC "Use gRPC for RPC" ON)
option(THEMISDB_RPC_USE_THRIFT "Use Thrift for RPC" OFF)

if(THEMISDB_BUILD_RPC_PLUGIN)
    add_subdirectory(plugins/rpc)
endif()
```

---

## 13. Migration Path

### 13.1 Backward Compatibility

- Stream Protocol continues to work locally without RPC plugin
- RPC plugin is opt-in via configuration
- Existing installations work unchanged

### 13.2 Upgrade Process

1. Install RPC plugin (enterprise license required)
2. Configure RPC settings in config file
3. Restart ThemisDB instances
4. Enable distributed features

---

## 14. License and Distribution

### 14.1 Licensing

- **Community Edition:** Does not include RPC plugin
- **Enterprise Edition:** Includes RPC plugin with license key
- **License Verification:** Plugin checks for valid enterprise license on load

### 14.2 Distribution

```bash
# Community package
themisdb-community-1.3.0.tar.gz
  |- bin/themisdb
  |- lib/libthemisdb_core.so
  |- etc/themisdb.conf

# Enterprise package
themisdb-enterprise-1.3.0.tar.gz
  |- bin/themisdb
  |- lib/libthemisdb_core.so
  |- lib/plugins/librpc_plugin.so  # <- RPC plugin
  |- etc/themisdb.conf
  |- etc/themisdb-enterprise.conf
  |- LICENSE.txt
```

---

## 15. Future Enhancements

### 15.1 Advanced Features

- **Service Mesh Integration:** Istio, Linkerd support
- **Dynamic Service Discovery:** Consul, etcd integration
- **Load Balancing:** Client-side and server-side load balancing
- **Compression:** Snappy, Brotli for RPC payloads
- **Multi-tenancy:** Tenant isolation at RPC level

### 15.2 Additional Protocols

- **WebSockets:** For browser-based clients
- **GraphQL:** Query-based API
- **REST API Gateway:** HTTP/JSON wrapper around RPC

---

## 16. Success Criteria

### 16.1 Functional Requirements

- [ ] Plugin loads and initializes successfully
- [ ] gRPC client can connect to server
- [ ] Stream Protocol can transfer files via RPC
- [ ] Authentication and authorization work
- [ ] Retry and circuit breaker policies function correctly

### 16.2 Performance Requirements

- [ ] Throughput >= 1 GB/s for large file transfers
- [ ] Latency < 10ms for small requests (p99)
- [ ] Support 1000+ concurrent connections
- [ ] Memory usage < 100 MB for plugin

### 16.3 Reliability Requirements

- [ ] 99.9% availability
- [ ] Graceful degradation on failures
- [ ] Zero data loss on network failures
- [ ] Automatic recovery from transient errors

---

## 17. References

### 17.1 Related Documentation

- Stream Protocol Implementation (src/sharding/stream_protocol.cpp)
- Plugin Architecture Best Practices
- gRPC C++ Documentation: https://grpc.io/docs/languages/cpp/
- OpenTelemetry C++: https://opentelemetry.io/docs/instrumentation/cpp/

### 17.2 Similar Implementations

- Apache Arrow Flight (gRPC-based data transfer)
- TiKV (Rust, gRPC for distributed KV store)
- ScyllaDB (Thrift/CQL for distributed database)

---

## Appendix A: Example Usage

### A.1 Client Example

```cpp
// Initialize plugin
auto& pluginMgr = PluginManager::instance();
pluginMgr.loadPlugin("/opt/themisdb/plugins/librpc_plugin.so");

auto* rpcPlugin = pluginMgr.getPlugin("grpc_rpc");

// Create client
ClientConfig clientConfig;
clientConfig.address = "192.168.1.100:50051";
clientConfig.tls_enabled = true;
clientConfig.cert_file = "/etc/themisdb/client.crt";

auto client = rpcPlugin->createClient(clientConfig);
client->connect(clientConfig.address);

// Use with Stream Protocol
StreamProtocol stream;
stream.setRPCClient(client);

auto chunk = stream.createChunk("/data/large_file.dat", 0, 1024*1024);
bool success = stream.sendChunk(*chunk, "shard-002");
```

### A.2 Server Example

```cpp
// Create server
ServerConfig serverConfig;
serverConfig.listen_address = "0.0.0.0:50051";
serverConfig.tls_enabled = true;

auto server = rpcPlugin->createServer(serverConfig);

// Register Stream service
auto streamService = std::make_unique<StreamRPCService>();
server->registerService("StreamService", streamService.get());

// Start server
server->start();
```

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-06  
**Author:** ThemisDB Development Team  
**Status:** Planning/Specification

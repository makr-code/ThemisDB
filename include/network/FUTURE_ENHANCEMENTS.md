# Network Module Headers - Future Enhancements

## Planned Interface Extensions

### WebSocket Server Interface
**Priority:** High  
**Target Version:** v1.7.0

Add WebSocket server interface for real-time bidirectional communication.

**Proposed Interface:**
```cpp
namespace themis::network {

class WebSocketServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8767;
        size_t num_threads = std::thread::hardware_concurrency();
        
        uint32_t max_connections = 10000;
        uint32_t max_message_size_mb = 16;
        uint32_t ping_interval_sec = 30;
        
        bool enable_tls = false;
        std::string tls_cert_path;
        std::string tls_key_path;
        
        bool enable_compression = true;  // Per-message deflate
        bool require_auth = true;
    };
    
    WebSocketServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> index_mgr
    );
    
    ~WebSocketServer();
    
    void start();
    void stop();
    bool isRunning() const;
    
    // Server-initiated push
    void broadcast(const std::string& topic, const std::string& message);
    void send(const std::string& session_id, const std::string& message);
    
    struct Stats {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t messages_sent = 0;
        uint64_t messages_received = 0;
        uint64_t broadcasts = 0;
    };
    Stats getStats() const;
};

} // namespace themis::network
```

**Client Interface:**
```cpp
namespace themis::network {

class WebSocketClient {
public:
    struct Config {
        std::chrono::seconds connect_timeout{5};
        std::chrono::seconds ping_interval{30};
        bool enable_compression = true;
        bool verify_tls = true;
    };
    
    WebSocketClient(const std::string& url, const Config& config = Config{});
    ~WebSocketClient();
    
    void connect();
    void disconnect();
    bool isConnected() const;
    
    void send(const std::string& message);
    void subscribe(const std::string& topic, 
                   std::function<void(const std::string&)> callback);
    void unsubscribe(const std::string& topic);
    
    void setMessageHandler(std::function<void(const std::string&)> handler);
    void setErrorHandler(std::function<void(const std::string&)> handler);
};

} // namespace themis::network
```

**Integration Example:**
```cpp
// Server-side
WebSocketServer ws_server(config, storage, index_mgr);
ws_server.start();

// Broadcast document change event
ws_server.broadcast("documents/articles", 
    R"({"event": "update", "uuid": "doc_123"})");

// Client-side
WebSocketClient client("wss://server.example.com:8767");
client.connect();

client.subscribe("documents/articles", [](const std::string& msg) {
    std::cout << "Document changed: " << msg << std::endl;
});
```

---

### UDP Server Interface
**Priority:** Medium  
**Target Version:** v1.8.0

Add UDP server interface for high-throughput, low-latency ingestion.

**Proposed Interface:**
```cpp
namespace themis::network {

class UDPServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8768;
        size_t num_threads = 4;
        
        size_t max_packet_size = 65507;  // Max UDP packet
        bool enable_batching = true;
        uint32_t batch_interval_ms = 100;
        bool enable_acks = false;  // Fire-and-forget
        
        uint32_t max_packets_per_second = 1000000;
    };
    
    UDPServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage
    );
    
    ~UDPServer();
    
    void start();
    void stop();
    bool isRunning() const;
    
    struct Stats {
        uint64_t packets_received = 0;
        uint64_t packets_dropped = 0;
        uint64_t bytes_received = 0;
        uint64_t parse_errors = 0;
    };
    Stats getStats() const;
};

class UDPClient {
public:
    struct Config {
        std::chrono::milliseconds send_timeout{1000};
        bool enable_acks = false;
        size_t max_retries = 3;
    };
    
    UDPClient(const std::string& target, const Config& config = Config{});
    ~UDPClient();
    
    void send(const std::vector<uint8_t>& data);
    void send(const std::vector<uint8_t>& data, bool wait_for_ack);
};

} // namespace themis::network
```

---

### QUIC Server Interface
**Priority:** Medium  
**Target Version:** v2.0.0

Add QUIC (HTTP/3) server interface for modern low-latency communication.

**Proposed Interface:**
```cpp
namespace themis::network {

class QUICServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8769;
        size_t num_threads = std::thread::hardware_concurrency();
        
        uint32_t max_streams_per_connection = 100;
        bool enable_0rtt = true;
        std::string congestion_control = "bbr";  // bbr or cubic
        
        std::string tls_cert_path;
        std::string tls_key_path;
        
        uint32_t max_idle_timeout_sec = 60;
    };
    
    QUICServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> index_mgr
    );
    
    ~QUICServer();
    
    void start();
    void stop();
    bool isRunning() const;
    
    struct Stats {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t total_streams = 0;
        uint64_t zero_rtt_accepted = 0;
        uint64_t zero_rtt_rejected = 0;
        uint64_t migrations = 0;  // Connection migration events
    };
    Stats getStats() const;
};

class QUICClient {
public:
    struct Config {
        std::chrono::seconds connect_timeout{5};
        bool enable_0rtt = true;
        bool verify_tls = true;
        std::string congestion_control = "bbr";
    };
    
    QUICClient(const std::string& url, const Config& config = Config{});
    ~QUICClient();
    
    void connect();
    void disconnect();
    bool isConnected() const;
    
    class Stream {
    public:
        void send(const std::vector<uint8_t>& data);
        std::vector<uint8_t> receive();
        void close();
    };
    
    std::unique_ptr<Stream> openStream();
};

} // namespace themis::network
```

---

### Network Quality of Service (QoS) Interface
**Priority:** Medium  
**Target Version:** v1.8.0

Add QoS management for bandwidth control and traffic prioritization.

**Proposed Interface:**
```cpp
namespace themis::network {

enum class Priority {
    CRITICAL = 0,  // Interactive queries
    HIGH = 1,      // Transactional operations
    MEDIUM = 2,    // Analytical queries
    LOW = 3        // Batch operations, backups
};

class QoSManager {
public:
    struct Config {
        uint64_t max_bandwidth_bps = 1'000'000'000;  // 1 Gbps
        uint64_t per_connection_limit_bps = 100'000'000;  // 100 Mbps
        bool enable_fair_queuing = true;
        bool enable_priority_queuing = true;
    };
    
    explicit QoSManager(const Config& config);
    ~QoSManager();
    
    void setPriority(uint64_t connection_id, Priority priority);
    void setBandwidthLimit(uint64_t connection_id, uint64_t bps);
    void setTokenBucket(uint64_t connection_id, uint64_t rate_bps, uint64_t burst_bytes);
    
    struct Stats {
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_shaped = 0;  // Delayed due to QoS
        uint64_t priority_violations = 0;  // Low priority exceeded quota
        
        std::map<Priority, uint64_t> bytes_per_priority;
    };
    Stats getStats() const;
};

} // namespace themis::network
```

---

### Distributed Tracing Interface
**Priority:** Medium  
**Target Version:** v1.7.0

Add OpenTelemetry integration for distributed tracing.

**Proposed Interface:**
```cpp
namespace themis::network {

class TracingContext {
public:
    struct Config {
        std::string service_name = "themisdb-wire-protocol";
        std::string jaeger_endpoint;
        std::string zipkin_endpoint;
        double sampling_rate = 0.1;  // 10% sampling
    };
    
    explicit TracingContext(const Config& config);
    ~TracingContext();
    
    class Span {
    public:
        void setAttribute(const std::string& key, const std::string& value);
        void setAttribute(const std::string& key, int64_t value);
        void setAttribute(const std::string& key, double value);
        void addEvent(const std::string& name);
        void setStatus(bool success, const std::string& description = "");
        void end();
    };
    
    std::unique_ptr<Span> startSpan(const std::string& name);
    std::unique_ptr<Span> startSpan(const std::string& name, 
                                      const std::map<std::string, std::string>& attributes);
    
    // Extract trace context from incoming request
    void extractContext(const std::map<std::string, std::string>& headers);
    
    // Inject trace context into outgoing request
    void injectContext(std::map<std::string, std::string>& headers);
};

} // namespace themis::network
```

**Usage Example:**
```cpp
TracingContext tracing(tracing_config);

void handleRequest(const Request& req) {
    // Extract parent trace context
    tracing.extractContext(req.headers);
    
    auto span = tracing.startSpan("wire_protocol.handle_get",
        {{"collection", req.collection}, {"uuid", req.uuid}}
    );
    
    auto result = storage_->get(key);
    
    span->setAttribute("result_size", result.size());
    span->setStatus(true);
    span->end();
}
```

---

### Load Balancer Interface with Raft
**Priority:** High  
**Target Version:** v1.8.0

Add Raft-based load balancer for distributed query routing.

**Proposed Interface:**
```cpp
namespace themis::network {

enum class LoadBalancingStrategy {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    WEIGHTED_ROUND_ROBIN,
    CONSISTENT_HASH
};

class RaftLoadBalancer {
public:
    struct Config {
        uint16_t raft_port = 8774;
        uint32_t health_check_interval_ms = 5000;
        double rebalance_threshold = 0.2;  // 20% load imbalance
        LoadBalancingStrategy strategy = LoadBalancingStrategy::LEAST_CONNECTIONS;
    };
    
    explicit RaftLoadBalancer(const Config& config);
    ~RaftLoadBalancer();
    
    void addBackend(const std::string& address, double weight = 1.0);
    void removeBackend(const std::string& address);
    void updateWeight(const std::string& address, double weight);
    
    struct Backend {
        std::string address;
        double weight = 1.0;
        bool healthy = true;
        uint64_t active_connections = 0;
        std::chrono::steady_clock::time_point last_health_check;
    };
    
    std::vector<Backend> getBackends() const;
    
    // Get connection to least loaded backend
    std::unique_ptr<WireProtocolConnectionPool::ConnectionHandle> getConnection();
    
    // Get connection for specific key (consistent hashing)
    std::unique_ptr<WireProtocolConnectionPool::ConnectionHandle> getConnection(
        const std::string& key
    );
    
    void start();
    void stop();
    bool isLeader() const;
    
    struct Stats {
        uint64_t total_requests = 0;
        uint64_t failed_backends = 0;
        uint64_t rebalance_events = 0;
        std::map<std::string, uint64_t> requests_per_backend;
    };
    Stats getStats() const;
};

} // namespace themis::network
```

---

### Connection Migration Interface (QUIC)
**Priority:** Low  
**Target Version:** v2.0.0

Add connection migration support for mobile clients.

**Proposed Interface:**
```cpp
namespace themis::network {

class ConnectionMigration {
public:
    enum class MigrationReason {
        NETWORK_CHANGE,      // Wi-Fi to cellular
        IP_ADDRESS_CHANGE,   // DHCP renewal
        PATH_DEGRADATION,    // High packet loss on current path
        LOAD_BALANCING       // Server-initiated migration
    };
    
    struct MigrationEvent {
        MigrationReason reason;
        std::string old_address;
        std::string new_address;
        std::chrono::steady_clock::time_point timestamp;
        bool successful;
    };
    
    void enableMigration(bool enable);
    bool isMigrationEnabled() const;
    
    void setMigrationCallback(
        std::function<void(const MigrationEvent&)> callback
    );
    
    std::vector<MigrationEvent> getMigrationHistory() const;
};

} // namespace themis::network
```

---

### Network Metrics Interface
**Priority:** Medium  
**Target Version:** v1.7.0

Add comprehensive network metrics interface for monitoring.

**Proposed Interface:**
```cpp
namespace themis::network {

class NetworkMetrics {
public:
    struct LatencyStats {
        double p50_ms = 0.0;
        double p95_ms = 0.0;
        double p99_ms = 0.0;
        double p999_ms = 0.0;
        double max_ms = 0.0;
    };
    
    struct ThroughputStats {
        uint64_t bytes_per_second = 0;
        uint64_t packets_per_second = 0;
        uint64_t requests_per_second = 0;
    };
    
    struct ErrorStats {
        uint64_t connection_errors = 0;
        uint64_t timeout_errors = 0;
        uint64_t parse_errors = 0;
        uint64_t auth_errors = 0;
        double error_rate = 0.0;
    };
    
    LatencyStats getLatencyStats() const;
    ThroughputStats getThroughputStats() const;
    ErrorStats getErrorStats() const;
    
    // Histogram for detailed latency distribution
    std::map<uint64_t, uint64_t> getLatencyHistogram() const;
    
    void reset();
};

} // namespace themis::network
```

---

### Circuit Breaker Interface Enhancement
**Priority:** Medium  
**Target Version:** v1.8.0

Enhance circuit breaker with adaptive thresholds and half-open state.

**Proposed Interface:**
```cpp
namespace themis::network {

enum class CircuitState {
    CLOSED,      // Normal operation
    HALF_OPEN,   // Testing if service recovered
    OPEN         // Circuit opened, fail fast
};

class AdaptiveCircuitBreaker {
public:
    struct Config {
        size_t failure_threshold = 10;
        size_t success_threshold = 5;  // For half-open state
        std::chrono::seconds open_timeout{60};
        std::chrono::seconds half_open_timeout{30};
        bool enable_adaptive_threshold = true;
        double adaptive_factor = 0.1;  // Adjust threshold by 10%
    };
    
    explicit AdaptiveCircuitBreaker(const Config& config);
    
    bool shouldAllow();
    void recordSuccess();
    void recordFailure();
    
    CircuitState getState() const;
    
    struct Stats {
        CircuitState state = CircuitState::CLOSED;
        uint64_t total_calls = 0;
        uint64_t successful_calls = 0;
        uint64_t failed_calls = 0;
        uint64_t rejected_calls = 0;  // Rejected due to open circuit
        std::chrono::steady_clock::time_point last_state_change;
    };
    Stats getStats() const;
    
    void setStateChangeCallback(
        std::function<void(CircuitState, CircuitState)> callback
    );
};

} // namespace themis::network
```

---

### Connection Pool Strategy Interface
**Priority:** Low  
**Target Version:** v1.9.0

Add pluggable connection pool strategies.

**Proposed Interface:**
```cpp
namespace themis::network {

class IPoolingStrategy {
public:
    virtual ~IPoolingStrategy() = default;
    
    virtual size_t getIdealConnectionCount(
        size_t current_count,
        size_t active_count,
        double load
    ) = 0;
    
    virtual bool shouldCreateConnection(
        size_t current_count,
        size_t max_count,
        size_t available_count
    ) = 0;
    
    virtual bool shouldRemoveConnection(
        const Connection& conn,
        std::chrono::seconds idle_time
    ) = 0;
};

class AdaptivePoolingStrategy : public IPoolingStrategy {
public:
    struct Config {
        double target_utilization = 0.8;  // 80% target
        double scale_up_factor = 1.5;
        double scale_down_factor = 0.7;
        std::chrono::seconds min_idle_time{300};  // 5 minutes
    };
    
    explicit AdaptivePoolingStrategy(const Config& config);
    
    size_t getIdealConnectionCount(
        size_t current_count,
        size_t active_count,
        double load
    ) override;
    
    bool shouldCreateConnection(
        size_t current_count,
        size_t max_count,
        size_t available_count
    ) override;
    
    bool shouldRemoveConnection(
        const Connection& conn,
        std::chrono::seconds idle_time
    ) override;
};

} // namespace themis::network
```

---

### Zero-Copy I/O Interface
**Priority:** Low  
**Target Version:** v2.1.0

Add zero-copy I/O interface for ultra-low latency.

**Proposed Interface:**
```cpp
namespace themis::network {

class ZeroCopySocket {
public:
    struct Config {
        bool enable_sendfile = true;      // Linux sendfile()
        bool enable_splice = true;        // Linux splice()
        bool enable_zerocopy_send = true; // Linux MSG_ZEROCOPY
        size_t buffer_size = 1024 * 1024; // 1 MB
    };
    
    explicit ZeroCopySocket(socket_t socket, const Config& config = Config{});
    ~ZeroCopySocket();
    
    // Zero-copy send from file
    ssize_t sendFile(int fd, off_t offset, size_t count);
    
    // Zero-copy send from memory (if supported by kernel)
    ssize_t sendZeroCopy(const void* buffer, size_t size);
    
    // Memory-mapped receive (if supported)
    void* receiveZeroCopy(size_t* size_out);
    void releaseZeroCopyBuffer(void* buffer);
    
    struct Stats {
        uint64_t total_bytes_sent = 0;
        uint64_t zero_copy_bytes = 0;
        uint64_t fallback_bytes = 0;  // Fallback to normal send
        
        double getZeroCopyRate() const {
            return total_bytes_sent > 0 
                ? (double)zero_copy_bytes / total_bytes_sent 
                : 0.0;
        }
    };
    Stats getStats() const;
};

} // namespace themis::network
```

---

## API Stability Guarantees

### Stable APIs (v1.x.x)
These interfaces will maintain backward compatibility:
- `WireProtocolServer`
- `WireProtocolConnectionPool`
- `SocketTimeoutManager`
- Core statistics structures

### Experimental APIs (v2.x.x)
These interfaces may change in future versions:
- `WebSocketServer` (until v1.8.0)
- `UDPServer` (until v1.9.0)
- `QUICServer` (until v2.1.0)
- All RDMA and kernel bypass APIs

### Deprecation Policy
- 6 months notice before deprecation
- Alternative API provided
- Migration guide published
- Deprecation warnings in headers

---

## References

- [WebSocket Protocol (RFC 6455)](https://tools.ietf.org/html/rfc6455)
- [QUIC Protocol (RFC 9000)](https://tools.ietf.org/html/rfc9000)
- [OpenTelemetry Specification](https://opentelemetry.io/docs/specs/)
- [Circuit Breaker Pattern](https://martinfowler.com/bliki/CircuitBreaker.html)
- [Zero-Copy Networking](https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html)

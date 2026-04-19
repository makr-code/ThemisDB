> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Network Module Headers

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ../src/network/README.md · ../src/network/ARCHITECTURE.md -->

## Module Purpose

The Network headers define the public interfaces for ThemisDB's high-performance networking layer. These headers expose wire protocol server capabilities, connection pooling abstractions, socket timeout management, QoS, UDP fast-path, QUIC, gRPC, geo-topology routing, and service mesh integration without requiring clients to depend on implementation details or external networking libraries beyond standard interfaces.

## Scope

**In Scope:**
- Wire Protocol Server interface for binary TCP communication (port 8766)
- Wire Protocol V2 interface for multiplexed binary communication
- WebSocket upgrade on port 8766 (`wire_protocol_websocket.h`, guarded by `THEMIS_ENABLE_WEBSOCKET`)
- Connection pool interfaces for client-side and server-side pooling
- Socket timeout manager with circuit breaker pattern
- QoS manager — per-tenant bandwidth quotas, token bucket, priority queuing
- Protocol buffer wire format parsing utilities
- Network security interfaces (TLS/mTLS configuration)
- Health checking and keepalive abstractions
- Network statistics and monitoring interfaces
- Cross-platform socket abstractions (Windows/Unix)
- UDP fast-path for read-only queries (port 8769)
- QUIC/HTTP3 transport (port 8770, guarded by `THEMIS_ENABLE_HTTP3`)
- gRPC native transport (port 8771, guarded by `THEMIS_ENABLE_GRPC`)
- Geo topology router for geo-distributed cluster routing
- Service mesh integration (Istio/Envoy sidecar, guarded by `THEMIS_ENABLE_SERVICE_MESH`)
- Connection-level compression helpers (LZ4, Zstd — `connection_compression.h`); dictionary-trained Zstd (`ZstdDictionaryCompressor`)
- Batch write processor (`wire_protocol_batch.h`): `WireProtocolBatcher` + `NagleController`
- Zero-copy serialization (`wire_protocol_zero_copy.h`): `ZeroCopyFrameBuilder` + `MemoryMappedPayload`
- UDP ingestion server (`udp_server.h`, port 8768): fire-and-forget metrics/logs/events
- Raft-coordinated load balancer (`raft_load_balancer.h`, port 8774): multi-strategy routing with leader election

**Out of Scope:**
- HTTP/REST API server (handled by api module headers)
- Concrete TLS implementation (delegated to OpenSSL/Boost.Asio)

## Key Components

### Wire Protocol Server Interface
**Location:** `wire_protocol_server.h`

Public interface for ThemisDB's high-performance binary TCP server.

**Purpose:**
- Exposes server configuration options
- Defines lifecycle management (start, stop, wait)
- Provides statistics and monitoring interfaces
- Abstracts session management from clients
- Enables transport security validation

**Key Interfaces:**
```cpp
namespace themis::network {

class WireProtocolServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8766;
        size_t num_io_threads = 4;
        size_t num_worker_threads = std::thread::hardware_concurrency();

        // Security limits
        uint32_t max_connections = 1000;
        uint32_t max_connections_per_ip = 10;
        uint32_t max_frame_size_mb = 64;
        uint32_t connection_timeout_sec = 300;
        uint32_t auth_timeout_sec = 10;
        uint32_t request_timeout_sec = 30;

        // Rate limiting
        uint32_t max_requests_per_second = 1000;
        uint32_t max_requests_per_minute = 10000;

        // TLS configuration
        bool enable_tls = false;
        std::string tls_cert_path;
        std::string tls_key_path;
        std::string tls_ca_cert_path;
        bool tls_require_client_cert = false;  // mTLS

        // Authentication
        bool require_auth = true;
        std::string auth_mechanism = "SCRAM-SHA-256";
    };

    WireProtocolServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<ProcessGraphManager> process_graph,
        std::shared_ptr<TSStore> ts_store,
        std::shared_ptr<ContinuousAggregateManager> agg_manager
    );

    ~WireProtocolServer();

    // Lifecycle management
    bool validateTransportSecurity(int argc, const char* const argv[]) const;
    void start();
    void stop();
    void wait();
    bool isRunning() const;

    // Monitoring
    size_t getActiveConnections() const;

    struct Stats {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t rejected_connections = 0;
        uint64_t total_requests = 0;
        uint64_t total_errors = 0;
        uint64_t auth_failures = 0;
        uint64_t bytes_received = 0;
        uint64_t bytes_sent = 0;
    };
    Stats getStats() const;

private:
    class Session;  // Internal session management
    // ... internal implementation details
};

} // namespace themis::network
```

**Design Patterns:**
- **Builder Pattern**: Configuration via Config struct
- **RAII**: Automatic cleanup on destruction
- **Thread Pool Pattern**: Separate I/O and worker threads
- **Session Pattern**: Per-connection state management

**Configuration Example:**
```cpp
WireProtocolServer::Config config;
config.port = 8766;
config.num_io_threads = 4;
config.num_worker_threads = 16;
config.max_connections = 1000;
config.enable_tls = true;
config.tls_cert_path = "/etc/themisdb/server.crt";
config.tls_key_path = "/etc/themisdb/server.key";
config.require_auth = true;

WireProtocolServer server(config, storage, index_mgr, ...);

if (!server.validateTransportSecurity(argc, argv)) {
    std::cerr << "Transport security validation failed\n";
    return 1;
}

server.start();
server.wait();  // Block until server stops
```

**Thread Safety:** Thread-safe for concurrent access to statistics and lifecycle methods

**Performance Characteristics:**
- Non-blocking I/O via Boost.Asio
- Lock-free statistics (atomic counters)
- Zero-copy buffer management where possible
- Separate I/O and CPU thread pools prevent blocking

---

### Wire Protocol Connection Pool Interface
**Location:** `wire_protocol_connection_pool.h`

Client-side connection pooling for efficient TCP connection reuse.

**Purpose:**
- Reduces TCP handshake overhead
- Reuses authenticated sessions
- Supports both plain TCP and TLS/mTLS
- Provides RAII connection handles
- Implements background connection maintenance

**Key Interfaces:**
```cpp
namespace themis::network {

class SocketWrapper {
public:
    SocketWrapper(std::shared_ptr<tcp::socket> plain_socket);
    SocketWrapper(std::shared_ptr<ssl::stream<tcp::socket>> ssl_socket);

    bool is_open() const;
    void close(boost::system::error_code& ec);
    tcp::socket& lowest_layer();
    bool is_ssl() const;

    tcp::socket* plain_socket();
    ssl::stream<tcp::socket>* ssl_socket();
};

class WireProtocolConnectionPool {
public:
    struct Config {
        size_t min_connections_per_target = 2;
        size_t max_connections_per_target = 20;
        std::chrono::seconds idle_timeout{60};
        std::chrono::seconds connect_timeout{5};
        std::chrono::seconds acquire_timeout{10};
        std::chrono::seconds keepalive_interval{30};

        bool enable_ssl = false;
        bool enable_mtls = false;
        std::string ssl_cert_path;
        std::string ssl_key_path;
        std::string ssl_ca_cert_path;

        size_t max_retries = 3;
        bool enable_warmup = true;
    };

    explicit WireProtocolConnectionPool(const Config& config = Config{});
    ~WireProtocolConnectionPool();

    // Non-copyable, non-movable
    WireProtocolConnectionPool(const WireProtocolConnectionPool&) = delete;
    WireProtocolConnectionPool& operator=(const WireProtocolConnectionPool&) = delete;

    class ConnectionHandle {
    public:
        ConnectionHandle(
            std::shared_ptr<SocketWrapper> socket,
            WireProtocolConnectionPool* pool,
            const std::string& target
        );
        ~ConnectionHandle();  // Automatic return to pool

        // Move-only
        ConnectionHandle(ConnectionHandle&& other) noexcept;
        ConnectionHandle& operator=(ConnectionHandle&& other) noexcept;

        tcp::socket& socket();
        SocketWrapper& socketWrapper();
        bool isValid() const;
    };

    ConnectionHandle acquireConnection(const std::string& target);

    struct Stats {
        size_t total_connections = 0;
        size_t available_connections = 0;
        size_t in_use_connections = 0;
        size_t stale_connections_removed = 0;
        size_t failed_connections = 0;
        size_t acquire_timeouts = 0;
        size_t connections_created = 0;
        size_t connections_reused = 0;
        size_t keepalive_checks_sent = 0;

        double getReuseRate() const;
    };
    Stats getStats() const;

    void warmup(const std::string& target);
    void clear();
    void pruneStaleConnections();

private:
    // ... internal implementation details
};

} // namespace themis::network
```

**Usage Example:**
```cpp
WireProtocolConnectionPool::Config config;
config.min_connections_per_target = 2;
config.max_connections_per_target = 20;
config.enable_ssl = true;
config.ssl_cert_path = "/etc/themisdb/client.crt";
config.ssl_key_path = "/etc/themisdb/client.key";

WireProtocolConnectionPool pool(config);

// Pre-create connections
pool.warmup("server1.example.com:8766");

// RAII connection handle
{
    auto conn = pool.acquireConnection("server1.example.com:8766");
    // Use conn.socket() for I/O operations

    // Connection automatically returned to pool on scope exit
}

// Monitor pool health
auto stats = pool.getStats();
std::cout << "Reuse rate: " << (stats.getReuseRate() * 100) << "%\n";
```

**Design Patterns:**
- **RAII**: ConnectionHandle automatically returns connection to pool
- **Object Pool Pattern**: Reuse expensive connections
- **Factory Pattern**: Per-target pool creation
- **Background Thread Pattern**: Maintenance thread for cleanup

**Thread Safety:** Fully thread-safe with per-target mutexes

---

### Socket Timeout Manager Interface
**Location:** `socket_timeout_manager.h`

Cross-platform socket timeout management with circuit breaker pattern.

**Purpose:**
- Prevents resource exhaustion from hanging connections
- Implements circuit breaker for problematic connections
- Provides platform-independent timeout API (Windows/Unix)
- Configures TCP keepalive and performance options
- Monitors socket health with alerting

**Key Interfaces:**
```cpp
namespace themis::network {

#ifdef _WIN32
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
#endif

struct SocketTimeoutConfig {
    std::chrono::milliseconds accept_timeout{5000};
    std::chrono::milliseconds read_timeout{30000};
    std::chrono::milliseconds write_timeout{30000};
    std::chrono::milliseconds keepalive_interval{60000};
    bool enable_tcp_keepalive{true};
    bool enable_tcp_nodelay{true};
    int max_retry_attempts{3};

    size_t timeout_threshold{10};
    std::chrono::seconds reset_timeout{60};
};

struct SocketTimeoutStats {
    std::atomic<uint64_t> accept_timeouts{0};
    std::atomic<uint64_t> read_timeouts{0};
    std::atomic<uint64_t> write_timeouts{0};
    std::atomic<uint64_t> successful_operations{0};
    std::atomic<uint64_t> failed_operations{0};
    std::atomic<uint64_t> total_bytes_read{0};
    std::atomic<uint64_t> total_bytes_written{0};

    void reset();
    double getTimeoutRate() const;
};

enum class SocketHealthState {
    HEALTHY,
    DEGRADED,
    CIRCUIT_OPEN
};

class SocketTimeoutManager {
public:
    explicit SocketTimeoutManager(const SocketTimeoutConfig& config = SocketTimeoutConfig());
    ~SocketTimeoutManager();

    // Non-copyable, movable
    SocketTimeoutManager(const SocketTimeoutManager&) = delete;
    SocketTimeoutManager& operator=(const SocketTimeoutManager&) = delete;
    SocketTimeoutManager(SocketTimeoutManager&&) = default;
    SocketTimeoutManager& operator=(SocketTimeoutManager&&) = default;

    bool configureSocket(socket_t socket);

    socket_t acceptWithTimeout(socket_t server_socket,
                                std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));

    ssize_t readWithTimeout(socket_t socket, void* buffer, size_t size,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));

    ssize_t writeWithTimeout(socket_t socket, const void* buffer, size_t size,
                             std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));

    void closeSocket(socket_t socket);

    bool shouldAcceptConnection() const;
    void recordTimeout();
    void recordSuccess();

    SocketHealthState getHealthState() const;
    const SocketTimeoutStats& getStats() const;
    void resetStats();
    const SocketTimeoutConfig& getConfig() const;

    void setAlertCallback(std::function<void(SocketHealthState, const std::string&)> callback);

private:
    // ... internal implementation details
};

class SocketTimeoutGuard {
public:
    SocketTimeoutGuard(SocketTimeoutManager& manager, socket_t socket);
    ~SocketTimeoutGuard();

    // Non-copyable, movable
    SocketTimeoutGuard(const SocketTimeoutGuard&) = delete;
    SocketTimeoutGuard& operator=(const SocketTimeoutGuard&) = delete;
    SocketTimeoutGuard(SocketTimeoutGuard&& other) noexcept;

    socket_t get() const;
    socket_t release();
    bool valid() const;
};

} // namespace themis::network
```

**Usage Example:**
```cpp
SocketTimeoutConfig config;
config.accept_timeout = std::chrono::milliseconds(5000);
config.read_timeout = std::chrono::milliseconds(30000);
config.enable_tcp_keepalive = true;
config.timeout_threshold = 10;

SocketTimeoutManager manager(config);

// Configure alert callback
manager.setAlertCallback([](SocketHealthState state, const std::string& msg) {
    if (state == SocketHealthState::CIRCUIT_OPEN) {
        std::cerr << "CRITICAL: Circuit opened - " << msg << std::endl;
    }
});

// Accept with timeout
auto client_socket = manager.acceptWithTimeout(server_socket);
if (client_socket == INVALID_SOCKET_VALUE) {
    // Timeout or error
    return;
}

// RAII guard for automatic cleanup
SocketTimeoutGuard guard(manager, client_socket);

// Read with timeout
std::vector<char> buffer(4096);
ssize_t bytes = manager.readWithTimeout(guard.get(), buffer.data(), buffer.size());

if (bytes > 0) {
    guard.release();  // Keep socket open
}
// Otherwise socket is automatically closed
```

**Design Patterns:**
- **RAII**: SocketTimeoutGuard for automatic socket cleanup
- **Circuit Breaker Pattern**: Automatic failure detection and recovery
- **Observer Pattern**: Alert callbacks for health state changes
- **Strategy Pattern**: Configurable timeout strategies

**Platform Abstraction:**
Works seamlessly on both Windows (SOCKET) and Unix (int file descriptor)

**Thread Safety:** Thread-safe for concurrent socket operations

---

### Wire Protocol Helpers Interface
**Location:** `wire_protocol_helpers.h`

Lightweight protocol buffer wire format parsing without libprotobuf dependency.

**Purpose:**
- Manual protobuf parsing for production use
- Zero external dependency (no libprotobuf)
- Efficient serialization/deserialization
- Specialized helpers for timeseries messages

**Key Interfaces:**
```cpp
namespace themis::network {

class ProtobufParser {
public:
    explicit ProtobufParser(const std::vector<uint8_t>& data);

    bool readVarint(uint64_t& value);
    bool readFixed64(uint64_t& value);
    bool readFixed32(uint32_t& value);
    bool readLengthDelimited(std::vector<uint8_t>& value);
    bool readString(std::string& value);
    bool readTag(uint32_t& field_number, uint32_t& wire_type);
    bool skipField(uint32_t wire_type);

    bool atEnd() const;
    size_t position() const;
};

class ProtobufSerializer {
public:
    ProtobufSerializer() = default;

    void writeVarint(uint64_t value);
    void writeFixed64(uint64_t value);
    void writeFixed32(uint32_t value);
    void writeLengthDelimited(const std::vector<uint8_t>& value);
    void writeString(const std::string& value);
    void writeTag(uint32_t field_number, uint32_t wire_type);
    void writeDouble(double value);

    const std::vector<uint8_t>& data() const;
    std::vector<uint8_t> take();
};

struct TimeSeriesQueryRequest {
    std::string collection;
    uint64_t start_time_ns = 0;
    uint64_t end_time_ns = 0;
    uint32_t aggregation = 0;  // 0=AVG, 1=SUM, 2=MIN, 3=MAX, 4=COUNT
    uint64_t bucket_size_ns = 0;

    static bool parse(const std::vector<uint8_t>& data,
                      TimeSeriesQueryRequest& request);
};

struct TimeSeriesBucket {
    uint64_t timestamp_ns = 0;
    double value = 0.0;
    uint64_t count = 0;
    double min = 0.0;
    double max = 0.0;

    std::vector<uint8_t> serialize() const;
};

struct TimeSeriesStats {
    uint64_t total_data_points = 0;
    uint64_t buckets_returned = 0;
    double data_density = 0.0;

    std::vector<uint8_t> serialize() const;
};

struct TimeSeriesQueryResponse {
    std::vector<TimeSeriesBucket> buckets;
    uint64_t query_time_us = 0;
    TimeSeriesStats stats;

    std::vector<uint8_t> serialize() const;
};

} // namespace themis::network
```

**Usage Example:**
```cpp
// Parsing
ProtobufParser parser(payload_data);

uint32_t field_number, wire_type;
std::string collection;
uint64_t start_time_ns = 0;

while (!parser.atEnd()) {
    if (!parser.readTag(field_number, wire_type)) {
        break;  // Error
    }

    if (field_number == 1) {
        parser.readString(collection);
    } else if (field_number == 2) {
        parser.readVarint(start_time_ns);
    } else {
        parser.skipField(wire_type);
    }
}

// Serialization
ProtobufSerializer serializer;
serializer.writeTag(1, 2);  // field 1, wire_type 2 (length-delimited)
serializer.writeString("metrics");
serializer.writeTag(2, 0);  // field 2, wire_type 0 (varint)
serializer.writeVarint(1234567890ULL);

auto data = serializer.take();
```

**Design Patterns:**
- **Builder Pattern**: Incremental serialization
- **Parser Combinator**: Composable parsing primitives
- **Value Object**: Immutable data structures

**Performance:**
- 5-10x faster than libprotobuf for simple messages
- Zero heap allocations in fast path
- Minimal stack usage (stateless parsing)

**Thread Safety:** Stateless, thread-safe

---

## Integration Patterns

### Server Module Integration
```cpp
// Wire protocol server shares storage with HTTP server
auto storage = std::make_shared<RocksDBWrapper>(storage_config);
auto index_mgr = std::make_shared<SecondaryIndexManager>(storage);

// HTTP server (port 8765)
HTTPServer http_server(http_config, storage, index_mgr);

// Wire protocol server (port 8766, separate thread pool)
WireProtocolServer wire_server(wire_config, storage, index_mgr, ...);

// Both servers share storage, but have isolated I/O
http_server.start();
wire_server.start();
```

### Client Integration
```cpp
// Client-side connection pooling
WireProtocolConnectionPool pool(pool_config);

// Acquire connection
auto conn = pool.acquireConnection("server.example.com:8766");

// Send request
std::vector<uint8_t> request = buildGetRequest(collection, uuid);
boost::asio::write(conn.socket(), boost::asio::buffer(request));

// Receive response
std::vector<uint8_t> response(1024);
size_t bytes = boost::asio::read(conn.socket(), boost::asio::buffer(response));

// Connection automatically returned to pool on scope exit
```

### Monitoring Integration
```cpp
// Collect metrics from all network components
void collectMetrics() {
    auto server_stats = wire_server.getStats();
    auto pool_stats = connection_pool.getStats();
    auto timeout_stats = timeout_manager.getStats();

    // Export to Prometheus
    active_connections.Set(server_stats.active_connections);
    total_requests.Inc(server_stats.total_requests);
    connection_reuse_rate.Set(pool_stats.getReuseRate());
    timeout_rate.Set(timeout_stats.getTimeoutRate());
}
```

---

## Performance Characteristics

| Component | Operation | Latency | Throughput | Notes |
|-----------|-----------|---------|------------|-------|
| Wire Protocol Server | Accept | 1-5 ms | 10K/s | Platform dependent |
| Wire Protocol Server | Frame I/O | 100-500 μs | 100K ops/s | Zero-copy |
| Connection Pool | Acquire (cached) | 10-100 μs | 1M ops/s | Lock-free fast path |
| Connection Pool | Acquire (new) | 10-50 ms | 1K/s | TCP+TLS handshake |
| Socket Timeout | Health check | 1 μs | N/A | Atomic counter |
| Protocol Helpers | Parse/serialize | 1-10 μs | 1M ops/s | Zero-copy parsing |

---

## Best Practices

### Configuration
```cpp
// Production configuration
WireProtocolServer::Config config;
config.enable_tls = true;                // Always enable TLS in production
config.require_auth = true;              // Always require authentication
config.max_connections_per_ip = 10;      // Prevent abuse
config.max_frame_size_mb = 64;           // Prevent memory exhaustion
config.request_timeout_sec = 30;         // Prevent slow loris attacks
```

### Connection Pooling
```cpp
// Warmup connections for hot targets
pool.warmup("primary.example.com:8766");
pool.warmup("secondary.example.com:8766");

// Monitor pool health
auto stats = pool.getStats();
if (stats.getReuseRate() < 0.8) {
    // Low reuse rate, investigate connection issues
}
```

### Error Handling
```cpp
// Always handle timeout errors
try {
    auto conn = pool.acquireConnection(target);
    // Use connection
} catch (const std::runtime_error& e) {
    if (std::string(e.what()).find("timeout") != std::string::npos) {
        // Timeout occurred, retry or fail gracefully
    }
}
```

### Health Monitoring
```cpp
// Set up alerting for circuit breaker
timeout_manager.setAlertCallback([](SocketHealthState state, const std::string& msg) {
    if (state == SocketHealthState::CIRCUIT_OPEN) {
        // Page on-call engineer, circuit opened
        alert_system.send_alert(AlertLevel::CRITICAL, msg);
    }
});
```

---

## Dependencies

**Required Headers:**
```cpp
#include <boost/asio.hpp>               // Async I/O
#include <boost/asio/ssl.hpp>           // TLS support
#include <boost/asio/thread_pool.hpp>   // Thread pool
#include <memory>                        // Smart pointers
#include <string>                        // String operations
#include <vector>                        // Buffer types
#include <chrono>                        // Timeouts
#include <atomic>                        // Lock-free stats
#include <functional>                    // Callbacks
```

**Platform-Specific:**
```cpp
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/tcp.h>
    #include <poll.h>
#endif
```

---

## Thread Safety Guarantees

| Component | Thread Safety | Notes |
|-----------|---------------|-------|
| WireProtocolServer | Thread-safe | Internal locking for shared state |
| WireProtocolConnectionPool | Thread-safe | Per-target mutexes |
| SocketTimeoutManager | Thread-safe | Atomic statistics |
| ProtobufParser | Not thread-safe | Stateful parser (not shared) |
| ProtobufSerializer | Not thread-safe | Stateful serializer (not shared) |
| ConnectionHandle | Thread-safe | RAII, move-only |
| SocketTimeoutGuard | Thread-safe | RAII, move-only |

---

## Known Limitations

1. **Boost.Asio Dependency:** Requires Boost 1.70+ (async I/O library)
2. **OpenSSL Dependency:** Required for TLS/mTLS (no alternative crypto backend)
3. **IPv6 Support:** `Config::enable_ipv6 = true` enables IPv6 binding; `Config::ipv6_dual_stack = true` (default) accepts both IPv4-mapped and native IPv6 clients on a single socket via `IPV6_V6ONLY=0`.
4. **Platform-Specific Code:** Some features (TCP keepalive, TCP_CORK, TCP_NOPUSH) have platform differences
5. **RaftLoadBalancer (Known Issue):** Simulates Raft consensus in-process (no real network RPC between nodes); full distributed multi-node Raft is planned for a future milestone. See `ROADMAP.md` for details.

---

## Version History

- **v1.0.0** - Initial wire protocol server interface
- **v1.1.0** - Added connection pool interface
- **v1.2.0** - Added TLS/mTLS support
- **v1.3.0** - Added socket timeout manager
- **v1.4.0** - Added rate limiting and security features
- **v1.5.0** - Added timeseries protocol helpers
- **v1.6.0** - Added BPMN process orchestration support
- **v1.8.0** - Added UDP ingestion server (`udp_server.h`), Raft load balancer (`raft_load_balancer.h`), batch write processor (`wire_protocol_batch.h`), zero-copy serialization (`wire_protocol_zero_copy.h`), dictionary compression (`connection_compression.h` extended with `ZstdDictionaryCompressor`)
- **v1.9.0** - Added IPv6 dual-stack support (`Config::enable_ipv6`, `Config::ipv6_dual_stack`)

---

## References

- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [Protocol Buffers Wire Format](https://developers.google.com/protocol-buffers/docs/encoding)
- [TCP Socket Programming Guide](https://beej.us/guide/bgnet/)
- [Transport Security Best Practices](../../docs/security/network.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

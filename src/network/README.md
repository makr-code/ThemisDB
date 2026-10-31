---
Author: "ThemisDB Contributors"
Created: "unknown (pre-existing document; creation date not recorded)"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> **Build/tests:** `cmake --preset linux-release -DTHEMIS_BUILD_TESTS=ON`, then build in `build-gcc-linux-release`.

# ThemisDB Network Module - Implementation

<!-- Status: PRODUCTION_CANDIDATE per root roadmap; source review: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/network/README.md -->
<!-- Primärdokumentation: src/network/ -->

## Module Purpose

The Network module implements ThemisDB's networking layer for distributed communication, client connections, and inter-node messaging. It provides TCP/IP socket management, connection pooling, TLS/mTLS configuration, protocol handling, and distributed communication patterns. Performance depends on build options and deployment; see the benchmark evidence notes below.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `wire_protocol_server.cpp` | Core TCP server (port 8766) with async I/O, rate limiting, and opcode dispatch |
| `wire_protocol_connection_pool.cpp` | Client-side connection pool lifecycle, health tracking, and RAII handles |
| `wire_protocol_helpers.cpp` | Wire protocol frame parsing and serialization (lightweight, no libprotobuf) |
| `wire_protocol_v2.cpp` | V2 multiplexed protocol: multi-stream, flow control, server push |
| `wire_protocol_server_ws.cpp` | WebSocket upgrade on port 8766 (guarded by `THEMIS_ENABLE_WEBSOCKET`) |
| `wire_protocol_performance.cpp` | Performance monitoring and benchmarking helpers |
| `socket_timeout_manager.cpp` | Socket timeout enforcement and circuit breaker |
| `qos_manager.cpp` | Per-tenant bandwidth quotas, priority queuing, token bucket |
| `udp_fast_path.cpp` | UDP read-only fast-path, port 8769 |
| `quic_transport.cpp` | QUIC/HTTP3 transport, port 8770 (guarded by `THEMIS_ENABLE_HTTP3`) |
| `grpc_transport.cpp` | gRPC native transport, port 8771 (guarded by `THEMIS_ENABLE_GRPC`) |
| `geo_topology_router.cpp` | Network topology-aware routing for geo-distributed clusters |
| `service_mesh.cpp` | Istio/Envoy sidecar probe server (guarded by `THEMIS_ENABLE_SERVICE_MESH`) |
| `envoy_xds.cpp` | Envoy xDS v3 REST polling client (guarded by `THEMIS_ENABLE_SERVICE_MESH`) |
| `adaptive_circuit_breaker.cpp` | `AdaptiveCircuitBreaker` — CLOSED/OPEN/HALF_OPEN state machine with load-adaptive threshold |
| `connection_compression.cpp` | `ZstdDictionaryCompressor` — dictionary-trained Zstd compression for wire payloads |
| `wire_protocol_batch.cpp` | `WireProtocolBatcher` (writev coalescing) + `NagleController` (TCP_CORK/TCP_NOPUSH) |
| `wire_protocol_zero_copy.cpp` | `ZeroCopyFrameBuilder` (writev, no heap alloc) + `MemoryMappedPayload` (mmap/sendfile) |
| `udp_server.cpp` | `UDPServer` — fire-and-forget ingestion on port 8768 (METRIC, LOG, EVENT, BATCH, PING) |
| `raft_load_balancer.cpp` | `RaftLoadBalancer` — 5 routing strategies, health-based failover, consistent hashing (port 8774) |
| `io_uring_batcher.cpp` | `IoUringBatchedSender` — single `io_uring_enter` for N concurrent writev SQEs (guarded by `THEMIS_ENABLE_IO_URING`) |
| `kernel_bypass.cpp` | `DPDKServer` + `IoUringServer` + `CpuPinner` + `NumaAllocator` + `ZeroCopyDmaBuffer` |
| `network_audit_log.cpp` | `NetworkAuditLog` — standalone event buffer/callback implementation; production call-site integration is not verified |
| `quic_server.cpp` | `QUICServer` + `QUICClient` — QUIC/HTTP3 server with 0-RTT, BBR/Cubic congestion control (guarded by `THEMIS_ENABLE_HTTP3`) |
| `adaptive_io_scaler.h` | `AdaptiveIOScaler` — background I/O thread scaler based on active connection ratio (header-only) |

## Current Delivery Status

**Maturity:** PRODUCTION_CANDIDATE (per the root [roadmap](../../ROADMAP.md)). Transport and protocol capabilities vary by build option, runtime configuration, and integration; this status does not assert that every listed transport or feature is enabled or production-ready in every deployment. Core Wire Protocol V1 handlers (HELLO, AUTH, GET, PUT, DELETE, VECTOR_SEARCH) are implemented; QUERY_AQL and GEO_QUERY return structured errors directing clients to the HTTP REST API while engine integration remains pending.

**Historical validation:** 2026-03-10. This date is not evidence of the current
implementation; verify behavior against the linked headers/source and focused
tests when making changes.

### Current Security & Validation Notes

- The QoS manager command-injection issue has been fixed in the active code path: `src/network/qos_manager.cpp` now validates interface names before invoking `tc` and uses `posix_spawn()` with an explicit `argv[]` array instead of shell-based `std::system()` execution.
- The regression coverage is in `tests/network/test_qos_manager_command_injection.cpp` and exercises malicious interface names containing shell metacharacters, whitespace, traversal patterns, length violations, and leading-hyphen cases.
- This module should be treated as source-of-truth evidence for the live state; archived gap reports and stale scanner output are not authoritative when they contradict current implementation or tests.
- Full CMake/CTest validation in this sandbox remains blocked by dependency bootstrap/network resolution issues, so end-to-end CI confirmation still depends on a network-enabled build environment.

## Scope

**In Scope:**
- Wire Protocol Server (binary TCP protocol)
- Client-side connection pool management
- Socket timeout management with circuit breaker pattern
- TLS/SSL and mutual TLS (mTLS) support
- Protocol buffer wire format helpers (lightweight parser/serializer)
- Network security (authentication, rate limiting, connection limits)
- Health checking and keepalive mechanisms
- Bounded retry policy contexts for eligible transient failures; application
  request replay remains caller-owned
- Connection lifecycle management
- Performance monitoring and statistics collection

**Out of Scope:**
- HTTP/REST API server (handled by api module)
- gRPC API service layer (handled by server/api modules; `grpc_transport.cpp` provides transport only)
- Custom application protocols above wire protocol layer

## Key Components

### Wire Protocol Server
**Location:** `wire_protocol_server.cpp` (1224 lines)
**Header:** `../../include/network/wire_protocol_server.h`

High-performance binary TCP server for native client communication with ThemisDB.

**Purpose:**
- Provides low-latency binary protocol alternative to HTTP/REST
- Enables efficient bulk operations and streaming
- Supports implemented wire opcodes; query/geospatial operations depend on
  their configured engine/backend integrations
- Implements comprehensive security (TLS, authentication, rate limiting)
- Isolates network I/O from request processing via separate thread pools

**Key Features:**
```cpp
WireProtocolServer::Config config;
config.port = 8766;
config.num_io_threads = 4;           // Dedicated I/O threads
config.num_worker_threads = 16;      // Request processing threads
config.max_connections = 1000;
config.max_connections_per_ip = 10;
config.enable_tls = true;
config.require_auth = true;

// Security limits
config.max_frame_size_mb = 64;
config.connection_timeout_sec = 300;
config.auth_timeout_sec = 10;
config.request_timeout_sec = 30;

// Rate limiting (per IP)
config.max_requests_per_second = 1000;
config.max_requests_per_minute = 10000;

// IPv6 dual-stack (optional — defaults to IPv4-only)
// config.enable_ipv6    = true;  // bind to "::" instead of "0.0.0.0"
// config.ipv6_dual_stack = true; // IPV6_V6ONLY=0: accepts IPv4-mapped connections too

WireProtocolServer server(
    config, storage, secondary_index, graph_index,
    vector_index, tx_manager, process_graph,
    ts_store, agg_manager, query_engine
);

// Validate transport security before starting
if (!server.validateTransportSecurity(argc, argv)) {
    throw std::runtime_error("Wire Protocol transport security validation failed");
}

// start() also requires query_engine and a geospatial backend/bridge.
server.start();
```

**Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│                 Wire Protocol Server                      │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────────┐         ┌────────────────┐            │
│  │ IO Context   │────────▶│  Acceptor      │            │
│  │ (Boost.Asio) │         │  (TCP Socket)  │            │
│  └──────────────┘         └────────────────┘            │
│         │                         │                       │
│         │                         │ accept()              │
│         │                         ▼                       │
│         │              ┌──────────────────┐              │
│         │              │ Session objects   │              │
│         │              │ (per connection)  │              │
│         │              └──────────────────┘              │
│         │                         │                       │
│         │                         │ async_read/write      │
│         ▼                         ▼                       │
│  ┌──────────────┐       ┌────────────────┐              │
│  │ IO Threads   │       │  Sessions      │              │
│  │ (configured) │──────▶│ (configured cap)│              │
│  └──────────────┘       └────────────────┘              │
│                                  │                        │
│                                  │ dispatch               │
│                                  ▼                        │
│                         ┌────────────────┐               │
│                         │ Worker Pool    │               │
│                         │ (configured)   │               │
│                         └────────────────┘               │
│                                  │                        │
│                                  │ execute                │
│                                  ▼                        │
│                         ┌────────────────┐               │
│                         │ Storage Layer  │               │
│                         │ Indexes, etc.  │               │
│                         └────────────────┘               │
└─────────────────────────────────────────────────────────┘
```

**Session Management:**
Each client connection is represented by a `Session` object that:
- Manages authentication state machine
- Handles frame parsing (header + payload + checksum)
- Dispatches OpCode-based message handlers
- Maintains per-connection write queue (prevents write-write races)
- Implements timeout protection for all operations
- Tracks per-session statistics

**OpCode Handlers:**
```cpp
// Connection lifecycle
void handleHello();           // Protocol handshake — implemented (returns server info + capabilities)
void handleAuthRequest();     // Token-based authentication — implemented (sets authenticated_ flag)
void handlePing();            // Keepalive — implemented
void handleClose();           // Graceful close — implemented

// CRUD operations — implemented (dispatch to RocksDBWrapper via collection:key prefix)
void handleGet();             // Single entity retrieval
void handlePut();             // Entity storage
void handleDelete();          // Entity deletion

// Query operations
void handleQuery();           // AQL query — returns structured error; use HTTP REST API
void handleVectorSearch();    // Vector similarity search — implemented (VectorIndexManager::searchKnn)
void handleGeoQuery();        // Geospatial query — returns structured error; use HTTP REST API
void handleTimeseriesQuery(); // Timeseries aggregation — implemented (TSStore dispatch)

// Process orchestration (BPMN) — implemented (requires authentication guard)
void handleBpmnStartProcess();
void handleBpmnTaskComplete();
void handleBpmnQueryInstance();
```

**Security Features:**
1. **Transport Security:**
   - TLS 1.2+ with strong cipher suites
   - Mutual TLS (mTLS) for client certificate validation
   - Certificate validation and revocation checks
   - Production safety enforcement (validates TLS in production mode)

2. **Authentication:**
   - `auth_mechanism` accepts the configured `SCRAM-SHA-256` label, while the
     current wire AUTH handler validates a pre-shared JSON token; it does not
     perform a SCRAM challenge-response exchange
   - Per-session authentication state
   - Configurable auth timeout (default 10s)
   - Failed auth attempt tracking

3. **Rate Limiting:**
   - Per-IP connection limits
   - Per-IP request rate limits (per second, per minute)
   - Automatic rejection of excessive connections

4. **Connection Protection:**
   - Maximum frame size limit (prevents memory exhaustion)
   - Connection timeout (idle connections auto-close)
   - Request timeout (prevents long-running requests)
   - Graceful shutdown with in-flight request completion

**Performance Optimizations:**
- **Thread Pool Isolation:** Separate I/O and worker threads prevent blocking
- **Zero-Copy I/O:** Direct buffer access where possible
- **Statistics:** Component snapshots with synchronization where shared state is accessed
- **Async I/O:** Boost.Asio for efficient event-driven I/O
- **Connection Pooling:** Reuse connections to reduce handshake overhead

**Wire Protocol Format:**
```
Frame Structure:
┌──────────┬─────────┬─────────┬──────────┬─────────────┬────────────┐
│ Magic    │ Version │ OpCode  │ Flags    │ PayloadSize │ Payload+CRC│
│ 4 bytes  │ 1 byte  │ 1 byte  │ 2 bytes  │ 4 bytes     │ variable   │
└──────────┴─────────┴─────────┴──────────┴─────────────┴────────────┘

The TCP wire server uses a 12-byte header: magic `TMDB` (`0x544D4442`),
version, opcode, flags, and a big-endian payload length. The body is JSON for
the current server handlers; a CRC32 follows the payload and covers the header
and payload. This is separate from the standalone `PacketParser` format in
`include/network/wire_protocol.h` (`0xDB 0x01`, 7-byte header).
```

**Error Handling:**
```cpp
// The server API accepts a numeric response code; NetworkErrorCode is a
// separate canonical contract taxonomy in network_api_contract.h.
void sendError(uint32_t error_code, const std::string& message);
```

**Statistics:**
```cpp
struct Stats {
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t rejected_connections;  // Rate limit/max conn
    uint64_t total_requests;
    uint64_t total_errors;
    uint64_t auth_failures;
    uint64_t bytes_received;
    uint64_t bytes_sent;
};

auto stats = server.getStats();
double error_rate = stats.total_requests == 0 ? 0.0 :
    static_cast<double>(stats.total_errors) / stats.total_requests;
```

Shared server state uses synchronization, but callers must not infer that every
returned snapshot or callback is safe for concurrent mutation.

---

### Wire Protocol Connection Pool
**Location:** `wire_protocol_connection_pool.cpp` (599 lines)
**Header:** `../../include/network/wire_protocol_connection_pool.h`

Client-side connection pooling for efficient reuse of TCP connections.

**Purpose:**
- Reduce TCP handshake overhead (3-way handshake + TLS handshake)
- Reuse authenticated sessions
- Improve throughput under high concurrency
- Lower latency for subsequent requests
- Support both plain TCP and TLS/mTLS connections

**Key Features:**
```cpp
WireProtocolConnectionPool::Config config;
config.min_connections_per_target = 2;
config.max_connections_per_target = 20;
config.idle_timeout = std::chrono::seconds(60);
config.connect_timeout = std::chrono::seconds(5);
config.acquire_timeout = std::chrono::seconds(10);
config.keepalive_interval = std::chrono::seconds(30);
config.enable_ssl = true;
config.enable_mtls = true;
config.ssl_cert_path = "/path/to/client.crt";
config.ssl_key_path = "/path/to/client.key";
config.ssl_ca_cert_path = "/path/to/ca.crt";
config.max_retries = 3;
config.enable_warmup = true;

WireProtocolConnectionPool pool(config);

// Pre-create connections for hot targets
pool.warmup("server1.example.com:8766");
pool.warmup("server2.example.com:8766");

// Acquire connection (RAII handle)
{
    auto conn = pool.acquireConnection("server1.example.com:8766");
    // socket() is plain TCP only; use socketWrapper() for SSL connections.

    // Connection automatically returned to pool when `conn` goes out of scope
}
```

**Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│          Wire Protocol Connection Pool                   │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  Per-Target Pools:                                       │
│  ┌─────────────────────────────────────────────┐        │
│  │ server1.example.com:8766                     │        │
│  │ ┌─────────┬─────────┬─────────┬─────────┐  │        │
│  │ │ Conn 1  │ Conn 2  │ Conn 3  │ ...     │  │        │
│  │ │ (idle)  │ (in-use)│ (idle)  │         │  │        │
│  │ └─────────┴─────────┴─────────┴─────────┘  │        │
│  └─────────────────────────────────────────────┘        │
│                                                           │
│  ┌─────────────────────────────────────────────┐        │
│  │ server2.example.com:8766                     │        │
│  │ ┌─────────┬─────────┬─────────┐             │        │
│  │ │ Conn 1  │ Conn 2  │ Conn 3  │             │        │
│  │ │ (idle)  │ (idle)  │ (in-use)│             │        │
│  │ └─────────┴─────────┴─────────┘             │        │
│  └─────────────────────────────────────────────┘        │
│                                                           │
│  Background Maintenance Thread:                          │
│  - Remove stale connections (idle > timeout)             │
│  - Send keepalive pings                                  │
│  - Health check connections                              │
│  - Enforce min/max connection limits                     │
└─────────────────────────────────────────────────────────┘
```

**SocketWrapper Abstraction:**
```cpp
class SocketWrapper {
    // Supports both plain and SSL sockets
    std::shared_ptr<tcp::socket> plain_socket_;
    std::shared_ptr<ssl::stream<tcp::socket>> ssl_socket_;

public:
    bool is_open() const;
    void close(boost::system::error_code& ec);
    tcp::socket& lowest_layer();
    bool is_ssl() const;
};
```

**RAII Connection Handle:**
```cpp
class ConnectionHandle {
    // Automatic return to pool on destruction
    ~ConnectionHandle() {
        pool_->releaseConnection(target_, socket_);
    }

public:
    tcp::socket& socket();               // Backward compatible
    SocketWrapper& socketWrapper();      // Access SSL/plain wrapper
    bool isValid() const;
};
```

**Pooling Strategy:**
1. **Lazy Creation:** Connections created on demand
2. **Warmup:** Pre-create minimum connections for hot targets
3. **Stale Removal:** Background thread removes idle connections
4. **Health Checks:** Periodic keepalive pings verify connection health
5. **Automatic Reconnection:** Failed connections are recreated

**Statistics:**
```cpp
struct Stats {
    size_t total_connections;
    size_t available_connections;
    size_t in_use_connections;
    size_t stale_connections_removed;
    size_t failed_connections;
    size_t acquire_timeouts;
    size_t connections_created;
    size_t connections_reused;
    size_t keepalive_checks_sent;

    double getReuseRate() const;  // 0.0 - 1.0
};

auto stats = pool.getStats();
std::cout << "Connection reuse rate: " << (stats.getReuseRate() * 100) << "%\n";
```

**Performance Benefits:**
- Reuse avoids a new connection handshake when an eligible connection is
  available. The module has no benchmark proving a fixed latency or throughput
  improvement; results depend on workload and deployment.

**TLS/mTLS Support:**
- Configured TLS certificate/key and CA paths
- Client certificate authentication (mTLS)
- Exact verification/session behavior depends on the transport implementation

The pool synchronizes shared per-target state. A checked-out connection handle
is owned by its caller and should not be used concurrently by multiple threads.

---

### Socket Timeout Manager
**Location:** `socket_timeout_manager.cpp` (422 lines)
**Header:** `../../include/network/socket_timeout_manager.h`

Cross-platform socket timeout management with circuit breaker pattern.

**Purpose:**
- Prevent hanging connections and resource exhaustion
- Implement circuit breaker for problematic connections
- Provide platform-independent timeout API (Windows/Unix)
- Monitor socket health and trigger alerts
- Configure TCP keepalive and performance options

**Key Features:**
```cpp
SocketTimeoutConfig config;
config.accept_timeout = std::chrono::milliseconds(5000);     // 5s
config.read_timeout = std::chrono::milliseconds(30000);      // 30s
config.write_timeout = std::chrono::milliseconds(30000);     // 30s
config.keepalive_interval = std::chrono::milliseconds(60000); // 60s
config.enable_tcp_keepalive = true;
config.enable_tcp_nodelay = true;  // Disable Nagle's algorithm
config.max_retry_attempts = 3;

// Circuit breaker thresholds
config.timeout_threshold = 10;  // Open circuit after 10 timeouts
config.reset_timeout = std::chrono::seconds(60);  // Retry after 60s

SocketTimeoutManager manager(config);

// Accept with timeout
auto client_socket = manager.acceptWithTimeout(server_socket);
if (client_socket == INVALID_SOCKET_VALUE) {
    // Timeout or error
}

// Read with timeout
std::vector<char> buffer(4096);
ssize_t bytes = manager.readWithTimeout(client_socket, buffer.data(), buffer.size());

// Write with timeout
ssize_t sent = manager.writeWithTimeout(client_socket, data.data(), data.size());
```

**Circuit Breaker Pattern:**
```
Health States:
┌─────────┐  consecutive_timeouts < threshold
│ HEALTHY │◀──────────────────────────────────┐
└─────────┘                                     │
     │                                          │
     │ timeouts increasing                      │ reset after timeout period
     ▼                                          │
┌─────────┐                                     │
│DEGRADED │                                     │
└─────────┘                                     │
     │                                          │
     │ consecutive_timeouts >= threshold        │
     ▼                                          │
┌─────────┐                                     │
│ CIRCUIT │                                     │
│  OPEN   │─────────────────────────────────────┘
└─────────┘
   (refuse new connections temporarily)
```

**Health Monitoring:**
```cpp
enum class SocketHealthState {
    HEALTHY,      // Normal operation
    DEGRADED,     // Experiencing timeouts but operational
    CIRCUIT_OPEN  // Too many timeouts, refuse connections
};

manager.setAlertCallback([](SocketHealthState state, const std::string& msg) {
    if (state == SocketHealthState::CIRCUIT_OPEN) {
        std::cerr << "ALERT: Circuit opened - " << msg << std::endl;
        // Trigger monitoring alert, page on-call engineer, etc.
    }
});
```

**Platform Abstraction:**
```cpp
#ifdef _WIN32
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
#endif

// Unified API works on both Windows and Unix
bool setNonBlocking(socket_t socket);
bool configureTCPKeepalive(socket_t socket);
bool configureTCPNoDelay(socket_t socket);
```

**Statistics:**
```cpp
struct SocketTimeoutStats {
    std::atomic<uint64_t> accept_timeouts;
    std::atomic<uint64_t> read_timeouts;
    std::atomic<uint64_t> write_timeouts;
    std::atomic<uint64_t> successful_operations;
    std::atomic<uint64_t> failed_operations;
    std::atomic<uint64_t> total_bytes_read;
    std::atomic<uint64_t> total_bytes_written;

    double getTimeoutRate() const;
};

auto stats = manager.getStats();
std::cout << "Timeout rate: " << (stats.getTimeoutRate() * 100) << "%\n";
```

**RAII Guard:**
```cpp
auto manager = std::make_shared<SocketTimeoutManager>(config);
SocketTimeoutGuard guard(manager, client_socket);

// Perform operations
auto bytes = manager->readWithTimeout(guard.get(), buffer, size);

if (success) {
    guard.release();  // Keep socket open
}
// Otherwise socket is automatically closed on scope exit
```

Do not overlap operations on the same socket. The public interface does not
promise blanket synchronization for a shared manager or socket.

---

### Wire Protocol Helpers
**Location:** `wire_protocol_helpers.cpp` (320 lines)
**Header:** `../../include/network/wire_protocol_helpers.h`

Lightweight protobuf wire format parser/serializer without protobuf library dependency.

**Purpose:**
- Manual protobuf parsing for production use (no libprotobuf dependency)
- Efficient serialization/deserialization of wire protocol messages
- Support for varint encoding, length-delimited fields, fixed-width integers
- Specialized parsers for timeseries query messages

**Key Features:**
```cpp
// Parsing example
ProtobufParser parser(payload_data);

uint32_t field_number, wire_type;
while (!parser.atEnd()) {
    if (!parser.readTag(field_number, wire_type)) {
        // Error handling
        break;
    }

    switch (field_number) {
        case 1: {  // collection field
            std::string collection;
            parser.readString(collection);
            break;
        }
        case 2: {  // start_time_ns field
            uint64_t start_time;
            parser.readVarint(start_time);
            break;
        }
        default:
            parser.skipField(wire_type);  // Unknown field
    }
}

// Serialization example
ProtobufSerializer serializer;
serializer.writeTag(1, 2);  // field 1, wire_type 2 (length-delimited)
serializer.writeString("metrics");
serializer.writeTag(2, 0);  // field 2, wire_type 0 (varint)
serializer.writeVarint(1234567890);

auto data = serializer.take();
```

**Protobuf Wire Types:**
```cpp
enum WireType {
    VARINT = 0,           // int32, int64, uint32, uint64, bool, enum
    FIXED64 = 1,          // fixed64, sfixed64, double
    LENGTH_DELIMITED = 2, // string, bytes, embedded messages
    FIXED32 = 5           // fixed32, sfixed32, float
};
```

**Timeseries Message Helpers:**
```cpp
// Parse timeseries query request
struct TimeSeriesQueryRequest {
    std::string collection;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint32_t aggregation;  // 0=AVG, 1=SUM, 2=MIN, 3=MAX, 4=COUNT
    uint64_t bucket_size_ns;

    static bool parse(const std::vector<uint8_t>& data,
                      TimeSeriesQueryRequest& request);
};

// Serialize timeseries query response
struct TimeSeriesQueryResponse {
    std::vector<TimeSeriesBucket> buckets;
    uint64_t query_time_us;
    TimeSeriesStats stats;

    std::vector<uint8_t> serialize() const;
};
```

**Performance:** No current network benchmark compares these helpers with
libprotobuf or establishes a speedup/allocation guarantee.

`ProtobufParser` holds a reference to input bytes and a mutable cursor;
`ProtobufSerializer` owns a mutable output buffer. Use separate instances for
concurrent operations; neither class provides internal synchronization.

---

### Protocol Buffer Definitions
**Location:** `themis_wire_v1.proto` (383 lines)

Protobuf schema for wire protocol messages (reference documentation).

**Purpose:**
- Define wire protocol message format
- Document message structure for client implementations
- Generate code for official client libraries
- Serve as protocol specification

**Key Messages:**
```protobuf
// Connection lifecycle
message HelloRequest { ... }
message HelloAck { ... }
message AuthRequest { ... }
message AuthResponse { ... }
message AuthSuccess { ... }

// CRUD operations
message GetRequest { ... }
message GetResponse { ... }
message PutRequest { ... }
message PutResponse { ... }
message DeleteRequest { ... }
message DeleteResponse { ... }

// Query operations
message QueryRequest { ... }
message QueryResponse { ... }
message VectorSearchRequest { ... }
message VectorSearchResponse { ... }

// Timeseries operations
message TimeSeriesQueryRequest { ... }
message TimeSeriesQueryResponse { ... }
message TimeSeriesBucket { ... }

// BPMN process operations
message StartProcessRequest { ... }
message StartProcessResponse { ... }
```

**Protocol Version:** v1.0.0
**Syntax:** proto3
**Optimization:** `optimize_for = SPEED`
**Arenas:** `cc_enable_arenas = true`

---

## Integration with Other Modules

### Server Module
- Wire Protocol Server integrates with HTTP server via shared storage/index managers
- Separate thread pools prevent interference
- Unified authentication and authorization
- Shared monitoring and metrics collection

### Storage Module
- Direct access to RocksDBWrapper for key-value operations
- Transaction manager integration for ACID guarantees
- Snapshot isolation for consistent reads

### Index Module
- Secondary index queries via SecondaryIndexManager
- Graph traversals via GraphIndexManager
- Vector similarity search via VectorIndexManager
- Timeseries aggregations via TSStore

### Security Module
- Transport security validation (TLS enforcement)
- Shared-token AUTH handler; `auth_mechanism` currently accepts the
  `SCRAM-SHA-256` configuration label but does not implement a SCRAM exchange
- Rate limiting and connection limits
- `NetworkAuditLog` exists as a standalone buffer/callback API; production
  event wiring remains unverified (see `PRODUCTION_REQUIREMENTS.md`)

### Performance Module
- Connection pooling reduces handshake overhead
- Zero-copy I/O where possible
- Synchronized/atomic statistics where declared by component interfaces
- Async I/O for efficient event handling

---

## Performance Optimization

### Zero-Copy I/O
```cpp
// Direct buffer access without intermediate copies
void asyncReadPayload(uint32_t payload_size) {
    payload_buffer_.resize(payload_size);
    async_read(socket_, buffer(payload_buffer_),
               [this](const error_code& ec, size_t bytes) {
        // payload_buffer_ contains data without copy
    });
}
```

### Lock-Free Statistics
```cpp
// Atomic counters avoid lock contention
std::atomic<uint64_t> total_requests_{0};
std::atomic<uint64_t> bytes_received_{0};

// Increment without locks
total_requests_.fetch_add(1, std::memory_order_relaxed);
```

### Thread Pool Tuning
```cpp
// Separate I/O and worker threads
config.num_io_threads = 4;        // Network I/O (accept, read, write)
config.num_worker_threads = 16;   // Request processing (CPU-bound)

// Prevents CPU-heavy requests from blocking I/O
```

### Connection Reuse
```cpp
// Connection pooling reduces handshake overhead
// TCP handshake: 3 RTT (SYN, SYN-ACK, ACK)
// TLS handshake: 2 RTT (ClientHello, ServerHello, etc.)
// Auth handshake: 2 RTT (challenge, response)
// Total: 7 RTT saved per reused connection

auto conn = pool.acquireConnection(target);
// Connection already established, authenticated, ready
```

### Keepalive Optimization
```cpp
// TCP keepalive prevents connection timeouts
config.enable_tcp_keepalive = true;
config.keepalive_interval = std::chrono::seconds(60);

// Sends TCP keepalive probes to detect dead connections
```

### Nagle's Algorithm Disable
```cpp
// Disable Nagle's algorithm for low latency
config.enable_tcp_nodelay = true;

// Trades bandwidth for latency (immediate send)
```

---

## Network Security

### TLS/mTLS Configuration
```cpp
// Server-side TLS
config.enable_tls = true;
config.tls_cert_path = "/etc/themisdb/server.crt";
config.tls_key_path = "/etc/themisdb/server.key";
config.tls_ca_cert_path = "/etc/themisdb/ca.crt";
config.tls_require_client_cert = true;  // Enable mTLS

// Client-side TLS
pool_config.enable_ssl = true;
pool_config.enable_mtls = true;
pool_config.ssl_cert_path = "/etc/themisdb/client.crt";
pool_config.ssl_key_path = "/etc/themisdb/client.key";
pool_config.ssl_ca_cert_path = "/etc/themisdb/ca.crt";
```

### Authentication
```cpp
// Token-based authentication in the current wire AUTH handler.
// handleAuthRequest() validates {"token":"...","username":"..."} payload.
// Config::auth_token sets the expected pre-shared token. When authentication is
// required, server startup rejects an empty or blank token.
// On success: authenticated_ = true; username_ is recorded.
// On failure: error 0x0401; stats_.auth_failures incremented.
config.require_auth = true;
config.auth_timeout_sec = 10;
config.auth_token = "my-secret-token";  // required when require_auth is true
```

### Rate Limiting
```cpp
// Per-IP rate limiting
config.max_requests_per_second = 1000;
config.max_requests_per_minute = 10000;

// The server applies these configured per-IP limits before request dispatch.
```

### Connection Limits
```cpp
// Global and per-IP limits
config.max_connections = 1000;
config.max_connections_per_ip = 10;

// Circuit breaker for repeated failures
if (consecutive_timeouts >= config.timeout_threshold) {
    health_state_ = SocketHealthState::CIRCUIT_OPEN;
    // Refuse new connections until reset timeout
}
```

---

## Monitoring and Observability

### Metrics Collection
```cpp
// Server statistics
auto stats = server.getStats();
std::cout << "Active connections: " << stats.active_connections << "\n";
std::cout << "Total requests: " << stats.total_requests << "\n";
std::cout << "Error rate: " << (double)stats.total_errors / stats.total_requests << "\n";
std::cout << "Auth failures: " << stats.auth_failures << "\n";

// Connection pool statistics
auto pool_stats = pool.getStats();
std::cout << "Reuse rate: " << (pool_stats.getReuseRate() * 100) << "%\n";
std::cout << "Stale connections removed: " << pool_stats.stale_connections_removed << "\n";

// Socket timeout statistics
auto timeout_stats = manager.getStats();
std::cout << "Timeout rate: " << (timeout_stats.getTimeoutRate() * 100) << "%\n";
std::cout << "Read timeouts: " << timeout_stats.read_timeouts << "\n";
```

### Health Checks
```cpp
// Health state monitoring
manager.setAlertCallback([](SocketHealthState state, const std::string& msg) {
    switch (state) {
        case SocketHealthState::DEGRADED:
            std::cerr << "WARNING: " << msg << std::endl;
            break;
        case SocketHealthState::CIRCUIT_OPEN:
            std::cerr << "CRITICAL: " << msg << std::endl;
            // Trigger alert, page on-call, etc.
            break;
        default:
            break;
    }
});
```

### Prometheus Integration

The network module does not bundle a Prometheus exporter. A consuming service
may adapt component snapshots or register its own observability sink.

```cpp
// Export metrics to Prometheus
prometheus::Counter& requests_total = /* ... */;
prometheus::Histogram& request_duration = /* ... */;
prometheus::Gauge& active_connections = /* ... */;

// Update metrics
requests_total.Increment();
request_duration.Observe(duration_ms);
active_connections.Set(stats.active_connections);
```

---

## Error Handling

### Network Errors
```cpp
void handleError(const std::string& context, const boost::system::error_code& ec) {
    if (ec == boost::asio::error::eof) {
        // Client closed connection gracefully
        close();
    } else if (ec == boost::asio::error::operation_aborted) {
        // Timeout or explicit cancel
        stats_mutex_.lock();
        stats_.total_errors++;
        stats_mutex_.unlock();
    } else {
        // Other error
        std::cerr << "[" << context << "] Error: " << ec.message() << std::endl;
    }
}
```

### Authentication Timeout Handling
```cpp
void startAuthTimeout(std::chrono::seconds timeout) {
    timeout_timer_ = std::make_unique<net::steady_timer>(*io_context_);
    timeout_timer_->expires_after(timeout);
    timeout_timer_->async_wait([self = shared_from_this()](const error_code& ec) {
        if (!ec) {
            // Authentication timeout expired
            self->sendError(
                static_cast<uint32_t>(NetworkErrorCode::AUTH_TIMEOUT),
                "Authentication timeout");
            self->close();
        }
    });
}
```

### Retry Logic
```cpp
auto policy = WireRetryPolicy::forConnectionIO();
RetryContext retry_context(policy);
auto delay = retry_context.nextDelay(WireErrorClass::kTransient);
if (delay) {
    std::this_thread::sleep_for(*delay);
}
// RetryContext computes bounded backoff; it does not replay application
// requests or guarantee that a failed write was not delivered.
```

---

## Testing Recommendations

### Focused Tests

Network tests are discovered from `tests/network/test_*.cpp`; focused suites
include `test_network_contract_hardening_focused.cpp`,
`test_network_transport_resilience_focused.cpp`,
`test_network_lifecycle_guardrails_focused.cpp`,
`test_network_routing_hardening_focused.cpp`, and
`test_qos_manager_command_injection.cpp`. See
[`MODULE_GUIDE.md`](MODULE_GUIDE.md) for the current focused CTest command.

These are primarily contract, component, and in-process resilience checks; do
not assume they exercise live sockets, TLS handshakes, multi-node routing, or
deployment-scale throughput. Consult the benchmark source and
[`RESEARCH.md`](RESEARCH.md) for the evidence boundary.

---

## Dependencies

**Required:**
- Boost.Asio (async I/O, networking)
- Boost.Beast (TLS/SSL support)
- OpenSSL (cryptography, TLS)
- nlohmann/json (JSON serialization)

**Optional:**
- Prometheus C++ client (metrics export)

**Internal:**
- storage (RocksDBWrapper, key-value operations)
- index (secondary, graph, vector, timeseries indexes)
- transaction (MVCC transaction manager)
- security (transport security validation, authentication)

---

## Build Configuration

The canonical source registration and feature guards are in
[`cmake/CMakeLists.txt`](../../cmake/CMakeLists.txt). Optional transport
sources depend on their `THEMIS_ENABLE_*` flags; do not treat every public
header as an enabled runtime listener. Network test targets are discovered
from `tests/network/test_*.cpp`; benchmarks require
`THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark.

---

## Performance Evidence

The network benchmark sources define in-process release/routing regression
gates; protocol-path cases are stubs. They do not establish deployment
latency, throughput, or capacity. See
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) and
[`RESEARCH.md`](RESEARCH.md) for case-level limits and measurement guidance.

---

## Known Limitations

1. **QUERY_AQL and GEO_QUERY are integration-dependent:** `QUERY_AQL` executes only if `query_engine_` is wired, and `GEO_QUERY` requires either a configured spatial index or the near-query bridge callback. Without this wiring, handlers return structured integration errors (`AQL_NOT_INTEGRATED`/`GEO_NOT_INTEGRATED`) with HTTP REST fallback hints.
2. **Limited Kernel Bypass:** No DPDK support; `io_uring` path is guarded by `THEMIS_ENABLE_IO_URING` and off by default.
3. **WebSocket binary frames not dispatched:** Binary frames over WebSocket are received but not forwarded to storage; clients must use text/JSON frames.

---

## Troubleshooting

### Server does not start with TLS enabled
- Verify `config.tls_cert_path`, `config.tls_key_path`, and (for mTLS) `config.tls_ca_cert_path`.
- Run `validateTransportSecurity(argc, argv)` before `start()` and fail fast on `false`.
- Check certificate/key readability and matching keypair.

### Frequent authentication timeouts
- Increase `config.auth_timeout_sec` for high-latency environments.
- Ensure client sends AUTH immediately after HELLO.
- Validate server/client clock skew when token expiry is enforced externally.

### High reject rate from limits/rate limiting
- Inspect `Stats.rejected_connections` and `Stats.auth_failures`.
- Revisit `max_connections`, `max_connections_per_ip`, `max_requests_per_second`, and `max_requests_per_minute`.
- Confirm expected traffic path (TCP 8766, UDP 8769, QUIC 8770, gRPC 8771) and feature flags.

### QUERY_AQL / GEO_QUERY return integration errors
- `QUERY_AQL` executes only when `query_engine_` is wired on the server.
- `GEO_QUERY` executes only when a spatial index is configured (or a near-query bridge callback is registered).
- If these integrations are not present, the wire protocol returns structured integration errors (`AQL_NOT_INTEGRATED`, `GEO_NOT_INTEGRATED`) with HTTP REST fallback hints.

---

## Version History

- **v1.0.0** - Initial wire protocol server
- **v1.1.0** - Added connection pooling
- **v1.2.0** - Added TLS/mTLS support
- **v1.3.0** - Added socket timeout manager with circuit breaker; gRPC transport (port 8771)
- **v1.4.0** - Added rate limiting and connection limits
- **v1.5.0** - Added timeseries query support
- **v1.6.0** - Added BPMN process orchestration support
- **v1.7.0** - WebSocket upgrade on port 8766; Wire Protocol V2 (multiplexing, flow control, server push)
- **v1.8.0** - UDP fast-path (port 8769); QoS manager (per-tenant bandwidth); connection-level compression (LZ4, Zstd)
- **v1.9.0** - QUIC/HTTP3 transport (port 8770); geo topology router; service mesh integration

---

## References

- [Wire Protocol Specification](../../docs/architecture/wire-protocol.md)
- [Network Security Module Guide](./SECURITY.md)
- [Network Performance Expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Network Architecture](./ARCHITECTURE.md)
- [Network Roadmap](./ROADMAP.md)
- [Network Future Enhancements](./FUTURE_ENHANCEMENTS.md)
- [Public Header Documentation](../../include/network/README.md)
- [German Network Overview](../../docs/de/network/README.md)
- [Network Troubleshooting](../../docs/troubleshooting/network_troubleshooting.md)
- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [Protocol Buffers Wire Format](https://developers.google.com/protocol-buffers/docs/encoding)

## Scientific References

1. Postel, J. (1981). **Transmission Control Protocol**. RFC 793. IETF. https://doi.org/10.17487/RFC0793

2. Rescorla, E. (2018). **The Transport Layer Security (TLS) Protocol Version 1.3**. RFC 8446. IETF. https://doi.org/10.17487/RFC8446

3. Belshe, M., Peon, R., & Thomson, M. (2015). **Hypertext Transfer Protocol Version 2 (HTTP/2)**. RFC 7540. IETF. https://doi.org/10.17487/RFC7540

4. Nygard, M. T. (2018). **Release It!: Design and Deploy Production-Ready Software (2nd ed.)**. Pragmatic Bookshelf. ISBN: 978-1-680-50239-8

5. Harchol-Balter, M. (2013). **Performance Modeling and Design of Computer Systems: Queueing Theory in Action**. Cambridge University Press. https://doi.org/10.1017/CBO9781139226424

## Sourcecode Verification (Module: network/readme)

- Verified files:
    - `src/network/wire_protocol_server.cpp`
    - `src/network/wire_protocol_connection_pool.cpp`
    - `src/network/wire_protocol_v2.cpp`
    - `src/network/wire_protocol_server_ws.cpp`
    - `src/network/udp_fast_path.cpp`
    - `src/network/udp_server.cpp`
    - `src/network/quic_transport.cpp`
    - `src/network/grpc_transport.cpp`
    - `src/network/socket_timeout_manager.cpp`
    - `src/network/adaptive_circuit_breaker.cpp`
    - `src/network/connection_compression.cpp`
    - `src/network/wire_protocol_batch.cpp`
    - `src/network/wire_protocol_zero_copy.cpp`
- Verified behavior surfaces:
    - auth/session/frame validation and opcode dispatch
    - transport gating and multi-transport module surfaces
    - timeout/rate-limit/backpressure and batching/compression paths
- Note:
    - Forward planning is tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`.
    - Historical implementation record remains in `CHANGELOG.md`.

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/network/ -->

# Network Module - Future Enhancements
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · docs/de/network/README.md -->

## Scope

- Binary TCP wire protocol server (port 8766) with v1 and v2 frame formats
- WebSocket upgrade on port 8766 (JSON text-frame messages; guarded by `THEMIS_ENABLE_WEBSOCKET`)
- QUIC/HTTP3 transport (port 8770, TLS 1.3 mandatory, ngtcp2; guarded by `THEMIS_ENABLE_HTTP3`)
- gRPC native transport (port 8771, bidirectional streaming; guarded by `THEMIS_ENABLE_GRPC`)
- UDP fast-path for read-only queries (port 8769; opcodes: GET, QUERY_AQL, VECTOR_SEARCH, PING)
- TLS 1.3 and mutual TLS (mTLS), per-IP rate limiting, and circuit breaker for socket timeouts
- Connection multiplexing (v2 protocol, multiple logical streams per TCP connection)
- Service mesh integration (Istio/Envoy sidecar; `THEMIS_ENABLE_SERVICE_MESH`)

## Design Constraints

- [ ] Wire protocol frame parsing must allocate zero heap objects on the hot path for v1 frames
- [ ] TLS 1.3 is mandatory for all external connections; TLS 1.2 fallback is not permitted
- [ ] Per-IP rate limiting must be enforced before authentication to prevent pre-auth DoS
- [ ] Connection multiplexing (v2) stream state machine must be deterministic; illegal state transitions must close the stream with RST_STREAM
- [ ] WebSocket binary frame dispatch is not yet supported; clients must use text/JSON frames or native TCP binary
- [ ] QUIC 0-RTT resumption is permitted for read-only requests only; mutating requests must use 1-RTT
- [ ] UDP fast-path opcodes are read-only (GET, QUERY_AQL, VECTOR_SEARCH, PING); write opcodes must be rejected
- [ ] All async write buffers must use `shared_ptr`-owned lifetime to prevent use-after-free

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `WireProtocolServer::start(config)` | Server bootstrap | Binds TCP port 8766; spawns I/O + worker thread pools |
| `WireProtocolServer::getActiveConnections()` | Prometheus metrics | Counts both binary and WebSocket sessions |
| `WireProtocolV2Session::sendFrame(type, payload)` | Protocol handler | Validates stream state; uses shared_ptr buffer |
| `QuicTransport::start(config)` | Server bootstrap | Port 8770; TLS 1.3 + ALPN "tmdb"; requires ngtcp2 |
| `GrpcTransport::start(config)` | Server bootstrap | Port 8771; `AsyncGenericService` bidirectional streaming |
| `UDPFastPath::start(config)` | Server bootstrap | Port 8769; read-only opcode filter; per-source-IP rate limit |
| `ConnectionPool::acquire(endpoint)` | Client-side query router | Returns pooled connection with retry/backoff |
| `ServiceMeshIntegration::start(config)` | Deployment bootstrap | Probe server + Envoy xDS v3 REST polling; port 8082 |

## Planned Features

### `WireProtocolServer`: ProcessGraph Visit Timestamp TODO
**Priority:** Low
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)

`wire_protocol_server.cpp` line 2306 had: "TODO: `ProcessGraphManager` doesn't store individual visit timestamps per node" — the temporal-graph traversal path in the wire server cannot reconstruct visit ordering for graph result pages.

**Implementation Notes:**
- `[x]` Add `ProcessGraphVisitLog` (map from node ID to `std::chrono::system_clock::time_point`) to `ProcessGraphManager` state; update on each traversal step.
- `[x]` Expose via `ProcessGraphManager::getVisitTimestamp(instance_id, node_id)` for use in the wire protocol server response encoding.
- `[x]` Add unit test verifying visit timestamps are populated and ordered correctly across multi-hop traversals.

---

### `WireProtocolServer`: WebSocket Binary Frame Dispatch

**Priority:** High
**Target Version:** v1.7.0

Add WebSocket support for real-time bidirectional communication.

**Features:**
- WebSocket server for browser-based clients
- Unified protocol (HTTP upgrade to WebSocket)
- Server-push notifications (document changes, query results)
- Streaming query results
- Real-time collaborative editing support

**Benefits:**
- Enables browser-based real-time applications
- Lower latency than HTTP polling
- Bidirectional communication (server can push to client)
- Compatible with existing web infrastructure

**API:**
```cpp
WebSocketServer::Config config;
config.port = 8767;
config.enable_tls = true;
config.max_connections = 10000;
config.enable_compression = true;  // Per-message deflate

WebSocketServer ws_server(config, storage, index_mgr);
ws_server.start();

// Client-side
WebSocketClient client("wss://server.example.com:8767");
client.connect();

// Subscribe to document changes
client.subscribe("documents/articles", [](const auto& change) {
    std::cout << "Document changed: " << change.uuid << std::endl;
});

// Send query, receive streaming results
client.query("FOR doc IN articles FILTER doc.category == 'tech' RETURN doc",
    [](const auto& result) {
        // Called for each result document as it arrives
        std::cout << "Result: " << result << std::endl;
    }
);
```

**Protocol:**
```
WebSocket Frame Format:
┌──────────┬─────────┬──────────┬─────────┐
│ FIN/RSV  │ OpCode  │ Mask     │ Payload │
│ (1 byte) │ (4 bits)│ (1-9 b)  │ (N byte)│
└──────────┴─────────┴──────────┴─────────┘

OpCodes:
- 0x1: Text frame (JSON)
- 0x2: Binary frame (Protobuf)
- 0x8: Close
- 0x9: Ping
- 0xA: Pong

ThemisDB Message Format (JSON):
{
    "id": "req_12345",
    "type": "query" | "subscribe" | "unsubscribe" | "update",
    "payload": { ... }
}
```

**Implementation:**
- Use Boost.Beast for WebSocket protocol
- Reuse existing authentication and authorization
- Share storage/index managers with wire protocol server
- Implement backpressure for slow clients

---

### UDP Protocol Support
**Priority:** Medium
**Target Version:** v1.8.0

Add UDP support for low-latency, fire-and-forget operations.

**Features:**
- UDP server for high-throughput ingestion
- Connection-less protocol (no handshake overhead)
- Suitable for metrics, logs, events
- Optional reliability via application-level ACKs
- Packet loss tolerance

**Use Cases:**
- Metrics ingestion (Prometheus remote write, StatsD)
- Log aggregation (syslog, structured logs)
- IoT sensor data ingestion
- Real-time event streaming
- High-frequency trading data feeds

**API:**
```cpp
UDPServer::Config config;
config.port = 8768;
config.max_packet_size = 65507;  // Max UDP packet size
config.enable_batching = true;    // Batch small packets
config.batch_interval_ms = 100;
config.enable_acks = false;       // Fire-and-forget (no reliability)

UDPServer udp_server(config, storage, index_mgr);
udp_server.start();

// Client-side
UDPClient client("server.example.com:8768");

// Send metric (fire-and-forget)
client.send_metric("cpu.usage", 85.5, timestamp);

// Send with optional ACK
client.send_metric("important.metric", 123.4, timestamp,
    /*wait_for_ack=*/true,
    /*timeout_ms=*/1000
);
```

**Packet Format:**
```
UDP Packet:
┌──────────┬─────────┬──────────┬─────────┐
│ Magic    │ Version │ OpCode   │ Payload │
│ (2 bytes)│ (1 byte)│ (1 byte) │ (N byte)│
└──────────┴─────────┴──────────┴─────────┘

- Magic: 0x544D ("TM")
- Version: 1
- OpCode: METRIC=1, LOG=2, EVENT=3, BATCH=4

Payload:
- Protobuf or MessagePack encoded data
```

**Trade-offs:**
- ✅ Lower latency (no handshake)
- ✅ Higher throughput (no flow control)
- ✅ Lower CPU overhead
- ❌ No reliability guarantee (packets may be lost)
- ❌ No ordering guarantee
- ❌ No congestion control

**Implementation:**
- Use Boost.Asio UDP sockets
- Implement application-level ACKs (optional)
- Add packet deduplication (sequence numbers)
- Implement rate limiting per source IP
- Add metrics for packet loss monitoring

---

### QUIC Protocol Support
**Priority:** Medium
**Target Version:** v2.0.0
**Status:** ✅ Implemented (v2.0.0, 2026-04-13)

QUIC (HTTP/3) support for modern low-latency communication.

**Implementation:** `include/network/quic_server.h` / `src/network/quic_server.cpp`

**Classes delivered:**
- `QUICServer` — QUIC/HTTP/3 server (port 8769 default); ALPN "h3" + "tmdb"; UDP + TLS 1.3; guarded by `THEMIS_ENABLE_HTTP3`
- `QUICClient` — QUIC client; `quic://host:port` URL; `openStream()` independent bidirectional streams
- `QUICClient::Stream` — independent QUIC stream; `send()` / `receive()` / `close()` / `streamId()`

**Features delivered:**
- [x] QUIC protocol (UDP + TLS 1.3)
- [x] 0-RTT connection establishment (`enable_0rtt=true`; `zero_rtt_accepted`/`zero_rtt_rejected` stats)
- [x] Built-in encryption (no plain QUIC; TLS 1.3 mandatory)
- [x] Multiple streams over single connection (`max_streams_per_connection`, default 100)
- [x] Better loss recovery than TCP (QUIC per-packet retransmit via ngtcp2)
- [x] Connection migration (mobile clients; `migrations` stat)
- [x] Congestion control: BBR (default) or Cubic (`congestion_control` config field)
- [x] HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3 clients
- [x] QUIC-specific metrics (`zero_rtt_accepted`, `zero_rtt_rejected`, `migrations`, `handshakes_completed`)

**Tests:** 35 focused tests (QS-01…QS-35) in `tests/test_quic_server.cpp`; CMake target: `test_quic_server_focused`

---

### Kernel Bypass (DPDK/io_uring)
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ Implemented (v1.9.0, 2026-04-13)

Kernel bypass support for ultra-low latency applications.

**Implementation:** `include/network/kernel_bypass.h` / `src/network/kernel_bypass.cpp`

**Classes delivered:**
- `DPDKServer` — DPDK EAL integration; poll-mode RX/TX; guarded by `THEMIS_ENABLE_DPDK`; port 8772 default
- `IoUringServer` — io_uring SQPOLL async server; fixed-buffer zero-copy sends; guarded by `THEMIS_ENABLE_IO_URING` + Linux; port 8773 default
- `CpuPinner` — `pinCallerToCore()` / `pinThreadToCore()` / `numaNodeForCore()` / `coresOnNuma()`
- `NumaAllocator` — NUMA-local allocation (`THEMIS_ENABLE_NUMA`) with `posix_memalign` fallback
- `ZeroCopyDmaBuffer` — huge-page (`MAP_HUGETLB`) DMA buffer; `mbind()` NUMA binding; move-only

**Features:**
- [x] DPDK (Data Plane Development Kit) for 10G/40G/100G NICs
- [x] io_uring for efficient async I/O on Linux
- [x] Zero-copy networking (fixed-buffer `IORING_REGISTER_BUFFERS` + `ZeroCopyDmaBuffer`)
- [x] User-space TCP/IP stack (DPDK PMD poll loop)
- [x] CPU pinning and NUMA awareness (`CpuPinner` / `NumaAllocator`)

**DPDK Integration:**
```cpp
DPDKServer::Config config;
config.port = 8772;                    // default DPDK server port
config.pci_address = "0000:05:00.0";  // NIC PCI address
config.num_rx_queues = 4;
config.num_tx_queues = 4;
config.cpu_core_mask = 0x0F;           // Cores 0-3
config.huge_pages_mb = 2048;

DPDKServer dpdk_server(config, storage, index_mgr);
dpdk_server.start();
```

**io_uring Integration:**
```cpp
IoUringServer::Config config;
config.port              = 8773;       // default io_uring server port
config.ring_size         = 4096;
config.sq_thread_cpu     = 2;          // CPU for kernel SQ polling
config.sq_thread_idle_ms = 1000;

IoUringServer uring_server(config, storage, index_mgr);
uring_server.start();
```

**Performance Targets:**
- DPDK: 1-10 μs latency, 100 Gbps throughput
- io_uring: 10-50 μs latency, 10 Gbps throughput

**Use Cases:**
- High-frequency trading
- Real-time analytics
- Low-latency microservices
- Ultra-high throughput ingestion

**Trade-offs:**
- ✅ Ultra-low latency
- ✅ Very high throughput
- ✅ Efficient CPU usage
- ❌ Complex setup (huge pages, CPU pinning)
- ❌ Hardware specific (DPDK requires compatible NIC)
- ❌ Limited OS compatibility (Linux only for io_uring)

**Tests:** 40 focused tests (KBP-01…KBP-40) in `tests/test_kernel_bypass.cpp`

---

### Service Mesh Integration
**Priority:** Low
**Target Version:** v1.10.0

Add support for service mesh integration (Istio, Linkerd, Consul Connect).

**Features:**
- Envoy sidecar integration
- Automatic mTLS between services
- Traffic management (load balancing, retry, circuit breaking)
- Observability (distributed tracing, metrics)
- Service discovery integration

**Istio Integration:**
```yaml
# themisdb-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
  template:
    metadata:
      annotations:
        sidecar.istio.io/inject: "true"
        traffic.sidecar.istio.io/includeInboundPorts: "8766,8767"
    spec:
      containers:
      - name: themisdb
        image: themisdb/themisdb:latest
        env:
        - name: WIRE_PROTOCOL_PORT
          value: "8766"
        - name: ENABLE_TLS
          value: "false"  # mTLS handled by Envoy
```

**Envoy Filter Configuration:**
```yaml
apiVersion: networking.istio.io/v1alpha3
kind: EnvoyFilter
metadata:
  name: themisdb-wire-protocol
spec:
  configPatches:
  - applyTo: NETWORK_FILTER
    match:
      listener:
        portNumber: 8766
    patch:
      operation: INSERT_BEFORE
      value:
        name: envoy.filters.network.themis_wire_protocol
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.network.themis_wire_protocol.v3.ThemisWireProtocol
          stat_prefix: themis_wire
```

**Benefits:**
- Automatic service discovery
- Zero-trust security (automatic mTLS)
- Traffic management (retries, timeouts, circuit breaking)
- Observability (tracing, metrics, logs)
- Multi-cluster support

**Implementation:**
- Envoy filter for wire protocol parsing
- OpenTelemetry integration for tracing
- Prometheus metrics export
- Health check endpoints (/healthz, /ready)

---

## HTTP/3 Support
**Priority:** Low
**Target Version:** v2.0.0

Add HTTP/3 support for HTTP API (complementary to wire protocol).

**Features:**
- HTTP/3 over QUIC
- 0-RTT connection establishment
- Better performance than HTTP/2
- Compatible with existing HTTP clients (with upgrade)

**API:**
```cpp
HTTP3Server::Config config;
config.port = 443;
config.enable_0rtt = true;
config.alt_svc_max_age = 86400;  // 24 hours

HTTP3Server http3_server(config, storage, index_mgr);
http3_server.start();

// Advertise HTTP/3 support via Alt-Svc header
// Alt-Svc: h3=":443"; ma=86400
```

**Benefits:**
- Faster than HTTP/2 (0-RTT, better loss recovery)
- Better mobile performance (connection migration)
- Standardized protocol (vs custom wire protocol)

---

### Multicast Support
**Priority:** Low
**Target Version:** v1.11.0

Add IP multicast support for efficient one-to-many communication.

**Features:**
- IP multicast for pub/sub messaging
- Efficient broadcast to multiple subscribers
- Topic-based subscriptions
- Reliable multicast (PGM/NORM)

**Use Cases:**
- Real-time data distribution (market data, sensor networks)
- Pub/sub messaging for IoT
- Cluster coordination (Raft heartbeats)
- Live query result streaming to multiple clients

**API:**
```cpp
MulticastServer::Config config;
config.multicast_group = "239.255.1.1";
config.port = 8772;
config.ttl = 10;  // TTL for multicast packets
config.enable_pgm = true;  // Reliable multicast

MulticastServer mcast_server(config, storage);
mcast_server.start();

// Publish to multicast group
mcast_server.publish("topic.metrics", metric_data);

// Client-side
MulticastClient client("239.255.1.1:8772");
client.subscribe("topic.metrics", [](const auto& data) {
    std::cout << "Received: " << data << std::endl;
});
```

**Protocols:**
- PGM (Pragmatic General Multicast) for reliable multicast
- NORM (Nack-Oriented Reliable Multicast)
- SSM (Source-Specific Multicast) for security

---

### RDMA Support
**Priority:** Low
**Target Version:** v2.1.0

Add RDMA (Remote Direct Memory Access) support for ultra-low latency.

**Features:**
- InfiniBand and RoCE (RDMA over Converged Ethernet)
- Zero-copy data transfer
- Kernel bypass
- RDMA verbs API
- One-sided operations (RDMA READ/WRITE)

**Benefits:**
- **Sub-microsecond latency:** RDMA bypasses kernel
- **Zero-copy:** Direct memory-to-memory transfer
- **Low CPU overhead:** DMA offload to NIC
- **High bandwidth:** 100-400 Gbps

**API:**
```cpp
RDMAServer::Config config;
config.port = 8773;
config.device_name = "mlx5_0";  // InfiniBand device
config.gid_index = 0;
config.max_wr = 1024;  // Work requests
config.max_sge = 4;    // Scatter-gather elements

RDMAServer rdma_server(config, storage, index_mgr);
rdma_server.start();

// Client-side RDMA READ (one-sided)
RDMAClient client("rdma://server.example.com:8773");
client.rdma_read(remote_addr, length, local_buffer);
// Data transferred directly from server memory to local_buffer
```

**Use Cases:**
- High-performance computing (HPC)
- Distributed storage (Ceph, GlusterFS)
- Low-latency databases
- Machine learning training clusters

**Trade-offs:**
- ✅ Ultra-low latency (< 1 μs)
- ✅ Zero-copy, low CPU
- ✅ Very high bandwidth
- ❌ Requires specialized hardware (InfiniBand or RoCE NICs)
- ❌ Complex programming model
- ❌ Limited to data center use

---

### Load Balancing with Raft Coordination
**Priority:** High
**Target Version:** v1.8.0

Add Raft-based load balancing for distributed query routing.

**Features:**
- Raft consensus for load balancer state
- Automatic failover on node failures
- Health-based routing decisions
- Dynamic weight adjustment based on load
- Cross-datacenter routing

**Architecture:**
```
┌─────────────────────────────────────────────────────┐
│          Raft Load Balancer Cluster                  │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐             │
│  │Leader   │  │Follower │  │Follower │             │
│  │(active) │  │(standby)│  │(standby)│             │
│  └─────────┘  └─────────┘  └─────────┘             │
│       │              │             │                 │
│       │   (consensus on routing decisions)          │
│       │              │             │                 │
│       └──────────────┴─────────────┘                │
│                      │                               │
└──────────────────────┼───────────────────────────────┘
                       │
         ┌─────────────┼─────────────┐
         │             │             │
    ┌────▼────┐  ┌────▼────┐  ┌────▼────┐
    │ Shard 1 │  │ Shard 2 │  │ Shard 3 │
    │ (node1) │  │ (node2) │  │ (node3) │
    └─────────┘  └─────────┘  └─────────┘
```

**API:**
```cpp
RaftLoadBalancer::Config config;
config.raft_port = 8774;
config.health_check_interval_ms = 5000;
config.rebalance_threshold = 0.2;  // 20% load imbalance

RaftLoadBalancer lb(config);
lb.add_backend("node1:8766", /*weight=*/1.0);
lb.add_backend("node2:8766", /*weight=*/1.0);
lb.add_backend("node3:8766", /*weight=*/1.0);
lb.start();

// Client-side
auto conn = lb.get_connection();
// Automatically routed to least loaded backend
```

**Load Balancing Strategies:**
- **Round Robin:** Simple, predictable
- **Least Connections:** Route to backend with fewest active connections
- **Weighted Round Robin:** Distribute based on capacity weights
- **Health-Based:** Exclude unhealthy backends
- **Consistent Hashing:** Sticky routing for caching

**Failover:**
- Raft leader monitors backend health
- Automatic removal of failed backends
- Automatic re-addition when backend recovers
- Leader election on LB leader failure

---

### Bandwidth Management and QoS
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0, Issue #190, PR copilot/add-bandwidth-management-qos)

Add bandwidth management and quality of service (QoS) features.

**Features:**
- Per-connection bandwidth limits ✅
- Traffic shaping (token bucket, leaky bucket) ✅
- Priority queuing (high/medium/low) ✅
- Fair queuing (prevent starvation) ✅
- Congestion control integration ✅

**API:**
```cpp
QoSManager::Config config;
config.max_bandwidth_mbps = 1000;  // 1 Gbps total
config.per_connection_limit_mbps = 100;  // 100 Mbps per connection
config.enable_fair_queuing = true;
config.enable_priority_queuing = true;

QoSManager qos(config);

// Set connection priority
qos.set_priority(connection_id, Priority::HIGH);

// Set bandwidth limit
qos.set_bandwidth_limit(connection_id, 50 * 1024 * 1024);  // 50 Mbps

// Traffic shaping with token bucket
qos.set_token_bucket(connection_id,
    /*rate=*/10'000'000,    // 10 MB/s
    /*burst=*/100'000'000   // 100 MB burst
);
```

**Priority Levels:**
- **CRITICAL:** Interactive queries, low latency required ✅
- **HIGH:** Transactional operations, OLTP ✅
- **MEDIUM:** Analytical queries, OLAP ✅
- **LOW:** Batch operations, backups, replication ✅

**Implementation:**
- Token bucket algorithm for rate limiting ✅
- Leaky bucket algorithm for strict constant-rate shaping ✅
- Priority queue for packet scheduling ✅
- Fair queuing to prevent starvation ✅
- Congestion control integration (AIMD, allowSend() enforces cwnd) ✅
- Integration with Linux tc (traffic control) ✅

**Files:**
- `include/network/qos_manager.h` — `LeakyBucket`, `CongestionController`, extended `QoSManager::Config` + `ConnectionStats`
- `src/network/qos_manager.cpp` — implementations of all new classes and methods
- `tests/test_bandwidth_management_qos.cpp` — 41 focused tests
- `.github/workflows/bandwidth-management-qos-ci.yml` — CI workflow

---

### Distributed Tracing
**Priority:** Medium
**Target Version:** v1.7.0

Add distributed tracing support for request flow visualization.

**Features:**
- OpenTelemetry integration
- Jaeger/Zipkin exporter
- Trace context propagation
- Span attributes and events
- Sampling strategies

**API:**
```cpp
// Initialize tracing
TracingConfig config;
config.service_name = "themisdb-wire-protocol";
config.jaeger_endpoint = "http://jaeger:14268/api/traces";
config.sampling_rate = 0.1;  // 10% sampling

Tracer tracer(config);

// Instrument wire protocol server
void Session::handleGet() {
    auto span = tracer.start_span("wire_protocol.get",
        {{"collection", collection}, {"uuid", uuid}}
    );

    auto result = storage_->get(key);

    span.set_attribute("result_size", result.size());
    span.end();
}
```

**Trace Propagation:**
```
Client Request
    ↓
┌──────────────────┐
│ Wire Protocol    │ span: wire_protocol.handle_request
│ Server           │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Storage Layer    │ span: storage.get
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ RocksDB          │ span: rocksdb.read
└──────────────────┘
```

---

## Research and Exploration

### Network Function Virtualization (NFV)
**Target:** Long-term exploration

Investigate NFV for flexible network function deployment.

**Potential Benefits:**
- Dynamic network function deployment
- Software-defined networking integration
- Cloud-native networking

### eBPF-based Networking
**Target:** v2.2.0

Explore eBPF for programmable networking in the kernel.

**Potential Features:**
- Custom protocol parsing in kernel
- Zero-copy socket operations
- Dynamic rate limiting
- Connection tracking

### Machine Learning-based QoS
**Target:** v2.3.0

Use ML to predict load and optimize routing decisions.

**Potential Features:**
- Predictive load balancing
- Anomaly detection (DDoS, abuse)
- Adaptive timeout adjustment
- Traffic pattern analysis

---

## Version Roadmap

### v1.7.0 (Q2 2026)
- ✅ WebSocket protocol support
- ✅ Distributed tracing (OpenTelemetry)

### v1.8.0 (Q3 2026)
- ✅ UDP protocol support
- ✅ Load balancing with Raft coordination
- ✅ Bandwidth management and QoS

### v1.9.0 (Q4 2026)
- ✅ Kernel bypass (DPDK/io_uring)
- ✅ Enhanced metrics and monitoring

### v1.10.0 (Q1 2027)
- ✅ Service mesh integration (Istio, Linkerd)
- ✅ Multicast support

### v2.0.0 (Q2 2027)
- ✅ QUIC protocol support
- ✅ HTTP/3 support

### v2.1.0 (Q3 2027)
- ✅ RDMA support
- ✅ Advanced traffic shaping

### v2.2.0+ (Q4 2027+)
- 🔬 eBPF-based networking
- 🔬 ML-based QoS
- 🔬 Network function virtualization

---

## Community Contributions

We welcome community contributions for these features! Areas where contributions would be especially valuable:

1. **Client libraries:** SDKs for various languages (Python, Java, Go, Rust, etc.)
2. **Protocol implementations:** WebSocket, QUIC, UDP clients
3. **Performance benchmarks:** Latency and throughput testing
4. **Integration examples:** Service mesh, monitoring, tracing
5. **Documentation:** Tutorials, best practices, case studies

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

---

## References

- [WebSocket Protocol (RFC 6455)](https://tools.ietf.org/html/rfc6455)
- [QUIC Protocol (RFC 9000)](https://tools.ietf.org/html/rfc9000)
- [HTTP/3 (RFC 9114)](https://tools.ietf.org/html/rfc9114)
- [DPDK Documentation](https://doc.dpdk.org/)
- [io_uring Documentation](https://kernel.dk/io_uring.pdf)
- [OpenTelemetry](https://opentelemetry.io/)
- [Istio Service Mesh](https://istio.io/)

## Scientific References

### QUIC / HTTP3 / Transport Protocols

[1] J. Iyengar and M. Thomson (Eds.), "QUIC: A UDP-Based Multiplexed and Secure Transport," *RFC 9000*, Internet Engineering Task Force, May 2021. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9000

[2] M. Bishop (Ed.), "HTTP/3," *RFC 9114*, Internet Engineering Task Force, Jun. 2022. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9114

[3] L. Tung, D. Keles, and S. Walfish, "QUIC's Impact on Network Performance: A Study of HTTP/3 vs HTTP/2," *ACM SIGCOMM Computer Communication Review*, vol. 52, no. 1, pp. 22–27, 2022.

[4] P. Tiesel and A. Feldmann, "One Size Does Not Fit All: On the Coexistence of Heterogeneous QUIC Implementations," in *Proc. ACM Internet Measurement Conference (IMC)*, 2020, pp. 272–279.

### UDP Fast-Path / Kernel Bypass

[5] E. Kohler, R. Morris, B. Chen, J. Jannotti, and M. F. Kaashoek, "The Click Modular Router," *ACM Transactions on Computer Systems*, vol. 18, no. 3, pp. 263–297, Aug. 2000.

[6] Intel, "Data Plane Development Kit (DPDK): High-Performance Packet Processing," *Intel Networking Division*, 2024. [Online]. Available: https://doc.dpdk.org/

[7] A. Bhattacharyya, K. Bhattacharyya, and A. Pal, "io_uring: A Zero-Copy I/O Interface for the Linux Kernel," in *Proc. USENIX Annual Technical Conference (ATC)*, 2022.

### gRPC / RPC Frameworks

[8] gRPC Authors, "gRPC: A High-Performance, Open-Source Universal RPC Framework," Google, 2024. [Online]. Available: https://grpc.io/

[9] T. Kivikangas and T. Mikola, "Performance Comparison of gRPC and REST API for Microservices Architecture," *IEEE Access*, vol. 10, pp. 58 532–58 543, 2022. doi: 10.1109/ACCESS.2022.3179476.

### Connection Multiplexing / HTTP/2 Framing

[10] M. Belshe, R. Peon, and M. Thomson (Eds.), "Hypertext Transfer Protocol Version 2 (HTTP/2)," *RFC 7540*, Internet Engineering Task Force, May 2015.

[11] N. Siekkinen, M. Luukkainen, and N. Manner, "Efficient Application-Level Multiplexing over HTTP/2: A Measurement Study," in *Proc. IEEE International Conference on Communications (ICC)*, 2021.

### Service Mesh / Envoy Proxy

[12] A. Klein, "Envoy Proxy: An Open Source Edge and Service Proxy, Designed for Cloud-Native Applications," *USENIX ;login: The USENIX Magazine*, vol. 43, no. 4, 2018.

[13] W. Morgan, "The Service Mesh: An Increasingly Critical Component of the Cloud-Native Stack," *O'Reilly Media*, 2023.

### TLS 1.3 / Network Security

[14] E. Rescorla, "The Transport Layer Security (TLS) Protocol Version 1.3," *RFC 8446*, Internet Engineering Task Force, Aug. 2018.

[15] N. Aviram, S. Schinzel, J. Somorovsky, N. Heninger, M. Dankel, J. Steube, L. Valenta, D. Adrian, J. A. Halderman, V. Dukhovni, E. Käsper, S. Cohney, S. Engels, C. Paar, and Y. Shavitt, "DROWN: Breaking TLS Using SSLv2," in *Proc. USENIX Security Symposium*, 2016, pp. 689–706.

### QoS / Rate Limiting

[16] S. Floyd and V. Jacobson, "Random Early Detection Gateways for Congestion Avoidance," *IEEE/ACM Transactions on Networking*, vol. 1, no. 4, pp. 397–413, Aug. 1993. doi: 10.1109/90.251892.

[17] L. Breslau, E. W. Knightly, P. Bhattacharya, and B. Vinnakota, "Internet QoS: Big Picture and Current Issues," in *Proc. IEEE INFOCOM*, 2000, pp. 3–8.

### eBPF-Based Networking (Research & Exploration)

[18] T. Høiland-Jørgensen, M. B. Brouer, D. Ahern, J. Fastabend, T. Herbert, D. Johansen, and D. Taht, "The eXpress Data Path: Fast Programmable Packet Processing in the Operating System Kernel," in *Proc. ACM CoNEXT*, 2018, pp. 54–66. doi: 10.1145/3281411.3281443.

[19] B. Gregg, "BPF Performance Tools: Linux System and Application Observability," *Addison-Wesley Professional*, 2019, ISBN 0-13-655482-4.

### WebSocket Protocol

[20] I. Fette and A. Melnikov, "The WebSocket Protocol," *RFC 6455*, Internet Engineering Task Force, Dec. 2011.

[21] V. Pimentel and B. G. Nickerson, "Communicating and Displaying Real-Time Data with WebSocket," *IEEE Internet Computing*, vol. 16, no. 4, pp. 45–53, 2012. doi: 10.1109/MIC.2012.64.

## Test Strategy

- Unit test coverage ≥ 80% across wire protocol v1/v2, WebSocket upgrade, QUIC, gRPC, and UDP fast-path modules
- Protocol conformance tests: all 8 frame type transitions in v2 stream state machine verified (IDLE→OPEN→HALF_CLOSED→CLOSED); RST_STREAM on illegal transitions
- TLS integration tests: TLS 1.3 handshake with mTLS; rejected connection on expired/invalid certificate; TLS 1.2 rejected
- Rate-limit enforcement tests: per-IP rate limit triggers at configured threshold; WebSocket connections counted against same limit
- Regression benchmarks: WebSocket vs. native binary throughput (connections/sec); p99 latency must not regress by > 5% between releases
- UDP fast-path tests: write opcode rejection, per-source-IP rate limiting, compact 10-byte header echo, response builder correctness

## Performance Targets

- TCP wire protocol: ≥ 100,000 requests/sec on a single server core (128-byte payload, no TLS)
- TLS 1.3 handshake latency: < 5 ms p99 for new connections; < 1 ms p99 for session resumption
- WebSocket text-frame round-trip latency: < 2 ms p99 on localhost
- QUIC connection establishment (0-RTT resumption): < 2 ms p99
- gRPC bidirectional streaming throughput: ≥ 50,000 messages/sec per stream
- UDP fast-path GET response latency: < 500 µs p99 on localhost
- Connection multiplexing (v2): ≥ 10 logical streams per physical connection with < 5% overhead vs. single-stream

## Security / Reliability

- All external connections must use TLS 1.3; plaintext TCP connections are rejected unless `allow_plaintext` is explicitly set in config (disabled by default)
- Per-IP rate limiting is enforced before authentication to prevent pre-auth amplification attacks
- Circuit breaker trips on ≥ 5 consecutive socket timeouts (configurable); recovery requires explicit reset or backoff expiry
- QUIC 0-RTT anti-replay protection must be enabled; replayed 0-RTT requests for mutating opcodes must be rejected
- v2 frame flow control (WINDOW_UPDATE) prevents unbounded memory growth from slow consumers; connection is terminated if receive buffer exceeds `max_frame_buffer_bytes`
- Service mesh sidecar health probes must not be accessible from external network interfaces (bind to loopback or pod-local address only)
- Authentication tokens must not appear in structured network audit logs; log only token hash (SHA-256 truncated to 16 hex chars)

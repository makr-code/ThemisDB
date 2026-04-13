# Network Module Roadmap

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · docs/de/network/README.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.8.0 – Production-grade networking layer. All transport paths (TCP, WebSocket, UDP fast-path, UDP ingestion, QUIC/HTTP3, gRPC), wire protocol optimizations (batch writes, zero-copy, dictionary compression), and Raft-coordinated load balancing are fully implemented. Note: `RaftLoadBalancer` simulates Raft consensus in-process; full distributed multi-node Raft is planned for a future milestone.

## Completed ✅
- [x] WireProtocolServer – high-performance binary TCP server (port 8766)
- [x] Dedicated I/O thread pool + worker thread pool separation
- [x] Connection pool management (client-side and server-side)
- [x] TLS 1.3 and mutual TLS (mTLS) support
- [x] Per-IP rate limiting (requests/sec and requests/min)
- [x] Connection limits (global and per-IP)
- [x] Circuit breaker pattern for socket timeouts
- [x] Protocol buffer wire format helpers (lightweight parser/serializer)
- [x] Authentication (token-based) with configurable auth timeout — `handleAuthRequest()` validates token, sets `authenticated_` flag; new `Config::auth_token` field; see NETWORK-MISSING-002 in `docs/de/network/missing-implementations.md` (resolved 2026-03-10)
- [x] Wire Protocol V1 opcode handlers — HELLO, AUTH, GET, PUT, DELETE, QUERY_AQL, VECTOR_SEARCH, GEO_QUERY fully implemented; see NETWORK-MISSING-001 (resolved 2026-03-10)
- [x] Health checking and keepalive mechanisms
- [x] Automatic retry logic with configurable back-off
- [x] Transport security validation (`validateTransportSecurity`)
- [x] Prometheus metrics for connection and request statistics
- [x] WebSocket upgrade support on wire protocol port (PR #2209, Q2 2026)
  - Protocol detection: HTTP "GET " prefix vs binary "TMDB" magic on port 8766
  - `WireProtocolWebSocketSession` (`include/network/wire_protocol_websocket.h`)
  - JSON text-frame messages: ping, get, put, delete (query redirects to HTTP API)
  - Guarded by `THEMIS_ENABLE_WEBSOCKET`; config: `Config::enable_websocket_upgrade`
  - `getActiveConnections()` includes both binary and WebSocket sessions
  - Connection-count accounting preserved across protocol upgrade
- [x] Connection multiplexing (multiple logical streams per TCP connection) (Issue: #2415)
- [x] Per-tenant network bandwidth quotas (Issue: #2205)
- [x] Connection-level compression (LZ4, Zstd) (Issue: #2416)
- [x] TCP backlog management and backpressure handling (Issue: #FEATURE)
  - `Config::tcp_backlog` (int, default 128) passed to `listen(2)` for OS-level queue control
  - Global `max_connections` enforced via atomic `active_connection_count_` (fast path, no lock)
  - Per-IP limit check retained as a secondary guard
  - Rejected sockets closed immediately to free kernel resources
  - `Stats::rejected_connections` incremented on every rejected connection
  - Overload state logged once on entry ("Backpressure: connection limit reached")
  - Recovery state logged once when active count drops below `max_connections`
  - Focused unit tests in `tests/test_wire_protocol_backpressure.cpp` (`WireProtocolBackpressureFocusedTests`)
- [x] Bandwidth Management and QoS (Issue: #190, v1.8.0)
  - `LeakyBucket` class: constant-rate traffic shaper (capacity + drain rate, overflow detection)
  - `CongestionController` class: AIMD (slow-start + additive-increase/multiplicative-decrease), SRTT estimation
  - `QoSManager::Config` extended: `max_bandwidth_mbps`, `per_connection_limit_mbps`, `enable_priority_queuing`, `starvation_guard_threshold`
  - Snake-case API: `set_priority()`, `set_bandwidth_limit()`, `set_token_bucket()`
  - Leaky bucket integration: `setLeakyBucket()` / `clearLeakyBucket()`; `allowSend()` enforces conformance
  - Priority queue scheduling: `enqueueSend()` / `dequeueForSend()` / `getPendingQueueDepth()`
  - Fair-queuing starvation guard: forces lower-priority service after `starvation_guard_threshold` high-priority dequeues
  - Congestion control integration: `recordAck()` / `recordLoss()` / `getCongestionWindow()`; `allowSend()` gates on cwnd
  - Linux tc integration: `configureTc(TcConfig)` sets up HTB qdisc; interface-name injection prevention
  - `ConnectionStats` extended: `congestion_window`, `congestion_ssthresh`, `smoothed_rtt_us`
  - Focused unit tests in `tests/test_bandwidth_management_qos.cpp` (`BandwidthManagementQoSFocusedTests`, 41 tests)
  - CI: `.github/workflows/bandwidth-management-qos-ci.yml`
- [x] Wire protocol performance optimizations — batch writes, zero-copy serialization, dictionary compression (v1.8.0)
  - `WireProtocolBatcher` in `include/network/wire_protocol_batch.h` / `src/network/wire_protocol_batch.cpp`
    - Accumulates outbound frames in an iovec list and flushes via a single `writev(2)` call
    - `NagleController`: per-socket `TCP_CORK` (Linux) / `TCP_NOPUSH` (BSD/macOS) helper; also exposes `TCP_NODELAY`
    - `BatchStats` for observability (frames coalesced, syscalls saved)
    - Performance target: ~10× reduction in syscall count for small-message workloads
  - `ZeroCopyFrameBuilder` + `MemoryMappedPayload` in `include/network/wire_protocol_zero_copy.h` / `src/network/wire_protocol_zero_copy.cpp`
    - `ZeroCopyFrameBuilder`: assembles frame header + caller-owned payload in a single `writev(2)` — no intermediate heap allocation
    - `MemoryMappedPayload`: memory-maps a file or anonymous region; kernel transfers directly from page cache to NIC (true zero-copy)
    - Performance target: <1 ms (p99) round-trip for payloads ≤ 64 KiB; <10 MB overhead per 1 000 connections
  - `ZstdDictionaryCompressor` in `include/network/connection_compression.h` / `src/network/connection_compression.cpp`
    - Dictionary-trained Zstd compression for payloads with shared structure (JSON keys, Protobuf field tags)
    - Wire format: `[dict_id: uint32_t LE][original_size: uint32_t LE][compressed_data...]`
    - Falls back to generic Zstd compression if training fails or dict is too small
    - Config: `compression_level` (1–22), `min_compress_bytes` (256), `dict_max_size` (112 KiB default)
  - Unit tests in `tests/test_wire_protocol_optimizations.cpp` (`WireProtocolOptimizations`, 39 tests)
- [x] UDP ingestion server for high-throughput write operations (v1.8.0, Issue: #FEATURE)
  - `UDPServer` class in `include/network/udp_server.h` / `src/network/udp_server.cpp`
  - Port 8768 (dedicated; separate from read-only UDP fast-path on port 8769)
  - Fire-and-forget UDP transport for metrics, logs, events, and batched payloads
  - Packet format: `[magic: 0x54 0x4D][version: 0x01][opcode][seq_num: uint32_t BE][flags][payload_len: uint16_t BE][payload...]`
  - Opcodes: `METRIC (0x01)`, `LOG (0x02)`, `EVENT (0x03)`, `BATCH (0x04)`, `PING (0xFE)`
  - Optional ACK when `FLAGS_ACK_REQUESTED` is set; per-source-IP deduplication via sequence number
  - Unit tests in `tests/test_udp_server.cpp` (59 tests: magic bytes, opcodes, header constants, config defaults)
- [x] Raft-coordinated load balancer for distributed query routing (v1.8.0, Issue: #78)
  - `RaftLoadBalancer` class in `include/network/raft_load_balancer.h` / `src/network/raft_load_balancer.cpp`
  - Intra-cluster Raft communication on port 8774 (`Config::raft_port`)
  - Supported strategies: `ROUND_ROBIN`, `LEAST_CONNECTIONS`, `WEIGHTED_ROUND_ROBIN`, `HEALTH_BASED`, `CONSISTENT_HASH`
  - Raft leader propagates backend weight/health updates to followers via consensus log
  - Health-based automatic failover: unhealthy backends excluded, re-admitted on recovery
  - Consistent hashing for sticky routing (session affinity / cache locality)
  - Cross-datacenter preference: routes to local datacenter first, falls back to remote on failure
  - `Stats`: `total_requests`, `total_failures`, `failover_events`, `rebalance_events`
  - Unit tests in `tests/test_raft_load_balancer.cpp` (`RaftLoadBalancerTest`, 26 tests)
- [x] **io_uring batched sender** — `IoUringBatchedSender` in `io_uring_batcher.h/cpp` (Issue: #4581) (2026-04-12)
  - Single `io_uring_enter()` syscall for N concurrent `WireProtocolBatcher` flushes (`IORING_OP_WRITEV` SQEs)
  - CQE reap + per-operation error reporting; transparent `writev(2)` fallback when io_uring unavailable
  - Guarded by `THEMIS_ENABLE_IO_URING`; no ABI change to `WireProtocolBatcher`
  - 12 focused tests (IUB-01…IUB-12) in `tests/test_io_uring_batcher.cpp`

## In Progress 🚧
- [x] UDP-based fast-path for read-only queries (Target: Q3 2026) (Issue: #1962) (PR: #3098)
  - UDP socket on port 8769 (dedicated, separate from TCP wire protocol port 8766)
  - Read-only opcodes only: GET, QUERY_AQL, VECTOR_SEARCH, PING
  - Per-source-IP rate limiting (configurable packets/second)
  - Compact 10-byte binary header with request-ID echo for correlation
  - `UDPFastPath` class in `include/network/udp_fast_path.h` / `src/network/udp_fast_path.cpp`
  - Unit tests in `tests/test_udp_fast_path.cpp` (config, packet validation, opcode filter, response builder)
  - ⚠️ Implementation complete; ROADMAP status reflects pending production validation / integration tests
- [x] QUIC/HTTP3 transport layer integration (Target: Q3 2026) (Issue: #1994)
  - `QuicTransport` class in `include/network/quic_transport.h` / `src/network/quic_transport.cpp`
  - Port 8770 (dedicated, separate from TCP 8766 and UDP 8769)
  - TLS 1.3 mandatory; ALPN "tmdb" advertises binary wire protocol over QUIC
  - 0-RTT connection resumption, connection migration supported (ngtcp2)
  - Configurable idle timeout, max streams, flow-control windows, connection limits
  - Per-connection rate tracking; `QuicTransport::Stats` for Prometheus integration
  - Guarded by `THEMIS_ENABLE_HTTP3`; requires ngtcp2 + OpenSSL
  - `Http3Session::doRead()` and `Http3Session::onRead()` stubs resolved (server module)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Adaptive connection pool sizing driven by real-time utilisation metrics (Issue: #FEATURE)
  - `IPoolingStrategy` interface + `AdaptivePoolingStrategy` implementation in `wire_protocol_connection_pool.h/.cpp`
  - Scale-up when idle fraction drops below `(1 - scale_up_threshold)` (default 0.8)
  - Scale-down when oldest idle connection exceeds `min_idle_time` and pool > min_count
  - `Config::enable_adaptive_sizing` / `Config::adaptive_strategy` opt-in fields
  - `Stats::utilization` + `Stats::pool_size_adaptations` metrics exposed via `getStats()`
  - `adaptPoolSize()` called from existing maintenance thread every 10 s
  - Unit tests in `test_wire_protocol_connection_pool.cpp` (AdaptivePoolingStrategyTest + pool tests)
- [x] Adaptive circuit breaker for network failure resilience (Issue: #FEATURE)
  - `AdaptiveCircuitBreaker` in `include/network/adaptive_circuit_breaker.h` / `src/network/adaptive_circuit_breaker.cpp`
  - CLOSED → OPEN → HALF_OPEN → CLOSED state machine; thread-safe (atomic state, mutex for counters)
  - `shouldAllow()` / `recordSuccess()` / `recordFailure()` / `getState()` / `getStats()`
  - Adaptive threshold: reduces `failure_threshold` on repeated trips; restores on full recovery
  - Half-open timeout: re-opens circuit if probe window expires without enough successes
  - State-change callback for Prometheus / logging integration
  - Unit tests in `tests/test_network_circuit_breaker.cpp` (`NetworkCircuitBreakerFocusedTests`, 19 tests)
- [?] Adaptive I/O thread scaling based on connection load
- [?] Structured network audit log (connection open/close/auth events)

### Long-term (6-12 months)
- [?] RDMA support for ultra-low-latency inter-node communication
- [x] IPv6 dual-stack support (Issue: #FEATURE, v1.9.0)
  - `Config::enable_ipv6` (bool, default false) – switches acceptor to IPv6 socket
  - `Config::ipv6_dual_stack` (bool, default true) – clears IPV6_V6ONLY so a single socket accepts both IPv4-mapped and native IPv6 clients
  - When `enable_ipv6=true` and `host` is the default "0.0.0.0" it is automatically promoted to "::"
  - Explicit IPv6 addresses in `host` (e.g. "::1", "fe80::1") always honoured
  - Connection-tracking maps keyed by `address().to_string()` – works transparently for IPv6 strings
  - Unit tests in `tests/test_wire_protocol_ipv6.cpp` (`WireProtocolIPv6FocusedTests`, 18 tests)

## Implementation Phases

### Phase 1: Production Networking Stack (Status: Completed ✅)
- [x] WireProtocolServer: high-performance binary TCP server on port 8766 (`network/wire_protocol_server.cpp`)
- [x] Dedicated I/O thread pool and worker thread pool separation
- [x] Client-side and server-side connection pool management
- [x] TLS 1.3 and mutual TLS (mTLS) support
- [x] Per-IP rate limiting (requests/sec and requests/min)
- [x] Connection limits (global and per-IP)
- [x] Circuit breaker pattern for socket timeouts
- [x] Protocol buffer wire format helpers (lightweight parser/serializer)
- [x] Token-based authentication with configurable auth timeout
- [x] Health checking, keepalive mechanisms, and automatic retry with back-off
- [x] Transport security validation (`validateTransportSecurity`)
- [x] Prometheus metrics for connection and request statistics

### Phase 2: Alternative Transports (Status: In Progress 🚧)
- [x] WebSocket upgrade support on wire protocol port (Target: Q2 2026)
  - Protocol detection: HTTP "GET " prefix vs binary "TMDB" magic on port 8766
  - `WireProtocolWebSocketSession` (include/network/wire_protocol_websocket.h)
  - JSON text-frame messages: ping, get, put, delete, query
  - Guarded by `THEMIS_ENABLE_WEBSOCKET`; config: `enable_websocket_upgrade`
- [x] UDP-based fast-path for read-only queries (Target: Q3 2026)
- [x] QUIC/HTTP3 transport layer integration (Target: Q3 2026)

### Phase 3: Advanced Networking & Service Mesh (Status: Completed ✅)
- [x] gRPC native transport (separate from server module)
  - Port 8771; `AsyncGenericService` bidirectional streaming; guarded by `THEMIS_ENABLE_GRPC`
- [x] Connection multiplexing (multiple logical streams per TCP connection)
- [x] Per-tenant network bandwidth quotas
- [x] Connection-level compression (LZ4, Zstd)
- [x] Network topology-aware routing for geo-distributed clusters
  - `GeoTopologyRouter` (include/network/geo_topology_router.h); strategies: PREFER_LOCAL, LOWEST_LATENCY, ROUND_ROBIN
- [x] Service mesh integration (Istio/Envoy sidecar compatibility)
  - `ServiceMeshIntegration` probe server + `EnvoyXdsClient` xDS v3 REST polling; guarded by `THEMIS_ENABLE_SERVICE_MESH`

## Production Readiness Checklist
- [x] Wire Protocol V1 opcode handlers implemented: HELLO, AUTH_REQUEST, GET, PUT, DELETE, QUERY_AQL, VECTOR_SEARCH, GEO_QUERY (2026-03-10)
  - HELLO: server info + capabilities response
  - AUTH_REQUEST: token validation, `authenticated_.store(true)`, stats.auth_failures accounting
  - GET/PUT/DELETE: RocksDB dispatch with collection-prefixed keys
  - QUERY_AQL / GEO_QUERY: structured error with HTTP REST API redirect hint
  - VECTOR_SEARCH: `VectorIndexManager::searchKnn()` dispatch
- [x] Authentication wired: `authenticated_` flag correctly set after successful AUTH (2026-03-10)
- [x] `Config::auth_token` field added for pre-shared token validation (2026-03-10)
- [x] Focused standalone test targets added for network components in `tests/CMakeLists.txt` (2026-03-10):
  - `WireProtocolV1HandlersFocusedTests` (`test_wire_protocol_v1_handlers.cpp`) — Config defaults, auth decision logic, response contracts
  - `QoSManagerFocusedTests`, `NetworkTimeoutFocusedTests`, `WireProtocolConnectionPoolFocusedTests`
  - `BandwidthManagementQoSFocusedTests` (`test_bandwidth_management_qos.cpp`, 2026-03-15) — LeakyBucket, CongestionController, priority queuing, fair queuing, Linux tc
  - `WireProtocolPerformanceFocusedTests`, `UDPFastPathFocusedTests`, `GeoTopologyRouterFocusedTests`
  - `WireProtocolV2FocusedTests`, `WireProtocolWebSocketFocusedTests` (THEMIS_ENABLE_WEBSOCKET)
  - `QuicTransportFocusedTests` (THEMIS_ENABLE_HTTP3), `GrpcTransportFocusedTests` (THEMIS_ENABLE_GRPC)
  - `NetworkCircuitBreakerFocusedTests` (`test_network_circuit_breaker.cpp`, 2026-03-11) — AdaptiveCircuitBreaker
- [x] Unit tests added for WebSocket upgrade (`test_wire_protocol_websocket.cpp`)
- [x] Protocol detection logic tested (8 test cases covering all relevant prefixes)
- [x] Security: connection-count accounting correct across WS upgrade
- [x] `getActiveConnections()` counts both binary and WebSocket sessions
- [x] Binary/text frame mode correctly tracked per queued message
- [x] Welcome frame sent on connect; graceful close handling
- [x] V2 multiplexed protocol fully implemented (`wire_protocol_v2.cpp`)
  - Frame types: DATA, HEADERS, RST_STREAM, SETTINGS, PING, GOAWAY, WINDOW_UPDATE, PUSH_PROMISE
  - Stream state machine: IDLE → OPEN → HALF_CLOSED_{LOCAL,REMOTE} → CLOSED
  - Flow control: per-stream WINDOW_UPDATE on DATA frame receipt
  - Server push: `push_promise()` + `send_data()` on even stream IDs
  - Session cleanup: disconnected sessions erased from server map
  - Buffer lifetime safety: all async writes use `shared_ptr`-owned buffers
  - COMPRESSED DATA frames guarded with RST_STREAM (LZ4 tracked in #2416)
- [x] Unit tests added for V2 protocol (`test_wire_protocol_v2.cpp`, 29 tests)
- [x] Unit tests added for QUIC transport (`test_quic_transport.cpp`, 17 tests)
  - Config defaults, port validation, stats initialisation, protocol constants, isRunning state
- [x] QUIC/HTTP3 stub resolved (`Http3Session::doRead()`, `Http3Session::onRead()`)
- [x] Unit tests added for gRPC native transport (`test_grpc_transport.cpp`, 16 tests)
  - Config defaults, TLS flags, port validation (incl. conflict with 50051), stats, address format, isRunning state
- [x] Unit tests added for geo topology router (`test_geo_topology_router.cpp`, 26 tests)
  - Config defaults, PREFER_LOCAL/LOWEST_LATENCY/ROUND_ROBIN strategies, zone/datacenter affinity
  - Cross-region fallback, selectEndpointInRegion, getRankedShards ordering, stats accumulation, edge cases
- [x] Unit tests added for backpressure / TCP backlog management (`test_wire_protocol_backpressure.cpp`, 18 tests)
  - tcp_backlog default (128), custom/high values, min-one edge case
  - max_connections default and reconfiguration (including unlimited=0)
  - rejected_connections stat starts at zero; all Stats fields default to zero
  - overloaded_ state-machine: not overloaded initially, set on first rejection,
    cleared on recovery; unlimited config never overloads
  - Config field coexistence (tcp_backlog, max_connections, port, TLS)
- [?] Integration tests (TLS handshake with WS upgrade, rate-limit enforcement for WS)
- [?] Performance benchmarks (connections/sec via WS vs. native binary)
- [?] Full binary frame dispatch over WebSocket (text/JSON frames fully functional)
- [x] Unit tests added for UDP ingestion server (`test_udp_server.cpp`, 59 tests, 2026-03-15)
  - Magic bytes, version/opcode constants, header/ACK sizes, config defaults, stats initialization
- [x] Unit tests added for wire protocol optimizations (`test_wire_protocol_optimizations.cpp`, 39 tests, 2026-03-14)
  - `ZeroCopyFrameBuilder`: header-only frame, frame+payload, size check, stats tracking
  - `MemoryMappedPayload`: anonymous mapping, write/read, zero-size throws, move semantics, file mapping
  - `NagleController`: default mode, TCP_NODELAY, TCP_CORK, uncork restores mode, invalid fd
  - `WireProtocolBatcher`: coalescing correctness, flush, stats
  - `ZstdDictionaryCompressor`: train+compress+decompress round-trip, fallback without dict
- [x] Unit tests added for RaftLoadBalancer (`test_raft_load_balancer.cpp`, 26 tests, 2026-03-15)
  - Leader election, health-based failover, dynamic weight updates, cross-datacenter routing
  - All 5 strategies: ROUND_ROBIN, LEAST_CONNECTIONS, WEIGHTED_ROUND_ROBIN, HEALTH_BASED, CONSISTENT_HASH
  - Add/remove/duplicate-add backend, stats tracking, strategy switching at runtime

## Known Issues & Limitations
- Wire Protocol V1 opcode handlers (HELLO, AUTH, GET, PUT, DELETE) are fully implemented
  (resolved 2026-03-10, see `NETWORK-MISSING-001`/`NETWORK-MISSING-002`).
  QUERY_AQL and GEO_QUERY return structured errors directing clients to the HTTP REST API;
  VECTOR_SEARCH dispatches to `VectorIndexManager::searchKnn`.
- `grpc_transport.cpp` was missing from `cmake/CMakeLists.txt` (fixed: 2026-03-09).
- `envoy_xds.cpp`, `service_mesh.cpp`, `socket_timeout_manager.cpp`, `udp_fast_path.cpp`, `wire_protocol_server_ws.cpp` were missing from `cmake/ModularBuild.cmake` (fixed: 2026-03-10).
- WebSocket upgrade support is implemented; binary frames over WebSocket are not yet
  dispatched (clients receive a structured error and should use text/JSON frames or
  the native TCP binary connection).
- UDP fast-path is implemented (`UDPFastPath`, port 8769); UDP ingestion server is implemented (`UDPServer`, port 8768); QUIC transport is implemented (`QuicTransport`, port 8770).
- gRPC native transport is implemented (`GrpcTransport`, port 8771); this module provides
  the transport layer only — the gRPC service layer lives in the server/api modules.
- Service mesh integration is implemented (`ServiceMeshIntegration`, port 8082);
  see `include/network/service_mesh.h` / `src/network/service_mesh.cpp`.
  Guarded by `THEMIS_ENABLE_SERVICE_MESH`.
- `RaftLoadBalancer` simulates Raft consensus in-process (leader election + follower replication
  without real network RPC); full distributed multi-node Raft is planned for a future milestone.

## Breaking Changes
- Wire protocol frame format is versioned; v2 frame format planned with extended metadata fields.
- `WireProtocolServer::Config` gained new field `auth_token` (default: empty string);
  defaults remain backward-compatible.
- `WireProtocolServer::Config` gained new fields `enable_ipv6` (default: false) and
  `ipv6_dual_stack` (default: true); existing deployments are unaffected (default binding
  remains IPv4 "0.0.0.0").
- `WireProtocolServer::Config` gained new field `tcp_backlog` (default: 128); existing
  deployments are unaffected as 128 matches the previous implicit OS default.

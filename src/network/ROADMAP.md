# Network Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-grade networking layer. Binary wire protocol server, connection pooling, TLS/mTLS, circuit breaker, and rate limiting are fully implemented.

## Completed ✅
- [x] WireProtocolServer – high-performance binary TCP server (port 8766)
- [x] Dedicated I/O thread pool + worker thread pool separation
- [x] Connection pool management (client-side and server-side)
- [x] TLS 1.3 and mutual TLS (mTLS) support
- [x] Per-IP rate limiting (requests/sec and requests/min)
- [x] Connection limits (global and per-IP)
- [x] Circuit breaker pattern for socket timeouts
- [x] Protocol buffer wire format helpers (lightweight parser/serializer)
- [x] Authentication (token-based) with configurable auth timeout
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

## In Progress 🚧
- [~] UDP-based fast-path for read-only queries (Target: Q3 2026) (Issue: #1962) (PR: #3098)
  - UDP socket on port 8769 (dedicated, separate from TCP wire protocol port 8766)
  - Read-only opcodes only: GET, QUERY_AQL, VECTOR_SEARCH, PING
  - Per-source-IP rate limiting (configurable packets/second)
  - Compact 10-byte binary header with request-ID echo for correlation
  - `UDPFastPath` class in `include/network/udp_fast_path.h` / `src/network/udp_fast_path.cpp`
  - Unit tests in `tests/test_udp_fast_path.cpp` (config, packet validation, opcode filter, response builder)
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
- [?] Adaptive I/O thread scaling based on connection load
- [?] Structured network audit log (connection open/close/auth events)

### Long-term (6-12 months)
- [~] Service mesh integration (Istio/Envoy sidecar compatibility) (Issue: #2417)
- [?] RDMA support for ultra-low-latency inter-node communication
- [?] IPv6 dual-stack support
- [x] gRPC native transport (separate from server module) (Issue: #2024)
  - `GrpcTransport` class in `include/network/grpc_transport.h` / `src/network/grpc_transport.cpp`
  - Port 8771 (dedicated; does not conflict with TCP 8766, UDP 8769, QUIC 8770, or gRPC API 50051)
  - Carries binary wire protocol frames over gRPC `AsyncGenericService` bidirectional streaming
  - TLS / mTLS credentials via gRPC server credentials (same cert/key pattern as api/grpc_server.cpp)
  - Configurable: num_threads, max_connections, max_message_size_bytes, keepalive
  - Guarded by `THEMIS_ENABLE_GRPC`; unit tests in `tests/test_grpc_transport.cpp` (16 tests)
- [I] Network topology-aware routing for geo-distributed clusters (Issue: #2207)

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
- [~] UDP-based fast-path for read-only queries (Target: Q3 2026)
- [x] QUIC/HTTP3 transport layer integration (Target: Q3 2026)

### Phase 3: Advanced Networking & Service Mesh (Status: Planned 📋)
- [x] gRPC native transport (separate from server module)
  - Port 8771; `AsyncGenericService` bidirectional streaming; guarded by `THEMIS_ENABLE_GRPC`
- [x] Connection multiplexing (multiple logical streams per TCP connection)
- [x] Per-tenant network bandwidth quotas
- [x] Connection-level compression (LZ4, Zstd)
- [ ] Network topology-aware routing for geo-distributed clusters
- [~] Service mesh integration (Istio/Envoy sidecar compatibility)

## Production Readiness Checklist
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
- [~] Unit tests added for service mesh integration (`test_service_mesh.cpp`)
  - Config defaults, port validation, TLS offload flag, Istio annotation helpers, isRunning state, start/stop lifecycle
- [?] Integration tests (Istio sidecar injection, probe server reachability, drain timeout)
- [?] Performance benchmarks (connections/sec via WS vs. native binary)
- [?] Full binary frame dispatch over WebSocket (text/JSON frames fully functional)

## Known Issues & Limitations
- WebSocket upgrade support is implemented; binary frames over WebSocket are not yet
  dispatched (clients receive a structured error and should use text/JSON frames or
  the native TCP binary connection).
- UDP fast-path is in progress; QUIC transport is implemented (`QuicTransport`, port 8770).
- gRPC native transport is implemented (`GrpcTransport`, port 8771); this module provides
  the transport layer only — the gRPC service layer lives in the server/api modules.
- Service mesh integration is in progress (`ServiceMeshIntegration`, port 8082);
  see `include/network/service_mesh.h` / `src/network/service_mesh.cpp`.
  Guarded by `THEMIS_ENABLE_SERVICE_MESH`.

## Breaking Changes
- Wire protocol frame format is versioned; v2 frame format planned with extended metadata fields.
- `WireProtocolServer::Config` may gain new fields; defaults remain backward-compatible.

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

## In Progress 🚧
- [I] UDP-based fast-path for read-only queries (Target: Q3 2026) (Issue: #1962)
- [I] QUIC/HTTP3 transport layer integration (Target: Q3 2026) (Issue: #1994)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [!] Connection multiplexing (multiple logical streams per TCP connection) (Issue: #2415)
- [?] Adaptive I/O thread scaling based on connection load
- [I] Per-tenant network bandwidth quotas (Issue: #2205)
- [!] Connection-level compression (LZ4, Zstd) (Issue: #2416)
- [?] Structured network audit log (connection open/close/auth events)

### Long-term (6-12 months)
- [!] Service mesh integration (Istio/Envoy sidecar compatibility) (Issue: #2417)
- [?] RDMA support for ultra-low-latency inter-node communication
- [?] IPv6 dual-stack support
- [I] gRPC native transport (separate from server module) (Issue: #2024)
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
- [ ] UDP-based fast-path for read-only queries (Target: Q3 2026)
- [ ] QUIC/HTTP3 transport layer integration (Target: Q3 2026)

### Phase 3: Advanced Networking & Service Mesh (Status: Planned 📋)
- [ ] gRPC native transport (separate from server module)
- [ ] Connection multiplexing (multiple logical streams per TCP connection)
- [ ] Per-tenant network bandwidth quotas
- [ ] Connection-level compression (LZ4, Zstd)
- [ ] Network topology-aware routing for geo-distributed clusters
- [ ] Service mesh integration (Istio/Envoy sidecar compatibility)

## Production Readiness Checklist
- [x] Unit tests added for WebSocket upgrade (`test_wire_protocol_websocket.cpp`)
- [x] Protocol detection logic tested (8 test cases covering all relevant prefixes)
- [x] Security: connection-count accounting correct across WS upgrade
- [x] `getActiveConnections()` counts both binary and WebSocket sessions
- [x] Binary/text frame mode correctly tracked per queued message
- [x] Welcome frame sent on connect; graceful close handling
- [?] Integration tests (TLS handshake with WS upgrade, rate-limit enforcement for WS)
- [?] Performance benchmarks (connections/sec via WS vs. native binary)
- [?] Full binary frame dispatch over WebSocket (text/JSON frames fully functional)

## Known Issues & Limitations
- WebSocket upgrade support is implemented; binary frames over WebSocket are not yet
  dispatched (clients receive a structured error and should use text/JSON frames or
  the native TCP binary connection).
- UDP and QUIC transports are not yet implemented.
- gRPC server is handled by the server module; this module provides only the binary wire protocol.
- Service mesh integration is a future enhancement.

## Breaking Changes
- Wire protocol frame format is versioned; v2 frame format planned with extended metadata fields.
- `WireProtocolServer::Config` may gain new fields; defaults remain backward-compatible.

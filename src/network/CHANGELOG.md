<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Network Module

## [Unreleased]
- Full binary frame dispatch over WebSocket (in progress)
- Integration tests for TLS + WebSocket
- Performance benchmarks for all transports
- IPv6 CIDR policies in ZeroTrustPolicyEnforcer

---

## [1.5.0] — Service Mesh & Advanced Routing
### Added
- Service mesh integration with Envoy xDS API (`envoy_xds.cpp`)
- Geo-topology-aware routing (`geo_topology_router.cpp`)
- Per-tenant bandwidth quotas via QoS manager (`qos_manager.cpp`)
- Adaptive circuit breaker with dynamic threshold tuning (`adaptive_circuit_breaker.cpp`)

### Changed
- Connection pool sizing is now fully adaptive based on observed latency and queue depth

---

## [1.4.0] — Multiplexing & Compression
### Added
- Connection multiplexing via Wire Protocol V2 frame types (`wire_protocol_v2.cpp`)
- LZ4 and Zstd per-connection compression negotiated at handshake
- TCP backlog management with configurable backpressure handling
- Socket timeout management (`socket_timeout_manager.cpp`)

---

## [1.3.0] — Multi-Transport Support
### Added
- QUIC / HTTP3 transport on port 8770 (`quic_transport.cpp`)
- gRPC native transport on port 8771 (`grpc_transport.cpp`)
- UDP fast-path for read-only queries on port 8769 (`udp_fast_path.cpp`)

---

## [1.2.0] — WebSocket & IPv6
### Added
- WebSocket upgrade on port 8766 — text/JSON frames (`wire_protocol_server_ws.cpp`)
- IPv6 dual-stack support across all transports
- Per-connection helper utilities (`wire_protocol_helpers.cpp`)
- Performance instrumentation for protocol hot paths (`wire_protocol_performance.cpp`)

---

## [1.1.0] — Security & Rate Limiting
### Added
- TLS 1.3 and mutual TLS (mTLS) enforcement
- Per-IP rate limiting and per-connection limits
- Token-based authentication integrated into Wire Protocol V1 handshake
- Protocol detection on magic bytes to reject insecure/invalid upgrade attempts

---

## [1.0.0] — Wire Protocol V1 Core
### Added
- `WireProtocolServer` — binary TCP on port 8766 (`wire_protocol_server.cpp`)
- Wire Protocol V1 opcodes: `HELLO`, `AUTH`, `GET`, `PUT`, `DELETE`, `QUERY_AQL`, `VECTOR_SEARCH`, `GEO_QUERY`
- Connection pool management (`wire_protocol_connection_pool.cpp`)
- Circuit breaker (initial static-threshold implementation)

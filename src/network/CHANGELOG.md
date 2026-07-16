> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Network Module

## [Unreleased]
- Documentation governance sync: ROADMAP/FUTURE moved to future-only planning, and module docs aligned with source-verified wording (`README`, `ARCHITECTURE`, `SECURITY`, `PERFORMANCE_EXPECTATIONS`, `AUDIT`).
- Full binary frame dispatch over WebSocket (in progress)
- Integration tests for TLS + WebSocket
- Performance benchmarks for all transports
- IPv6 CIDR policies in ZeroTrustPolicyEnforcer
- Full distributed multi-node Raft consensus for RaftLoadBalancer

---

## [1.8.0] — Performance Optimizations, UDP Ingestion, and Raft Load Balancing
### Added
- **Wire protocol batch writes** (`wire_protocol_batch.cpp`): `WireProtocolBatcher` coalesces outbound frames via `writev(2)`; `NagleController` manages `TCP_CORK`/`TCP_NOPUSH` per socket; ~10× syscall reduction for small-message workloads
- **Zero-copy serialization** (`wire_protocol_zero_copy.cpp`): `ZeroCopyFrameBuilder` writes header + payload in a single `writev(2)` without heap allocation; `MemoryMappedPayload` enables true zero-copy for file-backed payloads via mmap/sendfile
- **Dictionary-trained Zstd compression** (`connection_compression.cpp`): `ZstdDictionaryCompressor` trains on representative samples and reuses the dictionary for all subsequent compress/decompress calls; falls back to generic Zstd on training failure
- **UDP ingestion server** (`udp_server.cpp`, port 8768): fire-and-forget transport for metrics, logs, events, and batched payloads; optional ACK with sequence-number deduplication; opcodes: `METRIC`, `LOG`, `EVENT`, `BATCH`, `PING`
- **Raft-coordinated load balancer** (`raft_load_balancer.cpp`, port 8774): five routing strategies (`ROUND_ROBIN`, `LEAST_CONNECTIONS`, `WEIGHTED_ROUND_ROBIN`, `HEALTH_BASED`, `CONSISTENT_HASH`); automatic health-based failover; cross-datacenter affinity; `Stats` counters for Prometheus
- Focused unit tests: `test_wire_protocol_optimizations.cpp` (39 tests), `test_udp_server.cpp` (59 tests), `test_raft_load_balancer.cpp` (26 tests)

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

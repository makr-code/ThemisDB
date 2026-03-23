<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Network Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/network/CHANGELOG.md`.

## [1.8.0] — 2026-01

### Added
- `wire_protocol_batch.h` — `WireProtocolBatcher` + `NagleController` for Nagle-style frame coalescing.
- `wire_protocol_zero_copy.h` — `ZeroCopyFrameBuilder` + `MemoryMappedPayload` for sendfile/mmap dispatch.
- `connection_compression.h` — `ZstdDictionaryCompressor` with shared-dictionary negotiation.
- `udp_server.h` — `UDPServer` on port 8768 with configurable backpressure queue.
- `raft_load_balancer.h` — `RaftLoadBalancer` on port 8774 with quorum-based routing.

## [1.5.0] — 2025-09

### Added
- `envoy_xds.h` — Envoy xDS v3 discovery client.
- `geo_topology_router.h` — `GeoTopologyRouter` for latency-zone-aware routing.
- `qos_manager.h` — `QosManager` with DSCP/TOS marking.
- `adaptive_circuit_breaker.h` — `AdaptiveCircuitBreaker` with sliding-window error rates.
- `service_mesh.h` — mTLS sidecar service mesh integration.

## [1.4.0] — 2025-06

### Added
- `wire_protocol_connection_pool.h` — Connection multiplexing with health-check eviction.
- `grpc_transport.h` — gRPC bidirectional streaming transport.
- `quic_transport.h` — QUIC/HTTP-3 transport with 0-RTT reconnect.
- `wire_protocol_websocket.h` — WebSocket frame adapter.
- `wire_protocol_server.h` — Multi-threaded epoll/kqueue frame server.
- `wire_protocol_helpers.h` — CRC32C framing utilities.
- `wire_protocol_performance.h` — Per-connection latency counters.
- `socket_timeout_manager.h` — Per-socket deadline management.
- `udp_fast_path.h` — io_uring/SO_ZEROCOPY UDP fast path.

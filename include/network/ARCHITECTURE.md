<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Network Module — Architecture Guide

## Overview

The network module provides all transport-layer infrastructure for ThemisDB: wire protocol framing, connection pooling, compression, load balancing, service mesh integration, QoS, and zero-copy I/O. It supports TCP, UDP, QUIC, gRPC, and WebSocket transports with Raft-based load balancing, Envoy xDS service discovery, and nagle-style batching.

## Design Principles

- **Zero-copy first** — `ZeroCopyFrameBuilder` and `MemoryMappedPayload` eliminate buffer copies on the hot path.
- **Pluggable transports** — gRPC, QUIC, UDP, and WebSocket share a common frame abstraction.
- **Resilience by default** — `AdaptiveCircuitBreaker` and `RaftLoadBalancer` provide automatic failover.
- **Observability** — Every connection, batch, and compression event emits Prometheus-compatible metrics.
- **Geo-aware routing** — `GeoTopologyRouter` selects replicas by latency zone before load metrics.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `adaptive_circuit_breaker.h` | `AdaptiveCircuitBreaker` | Adaptive half-open circuit breaking with sliding-window error rates |
| `connection_compression.h` | `ZstdDictionaryCompressor` | Zstd shared-dictionary compression for wire payloads |
| `envoy_xds.h` | `EnvoyXdsClient` | Envoy xDS v3 discovery for dynamic cluster/endpoint updates |
| `geo_topology_router.h` | `GeoTopologyRouter` | Latency-zone-aware replica selection |
| `grpc_transport.h` | `GrpcTransport` | gRPC bidirectional streaming transport adapter |
| `qos_manager.h` | `QosManager` | Per-connection QoS classification and rate limiting |
| `quic_transport.h` | `QuicTransport` | QUIC/HTTP-3 transport (0-RTT reconnect) |
| `raft_load_balancer.h` | `RaftLoadBalancer` | Raft-based consensus load balancer (port 8774) |
| `service_mesh.h` | `ServiceMeshClient` | Sidecar-based mTLS service mesh integration |
| `socket_timeout_manager.h` | `SocketTimeoutManager` | Per-socket read/write deadline management |
| `udp_fast_path.h` | `UdpFastPath` | Kernel-bypass UDP via SO_ZEROCOPY / io_uring |
| `udp_server.h` | `UDPServer` | UDP server on port 8768 with backpressure |
| `wire_protocol_batch.h` | `WireProtocolBatcher`, `NagleController` | Nagle-style frame batching with configurable flush intervals |
| `wire_protocol_connection_pool.h` | `WireProtocolConnectionPool` | Multiplexed connection pool with health-check eviction |
| `wire_protocol_helpers.h` | `WireProtocolHelpers` | CRC32C framing helpers, endian conversion |
| `wire_protocol_performance.h` | `WireProtocolPerformance` | Latency histogram and throughput counters |
| `wire_protocol_server.h` | `WireProtocolServer` | Multi-threaded frame server with epoll/kqueue backend |
| `wire_protocol_websocket.h` | `WireProtocolWebSocket` | WebSocket frame adapter for browser clients |
| `wire_protocol_zero_copy.h` | `ZeroCopyFrameBuilder`, `MemoryMappedPayload` | sendfile/mmap-based zero-copy frame dispatch |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `replication` | `GrpcTransport` / `RaftLoadBalancer` | Raft log replication and leader election |
| `observability` | `WireProtocolPerformance` | Exposes per-connection latency histograms |
| `query` | `WireProtocolServer` | Query result streaming |
| `scheduler` | `UDPServer` | Low-latency task dispatch |
| `service mesh (Envoy)` | `EnvoyXdsClient` | Dynamic endpoint discovery |

## Implementation

Implementation in `../../src/network/`.

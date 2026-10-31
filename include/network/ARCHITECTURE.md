---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

> **Build/tests:** `cmake --preset linux-release -DTHEMIS_BUILD_TESTS=ON`, then build in `build-gcc-linux-release`.

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/network/ARCHITECTURE.md -->

# Network Module — Public Header Architecture

**Module Path:** `include/network/`
**Implementation:** `../../src/network/`
**Canonical architecture doc:** [`../../src/network/ARCHITECTURE.md`](../../src/network/ARCHITECTURE.md)

---

## 1. Overview

`include/network/` contains 31 public header files for the network module. Their declarations cover wire framing and server lifecycle, transport adapters, connection pooling, resilience and QoS, topology routing, optional service-mesh integration, and observability. Optional components remain subject to the build definitions documented by their headers and in `cmake/CMakeLists.txt`.

Full design — io_uring ring sizing, QUIC flow control, circuit breaker state machine, envoy xDS — is in:
→ [`../../src/network/ARCHITECTURE.md`](../../src/network/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Wire Protocol Stack

| Header | Public Type | Purpose |
|--------|------------|---------|
| `wire_protocol_server.h` | `WireProtocolServer` | Root TCP wire protocol server |
| `wire_protocol_connection_pool.h` | `WireProtocolConnectionPool` | Connection pool management |
| `wire_protocol.h` | `PacketParser`, `PacketBuilder` | Standalone packet framing helpers |
| `wire_protocol_batch.h` | `WireProtocolBatcher`, `NagleController` | Batched writes and socket coalescing |
| `wire_protocol_helpers.h` | `ProtobufParser`, `ProtobufSerializer` | Lightweight protobuf wire-format helpers |
| `network_api_contract.h` | `NetworkErrorCode` | Shared network error categories and contract constants |
| `wire_protocol_performance.h` | `WireProtocolMetrics`, `PayloadBufferPool` | Performance counters and payload buffer reuse |
| `wire_protocol_websocket.h` | `WireProtocolWebSocketSession` | WebSocket upgrade/session framing |
| `wire_protocol_zero_copy.h` | `ZeroCopyFrameBuilder`, `MemoryMappedPayload` | Scatter-gather and file-backed payload writes |
| `wire_retry_policy.h` | `WireRetryPolicy` | Bounded retry policy |
| `wire_bootstrap_validation.h` | `RequiredBackend` | Startup dependency validation types |

### 2.2 Transport and I/O

| Header | Public Type | Purpose |
|--------|------------|---------|
| `quic_server.h` | `QUICServer` | QUIC/HTTP3 transport server |
| `quic_transport.h` | `QuicTransport` | QUIC connection and stream types |
| `grpc_transport.h` | `GrpcTransport` | gRPC channel transport binding |
| `io_uring_batcher.h` | `IoUringBatchedSender` | io_uring submission ring batcher |
| `kernel_bypass.h` | `DPDKServer`, `IoUringServer` | Optional DPDK and io_uring server paths |
| `udp_server.h` | `UDPServer` | UDP datagram server |
| `udp_fast_path.h` | `UDPFastPath` | UDP fast-path with GSO/GRO |
| `multipath_tcp.h` | `MptcpSubflowManager` | Multipath TCP subflow management |
| `bbr_congestion_control.h` | `BbrConfig`, `BbrCongestionController` | BBR congestion-control configuration and helper |

### 2.3 Resilience and QoS

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adaptive_circuit_breaker.h` | `AdaptiveCircuitBreaker` | Adaptive circuit breaker (closed/open/half-open) |
| `adaptive_io_scaler.h` | `AdaptiveIOScaler` | Dynamic I/O concurrency scaling |
| `qos_manager.h` | `QoSManager` | Per-connection QoS classification |
| `connection_compression.h` | `ZstdDictionaryCompressor` | Dictionary-trained Zstd helper |
| `socket_timeout_manager.h` | `SocketTimeoutManager` | Per-socket read/write timeout enforcement |

### 2.4 Routing and Service Mesh

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geo_topology_router.h` | `GeoTopologyRouter` | Geo-aware latency routing |
| `service_mesh.h` | `ServiceMeshIntegration` | Service-mesh health-probe server |
| `envoy_xds.h` | `EnvoyXdsClient` | Envoy xDS REST polling client |
| `raft_load_balancer.h` | `RaftLoadBalancer` | Raft-leader-aware load balancer |
| `network_observability.h` | `INetworkObservabilitySink`, `NetworkObservabilityRegistry` | Optional event sink and counters |

### 2.5 Auxiliary

| Header | Public Type | Purpose |
|--------|------------|---------|
| `network_audit_log.h` | `NetworkAuditLog` | Per-connection security audit |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::network` | Main network public types |
| Header-specific nested namespace | See the declaration in each header; do not assume `wire`, `quic`, or `mesh` namespaces |

## Build and test registration

The network implementation is part of the `themis_core` source set in
`cmake/CMakeLists.txt`. WebSocket, HTTP/3, gRPC, and service-mesh sources are
conditionally added by their `THEMIS_ENABLE_*` definitions. Focused test
targets are generated from `tests/network/test_*.cpp`; see
[`src/network/MODULE_GUIDE.md`](../../src/network/MODULE_GUIDE.md) for a
test-enabled configure and focused CTest command.

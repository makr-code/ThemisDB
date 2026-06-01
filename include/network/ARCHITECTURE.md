> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/network/ARCHITECTURE.md -->

# Network Module — Public Header Architecture

**Module Path:** `include/network/`
**Implementation:** `../../src/network/`
**Canonical architecture doc:** [`../../src/network/ARCHITECTURE.md`](../../src/network/ARCHITECTURE.md)

---

## 1. Overview

`include/network/` defines the public C++ contract for ThemisDB's network layer. With 25 headers the module covers: the wire protocol stack (TCP/UDP/WebSocket/zero-copy), QUIC server, io_uring async I/O, kernel bypass, circuit breaking, QoS management, geo-topology routing, service mesh, and audit logging.

Full design — io_uring ring sizing, QUIC flow control, circuit breaker state machine, envoy xDS — is in:
→ [`../../src/network/ARCHITECTURE.md`](../../src/network/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Wire Protocol Stack

| Header | Public Type | Purpose |
|--------|------------|---------|
| `wire_protocol_server.h` | `WireProtocolServer` | Root TCP wire protocol server |
| `wire_protocol_connection_pool.h` | `WireProtocolConnectionPool` | Connection pool management |
| `wire_protocol_batch.h` | `WireProtocolBatch` | Batched request framing |
| `wire_protocol_helpers.h` | — | Framing utility functions |
| `wire_protocol_performance.h` | `WireProtocolPerformanceConfig` | Throughput tuning parameters |
| `wire_protocol_websocket.h` | `WireProtocolWebSocket` | WebSocket wire protocol framing |
| `wire_protocol_zero_copy.h` | `WireProtocolZeroCopy` | Zero-copy scatter-gather I/O |

### 2.2 Transport and I/O

| Header | Public Type | Purpose |
|--------|------------|---------|
| `quic_server.h` | `QUICServer` | QUIC/HTTP3 transport server |
| `quic_transport.h` | `QUICTransport` | QUIC connection and stream types |
| `grpc_transport.h` | `GRPCTransport` | gRPC channel transport binding |
| `io_uring_batcher.h` | `IoUringBatcher` | io_uring submission ring batcher |
| `kernel_bypass.h` | `KernelBypass` | DPDK/RDMA kernel bypass |
| `udp_server.h` | `UDPServer` | UDP datagram server |
| `udp_fast_path.h` | `UDPFastPath` | UDP fast-path with GSO/GRO |

### 2.3 Resilience and QoS

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adaptive_circuit_breaker.h` | `AdaptiveCircuitBreaker` | Adaptive circuit breaker (closed/open/half-open) |
| `adaptive_io_scaler.h` | `AdaptiveIOScaler` | Dynamic I/O concurrency scaling |
| `qos_manager.h` | `QoSManager` | Per-connection QoS classification |
| `connection_compression.h` | `ConnectionCompression` | On-wire compression (zstd/lz4) |
| `socket_timeout_manager.h` | `SocketTimeoutManager` | Per-socket read/write timeout enforcement |

### 2.4 Routing and Service Mesh

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geo_topology_router.h` | `GeoTopologyRouter` | Geo-aware latency routing |
| `service_mesh.h` | `ServiceMesh` | Service mesh sidecar integration |
| `envoy_xds.h` | `EnvoyXDSClient` | Envoy xDS control-plane client |
| `raft_load_balancer.h` | `RaftLoadBalancer` | Raft-leader-aware load balancer |

### 2.5 Auxiliary

| Header | Public Type | Purpose |
|--------|------------|---------|
| `network_audit_log.h` | `NetworkAuditLog` | Per-connection security audit |
| `wire_bootstrap_validation.h` | `WireBootstrapValidation` | Startup wire-protocol self-test |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::network` | All network types |
| `themis::network::wire` | Wire protocol stack types |
| `themis::network::quic` | QUIC/HTTP3 transport types |
| `themis::network::mesh` | Service mesh and routing types |

# Network Module — Architecture Guide

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/network/README.md -->

**Version:** 1.2
**Last Updated:** 2026-04-06
**Module Path:** `src/network/`

---

## 1. Overview

The Network module implements ThemisDB's high-performance, secure networking layer for the
binary wire protocol. It provides the TCP server, connection pool, TLS/mTLS configuration,
socket timeout management, Quality-of-Service (QoS) management, and protocol buffer helpers.

In addition to the core TCP wire protocol, the module provides WebSocket upgrade
(port 8766, `wire_protocol_server_ws.cpp`), Wire Protocol V2 multiplexing
(`wire_protocol_v2.cpp`), UDP fast-path (port 8769, `udp_fast_path.cpp`), UDP ingestion
server (port 8768, `udp_server.cpp`), QUIC/HTTP3 transport (port 8770, `quic_transport.cpp`),
gRPC native transport (port 8771, `grpc_transport.cpp`), Raft-coordinated load balancing
(port 8774, `raft_load_balancer.cpp`), geo-topology routing, Istio/Envoy service mesh
integration, dictionary-trained Zstd compression (`connection_compression.cpp`), and
zero-copy/batch-write optimizations (`wire_protocol_zero_copy.cpp`, `wire_protocol_batch.cpp`).

The wire protocol is an alternative to HTTP/REST — it is a binary framing protocol
optimized for low-latency, high-throughput client connections. HTTP/REST and the gRPC
service layer (ThemisDBService, WalGrpcService) are handled by the `api` and `server`
modules; `grpc_transport.cpp` provides **transport only** (raw binary frames over gRPC
bidirectional streaming).

---

## 2. Design Principles

- **Separate I/O and Worker Threads** – dedicated I/O thread pool handles accept/read/write;
  a larger worker thread pool handles request processing.
- **Circuit Breaker for Connections** – `socket_timeout_manager.cpp` monitors idle and
  timed-out connections and enforces circuit-breaker logic.
- **TLS First** – plaintext connections are allowed only for localhost; all remote
  connections require TLS 1.3.
- **Per-IP Rate Limiting** – connection count and request rate are enforced per source IP
  to prevent abuse.
- **QoS** – `qos_manager.cpp` classifies traffic by tenant/priority and enforces bandwidth
  quotas and latency SLAs.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `wire_protocol_server.cpp` | Core TCP server: accept, rate limiting, frame dispatch |
| `wire_protocol_connection_pool.cpp` | Client-side connection pool for outbound connections |
| `wire_protocol_helpers.cpp` | Frame serialization/deserialization, message types |
| `wire_protocol_performance.cpp` | Performance monitoring for the wire protocol |
| `wire_protocol_server_ws.cpp` | WebSocket upgrade on port 8766 (`THEMIS_ENABLE_WEBSOCKET`) |
| `wire_protocol_v2.cpp` | Wire protocol v2: multi-stream, flow control, server push |
| `wire_protocol_batch.cpp` | Batch write processor: `WireProtocolBatcher` (writev coalescing) + `NagleController` (TCP_CORK/TCP_NOPUSH) |
| `wire_protocol_zero_copy.cpp` | Zero-copy serialization: `ZeroCopyFrameBuilder` + `MemoryMappedPayload` (mmap/sendfile) |
| `qos_manager.cpp` | QoS: traffic classification, bandwidth quotas, priority queuing |
| `socket_timeout_manager.cpp` | Socket timeout enforcement, circuit breaker |
| `adaptive_circuit_breaker.cpp` | Adaptive circuit breaker with load-adaptive threshold tuning |
| `connection_compression.cpp` | `ZstdDictionaryCompressor` — dictionary-trained Zstd for wire payloads |
| `udp_fast_path.cpp` | UDP read-only fast-path (port 8769) |
| `udp_server.cpp` | UDP ingestion server (port 8768): fire-and-forget metrics/logs/events |
| `quic_transport.cpp` | QUIC/HTTP3 transport (port 8770, `THEMIS_ENABLE_HTTP3`) |
| `grpc_transport.cpp` | gRPC native transport (port 8771, `THEMIS_ENABLE_GRPC`) |
| `raft_load_balancer.cpp` | Raft-coordinated load balancer (port 8774): leader election, health-based routing, consistent hashing |
| `geo_topology_router.cpp` | Network topology-aware routing for geo-distributed clusters |
| `service_mesh.cpp` | Istio/Envoy probe server (`THEMIS_ENABLE_SERVICE_MESH`) |
| `envoy_xds.cpp` | Envoy xDS v3 REST polling client (`THEMIS_ENABLE_SERVICE_MESH`) |
| `themis_wire_v1.proto` | Protobuf schema for wire protocol v1 message types |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                   ThemisDB Native Clients                        │
│   (CLI, SDK, inter-node RPCs)                                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │ TCP (binary wire protocol)
┌──────────────────────────▼──────────────────────────────────────┐
│                  WireProtocolServer                              │
│                                                                  │
│  ┌───────────────────┐  ┌──────────────────────────────────┐   │
│  │  I/O Thread Pool  │  │  TLS/mTLS (TLS 1.3)             │   │
│  │  (accept/read/    │  │  + Auth Middleware               │   │
│  │   write)          │  │  + Per-IP Rate Limiting          │   │
│  └────────┬──────────┘  └──────────────────────────────────┘   │
│           │                                                      │
│  ┌────────▼──────────────────────────────────────────────────┐  │
│  │  Frame Dispatcher (wire_protocol_helpers.cpp)             │  │
│  │  parse frame → route to handler                           │  │
│  └────────┬──────────────────────────────────────────────────┘  │
│           │                                                      │
│  ┌────────▼──────────────────────────────────────────────────┐  │
│  │  Worker Thread Pool (request processing)                  │  │
│  └───────────────────────────────────────────────────────────┘  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│  QoS Manager: traffic class → bandwidth quota + priority queue  │
│  SocketTimeoutManager: idle/request timeout + circuit breaker   │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Inbound Client Request

```
Client connects → TCP accept (I/O thread)
    │
    ├─ TLS handshake (TLS 1.3)
    ├─ Auth: Bearer token / mTLS client cert
    ├─ rate check: connections per IP, requests per second
    │
    ▼
Frame read: [4-byte length][1-byte type][payload]
    │
    ├─ QoS classification: tenant → priority class
    │
    ▼
Worker thread: deserialize Protobuf → route to src/server/ handler
    │
    ▼
Response serialized → frame write → client
```

### 4.2 Connection Pool (Outbound)

```
Replication / sharding: connect to peer node
    │
    ├─ wire_protocol_connection_pool.cpp:
    │       ├─ pool hit? → reuse existing connection
    │       └─ miss → new TCP connect + TLS + auth
    │
    ├─ SocketTimeoutManager: mark connection active
    │
    └─ release connection back to pool after use
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Delegates to** | `src/server/` | Request handlers after frame dispatch |
| **Uses** | `src/auth/` | Connection authentication |
| **Used by** | `src/replication/` | Inter-node wire protocol |
| **Used by** | `src/sharding/` | Cross-shard RPC |
| **Provides to** | Native SDK clients | Binary wire protocol endpoints |

---

## 6. Threading & Concurrency Model

- I/O thread pool (default: 4 threads) handles all socket I/O.
- Worker thread pool (default: 16 threads) handles request processing.
- Connection pool uses a per-peer mutex for checkout/checkin.
- `SocketTimeoutManager` runs a background timer thread.
- `QoSManager` uses lock-free priority queues per traffic class.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Separate I/O / worker pools | I/O never blocks on slow request processing |
| Connection reuse (pooling) | Avoids TCP handshake overhead for frequent peer connections |
| Frame size limits | `max_frame_size_mb = 64` prevents memory exhaustion |
| Zero-copy reads | Protobuf parsing directly from socket buffer |
| QoS priority queuing | High-priority requests (admin) bypass queue back-pressure |

---

## 8. Security Considerations

- TLS 1.3 required for all non-localhost connections (configurable).
- mTLS client certificates for service-to-service authentication.
- Per-IP connection limits prevent SYN-flood and connection exhaustion.
- Per-IP rate limits prevent request flooding.
- Frame size limit prevents memory exhaustion attacks.
- Auth timeout (`auth_timeout_sec = 10`) prevents slow-loris authentication attacks.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `network.wire.port` | 8766 | Wire protocol port |
| `network.wire.io_threads` | 4 | I/O thread pool size |
| `network.wire.worker_threads` | 16 | Worker thread pool size |
| `network.wire.max_connections` | 1000 | Max concurrent connections |
| `network.wire.max_connections_per_ip` | 10 | Per-IP connection limit |
| `network.wire.max_frame_size_mb` | 64 | Max frame payload size |
| `network.wire.connection_timeout_s` | 300 | Idle connection timeout |
| `network.wire.tls.enabled` | true | Require TLS |
| `network.wire.rps_per_ip` | 1000 | Max requests/sec per IP |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| TLS handshake failure | Close connection; log security event |
| Auth failure | Close connection after auth_timeout; log |
| Frame too large | Close connection; log violation |
| Rate limit exceeded | Reject with error frame; do not close connection |
| Idle timeout | `SocketTimeoutManager` closes gracefully |
| Peer connection failure | Circuit breaker in connection pool; retry with backoff |

---

## 11. Known Limitations & Future Work

- WebSocket binary frame dispatch is not yet implemented; clients must use text/JSON frames.
- DPDK kernel-bypass is not implemented; `io_uring` is guarded by `THEMIS_ENABLE_IO_URING`
  and off by default.
- IPv6 CIDR-based policies are not yet implemented in `ZeroTrustPolicyEnforcer`; IPv6
  clients are accepted but not subject to CIDR-level allow/deny rules.
- Integration tests combining TLS handshake + WebSocket upgrade are pending (NET-OPEN-02).
- `RaftLoadBalancer` simulates Raft consensus in-process; full distributed Raft over the
  network (multi-node leader election) is planned for a future milestone.

---

## 12. References

- `src/network/README.md` — module overview
- `src/network/themis_wire_v1.proto` — wire protocol schema
- `docs/WIRE_PROTOCOL_SPEC.md` — wire protocol specification
- `docs/architecture/wire_protocol_v1.md` — wire protocol architecture
- `ARCHITECTURE.md` (root) — full system architecture

# Network Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/network/`

---

## 1. Overview

The Network module implements ThemisDB's high-performance, secure networking layer for the
binary wire protocol. It provides the TCP server, connection pool, TLS/mTLS configuration,
socket timeout management, Quality-of-Service (QoS) management, and protocol buffer helpers.

The wire protocol is an alternative to HTTP/REST — it is a binary framing protocol
optimized for low-latency, high-throughput client connections. HTTP, gRPC, and WebSocket
are handled by the `api` module; this module focuses on the custom binary protocol.

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
| `wire_protocol_server.cpp` | Core TCP server: accept, auth, frame dispatch, rate limiting |
| `wire_protocol_connection_pool.cpp` | Client-side connection pool for outbound connections |
| `wire_protocol_helpers.cpp` | Frame serialization/deserialization, message types |
| `wire_protocol_performance.cpp` | Performance monitoring for the wire protocol |
| `wire_protocol_server_ws.cpp` | WebSocket transport layer for the wire protocol |
| `wire_protocol_v2.cpp` | Wire protocol v2 implementation (in progress) |
| `qos_manager.cpp` | QoS: traffic classification, bandwidth quotas, priority queuing |
| `socket_timeout_manager.cpp` | Socket timeout enforcement, circuit breaker |
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

- Wire protocol v2 (`wire_protocol_v2.cpp`) is in progress; v1 is current production.
- UDP-based protocols are not planned.
- Service mesh integration (Envoy, Istio) is planned but not implemented.
- WebSocket transport (`wire_protocol_server_ws.cpp`) for browser clients is experimental.

---

## 12. References

- `src/network/README.md` — module overview
- `src/network/themis_wire_v1.proto` — wire protocol schema
- `docs/WIRE_PROTOCOL_SPEC.md` — wire protocol specification
- `docs/architecture/wire_protocol_v1.md` — wire protocol architecture
- `ARCHITECTURE.md` (root) — full system architecture

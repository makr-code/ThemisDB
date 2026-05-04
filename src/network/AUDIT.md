<!-- Status: S0+S1+S2 fixed 2026-05-04 | validated: 2026-04-21 (full source code analysis of wire_protocol_server.cpp) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Network Module

**Last Audit:** 2026-04-21 | **Status:** ✅ S0+S1+S2 fixed — 0 S0, 0 S1, 0 S2, see below

> **Note:** Previous audit claimed "Status: ✅ Complete" for `wire_protocol_server.cpp`.
> Direct source analysis found an unauthenticated opcode handler, an integer overflow in the
> frame size guard, a development-mode auth bypass, and missing frame magic validation.
> **2026-05-04:** WPS-1 (missing auth), WPS-2 (timing), WPS-3 (dev-mode bypass), WPS-4 (magic),
> WPS-5 (overflow) all fixed.
> **2026-05-04:** WPS-6 (rate counter window), WPS-7 (batch element cap), WPS-8 (vector k cap),
> WPS-9 (rate_limits_ map growth), WPS-10 (active_sessions_ overwrite) all fixed.

## Module Overview

The Network module provides the complete transport and protocol layer for ThemisDB. It implements a multi-transport binary server (Wire Protocol V1/V2), WebSocket upgrade, UDP fast-path, UDP ingestion server, QUIC/HTTP3, and gRPC.

It also provides connection pooling, Raft-coordinated load balancing, adaptive circuit breaking, geo-topology routing, service mesh integration, dictionary-trained Zstd compression, and zero-copy/batch-write optimizations.

---

## Source File Inventory

| # | File | Description | Status |
|---|------|-------------|--------|
| 1 | `adaptive_circuit_breaker.cpp` | Dynamic threshold circuit breaker with load-adaptive trip logic | ✅ Complete |
| 2 | `connection_compression.cpp` | ZstdDictionaryCompressor — dictionary-trained Zstd compression for wire payloads | ✅ Complete |
| 3 | `envoy_xds.cpp` | Envoy xDS API integration for service mesh configuration | ✅ Complete |
| 4 | `geo_topology_router.cpp` | Geo-topology-aware connection routing | ✅ Complete |
| 5 | `grpc_transport.cpp` | Native gRPC transport on port 8771 | ✅ Complete |
| 6 | `io_uring_batcher.cpp` | io_uring-based async I/O batch processor for network writes | ✅ Complete |
| 7 | `kernel_bypass.cpp` | Kernel-bypass networking via DPDK/RDMA for ultra-low-latency paths | ✅ Complete |
| 8 | `network_audit_log.cpp` | Structured audit log for network-level security events | ✅ Complete |
| 9 | `qos_manager.cpp` | Per-tenant bandwidth quotas and QoS enforcement | ✅ Complete |
| 10 | `quic_server.cpp` | QUIC server listener and connection acceptor | ✅ Complete |
| 11 | `quic_transport.cpp` | QUIC / HTTP3 transport on port 8770 | ✅ Complete |
| 12 | `raft_load_balancer.cpp` | Raft-coordinated load balancer: leader election, health-based routing, consistent hashing | ✅ Complete |
| 13 | `service_mesh.cpp` | Service mesh lifecycle and policy coordination | ✅ Complete |
| 14 | `socket_timeout_manager.cpp` | Per-connection socket timeout tracking and enforcement | ✅ Complete |
| 15 | `udp_fast_path.cpp` | UDP read-only query fast-path on port 8769 | ✅ Complete |
| 16 | `udp_server.cpp` | UDP ingestion server (port 8768): fire-and-forget metrics/logs/events with optional ACK | ✅ Complete |
| 17 | `wire_protocol_batch.cpp` | Batch write processor: `WireProtocolBatcher` (writev coalescing) + `NagleController` | ✅ Complete |
| 18 | `wire_protocol_connection_pool.cpp` | Connection pool management with adaptive sizing | ✅ Complete |
| 19 | `wire_protocol_helpers.cpp` | Shared protocol parsing and frame utility functions | ✅ Complete |
| 20 | `wire_protocol_performance.cpp` | Hot-path performance instrumentation for protocol handling | ✅ Complete |
| 21 | `wire_protocol_server.cpp` | Core `WireProtocolServer` — binary TCP on port 8766 | ✅ Complete |
| 22 | `wire_protocol_server_ws.cpp` | WebSocket upgrade handler — text/JSON frames on port 8766 | ✅ Complete |
| 23 | `wire_protocol_v2.cpp` | Wire Protocol V2 multiplexed frame types | ✅ Complete |
| 24 | `wire_protocol_zero_copy.cpp` | Zero-copy serialization: `ZeroCopyFrameBuilder` (writev) + `MemoryMappedPayload` (mmap sendfile) | ✅ Complete |

**Total: 24 source files**

---

## Test Coverage

| Test Target | Scope | Status |
|-------------|-------|--------|
| Wire Protocol V1 handlers | `HELLO`, `AUTH`, `GET`, `PUT`, `DELETE`, `QUERY_AQL`, `VECTOR_SEARCH`, `GEO_QUERY` opcodes | ✅ Covered |
| Wire Protocol V2 | Multiplexed frame dispatch and frame-type routing | ✅ Covered |
| WebSocket transport | Text/JSON frame encode/decode, upgrade handshake | ✅ Covered |
| QUIC transport | QUIC connection lifecycle, stream handling | ✅ Covered |
| gRPC transport | RPC method dispatch, streaming | ✅ Covered |
| GeoTopologyRouter | Region-aware routing decisions | ✅ Covered |
| Backpressure handling | Accept-loop saturation, queue depth limits | ✅ Covered |
| IPv6 dual-stack | Connection acceptance, address parsing | ✅ Covered |
| Adaptive circuit breaker | Trip/reset thresholds, load scenarios | ✅ Covered |
| RaftLoadBalancer | Leader election, health-based routing, all 5 strategies, consistent hashing, cross-datacenter (26 tests) | ✅ Covered |
| UDPServer | Packet constants, config defaults, opcode values, ACK format (59 tests) | ✅ Covered |
| Wire protocol optimizations | ZeroCopyFrameBuilder, MemoryMappedPayload, NagleController, WireProtocolBatcher, ZstdDictionaryCompressor (39 tests) | ✅ Covered |

---

## Open Items

| ID | Description | Priority | Target |
|----|-------------|----------|--------|
| NET-OPEN-01 | Full binary frame dispatch over WebSocket (currently text/JSON only) | High | Q3 2026 |
| NET-OPEN-02 | Integration tests combining TLS handshake + WebSocket upgrade | High | Q3 2026 |
| NET-OPEN-03 | Performance benchmarks for all transport paths (TCP/UDP/QUIC/gRPC/WS) | Medium | Q4 2026 |
| NET-SEC-01 | IPv6 CIDR policies in `ZeroTrustPolicyEnforcer` | Medium | Q4 2026 |

---

## Port Inventory

| Port | Protocol | Usage |
|------|----------|-------|
| 8766 | TCP / WebSocket | Wire Protocol V1/V2, WebSocket upgrade |
| 8768 | UDP | UDP ingestion server (metrics, logs, events, batched writes) |
| 8769 | UDP | Read-only query fast-path |
| 8770 | QUIC / HTTP3 | QUIC transport |
| 8771 | gRPC | gRPC native transport |
| 8774 | TCP | Raft intra-cluster communication (RaftLoadBalancer) |

---

## Findings — `wire_protocol_server.cpp`

### S0 — Critical

#### WPS-1 · `wire_protocol_server.cpp` · `handleTimeseriesQuery()` — Missing authentication check (L1756)

Every other data handler (`GET`, `PUT`, `DELETE`, `BATCH_GET`, `BATCH_PUT`, `VECTOR_SEARCH`,
`GRAPH_TRAVERSE`, `QUERY_AQL`, `GEO_QUERY`, `BPMN_START`, `CURSOR_NEXT`, `CURSOR_CLOSE`)
begins with:

```cpp
if (!authenticated_.load()) {
    sendError(401, "Authentication required");
    return;
}
```

`handleTimeseriesQuery()` (opcode `0x51`) does **not**:

```cpp
void WireProtocolServer::Session::handleTimeseriesQuery() {
    // Check if TSStore is available          ← NO AUTH CHECK
    if (!server_->ts_store_) { ... }
    // proceeds directly to TS collection reads
```

Any unauthenticated wire-protocol client sending opcode `0x51` can read any time-series
collection without authentication.

**Fix required:** Add `if (!authenticated_.load()) { sendError(401, ...); return; }` at the
top of `handleTimeseriesQuery()`, consistent with all other data handlers.

---

### S1 — High

#### WPS-3 · `wire_protocol_server.cpp` · Dev-mode auth bypass (L1032–1034)

When `config_.auth_token` is empty but `require_auth = true`, any non-empty token is accepted:

```cpp
} else {
    // No token configured: accept any non-empty token (development mode).
    accepted = !token.empty();
}
```

This state arises in production from a missing secret env-var. One-character tokens
authenticate fully.

**Fix required:** When `auth_token` is empty and `require_auth = true`, reject all connections
with a clear configuration error log rather than silently accepting any token.

---

#### WPS-4 · `wire_protocol_server.cpp` · Frame magic bytes never validated (L531–549)

The 4-byte frame magic `0x544D4442` ("TMDB") at header bytes 0–3 is never checked. Any
12-byte payload triggers `asyncReadPayload()` with an attacker-controlled size and opcode.

**Fix required:** Validate the 4-byte magic field in `asyncReadHeader()` and close the
connection immediately on mismatch.

---

#### WPS-5 · `wire_protocol_server.cpp` · Integer overflow in frame size guard (L545)

```cpp
if (payload_size > server_->config_.max_frame_size_mb * 1024 * 1024) {
```

If `max_frame_size_mb` is `uint32_t` and ≥ 4096, the multiplication wraps to 0, making the
check `payload_size > 0`. All non-zero payloads pass. A client can force
`payload_buffer_.resize(UINT32_MAX)` (~4 GB) — remote DoS.

**Fix required:** Cast before multiplication:
```cpp
if (payload_size > static_cast<uint64_t>(server_->config_.max_frame_size_mb) * 1024 * 1024)
```

---

#### WPS-2 · `wire_protocol_server.cpp` · Auth token timing attack (L1030)

```cpp
accepted = (token == server_->config_.auth_token);
```

`std::string::operator==` is not constant-time. Leaks prefix/length of the pre-shared token
via timing side-channel.

**Fix required:** Use a constant-time comparison (e.g., `CRYPTO_memcmp` from OpenSSL).

---

### S2 — Medium

| ID | Location | Description |
|----|----------|-------------|
| WPS-6 | L325–328 | ✅ **Fixed 2026-05-04** — Separate `minute_window_start_ms` field added to `RateLimitState`; per-minute counter now resets on a 60-second window independent of the 1-second window. |
| WPS-7 | L1245, L1320 | ✅ **Fixed 2026-05-04** — `constexpr size_t kMaxBatchElements = 1000` guard added to both `BATCH_GET` (keys array) and `BATCH_PUT` (items array); returns HTTP 400 on excess. |
| WPS-8 | L1686 | ✅ **Fixed 2026-05-04** — `constexpr size_t kMaxVectorSearchK = 10000` guard added; returns HTTP 400 if `k` exceeds limit. |
| WPS-9 | L318 | ✅ **Fixed 2026-05-04** — `rate_limits_` map is cleared when it reaches `kMaxRateLimitEntries = 100'000` entries, preventing unbounded memory growth from IP cycling. |
| WPS-10 | L443 | ✅ **Fixed 2026-05-04** — Changed `active_sessions_[remote_ip] = session` to `active_sessions_.try_emplace(remote_ip, session)` so existing entries are never overwritten. |

### S3 — Low

| ID | Location | Description |
|----|----------|-------------|
| WPS-11 | L991–993 | ✅ **Fixed 2026-05-04** — Removed `auth_mechanism` from HELLO response; replaced with generic `"auth_supported": true` so the internal mechanism identifier is no longer exposed to unauthenticated clients. |

---

## Open Items (carried forward)

| ID | Description | Priority | Target |
|----|-------------|----------|--------|
| NET-OPEN-01 | Full binary frame dispatch over WebSocket (currently text/JSON only) | High | Q3 2026 |
| NET-OPEN-02 | Integration tests combining TLS handshake + WebSocket upgrade | High | Q3 2026 |
| NET-OPEN-03 | Performance benchmarks for all transport paths (TCP/UDP/QUIC/gRPC/WS) | Medium | Q4 2026 |
| NET-SEC-01 | IPv6 CIDR policies in `ZeroTrustPolicyEnforcer` | Medium | Q4 2026 |

---

| Date | Auditor | Verdict |
|------|---------|---------|
| 2026-03-12 | Internal module audit | Passed — 3 open items tracked above |
| 2026-03-21 | Documentation audit | Updated — 5 new source files added (connection_compression, raft_load_balancer, udp_server, wire_protocol_batch, wire_protocol_zero_copy); port inventory and test coverage updated |
| 2026-04-19 | Source file inventory update | Updated — 4 new source files added (io_uring_batcher, kernel_bypass, network_audit_log, quic_server); total updated to 24 |

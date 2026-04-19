# Network Module Production Readiness Assessment & Roadmap

**Status:** Not Production Ready  
**Version:** 1.6.0  
**Last Updated:** April 2026

---

## Executive Summary

The ThemisDB Network module provides a solid TCP wire protocol server with TLS/mTLS, connection pooling, circuit breaker, and timeout protection. However, **it is not yet production ready** for high-scale deployments. Several critical features—QoS/rate shaping, WebSocket, UDP, backpressure, audit logging, and OTel instrumentation—are either documented only or absent. This roadmap describes current gaps and a phased path to full production readiness.

---

## Current Assessment

### Production Readiness: ❌ Not Ready

### Implemented (solid foundation)

- TCP wire protocol server with TLS/mTLS support
- Per-IP connection limits and request rate limiting
- Frame size validation (`max_frame_size_mb`)
- Circuit breaker (`SocketTimeoutManager`) with alert callbacks
- Connection pooling with keep-alive and stale pruning
- SCRAM-SHA-256 authentication
- Graceful shutdown and timeout management

### Critical Gaps

#### Protocol & Transport

- **WebSocket**: API/roadmap only; no productive implementation
- **UDP**: API/roadmap only; no productive implementation
- **QUIC / HTTP/3**: Roadmap only; no implementation
- **Multicast, DPDK, io_uring, RDMA, eBPF**: Roadmap/docs only
- **TCP backpressure**: Slow-client / write-queue depth not enforced; no backpressure signal to producers

#### QoS & Traffic Shaping

- **Token Bucket / Leaky Bucket**: No per-connection bandwidth shaping
- **Priority Queues**: No CRITICAL / HIGH / MEDIUM / LOW traffic classes
- **Fair Queuing**: No starvation prevention between clients
- **Runtime quota / governance API**: No admin endpoint to adjust limits live

#### Observability

- **OpenTelemetry (OTel)**: No distributed tracing hooks on protocol paths
- **Prometheus metrics export**: No `/metrics` endpoint for the wire protocol server
- **Structured audit log**: No per-session/per-user audit trail

#### Security & Compliance

- **Dynamic ACL / policy layer**: No runtime update of connection allow/deny rules
- **mTLS enforcement at runtime**: Policy cannot be toggled without restart
- **Admin / Audit API**: No suspend/throttle/ban/audit endpoints

#### Testing

- **Fuzz tests for wire frames**: Partial; no oversize/invalid-frame fuzz coverage via real server
- **DoS / Slowloris tests**: No integration tests exercising connection-flood or slow-read attacks
- **Chaos / replay tests**: Not present for the network layer
- **Circuit-breaker tests**: No tests for CIRCUIT_OPEN → recovery path
- **Pool-overflow tests**: No tests for acquire-timeout under pool exhaustion

---

## Roadmap

### Q1 2026 – QoS, WebSocket, OTel Metrics, Admin API

**Goal:** Elevate stability and observability to production baseline

**Deliverables:**

1. **Token Bucket / Priority QoS** *(implemented in this update)*
   - `include/network/qos_manager.h` + `src/network/qos_manager.cpp`
   - Per-connection token bucket rate limiting (bps)
   - CRITICAL / HIGH / MEDIUM / LOW priority queues
   - Fair-queuing and backpressure signal
   - Unit tests in `tests/test_qos_manager.cpp`

2. **WebSocket Go-Live**
   - Implement `WebSocketServer` using Boost.Beast
   - Reuse auth / session logic from wire protocol server
   - Per-message deflate compression; binary (Protobuf) + text (JSON) frames
   - Backpressure for slow WebSocket clients (write-queue depth limit)
   - Integration tests and CI coverage

3. **OTel / Prometheus Metrics**
   - Instrument every protocol path (accept, read, write, dispatch) with spans
   - Export counters / histograms: `wire_requests_total`, `wire_request_duration_seconds`, `wire_active_connections`
   - `/metrics` endpoint on admin port (Prometheus scrape target)
   - Alert rules for error rate > 1 %, p99 latency > 500 ms, circuit-open events

4. **Admin API for Limits & Stats**
   - `GET /admin/network/stats` – connection, request, error counts
   - `POST /admin/network/connections/{id}/throttle` – apply token-bucket to live connection
   - `POST /admin/network/connections/{id}/ban` – close & block IP
   - `GET /admin/network/audit` – last N audit events (auth, request, close)

---

### Q2 2026 – UDP Go-Live, QUIC/HTTP3, Hot-Reload TLS, Service-Mesh / Audit

**Goal:** Expand transport coverage and zero-downtime operations

**Deliverables:**

1. **UDP Server Go-Live**
   - Boost.Asio UDP socket; magic-byte + version + opcode packet format
   - Rate limiting per source IP (token bucket)
   - Optional application-level ACKs (sequence numbers)
   - Metrics: `udp_packets_received_total`, `udp_packets_dropped_total`

2. **QUIC / HTTP/3**
   - Integrate `quiche` (Cloudflare) QUIC library
   - 0-RTT for resumed connections; stream multiplexing
   - HTTP/3 mapping for existing REST API compatibility

3. **Hot-Reload TLS & Config**
   - SIGHUP-triggered certificate rotation without connection disruption
   - Config watch (inotify / kqueue) for runtime limit updates
   - Graceful draining: accept new cert before closing old sessions

4. **Service-Mesh & Audit**
   - `/healthz` and `/readyz` endpoints for Kubernetes probes
   - W3C TraceContext propagation in wire protocol headers
   - Per-session audit log (JSON Lines): auth events, query types, errors
   - mTLS policy toggleable via admin API at runtime

---

### Q3 2026 – DPDK / io_uring / Multicast, Load-Balancer/Raft, Fuzz/Chaos/Replay

**Goal:** High-throughput kernel-bypass and resilience validation

**Deliverables:**

1. **io_uring Integration**
   - Linux io_uring backend for async I/O (replaces epoll path)
   - SQ polling mode for sub-10 µs wake latency
   - Zero-copy send path (registered buffers)

2. **DPDK Kernel Bypass**
   - Optional DPDK backend for 10 G / 100 G NICs
   - NUMA-aware buffer pools, CPU pinning
   - Fallback to standard Asio when DPDK not available

3. **IP Multicast**
   - One-to-many result streaming (pub/sub topics)
   - SSM (Source-Specific Multicast) for security
   - Integration with continuous aggregate notifications

4. **Raft Load Balancer**
   - Leader-elected routing decisions
   - Health-based backend weighting; consistent-hash sticky routing
   - Automatic failover and re-admission

5. **Fuzz / Chaos / Replay Test Suite**
   - LibFuzzer targets for wire-frame parser
   - Slowloris / connection-flood integration tests
   - Replay tests: record production traffic, replay under CI
   - Circuit-breaker recovery and pool-overflow tests

---

### Q4 2026 – RDMA, eBPF, Operator Dashboard, ML-Based QoS

**Goal:** Ultra-low-latency specialised transports and intelligent operations

**Deliverables:**

1. **RDMA Support**
   - InfiniBand / RoCE backend using `libibverbs`
   - One-sided RDMA READ/WRITE for storage access
   - Transparent fallback to TCP when RDMA unavailable

2. **eBPF Networking**
   - XDP program for fast-path packet classification
   - eBPF-based rate limiting (without user-space overhead)
   - Connection tracking in BPF maps

3. **Operator Dashboard**
   - Real-time connection graph, QoS buckets, circuit states
   - Integration with Grafana (pre-built dashboards in `grafana/`)
   - Anomaly alerts (ML-based traffic baseline)

4. **ML-Based QoS / Audit**
   - Predictive load balancing (LSTM traffic model)
   - Anomaly detection for DDoS / abuse patterns
   - Adaptive timeout calibration

---

## Success Criteria

The Network module will be considered **production ready** when:

1. ✅ QoS token bucket & priority queues deployed and tested
2. ✅ WebSocket server in production with TLS + backpressure
3. ✅ OTel tracing and Prometheus metrics on all protocol paths
4. ✅ Admin API for live throttle / ban / audit
5. ✅ UDP server in production with rate limiting
6. ✅ Fuzz tests run cleanly for 24 + hours without crashes
7. ✅ Slowloris / DoS integration tests pass in CI
8. ✅ Circuit-breaker tests cover OPEN → HALF-OPEN → CLOSED path
9. ✅ Hot-reload TLS without dropping live connections
10. ✅ Load test: 50 k concurrent connections, p99 < 10 ms under QoS shaping

---

## Risk Mitigation

- **Token bucket CPU overhead**: Pre-compute token refill on a background thread; avoid mutex on hot path (use atomics)
- **WebSocket memory pressure**: Hard cap write-queue bytes per connection; drop connection if backpressure limit exceeded
- **QUIC complexity**: Gate behind `THEMIS_ENABLE_QUIC` CMake option; keep TCP path as default
- **DPDK portability**: Wrap in `#ifdef THEMIS_ENABLE_DPDK` and provide stub fallback
- **eBPF kernel version dependency**: Require Linux ≥ 5.10; document clearly; skip on unsupported kernels

---

## References

- [Network Module README](../src/network/README.md)
- [Network Module Future Enhancements (src)](../src/network/FUTURE_ENHANCEMENTS.md)
- [Network Module Future Enhancements (include)](../include/network/FUTURE_ENHANCEMENTS.md)
- [Socket Timeout Manager](../include/network/socket_timeout_manager.h)
- [Wire Protocol Connection Pool](../include/network/wire_protocol_connection_pool.h)
- [Wire Protocol Server](../include/network/wire_protocol_server.h)
- [QoS Manager](../include/network/qos_manager.h)
- [Network Timeout Handling](../../ARCHIVED/implementation-summaries/NETWORK_TIMEOUT_HANDLING.md)

---

## Changelog

- **2026-02-20**: Initial production readiness assessment and Q1–Q4 roadmap created

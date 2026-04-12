<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Network Module Roadmap

## Current Status

v1.8.0 — production. All core transports (TCP, UDP, QUIC, gRPC, WebSocket) and zero-copy batching shipped. Raft load balancer and Envoy xDS integration active.

## Completed

- [x] Wire protocol framing, helpers, performance counters
- [x] Connection pool with health-check eviction
- [x] gRPC, QUIC, WebSocket transports
- [x] AdaptiveCircuitBreaker, GeoTopologyRouter, QosManager
- [x] Envoy xDS v3 service mesh
- [x] ZstdDictionaryCompressor shared-dict compression
- [x] WireProtocolBatcher + NagleController
- [x] ZeroCopyFrameBuilder + MemoryMappedPayload
- [x] UDPServer (port 8768) + UdpFastPath (io_uring)
- [x] RaftLoadBalancer (port 8774)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] Frame format specification (header, CRC32C, length prefix)
- [x] Transport interface abstractions (`ITransport`, `IConnectionPool`)

### Phase 2 — Core Implementation ✅
- [x] WireProtocolServer epoll/kqueue backend
- [x] Connection pool multiplexing
- [x] gRPC and QUIC adapters

### Phase 3 — Resilience & Advanced Routing ✅
- [x] AdaptiveCircuitBreaker sliding-window
- [x] GeoTopologyRouter zone scoring
- [x] RaftLoadBalancer quorum routing

### Phase 4 — Zero-Copy & Compression ✅
- [x] ZeroCopyFrameBuilder sendfile/mmap
- [x] ZstdDictionaryCompressor
- [x] NagleController flush tuning

### Phase 5 — Performance / Hardening (Planned)
- [ ] DPDK kernel-bypass data plane (Target: Q3 2026)
- [x] io_uring batched send on Linux 6.x (Target: Q3 2026)
  - `IoUringBatchedSender` in `include/network/io_uring_batcher.h` + `src/network/io_uring_batcher.cpp`
  - Single `io_uring_enter()` syscall for N concurrent `WireProtocolBatcher` flushes
  - `IORING_OP_WRITEV` SQEs; CQE reap + per-op error reporting
  - `THEMIS_ENABLE_IO_URING` guard; transparent `writev(2)` fallback otherwise
  - 12 focused tests in `tests/test_io_uring_batcher.cpp` (IUB-01…IUB-12)
- [ ] Persistent QUIC sessions across restarts (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All public headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] All transports tested under 1 Gbps synthetic load
- [x] Circuit breaker validated with chaos injection
- [x] TLS 1.3 enforced on gRPC and service mesh paths
- [ ] DPDK integration validated on bare-metal (Target: Q3 2026)

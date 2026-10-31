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
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/network/ROADMAP.md -->

# Network Module — Public Header Roadmap

**Module Path:** `include/network/`
**Canonical implementation roadmap:** [`../../src/network/ROADMAP.md`](../../src/network/ROADMAP.md)

---

## Overview

Tracks public network API contract stability, planned header additions, and breaking changes. Feature items affecting both implementation and headers are tracked in:

→ [`../../src/network/ROADMAP.md`](../../src/network/ROADMAP.md)

---

## Current Status

The current public surface contains 31 headers; optional transport headers do not imply that their build features are enabled. `AdaptiveCircuitBreaker` supports per-error-class thresholds, and `ZeroCopyFrameBuilder` exposes `writeToWithSendfile()`. The service-mesh-gated `EnvoyXdsClient` uses REST polling; see `src/network/ARCHITECTURE.md` for its current lifecycle and constraints.

---

## Completed ✅

- [x] `wire_protocol_server.h` / `wire_protocol_connection_pool.h` / `wire_protocol_batch.h` — wire protocol stack
- [x] `wire_protocol_zero_copy.h` / `wire_protocol_websocket.h` — zero-copy and WebSocket framing
- [x] `quic_server.h` / `quic_transport.h` — QUIC/HTTP3 transport
- [x] `grpc_transport.h` — gRPC channel binding
- [x] `io_uring_batcher.h` — io_uring async I/O batching
- [x] `kernel_bypass.h` — DPDK/RDMA kernel bypass (conditional)
- [x] `udp_server.h` / `udp_fast_path.h` — UDP fast-path with GSO/GRO
- [x] `adaptive_circuit_breaker.h` — adaptive circuit breaker with state machine
- [x] `adaptive_io_scaler.h` — dynamic I/O concurrency scaling
- [x] `qos_manager.h` — per-connection QoS classification
- [x] `connection_compression.h` — zstd/lz4 on-wire compression
- [x] `socket_timeout_manager.h` — per-socket read/write timeout
- [x] `geo_topology_router.h` — geo-aware latency routing
- [x] `service_mesh.h` / `envoy_xds.h` / `raft_load_balancer.h` — service mesh integration
- [x] `network_audit_log.h` / `wire_bootstrap_validation.h` — auxiliary

---

## In Progress

- [x] Extend `AdaptiveCircuitBreaker` with per-error-class thresholds (Target: 2026-Q3) — NCB-PEC-01..08 in tests/network/test_network_cb_per_error_class_focused.cpp
- [x] Document `EnvoyXDSClient` xDS resource subscription lifecycle in ARCHITECTURE (Target: 2026-Q3)

---

## Added API surface and remaining work

- [x] `multipath_tcp.h` — `MptcpSubflowManager` subflow-management API (Target: 2026-Q3)
- [x] `bbr_congestion_control.h` — `BbrConfig` and `BbrCongestionController` API (Target: 2026-Q4)
- [x] `network_observability.h` — `INetworkObservabilitySink` and `NetworkObservabilityRegistry` APIs (Target: 2026-Q4)
- [ ] Stabilize `kernel_bypass.h` as unconditional when DPDK is vendored (Target: 2027-Q1)

---

## Breaking Change History

None in v1.x. Wire protocol framing types are stable. `KernelBypass` API is conditionally compiled and not covered by the stable ABI guarantee. Any breaking change requires a MAJOR version bump.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/network/FUTURE_ENHANCEMENTS.md -->

# Network Module — Public Header Future Enhancements

**Module Path:** `include/network/`
**Canonical implementation enhancements:** [`../../src/network/FUTURE_ENHANCEMENTS.md`](../../src/network/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/network/`. Implementation-level enhancements are in:

→ [`../../src/network/FUTURE_ENHANCEMENTS.md`](../../src/network/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` `AdaptiveCircuitBreaker` state transitions must be thread-safe; no caller-visible lock exposure.
- `[x]` `KernelBypass` header must compile without DPDK headers when `THEMIS_ENABLE_KERNEL_BYPASS` is not defined.
- `[x]` `IoUringBatcher` must not expose `io_uring_sqe*` in public API; callers submit via typed request structs.
- `[x]` `QUICServer` and `WireProtocolServer` must share no state; each owns its accept loop.
- `[x]` `GeoTopologyRouter` decisions must be observable via `NetworkAuditLog` for compliance.
- `[x]` All compression configuration is in `ConnectionCompression`; per-message override is not permitted.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `WireProtocolServer::listen()` | `wire_protocol_server.h` | HttpServer bootstrap | ✅ Stable |
| `AdaptiveCircuitBreaker::allow()` | `adaptive_circuit_breaker.h` | RPC dispatch, retry logic | ✅ Stable |
| `QUICServer::accept()` | `quic_server.h` | HTTP3Session | ✅ Stable |
| `IoUringBatcher::submit()` | `io_uring_batcher.h` | Storage I/O path | ✅ Stable |
| `GeoTopologyRouter::route()` | `geo_topology_router.h` | DistributedGateway | ✅ Stable |
| `QoSManager::classify()` | `qos_manager.h` | Auth middleware | ✅ Stable |
| `EnvoyXDSClient::subscribe()` | `envoy_xds.h` | ServiceMesh | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `multipath_tcp.h` — `IMPTCPManager` interface for MPTCP subflow creation, scheduling, and teardown; enables bandwidth aggregation across multiple paths.
- Per-error-class thresholds in `AdaptiveCircuitBreaker`: `open(errorClass)` / `halfOpen(errorClass)` transitions for granular fault isolation.
- Document `EnvoyXDSClient` xDS resource subscription lifecycle in ARCHITECTURE.

### Medium-Term (Q4 2026)

- `bbr_congestion_control.h` — `BBRv2Config` struct and `ICongestionController` interface for pluggable BBRv2 integration.
- `network_observability.h` — `INetworkMetricsSink` interface for per-connection trace/span/metric emission; decouples transport from Prometheus/OTEL.
- `WireProtocolZeroCopy` extension: `sendScatterGather(iov[], n)` with registered buffer support for io_uring zero-copy sends.

### Long-Term

- Unconditional `kernel_bypass.h` once DPDK is vendored; removes `THEMIS_ENABLE_KERNEL_BYPASS` guard.
- RDMA-CM based replication transport: `RDMAReplicationTransport` for sub-100µs cross-shard log shipping.
- P4-programmable dataplane integration: `P4DataplaneConfig` for offloading QoS classification to SmartNICs.

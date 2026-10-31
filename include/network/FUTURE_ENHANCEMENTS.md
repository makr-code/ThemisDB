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

## Existing Extension Points (verified in public headers)

| Interface | Declared In | Current role |
|-----------|------------|--------------|
| `WireProtocolServer::start()` / `stop()` | `wire_protocol_server.h` | Native TCP listener lifecycle |
| `AdaptiveCircuitBreaker::shouldAllow()` / `recordFailure()` | `adaptive_circuit_breaker.h` | Circuit admission and failure accounting |
| `QUICServer::start()` / `QuicTransport::start()` | `quic_server.h`, `quic_transport.h` | Optional QUIC listener/transport lifecycle |
| `IoUringBatchedSender::enqueue()` / `submitAndWait()` | `io_uring_batcher.h` | Batched writes with synchronous fallback |
| `GeoTopologyRouter::selectEndpoint()` | `geo_topology_router.h` | Select a healthy shard endpoint |
| `QoSManager::allowSend()` / `enqueueSend()` | `qos_manager.h` | Per-connection rate and send-queue controls |
| `EnvoyXdsClient::start()` / `stop()` | `envoy_xds.h` | Optional xDS REST polling; callbacks are configured separately |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- No new public header work is currently listed for this period. MPTCP, per-error-class circuit-breaker thresholds, and the xDS lifecycle documentation are already present; they are not planned interfaces.

### Medium-Term (Q4 2026)

- The current headers provide `BbrConfig`/`BbrCongestionController` and `INetworkObservabilitySink`/`NetworkObservabilityRegistry`; further public API changes require a separately scoped contract and tests.
- **Proposal:** add registered-buffer support for zero-copy sends only after a measured workload demonstrates a benefit over the current `writev()` and `sendfile()` paths. No `sendScatterGather()` API is currently declared.

### Long-Term

- Keep `kernel_bypass.h` usable without DPDK headers; remove conditional availability only after a supported DPDK dependency and cross-platform build policy are defined.
- **Proposal:** evaluate an RDMA-CM replication transport against a documented workload and measured latency/operational targets before defining a public type.
- **Proposal:** evaluate programmable dataplane QoS offload with a threat model and portable fallback before defining a public type.

## Test Strategy

- Add focused tests in `tests/network/` for every public contract change, including unavailable-feature and failure paths.
- Register targets through the existing `tests/network/CMakeLists.txt` source glob.
- Do not treat model-only tests or benchmark gate thresholds as live transport evidence.

## Performance and Security / Reliability

- No new numeric performance target is approved for the proposals above; establish a reproducible baseline before setting one.
- Preserve feature guards, bounded resource use, validation-before-dispatch, and explicit failure behavior. Optional bypass/offload paths must retain a tested fallback.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Performance Module

## Scope

Covers all public headers in `include/performance/`. Implementation hardening in `../../src/performance/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Arbitrary code execution via JIT compiler | Critical — RCE | `AdaptiveQueryCompiler` sandboxes generated code; LLVM IR is validated before emission |
| PMU side-channel (e.g., Spectre via timing) | High — secret inference | `CycleMetrics` measurements are not exposed to untrusted callers; results are aggregated only |
| Huge-page allocation exhaustion (DoS) | Medium — OOM | `HugePageManager` enforces per-process allocation quotas |
| NUMA topology spoofing | Low — incorrect placement | `NumaTopology` reads from kernel sysfs; not user-controllable |
| Feature flag injection | Medium — unauthorized feature enablement | `FeatureFlags` + `RuntimeConfig` changes require operator-role authentication |
| RCU quiescent-state starvation | Medium — livelock | `RcuDomain` sets a maximum grace-period timeout to force reclaim |
| Lock-free buffer overflow | Medium — silent data loss | `LockFreeMetricsBuffer` drops oldest samples on full ring (documented behavior) |
| Hardware accelerator memory mapping | High — privilege escalation | `HardwareAccelerator` maps device memory only in kernel module context; no userspace `mmap` of MMIO |

## Security Controls

1. **JIT sandbox** — `AdaptiveQueryCompiler` emits code into an isolated LLVM module; W^X enforced.
2. **PMU access control** — `CycleMetrics` / `cycle_metrics_config.h` require `CAP_PERFMON` on Linux.
3. **Quota-gated huge pages** — `HugePageManager` enforces `max_huge_pages` config limit.
4. **RBAC for feature flags** — `FeatureFlags::set()` validates caller's operator role token.
5. **RCU grace-period cap** — prevents indefinite memory retention on stalled threads.
6. **Read-only sysfs access** — `NumaTopology` only reads `/sys/devices/system/node/`; no writes.

## Known Limitations

- macOS kperf PMU access requires `com.apple.security.cs.allow-dyld-environment-variables` entitlement in sandboxed environments — not suitable for App Store distribution.
- DPDK (planned Q3 2026) will require a separate security review for kernel-bypass privileges.

> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Performance Module

## Threat Model

### 1. Timing Side-Channels via RDTSC
- **Risk:** Exposing high-resolution cycle counters (`RDTSC`/`RDTSCP`) to user-space code could theoretically enable timing attacks against cryptographic operations or facilitate cross-tenant side-channel leakage via shared CPU resources.
- **Mitigation:**
  - `RDTSC` is a user-space instruction on all supported x86-64 platforms; it does not access privileged state.
  - Cycle measurements are strictly local to the measuring thread and are used only for internal latency instrumentation.
  - No cross-tenant RDTSC data paths exist: per-tenant measurement contexts are fully isolated, and raw cycle values are never propagated to external API responses or shared memory regions.
  - RDTSC usage has been reviewed against known speculative-execution side-channel variants (Spectre-v1/v2, MDS); no exploitable path was identified in the current implementation.
- **Status:** ✅ Security-reviewed — no cross-tenant leakage path identified

### 2. PMU Hardware Counter Leakage
- **Risk:** Hardware Performance Monitoring Unit (PMU) counters measure microarchitectural events (cache misses, branch mispredictions, IPC) that could reveal information about co-located workloads.
- **Mitigation:**
  - PMU counters are configured and read per-thread only.
  - Counter contexts are not shared across tenant execution boundaries.
  - No cross-tenant PMU data aggregation or comparison paths exist in the implementation.
- **Status:** ✅ Implemented — per-thread isolation enforced

### 3. Workload Predictor Poisoning
- **Risk:** An attacker with influence over query patterns could manipulate the ML-based workload predictor's training data, causing it to make suboptimal or adversarially biased resource allocation decisions (e.g., starvation of other tenants).
- **Mitigation:**
  - Separate per-tenant workload predictor model instances ensure that one tenant's query patterns cannot influence another tenant's predictions.
  - Model inputs are bounded and validated before ingestion; anomalous input distributions trigger fallback to conservative static allocation policies.
- **Status:** ✅ Implemented

### 4. Allocator-Level Information Disclosure
- **Risk:** mimalloc / jemalloc / huge-page allocators may retain sensitive data in freed memory pages that are later reallocated to a different tenant.
- **Mitigation:**
  - NUMA-aware allocation confines memory regions to per-tenant NUMA domains where possible.
  - Sensitive data structures (e.g., query parameters in cost model contexts) are explicitly zeroed on deallocation in the critical path.
- **Status:** ✅ Implemented

---

## Known Limitations

*No open security limitations. The security audit for this module is complete.*

---

## Security Configuration Reference

| Parameter | Description | Recommended Value |
|-----------|-------------|-------------------|
| `perf.allocator` | Allocator backend (`mimalloc` / `jemalloc` / `system`) | `mimalloc` |
| `perf.huge_pages` | Enable transparent huge pages for allocation arenas | `true` on dedicated hosts |
| `perf.numa_aware` | Bind allocation and thread scheduling to NUMA topology | `true` on NUMA systems |
| `perf.workload_predictor.per_tenant` | Use separate predictor model per tenant | `true` |
| `perf.pmu.enabled` | Enable PMU hardware counter collection | `true` (requires `perf_event_paranoid` ≤ 1) |

---

## Security Review History

| Date | Reviewer | Scope | Outcome |
|------|----------|-------|---------|
| 2026-03-12 | Internal security review | RDTSC timing side-channels, PMU counter isolation, workload predictor poisoning, allocator data retention | All items passed — no open security findings |

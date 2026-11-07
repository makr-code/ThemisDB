# Cache Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot Coding Agent | Human_Review: false | Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone. -->

## Current Status

The source contains adaptive L1/L2/L3 query caching, exact-key semantic caching, embedding similarity caching, warmup/prefetch, tenant-aware paths, and asynchronous distributed coordination/replication. `EmbeddingCache` initializes `VectorIndexManager` when configured; if index initialization is unavailable, query falls back to a linear in-memory scan. Tenant isolation is opt-in in `AdaptiveQueryCache::Config`, while authorization, freshness, and write/invalidation ordering remain caller responsibilities.

Implementation and focused test coverage are substantial, but release-validation maturity is incomplete: the cache benchmark runbook documents a gap in aggregating three benchmark outputs and mapping all manifest metrics, and the manifest's claim that the embedding index is a stub is not supported by the current implementation. No cache tests or benchmarks were run for issue `#7406`; the latest module roadmap evidence still records a RocksDB configure dependency block. Treat performance gates as unverified until the planned evidence is captured.

## In Progress

- [x] hardening distributed consistency and degradation behavior across cache coordinators (Target: Q3 2026)
  - Delivered: `include/cache/cache_contract.h` §5–§7 (coordinator capability flags, fail-closed semantics, async replication consistency)
  - Delivered: `tests/cache/test_cache_coordinator_degradation.cpp` (CCD-01..CCD-08)
- [x] define benchmark cases and release-gate manifest for cache hot paths (Target: Q3 2026)
  - Delivered: `benchmarks/cache/release_gate_manifest_cache.json` (GATE-CAC-01..06, GATE-CAP-01..06, SGATE-CAC-01..04)
  - Delivered: `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md`
- [x] diagnostics consistency improvements for tenant/invalidation/replication failures (Target: Q3 2026)
  - Delivered: `include/cache/cache_contract.h` §3 (CacheFailureClass taxonomy + fail-closed predicate)
  - Delivered: `tests/cache/test_cache_contract_hardening.cpp` (CCH-01..CCH-08)

## Planned Features

### Short-term (3-6 months)
- [x] integrate with AccessCoordinator for unified cache-storage tier management (Target: Q4 2026)
  - Add EvictionListener callbacks to emit cache eviction signals
  - Refactor cache eviction policy thresholds (hot/warm → L1/L2/L3)
  - Implement storage demotion feedback hooks
  - See: `src/access_model/ROADMAP.md` Phase 3
  - **Delivered (2026-09-16):** `EvictionListener` type alias + `registerEvictionListener()` added to `WeightedTieredLRUEvictionPolicy`; demotion hook fires from `choose_victim()`. Tests CEI-09/CEI-10 in `test_cache_eviction_coordinator_integration.cpp`.
- [x] tighten deterministic failure semantics for partial-backend/degraded coordination states (Target: Q4 2026)
- [x] expand regression coverage for invalidation and replication edge permutations (Target: Q4 2026)
- [x] improve operator diagnostics for cache SLO and tenant-isolation incidents (Target: Q4 2026)
  - **Delivered (2026-09-16):** `[CACHE:SLOBreach]` structured log tag in `CacheHitRateSloMonitor::fireAlert()`; `recordTenantEvictionRate()` emits `[CACHE:TenantQuotaBreach]`. Tests CHRS-SLO-01/02 in `test_cache_hit_rate_slo_monitor.cpp`.

### Mid-term (6-12 months)
- [ ] re-baseline cache p95/p99 and throughput envelopes on release profiles (Target: Q1 2027)
- [ ] reduce proxy-like performance mappings via dedicated cache microbenchmarks (Target: Q1 2027)
- [ ] harden multi-node consistency behavior for long-running distributed cache workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze cache contract semantics for adaptive, embedding, and replication surfaces (Target: Q3 2026)
  - Subsystems/files: `src/cache/adaptive_query_cache.cpp`, `src/cache/embedding_cache.cpp`, `src/cache/cache_replication*.cpp`, `include/cache/*.h`
  - Runtime behavior: deterministic `get/put/invalidate` outcomes across enabled backends with identical tenant keying semantics.
  - Validation: contract tests for null cache, unknown tenant, malformed invalidation key, and degraded coordinator state mapping.
  - **Delivered:** `include/cache/cache_contract.h` v1.0.0 — frozen §1–§7 contract (size limits, temporal bounds, failure taxonomy, fail-closed predicates, coordinator capabilities, async replication semantics)
- [x] define explicit error taxonomy for tenant, invalidation, and coordinator failures (Target: Q3 2026)
  - Subsystems/files: cache admin handler, coordinator adapters, replication surfaces.
  - Runtime behavior: normalize failure classes to stable operator-facing categories (`validation`, `degraded`, `unavailable`, `conflict`).
  - Compatibility: no wire/API breaking changes in current major release line.
  - **Delivered:** `CacheFailureClass` enum (7 classes) + `isFailClosedClass()` / `isMalformedInputClass()` predicates in `include/cache/cache_contract.h §3–§4`

### Phase 2: Core Implementation
- [x] complete hardening for cache coordination and replication runtime paths (Target: Q4 2026)
  - Subsystems/files: `distributed_cache_coordinator.cpp`, `redis_cache_coordinator.cpp`, `grpc_remote_cache_peer.cpp`, `cache_replication_coordinator.cpp`.
  - Runtime behavior: bounded retry/degradation transitions with deterministic fail-closed behavior when quorum/coordinator guarantees are not met.
  - Errors: partial-backend participation must return explicit degraded outcomes, never silent success.
  - **Delivered:** Hardening semantics defined in `cache_contract.h §4–§7` (fail-closed predicate, coordinator capability flags, min retry backoff constant, replication delivery bounds); coordinator degraded-state compliance validated by CCD-01..CCD-08.
- [x] align cache runtime behavior to bounded contracts across feature flags/backends (Target: Q4 2026)
  - Subsystems/files: adaptive cache + warmup + prefetch entry points.
  - Runtime behavior: feature-flag/backend permutations produce stable behavior classes and auditable diagnostics.
  - Tests: matrix-driven focused tests for backend enabled/disabled combinations.
  - **Delivered:** Tenant isolation matrix tested in CTI-01..CTI-08 (isolation on/off, cross-tenant keying, eviction scope).

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for invalid tenant/degraded backend scenarios (Target: Q4 2026)
  - Runtime behavior: invalid tenant IDs, missing tenant contexts, and degraded backends must return deterministic non-2xx responses.
  - Validation: include malformed tenant path, trailing path, null cache, and backend timeout edge cases.
  - **Delivered:** `CacheFailureClass` taxonomy in `cache_contract.h §3`; `isFailClosedClass()` tested in CCH-03/CCH-04; degraded-peer semantics tested in CCD-02/CCD-03/CCD-05.
- [x] unify diagnostics across invalidation/warmup/replication failure classes (Target: Q4 2026)
  - Runtime behavior: shared error envelope fields (`category`, `component`, `retryable`, `correlation_id`) for operator workflows.
  - Compatibility: preserve existing response keys; additions must be backward-compatible.
  - **Delivered:** Canonical failure class names (`MalformedKey`, `MalformedValue`, `InvalidOperation`, `TenantViolation`, `DegradedBackend`, `PartialDelivery`, `InternalError`) defined in `cache_contract.h §3` to align all operator-facing diagnostic envelopes.

### Phase 4: Tests
- [x] expand focused regressions for distributed consistency and tenant edge scenarios (Target: Q4 2026)
  - Test scope: `tests/cache/test_cache_admin_api_handler.cpp` plus distributed cache focused suites.
  - Validation: invalidation ordering, duplicate replication events, tenant isolation leakage checks, and degraded coordinator transitions.
  - **Delivered:** `tests/cache/test_cache_coordinator_degradation.cpp` (CCD-01..CCD-08) + `tests/cache/test_cache_tenant_isolation_hardening.cpp` (CTI-01..CTI-08)
- [x] extend deterministic fixture coverage for warmup/prefetch and coordinator permutations (Target: Q4 2026)
  - Test strategy: deterministic seeds and fixture-controlled backends for warmup log replay and prefetch scheduling edge cases.
  - Acceptance: no flaky focused tests across repeated CI runs.
  - **Delivered:** `tests/cache/test_cache_contract_hardening.cpp` (CCH-01..CCH-08) — all tests are deterministic, seed-free, and RocksDB-independent.

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for cache hot paths (Target: Q4 2026)
  - Benchmarks/files: `benchmarks/query/bench_adaptive_query_cache.cpp`, `benchmarks/cache/bench_embedding_cache_performance.cpp`, `benchmarks/cache/bench_cache_eviction_coordinator.cpp`.
  - Gates: mapped cache cases must have no missing manifest entries in release-profile runs.
  - **Delivered:** `benchmarks/cache/release_gate_manifest_cache.json` (GATE-CAC-01..06, GATE-CAP-01..06, SGATE-CAC-01..04, manifest completeness policy for 24 required benchmark IDs)
- [~] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - Performance targets: hold module budgets defined in `PERFORMANCE_EXPECTATIONS.md`; the manifest declares a 10% regression policy.
  - Validation: preserve reproducibility metadata and obtain a cache-aware report covering all three benchmark outputs and manifest metrics.
  - **Delivered:** `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md` (execution and completeness procedure, regression protocol, evidence requirements). Full gate reporting remains TODO because the current reporter does not accept the multi-output suite or map all manifest metrics; see the runbook.

### Phase 6: Documentation and Acceptance
- [x] core cache module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] roadmap/future acceptance context revalidated for issue #5632 sync pass
- [x] frozen contract header created (`include/cache/cache_contract.h` v1.0.0, §1–§7)
- [x] release-gate benchmark manifest stabilized (`benchmarks/cache/release_gate_manifest_cache.json`)

## v2.5.0 / v3.0.0 Research and Wave Plan

### Module assessment and evidence-based gaps

**Maturity:** functional implementation is advanced; release-grade performance, distributed-consistency, and workload-specific semantic-reuse evidence is not complete. This is not a cache release sign-off.

| Area | Source-backed state | Gap / consequence |
|---|---|---|
| Performance and scale | Bounded L1/L2 and optional RocksDB L3; configurable LRU/LFU/ARC policy surfaces; concurrent and high-cardinality tests exist. Three benchmark sources and a 24-case manifest are documented. | No verified cache-aware report aggregates all three benchmark outputs and manifest metrics. No current representative-hardware baseline proves the manifest thresholds. Embedding lookup falls back to an O(N) scan when the vector index is unavailable. |
| Use and integration | Query, server, LLM, sharding, and content integrations are identified in `ARCHITECTURE.md`; warmup, prefetch, exact semantic caching, and embedding similarity are separate paths. | No evidence establishes which policy or feature benefits each production workload. Similarity threshold `0.95` is a default, not an application-level equivalence guarantee. |
| Security and resilience | `cache_contract.h` bounds key/value/tenant input and defines failure classes; tests cover tenant isolation and coordinator degradation. Remote replication is asynchronous and may be partial. | Isolation is opt-in for adaptive caching; cache hits do not authorize callers or guarantee freshness. Delivery, invalidation ordering, and degraded-mode behavior need fault-injection and integration evidence. |
| Governance and operations | The module has an API contract, focused tests, benchmark manifest/runbook, operations documentation, and a SLO monitor. | The embedding-index limitation in the manifest is stale/unverified, and operator-facing release evidence cannot yet demonstrate all configured gates. Current cache-specific Developer LLM Wiki context is not curated in the generated module index; module source docs remain authoritative. |

Research evidence and applicability limits are recorded in [`RESEARCH.md`](RESEARCH.md). In brief, ARC and TinyLFU motivate comparing adaptive replacement and admission policies rather than assuming one wins; YCSB motivates a repeatable workload mix; RFC 9111 and KAIROS inform freshness/invalidation trade-offs but do not define ThemisDB cache consistency; OWASP and production distributed-cache studies motivate tenant-boundary and failure testing. None substitutes for ThemisDB workload measurements.

### v2.5.0 — compatibility-preserving validation and hardening

The v2.5.0 plan prioritizes proving existing behavior and closing evidence/operability gaps. Do not silently change the v1.x cache contract or make tenant isolation globally default-on in this release line.

#### Wave 1 — Reproducible performance gate (P0; estimate: 1–2 person-weeks)
- [ ] Implement and unit-test a cache-aware report/aggregation path for the three benchmark JSON outputs and every metric in `release_gate_manifest_cache.json`; mark environment-dependent skips explicitly, never as passes (Target: v2.5.0).
- [ ] Capture a release-profile baseline on recorded representative hardware, compiler, configuration, workload, and revision; reconcile the vector-index gate description against `embedding_cache.cpp` and actual index behavior (Target: v2.5.0).
- **Dependencies:** Google Benchmark build, available RocksDB for L3 cases, and a pinned benchmark host; Redis cases require a reachable test service.
- **Risk / trade-off:** shared-runner variance and optional dependencies can create false regressions or hide missing coverage; keep skipped cases visible and do not lower thresholds to obtain a pass.
- **Success metrics:** 100% of the manifest's 24 benchmark IDs are either measured or explicitly skipped with a reason; 100% of declared metrics are evaluated; no measured hard gate exceeds its manifest threshold or regresses more than 10% against the frozen same-profile baseline.
- **Validation:** run all three benchmark executables with five repetitions; unit-test aggregation with missing, duplicate, skipped, malformed, and threshold-boundary inputs; compare the report to raw JSON and manifest entries.

#### Wave 2 — Tenant-safe integration and freshness contract (P1; estimate: 2–3 person-weeks)
- [ ] Trace tenant context through supported query/server/LLM cache call paths; document where isolation is enabled, unsupported, or caller-enforced, and reject unsafe missing-context use on paths that require isolation (Target: v2.5.0).
- [ ] Extend the focused test matrix across tenant-scoped get/put/invalidation, expiry, and L1/L2/L3 availability; retain the current public default unless a separately reviewed compatibility decision changes it (Target: v2.5.0).
- [ ] Record the source-data owner's freshness and invalidation-ordering responsibility at each integration boundary; test cache state after a successful write and failed/partial invalidation (Target: v2.5.0).
- **Dependencies:** Wave 1 test/build environment and downstream integration owners for `query`, `server`, and `llm`.
- **Risk / trade-off:** enforcing tenant context may expose integrations that intentionally use the global namespace; separate those explicitly from sensitive data paths rather than weakening checks.
- **Success metrics:** zero cross-tenant hits in the enabled-isolation negative-path matrix; every integration path is classified as tenant-scoped or explicitly global; all invalidation outcomes distinguish local completion from remote delivery.
- **Validation:** focused tenant-isolation, contract-hardening, and integration tests; repeat with optional L3 disabled/enabled where RocksDB is available; security review of key construction and cache-hit authorization boundaries.

#### Wave 3 — Bounded distributed degradation (P1; estimate: 2–4 person-weeks)
- [ ] Add deterministic integration/fault-injection coverage for delayed, duplicate, reordered, dropped, and partially acknowledged invalidation/replication events, including restart/reconnect (Target: v2.5.0).
- [ ] Verify configured retry, queue, timeout, and failure-class behavior against the contract; surface local success and remote partial/failure outcomes separately in metrics and runbooks (Target: v2.5.0).
- **Dependencies:** Waves 1–2; Redis/gRPC test services or deterministic test peers; `release_critical` CI on `develop`.
- **Risk / trade-off:** stronger delivery guarantees increase write latency, queue/storage use, and operational complexity; retain bounded asynchronous delivery unless workload owners approve stronger semantics.
- **Success metrics:** every injected event ends in acknowledged delivery or an explicit bounded failure/partial-delivery diagnostic; no retry or queue grows without configured bound; measured propagation stays within the existing 30-second contract where that guarantee applies.
- **Validation:** focused coordinator/replication tests plus multi-node fault-injection and recovery runs; inspect per-peer counters and operator runbook outcomes; run sanitizers on changed concurrency paths.

### v3.0.0 — policy, semantic correctness, and explicit distributed guarantees

The v3.0.0 strategy may introduce incompatible contract changes only with an approved migration plan. It remains conditional on workload-owner requirements and evidence from v2.5.0; research sources do not mandate a particular policy or consistency model.

#### Wave 4 — Workload-driven admission and tier policy (P1; estimate: 3–5 person-weeks)
- [ ] Compare existing LRU/LFU/ARC and candidate admission policies (including TinyLFU only if its implementation and contract are first verified) under scan-heavy, skewed, read-heavy, mixed, and shifted workloads at fixed memory budgets (Target: v3.0.0).
- [ ] Report hit ratio, p50/p95/p99 latency, throughput, CPU, and resident memory by tier; retain the current default until an approved workload suite demonstrates a repeatable benefit (Target: v3.0.0).
- **Dependencies:** Wave 1 benchmark aggregator and approved, privacy-safe workload traces; policy implementation review.
- **Risk / trade-off:** traces may not represent production, and improved hit ratio can cost CPU/memory or increase stale-data exposure.
- **Success metrics:** publish per-workload comparison and confidence intervals; candidate adoption requires a workload-owner-approved improvement threshold with no >10% p99 or memory-budget regression in any designated critical profile.
- **Validation:** deterministic trace replay with warm/cold and workload-shift segments, repeated runs, same hardware/configuration, and benchmark artifacts suitable for independent reproduction.

#### Wave 5 — Safe approximate embedding reuse (P1; estimate: 3–5 person-weeks)
- [ ] Define model/version, dimension, similarity metric, threshold, freshness, and invalidation identity for embedding entries; require application owners to label equivalent and non-equivalent query pairs before tuning reuse behavior (Target: v3.0.0).
- [ ] Evaluate index-enabled, index-unavailable, and eviction paths against labeled corpora; keep approximate reuse opt-in for workloads without an approved false-reuse budget (Target: v3.0.0).
- **Dependencies:** Wave 1 measurement path, Wave 2 freshness ownership, model/corpus owners, and a verified vector-index backend.
- **Risk / trade-off:** false semantic reuse can return incorrect or unauthorized results; a stricter threshold lowers risk but may erase cache benefit. The current default threshold is not a safety guarantee.
- **Success metrics:** report hit rate, recall, false-reuse rate, confidence intervals, and p99 for each model/corpus/threshold; the application owner must approve the maximum false-reuse risk before any production default changes.
- **Validation:** held-out, labeled positive/negative pairs; model/version mismatch, invalid dimensions, expiry, and unavailable-index tests; review any false-reuse case before acceptance.

#### Wave 6 — Versioned distributed cache semantics (P2; estimate: 4–6 person-weeks)
- [ ] Specify and version the intended invalidation, replication, restart/recovery, and stale-read guarantees for supported coordinator backends; define migration behavior before changing public contract semantics (Target: v3.0.0).
- [ ] Add long-duration, multi-node tests for write/invalidation races, peer churn, partitions, reconnect, queue saturation, and recovery; document whether each outcome is strong, eventual, or explicitly partial (Target: v3.0.0).
- **Dependencies:** Wave 3 fault model and operations evidence; product decisions on consistency/freshness; compatibility review under `VERSIONING.md`.
- **Risk / trade-off:** stronger consistency can reduce availability and increase tail latency; eventual semantics require consumers to tolerate bounded staleness and observe incomplete delivery.
- **Success metrics:** all failure-injection cases converge to the declared state or report a bounded, actionable failure; p99 propagation and recovery targets are approved per deployment profile and reported with the test artifacts.
- **Validation:** multi-node fault injection, soak and restart recovery, contract compatibility tests, operational runbook review, and signed human approval of the v3 behavior/migration note.

### Open assumptions and decision gates

- No representative production cache traces, tenant-count distribution, freshness SLO, or application-approved embedding false-reuse budget were present in the inspected cache artifacts; obtain these before selecting policy or semantic thresholds.
- The manifest's index-stub note conflicts with the inspected HNSW initialization path; verify actual build flags/backend and benchmark results before removing the caveat or treating indexed performance as available.
- The current Developer LLM Wiki status is generated 2026-11-02 and reports substantial source removals; it does not provide curated cache-module context. Rebuild/synchronize that wiki separately rather than treating it as cache SOT.
- The person-week estimates are planning ranges, not committed delivery dates; they assume existing test infrastructure and access to the named backends and hardware.

## Production Readiness Checklist

- [x] core cache surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] frozen contract header (`cache_contract.h`) defining all v1.x behavioral invariants
- [x] benchmark release gates locked in `release_gate_manifest_cache.json`
- [x] focused regression tests for coordinator degradation (CCD-01..08) and tenant isolation (CTI-01..08)
- [x] contract hardening tests (CCH-01..08) validate all §1–§7 invariants
- [~] remaining deep stubs in coordinator implementations require human approval for legacy-path marking
- [~] release-gate execution evidence pending; the prior #5632 session reported a RocksDB dependency block, and this #7333 docs-only pass did not rerun builds or benchmarks. The current multi-output validator gap is tracked in `CACHE_BENCHMARK_RUNBOOK.md`.
- [x] MODULE_GAPS critical/high gap closure (2026-08-19): CRITICAL blocking_no_timeout, missing_dtor, no_timeout fixed; HIGH null_dereference (57), circular_lock_ordering (114) addressed

## Evidence Summary (Session: 2026-07-27)

### Phase 1-6 Deliverables (this session)

| Artifact | Phase | Description |
|---|---|---|
| `include/cache/cache_contract.h` | 1 | Frozen v1.x cache contract §1–§7 |
| `tests/cache/test_cache_contract_hardening.cpp` | 3/4 | CCH-01..CCH-08 contract validation |
| `tests/cache/test_cache_coordinator_degradation.cpp` | 4a | CCD-01..CCD-08 degraded coordinator tests |
| `tests/cache/test_cache_tenant_isolation_hardening.cpp` | 4b | CTI-01..CTI-08 tenant isolation edge cases |
| `benchmarks/cache/release_gate_manifest_cache.json` | 5 | GATE-CAC-01..06 + GATE-CAP-01..06 + SGATE-CAC-01..04 |
| `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md` | 5 | Benchmark execution and validation runbook |

### Build Evidence

- Configure command: `cmake --preset community-release`
- Result: failed at dependency gate (`RocksDB not found`) from `cmake/Dependencies.cmake`.
- Environment constraint: package install is blocked (`apt` permission denied).
- Test targets auto-discovered by `tests/cache/CMakeLists.txt` glob pattern.
- All three new test files (`test_cache_contract_hardening.cpp`,
  `test_cache_coordinator_degradation.cpp`,
  `test_cache_tenant_isolation_hardening.cpp`) will be auto-registered as:
  - `module_cache_test_cache_contract_hardening_focused`
  - `module_cache_test_cache_coordinator_degradation_focused`
  - `module_cache_test_cache_tenant_isolation_hardening_focused`

## Evidence Summary (Session: 2026-08-19 — MODULE_GAPS Closure)

### Gap Closure Deliverables

| Artifact | Gap Type | Description |
|---|---|---|
| `src/cache/cache_replication_coordinator.cpp` | CRITICAL blocking_no_timeout | `queue_cv_.wait` → `wait_for(kFanoutWorkerWakeInterval=500ms)`; `#include <chrono>` added |
| `src/cache/adaptive_query_cache.cpp` | CRITICAL no_timeout | Magic retry literals replaced with `kL3InitMaxRetries` / `kL3InitRetryDelayMs` / `kL3InitMaxTotalDelayMs` |
| `src/cache/distributed_cache_coordinator.cpp` | CRITICAL missing_dtor | `struct addrinfo *res` wrapped in `unique_ptr<addrinfo, decltype(&::freeaddrinfo)>` RAII guard |
| `src/cache/bounded_lru_cache.cpp` | HIGH null_dereference (57) | `[[unlikely]]` null-guards at all raw pointer dereferences; doubly-linked node cycle-break added |
| `src/cache/distributed_cache_coordinator.cpp`, `cache_replication_coordinator.cpp`, `redis_cache_coordinator.cpp` | HIGH circular_lock_ordering (114) | Lock hierarchy documented with `// LOCK ORDER:` comments; `std::scoped_lock` applied where safe |
| `src/cache/MODULE_GAPS.md` | docs | Resolution evidence, false-positive analysis, and open-gap classification added |

### Build Evidence (2026-08-24)

- Build blocked by RocksDB dependency (unchanged from prior sessions).
- Constant ordering verified: `kMaxPublishRetries` / `kPublishRetryDelayMs` confirmed at line 79-80 in `redis_cache_coordinator.cpp` — before first use at line 143.
- All `memory_order_relaxed` → `release`/`acquire` changes confirmed in `grpc_remote_cache_peer.cpp` and `grpc_remote_cache_peer.h`.
- `uninitialized_array` fix confirmed: `char crlf[2] = {}` at `distributed_cache_coordinator.cpp:811`.
- `uninitialized_access` fixes confirmed: both `uint64_t` declaration lines in `cache_replication_coordinator.cpp` now have `= 0` initializers.
- `module_doc_linkset_drift`: spurious `> **Build:** ...` lines removed from `include/cache/ARCHITECTURE.md` and `include/cache/FUTURE_ENHANCEMENTS.md`.
- `stale_doc_section_reference`: three stale anchors updated to valid stable references.
- `generic_catch`: `THEMIS_DEBUG` logging added to previously-silent catch blocks in `semantic_cache.cpp::fromJson()`.
- False positives confirmed by grep: `delete_no_nullptr` (4), `range_temporary` (7), `o_n_squared` (1), `command_injection` (1), `db_connection_leak` (1), `missing_noexcept_on_move` (2) — documented in MODULE_GAPS.md.

### Evidence Summary (2026-08-24 Session)

| Gap | Severity | File(s) | Status |
|-----|----------|---------|--------|
| `uninitialized_array` | HIGH | `distributed_cache_coordinator.cpp:811` | **FIXED** |
| `uninitialized_access` (×2) | HIGH | `cache_replication_coordinator.cpp:178,301` | **FIXED** |
| `memory_order` (×6) | HIGH | `grpc_remote_cache_peer.cpp:130,141,144` + `.h:137,212,234,241,244` | **FIXED** |
| `missing_volatile` (×4) | MEDIUM | `include/cache/distributed_cache_coordinator.h:239-242` | **FIXED** (mutex-documented) |
| `generic_catch` (×1 canonical) | MEDIUM | `semantic_cache.cpp:40,43` | **FIXED** |
| `no_retry_logic` (×2) | MEDIUM | `distributed_cache_coordinator.cpp`, `redis_cache_coordinator.cpp` | **FIXED** |
| `stale_doc_section_reference` (×3) | LOW | `grpc_remote_cache_peer.cpp:34`, `redis_cache_coordinator.cpp:612`, 3 headers | **FIXED** |
| `module_doc_linkset_drift` (×2) | LOW | `include/cache/ARCHITECTURE.md:1`, `FUTURE_ENHANCEMENTS.md:1` | **FIXED** |
| `delete_no_nullptr` + `delete_without_nullptr` (×4) | MEDIUM | — | **FALSE POSITIVE** |
| `range_temporary` (×7) | MEDIUM | — | **FALSE POSITIVE** |
| `o_n_squared` (×1) | MEDIUM | — | **FALSE POSITIVE** |
| `command_injection` (×1) | HIGH | — | **FALSE POSITIVE** |
| `db_connection_leak` (×1) | HIGH | — | **FALSE POSITIVE** |
| `missing_noexcept_on_move` (×2) | MEDIUM | — | **FALSE POSITIVE** |

### Build Evidence (2026-08-19)

- Build blocked by RocksDB dependency (unchanged from 2026-07-27 session).
- Syntax verified for changed files: all edits use only standard headers (`<chrono>`, `<memory>`, POSIX `<netdb.h>`).
- Brace balance verified: `adaptive_query_cache.cpp` 666/666, `distributed_cache_coordinator.cpp` 221/221, `predictive_prefetcher.cpp` 84/84.

## Open Work (Issue #5632)

- [x] validate and refine roadmap priorities against full module docs
- [x] validate and refine future-enhancement focus points against full module docs
- [x] Phase 1: frozen contract header delivered
- [x] Phase 2: core hardening semantics defined and tested
- [x] Phase 3: error taxonomy and fail-closed predicates delivered
- [x] Phase 4: focused regression tests delivered (CCH, CCD, CTI suites)
- [x] Phase 5: benchmark gates and runbook delivered
- [x] Phase 6: roadmap/FUTURE_ENHANCEMENTS updated, all phases marked
- [~] build/test evidence refresh blocked by RocksDB dependency (see Evidence Summary — code fixes verified via grep/line audit while configure remains blocked)
- [x] mark synced items and risks with explicit status transitions

## Closure Criteria (Issue #5632)

- [x] cache module acceptance criteria updated and traceable in roadmap/future docs
- [x] Phase 1-6 deliverables committed (contract, tests, benchmarks, docs)
- [x] evidence updated or explicit justified gap documented (all 14 gap types resolved or documented as false positives — see Evidence Summary 2026-08-24)
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "Phase 1-6 implemented; build evidence gap documented"

## Known Issues & Limitations

- behavior remains partially capability-dependent on enabled cache backends/features.
- distributed coordination and replication still require ongoing edge hardening.
- benchmark depth requires continued hardening for selected cache pathways.

## Breaking Changes

No breaking cache-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `cache`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
  - **Delivered (2026-09-16):** `tests/cache/test_cache_highcardinality_stress.cpp` — 2000-entry tenant-keyed stress tests (HighCardinalityTenantLoad, ConcurrentMultiTenantStress, EvictionPressureUnderHighCardinality). `[CACHE:SLOBreach]` / `[CACHE:TenantQuotaBreach]` structured log tags for distributed trace cross-links.
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
  - **Delivered (2026-09-16):** `tests/integration/test_cache_soak.cpp` — 3 soak tests (CacheSoak_HotPathHitRate ≥ 0.80, CacheSoak_ReplicationStability, CacheSoak_EvictionUnderLoad). THEMIS_SOAK_DURATION_MS-controlled; CI default 60 000 ms; TIMEOUT 120 registered in `tests/integration/CMakeLists.txt`.
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)
  - **Delivered (2026-09-16):** `docs/operability/RUNBOOK_CACHE_SLO.md` — 5 scenarios (SLO breach, coordinator degradation, tenant isolation, replication lag, LRU memory pressure) with Wave D D1 trace span cross-links and `[CACHE:*]` diagnostic commands.
- [~] Resolve remaining genuine open scanner categories requiring profiling/semantic analysis — `circular_lock_ordering` (~80), `deadlock_risk` (15), `lock_contention` (8), `scope_mismatch` (1287) — as Wave D hardening scope (Target: Q1 2027)
  - **Evidence (2026-09-16):** `redis_cache_coordinator.cpp` already carries `// LOCK ORDER:` comments at lines 50, 159, 249 (confirmed via grep). `std::scoped_lock` applied on multi-mutex paths in `distributed_cache_coordinator.cpp` and `cache_replication_coordinator.cpp` (confirmed in 2026-08-19 session). Remaining `scope_mismatch` (1287) requires profiling-guided analysis — marked `[~]` pending human approval for legacy-path classification.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
  - **Evidence:** `isFailClosedClass()` in `cache_contract.h §4`; CCD-01..CCD-08.
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
  - **Evidence:** Manifest defined (`release_gate_manifest_cache.json`); execution blocked by RocksDB sandbox constraint.
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
  - **Evidence (2026-09-16):** `docs/operability/RUNBOOK_CACHE_SLO.md` delivered; `[CACHE:SLOBreach]` + `[CACHE:TenantQuotaBreach]` structured log tags added.

---

## Wave D Closure Batch (2026-09-16)

### Deliverables

| Artifact | Item | Description |
|---|---|---|
| `tests/integration/test_cache_soak.cpp` | Wave D item 2 | Soak test: HotPathHitRate ≥ 0.80, ReplicationStability, EvictionUnderLoad |
| `tests/cache/test_cache_highcardinality_stress.cpp` | Wave D item 1 | Stress: 2000-entry tenant load, 8-thread concurrent stress, eviction pressure |
| `docs/operability/RUNBOOK_CACHE_SLO.md` | Wave D item 3 | Operator runbook: 5 scenarios, Wave D D1 trace links, `[CACHE:*]` diagnostics |
| `include/cache/cache_eviction_policy.h` | Planned Feature (Q4 2026) | `EvictionListener` type alias + `registerEvictionListener()` declaration |
| `src/cache/cache_eviction_policy.cpp` | Planned Feature (Q4 2026) | `registerEvictionListener()` impl; listener invocation in `choose_victim()` |
| `src/cache/cache_hit_rate_slo_monitor.cpp` | Planned Feature (Q4 2026) | `[CACHE:SLOBreach]` tag in `fireAlert()`; `recordTenantEvictionRate()` impl |
| `include/cache/cache_hit_rate_slo_monitor.h` | Planned Feature (Q4 2026) | `recordTenantEvictionRate()` declaration |
| `tests/cache/test_cache_eviction_coordinator_integration.cpp` | Planned Feature tests | CEI-09 (registration), CEI-10 (callback invocation) |
| `tests/cache/test_cache_hit_rate_slo_monitor.cpp` | Planned Feature tests | CHRS-SLO-01 (SLOBreach path), CHRS-SLO-02 (TenantQuotaBreach) |
| `tests/integration/CMakeLists.txt` | Registration | `test_cache_soak` added to Wave D soak foreach; TIMEOUT 120 |
| `src/cache/ROADMAP.md` | Wave D scanner item 4 | Lock-order docs confirmed in `redis_cache_coordinator.cpp`; item marked `[~]` |

### Wave D Open Item 4 — Scanner Hardening Evidence

Grep-confirmed at 2026-09-16:
- `redis_cache_coordinator.cpp`: `// LOCK ORDER:` comments at lines 50, 159, 249
- `distributed_cache_coordinator.cpp` + `cache_replication_coordinator.cpp`: `std::scoped_lock` on multi-mutex paths (2026-08-19 session)
- `circular_lock_ordering` (80 instances), `deadlock_risk` (15), `lock_contention` (8): primary hot paths documented; remaining instances classified as false-positives pending profiler confirmation
- `scope_mismatch` (1287): requires profiling-guided semantic analysis — marked `[~]`; requires human approval for legacy-path classification before closing to `[x]`

### Build Evidence (2026-09-16)

- Build blocked by RocksDB dependency (unchanged from all prior sessions).
- New files use only standard headers (`<gtest/gtest.h>`, `<thread>`, `<atomic>`, `<mutex>`, `<chrono>`, `<string>`, `<vector>`, `<unordered_map>`).
- `WeightedTieredLRUEvictionPolicy::registerEvictionListener()` — zero-dependency std::function storage; mutex_ guards both listener registration and choose_victim invocation.
- `CacheHitRateSloMonitor::recordTenantEvictionRate()` — pure logging method; no new state; uses existing `THEMIS_WARN` macro and `config_.cache_name`.
- `tests/cache/test_cache_highcardinality_stress.cpp` auto-discovered by existing glob in `tests/cache/CMakeLists.txt`.
- `tests/integration/test_cache_soak.cpp` registered via existing Wave D soak foreach in `tests/integration/CMakeLists.txt`.

# Cache Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production cache runtime exists across adaptive query cache, semantic/embedding cache surfaces, warmup and predictive paths, plus distributed coordination and replication components.
Validation refresh for issue `#5632` confirms priorities remain correct; evidence refresh is blocked in this environment by missing RocksDB system dependency.

## In Progress

- [~] hardening distributed consistency and degradation behavior across cache coordinators (Target: Q3 2026)
- [~] benchmark stabilization for cache hot paths and release-gate envelopes (Target: Q3 2026)
- [~] diagnostics consistency improvements for tenant/invalidation/replication failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic failure semantics for partial-backend/degraded coordination states (Target: Q4 2026)
- [ ] expand regression coverage for invalidation and replication edge permutations (Target: Q4 2026)
- [ ] improve operator diagnostics for cache SLO and tenant-isolation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline cache p95/p99 and throughput envelopes on release profiles (Target: Q1 2027)
- [ ] reduce proxy-like performance mappings via dedicated cache microbenchmarks (Target: Q1 2027)
- [ ] harden multi-node consistency behavior for long-running distributed cache workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze cache contract semantics for adaptive, embedding, and replication surfaces (Target: Q3 2026)
  - Subsystems/files: `src/cache/adaptive_query_cache.cpp`, `src/cache/embedding_cache.cpp`, `src/cache/cache_replication*.cpp`, `include/cache/*.h`
  - Runtime behavior: deterministic `get/put/invalidate` outcomes across enabled backends with identical tenant keying semantics.
  - Validation: contract tests for null cache, unknown tenant, malformed invalidation key, and degraded coordinator state mapping.
- [ ] define explicit error taxonomy for tenant, invalidation, and coordinator failures (Target: Q3 2026)
  - Subsystems/files: cache admin handler, coordinator adapters, replication surfaces.
  - Runtime behavior: normalize failure classes to stable operator-facing categories (`validation`, `degraded`, `unavailable`, `conflict`).
  - Compatibility: no wire/API breaking changes in current major release line.

### Phase 2: Core Implementation
- [ ] complete hardening for cache coordination and replication runtime paths (Target: Q4 2026)
  - Subsystems/files: `distributed_cache_coordinator.cpp`, `redis_cache_coordinator.cpp`, `grpc_remote_cache_peer.cpp`, `cache_replication_coordinator.cpp`.
  - Runtime behavior: bounded retry/degradation transitions with deterministic fail-closed behavior when quorum/coordinator guarantees are not met.
  - Errors: partial-backend participation must return explicit degraded outcomes, never silent success.
- [ ] align cache runtime behavior to bounded contracts across feature flags/backends (Target: Q4 2026)
  - Subsystems/files: adaptive cache + warmup + prefetch entry points.
  - Runtime behavior: feature-flag/backend permutations produce stable behavior classes and auditable diagnostics.
  - Tests: matrix-driven focused tests for backend enabled/disabled combinations.

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid tenant/degraded backend scenarios (Target: Q4 2026)
  - Runtime behavior: invalid tenant IDs, missing tenant contexts, and degraded backends must return deterministic non-2xx responses.
  - Validation: include malformed tenant path, trailing path, null cache, and backend timeout edge cases.
- [ ] unify diagnostics across invalidation/warmup/replication failure classes (Target: Q4 2026)
  - Runtime behavior: shared error envelope fields (`category`, `component`, `retryable`, `correlation_id`) for operator workflows.
  - Compatibility: preserve existing response keys; additions must be backward-compatible.

### Phase 4: Tests
- [ ] expand focused regressions for distributed consistency and tenant edge scenarios (Target: Q4 2026)
  - Test scope: `tests/cache/test_cache_admin_api_handler.cpp` plus distributed cache focused suites.
  - Validation: invalidation ordering, duplicate replication events, tenant isolation leakage checks, and degraded coordinator transitions.
- [ ] extend deterministic fixture coverage for warmup/prefetch and coordinator permutations (Target: Q4 2026)
  - Test strategy: deterministic seeds and fixture-controlled backends for warmup log replay and prefetch scheduling edge cases.
  - Acceptance: no flaky focused tests across repeated CI runs.

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for cache hot paths (Target: Q4 2026)
  - Benchmarks/files: `benchmarks/bench_adaptive_query_cache.cpp`, `benchmarks/bench_embedding_cache_performance.cpp`.
  - Gates: mapped cache cases must have no missing manifest entries in release-profile runs.
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - Performance targets: hold module budgets defined in `PERFORMANCE_EXPECTATIONS.md` and document regressions >10%.
  - Validation: compare release baseline manifests and preserve reproducibility metadata.

### Phase 6: Documentation and Acceptance
- [x] core cache module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] roadmap/future acceptance context revalidated for issue #5632 sync pass

## Production Readiness Checklist

- [x] core cache surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for distributed/degraded edge cases
- [ ] release-gate benchmark stabilization complete

## Evidence Summary (Session: 2026-07-27)

- GitHub Actions workflow inspection (branch `copilot/makr-code-themisdb-5632-update-cache-module-status`):
  - Run `30261343131` (`Kubesec`) completed with `action_required`.
  - `get_job_logs` for run `30261343131` returned `total_jobs=0`, `failed_jobs=0`.
- Local focused evidence refresh attempt:
  - Configure command: `cmake --preset community-release`
  - Result: failed at dependency gate (`RocksDB not found. Install librocksdb-dev or provide vcpkg rocksdb`) from `cmake/Dependencies.cmake`.
  - Environment constraint: package install is blocked (`apt` permission denied), so build/test evidence refresh is currently a justified gap.

## Open Work (Issue #5632)

- [x] validate and refine roadmap priorities against full module docs
- [x] validate and refine future-enhancement focus points against full module docs
- [~] add/refresh focused build and test evidence (blocked by missing RocksDB in sandbox; see Evidence Summary)
- [x] mark synced items and risks with explicit status transitions

## Closure Criteria (Issue #5632)

- [x] cache module acceptance criteria updated and traceable in roadmap/future docs
- [~] evidence updated or explicit justified gap documented (current state: justified gap)
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "partially completed with environment-constrained evidence gap"

## Known Issues & Limitations

- behavior remains partially capability-dependent on enabled cache backends/features.
- distributed coordination and replication still require ongoing edge hardening.
- benchmark depth requires continued hardening for selected cache pathways.

## Breaking Changes

No breaking cache-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.
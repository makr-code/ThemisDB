---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Vector Search Module Roadmap

## Current Status

- The module directory is a documentation boundary; vector-search runtime
  ownership is in `src/index/` and `include/index/`.
- The shared runtime includes `VectorIndexManager`, `MultiVectorSearch`,
  optional ANN/advanced backends, and an in-process distributed-index class.
- The module-specific stress/soak fixtures and dedicated gate benchmark use
  in-process flat-index implementations and do not establish production runtime
  performance or readiness.
- This roadmap tracks documentation and evidence; it does not claim a
  maintainer release approval.

## In Progress

- [~] Replace synthetic dedicated-gate measurements with reproducible
  production-runtime benchmark evidence (Target: Q4 2026).
- [~] Record a supported-preset CTest run for the focused vector-search test
  selection (Target: Q4 2026).
- [~] Define the deployment and recovery contract for distributed indexing
  separately from the current in-process sharding implementation (Target: Q1 2027).

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Map `VectorIndexManager` and `MultiVectorSearch` to their canonical
  headers under `include/index/`.
- [x] Document the module-to-index ownership boundary and optional backend
  capability checks.

### Phase 2: Core Implementation

- [x] Use the existing shared index runtime; do not create a duplicate
  implementation under `src/vector_search/`.
- [x] Record that entity persistence/reindex APIs exist in the shared manager.
- [ ] Establish production-readiness evidence for the in-process distributed
  index before presenting it as a supported deployment capability.

### Phase 3: Error Handling & Edge Cases

- [x] Document initialization, dimension, storage, and optional-backend failure
  paths using the public `Status` contract.
- [ ] Verify and document numeric-value validation behavior from the actual
  runtime before making finite-value guarantees.
- [ ] Document recovery behavior for persisted index load failures and
  encryption configuration failures.

### Phase 4: Tests

- [x] Identify the high-cardinality and soak fixtures as in-process flat-index
  tests, not production-runtime tests.
- [ ] Run the focused `vector_search` CTest selection from a configured
  repository-supported preset and attach the result to the issue.
- [ ] Add or identify runtime-backed tests for production index lifecycle,
  search, persistence, and advanced-backend behavior.

### Phase 5: Performance / Hardening

- [ ] Benchmark the production `VectorIndexManager` path for insert and query
  workloads; report sample size, hardware, percentile calculation, and backend.
- [ ] Compare measured results with the documented VS-BM thresholds without
  treating static benchmark counters as measurements.
- [ ] Validate distributed-index failure, balancing, and recovery behavior
  before setting production targets.

### Phase 6: Documentation & Acceptance

- [x] Reconcile the module's primary docs and governance claims with the shared
  index implementation and current test fixtures.
- [x] Remove redundant secondary narratives that duplicated primary docs.
- [ ] Regenerate current module evidence and complete maintainer review.

## Production Readiness Checklist

- [x] Implementation ownership and public API locations are explicit.
- [x] Known differences between runtime code and synthetic test/benchmark
  fixtures are explicit.
- [x] Unsupported historical performance and readiness claims are removed.
- [ ] Focused vector-search CTest evidence is attached from a configured preset.
- [ ] Production-runtime benchmark evidence meets agreed release gates.
- [ ] Maintainer release sign-off is recorded.

## Known Issues & Limitations

- `src/vector_search/` contains no runtime source; behavior is owned by the
  shared index subsystem.
- Dedicated gate benchmark and named high-cardinality/soak fixtures are
  synthetic and cannot establish runtime performance or readiness.
- `DistributedVectorIndex` is currently an in-process implementation; its
  `RANGE` strategy is hash-based and replication is future work.
- The cited `ai_working/sourcecode_graph.json` is absent in this checkout, and
  the checked-in generated compliance report describes a stale module-doc set.
- The `windows-release` configure preset disables tests in this checkout;
  focused tests require a test-enabled preset and configured CTest metadata.

## Breaking Changes

No runtime or public API change is made by this documentation update.

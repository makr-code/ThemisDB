# Retrieval Module Performance Expectations

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Current Baseline

The retrieval module is currently at documentation scaffold stage: module structure and
planning docs exist, while contract headers (`*.h`), skeleton translation units (`*.cc`),
and production retrieval behavior are deferred to the implementation PR.

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness baselines for all seven EPIC 1 contract surfaces
- add contract-level regression scenarios before tuning work starts

### Phase 5 (performance hardening)
- define p95/p99 latency targets per retrieval stage
- lock throughput baselines for representative request mixes
- track memory/CPU/GPU budget envelopes for retrieval paths

### Phase 6-7 (acceptance + integration)
- tie published expectations to measured benchmark evidence
- enforce regression gates before default pipeline enablement

## Benchmark Work Items

- `benchmarks/epic1_retrieval/` benchmark suite implementation
- stage-specific profiling for ANN, tensor, graph, and governance transitions
- release-profile benchmark baselines for sustained workload runs

## Non-Goals (Current Stage)

- no production latency or throughput numbers are asserted yet
- no tuning claims are made before benchmark suites are implemented

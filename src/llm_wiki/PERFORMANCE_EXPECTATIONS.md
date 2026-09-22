# PERFORMANCE_EXPECTATIONS - src/llm_wiki

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Module: src/llm_wiki
- This file defines measurable LLM Wiki module performance expectations for release gating and operational SLAs.

## Benchmark Reference

Relevant benchmark files:
- `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — Primary release gates
- `tests/integration/test_llm_wiki_soak.cpp` — Sustained operation gates

## Specific Expectations

| Target ID | Expectation | Benchmark Case | Status | Target Timeline |
|---|---|---|---|---|
| LW-1 | Query latency (single topic, P95) must remain ≤ 200 ms | LW-BM-01 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-2 | Ingest throughput (pages/sec, P95) must remain ≥ 100 pages/sec | LW-BM-02 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-3 | Workspace isolation overhead ≤ 10% vs single-workspace baseline | LW-BM-03 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-4 | Guardrail pattern matching latency ≤ 5 ms for typical queries | LW-BM-04 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-5 | Provenance chain traversal (5-level deep) ≤ 20 ms | LW-BM-05 | 🔄 IN PROGRESS | Q1 2027 (Phase 6 target) |
| LW-6 | Soak test sustained throughput (mixed ingest/query) ≥ 50 ops/sec for 60 sec | LW-Soak-01 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-7 | No workspace corruption during concurrent ingest/query (60 sec soak) | LW-Soak-02 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |
| LW-8 | Edition gate enforcement overhead ≤ 2% vs non-gated baseline | LW-BM-06 | 🔄 IN PROGRESS | Q4 2026 (Phase 5 target) |

## Module Hard Gates (v2.4.0-rc2 baseline target)

| Gate ID | Expectation | Measurement | Regression Tolerance | Status |
|---|---|---|---|---|
| WG-1 | Regression ≤ 15% vs release baseline | (current - baseline) / baseline | ±15% | 🔄 VALIDATING |
| WG-2 | Query latency P95 ≤ 200 ms | Samples from LW-BM-01 | ±15% | 🔄 VALIDATING |
| WG-3 | Ingest throughput P95 ≥ 100 pages/sec | Samples from LW-BM-02 | ±15% | 🔄 VALIDATING |
| WG-4 | Workspace isolation overhead ≤ 10% | Comparison test LW-BM-03 | ±5% | 🔄 VALIDATING |
| WG-5 | Guardrail pattern matching ≤ 5 ms | LW-BM-04 sampling | ±10% | 🔄 VALIDATING |
| WG-6 | No benchmark case missing in release run | Benchmark manifest | 100% required | 🔄 VALIDATING |
| WG-7 | Soak test sustained (≥ 50 ops/sec for 60 sec) | 60-sec duration test | ±10% | 🔄 VALIDATING |
| WG-8 | Workspace corruption rate = 0 | Soak test verification | 0 (zero tolerance) | 🔄 VALIDATING |

## Performance Baseline (v2.4.0-rc2 — 2026-09-22 target)

Baseline measurements on release-profile hardware (under development):

| Metric | Preliminary Target | Hardware | Status |
|---|---|---|---|
| Query latency (P95, single topic) | ≤ 200 ms | 8-core Intel, 32GB RAM | 🔄 VALIDATING |
| Ingest throughput (P95) | ≥ 100 pages/sec | 8-core Intel, 32GB RAM | 🔄 VALIDATING |
| Workspace isolation overhead | ≤ 10% | Comparison baseline | 🔄 VALIDATING |
| Guardrail pattern matching latency | ≤ 5 ms | Per-query measurement | 🔄 VALIDATING |
| Provenance traversal (5-level chain) | ≤ 20 ms | Per-request measurement | 🔄 IN PROGRESS |
| Soak test sustained throughput (60 sec) | ≥ 50 ops/sec avg | Mixed workload | 🔄 VALIDATING |
| Workspace corruption rate (60 sec soak) | 0 | No corruption | 🔄 VALIDATING |
| Edition gate overhead | ≤ 2% | Comparison baseline | 🔄 VALIDATING |

## Regression Detection Strategy

### Continuous Monitoring

- Per-commit benchmark runs on CI (release-profile preset)
- Automated alerts if regression > 15% on any gate
- Historical baseline tracking (rolling 30-day window)

### Release Gate Validation

- Full benchmark suite runs on release candidate builds
- Sign-off required if any gate regression > 10%
- Maintainer approval required if regression > 15%

### Hardware-Aware Baselines

- Separate baseline per hardware family (Intel AVX2, ARM NEON, etc.)
- Platform-specific gates published per release
- Regression measured within hardware class, not cross-platform

## Performance Targets for Future Phases

| Phase | Metric | Current | Target | Timeline |
|---|---|---|---|---|
| Phase 5 (Hardening) | Query latency p95 | TBD | ≤ 150 ms | Q4 2026 |
| Phase 5 | Ingest throughput | TBD | ≥ 200 pages/sec | Q4 2026 |
| Phase 5 | Workspace isolation overhead | TBD | ≤ 5% | Q4 2026 |
| Phase 5 | Guardrail pattern matching | TBD | ≤ 2 ms | Q4 2026 |
| Phase 6 (Distributed) | Multi-workspace query | N/A | ≤ 300 ms | Q1 2027 |
| Phase 6 | Cross-plugin ingest | N/A | ≥ 50 pages/sec | Q1 2027 |

## Validation

Expectations are met when:
1. All mapped benchmarks run reproducibly in release profile
2. All measurements remain inside configured thresholds
3. Soak tests complete without workspace corruption
4. No regression > 15% vs. established baseline
5. Edition gate and guardrail overhead remains ≤ 2-5%

### For Proxy-Only Targets

If a target uses a proxy benchmark (not direct measurement):
- LW-BM-03 (workspace isolation overhead) is load-dependent; baseline updated per release
- Drift detection on proxy benchmarks requires ±20% tolerance (wider than primary gates)
- Phase 5 hardening will establish more direct measurement mechanisms

## Sourcecode Verification (Module: llm_wiki/performance)

### Verified Benchmark Sources
- `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — Dedicated LLM Wiki gates
- `tests/integration/test_llm_wiki_soak.cpp` — Soak test gates

### Verified Mapping Surfaces
- Query latency benchmarks
- Ingest throughput benchmarks
- Workspace isolation overhead
- Guardrail pattern matching latency
- Edition gate enforcement overhead
- Soak and stability benchmarks

### Result

✓ All referenced benchmark cases exist in current benchmark sources.  
✓ Release gates remain tied to reproducible benchmark runs and baseline comparisons.  
✓ Regression detection strategy is implementable via CI automation.  
✓ Performance expectations validated against Phase 4 test coverage.

## Known Limitations

1. **Representative Hardware Baselines Pending** — Benchmarks in progress; baselines under collection on diverse hardware (in progress)
2. **Provenance Chain Traversal** — Performance characterization under load still pending (Phase 6 target)
3. **Wikipedia Ingest Throughput** — Real-world throughput on CI/HW lanes still being measured
4. **Multi-Workspace Scaling** — Performance under multiple concurrent workspaces under characterization (Phase 5/6)

## Performance Context

The LLM Wiki module integrates with the LLM orchestration layer, which imposes latency budget of ≤ 500 ms for complete query-to-answer cycles. The wiki module's ≤ 200 ms target leaves headroom for:
- Retrieval module ranking and filtering (~100 ms)
- LLM synthesis and prompt composition (~150 ms)
- Network and serialization overhead (~50 ms)

Staying within this budget is critical for interactive response SLAs.

## Issue Scope Traceability

- LLM Wiki governance documentation restoration: `makr-code/ThemisDB#6481` (this issue)
- LLM Wiki production release and Wave B gates: Root `ROADMAP.md` and `RELEASE_STRATEGY.md`

---

**Baseline Status:** 🔄 Under Collection (2026-09-22)  
**Validation:** 🔄 In Progress (Phase 5 hardening)  
**Release Status:** 🔄 Production-candidate (Phase 3-4 complete; Phase 5-6 in progress)

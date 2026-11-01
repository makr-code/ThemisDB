# Scraper Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Current Status

The source implements seed collection, fetch/render selection, HTML search/API processing, evaluation, and record writing. This roadmap is a planning artifact; source presence and focused unit tests do not by themselves establish production readiness or successful runtime validation.

## Present in source (not evidence of end-to-end integration)

- [x] Diagnostic event/sink types and fault-classification helpers exist — evidence: `include/scraper/scraper_diagnostics.h`; `ScraperPlugin` does not emit these events.
- [x] Token-bucket controller can be injected and applied once per seed — evidence: `include/scraper/scraper_burst_controller.h`, `src/scraper/scraper_plugin.cpp`.
- [x] Benchmark targets cover helper/type paths — evidence: `benchmarks/scraper/`; they do not measure end-to-end extraction or scraping throughput.

## Planned Features

### Short-term (3-6 months)
- [x] add token-bucket acquisition/utilization/reset behavior and focused unit tests (Target: Q4 2026) — evidence: `include/scraper/scraper_burst_controller.h`, `tests/scraper/test_scraper_burst_hardening_focused.cpp` (SCR-17..20); this does not establish sustained multi-source runtime reliability.
- [x] add unit coverage for render-contract classification and pagination-limit helper boundaries (Target: Q4 2026) — evidence: `include/scraper/scraper_render_contract.h`, `tests/scraper/test_scraper_render_pagination_focused.cpp` (SCR-21..24); these helpers are not wired into the plugin runtime.
- [x] add tests for standalone run-summary collection (Target: Q4 2026) — evidence: `include/scraper/scraper_run_summary.h`, `tests/scraper/test_scraper_run_summary_focused.cpp` (SCR-25..28); plugin diagnostics are not connected to the collector.

### Mid-term (6-12 months)
- [ ] Establish measured p95/p99 baselines for end-to-end scraping and writes (Target: Q1 2027) — TODO: current release-gate cases measure helper paths only; no baseline result is documented.
- [ ] Add scraper-native fetch/parse/evaluate/write benchmark workloads (Target: Q1 2027) — TODO: current pipeline-depth cases exercise diagnostics, summary, request allocation, and classification helpers.
- [ ] Validate long-run scraper runtime reliability (Target: Q1 2027) — the integration soak test uses in-process fetch/queue stubs, not `ScraperPlugin` or durable storage; runtime coverage remains unverified.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze scrape pipeline and writer contracts for current major line (Target: Q3 2026) — evidence: include/scraper/scraper_api_contract.h
- [x] define explicit error taxonomy for scraper failure classes (Target: Q3 2026) — evidence: include/scraper/scraper_api_contract.h

### Phase 2: Core Implementation
- [x] define fetch/search/evaluator interfaces and failure/result types (Target: Q4 2026) — evidence: relevant `include/scraper/` headers. TODO: verify all runtime paths against these contracts; the diagnostics helpers alone do not provide runtime enforcement.
- [ ] Align metadata-writer behavior to bounded runtime contracts (Target: Q4 2026) — TODO: source declares writer result fields; no end-to-end write deadline or plugin-emitted write diagnostic is implemented.

### Phase 3: Error Handling and Edge Cases
- [x] provide fault-classification helpers and a sink interface (Target: Q4 2026) — evidence: `include/scraper/scraper_diagnostics.h`.
- [ ] Wire consistent diagnostics through fetch, parse, evaluator, and writer paths (Target: Q4 2026) — TODO: `ScraperPlugin` currently does not emit `ScraperDiagnosticEvent`.

### Phase 4: Tests
- [x] add focused tests for API contract value types and fail-closed predicates (Target: Q4 2026) — evidence: `tests/scraper/test_scraper_contract_hardening_focused.cpp`; these are not malformed-source integration tests.
- [x] add focused token-bucket and helper tests (Target: Q4 2026) — evidence: `tests/scraper/test_scraper_burst_hardening_focused.cpp`, `tests/scraper/test_scraper_render_pagination_focused.cpp`; these do not test end-to-end extraction workloads.

### Phase 5: Performance and Hardening
- [x] add source-declared microbenchmark cases for helper/type paths (Target: Q4 2026) — evidence: `benchmarks/scraper/bench_scraper_release_gates.cpp`; these are not extraction or processor-path benchmarks.
- [ ] Validate p95/p99 and throughput behavior against representative release baselines (Target: Q4 2026) — TODO: benchmark source labels are not measured results; current cases do not benchmark the scrape hot path.

### Phase 6: Documentation and Acceptance
- [x] source review of core scraper module docs; human review and runtime validation remain separate
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core scraper surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] Remaining fetch/parse/write hardening verified — TODO: helper headers and focused unit coverage do not establish end-to-end runtime behavior.
- [ ] Release benchmark stabilization verified — TODO: requires benchmark runs and an agreed representative baseline.

## Known Issues and Limitations

- runtime behavior depends on source quality, render mode, and crawl policy configuration.
- The main loop does not apply `respect_robots`, `same_domain_only`, crawl `max_depth`/`max_pages`, parsed `api_options`, `results_per_page`, or LLM `model_path`/`temperature`; diagnostics and render-timeout helpers are not wired into it.
- The integration soak source uses stubs and does not exercise the scraper runtime or durable writer.
- Dedicated end-to-end scraper benchmark workloads and measured release baselines remain TODOs.

## Breaking Changes

No breaking scraper contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `scraper`
- [x] Add a high-cardinality stress test source (Target: Q1 2027) — evidence: `tests/scraper/test_scraper_highcardinality_stress.cpp`; execution and exporter reliability are not established here.
- [ ] Validate long-duration soak coverage for the module's actual runtime paths (Target: Q1 2027) — TODO: `tests/integration/test_scraper_ingestion_soak.cpp` currently exercises in-process stubs.
- [x] Maintain a scraper ingestion runbook reference (Target: Q1 2027) — evidence: `docs/operability/RUNBOOK_SCRAPER_INGESTION.md`; log/telemetry patterns in that file are not emitted by the plugin code reviewed here.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] Establish module runtime fail-closed behavior with integration evidence (Target: Q1 2027) — TODO: helper predicates alone do not prove integration.
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027).
- [ ] Operator-critical runtime paths emit diagnostics/alerts consistent with the runbook (Target: Q1 2027) — TODO: plugin diagnostic emission is not implemented.

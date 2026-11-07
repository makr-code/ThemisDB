# Scraper Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: review | validated: source review only, 2026-11-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-11-07<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.
**Related issue / milestone:** #7450 · DOC-WEEKLY-2026-45

## Current Status

**Maturity assessment — partial implementation, not production-validated.** The module has a synchronous scraper path, configuration and API helpers, metadata writing, and focused helper tests. Code inspection identifies important controls and failure behavior that are declared or partly implemented but not consistently enforced in the runtime path. Existing test sources and helper microbenchmarks do not establish end-to-end operation, durable-write behavior, runtime safety, or performance. No tests, builds, or benchmarks were run for this roadmap update.

Planning version labels below are targets, not release commitments or dates. The root [`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for project version and release readiness: it currently identifies `v2.4.0-alpha`. `VERSIONING.md` identifies `v2.5.0` as the next minor stable path. There is no verified release date or approved `v3.0.0` schedule. The v2.5.0 plan preserves stable APIs; v3.0.0 may redesign contracts only with explicit migration documentation.

### Implementation presence vs. verification evidence

| Area | Present in source | Test/benchmark evidence and limit |
|---|---|---|
| Crawl policy and fetch | Configuration parses crawl controls; URL policy uses string-prefix patterns. `collectSeeds()` adds catalog URLs without policy validation; fetch follows redirects and ignores curl result. The synchronous main loop does not apply depth/page limits, robots, same-domain policy, cancellation, or diagnostics. | Focused tests do not establish enforcement in the live plugin path. No end-to-end SSRF/redirect or bounded-crawl evidence is documented. |
| API retrieval | API configuration and a client with pagination are present. The plugin API loop skips denied results but permits the exact endpoint; HTML request delay is not applied to API requests. Pagination stops on fetch/parse failures; the client follows redirects, uses a fixed 30-second timeout, and has no evident response-size cap. | No documented integration evidence for request limits, retry/error semantics, or large/failed responses. |
| Robots and sitemap | Helper source files exist. Robots handling is simplified and treats network error/empty results as allow-all. Neither helper is included in `src/scraper/CMakeLists.txt`, so they are not part of the normal plugin target. | Standalone helper presence is not runtime integration or standards conformance. |
| Diagnostics, cancellation, writes | Diagnostic types/helpers and writer result types exist. Plugin diagnostic emission and cancellation are not wired through the synchronous loop; writer flush return is ignored. | Contract declarations in `include/scraper/scraper_api_contract.h` are stronger than demonstrated plugin behavior. No durable-writer runtime soak evidence. |
| Tests and performance | Focused tests are auto-registered by `tests/scraper/CMakeLists.txt`; helper/type microbenchmarks are present. | Integration soak and high-cardinality stress sources use stubs. Benchmark thresholds are labels, not pass/fail results or measured percentiles; no end-to-end scrape baseline exists. |

### Research findings mapped to current gaps

Research supports design choices and validation criteria; it is not evidence that the scraper already implements them.

| Source | Relevant finding | Current gap / roadmap implication |
|---|---|---|
| [R1] RFC 9309, Robots Exclusion Protocol | Defines robots parsing and availability/error behavior; robots rules are not authorization. 4xx unavailable and network/5xx unreachable cases are distinct. | Replace allow-all-on-network-error behavior only through an explicit, documented policy decision; parse and cache according to the chosen RFC behavior and test each response class. |
| [R2] OWASP SSRF Prevention Cheat Sheet | String URL checks alone are not a complete boundary; allowlisting and careful handling of DNS resolution and redirects are important. | The current prefix policy and redirect-following fetch need a threat model and enforceable destination checks across seed, resolved address, and redirect handling. |
| [R3] Heydon & Najork, “Mercator” (1999) | Provides foundational rationale for modular crawler/frontier/scheduling architecture. | Consider a bounded scheduler/frontier only if measured workload justifies it; this paper is architectural context, not current throughput evidence. |
| [R4] Google SRE Workbook, “Monitoring Systems with Advanced Analytics” | Operational comparisons should use structured signals and latency distributions/percentiles, not unlabeled averages. | Instrument the runtime and report measured p50/p95/p99, throughput, failures, and resource use against a recorded baseline. |
| [R5] ISO 28500:2017 (WARC) | Specifies a web-archive file format. | Evaluate WARC only if lawful archival capture is a confirmed requirement; no archive feature is claimed here. |
| [R6] RFC 9110, HTTP Semantics | Establishes HTTP response and redirect semantics. | Make status, redirect, timeout, and retry outcomes explicit and testable rather than silently continuing or treating all outcomes alike. |

## Present in source (not evidence of end-to-end integration)

- [x] Diagnostic event/sink types and fault-classification helpers exist — evidence: `include/scraper/scraper_diagnostics.h`; `ScraperPlugin` does not emit these events.
- [x] Token-bucket controller can be injected and applied once per seed — evidence: `include/scraper/scraper_burst_controller.h`, `src/scraper/scraper_plugin.cpp`; this does not demonstrate per-host/API pacing.
- [x] Benchmark targets cover helper/type paths — evidence: `benchmarks/scraper/`; they do not measure end-to-end extraction or scraping throughput.
- [x] Configuration parses crawl and API controls — evidence: `src/scraper/scraper_config.cpp`; parsing does not establish that the main loop enforces those controls.

## Planned Features

Version targets are planning lanes, not schedule or release promises. Effort estimates are indicative person-weeks (PW), with ±50% uncertainty and subject to external policy/legal review, workload definition, integration constraints, and benchmark environment.

### v2.5.0 — operational safety (stable API compatibility)

#### Wave 1 — Contract, threat model, and policy decisions

- **Goal:** Agree on observable crawl, egress, robots, API, error, and data-handling behavior before implementation.
- **Tasks:**
  - [ ] Document threat boundaries and allowed schemes/hosts/ports, DNS/address handling, redirect policy, and private/link-local destination behavior (Target: v2.5.0).
  - [ ] Decide robots behavior for success, 4xx unavailable, network/5xx unreachable, parse errors, caching, and user-agent (Target: v2.5.0; use [R1], without treating robots as authorization).
  - [ ] Specify request/page/depth/response-size/time limits, API pacing, retry rules, cancellation, flush/error outcomes, and compatibility constraints (Target: v2.5.0).
  - [ ] Obtain legal/privacy approval for source use, retention, and licensing assumptions; decide whether archival capture is in scope (Target: v2.5.0).
- **Dependencies:** Product/operator workload and allowed-source policy; security review; legal/privacy owner for source-use and retention decisions.
- **Risks/trade-offs:** Conservative fail-closed behavior may reduce crawl availability; broad or implicit allow rules weaken egress controls. Robots policy is not access authorization.
- **Measurable KPIs:** Reviewed decision record covers 100% of listed policy decisions; each runtime control has a named owner, default, and testable acceptance criterion.
- **Validation:** Human security and legal/privacy review of the decision record and API compatibility review. No implementation completion is implied by approval.
- **Indicative effort:** 1–2 PW, excluding review queue time.

#### Wave 2 — Enforce bounded crawl, egress, robots, domain, and API controls

- **Goal:** Apply approved policy to every outbound request and bound the work and resource consumed by one crawl.
- **Tasks:**
  - [ ] Enforce seed/catalog URL validation, destination checks, redirect revalidation, and approved egress policy (Target: v2.5.0; [R2], [R6]).
  - [ ] Wire page/depth limits, same-domain policy, cancellation, response-size/time bounds, and API-specific pacing/endpoint rules into the plugin runtime (Target: v2.5.0).
  - [ ] Integrate robots behavior into the normal target or remove unsupported claims; test the approved status/error/cache policy (Target: v2.5.0; [R1]).
  - [ ] Decide whether sitemap support is required and, if so, integrate and constrain it under the same crawl policy (Target: v2.5.0).
- **Dependencies:** Wave 1 decisions; implementation review of libcurl and plugin request boundaries.
- **Risks/trade-offs:** DNS rebinding, redirect chains, proxy behavior, and changes between validation and connection can complicate destination enforcement. Tight response/work limits can reject legitimate large sources.
- **Measurable KPIs:** 100% of outbound request paths pass the common policy gate; configured page/depth/byte/time ceilings are never exceeded in deterministic tests; denied destinations receive no HTTP request.
- **Validation:** Deterministic local HTTP tests cover seed and catalog URLs, redirects, DNS/address cases, robots response classes, limits, API pacing, and cancellation; inspect target linkage to verify required helpers are in the normal build.
- **Indicative effort:** 3–6 PW, excluding environment-specific egress review.

#### Wave 3 — Explicit diagnostics, errors, retries, flush, and cancellation

- **Goal:** Make each fetch/parse/evaluate/write outcome observable and ensure failures are not silently discarded.
- **Tasks:**
  - [ ] Emit structured diagnostics for request, parse, evaluation, and writer outcomes with stable error categories (Target: v2.5.0; [R4], [R6]).
  - [ ] Define bounded retry/backoff behavior by failure class and surface exhausted retries and HTTP outcomes to callers (Target: v2.5.0).
  - [ ] Propagate cancellation and deadlines through active work; check and report writer flush/close results (Target: v2.5.0).
  - [ ] Align implementation behavior with documented API contract declarations, or narrow the declarations with a compatibility-reviewed change (Target: v2.5.0).
- **Dependencies:** Wave 1 failure contract; Wave 2 request/cancellation boundaries; writer API behavior.
- **Risks/trade-offs:** Retries can amplify load or duplicate writes; diagnostics can expose URLs or content unless redaction and retention are specified.
- **Measurable KPIs:** Every injected fetch/parse/evaluate/write failure yields one classified terminal outcome; cancellation completes within the configured deadline plus documented cleanup allowance; flush failures are returned/reported, never silently counted as success.
- **Validation:** Deterministic fault-injection tests cover each failure class, retry ceiling, cancellation, redaction, and failed flush; compare emitted event counts with operation outcomes.
- **Indicative effort:** 2–4 PW.

#### Wave 4 — End-to-end evidence, baseline, and runtime soak

- **Goal:** Establish reproducible runtime correctness, resource, and performance evidence using the actual plugin and writer.
- **Tasks:**
  - [ ] Add deterministic local-server tests using the real plugin request path and a verifiable writer (Target: v2.5.0).
  - [ ] Add scraper-native fetch/parse/evaluate/write benchmarks and publish reproducible environment, workload, p50/p95/p99, throughput, error rate, and RSS results (Target: v2.5.0; [R4]).
  - [ ] Run a representative runtime soak with durable writes, bounded resources, and restart/failure scenarios; publish results and limitations (Target: v2.5.0).
- **Dependencies:** Waves 2–3; representative workload, test environment, durable writer, and benchmark hardware.
- **Risks/trade-offs:** Results may vary by source and network; a synthetic local workload cannot establish performance on all external sites.
- **Measurable KPIs:** Baseline includes p50/p95/p99 latency, documents/s, error counts, peak RSS, and resource-limit outcomes. Soak duration and workload must be agreed in Wave 1; all acknowledged writes are accounted for, with zero unexplained loss or duplicate records under the tested contract.
- **Validation:** Review and archive raw benchmark/soak results; rerun deterministic cases in CI. Compare future changes against the recorded same-hardware baseline; no numeric performance claim is made before measurements exist.
- **Indicative effort:** 3–5 PW, excluding soak wall-clock and infrastructure provisioning.

**Indicative v2.5.0 total:** 9–17 PW plus external review and environment/soak time.

### v3.0.0 — strategic scaling and durability (planning only)

#### Wave 1 — Workload gate, async/frontier contract, and migration

- **Goal:** Decide from v2.5.0 evidence whether an asynchronous bounded scheduler/frontier is warranted; define a migration-safe v3 contract if so.
- **Tasks:**
  - [ ] Compare representative v2 workload, latency, throughput, and resource data against operator requirements; record a go/no-go for scheduler redesign (Target: v3.0.0).
  - [ ] If justified, specify bounded queue/frontier semantics, worker lifecycle, cancellation, ordering, error propagation, and API migration path (Target: v3.0.0; [R3]).
  - [ ] Publish migration guide and compatibility/deprecation plan before changing public contracts (Target: v3.0.0).
- **Dependencies:** Measured v2.5.0 baseline; workload owners; API and release planning.
- **Risks/trade-offs:** Async design adds lifecycle and backpressure complexity; redesign without measured demand increases maintenance cost.
- **Measurable KPIs:** Approved go/no-go based on named workload thresholds; if proceeding, every changed public contract has migration guidance and compatibility tests before implementation.
- **Validation:** Architecture review, workload review, and migration-document review; no async redesign is presumed.
- **Indicative effort:** 2–4 PW for decision/specification; implementation estimate only after design.

#### Wave 2 — Bounded scheduler and per-host fairness

- **Goal:** If Wave 1 approves redesign, scale concurrency without unbounded queues or violating per-host policy.
- **Tasks:**
  - [ ] Implement bounded frontier/queue and worker limits with explicit backpressure and shutdown behavior (Target: v3.0.0).
  - [ ] Add per-host fair concurrency and pacing controls, with policy checks on every request and redirect (Target: v3.0.0).
  - [ ] Add deterministic scheduler tests for queue saturation, fairness, cancellation, and host isolation (Target: v3.0.0).
- **Dependencies:** Wave 1 go decision and contract; v2.5.0 safety controls.
- **Risks/trade-offs:** Fairness can reduce peak throughput; queue bounds and host identity semantics need careful definition behind proxies and redirects.
- **Measurable KPIs:** Queue and worker counts never exceed configured bounds; configured per-host concurrency is never exceeded; under the agreed multi-host workload, no eligible host is starved beyond the specified fairness window.
- **Validation:** Deterministic multi-host simulation plus stress test; measure queue depth, per-host wait distributions, throughput, and RSS.
- **Indicative effort:** 5–9 PW, conditional on Wave 1.

#### Wave 3 — Durable checkpoint, restart, deduplication, and idempotent writes

- **Goal:** Define and implement recoverable progress with explicit acknowledgement and duplicate semantics.
- **Tasks:**
  - [ ] Specify checkpoint durability and acknowledgement boundary, deduplication key, retry, and idempotent-write contract (Target: v3.0.0).
  - [ ] Implement checkpoint/restart and bounded deduplication using the agreed storage contract (Target: v3.0.0).
  - [ ] Test crashes/failures at each checkpoint/write boundary and document operator recovery (Target: v3.0.0).
- **Dependencies:** Approved storage/retention model; Wave 1 contract; durable writer capabilities.
- **Risks/trade-offs:** Exactly-once guarantees may not be achievable across external sources and storage; checkpoint retention adds storage and migration obligations.
- **Measurable KPIs:** Zero loss of acknowledged checkpoint work in the defined fault matrix; duplicate writes remain within the documented idempotency contract; restart completes within an agreed recovery objective.
- **Validation:** Fault-injection restart tests with durable storage and reconciliation of source items, checkpoints, and committed writes.
- **Indicative effort:** 4–8 PW, depending on writer and storage support.

#### Wave 4 — Representative scaling, chaos/soak, and data-governance decision

- **Goal:** Validate the chosen v3 architecture under representative multi-domain load and close archival/provenance policy questions.
- **Tasks:**
  - [ ] Run comparative scaling tests against the measured v2 baseline at the same per-host policy; evaluate the proposed target of at least 2x documents/s with 4x workers, subject to workload and bottleneck findings (Target: v3.0.0; target is proposed, not measured).
  - [ ] Run multi-domain chaos and long-duration soak tests for bounded queues/RSS, fairness, restart, and durable writes (Target: v3.0.0).
  - [ ] Obtain legal/privacy review of source use, provenance, retention, licensing, and any archival-capture requirement; evaluate WARC only if lawful archival capture is required (Target: v3.0.0; [R5]).
- **Dependencies:** Waves 2–3; agreed representative workload, baseline hardware, durable environment, and legal/privacy review.
- **Risks/trade-offs:** External sites and network conditions reduce repeatability; performance targets may need revision after baseline comparison. Provenance metadata does not itself settle lawful use or retention.
- **Measurable KPIs:** Compare p50/p95/p99, docs/s, error rate, peak RSS, queue bounds, per-host fairness, recovery time, and acknowledged-work reconciliation. Proposed scaling target is ≥2x docs/s with 4x workers at the same per-host policy; queue/RSS bounds and zero loss of acknowledged checkpoint work are required for the tested contract.
- **Validation:** Publish repeatable test setup and raw results; run controlled scaling, fault, and soak cases; record legal/privacy decision separately from technical test evidence.
- **Indicative effort:** 4–8 PW, excluding infrastructure, soak duration, and external review.

**Indicative v3.0.0 total:** 15–29 PW if all conditional work proceeds; not a schedule commitment.

### Short-term (3-6 months)
- [x] add token-bucket acquisition/utilization/reset behavior and focused unit tests (historical target: Q4 2026) — evidence: `include/scraper/scraper_burst_controller.h`, `tests/scraper/test_scraper_burst_hardening_focused.cpp` (SCR-17..20); does not establish sustained multi-source runtime reliability.
- [x] add unit coverage for render-contract classification and pagination-limit helper boundaries (historical target: Q4 2026) — evidence: `include/scraper/scraper_render_contract.h`, `tests/scraper/test_scraper_render_pagination_focused.cpp` (SCR-21..24); helpers are not wired into the plugin runtime.
- [x] add tests for standalone run-summary collection (historical target: Q4 2026) — evidence: `include/scraper/scraper_run_summary.h`, `tests/scraper/test_scraper_run_summary_focused.cpp` (SCR-25..28); plugin diagnostics are not connected to the collector.

### Mid-term (6-12 months)
- [ ] Establish measured p95/p99 baselines for end-to-end scraping and writes — no documented end-to-end baseline; see v2.5.0 Wave 4.
- [ ] Add scraper-native fetch/parse/evaluate/write benchmark workloads — current benchmark cases exercise helpers, not the scrape hot path.
- [ ] Validate long-run scraper runtime reliability — integration soak uses in-process fetch/queue stubs, not `ScraperPlugin` or durable storage.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] provide declared scrape pipeline and writer contract types for the current major line — evidence: `include/scraper/scraper_api_contract.h`; declaration is not proof of implementation conformance.
- [x] provide declared failure/result types — evidence: `include/scraper/scraper_api_contract.h`; runtime behavior remains to be aligned.

### Phase 2: Core Implementation
- [x] define fetch/search/evaluator interfaces and failure/result types — evidence: relevant `include/scraper/` headers. Runtime paths have not been verified against all declared contracts.
- [ ] Align metadata-writer behavior to bounded runtime contracts — writer result fields exist; no demonstrated end-to-end write deadline or plugin-emitted write diagnostic.

### Phase 3: Error Handling and Edge Cases
- [x] provide fault-classification helpers and a sink interface — evidence: `include/scraper/scraper_diagnostics.h`.
- [ ] Wire consistent diagnostics through fetch, parse, evaluator, and writer paths — `ScraperPlugin` does not currently emit `ScraperDiagnosticEvent`.

### Phase 4: Tests
- [x] add focused tests for API contract value types and fail-closed predicates — evidence: `tests/scraper/test_scraper_contract_hardening_focused.cpp`; not malformed-source integration tests.
- [x] add focused token-bucket and helper tests — evidence: `tests/scraper/test_scraper_burst_hardening_focused.cpp`, `tests/scraper/test_scraper_render_pagination_focused.cpp`; not end-to-end extraction tests.

### Phase 5: Performance and Hardening
- [x] add source-declared microbenchmark cases for helper/type paths — evidence: `benchmarks/scraper/bench_scraper_release_gates.cpp`; not extraction or processor-path benchmarks.
- [ ] Validate p95/p99 and throughput against representative release baselines — labels in benchmark sources are not measured results; current cases do not benchmark the scrape hot path.

### Phase 6: Documentation and Acceptance
- [x] source review of core scraper module docs; human review and runtime validation remain separate.
- [x] roadmap/future planning separated from historical changelog entries.

## Production Readiness Checklist

- [x] Core scraper surfaces and source evidence documented; this is not a production-readiness approval.
- [x] Known module failure, policy, and integration gaps recorded in this roadmap.
- [x] Helper benchmark scope and evidence limitations documented.
- [ ] Remaining fetch/parse/write hardening verified with runtime evidence.
- [ ] Release benchmark stabilization verified against an agreed representative baseline.
- [ ] Legal/privacy and source-use policy approved; provenance metadata alone is not approval.

## Known Issues and Limitations

- Runtime behavior depends on source quality, render mode, and crawl policy configuration.
- `collectSeeds()` does not policy-validate catalog URLs; the plugin main loop does not apply `respect_robots`, `same_domain_only`, crawl `max_depth`/`max_pages`, cancellation, or diagnostics.
- URL policy is string-prefix pattern matching, not resolved-IP or redirect egress enforcement. HTTP fetch follows redirects and ignores the curl result.
- HTML request delay is not applied to API requests; the API loop skips denied results but permits the exact endpoint. API pagination stops on fetch/parse failures; the client follows redirects, uses a fixed 30-second timeout, and has no evident response-size cap.
- Robots and sitemap helper sources are omitted from `src/scraper/CMakeLists.txt`; robots helper network-error/empty-result behavior is allow-all and parsing is simplified.
- Writer flush return is ignored. Contract declarations in `include/scraper/scraper_api_contract.h` are stronger than demonstrated plugin behavior.
- Integration soak and high-cardinality stress sources use stubs; no durable-writer runtime soak or measured end-to-end benchmark baseline is documented.
- Metadata writer provenance fields exist, but source-use, retention, and licensing decisions have not been established. This roadmap makes no legal/privacy compliance claim.

## Risks, Assumptions, and Knowledge Gaps

- The v2.5.0 and v3.0.0 labels are planning targets only. Root release documents govern current version and readiness; no v3 date or approved schedule is known.
- Assumes maintainers can define allowed-source and egress policy and provide a representative workload, local test server, durable writer, and benchmark environment.
- Legal/privacy approval is an external blocking decision for permitted sources, collection purpose, retention, licensing, and whether archival capture is required.
- DNS resolution, proxies, TLS, redirects, and connection timing may require controls below the current URL-string policy; implementation feasibility and platform behavior need security review.
- Desired robots behavior, cache duration, user-agent identity, treatment of nonstandard responses, and whether sitemap ingestion is required remain policy decisions.
- The appropriate retry/idempotency and checkpoint guarantees depend on writer/storage semantics and have not been verified.
- The architecture should remain synchronous unless workload evidence justifies an async bounded frontier. Mercator is background, not proof of a performance need.
- Proposed v3 scaling targets are hypotheses to compare with a measured v2 baseline, not achieved results or universal SLOs.

## Breaking Changes

The v2.5.0 plan preserves stable APIs. A v3.0.0 contract redesign is conditional on an approved architecture decision and requires migration notes, compatibility/deprecation planning, and a changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027, subject to program-level approval and validation.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `scraper`
- [x] Add a high-cardinality stress test source (Target: Q1 2027) — evidence: `tests/scraper/test_scraper_highcardinality_stress.cpp`; it uses stub fetch/rate-limiter paths, and execution/exporter reliability are not established.
- [ ] Validate long-duration soak coverage for the module's actual runtime paths (Target: Q1 2027) — `tests/integration/test_scraper_ingestion_soak.cpp` currently exercises in-process stubs, not the plugin and durable writer.
- [x] Maintain a scraper ingestion runbook reference (Target: Q1 2027) — evidence: `docs/operability/RUNBOOK_SCRAPER_INGESTION.md`; log/telemetry patterns in that file are not emitted by reviewed plugin code.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing).
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027); no measured runtime baseline is currently documented.
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] Establish module runtime fail-closed behavior with integration evidence (Target: Q1 2027) — helper predicates alone do not prove integration.
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027).
- [ ] Operator-critical runtime paths emit diagnostics/alerts consistent with the runbook (Target: Q1 2027) — plugin diagnostic emission is not implemented.

## Research References

- [R1] IETF, [RFC 9309: Robots Exclusion Protocol](https://www.rfc-editor.org/rfc/rfc9309), 2022.
- [R2] OWASP, [Server-Side Request Forgery Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Server_Side_Request_Forgery_Prevention_Cheat_Sheet.html).
- [R3] Heydon & Najork, [“Mercator: A scalable, extensible Web crawler,” World Wide Web 2, 219–229 (1999)](https://doi.org/10.1023/A:1019213109274).
- [R4] Google SRE Workbook, [“Monitoring Systems with Advanced Analytics”](https://sre.google/workbook/monitoring/).
- [R5] ISO 28500:2017, [WARC file format](https://www.iso.org/obp/ui/en/#!iso:std:68004:en).
- [R6] IETF, [RFC 9110: HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110).

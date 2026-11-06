# Content Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · PERFORMANCE_EXPECTATIONS.md -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
-->

## Current Status

**Maturity assessment (source-checked 2026-11-06): substantial implementation; release evidence incomplete.** The checkout contains manager, processor, validation/security, enrichment, versioning, streaming, and asynchronous-processing code, along with focused test sources. This establishes implementation surfaces, not complete end-to-end format support, test pass status, production quality, or universal performance. `ContentManager`'s current direct routes are narrower than the public processor inventory; see [architecture](ARCHITECTURE.md) and [module overview](README.md). Optional processors, policies, and external dependencies affect behavior.

The [module README](README.md) and [architecture](ARCHITECTURE.md) explicitly leave several public processor integrations unverified: public class/header presence is not proof of end-to-end manager, server, or plugin support. [RESEARCH.md](RESEARCH.md) further records that no corpus, labeling protocol, semantic-quality score, or reproducible performance result is established. Accordingly, this roadmap makes no extraction or embedding quality claim; use-case acceptance must first trace the actual integration path and, before any quality claim, use an approved task-specific labeled evaluation protocol and reportable results (or mark quality unevaluated).

The repository [`VERSION`](../../VERSION) is `2.4.0`; root [`ROADMAP.md`](../../ROADMAP.md) identifies `2.4.0-alpha`. [`VERSIONING.md`](../../VERSIONING.md) calls `2.5.0` the next planned MINOR; it does not schedule a `3.0.0` release. The v3.0.0 section below is strategic and provisional, not a release commitment. The module has no checked-in benchmark result/hardware manifest under `benchmarks/content/`. The CMake file registers `bench_content_processor_paths` (synthetic work), `bench_content_versioning`, and `bench_content_release_gates` (API value/enum microbenchmarks); `bench_text_extraction.cpp` is present but unregistered. Therefore, documented thresholds in [performance expectations](PERFORMANCE_EXPECTATIONS.md) are expectations, not measured results.

The historical [Wave A](WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md), [Wave B](WAVE_B_CLOSURE_EVIDENCE_BUNDLE.md), [Wave C](WAVE_C_CLOSURE_EVIDENCE_BUNDLE.md), and [Wave D](WAVE_D_CLOSURE_EVIDENCE_BUNDLE.md) closure bundles report completion, but this roadmap does not treat those reports as independently revalidated release evidence. Some test names cited in those bundles are absent from the current `tests/content/` inventory; the present CMake registration glob creates focused targets with the `content` label, while the separate soak target is `not_release_critical`. Registration and source presence do not establish that tests ran or passed. Reconcile those records against current code and CI artifacts before using them for release promotion.

### Batch 5 / CMT-7504, CMT-7505, CMT-7506 — Historical references only

These pointers preserve historical tracking context only; they do not assert current completion, verified outcomes, or release sign-off. Recheck each record against current source and CI evidence before relying on it:

- **CMT-7504:** [documentation linkset synchronization record](CMT-7504-DOCUMENTATION_SYNC.md).
- **CMT-7505:** [test-coverage correlation report](CMT-7505-TEST_COVERAGE_CORRELATION.md); its estimated mapping is not a measured test-coverage or execution result.
- **CMT-7506:** [v2.4.0 GA promotion sign-off tracker](CMT-7506-GA_PROMOTION_SIGN_OFF.md); this historical tracker is not current release approval.

**Wave B performance evidence conflict:** the [Wave B closure bundle](WAVE_B_CLOSURE_EVIDENCE_BUNDLE.md) reports extraction and concurrent-ingestion p95/p99 gates as passed, including per-path p99 figures. The current [performance expectations](PERFORMANCE_EXPECTATIONS.md) and [benchmark inventory](../../benchmarks/content/README.md) instead show that the registered processor-path benchmark is synthetic, the other registered targets are versioning and API microbenchmarks, the extraction source is mock and unregistered, and no result manifest or representative-hardware baseline is established. The bundle's figures are therefore historical claims, **not verified measurements**, and must not be repeated as passed production gates. Register production extraction benchmark targets and re-measure on representative hardware with complete manifests before any content p95/p99 claim or performance gate can be accepted for release.

## Planned Features

- [ ] Reconcile manager/server/plugin format routes with the public processor inventory and define which paths are required versus optional (v2.5.0, Wave 1).
- [ ] Replace benchmark-source/expectation ambiguity with measured, reproducible production-path evidence before claiming content performance gates (v2.5.0, Wave 2).
- [ ] Revalidate security, soak, and Wave A–D completion evidence against current tests and CI before module readiness sign-off (v2.5.0, Wave 3).
- [ ] Establish a reviewed public API and migration plan before selecting any v3.0.0 breaking changes.

### v2.5.0 Plan

#### Wave 1 — Integration and behavior contract

- **Goal:** Establish an evidence-backed statement of which content paths are supported end to end; retain the current API unless an approved issue requires change.
- **Tasks:**
  - [ ] Trace required format routes from caller/server/plugin through manager dispatch and persistence; record optional dependency/feature-flag conditions.
  - [ ] Define acceptance for each selected application/use case from the traced input-to-output path; before making an extraction or embedding quality claim, use an approved task-specific labeled corpus, protocol, and metric, otherwise state that quality is unevaluated.
  - [ ] Add or refine focused integration regressions for each agreed required route, stream lifetime/buffering boundary, and structured failure path.
  - [ ] Update route and limitation documentation only after code and tests support the behavior.
- **Dependencies:** Content/API owner chooses required formats and deployments; optional processor dependencies are available in the chosen CI configuration.
- **Risk / trade-off:** Broadening routing may change behavior for callers who currently receive stored metadata or a structured failure; keep scope to explicitly approved routes.
- **Measurable success:** 100% of the agreed required routes and use cases have a named source path and focused test exercising that path; no public inventory item is described as end-to-end support solely because a class/header exists. Every extraction/embedding quality claim has an approved task-specific labeled corpus, protocol, metric, and recorded result; otherwise it is explicitly marked unevaluated.
- **Validation:** **Tests:** run focused content tests plus the project release-profile content selection; demonstrate the actual manager/caller path, not helper-only mirrors. **Bench:** no performance claim in this wave without an approved workload. **Quality evaluation:** define task-specific labels/metrics and record results before any quality statement; otherwise report quality as unevaluated. **Security:** cover MIME/size/policy rejection and ensure failures do not fall through to a different processor. **Governance:** review use-case acceptance, corpus approval, API compatibility, and changelog requirements for any behavior change.
- **Effort:** 2–4 engineer-weeks, assuming a bounded required-format list and existing CI dependencies.
- **Unknown assumptions:** Which processor families are contractual for 2.5.0; whether the caller/plugin paths outside `ContentManager` are in release scope; which optional libraries are available on supported CI platforms.

#### Wave 2 — Production-path benchmark evidence

- **Goal:** Make the selected content performance gates reproducible and representative of production code rather than synthetic or mock work.
- **Tasks:**
  - [ ] Select release-critical workloads and connect registered benchmarks to the corresponding production processor/ingestion paths.
  - [ ] Register production extraction benchmark targets for the selected release-gated paths; synthetic processor loops and API/versioning microbenchmarks do not satisfy this prerequisite.
  - [ ] Define fixture provenance, build configuration, hardware/environment manifest, warm-up/repetition method, and percentile reporting.
  - [ ] Record reviewed baselines and apply the existing regression policy only to cases with complete, reproducible results; update benchmark mappings.
- **Dependencies:** Wave 1 scope, stable benchmark fixtures, representative hardware, and agreement on which performance expectations are release-blocking.
- **Risk / trade-off:** Production dependencies and hardware variability increase benchmark cost; synthetic microbenchmarks remain useful for isolated code but cannot stand in for end-to-end extraction.
- **Measurable success:** Every release-gated extraction case has a registered benchmark that invokes its production path, an archived representative-hardware run manifest with all mapped cases present, and a reviewed baseline; any stated regression limit is evaluated against that matched baseline. No historical Wave B figure or numeric latency claim is accepted without those artifacts.
- **Validation:** **Tests:** focused correctness tests for benchmarked paths pass before measurement. **Bench:** build and run each selected target in release profile on representative hardware; archive raw output and environment metadata. **Security:** verify benchmark fixtures are approved and contain no sensitive production payloads. **Governance:** performance owner approves mappings, thresholds, and evidence retention; update `PERFORMANCE_EXPECTATIONS.md` in the same implementation change.
- **Effort:** 2–5 engineer-weeks, excluding hardware procurement/queue time and repeated baseline capture.
- **Unknown assumptions:** Representative hardware and CI availability; corpus licensing/fixture approval; whether current CTG-1/CTG-2 thresholds remain suitable after measurement.

#### Wave 3 — Security, operability, and release evidence

- **Goal:** Reconcile the historical Wave A–D claims with executable current evidence and close only the release-critical gaps confirmed by that review.
- **Tasks:**
  - [ ] Map each claimed security, failure, soak, observability, and runbook criterion to current implementation, test source, registration, and a dated passing CI artifact.
  - [ ] Replace stale/missing evidence references and classify simulations, helper tests, non-release-critical soak tests, and unverified claims accurately.
  - [ ] Obtain content-owner, security, operations, and release sign-off against the repository release checklist.
- **Dependencies:** Waves 1–2 evidence; current CI artifacts; maintainers who own the security and operations gates.
- **Risk / trade-off:** Evidence reconciliation may downgrade historical completion claims or expose implementation work beyond the 2.5.0 window; do not retain a green status just to meet a date.
- **Measurable success:** 100% of content release checklist items are either linked to current source plus passing evidence or explicitly tracked as open with an owner; zero unresolved critical/high content security findings at sign-off; no unsupported performance or end-to-end claims remain.
- **Validation:** **Tests:** release-profile content test report includes executed/pass/fail/skip counts; soak evidence is labeled with its actual scope and gate status. **Bench:** attach Wave 2 manifests or explicitly mark unmeasured cases as non-gates. **Security:** review MIME, size, archive, policy, dependency-degradation, and diagnostic paths against the content threat model. **Governance:** approval and evidence links recorded in the release sign-off; follow root `ROADMAP.md`, `RELEASE_STRATEGY.md`, and `VERSIONING.md`.
- **Effort:** 1–3 engineer-weeks after source/CI evidence is accessible; remediation discovered in the audit is additional.
- **Unknown assumptions:** Availability and retention of historical CI artifacts; current security review criteria; whether all existing runbook and telemetry references remain valid.

### v3.0.0 Strategic Plan — Provisional, Not Scheduled

This is forward-looking module planning only. It is not a committed product release or date; promote it to a release plan only through root roadmap and release-governance approval.

#### Wave 1 — Major-version API and migration design

- **Goal:** Decide whether a major-version content API change is needed and, if so, make its compatibility and migration contract explicit before implementation.
- **Tasks:**
  - [ ] Inventory public `include/content/` APIs, serialized configuration forms, error/result contracts, and downstream usage.
  - [ ] Propose only issue-backed breaking changes; document old/new behavior, migration examples, deprecation sequence, and rollback constraints.
  - [ ] Add contract/serialization compatibility tests for supported configuration forms and migration fixtures.
- **Dependencies:** v2.5.0 route/evidence inventory; API owners and downstream consumer input; root versioning policy.
- **Risk / trade-off:** A wider API redesign increases migration burden and release risk; retain compatible interfaces unless evidence justifies a break.
- **Measurable success:** 100% of approved public breaking changes have an API entry, before/after migration note, compatibility test, and named consumer impact review; no unreviewed break is included.
- **Validation:** **Tests:** compile/API contract and serialization compatibility tests pass for supported toolchains. **Bench:** compare only affected paths with v2.5.0 baselines; no speedup is presumed. **Security:** threat-model changes to trust boundaries, payload handling, and configuration. **Governance:** versioning/semver review and explicit maintainer approval before implementation.
- **Effort:** 2–4 engineer-weeks for inventory/design/review, excluding implementation and downstream migrations.
- **Unknown assumptions:** Whether any breaking change is required; supported ABI/API guarantees; downstream projects and platform/compiler matrix.

#### Wave 2 — Approved integration and resource contracts

- **Goal:** Implement only the v3.0.0 changes approved in Wave 1, with explicit dispatch, optional-dependency behavior, bounded resource use, and observable failures.
- **Tasks:**
  - [ ] Implement the approved route/API changes with explicit configuration and backward/migration behavior.
  - [ ] Add end-to-end tests for supported processor dependencies, failure/degraded modes, stream/buffer limits, and persistence/retrieval boundaries.
  - [ ] Document resource limits and operator-visible diagnostics from the actual implementation.
- **Dependencies:** Approved Wave 1 contracts; supported third-party libraries; storage/index integration test environment.
- **Risk / trade-off:** More direct processor integration can increase binary/dependency footprint and operational variability; keep optional capabilities explicit and fail safely when unavailable.
- **Measurable success:** Every newly supported contract path has end-to-end tests across success and relevant unavailable/failure modes; all configured limits have boundary tests; no silent success when a required stage fails.
- **Validation:** **Tests:** focused and integration suites on supported configurations. **Bench:** compare changed production paths with reviewed v2.5.0 baselines and publish manifests. **Security:** test untrusted input, archive/resource bounds, policy ordering, and safe diagnostics. **Governance:** API owner, security reviewer, and operations owner approve contracts and configuration defaults.
- **Effort:** 4–8 engineer-weeks, conditional on Wave 1 scope and dependency availability.
- **Unknown assumptions:** Size of the approved change set; storage/index test fixture availability; optional library support across target platforms.

#### Wave 3 — Compatibility, performance, and production acceptance

- **Goal:** Demonstrate that the major release is migratable, secure, operable, and evidence-backed before promotion.
- **Tasks:**
  - [ ] Run the documented migration/compatibility matrix against representative consumers and supported configurations.
  - [ ] Re-run release-critical tests, security review, production-path benchmarks, and the agreed sustained-load/soak suite; archive reproducible evidence.
  - [ ] Close or explicitly defer non-blocking issues, publish breaking-change notes, and complete human release sign-off.
- **Dependencies:** Waves 1–2 complete; representative CI/hardware; release candidate and migration test environment.
- **Risk / trade-off:** Full matrix and soak validation take time; unsupported platforms or optional dependencies may require an explicit support reduction rather than an unevidenced pass.
- **Measurable success:** All declared supported configurations have recorded build/test status; all release-gated benchmark cases have complete manifests and reviewed comparisons; zero open release-blocking security findings; migration and rollback guidance reviewed; all exceptions have an owner and approval.
- **Validation:** **Tests:** release-profile and integration results include pass/fail/skip counts, plus sustained-load outcomes. **Bench:** representative-hardware p95/p99 and throughput artifacts use the same workload/build method as the approved baseline. **Security:** final threat-model review and remediation evidence. **Governance:** breaking-change/changelog review, compatibility sign-off, and human GA approval under root release policy.
- **Effort:** 2–5 engineer-weeks plus hardware/soak elapsed time; remediation is additional.
- **Unknown assumptions:** Availability of representative hardware and supported consumer projects; final v3.0.0 release criteria; acceptable migration window and rollback model.

## Implementation Phases

These phases summarize current evidence and forward work; they do not reset completed implementation tasks.

### Phase 1: Design / API Contract
- [x] Public content manager, processor, and stage-configuration interfaces exist.
- [ ] Inventory actual caller-to-processor routes and agree required versus optional contracts (v2.5.0 Wave 1).
- [ ] Approve and document any v3.0.0 breaking changes and migrations (v3.0.0 Wave 1).

### Phase 2: Core Implementation
- [x] Manager, processors, validation/security, versioning, and async worker source paths are present.
- [ ] Close only route/integration gaps confirmed by the v2.5.0 contract review.
- [ ] Implement approved v3.0.0 contract changes (v3.0.0 Wave 2).

### Phase 3: Error Handling and Edge Cases
- [x] Focused tests exist for pipeline configuration, security configuration, and selected stream/chunk boundaries.
- [ ] Expand end-to-end failure/dependency and bounded-resource cases to match the agreed required paths.

### Phase 4: Tests
- [x] `tests/content/CMakeLists.txt` discovers `test_*.cpp` focused test sources and separately registers the soak target.
- [ ] Establish current execution evidence; source presence and CMake registration alone do not prove execution or pass status.

### Phase 5: Performance and Hardening
- [x] Three content benchmark targets are registered; the processor-path benchmark is synthetic and the release-gates target is API-level.
- [ ] Register and run production-path benchmarks for selected release gates; archive results before asserting performance.

### Phase 6: Documentation and Acceptance
- [x] Module overview, architecture, research method, performance expectations, and future-enhancement documents exist.
- [ ] Reconcile stale claims and historical Wave A–D evidence; complete release sign-off only from current traceable evidence.

## Production Readiness Checklist

- [x] Content implementation, public headers, focused tests, and benchmark sources are present in the checkout.
- [ ] Required format routes are traced and tested end to end; optional capabilities and failure behavior are explicit.
- [ ] Application/use-case acceptance is based on traced integration paths; extraction/embedding quality claims have task-specific labeled evaluation evidence or are explicitly marked unevaluated.
- [ ] Release-profile test execution and pass/fail/skip evidence is available for the supported configuration matrix.
- [ ] Production-path benchmark targets are registered and their representative-hardware results, thresholds, and complete run manifests are reviewed before accepting any content performance gate; synthetic sources and historical Wave B figures are not represented as verified measurements.
- [ ] Security and resource-bound controls are reviewed against actual execution paths, with no release-blocking findings.
- [ ] Historical Wave A–D claims are reconciled to current test sources, registrations, and dated CI artifacts.
- [ ] Operator guidance and final release sign-off are current and approved.

## Known Issues & Limitations

- Public processor inventory does not establish direct `ContentManager` dispatch or server/plugin end-to-end support; see [architecture](ARCHITECTURE.md).
- No corpus, labeling protocol, semantic-quality score, or reproducible performance result is established by the current research evidence; quality must not be inferred from implementation or processor inventory.
- Test sources are registered by glob, but this checkout alone does not establish successful compilation or execution. The focused CMake labels are `content`; the soak target is explicitly `not_release_critical`.
- The registered processor-path benchmark uses deterministic synthetic work; the unregistered extraction benchmark source is mock work. No benchmark result/hardware manifest was found in `benchmarks/content/`.
- Optional dependencies, build flags, policies, and runtime configuration change available behavior. See [operations](OPERATIONS.md) and [performance expectations](PERFORMANCE_EXPECTATIONS.md).
- Wave A–D closure bundles are historical reports, not a substitute for revalidated current release evidence.
- The [CMT-7505 coverage report](CMT-7505-TEST_COVERAGE_CORRELATION.md) contains historical inventory/status statements that differ from the current test directory and CMake registration; do not use its estimated coverage percentage as a measured coverage result.

## Breaking Changes

No content-module breaking change is approved by this roadmap. Preserve existing contracts within the current major line. Any v3.0.0 break requires an issue/owner, API and configuration migration guidance, compatibility tests, changelog entry, and approval under [`VERSIONING.md`](../../VERSIONING.md) before merge.

## Technical and Research References

- [OWASP File Upload Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/File_Upload_Cheat_Sheet.html) — guidance on defense in depth for untrusted uploaded files, including allowlisting, size limits, and validation; relevant to the content ingestion threat model, not evidence that this module satisfies each recommendation.
- [RFC 9110 §8.3, Content-Type](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.3) — defines HTTP media-type metadata; relevant when evaluating declared/detected MIME handling, but not a security validation recipe.
- Qu, Bao & Tu, [“Is Semantic Chunking Worth the Computational Cost?”](https://arxiv.org/abs/2410.13070) (arXiv:2410.13070) — reports context-dependent, inconsistent gains from semantic chunking. This supports evaluating chunking choices with task-specific labeled workloads rather than adopting an algorithm change without validation; it is external research, not ThemisDB evidence.
- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md) — benchmark setup and reporting guidance; useful for reproducible measurements, not a source of ThemisDB results.
- [C++ Core Guidelines: Resource management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r-resource-management) — RAII/resource-lifetime guidance relevant to adapter and worker ownership review; not a substitute for tests.
- [NIST SP 800-218, Secure Software Development Framework](https://csrc.nist.gov/pubs/sp/800/218/final) — secure-development and verification practices relevant to traceable security/release gates, not a content-module certification.

## Canonical Evidence References

- [Manager implementation](content_manager.cpp), [manager/API behavior summary](ARCHITECTURE.md), and [module overview and integration caveats](README.md).
- [Processor-chain contract](../../include/content/processor_chain_config.h) and [public manager API](../../include/content/content_manager.h).
- [Focused test registration](../../tests/content/CMakeLists.txt), [streaming tests](../../tests/content/test_content_streaming_ingestion.cpp), [pipeline configuration tests](../../tests/content/test_content_pipeline_hardening.cpp), [security tests](../../tests/content/test_content_security.cpp), [versioning tests](../../tests/content/test_content_version_manager.cpp), and [soak test](../../tests/content/test_content_soak.cpp).
- [Benchmark registration](../../benchmarks/content/CMakeLists.txt), [synthetic processor-path benchmark](../../benchmarks/content/bench_content_processor_paths.cpp), [versioning benchmark](../../benchmarks/content/bench_content_versioning.cpp), [API contract microbenchmark](../../benchmarks/content/bench_content_release_gates.cpp), and [benchmark notes](../../benchmarks/content/README.md).
- [Research and evaluation method](RESEARCH.md), [performance expectations](PERFORMANCE_EXPECTATIONS.md), [security overview](SECURITY.md), and [operations guidance](OPERATIONS.md).
- [Root roadmap](../../ROADMAP.md), [release strategy](../../RELEASE_STRATEGY.md), and [versioning policy](../../VERSIONING.md).

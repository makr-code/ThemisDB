---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-06"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# AI Module Roadmap

AI provenance note: Exact model backend not verifiable from final document text alone.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source-checked: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Historical Waves: A runtime controls implemented · B/C include cross-module work · D operability evidence incomplete. See the proposed v2.5.0/v3.0.0 waves below. -->

## Current Status

The in-scope production implementation is a controlled plugin-generation path and
its CAI/ethics adapter. This is hardened input/output orchestration, not a model
serving engine, a general AI/ML framework, or an isolated code-execution sandbox.

| Scope | Source-verified state | Evidence limit |
|---|---|---|
| Runtime | Prompt validation, configurable endpoint transport, bounded request/response sizes, endpoint allow-listing, retry/backoff, structured errors, output mapping, optional safety/telemetry callbacks, and artifact materialization are present. | Materializing files and reading them back does not isolate or execute generated code. |
| Public API | `AIPluginGenerator` exposes prompt/result/configuration types, limits, endpoint callbacks, optional gates, and seven outcome counters. | The API documents that concurrent `generatePlugin()` calls on one instance are not thread-safe. |
| Tests | `test_ai_plugin_generator.cpp` covers deterministic transport, validation, parsing, limits, allow-listing, gates, and counters. | The endpoint callbacks do not exercise a live provider. |
| Soak/stress | AI generation soak and high-cardinality stress test sources exist. | These use in-process stubs/simulations; they are not production-path, provider, or throughput evidence. |
| Benchmark | `bench_ai_plugin_generator.cpp` measures validation, orchestration, and malformed-response handling using canned JSON. | It excludes model inference, provider queueing, output quality, and provider cost. |

**Evidence maturity:** hardened component; module release readiness remains
unproven until the proposed validation waves below have current test,
representative-environment, and security evidence. Existing historical wave
closure records are not substitutes for those gates.

**Historical wave scope:** Wave A describes implemented in-scope generator
controls. Wave B's Self-RAG, RotatE, and Multi-Task LoRA work is owned by other
modules (`rag`, `graph`, and `training`). Wave C includes the in-scope
`CAIEthicsIntegration` adapter, but federated learning and graph phase gates are
cross-module work. Wave D's soak/stress sources are simulations; representative
hardware and provider-backed evidence remain open. Do not count cross-module
items or simulated tests as AI-module release sign-off.

**Files checked for issue #7399:** `src/ai/ai_plugin_generator.cpp`,
`src/ai/cai_ethics_integration.cpp`, `include/ai/ai_plugin_generator.h`,
`include/ai/cai_ethics_integration.h`, all `tests/ai/test_*.cpp`, and
`benchmarks/ai/bench_ai_plugin_generator.cpp`. The corresponding limits and
research references are recorded in [`RESEARCH.md`](./RESEARCH.md).

## v2.5.0 and v3.0.0 Research Roadmap

This is the proposed module plan for issue #7399, not a release commitment. `v2.5.0` is treated as
the next backward-compatible MINOR target; `v3.0.0` is a strategic MAJOR target.
Confirm active GitHub milestones before assigning implementation PRs. Effort
estimates are engineering person-weeks and exclude shared platform work and
human review time.

### v2.5.0 — Bounded, measurable, secure generation

#### Wave 1 — Operational and contract evidence (P0, 2–3 person-weeks)

- **Goal:** Close the gap between existing callbacks/counters/traces and
  reproducible operational evidence without changing the public API.
- **Tasks:**
  - [ ] Reconcile the documented error taxonomy and counters with every
    `generatePlugin()` return path; add focused regression cases for each class.
  - [ ] Capture source-compatible p50/p95/p99 latency and error-rate baselines
    for validation, parsing, and orchestration separately from provider time.
  - [ ] Verify the documented timeout, retry, redaction, and thread-safety
    behavior against the implementation and runbook.
- **Dependencies:** Existing focused AI test registration; runbook and tracing
  infrastructure. Reuse shared OpenTelemetry conventions rather than adding a
  second telemetry system.
- **Risk / trade-off:** More instrumentation can add latency or expose sensitive
  data; retain bounded, redacted attributes and measure instrumentation overhead.
- **Success metrics:** All seven outcome counters have a tested success/error
  mapping; 100% of registered benchmark cases emit versioned results; p99
  orchestration regression is at most 10% against the same-build baseline.
- **Validation:** `module_ai_test_ai_plugin_generator_focused`, error-path unit
  tests, benchmark result comparison, trace-attribute/redaction checks.

#### Wave 2 — Isolated artifact verification (P0, 4–6 person-weeks)

- **Goal:** Prevent generated artifacts from being treated as safe solely
  because parsing, callback policy, or file round-trip checks passed.
- **Tasks:**
  - [ ] Design a least-privilege, disposable build/test environment with network
    access disabled, resource/time limits, and a non-privileged identity.
  - [ ] Run generated artifacts through compile, generated tests, and configured
    static/security checks before any promotion or installation.
  - [ ] Record artifact hashes and scanner/build outcomes; reject missing,
    inconclusive, or failed verification results.
  - [ ] Add adversarial tests for traversal, oversized artifacts, malformed
    manifests, build timeouts, tool failures, and cleanup after interruption.
- **Dependencies:** A supported sandbox/runner and plugin build/security tooling;
  no production-code execution is enabled by this plan until those controls are
  available.
- **Risk / trade-off:** Strong isolation increases build latency and operational
  complexity; fail closed if isolation or cleanup cannot be verified.
- **Success metrics:** 100% of artifacts promoted by the gated path have a
  recorded hash and successful isolated build/test/security result; zero
  promotion on verifier failure; zero unresolved critical/high findings in the
  acceptance corpus.
- **Validation:** Deterministic integration fixtures, negative/fault-injection
  tests, sandbox boundary tests, secret/dependency scanning, and human security
  review.

#### Wave 3 — Provider-backed quality and performance baseline (P1, 3–5 person-weeks)

- **Goal:** Measure model-dependent quality and end-to-end costs separately from
  the callback benchmark.
- **Tasks:**
  - [ ] Create a versioned acceptance set of at least 50 representative plugin
    tasks; capture provider/model, prompt version, decoding settings, limits,
    and output hashes for every run.
  - [ ] Measure compile/test pass rate, security findings, human-reviewed
    functional quality, rejection rate, latency p50/p95/p99, and cost per
    accepted artifact; report provider and local orchestration separately.
  - [ ] Repeat each case at least three times and publish uncertainty and known
    dataset/model coverage limits before setting additional quality thresholds.
- **Dependencies:** Wave 1 baseline, Wave 2 isolated verification, approved
  provider credentials and an approved dataset.
- **Risk / trade-off:** Provider/model changes and stochastic output reduce
  comparability; pin identifiers/configuration and do not compare unlike runs.
- **Success metrics:** 100% of evaluation outputs have complete provenance and
  verification results; at least 90% of the fixed acceptance set passes
  compile-and-test checks, with zero critical security findings. This is a
  proposed release gate requiring maintainer ratification, not a current result.
- **Validation:** Reproducible provider run, independent artifact test/security
  checks, human review of a stratified sample, and comparison to the recorded
  baseline.

### v3.0.0 — Versioned generation contract and governed lifecycle

#### Wave 4 — Provider-neutral, versioned generation contract (P1, 4–6 person-weeks)

- **Goal:** Define a stable, explicit contract for provider capabilities,
  structured generation results, cancellation/timeouts, and verification state.
- **Tasks:**
  - [ ] Specify a versioned request/result schema and provider contract tests,
    including unsupported capability and partial-response behavior.
  - [ ] Decide whether a breaking API change is necessary; if so, document the
    migration and remove obsolete API only under the major-version policy.
  - [ ] Add contract fixtures for every supported provider adapter and make
    fail-closed behavior uniform.
- **Dependencies:** v2.5.0 Waves 1–3 evidence and human API design approval.
- **Risk / trade-off:** Abstraction may hide provider-specific limits or increase
  the public API surface; keep provider extensions explicit and avoid silent
  compatibility fallbacks.
- **Success metrics:** 100% of supported adapters pass the same contract suite;
  all invalid/partial responses fail closed; every breaking change has a
  migration note and compatibility test.
- **Validation:** Schema/contract tests, adapter conformance tests, API review,
  and migration tests on representative clients.

#### Wave 5 — Artifact provenance, policy, and lifecycle governance (P1, 5–8 person-weeks)

- **Goal:** Make generated artifacts reviewable, attributable, revocable, and
  reproducible through the plugin lifecycle.
- **Tasks:**
  - [ ] Bind each artifact to model/provider and prompt/schema versions, policy
    results, dependency inventory, build/test results, and immutable hash.
  - [ ] Define retention, redaction, human approval, rollback, and revocation
    procedures for generated artifacts.
  - [ ] Add red-team regression cases for prompt injection, improper output
    handling, sensitive-data disclosure, and excessive permissions.
- **Dependencies:** Wave 2 isolation and Wave 4 schema; shared plugin registry,
  security, and audit contracts.
- **Risk / trade-off:** Provenance improves auditability but can expose
  confidential prompts or provider metadata; apply least-data retention and
  access control.
- **Success metrics:** 100% of released artifacts have complete provenance and
  human approval; zero release on failed policy or missing provenance; zero
  known high-severity regression in the red-team corpus.
- **Validation:** Reproducibility/rollback drills, adversarial regression suite,
  audit-log review, and security/governance sign-off.

### Open assumptions and decision gates

- The scope here is limited to `src/ai/`, `include/ai/`, `tests/ai/`, and
  `benchmarks/ai/`. RAG, graph completion, federated training, and general
  inference serving remain owned by their implementation modules.
- No live-provider performance or generation-quality measurements were available
  in the inspected artifacts. The provider-backed acceptance gate is proposed
  work, not a claim about current quality.
- The 90% compile/test target and effort ranges are planning assumptions and
  require maintainer review before becoming release gates.
- A dedicated OS/container isolation facility, accepted provider/data policy,
  and active release milestones were not verified in this source review.

## Module Documentation Enhancements (2026-07-19)

- [x] Added comprehensive Doxygen documentation to all implementation functions
- [x] Documented `AIPluginGenerator::validatePrompt()` with detailed validation rules
- [x] Documented `AIPluginGenerator::generatePlugin()` with complete execution pipeline
- [x] Documented all CAI ethics integration functions with semantics and contracts
- [x] Added inline documentation for error handling, thread-safety, and retry policies
- [x] Lines added: ai_plugin_generator.cpp +133, cai_ethics_integration.cpp +130
- [x] Total module documentation lines: ~1,473 (up from 1,210)

## In Progress

- [x] Validation hardening for non-description prompt fields (Target: Q3 2026)
- [x] Endpoint safety hardening (allow-list, response-size limits) (Target: Q3 2026)
- [x] Performance gate consolidation for AI generation proxy benchmarks (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] Enforce schema-level validation for all generated payload fields (Target: Q4 2026)
- [x] Introduce deterministic retry/backoff policy for transient endpoint failures (Target: Q4 2026)
- [x] Add explicit redaction policy for diagnostic output fields (Target: Q4 2026)

### Mid-term (6-12 months)
- [x] Integrate optional sandbox verification gate for generated code artifacts (artifact materialization + optional callback verification enforced in generator path) (Target: Q1 2027)
- [x] Add dedicated benchmark target for AI plugin generation path (Target: Q1 2027)
- [x] Expand observability counters for error classes and endpoint quality signals (Target: Q1 2027)

### Historical cross-module ML work (not AI-module roadmap scope)
- CAI/ethics integration is present in `src/ai/cai_ethics_integration.cpp`; CAI evaluation suites and benchmark claims cited in the historical Wave C records are outside `tests/ai/` and are not verified by this roadmap.
- Federated learning and graph phase-gate orchestration belong to other modules; their implementation and acceptance tests are not AI-module deliverables.
- Self-RAG, RotatE, and Multi-Task LoRA are owned by `rag`, `graph`, and `training`; their historical completion states do not establish AI-module implementation or release readiness.
- The optional artifact materialization/callback gate and dedicated callback benchmark are implemented; neither is an operating-system sandbox or provider-backed quality benchmark.
- Existing counters and trace hooks are part of the baseline; representative-hardware and provider-backed measurements remain open in the v2.5.0 plan.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Stable API for prompt/config/result types in public header
- [x] Validation-first behavior contract defined and implemented

### Phase 2: Core Implementation
- [x] Endpoint invocation path implemented with configurable transport
- [x] JSON response mapping to `GeneratedPlugin` implemented

### Phase 3: Error Handling and Edge Cases
- [x] Non-2xx, transport, and parse failures normalized to structured errors
- [x] Extended validation for capability/dependency fields (Target: Q3 2026)

### Phase 4: Tests
- [x] Focused unit coverage for constructor, validation, and endpoint/error paths
- [x] Integration suite with deterministic endpoint fixtures (Target: Q3 2026)

### Phase 5: Performance and Hardening
- [x] Add module-specific benchmark instead of proxy-only tracking (Target: Q1 2027)
- [x] Enforce endpoint allow-list and payload size bounds (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] Core module docs aligned with source-verifiable behavior
- [x] Completed work tracked in changelog; roadmap remains forward-looking

## Production Readiness Checklist

- [x] Validation-first execution path documented and verified
- [x] Structured error handling for endpoint and parse failures verified
- [x] Proxy benchmark mapping documented in performance expectations
- [x] Dedicated benchmark target registered
- [x] Hardening follow-ups closed for endpoint safety controls
- [ ] Generated artifacts verified in an isolated build/test/security environment
- [ ] Provider-backed quality, latency, and cost baseline recorded
- [ ] Representative-environment soak and security evidence reviewed

## References

- [Future enhancements](./FUTURE_ENHANCEMENTS.md)
- [Research notes and external references](./RESEARCH.md)
- [Public API roadmap](../../include/ai/ROADMAP.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)

## Known Issues & Limitations

- `AIPluginGenerator` provides opt-in C1/C2 callbacks; `LLMAQLHandler` inference, streaming, RAG, and chat paths belong to the `llm` module and are not evidence of in-scope `ai` runtime coverage.
- Artifact materialization and read-back verification do not provide OS-level isolation or execute generated code.
- The AI benchmark and sustained-load tests use callbacks/stubs; no provider-backed quality or representative-hardware performance results were verified.
- Schema-level validation for all LLM output fields is enforced: code fields ≤ 1 MiB, `security_report` ≤ 64 KiB, `version` ≤ 64 chars (defaults to `0.1.0`), `manifest.description` truncated at 8192 bytes, oversized `build_dependencies` entries silently dropped.

## Historical Wave C Timeline and Dependencies (cross-module)

- Start: Q3 2027 (early July)
- Target: End Q4 2027 (mid-December)
- Estimated Effort: 16–24 weeks total (depending on federated security requirements)

### Dependencies
- [x] Wave A + Wave B stability checks tracked in focused regression suites and release-gate docs (`CTEST.md`, Wave A `#5038`, Wave B `#5039`)
- [x] Constitutional AI principles formalized in ethics framework (`src/ai/cai_ethics_integration.cpp`, `tests/test_cai_safety_module.cpp`)
- [x] Multi-node federated benchmark infra/security review tracking established (FEDERATED-BENCH-01 coverage + issue traceability `#5040`/`#5039`)
- [x] Graph phase gate orchestrator implemented and all exit criteria confirmed (GRAPH_PHASE_GATE-01..15 + GRAPH_PHASE_GATE-BENCH-01 — issue #6287)

### Wave C Exit Criteria ✅ COMPLETE

> **Closure Date:** 2026-09-16 | **Evidence:** `src/ai/WAVE_C_CLOSURE_EVIDENCE_BUNDLE.md`

- [x] C1 CAI Safety: CAI-01..15 + CAI-BENCH-01 all passing (`tests/test_cai_safety_module.cpp`)
- [x] C1 CAI Safety: safety alignment ≥ 0.80, latency ≤ 2.0 s, FPR ≤ 10%
- [x] C2 Federated Learning: FEDERATED-01..15 + FEDERATED-BENCH-01 all passing (`tests/test_federated_privacy_training.cpp`)
- [x] C2 Federated Learning: convergence ≥ 95% of centralized baseline, overhead ≤ 2.0 s, DP budget configurable
- [x] C3 Graph Phase Gate: GRAPH_PHASE_GATE-01..15 + BENCH-01 all passing (`tests/graph/test_graph_phase_gate_orchestration.cpp`)
- [x] C3 Graph Phase Gate: BENCH-01 ≤ 500 ms wall-clock
- [x] Production-runtime hook integration: C1 safety gate + C2 telemetry in `LLMAQLHandler` paths (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`)

### References
- `src/ai/FUTURE_ENHANCEMENTS.md#wave-c--strategic-ml-enhancements-q3-2027`
- `docs/research/ml_enhancements_bibliography.md`
- `#5040` (Wave C Issue)
- `#5038` (Wave A Issue)
- `#5039` (Wave B Issue)

## Historical Wave C Research Publication Opportunity

- Joint paper: ThemisDB Integration of Research-Backed ML Features
- Target venue window: MLSys 2028 / FAccT 2028
- The dedicated benchmark at `benchmarks/ai/bench_ai_plugin_generator.cpp` measures callback-backed validation and orchestration, not live inference or output quality.
- Field-level prompt validation is implemented; generated artifact security still requires independent verification and operating-system isolation.
- Sandbox artifact materialization and optional callback verification are enforced when `enable_sandbox_gate` is enabled; neither constitutes a sandbox.
- Wave B features are cross-module work and remain governed by their owning module roadmaps.

## Wave A — Runtime Reliability ✅ COMPLETE

> **Closure Date:** 2026-09-16 | **Evidence:** `src/ai/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`

### A1: Prompt Validation Hardening
- [x] Description length enforced (max 8 192 bytes)
- [x] Token-list size limits (required_capabilities, dependencies)
- [x] Duplicate token detection and rejection
- [x] ASCII control-character stripping on all string fields
- [x] Format validation for capability/dependency tokens

### A2: Endpoint Safety Hardening
- [x] Configurable endpoint allow-list enforced before outbound calls
- [x] Request payload ceiling (256 KiB) enforced fail-closed
- [x] Response payload ceiling (8 MiB) enforced before parse
- [x] Non-2xx HTTP responses normalized to structured error
- [x] Transport-level failures normalized to structured error

### A3: Retry / Backoff Policy
- [x] 3-attempt retry for transient endpoint failures
- [x] Exponential backoff: 100 ms / 200 ms / 400 ms
- [x] Non-retryable failures fail immediately
- [x] Retry attempts logged at WARN level

### A4: Output-Field Validation
- [x] Generated code ≤ 1 MiB per field
- [x] `security_report` ≤ 64 KiB
- [x] `version` ≤ 64 chars (default `0.1.0`)
- [x] `manifest.description` truncated at 8 192 bytes
- [x] Oversized `build_dependencies` entries dropped

### A5: Fail-Closed Error Handling
- [x] All error branches return structured `Error` result
- [x] Stats struct: 7 observable counters
- [x] Log redaction: sensitive fields bounded at 120 chars
- [x] Thread-safety contract documented (not thread-safe for concurrent `generatePlugin`)

### Wave A Exit Criteria
- [x] Validation-first execution path verified
- [x] Endpoint safety controls verified
- [x] Retry policy documented and source-verified
- [x] Output-field validation complete
- [x] All HIGH-severity MODULE_GAPS.md findings resolved (2026-07-19)
- [x] Focused test coverage: `tests/ai/test_ai_plugin_generator.cpp`, `tests/ai/test_ai_decision_auditor.cpp`
- [x] Benchmark target registered: `benchmarks/ai/bench_ai_plugin_generator.cpp`

---

## Historical Wave B Tracking (cross-module)

### B1: Self-RAG (Self-Retrieving, Auto-Critique)
- [x] Retrieval controller (binary classify: retrieve now?)
- [x] Critic model (Relevant/Partial/Irrelevant)
- [x] Iterative refinement loop (max 3 rounds)
- [x] Unit tests SELF_RAG-01..12
- [x] InferenceEngineEnhanced callback integration
- [x] ALCE benchmark vs vanilla RAG

### B2: Knowledge Graph Completion (RotatE)
- [x] RotatE embedding model (relation-as-rotation)
- [x] Triple loss with negative sampling
- [x] Link-prediction head
- [x] Unit tests KGC-01..15
- [x] TransE baseline benchmark
- [x] KnowledgeGraphReasoner integration

### B3: Multi-Task LoRA Fine-Tuning
- [x] Shared LoRA base + task-specific projections
- [x] Domain-gating mechanism
- [x] Joint loss with configurable task weighting
- [x] Unit tests MTL-01..10
- [x] Shared-vs-single-task baseline ablation
- [x] 3-task benchmark evaluation

### Acceptance Gates
- [x] Hallucination rate reduction ≥ 20% vs standard RAG
- [x] Self-RAG latency increase ≤ 1.5× vs baseline
- [x] Precision@K retrieval ≥ 0.85 on golden-doc tests
- [x] RotatE MRR ≥ 0.35 and Hits@10 ≥ 0.55 on deterministic acceptance fixture
- [x] RotatE inference latency ≤ 50 ms for top-20 predictions
- [x] Multi-task LoRA average task performance ≥ +8% vs single-task
- [x] Multi-task LoRA training time increase ≤ 15%

### Wave B Exit Criteria ✅ SIGNED OFF 2026-09-09

> **Sign-off:** Issue [#6286](https://github.com/makr-code/ThemisDB/issues/6286) ✅ SIGNED OFF 2026-09-09 | **Evidence:** `src/ai/WAVE_B_CLOSURE_EVIDENCE_BUNDLE.md`

- [x] B1 Self-RAG: ALCE-01..05 acceptance tests passing (`tests/rag/test_self_rag_alce.cpp`)
- [x] B2 RotatE KGC: KGC-01..15 unit tests passing (`tests/test_rotate_completion.cpp`)
- [x] B3 Multi-Task LoRA: MTL acceptance tests passing (`tests/training/test_multitask_lora_acceptance_gates.cpp`)
- [x] All seven acceptance-gate criteria met (hallucination, latency, precision, MRR, Hits@10, LoRA perf, LoRA overhead)
- [x] CI evidence archival complete on `develop`
- [~] Production promotion: gated on Wave A hardware baselines (pending Q4 2026)

### Dependencies
- [~] Wave A deployment complete (Speculative Decoding, DPR, Fairness) — in progress Q4 2026
- [ ] LLM inference P95 latency < 200 ms
- [ ] KnowledgeGraphReasoner stable + benchmark suite passing

### References
- Research bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Future enhancements detail: `FUTURE_ENHANCEMENTS.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

---

## Wave D — Operability Hardening 🟡 IN PROGRESS Q1 2027

> **Detail:** `src/ai/WAVE_D_ROADMAP.md` | **Dependency:** Wave A–C exit criteria confirmed

### D1: AI Generation Operator Runbook
- [x] Draft runbook: incident taxonomy (validation/endpoint/transport/parse/sandbox failures) (Target: 2026-11)
- [x] Decision tree for endpoint-timeout + retry-storm scenarios (Target: 2026-11)
- [x] Stats-counter operational thresholds and alert rules (Target: 2026-12)
- [x] CAI safety gate latency incident section (Target: 2026-12)
- [x] Output: `docs/operability/RUNBOOK_AI_GENERATION.md`

### D2: Observability Expansion
- [x] `AIPluginGenerator::Stats` exposes seven outcome counters with focused test coverage
- [ ] CAI safety gate per-call latency histogram (p50/p95/p99) (Target: 2026-11)
- [ ] Federated aggregation round-latency counter — owned by the federated-learning module (Target: 2026-12)
- [x] OpenTelemetry span propagation in `generatePlugin` (Target: Q1 2027)
- [x] Span propagation in LLMAQLHandler CAI/federated gate paths — cross-module `llm` evidence

### D3: Soak Tests (Sustained Load)
- [x] `tests/ai/test_ai_generation_soak_60min.cpp` source exists; it simulates the pipeline and does not invoke `AIPluginGenerator` (Target: 2026-12)
- [ ] CAI 10-min sustained evaluation test — 500 evaluations/min (Target: 2026-12)
- [ ] Federated 30-min round-stability test (Target: Q1 2027)
- [ ] Execute full soak suite on representative hardware (Target: Q1 2027)

### D4: Representative-Hardware Baselines
- [ ] `generatePlugin` + `validatePrompt` p95/p99 hardware baselines (Target: Q1 2027)
- [ ] CAI safety gate p95/p99 overhead hardware baselines (Target: Q1 2027)
- [ ] Federated aggregation round p95 hardware baseline (Target: Q1 2027)

### Wave D Exit Criteria
- [x] AI generation runbook published (`docs/operability/RUNBOOK_AI_GENERATION.md`)
- [ ] Observability expansion complete (CAI latency histogram remains open; federated latency is cross-module)
- [x] OpenTelemetry span propagation in `generatePlugin`; LLMAQLHandler paths are cross-module
- [~] Soak test source created; it uses an in-process stub and is not representative-hardware or production-path evidence (full soak run pending)
- [ ] Representative-hardware p95/p99 baselines captured for AI generation + CAI gate
- [ ] Wave D sign-off: human approval at `docs/operability/WAVE_D_SIGN_OFF.md`

## Module Validation Evidence (2026-07-19)

### Documentation Status
- **@file Doxygen Headers**: 100% coverage (4/4 files)
  - `include/ai/ai_plugin_generator.h` — hardened implementation metadata
  - `include/ai/cai_ethics_integration.h` — hardened implementation metadata
  - `src/ai/ai_plugin_generator.cpp` — hardened implementation metadata
  - `src/ai/cai_ethics_integration.cpp` — hardened implementation metadata

- **Function/Method Documentation**: Enhanced 2026-07-19
  - Added comprehensive Doxygen comments to all public methods
  - Added parameter/return/error documentation to key functions
  - Documented thread-safety contracts and error handling semantics
  - Implementation files received the primary Doxygen expansion (+263 lines across 2 `.cpp` files)
  - Header files received follow-up contract clarifications for transport, validation, and latency semantics

### Code Quality Metrics
- **Lines of Code**:
  - Source: 946 lines (ai_plugin_generator.cpp: 634, cai_ethics_integration.cpp: 312)
  - Headers: 476 lines (ai_plugin_generator.h: 283, cai_ethics_integration.h: 193)
  - Total: 1,422 lines (module core)

- **API Stability**:
  - Public API stable (AIPluginGenerator, CAIEthicsIntegration)
  - Config structures fully documented with field semantics
  - Callback types fully documented with signatures and contract details
  - Zero breaking changes in current implementation

### Feature Coverage
- ✅ Phase 1: Stable API for prompt/config/result types
- ✅ Phase 2: Endpoint invocation with configurable transport
- ✅ Phase 3: Structured error handling and edge cases
- ✅ Phase 4: Focused unit test coverage (2 test files: test_ai_decision_auditor.cpp, test_ai_plugin_generator.cpp)
- ✅ Phase 5: Observable counters implemented (Stats struct with 7 counters)
- ✅ Phase 6: Documentation aligned with implementation

### Hardening Completeness
- ✅ Prompt validation (description length, token list sizes, format validation)
- ✅ Request sanitization (ASCII control character stripping)
- ✅ Endpoint allow-list enforcement
- ✅ Request/response size limits (256 KiB / 8 MiB)
- ✅ Retryable endpoint invocation (3 attempts, exponential backoff)
- ✅ Response parsing with malformed input rejection
- ✅ Output field validation (code size, manifest fields)
- ✅ Optional C1 CAI safety gate (Wave C feature)
- ✅ Optional sandbox artifact materialization + callback verification enforced in the generator path
- ✅ Optional C2 federated telemetry (Wave C feature)
- ✅ Observability counters for error classes

### Production Readiness Status
- **Maturity**: 🟡 Hardened implementation (all HIGH-severity gaps reviewed; full production validation still pending environment-complete build/test)
- **Configuration**: Validatable via CMakeLists.txt, CMakePresets.json
- **Testing**: Focused test targets auto-discovered (module_ai_*_focused.exe)
- **Error Handling**: Fail-closed with structured Error result types
- **Logging**: Redaction-aware with configurable truncation (120 chars max)
- **Thread Safety**: Document-specified (not thread-safe for concurrent generatePlugin)
- **Memory Safety**: RAII compliance, smart pointers for ownership
- **Security**: Input validation, output bounds checking, endpoint allow-listing

### Known Limitations (MODULE_GAPS.md reference)
- HIGH-severity scanner findings were reviewed and either remediated or reclassified with source-backed explanations (2026-07-19)
- MEDIUM-priority scope/documentation gaps remain tracked in `MODULE_GAPS.md`
- Dedicated benchmark target for the AI plugin generator path is registered and tracked in benchmark docs
- Sandbox verification now includes built-in artifact materialization, read-back verification, and optional callback enforcement

### Acceptance Criteria Status
- [x] Validation-first execution path implemented and verified
- [x] Structured error handling for all failure points
- [x] Hardening follow-ups complete for endpoint safety, payload validation, observability counters, benchmark coverage, and sandbox artifact verification
- [x] API documentation complete and comprehensive
- [x] Implementation semantics documented at function level
- [x] Roadmap and Future Enhancements synchronized (2026-07-19)
- [x] All HIGH-severity gaps reviewed with either code remediation or source-backed disposition notes (2026-07-19)

## Breaking Changes

No breaking API change planned. Any signature/semantic contract change requires explicit migration notes and changelog entry.
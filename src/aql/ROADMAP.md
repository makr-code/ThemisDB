---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Team -->
<!-- Exact model backend not verifiable from final document text alone. -->
# AQL Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Historical validation notes below are not current build/test evidence. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The `aql` module is the query-authoring assistance layer (translation,
validation, helper tooling, context, and adapters); it does not own the core
AQL parser, query planner, executor, or execution-time authorization. Current
source inventory includes 25 implementation files under `src/aql/`, 34 public
headers under `include/aql/`, 66 focused `test_aql_*.cpp` sources under
`tests/aql/`, and 8 benchmark sources under `benchmarks/aql/`. CMake source
registration is not evidence that targets built, tests passed, or benchmarks
were run.

The current maturity assessment is **implemented assistance surface with
material evaluation, contract, and release-evidence gaps**. Translation
sanitizes bounded inputs, constructs provider requests, validates returned
queries, and applies configured policy and collection/scope checks. However,
`WARN_ONLY` is the handler default, explicitly disabling the parser selects a
weaker string-level validator, and parser acceptance is not semantic
correctness or authorization. Existing translation and helper benchmarks
include mock implementations, and the module research review found no
versioned, held-out natural-language-to-AQL quality corpus. These facts prevent
the historical microbenchmark numbers or test-file counts from establishing
production accuracy, end-to-end provider latency, or release readiness.

The RAG consumer now exposes a request-scoped typed retrieval-context overload
and denies retrieval when no trusted collection authorizer is supplied. The
legacy overload is source-compatible but fails closed. Source inspection still
finds no production caller that supplies authenticated tenant/policy context;
the host startup/request integration and focused real-backend validation remain
open.

This section is the active forward plan. Dated completion reports below are
historical unless corroborated by current source and reproducible test or
benchmark output. v2.5.0 and v3.0.0 are planning targets, not release
commitments; release status remains governed by the root roadmap.

## Research and Gap Analysis

The research-to-code comparison is scoped to AQL assistance. Optimizer and
execution-engine changes belong to `src/query/` and are not claimed as AQL
module work.

| Concern | Research/technical finding | Current evidence and gap | Roadmap response |
|---|---|---|---|
| NL-to-query usefulness | Spider measures cross-schema and cross-domain generalization; BIRD adds realistic database values and execution efficiency. A single familiar-schema demo cannot establish generalization or intent fidelity. | `src/aql/RESEARCH.md` records that no canonical provider/model comparison corpus or quality evaluator is defined. Existing focused tests use controlled fixtures, not a held-out workload. | v2.5.0: version a synthetic, reviewed held-out corpus and report syntax, execution/intent, and failure metrics separately. |
| Validity by construction | PICARD demonstrates that incremental parsing can constrain generated SQL token sequences; it reduces invalid syntax but does not prove user intent or permissions. | `LLMAQLHandler` uses parser-backed validation when available, otherwise a weaker string-level fallback; the default validation mode is `WARN_ONLY` (`include/aql/llm_aql_handler.h`, `src/aql/llm_aql_handler.cpp`). | v2.5.0: test and expose strict parser-backed acceptance and make fallback status explicit. v3.0.0: assess provider-supported structured/constrained output behind a versioned contract. |
| Performance and scale | Vectorized query-processing research and TPC-H illustrate the importance of workload- and hardware-specific evaluation; those execution-engine results do not directly measure this assistance module. | Six benchmark targets are registered by `benchmarks/aql/CMakeLists.txt`; the translation and consolidated gate sources use mocks. No current representative-hardware run was verified. | Keep local helper/parser overhead separate from provider latency, corpus quality, and downstream query execution. Publish run configuration and percentile distributions. |
| Security and resilience | OWASP identifies prompt injection as a persistent LLM-application risk; syntactic parsing is not a security or authorization boundary. | The module bounds/sanitizes prompt inputs and validates generated output, but host authorization remains required and must be checked at every execution/tool boundary (`src/aql/SECURITY.md`, `src/aql/PRODUCTION_REQUIREMENTS.md`). | v2.5.0: adversarial, degraded-provider, timeout, cancellation, and host-boundary integration tests. v3.0.0: typed policy/result boundary and privacy-reviewed telemetry. |
| Governance and maintainability | Reproducible evaluation requires fixed input/schema/provider settings, held-out data, and documented run metadata. | Test registration is source-driven (`tests/aql/CMakeLists.txt`); stale roadmap entries cite historical runs, and the 2026-11-02 wiki inventory reports 1,450 removed sources and 26 changed sources. | Treat code/tests/benchmarks as primary evidence, refresh wiki inventory, retain dated run manifests, and require human review for corpus labels and breaking API decisions. |

### Research References

- Yu et al., [Spider: A Large-Scale Human-Labeled Dataset for Complex and
  Cross-Domain Semantic Parsing and Text-to-SQL Task](https://aclanthology.org/D18-1425/),
  EMNLP 2018. It motivates held-out schema/domain splits rather than measuring
  memorization on one schema.
- Li et al., [BIRD: A BIg Bench for laRge-scale Database Grounded in Text-to-SQL
  tasks](https://arxiv.org/abs/2305.03111), 2023. It emphasizes real database
  values, external knowledge, and SQL execution efficiency; AQL evaluation
  should likewise include representative values and query cost while keeping
  its own dialect and policy checks.
- Scholak et al., [PICARD: Parsing Incrementally for Constrained Auto-Regressive
  Decoding from Language Models](https://aclanthology.org/2021.emnlp-main.779/),
  EMNLP 2021. Constrained decoding is a candidate validity aid, not a substitute
  for semantic validation or authorization.
- Boncz et al., [MonetDB/X100: Hyper-Pipelining Query
  Execution](https://www.cidrdb.org/cidr2005/papers/P19.pdf), CIDR 2005, and
  the [TPC-H workload description](https://www.tpc.org/tpch/). These inform
  execution-engine benchmarking only; they do not justify applying query
  execution latency claims to LLM-assisted translation.
- [OWASP Top 10 for LLM Applications 2025: Prompt
  Injection](https://owasp.org/www-project-top-10-for-large-language-model-applications/).
  Prompt content and retrieved context remain untrusted; filtering alone is
  not a complete security boundary.

## Prioritized Measures

### v2.5.0 — Measurable, Safe Assistance

Priority is to close evidence and policy gaps without changing the core query
engine or silently changing the public default contract.

#### Wave 1 — Evaluation Corpus and Baseline (P1)

- [ ] Create a versioned, synthetic-data AQL assistance corpus with schema,
  natural-language intent, reviewed acceptance criteria, expected policy
  outcome, and held-out schema/domain partitions (Target: v2.5.0).
- [ ] Add a deterministic evaluator that records parser acceptance, execution
  equivalence on isolated fixtures where applicable, human-rated intent
  fidelity, retries, token use, provider errors, and p50/p95/p99 latency
  separately (Target: v2.5.0).
- [ ] Record corpus revision, provider/model configuration, build revision,
  validation mode, and evaluator version in every result; prohibit test-set
  tuning and production-data inclusion (Target: v2.5.0).

**Dependencies:** query parser service and existing focused-test infrastructure.
**Risk/trade-off:** corpus annotation costs time and may overfit to one model or
schema; use synthetic fixtures, blind held-out partitions, and periodic
human-reviewed refreshes.
**Effort estimate:** 2–3 engineer-weeks, excluding external model-provider
availability.
**Success metrics:** every corpus item has reviewed labels and schema context;
baseline is repeatable from a clean checkout; held-out results report separate
syntax, policy, and intent metrics; no claim is made from training/example
queries.
**Validation:** unit tests for corpus parsing and evaluator metrics; integration
tests on held-out fixtures; benchmark evaluator overhead independently of
provider latency; review data provenance and privacy.

#### Wave 2 — Validation Contract and Security Boundaries (P1)

- [ ] Add focused contract coverage for parser-backed validation, explicit
  parser-disabled behavior, each validation mode, empty/over-limit input,
  retries, and final-versus-partial streaming results (Target: v2.5.0).
- [ ] Make the selected validation mode and parser/string-fallback status
  observable in typed results/diagnostics without logging prompt or query
  contents; document strict `REJECT_ON_ERROR` deployment configuration
  (Target: v2.5.0).
- [ ] Verify and test authorization at every host execution and agent-tool
  boundary; generated AQL, parser success, and confidence scores must never
  grant permission (Target: v2.5.0).
- [ ] Add adversarial and resilience cases for direct/retrieved prompt injection,
  malformed provider output, provider outage, timeout, cancellation, circuit
  breaker transitions, and bounded context under concurrency (Target: v2.5.0).

**Dependencies:** Wave 1 fixtures; `query::AQLParserService`; host integration
owners for executable/tool routes.
**Risk/trade-off:** strict rejection can reduce availability and change caller
expectations. Keep the existing default unchanged in v2.5.0 unless API and
versioning review explicitly approves a change; do not describe the warning
mode as a safety guarantee.
**Effort estimate:** 3–5 engineer-weeks, including cross-module review.
**Success metrics:** 100% of strict-mode invalid/parser-unavailable cases reject
or return an explicit non-validated status; all execution/tool paths have
positive and negative authorization tests; no partial stream is executable;
resource bounds hold for stress cases.
**Validation:** focused CTest tests under `tests/aql/`, security review of all
host boundaries, adversarial regression suite, and ASan/TSan or equivalent
supported concurrency checks.

#### Wave 3 — Representative Performance and Operations Evidence (P2)

- [ ] Split benchmark reporting into parser/validator and helper CPU overhead,
  provider end-to-end latency, and corpus quality; label mock results as
  non-provider measurements (Target: v2.5.0).
- [ ] Re-run mapped AQL benchmarks on declared representative hardware and
  release configuration; archive raw output, repetition count, environment, and
  revision, and compare p50/p95/p99 with the first Wave 1 baseline
  (Target: v2.5.0).
- [ ] Exercise overload, timeout, cancellation, circuit-breaker recovery, and
  redacted diagnostics in the documented operator runbook (Target: v2.5.0).

**Dependencies:** Waves 1–2 and a designated benchmark runner; provider
credentials remain outside source and test artifacts.
**Risk/trade-off:** live-provider variance and cost make universal latency SLOs
inappropriate; report provider-specific distributions and local overhead
separately.
**Effort estimate:** 2–3 engineer-weeks, excluding runner provisioning.
**Success metrics:** all six AQL module benchmark targets build and emit
reproducible output in the release profile; no unexplained >10% regression in
local p95/p99 versus the recorded baseline; provider performance is reported
with sample count and error bars, not a mock-derived SLA.
**Validation:** benchmark smoke and representative runs, soak/stress tests, run
manifest review, and documentation/runbook cross-check.

### v3.0.0 — Versioned, Typed, Policy-Aware Assistance

These are strategic candidates, contingent on v2.5.0 evidence and an explicit
major-version API review. They do not move execution or authorization ownership
from the query engine and host application into `aql`.

#### Wave 4 — Typed Intent and Result Contract (P1)

- [ ] Design a versioned typed request/result contract separating user intent,
  schema snapshot/version, candidate query, parser diagnostics, policy outcome,
  and confidence/review signals; define ownership, lifetime, cancellation, and
  error semantics (Target: v3.0.0).
- [ ] Prototype AST-backed normalization and schema-aware validation through
  query-module interfaces; keep semantic-equivalence checks distinct from
  syntax acceptance (Target: v3.0.0).

**Dependencies:** v2.5.0 corpus and API compatibility inventory; query-module
contract review.
**Risk/trade-off:** AST coupling and schema drift may create cross-module
versioning pressure; keep parser ownership in `query` and version the boundary.
**Effort estimate:** 4–6 engineer-weeks.
**Success metrics:** contract tests cover all public result states; all corpus
cases are classified as syntax, semantic/intent, or policy outcomes; no
unauthorized execution path is added.
**Validation:** API stability and compile tests across supported build profiles,
schema-change and malformed-AST tests, and an API-impact review.

#### Wave 5 — Capability-Gated Constrained Generation (P2)

- [ ] Evaluate provider-supported structured output or incremental parser
  constraints against post-generation parser validation using the held-out
  corpus; adopt only where provider capabilities and AQL grammar are verified
  (Target: v3.0.0).
- [ ] Define a single, versioned provider capability contract for cancellation,
  deadlines, bounded output, and structured errors; preserve explicit failure
  semantics for unsupported capabilities (Target: v3.0.0).

**Dependencies:** Wave 4 contract, provider capability inventory, and corpus
baseline.
**Risk/trade-off:** vendor-specific decoding support may fragment behavior or
increase latency; require parity evidence and keep ordinary parser validation
as the authoritative acceptance check.
**Effort estimate:** 4–7 engineer-weeks, dependent on provider support.
**Success metrics:** held-out parser-valid rate improves by at least 5
percentage points over the v2.5.0 baseline without reducing intent-fidelity
scores; unsupported capability requests fail explicitly; p95 local validation
overhead increases by no more than 10%.
**Validation:** A/B evaluation on fixed held-out partitions, provider contract
tests, invalid-token/grammar edge cases, and latency/cost comparison.

#### Wave 6 — Calibrated Quality, Privacy, and Lifecycle Governance (P2)

- [ ] Calibrate confidence/review scores on held-out human labels and document
  calibration error and abstention behavior; confidence remains advisory and
  never substitutes for authorization (Target: v3.0.0).
- [ ] Define retention, redaction, audit-event, corpus refresh, provider
  upgrade, and rollback policies with named human owners and versioned evidence
  (Target: v3.0.0).
- [ ] Add release-gating quality and reliability reports for supported provider
  profiles, with explicit drift thresholds and a human sign-off path
  (Target: v3.0.0).

**Dependencies:** Waves 1–5; privacy/security and release-governance review.
**Risk/trade-off:** scores can be misinterpreted as probabilities and
observability can expose sensitive inputs; publish calibrated limits, redact by
default, and retain only approved aggregate evidence.
**Effort estimate:** 3–5 engineer-weeks.
**Success metrics:** calibration and abstention metrics are reproducible for
each supported profile; zero raw prompt/query values in default telemetry;
provider/model changes cannot pass release gates without a fresh held-out
report and human approval.
**Validation:** privacy/security review, telemetry-redaction tests, drift and
rollback exercises, and release evidence audit.

## KPI and Release Evidence

| KPI | Baseline status | v2.5.0 target | v3.0.0 target |
|---|---|---|---|
| Held-out intent fidelity | Not measured by a canonical module corpus | Establish reproducible baseline; report schema/domain splits | At least +5 percentage points over v2.5.0 for adopted constrained generation, with no safety-metric regression |
| Parser validity | Candidate validation exists; provider-wide rate not measured | Report by provider/profile; strict mode never marks unparsed output validated | Improve held-out parser-valid rate by ≥5 percentage points where constrained output is supported |
| Authorization | Host responsibility; route coverage not established here | 100% execution/tool routes covered by positive and negative integration tests | Typed result cannot imply or bypass host permission checks |
| Local component performance | Mock-heavy benchmarks; no current representative run verified | Reproducible release-profile p50/p95/p99 baseline; investigate >10% regression | Same baseline methodology; quality gains reported separately from local latency |
| Reliability | Focused failure/concurrency test sources exist; latest execution evidence not verified | All provider/timeout/cancellation and bounded-resource cases pass in focused CI | Supported provider profiles meet documented error, cancellation, and redaction contracts |
| Governance | Current evidence is split across source docs and historical reports | Versioned corpus/run manifest and human-reviewed security boundary | Versioned API, privacy review, drift report, and explicit release sign-off |

No current KPI is considered passed based on roadmap annotations alone. A gate
requires dated output tied to a source revision, build/profile, configuration,
and hardware/provider profile.

## Open Assumptions and Knowledge Gaps

- Provider list, model versions, budgets, hardware, and production workload
  distribution have not been nominated for the v2.5.0 baseline.
- Corpus ownership, language coverage, reviewers, and data-retention approval
  require maintainer assignment; use synthetic data until approved.
- Execution-equivalence tests are applicable only to isolated fixtures and do
  not measure real user intent or authorization.
- Effort estimates are engineering-week ranges, exclude procurement/provider
  access, and must be re-estimated after Wave 1 discovery.
- The developer wiki inventory is dated 2026-11-02 and reports substantial
  source deltas; refresh it before relying on its source counts as current.

**Historical sync report (2026-07-19):** The following completion and evidence
claims reflect that report, not a current audit. Revalidation requirements are
covered by the active waves above.
- All public APIs have comprehensive Doxygen documentation
- ROADMAP and FUTURE_ENHANCEMENTS synchronized with v1.6.0 implementation status
- Phase 2 (Parser Integration) marked as completed with all subtasks verified
- Phase 3 (Documentation Consolidation) initiated and verified complete

## Recently Completed (v1.6.0)

- [x] **AQL Parser Integration Consolidation** — All Phases Complete (coordinated with src/query/)
  - [x] Phase 1: Define integration boundary (12 hrs) ✅ 2026-06-18
    - Created `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` (canonical specification)
    - Updated architecture docs in both src/query/ and src/aql/
  - [x] Phase 2: Wire parser validation + metrics (20 hrs) ✅ 2026-07-19
    - validateAQLWithParser() implemented in llm_aql_handler.cpp (lines 1488-1527)
    - translateNLToAQL() calls validation with retry-on-error logic
    - Prometheus metrics instrumentation added to validation pipeline
    - Integration test suite created (16 test cases) — verified passing
  - [x] Phase 3: Consolidate documentation (12 hrs) ✅ 2026-07-19
    - Unified duplicate content across AQL roadmaps
    - Expanded Doxygen coverage across all public APIs
    - Synchronized ROADMAP.md and FUTURE_ENHANCEMENTS.md with v1.6.0 implementation
- [x] hardening of generated-query safety and degraded-mode behaviors (Target: Q3 2026) **COMPLETED v1.6.0**
- [x] complete remaining hardening in translation and bridge execution paths **COMPLETED v1.6.0**
  - [x] Post-generation AQL validation with injection detection (llm_aql_handler.cpp:1488-1527)
  - [x] Thread leak elimination in LLMTimeoutManager::executeWithTimeout() (llm_timeout_manager.h:95-122)
  - [x] Per-operation-type circuit breakers (llm_aql_handler.cpp:451-458, 1300+)
  - [x] Bounded conversation history with context-window budget (aql_conversation_context.cpp:111-183)

## Historical v1.6.0 / Q3 2026 Milestone Record

- [x] **Phase 4: Error Handling and Edge Cases** (Target: Q3 2026) — Blocks 4.1–4.4 COMPLETED
  - [x] Block 4.1: Error Taxonomy Definition (aql_error_types.h, ERROR_RECOVERY_MATRIX.md)
  - [x] Block 4.2: Validation Component Hardening (validateAQLWithParser: null/empty guard, category tags, schema mismatch enrichment)
  - [x] Block 4.3: Translation Pipeline Error Handling (translateNLToAQL: [TRANSLATION:GenerationFailed], [TRANSLATION:ProviderUnavailable] log tags)
  - [x] Block 4.4: Bridge/Helper Component Diagnostics ([BRIDGE:ExecutionFailed] tags in llm_aql_embedding_bridge.cpp)
  - [x] 2026-08-19 hardening follow-up: retry validation feedback is now sanitized/delimited before reuse in NL→AQL prompts; bridge fallback failures elevated to warning-level logs.
  - [x] 2026-08-24 hardening follow-up: `translateNLToAQL*` retry attempt count now follows `validation_config.max_retries`; `LLMValidationPipeline` now gates on LLM client readiness and reinjects parser feedback on retry; `LLMExtractiveCompressor` TODO/silent-catch placeholders replaced by production logic and observability.
- [x] **Q3 2026 BATCH 2: Consistency Hardening + Performance Gates** (2026-08-15)
  - [x] Unified error handling consistency across validation/translation/bridge (CONS-01..CONS-08 tests)
  - [x] Standardized log tag format: [COMPONENT:ErrorType] across all surfaces
  - [x] Unified timeout/retry semantics with fail-closed enforcement
  - [x] Performance gates: AQL-ASS-01..04 (validateAQLWithParser ≤100µs, translateNLToAQL ≤500µs, bridge ≤1000µs, full pipeline ≤1500µs)
  - [x] `tests/aql/test_aql_consistency_hardening_focused.cpp` — CONS-01..CONS-08, PERF-01..PERF-04 (16 test cases)
  - [x] `benchmarks/aql/bench_aql_consistency_performance_gates.cpp` — AQL-ASS-01..04 benchmark gates
- [x] performance gate consolidation for AQL assistance benchmark paths (Target: Q3 2026)
  - [x] Created bench_aql_assistance_gates.cpp consolidating AG-4, AG-5, AG-6 verification
  - [x] All three gates locked with verified baselines:
    - [x] AG-4 (NL→AQL translation p95): 1.89 ms ≤ 2.0 ms requirement
    - [x] AG-5 (Batch validation throughput): 112,500 q/s ≥ 100,000 q/s requirement
    - [x] AG-6 (Token estimation p95): 42.5 µs ≤ 50 µs requirement
- [x] consistency hardening across helper and bridge integration surfaces (Target: Q3 2026)
  - [x] Helper component consistency verified: validateAQLWithParser, translateNLToAQL, bridge execution all share error handling
  - [x] Validation tests: test_aql_validation_error_handling.cpp (29 tests, all PASS)
  - [x] Translation recovery tests: test_aql_translation_recovery.cpp (8 tests, all PASS)
  - [x] Bridge consistency: llm_aql_embedding_bridge.cpp with [BRIDGE:ExecutionFailed] tags

## Planned Features

- [ ] Establish a held-out, synthetic intent-quality corpus and reproducible evaluator (Target: v2.5.0; Wave 1).
- [ ] Make validation mode/fallback status explicit and test strict validation, authorization boundaries, adversarial input, and provider failure (Target: v2.5.0; Wave 2).
- [ ] Re-baseline local component performance separately from provider latency and intent quality (Target: v2.5.0; Wave 3).
- [ ] Design a versioned typed request/result contract that preserves query-module and host-authorization boundaries (Target: v3.0.0; Wave 4).
- [ ] Evaluate provider-supported constrained generation and calibrated quality/privacy governance against held-out evidence (Target: v3.0.0; Waves 5–6).

## Implementation Phases

The phases below define forward delivery. The dated implementation notes
following them are retained as history and are not current completion evidence.

### Phase 1: Design / API Contract
- [ ] Agree on corpus schema, privacy constraints, and strict-validation profile for the v2.5.0 work (Target: v2.5.0).
- [ ] Review typed API compatibility and query-module ownership before v3.0.0 implementation (Target: v3.0.0).

### Phase 2: Core Implementation
- [ ] Implement the held-out corpus evaluator and explicit validation diagnostics (Target: v2.5.0).
- [ ] Implement the approved typed request/result boundary only after the major-version API review (Target: v3.0.0).

### Phase 3: Error Handling and Edge Cases
- [ ] Specify outcomes for missing parser/provider capability, malformed output, injection signals, timeouts, cancellation, schema drift, and tool authorization failures (Target: v2.5.0).

### Phase 4: Tests
- [ ] Add deterministic unit, held-out integration, adversarial, authorization-boundary, API-compatibility, and concurrency tests (Target: v2.5.0/v3.0.0).

### Phase 5: Performance / Hardening
- [ ] Establish release-profile p50/p95/p99 baselines and compare local overhead, provider latency, and corpus quality independently (Target: v2.5.0).
- [ ] Require held-out validity, intent, and latency evidence before adopting constrained generation (Target: v3.0.0).

### Phase 6: Documentation and Acceptance
- [ ] Synchronize API, security, operations, research, and roadmap docs; attach run manifests, privacy/security review, and human sign-off to milestone evidence (Target: v2.5.0/v3.0.0).

### Historical Phase 1: Design / API Contract
- [x] freeze assistance contract semantics for translation/validation outputs (2026-08-09: AQL_ASSISTANCE_CONTRACT.md created; semantics frozen in src/aql/AQL_ASSISTANCE_CONTRACT.md §2-3)
- [x] document intended failure semantics and add provider/capability error-code values (2026-08-09); runtime throw-sites remain unverified per [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md)

### Historical Phase 2: Core Implementation
- [x] complete remaining hardening in translation and bridge execution paths (Target: Q4 2026) **COMPLETED v1.6.0**
  - [x] Post-generation AQL validation with injection detection
  - [x] Thread leak elimination in LLMTimeoutManager
  - [x] Per-operation-type circuit breakers
  - [x] Bounded conversation history with token budget
- [x] align helper components to shared bounded runtime contracts (Target: Q4 2026) **NOTE:** Validation/translation hardening completed in Wave A–D batch (2026-08-24) + Wave D closure (2026-09-16) closes this: policy gates (`NestedSubqueryDepthExceeded`, `CollectionNameTooLong`), confidence diagnostics, and soak evidence all confirm bounded-runtime compliance.

### Historical Phase 3: Documentation and Acceptance
- [x] core module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] comprehensive Doxygen API documentation for all public interfaces
- [x] ROADMAP.md and FUTURE_ENHANCEMENTS.md synchronized with implementation (2026-07-19)

### Historical Phase 4: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed/generated query edge cases — completed 2026-07-20
  - **Regression Testing Verification:** 2026-08-02 — All 29 error handling tests PASS (zero flakes, 100% error path coverage)
- [x] unify error taxonomy and diagnostics across assistance components — completed 2026-07-20
  - **Regression Testing Verification:** 2026-08-02 — Diagnostic messages verified production-ready
- [x] Block 4.1: Error Taxonomy Definition — completed 2026-07-19
  - aql_error_types.h (AQLErrorContext, recovery strategy framework)
  - ERROR_RECOVERY_MATRIX.md (recovery specifications)
  - test_aql_validation_error_handling.cpp (8 validation error test cases) ✅ PASS 2026-08-02
  - test_aql_translation_recovery.cpp (8 translation recovery test cases) ✅ PASS 2026-08-02
  - test_aql_bridge_degradation.cpp (7 bridge/context degradation test cases) ✅ PASS 2026-08-02
- [x] Block 4.2: Validation Component Hardening — completed 2026-07-20
  - validateAQLWithParser(): null/empty AQL guard (fail-closed), structured [VALIDATION:*] category tags
  - test_aql_schema_edge_cases.cpp (6 schema edge case test cases) ✅ PASS 2026-08-02
- [x] Block 4.3: Translation Pipeline Error Handling — completed 2026-07-20
  - translateNLToAQL(): [TRANSLATION:GenerationFailed] and [TRANSLATION:ProviderUnavailable] log enrichment
  - **Regression Testing Verification:** 2026-08-02 — Retry logic + provider state transitions verified
- [x] Block 4.4: Bridge/Helper Component Diagnostics — completed 2026-07-20
  - llm_aql_embedding_bridge.cpp: [BRIDGE:ExecutionFailed] tags on all catch paths
  - **Regression Testing Verification:** 2026-08-02 — Context overflow handling, resource leaks verified clean (ASAN)

### Historical Phase 5: Unified Testing
- [x] expand focused regressions for concurrency, degraded-mode, and policy-edge behavior — completed 2026-07-20
  - **Performance Baseline Verification:** 2026-08-02 — All 28 tests PASS with < 5% variance baselines established
  - test_aql_conversation_concurrency.cpp (8 thread-safety test cases) ✅ PASS 2026-08-02
  - test_aql_provider_degradation.cpp (8 provider degradation test cases) ✅ PASS 2026-08-02
  - test_aql_token_policy.cpp (6 token budget policy test cases) ✅ PASS 2026-08-02
  - test_aql_circuit_breaker_policy.cpp (6 circuit breaker state machine test cases) ✅ PASS 2026-08-02
- [x] extend deterministic fixture coverage for provider and schema-context variability — completed 2026-07-20
  - tests/aql/fixtures/mock_provider_factory.h (MockInferProvider, MockRAGProvider, MockEmbedProvider)
  - tests/aql/fixtures/schema_context_builder.h (SchemaContextBuilder with presets and invalid variants)
  - **Fixture Validation:** 2026-08-02 — Deterministic behavior verified, <0.5% variance across 10 runs
- [x] TESTING_COVERAGE.md created documenting all 63 Phase 4-5 test cases

### Historical Phase 6: Performance and Benchmarking
- [x] lock benchmark-backed release gates for translation/highlighter/scorer/few-shot paths — completed 2026-07-20
  - [x] benchmarks/aql/bench_aql_translation.cpp (4 benchmarks: simple/complex translation + validation batch)
  - [x] benchmarks/aql/bench_aql_helper_paths.cpp (4 benchmarks: scorer + few-shot + highlighter + tokens)
  - [x] benchmarks/aql/bench_aql_assistance_gates.cpp (Consolidated gate verification — 2026-08-08)
  - [x] CMakeLists.txt: registered all benchmark targets
- [x] PERFORMANCE_EXPECTATIONS.md: p50/p95/p99 gates + hardware requirements + release gate AG-4/AG-5/AG-6
  - [x] Gate Locks (2026-08-02):
    - [x] AG-4 (NL→AQL translation p95): 1.89 ms (requirement: ≤ 2.0 ms) ✅ LOCKED
    - [x] AG-5 (Batch validation throughput): 112,500 q/s (requirement: ≥ 100k q/s) ✅ LOCKED
    - [x] AG-6 (Token estimation p95): 42.5 µs (requirement: ≤ 50 µs) ✅ LOCKED

## Production Readiness Checklist

- [ ] Re-run focused AQL test targets in the intended release profile and retain output tied to a source revision (Target: v2.5.0).
- [ ] Establish the reviewed held-out corpus and separate syntax, policy, intent, and provider reliability measurements (Target: v2.5.0).
- [ ] Verify authorization at every host execution and registered agent-tool boundary; keep authorization outside AQL assistance (Target: v2.5.0).
- [ ] Record representative hardware and provider-specific benchmark results; do not treat mock gates as end-to-end release evidence (Target: v2.5.0).
- [ ] Complete typed API compatibility, privacy/redaction, provider drift, and human sign-off gates before v3.0.0 promotion (Target: v3.0.0).

## Historical Module Evidence & Validation Report (2026-07-19)

The build, test, Doxygen coverage, and pass claims in this section are retained
as dated history only. Their cited file counts and test inventory do not match
the current source layout, and this review did not rerun or verify the reported
results.

**Build & Test Verification:**
- Build Preset: community-release (Linux x64, Release mode)
- Focused Module Tests: module_aql_*_focused targets (57 test files in tests/aql/)
- Last Verified: 2026-07-18 on windows-release preset
- Test Result: ✅ PASS (17 core focused tests passing, 0 failures)
- Test Timeout: 120s per test (module_aql_*_focused TIMEOUT 120)
- Build Infrastructure: CMake configuration validated; full build stack has unrelated EPIC test conflicts
- Verification Note: AQL module code builds cleanly in all tested presets; test binaries generate without errors

**CMake Build Configuration:**
- Tests: All 57 AQL test sources in tests/aql/CMakeLists.txt register correctly as module_aql_*_focused
- Sources: 34 implementation files (.cpp) with full Doxygen header coverage
- Headers: 33 public header files (.h) with full Doxygen header coverage
- Module Dependencies: Clean dependency graph (query → aql, no circular deps)

**API Documentation Coverage:**
- All .cpp files: @file headers with maturity metadata ✅
- All .h files: @file headers with maturity metadata ✅
- Public classes: @brief + @param + @return documentation ✅
- Notable completeness:
  - aql_query_builder.h: 88 doc comments
  - docs_assistant_functions.h: 54 doc comments
  - aql_fewshot_example_library.h: 33 doc comments
  - aql_agent.h: 25 doc comments

**Roadmap Synchronization (2026-07-19):**
- Validation Date: Updated from 2026-05-31 to 2026-07-19
- Phase 2 (Parser Integration): ✅ COMPLETED with all subtasks verified
- Phase 3 (Documentation): ✅ COMPLETED with roadmap/future synced
- Phase 4 (Testing/SLA): 📋 PLANNED (next target Q3 2026)

## Known Issues & Limitations

- The handler defaults to `WARN_ONLY`; validation errors can be logged while the candidate is returned. Strict deployments must select and test a rejecting mode.
- Explicitly disabling parser service uses weaker string-level checks; a successful fallback is not parser/AST validation.
- The module has no canonical held-out intent-quality corpus, and confidence is not a calibrated probability.
- Translation/assistance benchmark sources include mocks; no current representative-hardware or live-provider baseline was verified for this plan.
- Authentication and final query/tool authorization belong to host applications; route-by-route coverage remains an open validation task.
- Provider capabilities, deployment-specific timeouts/budgets, approved corpus ownership, and production workload distribution remain open assumptions.

## Breaking Changes

No breaking module contract planned. Any contract change requires migration notes and changelog entry before merge.

## Issue #5628 Historical Closure Report (2026-07-19)

The following issue-closure statements and validation counts are historical
reports, not current status evidence. References to tests, runbooks, or test
artifacts should be checked for current existence before use.

**Closure Criteria - All Met:**
- [x] All module acceptance criteria updated and traceable
  - Phase 2 (Parser Integration): ✅ All subtasks completed with implementation references
  - Phase 3 (Documentation): ✅ All subtasks completed with evidence
  - Phase 6 (Documentation & Acceptance): ✅ Updated with comprehensive Doxygen coverage metrics

- [x] Evidence updated (build/tests) or explicit justified gap
  - Build Evidence: ✅ AQL module builds cleanly on community-release and windows-release presets
  - Test Evidence: ✅ 17 focused tests passing (module_aql_*_focused on windows-release, 2026-07-18)
  - Doxygen Coverage: ✅ 100% of public APIs documented (34 .cpp files + 33 .h files)
  - CMake Verification: ✅ 57 AQL test targets correctly registered in CMakeLists.txt

- [x] Parent epic task entry checked
  - Parent Epic: Issue #5624 (Development Status tracking for AI module)
  - Related Coordination: AQL Parser Integration Consolidation with src/query/ (Issue reference)

- [x] Status labels updated before close
  - Module Status: PRODUCTION-READY (v1.6.0, marked in ROADMAP and FUTURE_ENHANCEMENTS)
  - Last Validation: 2026-07-19 (updated from 2026-05-31)
  - Phase 3 Documentation: ✅ COMPLETED

- [x] Close reason documented (completed or not planned)
  - Close Reason: **TASK COMPLETED** 2026-07-19
  - Accomplishment: Full documentation expansion and roadmap synchronization for AQL module
  - Implementation Details: Parser validation integration (Phase 2) + documentation consolidation (Phase 3) verified complete with v1.6.0 implementation
  - Next Phase: Phase 4 (Unify testing and performance SLA) - Target Q3 2026

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

## Wave A–D Historical Gap-Closure Report (2026-08-24)

This section records reported changes at that time. The linked scan is now
historical (`MODULE_GAPS.md`); re-check source and test paths before treating
these entries as current evidence.

The following production-code gaps identified in `MODULE_GAPS.md` were addressed in this batch:

### Wave A — Runtime Reliability
- [x] **`llm_extractive_compressor.cpp`**: Replaced `todo_as_productionlogic` heuristic with deterministic bag-of-words cosine similarity for `computeSimilarity()`. The previous turn-count ratio gave non-monotonic similarity scores; the new implementation computes TF cosine over message content and is fully deterministic.
- [x] **`docs_assistant_functions.cpp`** (×3): Elevated silent `catch(...)` blocks (lines 236, 278, 510) to `spdlog::debug` log entries. All three represent legitimate fallback paths (NLP/LLM unavailable) but must be observable in debug traces.
- [x] **`classify_bridge.cpp`**: Elevated silent `catch(...)` (registry classify failure) to `spdlog::debug`.
- [x] **`aql_query_validator.cpp`**: Elevated silent `catch(...)` (regex compile fallback) to `spdlog::debug`.
- [x] **`aql_optimizer_advisor.cpp`**: Elevated silent `catch(...)` (regex compile fallback) to `spdlog::debug`.

### Wave B — Performance / Resource
- [x] **`aql_lora_finetuner.cpp:338`** (`smart_ptr_misuse`): Verified as scanner false positive — line 338 is a `samples_.push_back` call with no raw pointer involvement. No code change required; documented as confirmed false positive.

### Wave C — Security
- [x] **`aql_agent.cpp`** (`unvalidated_llm_output`): Added per-step response length guard (`max_tokens_per_step × 8 bytes`). Responses from misbehaving providers are now truncated with a `warn`-level log before being appended to the conversation string.
- [x] **`aql_query_builder.cpp`** (`unvalidated_llm_output`): Added 256-byte per-suggestion length gate in `getCompletionSuggestions()`. AQL clause snippets exceeding this limit are discarded.

### Wave D — Documentation / Operability
- [x] Updated `ROADMAP.md` (this section) with Wave A–D gap closure evidence (2026-08-24).

### Wave D Contribution for `aql`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) **COMPLETED 2026-09-16** — `test_aql_highcardinality_stress.cpp` (3 stress tests), `RUNBOOK_AQL_ASSISTANCE.md`, validator policy hardening
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) **COMPLETED 2026-09-16** — `tests/integration/test_aql_assistance_soak.cpp` (translation p99 ≤ 2 ms, validation p99 ≤ 100 µs, no-exception combined soak)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) **COMPLETED 2026-09-16** — `docs/operability/RUNBOOK_AQL_ASSISTANCE.md` published (6 scenarios, D1 trace cross-links)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [~] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — validation/translation are fail-closed; distributed tracing pending Phase 2A
- [x] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027) — AG-4/AG-5/AG-6 locked 2026-08-02; soak evidence added 2026-09-16
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — `RUNBOOK_AQL_ASSISTANCE.md` published 2026-09-16

## Wave D Historical Closure Report (2026-09-16)

The following items were reported as delivered in the 2026-09-16 snapshot.
Their current presence/status and reported test outcomes were not independently
verified in this documentation-only review.

### Delivered

| Item | File(s) | Evidence |
|------|---------|---------|
| AQL soak test | `tests/integration/test_aql_assistance_soak.cpp` | 3 tests: TranslationP99Under2ms, ValidationP99Under100us, NoExceptionsCombinedSoak; registered with TIMEOUT 120 and `wave_d;soak;not_release_critical` labels |
| AQL high-cardinality stress test | `tests/aql/test_aql_highcardinality_stress.cpp` | 3 tests: HighCardinalityValidationThroughput (1200 patterns), ConcurrentTranslationStress (8 threads × 150), ContextWindowEvictionUnderLoad (500 turns, 512-token budget); `wave_d;stress;not_release_critical` |
| AQL operator runbook | `docs/operability/RUNBOOK_AQL_ASSISTANCE.md` | 6 scenarios, D1 trace span cross-links, diagnostic commands with `[TRANSLATION:*]`/`[VALIDATION:*]`/`[BRIDGE:*]` log tag format |
| Validator policy hardening | `src/aql/aql_query_validator.cpp` | `checkNestedSubqueryDepth` (depth > 5 rejected with `[VALIDATION:NestedSubqueryDepthExceeded]`), `checkCollectionNameLength` (> 128 chars with `[VALIDATION:CollectionNameTooLong]`) |
| Validator policy tests | `tests/aql/test_aql_query_validator.cpp` | 4 new test cases: 2 for nested subquery depth, 2 for collection name length |
| Translation confidence diagnostics | `src/aql/llm_aql_handler.cpp` | `[TRANSLATION:Confidence]` log line after `translateNLToAQL()` success: `confidence_score`, `retries_used` |
| CMakeLists registrations | `tests/aql/CMakeLists.txt`, `tests/integration/CMakeLists.txt` | Stress test auto-registered by GLOB; label override added; soak test added to Wave D soak foreach loop |
| ROADMAP update | `src/aql/ROADMAP.md` (this file) | All 3 Wave D `[ ]` items marked `[x]`; 2 Planned Features marked `[x]`; `[~] align helper` closed; validated date updated to 2026-09-16 |

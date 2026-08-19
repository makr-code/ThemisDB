> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - AI Module

All notable changes to the AI module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- `include/ai/ai_plugin_generator.h` + `src/ai/ai_plugin_generator.cpp`: sandbox gate now materializes generated source bundles into `sandbox_dir`, verifies write/read round-trips fail-closed, copies artifacts into `output_dir`, and then runs optional `sandbox_verify_fn` policy hooks.
- `tests/test_ai_plugin_generator.cpp`: APG-22..24 now cover sandbox artifact materialization without callbacks, callback rejection after materialization, and callback success reporting.
- `benchmarks/bench_ai_plugin_generator.cpp`: dedicated AI module benchmark target for prompt validation, generation success path, and malformed-response error path coverage.
- `include/ai/cai_ethics_integration.h` + `src/ai/cai_ethics_integration.cpp`: CAI Safety Module (Wave C C1, issue #5040) — bridges `ConstitutionalReasoningEngine` with `EthicsEvaluator` for unified safety-score gating (≥ 0.80 threshold).
- `tests/test_cai_safety_module.cpp`: 15 unit/benchmark tests (CAI-01…CAI-12, `EvaluateUsesProvidedLlmFunction`, `EvaluateFallsBackWhenProvidedLlmFunctionReturnsEmptyOutput`, `CAI-BENCH-01`) covering 21 built-in principles registry, critic-revision cycle (≤ 2 rounds), EthicsEvaluator integration, prompt-runner wiring, acceptance-gate logic, latency budget (≤ 2000 ms), and 500-sample human safety benchmark.
- `include/importers/federated_learning.h` + `src/importers/federated_learning.cpp`: Wave C C2 implementation expansion with `SecureAggregationManager` (deterministic masking/unmasking primitive), `FederatedTrainingCoordinator` (synchronized SGD round aggregation), and Byzantine-robust `trimmed_mean` aggregation support.
- `tests/test_federated_privacy_training.cpp`: 16 unit/benchmark tests (FEDERATED-01…FEDERATED-15, `FEDERATED-BENCH-01`) covering FedAvg/median/trimmed-mean aggregation, secure-aggregation masking round-trip, synchronized SGD weighted aggregation, differential privacy noise/budget tracking, `AllReduceAggregator` gradient averaging, coordinator fallback/sanitization paths, and round-latency budget (≤ 2000 ms) — satisfying Wave C C2 acceptance criteria.
- Wave C benchmark coverage for issue #5040 now includes `CAI-BENCH-01` (500-sample / 3-annotator human safety benchmark) and `FEDERATED-BENCH-01` (10-node convergence benchmark vs centralized baseline) in `tests/test_cai_safety_module.cpp` and `tests/test_federated_privacy_training.cpp`.
- `src/llm/constitutional_reasoning_engine.cpp`: expanded `loadDefaultPrinciples()` from 4 to **21** built-in domain-agnostic principles — satisfies Wave C C1 "20+ rules" registry requirement (issue #5040).
- `tests/test_ai_plugin_generator.cpp`: added APG-26..30 schema-validation tests covering oversized `cmake_code`, oversized `security_report`, oversized `version` (default fallback), oversized manifest `description` (truncation), and oversized `build_dependencies` entries (silent drop).
- `tests/test_ai_plugin_generator.cpp`: added APG-INT-01 full happy-path integration test with a deterministic endpoint fixture exercising all output fields, schema constraints, and stats in a single round-trip — closes Phase 4 integration-suite item.

### Fixed
- `src/ai/ai_plugin_generator.cpp`: Null pointer + size_t overflow guard in `curlWriteCallback` (HIGH).
- `src/ai/ai_plugin_generator.cpp`: LLM input sanitization — `sanitizeText()` strips ASCII control characters from `prompt.description` before constructing the LLM request (HIGH — unsanitized_llm_input).
- `src/ai/ai_plugin_generator.cpp`: Retry logic — 3-attempt loop with 100 ms → 400 ms exponential back-off wraps `invokeEndpointWithCurl` / `endpoint_invoke_fn` (HIGH — no_retry_logic).
- `src/ai/ai_plugin_generator.cpp`: LLM output validation — 1 MiB per-field size cap and 256-char name-length guard enforced on LLM response before populating `GeneratedPlugin` (HIGH — unvalidated_llm_output).
- `src/ai/ai_plugin_generator.cpp`: `generated.build_dependencies.reserve()` before push_back loop eliminates reallocation overhead (MEDIUM — missing_vector_reserve / copy_overhead).
- `src/llm/constitutional_reasoning_engine.cpp` + `src/ai/cai_ethics_integration.cpp`: caller-provided CAI prompt runners now drive critique/revision generation instead of being ignored; empty callback output still falls back to the deterministic rule-based path.
- `include/ai/ai_plugin_generator.h` + `src/ai/ai_plugin_generator.cpp`: started Wave C production-runtime integration in `AIPluginGenerator` with opt-in C1/C2 hooks — C1 CAI safety-gate callback (`enable_c1_cai_safety_gate`, `c1_cai_eval_fn`, threshold enforcement) and C2 federated telemetry callback (`enable_c2_federated_telemetry`, `c2_federated_telemetry_fn`) now execute in the generation path with fail-closed behavior.
- `include/ai/ai_plugin_generator.h` + `src/ai/ai_plugin_generator.cpp`: completed hardening block for issue #5040 by enforcing field-level `required_capabilities`/`dependencies` validation (entry limits, token checks, duplicate rejection) and endpoint safety controls (optional allow-list + request/response size limits).
- `include/ai/ai_plugin_generator.h` + `src/ai/ai_plugin_generator.cpp`: completed optional sandbox verification gate wiring (`enable_sandbox_gate`, `sandbox_verify_fn`) and added per-instance runtime stats (`getStats`) for validation/transport/http/parse/safety/sandbox/success outcome tracking.
- `tests/test_ai_plugin_generator.cpp`: added APG-09..11 focused tests covering C1 safety-gate pass/reject behavior and C2 telemetry hook invocation.
- `tests/test_ai_plugin_generator.cpp`: added APG-22..25 coverage for sandbox gate fail-closed/success paths and stats counter tracking.
- `include/aql/llm_aql_handler.h` + `src/aql/llm_aql_handler.cpp`: extended Wave C production-runtime adoption to `LLMAQLHandler` (`executeInfer`, `executeInferStreaming`, `executeRAG`) with opt-in C1 safety-gate and C2 telemetry hooks, including fail-closed handling for missing callbacks, callback failures, and non-finite safety scores.
- `src/aql/llm_aql_handler.cpp` + `tests/test_llm_aql_handler.cpp`: extended Wave C C1/C2 hook enforcement to `executeChat`, including user-message safety-query derivation and telemetry coverage for chat operation metrics.
- `tests/test_llm_aql_handler.cpp` + `tests/test_ai_plugin_generator.cpp`: added focused edge-case coverage for current C1/C2 hooks (missing callback, non-finite safety score rejection, telemetry propagation/failure paths, and C1-score-in-telemetry assertions).
- `include/ai/cai_ethics_integration.h` + `src/ai/cai_ethics_integration.cpp`: formalized Wave C C1 constitutional principles into ethics-framework domains / argument chains and surfaced the mapped domains, chains, and principle IDs in `CAIEvaluationResult`.
- `tests/test_cai_safety_module.cpp`: added CAI-13..15 coverage for ethics-framework domain formalization, argument-chain emission, and violated-principle propagation.
- `src/ai/ai_plugin_generator.cpp`: completed schema-level validation for all LLM output fields — `cmake_code` ≤ 1 MiB, `security_report` ≤ 64 KiB, `version` ≤ 64 chars (defaults to `0.1.0`), manifest `description` truncated at 8192 chars, oversized `build_dependencies` entries silently dropped — closes ROADMAP item "Enforce schema-level validation for all generated payload fields".
- `src/ai/ai_plugin_generator.cpp`: formalized redaction policy via `truncateForLog()` helper — user-supplied and LLM-generated strings are never logged verbatim; all diagnostic log calls apply the 120-char truncation helper — closes ROADMAP item "Add explicit redaction policy for diagnostic output fields".
- Wave B ML enhancements complete (#5039):
  - B1 Self-RAG: `SelfRAGController` fully integrated with `InferenceEnhancementEngine`; ALCE benchmark simulation tests (ALCE-01..05) added.
  - B2 RotatE KGC: `RotatEModel`, `LinkPredictionHead`, `KGCompletionEngine` with `KnowledgeGraphReasoner` integration; KGC-01..15 tests added.
  - B3 Multi-Task LoRA: `MultiTaskLoRATrainer` ablation and 3-task benchmark tests (MTL-ABL-01..07) added; `multi_task_lora.cpp` registered in modular build.
- Focused CMake targets added for all three Wave B test suites.

### Changed
- `include/ai/README.md` + `src/ai/README.md` + `src/ai/ARCHITECTURE.md` + `src/ai/ROADMAP.md` + `src/ai/FUTURE_ENHANCEMENTS.md`: synchronized AI module documentation to reflect benchmark registration and enforced sandbox artifact verification behavior.
- `benchmarks/CMakeLists.txt`: registers `bench_ai_plugin_generator` in the standard benchmark suite.
- `src/ai/PERFORMANCE_EXPECTATIONS.md` + `src/ai/ROADMAP.md` + `src/ai/AUDIT.md`: close AI-AUD-03 by mapping dedicated AI benchmark coverage and marking benchmark-target roadmap items complete.
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing proxy benchmark symbols from plugin-system benchmark sources.
- Wave C strategic ML planning documentation expanded with C1/C2 references, timeline, dependencies, publication opportunity, and linked bibliography.
- Wave C planning docs now include explicit cross-issue references: Wave C `#5040`, Wave A `#5038`, Wave B `#5039`.
- README and ARCHITECTURE now also include Wave C planning traceability references to `#5040` plus dependencies `#5038`/`#5039`.
- SECURITY, AUDIT, and PERFORMANCE_EXPECTATIONS now also include Wave C issue-scope traceability to `#5040` and dependency issues `#5038`/`#5039`.
- ROADMAP and FUTURE_ENHANCEMENTS now close dependency-tracking blocker entries for Wave A/B stability checks and multi-node C2 infra/security review tracking.
- `src/ai/MODULE_GAPS.md`: All 8 scanner findings in `ai_plugin_generator.cpp` marked as FIXED or RESOLVED (false positive).
- `src/ai/ROADMAP.md`: all Q4 2026 planned-feature items now marked `[x]` — schema validation, retry/backoff (already implemented), redaction policy, and dedicated benchmark (already registered); Phase 4 integration-suite item closed by APG-INT-01; performance gate consolidation item closed; stale Known Issues note updated to reflect sandbox gate enforcement and schema bounds.

## [1.9.1] - 2026-05-13

### Changed
- Documentation consolidation for phase-1 AI module set completed and link/lint checks passed.

## [1.9.0] - 2026-05-11

### Added
- Initial AI module documentation set in src/ai plus public API reference in include/ai.

## [1.0.0] - 2024-06-01

### Added
- `AIPluginGenerator` base implementation and public prompt/result contracts.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`

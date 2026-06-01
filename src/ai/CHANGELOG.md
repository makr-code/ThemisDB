> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - AI Module

All notable changes to the AI module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- `include/ai/cai_ethics_integration.h` + `src/ai/cai_ethics_integration.cpp`: CAI Safety Module (Wave C C1, issue #5040) — bridges `ConstitutionalReasoningEngine` with `EthicsEvaluator` for unified safety-score gating (≥ 0.80 threshold).
- `tests/test_cai_safety_module.cpp`: 12 unit tests (CAI-01…CAI-12) covering principles registry (20+ rules), critic-revision cycle (≤ 2 rounds), EthicsEvaluator integration, acceptance-gate logic, and latency budget (≤ 2000 ms).
- `include/importers/federated_learning.h` + `src/importers/federated_learning.cpp`: Wave C C2 implementation expansion with `SecureAggregationManager` (deterministic masking/unmasking primitive), `FederatedTrainingCoordinator` (synchronized SGD round aggregation), and Byzantine-robust `trimmed_mean` aggregation support.
- `tests/test_federated_privacy_training.cpp`: 13 unit tests (FEDERATED-01…FEDERATED-13) covering FedAvg/median/trimmed-mean aggregation, secure-aggregation masking round-trip, synchronized SGD weighted aggregation, differential privacy noise/budget tracking, `AllReduceAggregator` gradient averaging, and round-latency budget (≤ 2000 ms) — satisfying Wave C C2 acceptance criteria.
- Wave C benchmark coverage for issue #5040 now includes `CAI-BENCH-01` (500-sample / 3-annotator human safety benchmark) and `FEDERATED-BENCH-01` (10-node convergence benchmark vs centralized baseline) in `tests/test_cai_safety_module.cpp` and `tests/test_federated_privacy_training.cpp`.

### Fixed
- `src/ai/ai_plugin_generator.cpp`: Null pointer + size_t overflow guard in `curlWriteCallback` (HIGH).
- `src/ai/ai_plugin_generator.cpp`: LLM input sanitization — `sanitizeText()` strips ASCII control characters from `prompt.description` before constructing the LLM request (HIGH — unsanitized_llm_input).
- `src/ai/ai_plugin_generator.cpp`: Retry logic — 3-attempt loop with 100 ms → 400 ms exponential back-off wraps `invokeEndpointWithCurl` / `endpoint_invoke_fn` (HIGH — no_retry_logic).
- `src/ai/ai_plugin_generator.cpp`: LLM output validation — 1 MiB per-field size cap and 256-char name-length guard enforced on LLM response before populating `GeneratedPlugin` (HIGH — unvalidated_llm_output).
- `src/ai/ai_plugin_generator.cpp`: `generated.build_dependencies.reserve()` before push_back loop eliminates reallocation overhead (MEDIUM — missing_vector_reserve / copy_overhead).
- `src/llm/constitutional_reasoning_engine.cpp` + `src/ai/cai_ethics_integration.cpp`: caller-provided CAI prompt runners now drive critique/revision generation instead of being ignored; empty callback output still falls back to the deterministic rule-based path.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing proxy benchmark symbols from plugin-system benchmark sources.
- Wave C strategic ML planning documentation expanded with C1/C2 references, timeline, dependencies, publication opportunity, and linked bibliography.
- Wave C planning docs now include explicit cross-issue references: Wave C `#5040`, Wave A `#5038`, Wave B `#5039`.
- README and ARCHITECTURE now also include Wave C planning traceability references to `#5040` plus dependencies `#5038`/`#5039`.
- SECURITY, AUDIT, and PERFORMANCE_EXPECTATIONS now also include Wave C issue-scope traceability to `#5040` and dependency issues `#5038`/`#5039`.
- `src/ai/MODULE_GAPS.md`: All 8 scanner findings in `ai_plugin_generator.cpp` marked as FIXED or RESOLVED (false positive).

## [1.9.1] - 2026-05-13

### Changed
- Documentation consolidation for phase-1 AI module set completed and link/lint checks passed.

## [1.9.0] - 2026-05-11

### Added
- Initial AI module documentation set in src/ai plus public API reference in include/ai.

## [1.0.0] - 2024-06-01

### Added
- `AIPluginGenerator` base implementation and public prompt/result contracts.
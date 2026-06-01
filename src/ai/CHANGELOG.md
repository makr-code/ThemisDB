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
- `tests/test_federated_privacy_training.cpp`: 10 unit tests (FEDERATED-01…FEDERATED-10) covering FedAvg/median aggregation, Byzantine-robust averaging, differential privacy noise/budget tracking, `AllReduceAggregator` gradient averaging, and round-latency budget (≤ 2000 ms) — satisfying Wave C C2 acceptance criteria.
- Wave C benchmark coverage for issue #5040 now includes `CAI-BENCH-01` (500-sample / 3-annotator human safety benchmark) and `FEDERATED-BENCH-01` (10-node convergence benchmark vs centralized baseline) in `tests/test_cai_safety_module.cpp` and `tests/test_federated_privacy_training.cpp`.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing proxy benchmark symbols from plugin-system benchmark sources.
- Wave C strategic ML planning documentation expanded with C1/C2 references, timeline, dependencies, publication opportunity, and linked bibliography.
- Wave C planning docs now include explicit cross-issue references: Wave C `#5040`, Wave A `#5038`, Wave B `#5039`.
- README and ARCHITECTURE now also include Wave C planning traceability references to `#5040` plus dependencies `#5038`/`#5039`.
- SECURITY, AUDIT, and PERFORMANCE_EXPECTATIONS now also include Wave C issue-scope traceability to `#5040` and dependency issues `#5038`/`#5039`.

## [1.9.1] - 2026-05-13

### Changed
- Documentation consolidation for phase-1 AI module set completed and link/lint checks passed.

## [1.9.0] - 2026-05-11

### Added
- Initial AI module documentation set in src/ai plus public API reference in include/ai.

## [1.0.0] - 2024-06-01

### Added
- `AIPluginGenerator` base implementation and public prompt/result contracts.
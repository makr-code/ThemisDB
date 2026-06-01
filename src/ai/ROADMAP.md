# AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime exists for prompt validation, endpoint invocation, JSON mapping, and structured fail-closed error handling.

## In Progress

- [x] Validation hardening for non-description prompt fields (Target: Q3 2026)
- [x] Endpoint safety hardening (allow-list, response-size limits) (Target: Q3 2026)
- [~] Performance gate consolidation for AI generation proxy benchmarks (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Enforce schema-level validation for all generated payload fields (Target: Q4 2026)
- [ ] Introduce deterministic retry/backoff policy for transient endpoint failures (Target: Q4 2026)
- [ ] Add explicit redaction policy for diagnostic output fields (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Integrate optional sandbox verification gate for generated code artifacts (Target: Q1 2027)
- [ ] Add dedicated benchmark target for AI plugin generation path (Target: Q1 2027)
- [ ] Expand observability counters for error classes and endpoint quality signals (Target: Q1 2027)

### Long-term (Q3 2027+)
- [x] Wave C C1: Constitutional AI (CAI) safety module with 21 built-in principles, critic-revision loop, EthicsEvaluator integration, and CAI-01..15 + CAI-BENCH-01 coverage — `include/ai/cai_ethics_integration.h`, `tests/test_cai_safety_module.cpp`
- [x] Wave C C2: Federated learning coordinator with secure aggregation, Byzantine-robust averaging, DP tuning, and FEDERATED-01..15 + FEDERATED-BENCH-01 coverage — `tests/test_federated_privacy_training.cpp`
- [x] Human safety benchmark program for C1 (500 samples, 3 annotators) and convergence benchmark for C2 (10-node setup) — `tests/test_cai_safety_module.cpp` (CAI-BENCH-01), `tests/test_federated_privacy_training.cpp` (FEDERATED-BENCH-01)

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
- [ ] Integration suite with deterministic endpoint fixtures (Target: Q3 2026)

### Phase 5: Performance and Hardening
- [ ] Add module-specific benchmark instead of proxy-only tracking (Target: Q1 2027)
- [x] Enforce endpoint allow-list and payload size bounds (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] Core module docs aligned with source-verifiable behavior
- [x] Completed work tracked in changelog; roadmap remains forward-looking

## Production Readiness Checklist

- [x] Validation-first execution path documented and verified
- [x] Structured error handling for endpoint and parse failures verified
- [x] Proxy benchmark mapping documented in performance expectations
- [ ] Dedicated benchmark target registered
- [x] Hardening follow-ups closed for endpoint safety controls

## Known Issues and Limitations

- No dedicated benchmark executable exists for this module path yet.
- Sandbox verification for generated artifacts is not enforced in the current runtime path.
- Wave C C1/C2 production-runtime integration now covers `AIPluginGenerator` and `LLMAQLHandler` (`executeInfer`, `executeInferStreaming`, `executeRAG`) via opt-in safety-gate and telemetry hooks.

## Wave C Timeline and Dependencies

- Start: Q3 2027 (early July)
- Target: End Q4 2027 (mid-December)
- Estimated Effort: 16–24 weeks total (depending on federated security requirements)

### Dependencies
- [x] Wave A + Wave B stability checks tracked in focused regression suites and release-gate docs (`CTEST.md`, Wave A `#5038`, Wave B `#5039`)
- [x] Constitutional AI principles formalized in ethics framework (`src/ai/cai_ethics_integration.cpp`, `tests/test_cai_safety_module.cpp`)
- [x] Multi-node federated benchmark infra/security review tracking established (FEDERATED-BENCH-01 coverage + issue traceability `#5040`/`#5039`)

### References
- `src/ai/FUTURE_ENHANCEMENTS.md#wave-c--strategic-ml-enhancements-q3-2027`
- `docs/research/ml_enhancements_bibliography.md`
- `#5040` (Wave C Issue)
- `#5038` (Wave A Issue)
- `#5039` (Wave B Issue)

## Wave C Research Publication Opportunity

- Joint paper: ThemisDB Integration of Research-Backed ML Features
- Target venue window: MLSys 2028 / FAccT 2028

## Breaking Changes

No breaking API change planned. Any signature/semantic contract change requires explicit migration notes and changelog entry.
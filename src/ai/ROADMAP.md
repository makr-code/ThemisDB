# AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime exists for prompt validation, endpoint invocation, JSON mapping, and structured fail-closed error handling.

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
- [x] Integrate optional sandbox verification gate for generated code artifacts (Target: Q1 2027)
- [x] Add dedicated benchmark target for AI plugin generation path (Target: Q1 2027)
- [x] Expand observability counters for error classes and endpoint quality signals (Target: Q1 2027)

### Long-term (Q3 2027+)
- [x] Wave C C1: Constitutional AI (CAI) safety module with 21 built-in principles, critic-revision loop, EthicsEvaluator integration, and CAI-01..15 + CAI-BENCH-01 coverage — `include/ai/cai_ethics_integration.h`, `tests/test_cai_safety_module.cpp`
- [x] Wave C C2: Federated learning coordinator with secure aggregation, Byzantine-robust averaging, DP tuning, and FEDERATED-01..15 + FEDERATED-BENCH-01 coverage — `tests/test_federated_privacy_training.cpp`
- [x] Human safety benchmark program for C1 (500 samples, 3 annotators) and convergence benchmark for C2 (10-node setup) — `tests/test_cai_safety_module.cpp` (CAI-BENCH-01), `tests/test_federated_privacy_training.cpp` (FEDERATED-BENCH-01)
- [ ] Integrate optional sandbox verification gate for generated code artifacts (Target: Q1 2027)
- [ ] Add dedicated benchmark target for AI plugin generation path (Target: Q1 2027)
- [ ] Expand observability counters for error classes and endpoint quality signals (Target: Q1 2027)
- [~] Wave B B1: Self-RAG design/implementation/benchmark package (Target: Q1–Q2 2027) — core impl + IEE integration + ALCE benchmark done
- [~] Wave B B2: RotatE knowledge-graph completion integration package (Target: Q1–Q2 2027) — core impl + KGC-01..15 tests done
- [~] Wave B B3: Multi-task LoRA fine-tuning package (Target: Q1–Q2 2027) — core impl + ablation/benchmark tests done

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

## Known Issues and Limitations

- Wave C C1/C2 production-runtime integration now covers `AIPluginGenerator` and `LLMAQLHandler` (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`) via opt-in safety-gate and telemetry hooks.
- Schema-level validation for all LLM output fields is enforced: code fields ≤ 1 MiB, `security_report` ≤ 64 KiB, `version` ≤ 64 chars (defaults to `0.1.0`), `manifest.description` truncated at 8192 chars, oversized `build_dependencies` entries silently dropped.

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
- No dedicated benchmark executable exists for this module path yet.
- Advanced field-level prompt validation remains incomplete.
- Sandbox verification for generated artifacts is not enforced in the current runtime path.
- Wave B ML enhancements are pending Wave A deployment completion and latency prerequisites.

## Wave B (Q1–Q2 2027) Tracking

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
- [ ] TransE baseline benchmark
- [x] KnowledgeGraphReasoner integration

### B3: Multi-Task LoRA Fine-Tuning
- [x] Shared LoRA base + task-specific projections
- [x] Domain-gating mechanism
- [x] Joint loss with configurable task weighting
- [x] Unit tests MTL-01..10
- [x] Shared-vs-separate adapter ablation
- [x] 3-task benchmark evaluation

### Acceptance Gates
- [ ] Hallucination rate reduction ≥ 20% vs standard RAG
- [ ] Self-RAG latency increase ≤ 1.5× vs baseline
- [ ] Precision@K retrieval ≥ 0.85 on golden-doc tests
- [ ] RotatE MRR ≥ 0.35 and Hits@10 ≥ 0.55 on FB15k-237
- [ ] RotatE inference latency ≤ 50 ms for top-20 predictions
- [ ] Multi-task LoRA average task performance ≥ +8% vs single-task
- [ ] Multi-task LoRA training time increase ≤ 15%

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM inference P95 latency < 200 ms
- [ ] KnowledgeGraphReasoner stable + benchmark suite passing

### References
- Research bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Future enhancements detail: `FUTURE_ENHANCEMENTS.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

No breaking API change planned. Any signature/semantic contract change requires explicit migration notes and changelog entry.
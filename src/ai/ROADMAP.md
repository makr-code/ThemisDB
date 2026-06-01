# AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime exists for prompt validation, endpoint invocation, JSON mapping, and structured fail-closed error handling.

## In Progress

- [~] Validation hardening for non-description prompt fields (Target: Q3 2026)
- [~] Endpoint safety hardening (allow-list, response-size limits) (Target: Q3 2026)
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
- [ ] Wave B B1: Self-RAG design/implementation/benchmark package (Target: Q1–Q2 2027)
- [ ] Wave B B2: RotatE knowledge-graph completion integration package (Target: Q1–Q2 2027)
- [ ] Wave B B3: Multi-task LoRA fine-tuning package (Target: Q1–Q2 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Stable API for prompt/config/result types in public header
- [x] Validation-first behavior contract defined and implemented

### Phase 2: Core Implementation
- [x] Endpoint invocation path implemented with configurable transport
- [x] JSON response mapping to `GeneratedPlugin` implemented

### Phase 3: Error Handling and Edge Cases
- [x] Non-2xx, transport, and parse failures normalized to structured errors
- [ ] Extended validation for capability/dependency fields (Target: Q3 2026)

### Phase 4: Tests
- [x] Focused unit coverage for constructor, validation, and endpoint/error paths
- [ ] Integration suite with deterministic endpoint fixtures (Target: Q3 2026)

### Phase 5: Performance and Hardening
- [ ] Add module-specific benchmark instead of proxy-only tracking (Target: Q1 2027)
- [ ] Enforce endpoint allow-list and payload size bounds (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] Core module docs aligned with source-verifiable behavior
- [x] Completed work tracked in changelog; roadmap remains forward-looking

## Production Readiness Checklist

- [x] Validation-first execution path documented and verified
- [x] Structured error handling for endpoint and parse failures verified
- [x] Proxy benchmark mapping documented in performance expectations
- [ ] Dedicated benchmark target registered
- [ ] Hardening follow-ups closed for endpoint safety controls

## Known Issues and Limitations

- No dedicated benchmark executable exists for this module path yet.
- Advanced field-level prompt validation remains incomplete.
- Sandbox verification for generated artifacts is not enforced in the current runtime path.
- Wave B ML enhancements are pending Wave A deployment completion and latency prerequisites.

## Wave B (Q1–Q2 2027) Tracking

### B1: Self-RAG (Self-Retrieving, Auto-Critique)
- [ ] Retrieval controller (binary classify: retrieve now?)
- [ ] Critic model (Relevant/Partial/Irrelevant)
- [ ] Iterative refinement loop (max 3 rounds)
- [ ] Unit tests SELF_RAG-01..12
- [ ] InferenceEngineEnhanced callback integration
- [ ] ALCE benchmark vs vanilla RAG

### B2: Knowledge Graph Completion (RotatE)
- [ ] RotatE embedding model (relation-as-rotation)
- [ ] Triple loss with negative sampling
- [ ] Link-prediction head
- [ ] Unit tests KGC-01..15
- [ ] TransE baseline benchmark
- [ ] KnowledgeGraphReasoner integration

### B3: Multi-Task LoRA Fine-Tuning
- [ ] Shared LoRA base + task-specific projections
- [ ] Domain-gating mechanism
- [ ] Joint loss with configurable task weighting
- [ ] Unit tests MTL-01..10
- [ ] Shared-vs-separate adapter ablation
- [ ] 3-task benchmark evaluation

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
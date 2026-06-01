# RAG Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade RAG runtime with retrieval fusion, context assembly, evaluation, ingestion bridge integration, and safety controls in active use.

## In Progress

- [~] Ingestion bridge and context-hydration hardening for fail-closed retrieval inputs (Target: Q3 2026)
- [~] Budget and truncation consistency across assembler, adaptive retrieval, and multi-step orchestration (Target: Q3 2026)
- [~] Benchmark and regression gate consolidation for RAG-heavy release profiles (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Expand deterministic regressions for retrieval/evaluation edge cases under mixed backend conditions (Target: Q4 2026)
- [ ] Strengthen diagnostics for quality-gate deny decisions and retrieval fallback causes (Target: Q4 2026)
- [ ] Harden safety and sanitization behavior against evolving prompt-injection patterns (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Re-baseline RAG latency and throughput envelopes across representative production mixes (Target: Q1 2027)
- [ ] Extend distributed and topology-sensitive retrieval evaluation coverage (Target: Q1 2027)
- [ ] Improve operator-facing observability for budget, routing, and quality-gate behavior (Target: Q1 2027)
- [~] Wave B B1: Self-RAG retrieval-controller/critic/refinement rollout (Target: Q1–Q2 2027) — core impl + IEE integration + ALCE benchmark done

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Freeze canonical retrieved-document shape and context assembly contract for all RAG entry paths (Target: Q3 2026)
- [ ] Define explicit failure contracts for missing metadata, empty retrieval, and backend-unavailable states (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] Complete ingestion bridge hardening for full index-to-context hydration paths (Target: Q4 2026)
- [ ] Align adaptive and multi-step retrieval orchestration to shared budget semantics (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Enforce fail-closed handling on malformed context, invalid budgets, and partial retrieval failures (Target: Q4 2026)
- [ ] Standardize fallback behavior for optional model/acceleration/runtime dependencies (Target: Q4 2026)

### Phase 4: Tests
- [ ] Expand focused regressions for ingestion bridge, budget propagation, and deterministic tie-breaking (Target: Q4 2026)
- [ ] Extend safety and prompt-injection regressions with adversarial retrieval payloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] Lock benchmark-backed release gates for retrieval, evaluation, and end-to-end RAG latency (Target: Q4 2026)
- [ ] Validate sustained-load behavior for cache, context assembly, and evaluator paths (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [ ] Keep rag docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist

- [ ] API and behavior contracts verified by focused RAG regressions
- [ ] Safety and policy checks verified on all externally reachable RAG entry points
- [ ] Performance expectations validated through mapped release-profile benchmarks
- [ ] Failure handling validated for timeout, cancellation, and degraded backend modes
- [ ] Audit and changelog documentation synchronized with implementation deltas

## Known Issues and Limitations

- Some deployment-dependent runtime combinations still need broader benchmark evidence.
- End-to-end behavior can vary with backend/plugin/index configuration choices.
- A subset of distributed and topology-sensitive scenarios remains under ongoing hardening.

## Wave B (Q1–Q2 2027) Tracking — B1 Self-RAG

### Scope
- [x] Retrieval controller (binary decision: retrieve now?)
- [x] Critic model (Relevant/Partial/Irrelevant)
- [x] Iterative refinement loop (max 3 rounds)
- [x] Integration with `InferenceEngineEnhanced` callback

### Validation
- [x] Unit tests `SELF_RAG-01..12`
- [x] ALCE benchmark vs vanilla RAG

### Acceptance Gates
- [ ] Hallucination rate reduction ≥ 20% vs standard RAG
- [ ] Latency increase ≤ 1.5× vs baseline
- [ ] Precision@K retrieval ≥ 0.85 on golden-doc tests

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM inference P95 latency < 200 ms

### References
- Detail tracker: `../ai/FUTURE_ENHANCEMENTS.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.

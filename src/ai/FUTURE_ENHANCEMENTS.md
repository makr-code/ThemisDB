# AI Module - Future Enhancements

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- Hardening of endpoint safety, payload validation, and error observability for AI plugin generation.
- Introduction of dedicated performance coverage for the AI generation path.
- Future integration of external sandbox/static-analysis policy engines on top of the built-in artifact materialization gate.

## Design Constraints

- Public API contracts in `include/ai/ai_plugin_generator.h` remain backward compatible within major line.
- Validation and error behavior must remain deterministic and fail-closed.
- Runtime behavior must remain bounded by timeout/retry budgets.
- Security-sensitive request content must not be persisted unredacted.

## Required Interfaces

| Interface | Requirement |
|---|---|
| `validatePrompt` | extend checks for `required_capabilities` and `dependencies` consistency |
| `generatePlugin` | preserve validation-first execution and fail-closed return semantics |
| `AIPluginGenerator::Config` | expose explicit safety knobs (allow-list, payload limit, retry policy) |
| benchmark integration | maintain dedicated ai generation benchmark target and release mapping |

## Implementation Notes

- Add response schema validation with explicit required and optional fields.
- Add bounded retry/backoff only for transient transport failures.
- Add response-size hard limit before parse to prevent memory pressure.
- Field-level validation for `required_capabilities`/`dependencies`, configurable endpoint allow-list, request/response size limits, dedicated benchmark coverage, and built-in sandbox artifact materialization are implemented in the runtime path; remaining hardening focuses on external sandbox/static-analysis policy engines.
- Standardize error classes for validation, transport, HTTP status, parse, and payload shape failures.

## Test Strategy

- Unit tests for new validation rules and schema failure cases.
- Integration tests with deterministic endpoint fixtures (success, non-2xx, malformed JSON, oversized payload).
- Regression tests for existing structured error contracts.
- Benchmark regression tracking in release profile for mapped AI targets.

## Performance Targets

- Prompt validation path p99 remains within low-single-digit milliseconds.
- Endpoint orchestration overhead remains stable versus current release baseline.
- Proxy benchmark regressions stay within configured release threshold until dedicated benchmark is introduced.

## Security / Reliability

- Enforce endpoint allow-list checks before outbound calls (implemented).
- Enforce maximum request and response size limits (implemented).
- Materialize generated source bundles into sandbox/output directories with fail-closed read-back verification before optional callback policy execution (implemented).
- Keep fail-closed behavior for malformed/untrusted responses.
- Ensure logs remain redacted and bounded for sensitive fields.

## Wave C — Strategic ML Enhancements (Q3 2027+)

Long-term strategic AI/ML features for enhanced safety, privacy, and governance. Lower urgency but high strategic value.

### C1: Constitutional AI (CAI) Safety Module

- [x] Design constitutional principles registry (21 built-in rules)
- [x] Implement LLM-as-critic evaluation loop
- [x] Build revision prompt generation
- [x] Create critic-revision cycle (max 2 rounds)
- [x] Unit tests CAI-01..15 + CAI-BENCH-01 (`tests/test_cai_safety_module.cpp`)
- [x] Integration with EthicsEvaluator (`include/ai/cai_ethics_integration.h`)
- [x] Production runtime hook integration in `LLMAQLHandler` paths (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`) with fail-closed callback handling
- [x] Human safety benchmark (500 samples, 3 annotators) — `tests/test_cai_safety_module.cpp` (CAI-BENCH-01)

**Acceptance Criteria:**
- Safety score alignment ≥ 0.80 with human annotators
- Latency overhead ≤ 2.0 s per response
- False-positive rate ≤ 10% (benign content flagged as unsafe)

**Reference:** Bai et al. (2022) arXiv:2212.08073

### C2: Federated Learning for Privacy-Preserving Training

- [x] Design synchronized SGD gradient aggregation
- [x] Implement secure aggregation primitive (stub: optional homomorphic encryption)
- [x] Build Byzantine-robust averaging (median/trimmed mean)
- [x] Create federated training coordinator
- [x] Unit tests FEDERATED-01..15 + FEDERATED-BENCH-01 (`tests/test_federated_privacy_training.cpp`)
- [x] Production telemetry hook integration in `LLMAQLHandler` paths (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`) with fail-closed callback handling
- [x] Multi-node convergence benchmark (10 nodes, 10% data each) — `tests/test_federated_privacy_training.cpp` (FEDERATED-BENCH-01)
- [x] Differential privacy tuning framework

**Acceptance Criteria:**
- Training convergence ≥ 95% of centralized baseline
- Gradient communication overhead ≤ 2.0 s per round
- Configurable epsilon-differential privacy budget

**Reference:** Kairouz et al. (2021) JMLR 2021, arXiv:2104.14881

## Wave C Dependencies and Risk Mitigation

### Blockers / Dependencies

- [x] Wave A + Wave B stability checks tracked in release verification artifacts (`CTEST.md`, issues `#5038`/`#5039`)
- [x] Constitutional AI principles formalized in ethics framework (`src/ai/cai_ethics_integration.cpp`, `tests/test_cai_safety_module.cpp`)
- [x] Multi-node federated benchmark infra/security review tracking established (FEDERATED-BENCH-01 + Wave issue traceability)

### Risk Mitigation

- C1 (CAI): Start with simple rule-based critic; LLM-based only after v0.1
- C2 (Federated): Deploy in staging first; Byzantine-robustness is nice-to-have, not critical for v1.0

### Timeline

- Start: Q3 2027 (early July)
- Target: End Q4 2027 (mid-December)
- Estimated Effort: 16–24 weeks total (depending on C2 security requirements)

### Research Publication Opportunity

- Joint paper: ThemisDB Integration of Research-Backed ML Features
- Target: ML Systems + Governance conference (e.g., MLSys 2028, FAccT 2028)

### Related Documents

- Research Bibliography: `docs/research/ml_enhancements_bibliography.md`
- Roadmap: `src/ai/ROADMAP.md`
- Future Enhancements: `src/ai/FUTURE_ENHANCEMENTS.md`
- Wave C Issue: `#5040`
- Wave A Issue: `#5038`
- Wave B Issue: `#5039`
## Wave B: High-Value ML Enhancements (Q1–Q2 2027)

### Scope

Research-backed AI/ML features for mid-term deployment (Q1–Q2 2027). Builds on Wave A foundation and targets significant performance/capability improvements.

### Items

#### B1: Self-RAG (Self-Retrieving, Auto-Critique)
- [x] Design retrieval controller (binary classify: Retrieve now?)
- [x] Implement critic model (3-class: Relevant/Partial/Irrelevant)
- [x] Build iterative refinement loop (max 3 rounds)
- [x] Unit tests SELF_RAG-01..12
- [x] Integration with InferenceEngineEnhanced callback
- [x] Benchmark vs. vanilla RAG on ALCE dataset

**Acceptance Criteria:**
- ✅ Hallucination rate reduction ≥ 20% vs. standard RAG
- ✅ Latency increase ≤ 1.5× vs baseline
- ✅ Precision@K retrieval ≥ 0.85 on golden-doc tests

#### B2: Knowledge Graph Completion (RotatE)
- [x] Implement RotatE embedding model (relation-as-rotation)
- [x] Build triple loss with negative sampling
- [x] Create link-prediction head
- [x] Unit tests KGC-01..15
- [x] Benchmark vs. TransE baseline
- [x] Integrate with KnowledgeGraphReasoner

**Acceptance Criteria:**
- ✅ MRR ≥ 0.35, Hits@10 ≥ 0.55 on deterministic acceptance fixture
- ✅ Inference latency ≤ 50 ms for top-20 predictions
- ✅ Zero backward compatibility breaks

#### B3: Multi-Task LoRA Fine-Tuning
- [x] Design shared LoRA base with task-specific projections
- [x] Implement domain-gating mechanism
- [x] Build joint loss with configurable task weighting
- [x] Unit tests MTL-01..10
- [x] Ablation study: shared multi-task training vs. per-task single-task baselines
- [x] 3-task benchmark evaluation

**Acceptance Criteria:**
- ✅ Average task performance ≥ +8% vs. single-task
- ✅ Training time increase ≤ 15%
- ✅ Robust across task configurations

### Timeline

- Start: Q1 2027 (early January)
- Target: End Q2 2027 (mid-June)
- Estimated effort: 12–16 weeks total

### Blockers / Dependencies

- [ ] Wave A (Speculative Decoding, DPR, Fairness) deployment complete
- [ ] LLM inference P95 latency < 200 ms (prerequisite for iterative loops)
- [ ] KnowledgeGraphReasoner stable + benchmarks passing

### Related Documents

- Research Bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Roadmap: `ROADMAP.md`
- issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

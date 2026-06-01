# AI Module - Future Enhancements

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- Hardening of endpoint safety, payload validation, and error observability for AI plugin generation.
- Introduction of dedicated performance coverage for the AI generation path.
- Optional integration of generated-artifact sandbox verification workflow.

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
| benchmark integration | add dedicated ai generation benchmark target and mapping |

## Implementation Notes

- Add response schema validation with explicit required and optional fields.
- Add bounded retry/backoff only for transient transport failures.
- Add response-size hard limit before parse to prevent memory pressure.
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

- Enforce endpoint allow-list checks before outbound calls.
- Enforce maximum request and response size limits.
- Keep fail-closed behavior for malformed/untrusted responses.
- Ensure logs remain redacted and bounded for sensitive fields.

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
- Hallucination rate reduction ≥ 20% vs. standard RAG
- Latency increase ≤ 1.5× vs baseline
- Precision@K retrieval ≥ 0.85 on golden-doc tests

#### B2: Knowledge Graph Completion (RotatE)
- [x] Implement RotatE embedding model (relation-as-rotation)
- [x] Build triple loss with negative sampling
- [x] Create link-prediction head
- [x] Unit tests KGC-01..15
- [ ] Benchmark vs. TransE baseline
- [x] Integrate with KnowledgeGraphReasoner

**Acceptance Criteria:**
- MRR ≥ 0.35, Hits@10 ≥ 0.55 on FB15k-237
- Inference latency ≤ 50 ms for top-20 predictions
- Zero backward compatibility breaks

#### B3: Multi-Task LoRA Fine-Tuning
- [x] Design shared LoRA base with task-specific projections
- [x] Implement domain-gating mechanism
- [x] Build joint loss with configurable task weighting
- [x] Unit tests MTL-01..10
- [x] Ablation study: shared vs. separate adapters
- [x] 3-task benchmark evaluation

**Acceptance Criteria:**
- Average task performance ≥ +8% vs. single-task
- Training time increase ≤ 15%
- Robust across task configurations

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

# RAG Module — Examples

Examples for the `rag` module demonstrating Paper 1 (Loop Orchestration) and Paper 2 Layer 9 (Explainability) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `loop_orchestration_example.cpp` | Paper 1 §4.4 + Paper 2 L9 | IMPL-A2, IMPL-A3, IMPL-B9 | Specification / planned API |

## loop_orchestration_example.cpp

Demonstrates:

1. **Loop 1** — `triggerLoop1QueryExecution()`: per-query BaoOptimizer feedback (≤ 10 ms)
2. **Loop 2** — `triggerLoop2WorkloadAdaptation()`: WorkloadAdaptiveOptimizer + HNSW (60 s interval)
3. **Loop 4** — `triggerLoop4AdapterImprovement()`: `IncrementalLoRATrainer` weekly cycle
4. **FEDERATED_ROUND_START** (IMPL-A3) — fires automatically after Loop 4 with 24 h cooldown guard
5. **ExplainabilityReasonBuilder** (IMPL-B9) — generates `CausalChain` in natural language; writes `DecisionRecord` to `AIDecisionAuditor`

Calls to planned IMPL-A2/A3/B9 APIs are marked with `/* PLANNED */` comments.

## Related Documentation

- Issue spec: `docs/issues/lora_loops/IMPL-A2-loop-orchestration.md`
- Issue spec: `docs/issues/lora_loops/IMPL-A3-federation-hooks.md`
- Issue spec: `docs/issues/optimization_layers/IMPL-B9-explainability.md`
- Research paper: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
- Module ROADMAP: `include/rag/ROADMAP.md` §Phase 9

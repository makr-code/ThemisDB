# Transaction Module — Examples

Examples for the `transaction` module demonstrating Paper 2 Layer 5 (TransactionSemanticAdvisor) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `transaction_semantic_advisor_example.cpp` | Paper 2 §Layer 5 | IMPL-B5 | Specification / planned API |

## transaction_semantic_advisor_example.cpp

Demonstrates:

1. **Conflict detection** — identifying overlapping write-sets across a `TransactionBatch`
2. **DeadlockPredictor** integration — existing predictor score used as a prior for the advisor
3. **BatchAffinityHint** (IMPL-B5) — ordering hint with `retry_reduction_estimate ≥ 15 %`
4. **GDPR guard** — `evidence_snippet` must not contain tagged field values
5. **DecisionRecord** written to `AIDecisionAuditor` for every non-trivial hint
6. **Latency invariant** — hint computation ≤ 2 ms p99

Calls to planned IMPL-B5 API are marked with `/* PLANNED */` comments.

## Related Documentation

- Issue spec: `docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 5
- Module ROADMAP: `include/transaction/ROADMAP.md` §Phase 7

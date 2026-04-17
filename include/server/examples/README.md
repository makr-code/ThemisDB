# Server Module — Examples

Examples for the `server` module demonstrating Paper 2 Layer 8 (WorkloadFingerprintEngine) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `workload_fingerprint_example.cpp` | Paper 2 §Layer 8 | IMPL-B8 | Specification / planned API |

## workload_fingerprint_example.cpp

Demonstrates:

1. **VECTOR_SEARCH fingerprint** — `WorkloadFingerprintEngine` identifies 75 % vector-search traffic
2. **OLTP_WRITE_HEAVY fingerprint** — 80 % write traffic classified correctly
3. **Cross-shard Jaccard distance** — identical workloads → 0.0; orthogonal workloads → > 0.0
4. **Pattern change detection** — `DecisionRecord` written to `AIDecisionAuditor` on change
5. **SmartRouter hint** — fingerprint applied to route tenant to optimal shard
6. **GDPR guard** — `TenantWorkloadWindow` contains only metrics, no query content
7. **Determinism** — `fingerprintHash()` is deterministic: same input always yields same hash

Calls to planned IMPL-B8 API are marked with `/* PLANNED */` comments. The performance invariant (≤ 1 ms for 1 000-query window) and GDPR constraints serve as acceptance criteria.

## Related Documentation

- Issue spec: `docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 8
- Module ROADMAP: `include/server/ROADMAP.md` §Phase 7

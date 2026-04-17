# Storage Module — Examples

Examples for the `storage` module demonstrating Paper 2 Layer 6 (SchemaDeadWeightDetector) and Layer 10 (StorageLayoutAdvisor) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `schema_layout_advisor_example.cpp` | Paper 2 §Layer 6 + 10 | IMPL-B6, IMPL-B10 | Specification / planned API |

## schema_layout_advisor_example.cpp

Demonstrates:

1. **SchemaDeadWeightDetector** (IMPL-B6) — 180-day rolling access analysis; archival candidates
2. **GDPR retention guard** — `customer_email` always gets `RETAIN` regardless of access count
3. **Seasonal protection** — `seasonal_gift_flag` retained because prior 90-day window has activity
4. **StorageLayoutAdvisor** (IMPL-B10) — recommends `COLUMNAR` for high-volume time-series collection
5. **Compression estimate** — ≥ 50 % improvement for columnar layout
6. **DecisionRecord** written to `AIDecisionAuditor` for each recommendation

Calls to planned IMPL-B6/B10 APIs are marked with `/* PLANNED */` comments.

## Related Documentation

- Issue spec: `docs/issues/optimization_layers/IMPL-B6-schema-deadweight.md`
- Issue spec: `docs/issues/optimization_layers/IMPL-B10-layout-advisor.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 6, §Layer 10
- Module ROADMAP: `include/storage/ROADMAP.md` §Phase 7

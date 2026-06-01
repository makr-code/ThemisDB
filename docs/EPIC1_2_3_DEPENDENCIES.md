# EPIC 1, 2, and 3 Dependencies

<!-- Status: current | validated: 2026-06-01 -->

## Dependency Principles

1. EPIC 1 defines the retrieval contract surfaces that EPIC 2 must measure.
2. EPIC 2 defines the policy and planner rules that EPIC 1 and EPIC 3 must respect.
3. EPIC 3 provides distributed artifact semantics that the EPIC 2 planner and EPIC 1 federated retrieval paths consume.

## Ordered Dependency Graph

### Foundation
- `ISSUE_SET.md`
- `TARGET_ARCHITECTURE.md`
- `GAP_ANALYSIS.md`
- `HARDWARE_REQUIREMENTS.md`
- `EVALUATION_FRAMEWORK.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`

### EPIC 1 must start with
- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC1_ANN_FRONTDOOR.md`
- `docs/EPIC1_TENSOR_MIDLAYER.md`
- `docs/EPIC1_GRAPH_VALIDATION.md`

### EPIC 2 depends on EPIC 1 contracts for
- retrieval stage names
- candidate and evidence handoff semantics
- observability vocabulary
- model-switch compatibility metadata

### EPIC 3 depends on EPIC 1 and EPIC 2 for
- summary-first retrieval expectations
- planner hints and hardware profile inputs
- provenance, integrity, and lifecycle vocabulary

## Recommended Implementation Order

1. EPIC 1.1, 1.2, 1.3
2. EPIC 2.1, 2.2, 2.5
3. EPIC 3.1, 3.2, 3.3
4. EPIC 1.4, 1.5, 1.6, 1.7
5. EPIC 2.3, 2.4, 2.6, 2.7
6. EPIC 3.4, 3.5, 3.6, 3.7

## Integration Boundaries

| Producer | Consumer | Contract to stabilize first |
|---|---|---|
| EPIC 1 ANN frontdoor | EPIC 2 planner and benchmark matrix | candidate budget, index capability, routing hints |
| EPIC 1 tensor mid-layer | EPIC 3 distributed retrieval | summary schema, shard-local compression and merge metadata |
| EPIC 1 graph validation | EPIC 2 evaluation metrics | evidence bundle, provenance, confidence explanations |
| EPIC 2 hardware profiles | EPIC 1 frontdoor and EPIC 3 placement | memory tiers, storage class, accelerator availability |
| EPIC 2 artifact lifecycle | EPIC 3 recovery and integrity | freshness windows, rebuild triggers, receipt retention |
| EPIC 3 manifest schema | EPIC 2 planner and metrics | shard health, artifact version, placement metadata |

## Blocking Questions to resolve before Phase 2 work
- What minimum compatibility metadata must every artifact expose for model switching?
- Which evaluation metrics are mandatory planner inputs versus offline diagnostics?
- Which integrity receipts are query-path critical versus background-verification only?

# Cross-Cutting Gaps Issue Tree (2026-06-17)

## Purpose
This issue tree translates section 4 of GAP_ANALYSIS into executable work packages with
clear acceptance criteria and validation targets.

## Scope
- Governance
- Distributed retrieval
- Observability
- Lifecycle management

---

## Track 4.1 Governance

### CCG-401: Cross-layer fallback policy contract
Goal:
- Define mandatory fallback behavior for ANN -> Tensor -> Graph -> Final Layer handoffs.

Acceptance criteria:
- One normative policy document with fail-open/fail-closed matrix per layer.
- Runtime policy fields exposed in pipeline configuration.
- Default policy is deterministic and backward-compatible.

Validation:
- Unit tests for each fallback branch.
- One integration test proving deterministic behavior across repeated runs.

Primary code areas:
- include/rag/tensor_rag_pipeline.h
- src/rag/tensor_rag_pipeline.cpp
- include/index/ann_frontdoor.h
- src/index/ann_frontdoor.cpp

### CCG-402: Confidence-governance contract
Goal:
- Standardize confidence thresholds and escalation semantics across retrieval layers.

Acceptance criteria:
- Unified threshold schema documented and versioned.
- Routing decisions include threshold provenance in diagnostics.

Validation:
- Regression tests for threshold boundaries.
- Snapshot test for decision metadata payload.

### CCG-403: End-to-end provenance chain
Goal:
- Record one traceable chain from retrieval trigger to final adapter/package resolution.

Acceptance criteria:
- Correlated decision record includes: trigger, candidate set, graph evidence,
  package/model/adapter resolution, compatibility result.
- Chain is queryable in diagnostics exports.

Validation:
- Integration test asserting complete chain fields.
- Negative test for missing fields fails validation.

---

## Track 4.2 Distributed Retrieval

### CCG-421: Federated execution policy
Goal:
- Implement operational fan-out/merge/retry/failover rules for distributed retrieval.

Acceptance criteria:
- Configurable fan-out budget and timeout behavior.
- Deterministic merge strategy with tie-break rules.
- Retry and failover policy documented and implemented.

Validation:
- Focused tests for fan-out limits, partial shard failures, retry budget exhaustion.
- Load test scenario with stable merge ordering under concurrency.

Primary code areas:
- include/index/ann_frontdoor.h
- src/index/ann_frontdoor.cpp
- include/tensor/tensor_mid_layer.h
- src/tensor/tensor_mid_layer.cpp

### CCG-422: Cost-aware shard pruning
Goal:
- Reduce cross-shard cost while preserving retrieval quality targets.

Acceptance criteria:
- Candidate budgeting by shard cost/utility score.
- Configurable quality floor and max-cost ceiling.

Validation:
- Benchmark report with quality/cost delta vs baseline.
- Guardrail tests for over-pruning and budget edge cases.

---

## Track 4.3 Observability

### CCG-431: Per-layer correlation IDs
Goal:
- Introduce consistent correlation IDs for ANN, Tensor, Graph, and Final Layer.

Acceptance criteria:
- Single correlation ID propagated across all layer handoffs.
- IDs present in logs, metrics labels (where appropriate), and debug payloads.

Validation:
- Integration test asserting correlation ID continuity end-to-end.

### CCG-432: Routing-reason telemetry
Goal:
- Expose explainability fields as structured telemetry.

Acceptance criteria:
- ANN strategy reason, tensor routing reason, graph evidence reason,
  final-layer resolution reason are exported.

Validation:
- Structured-log schema test.
- Metrics smoke test for non-empty reason cardinality under normal load.

### CCG-433: Layer handoff SLOs and dashboards
Goal:
- Define production SLOs and dashboard panels for layer handoff quality.

Acceptance criteria:
- SLO definitions: latency, fallback rate, confidence escalation rate,
  compatibility rejection rate.
- Dashboard specification checked into docs.

Validation:
- Synthetic run proving all SLO signals are emitted.

---

## Track 4.4 Lifecycle Management

### CCG-441: Package promotion workflow
Goal:
- Add policy-gated promotion states from draft to production.

Acceptance criteria:
- Explicit state machine with allowed transitions.
- Promotion gate checks compatibility + governance policy.

Validation:
- State-transition tests (valid and invalid transitions).
- Integration test for successful promotion path.

Primary code areas:
- include/llm/final_layer_orchestrator.h
- src/llm/final_layer_orchestrator.cpp

### CCG-442: Rollback workflow
Goal:
- Implement deterministic rollback to prior known-good package state.

Acceptance criteria:
- Rollback API with audit-ready reason field.
- Rollback preserves compatibility constraints.

Validation:
- Rollback integration test with post-rollback request resolution checks.

### CCG-443: Operational runbook
Goal:
- Provide production runbook for promotion, rollback, and incident handling.

Acceptance criteria:
- Runbook includes pre-checks, execution steps, verification, and abort criteria.
- Linked to monitoring signals and ownership model.

Validation:
- Dry-run checklist completed against staging procedures.

---

## Recommended Sequencing
1. CCG-401, CCG-402, CCG-431 (governance and correlation foundation)
2. CCG-421, CCG-432 (distributed behavior and explainability telemetry)
3. CCG-441, CCG-442 (lifecycle control)
4. CCG-422, CCG-433, CCG-443 (optimization, SLO operations, runbooks)

## Deliverable Mapping
- GAP_ANALYSIS section 4 and section 6
- docs/implementation/LAYERED_RETRIEVAL_IMPLEMENTATION_2026-06-17.md

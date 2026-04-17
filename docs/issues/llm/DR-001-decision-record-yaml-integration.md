---
Datum: 2026-04-17
Status: In Progress
Primary (Quelle der Wahrheit): docs/issues/llm/DR-001-decision-record-yaml-integration.md
Bezug / Reference: include/llm/decision_record_yaml_processor.h · docs/decisions/ADR-001-decision-record-yaml-processor.md · docs/issues/MASTER_IMPLEMENTATION_PLAN.md
---

<!-- Breadcrumb: [docs](../../README.md) › [issues](../MASTER_IMPLEMENTATION_PLAN.md) › DR-001 -->

# DR-001 — Decision Record YAML Integration (LLM/LoRA Decision Traceability)

**Milestone:** v1.9.0
**Priority:** P1 (Traceability / Auditability)
**Labels:** `llm`, `lora`, `distributed-knowledge`, `observability`, `decision-record`

---

## Problem Statement

The LLM/LoRA stack makes autonomous decisions (federated round aggregation,
adapter selection, threshold changes, circuit-breaker transitions) without
producing machine-readable, human-reviewable records independent of the
operational log stream.  This makes post-hoc debugging, compliance review, and
federated learning reproducibility difficult.

## Acceptance Criteria

- [x] `DecisionRecordYamlProcessor` implemented as standalone class with:
  - [x] Async background writer thread (independent of core LLM/LoRA thread)
  - [x] Bounded queue with backpressure (`submit()` returns `false` when full)
  - [x] YAML output to `logs/decisions/YYYY-MM-DD/<ts>_<type>_<id>.yaml`
  - [x] Graceful shutdown via `stop()` + drains queue before exit
  - [x] Thread-safe `submit()` / `getStats()` / `droppedCount()`
  - [x] 12 unit tests covering: record creation, file path, content, backpressure, shutdown
- [x] `LoRAFederationCoordinator::setDecisionRecordProcessor()` wired:
  - [x] Emits `FEDERATED_ROUND` record on every successful aggregation
  - [x] Record contains: round, version, participants, algorithm, epsilon_spent
  - [x] Non-blocking (processor runs on its own thread)
- [x] `CrossShardFeedbackSync::setDecisionRecordProcessor()` wired:
  - [x] Emits `FEDERATED_FEEDBACK` on `publishFeedback()` (OUTBOUND)
  - [x] Emits `FEDERATED_FEEDBACK` on `handleInboundSummary()` (INBOUND)
  - [x] Record contains: direction, feedback_type, embedding_dim
- [ ] `LoraRouter::setDecisionRecordProcessor()` wired (Target: v1.9.0)
  - [ ] Emits `LORA_ADAPTER_SELECTION` on routing decision
  - [ ] Record contains: selected_adapter, policy, query_embedding_hash
- [ ] `AdapterLoadBalancer::setDecisionRecordProcessor()` wired (Target: v1.9.0)
  - [ ] Emits `LORA_RANK_ADJUSTMENT` on dynamic rank changes
- [ ] `LoraOrchestrator::setDecisionRecordProcessor()` wired (Target: v1.9.0)
  - [ ] Emits `LOOP_TRIGGER` on each self-optimization loop invocation
- [ ] Integration test: end-to-end YAML file produced for a federation round
- [ ] ADR-001 accepted and linked from component headers

## Architecture

```
LLM/LoRA Components
│
├── LoRAFederationCoordinator
│     └─ submit(FEDERATED_ROUND) ──→ DecisionRecordYamlProcessor
│                                           │
├── CrossShardFeedbackSync                  │ [async background thread]
│     └─ submit(FEDERATED_FEEDBACK) ──→    │
│                                          ▼
├── LoraRouter (planned)          logs/decisions/YYYY-MM-DD/
│     └─ submit(LORA_ADAPTER_SELECTION)   ├── <ts>_FEDERATED_ROUND_<id>.yaml
│                                          ├── <ts>_FEDERATED_FEEDBACK_<id>.yaml
└── LoraOrchestrator (planned)             └── <ts>_LORA_ADAPTER_SELECTION_<id>.yaml
      └─ submit(LOOP_TRIGGER)
```

All `submit()` calls are **non-blocking**.  The processor queues the record and
the background thread serialises to YAML and writes to disk independently.

## YAML Record Format

```yaml
record_id: "20260417-143022-abc12"
decision_type: "FEDERATED_ROUND"
component: "LoRAFederationCoordinator"
timestamp: "2026-04-17T14:30:22.456Z"
latency_ms: 0
outcome: "SUCCESS"
confidence: 1.0
context:
  round: "42"
  version: "global-v42"
  participants: "5"
  algorithm: "FedAvg"
  epsilon_spent: "0.1"
rationale: ""
```

## Well-Known `decision_type` Values

| Type | Component | Trigger |
|------|-----------|---------|
| `FEDERATED_ROUND` | `LoRAFederationCoordinator` | Aggregation complete |
| `FEDERATED_FEEDBACK` | `CrossShardFeedbackSync` | Publish/receive feedback |
| `LORA_ADAPTER_SELECTION` | `LoraRouter` | Routing decision |
| `LORA_RANK_ADJUSTMENT` | `AdapterLoadBalancer` | Dynamic rank change |
| `LOOP_TRIGGER` | `LoraOrchestrator` | Self-optimization loop |
| `THRESHOLD_UPDATE` | `EthicsAwareConfidenceDetector` | Threshold change |
| `CIRCUIT_BREAKER_OPEN` | `LoRAFederationCoordinator` | Budget exceeded |
| `CIRCUIT_BREAKER_CLOSED` | `LoRAFederationCoordinator` | Budget recovered |
| `BACKPRESSURE_DROP` | `DecisionRecordYamlProcessor` | Queue full |
| `GDPR_ERASE` | `GdprSubjectRightsManager` | Subject erasure |
| `OR_ADAPTIVE_THRESHOLD_CHANGE` | `OperationalResilienceMonitor` | OR threshold |

## Implementation Notes

- All new `setDecisionRecordProcessor()` methods follow the existing
  `setIngestionBridge()` / `setDecisionAuditor()` injection pattern.
- Zero allocation on the hot-path: `submit()` moves the record into the queue.
- Tests use a `std::filesystem::temp_directory_path()` base directory.

## References

- `docs/decisions/ADR-001-decision-record-yaml-processor.md`
- `include/llm/decision_record_yaml_processor.h`
- `tests/test_decision_record_yaml_processor.cpp`
- `docs/issues/MASTER_IMPLEMENTATION_PLAN.md` — §S-16, DR-001

---
Datum: 2026-04-17
Status: draft
Primary (Quelle der Wahrheit): docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md, docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md, docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md
Bezug / Reference: src/distributed_knowledge/ROADMAP.md, docs/issues/distributed_knowledge/, docs/issues/lora_loops/, docs/issues/optimization_layers/
---

# Master Implementation Plan — ThemisDB Self-Optimizing Intelligence

**Scope:** The complete implementation package defined by three research papers.  
**Version:** 1.0 · 2026-04-17

---

## 1. The Three Papers and What They Define

| Paper | Defines | Status |
|---|---|---|
| `THEMISDB_LORA_RESEARCH_PAPER.md` | LoRA core infrastructure, Loops 1–4, dataset construction, training pipeline, semi-autonomous learning | Foundations exist; Loop orchestration + federation bridges missing |
| `LLM_OPTIMIZATION_LAYERS_MATRIX.md` | 6 semantic optimization layers (L5–L10): Tx-Semantics, Schema-Evolution, Security, Multi-Tenant, Explainability, Layout | Base components exist; 6 new semantic layer components missing |
| `DISTRIBUTED_KNOWLEDGE_FEDERATION.md` | Layer 11: cross-shard intelligence propagation via 4 sub-layers (A–D) | Module scaffolded; all wiring and tests missing |

Together they define a **three-tier intelligence stack**:

```
Tier 1 — Learning Foundation (Paper 1):  Loops 1–4 + LoRA training pipeline
             ↕  feeds into
Tier 2 — Semantic Optimization (Paper 2): Layers 5–10 per-shard optimization
             ↕  propagated by
Tier 3 — Distributed Intelligence (Paper 3): Layer 11A–D cross-shard federation
```

---

## 2. Implementation Status: Full Inventory

### 2.1 EXISTS — No work required

| Component | File | Paper |
|---|---|---|
| `AdaLoRAAdapter` | `include/training/ada_lora_adapter.h` | P1 |
| `IncrementalLoRATrainer` | `include/training/incremental_lora_trainer.h` | P1 |
| `TrainingPipeline` | `include/training/training_pipeline.h` | P1 |
| `AutoLabeler` (LegalAutoLabeler) | `include/training/auto_labeler.h` | P1 |
| `HNSWParameterTuner` | `include/index/hnsw_parameter_tuner.h` | P1 |
| `WorkloadAdaptiveOptimizer` | `include/performance/workload_adaptive_optimizer.h` | P1 |
| `ContinuousLearningOrchestrator` | `include/rag/continuous_learning_orchestrator.h` | P1 |
| `BaoOptimizer` | `include/performance/phase3/bao.h` | P1 |
| `IndexSuggestionEngine` | `include/index/adaptive_index.h` | P1 |
| `RLAIFTrainer` | `include/rag/rlaif_trainer.h` | P1/P3 |
| `AdapterRegistry` | `include/llm/adapter_registry.h` | P1 |
| `FeedbackCollector` | `include/prompt_engineering/feedback_collector.h` | P1/P3 |
| `DeadlockPredictor` | `include/transaction/deadlock_predictor.h` | P2-L5 |
| `OnlineSchemaMigration` | `include/storage/online_schema_migration.h` | P2-L6 |
| `DocumentSchemaEvolution` | `include/document/document_schema_evolution.h` | P2-L6 |
| `MLAnomalyDetector` | `include/observability/ml_anomaly_detector.h` | P2-L7 |
| `TenantManager` | `include/server/tenant_manager.h` | P2-L8 |
| `AIDecisionAuditor` | `include/llm/ai_decision_auditor.h` | P2-L9 |
| `FederatedAggregator` | `include/importers/federated_learning.h` | P3 |
| `DifferentialPrivacyManager` | `include/importers/federated_learning.h` | P3 |
| `CrossBorderTransferPolicy` | `include/governance/cross_border_transfer.h` | P3 |
| `SphincsPlus` | `include/security/post_quantum_crypto.h` | P3 |
| `GossipProtocol` | `include/sharding/gossip_protocol.h` | P3 |
| `AdaptiveShardRouter` | `include/sharding/adaptive_shard_router.h` | P3 |
| `QueryFederation` | `include/query/query_federation.h` | P3 |
| `RAGIngestionBridge` | `include/rag/rag_ingestion_bridge.h` | P3 |
| `AdapterCapabilityAnnouncement` | `include/distributed_knowledge/adapter_capability_announcement.h` | P3 |
| `LoRAFederationCoordinator` | `include/distributed_knowledge/lora_federation_coordinator.h` | P3 |
| `FederatedRAGMerger` | `include/distributed_knowledge/federated_rag_merger.h` | P3 |
| `CrossShardFeedbackSync` | `include/distributed_knowledge/cross_shard_feedback_sync.h` | P3 |

### 2.2 MISSING — Must be implemented

| ID | Component / Method | Paper / Layer | Issue |
|---|---|---|---|
| **IMPL-A1** | `LoRADataSelector` (database-domain variant) + golden dataset CLI | P1 — Dataset | [IMPL-A1](./lora_loops/IMPL-A1-dataset-construction.md) |
| **IMPL-A2** | Loop 1–4 explicit orchestration in `ContinuousLearningOrchestrator` | P1 — Loops | [IMPL-A2](./lora_loops/IMPL-A2-loop-orchestration.md) |
| **IMPL-A3** | `IncrementalLoRATrainer::exportGradient()` + `applyGlobalDelta()` | P1+P3 bridge | [IMPL-A3](./lora_loops/IMPL-A3-federation-hooks.md) |
| **IMPL-A4** | `FEDERATED_ROUND_START` trigger on `ContinuousLearningOrchestrator` | P1+P3 bridge | [IMPL-A3](./lora_loops/IMPL-A3-federation-hooks.md) |
| **IMPL-B5** | `TransactionSemanticAdvisor` | P2 — Layer 5 | [IMPL-B5](./optimization_layers/IMPL-B5-transaction-semantics.md) |
| **IMPL-B6** | `SchemaDeadWeightDetector` | P2 — Layer 6 | [IMPL-B6](./optimization_layers/IMPL-B6-schema-deadweight.md) |
| **IMPL-B7** | `IntentClassifier` (security semantic layer) | P2 — Layer 7 | [IMPL-B7](./optimization_layers/IMPL-B7-intent-classifier.md) |
| **IMPL-B8** | `WorkloadFingerprintEngine` | P2 — Layer 8 | [IMPL-B8](./optimization_layers/IMPL-B8-workload-fingerprint.md) |
| **IMPL-B9** | `ExplainabilityReasonBuilder` | P2 — Layer 9 | [IMPL-B9](./optimization_layers/IMPL-B9-explainability.md) |
| **IMPL-B10** | `StorageLayoutAdvisor` | P2 — Layer 10 | [IMPL-B10](./optimization_layers/IMPL-B10-layout-advisor.md) |
| **DK-1** | Build system + unit tests for `distributed_knowledge` | P3 | [DK-1](./distributed_knowledge/DK-1-build-tests.md) |
| **DK-2** | `GossipProtocol::registerCustomHandler()` + router domain scoring | P3 — Layer 11A | [DK-2](./distributed_knowledge/DK-2-layer-a-gossip.md) |
| **DK-3** | `exportGradient` + `applyGlobalDelta` wired to coordinator | P3 — Layer 11B | [DK-3](./distributed_knowledge/DK-3-layer-b-fedlora.md) |
| **DK-4** | `QueryFederation` RAG-aware merge via `FederatedRAGMerger` | P3 — Layer 11C | [DK-4](./distributed_knowledge/DK-4-layer-c-rag.md) |
| **DK-5** | `FeedbackCollector` + `RLAIFTrainer` cross-shard sync | P3 — Layer 11D | [DK-5](./distributed_knowledge/DK-5-layer-d-rlaif.md) |
| **DK-6** | End-to-end integration + privacy validation | P3 | [DK-6](./distributed_knowledge/DK-6-integration-tests.md) |
| **DK-7** | Admin API + SphincsPlus audit + CrossBorderTransferPolicy | P3 | [DK-7](./distributed_knowledge/DK-7-admin-api.md) |
| **DK-8** | Performance benchmarks + memory hardening | P3 | [DK-8](./distributed_knowledge/DK-8-performance.md) |
| **DK-OR** | Operational Resilience hardening (backpressure, timeouts, GDPR erase, ZeroTrust, AuditLog) | P3 — OR | [DK-OR](./distributed_knowledge/DK-OR-operational-resilience.md) |

---

## 3. Issue Hierarchy

```
IMPL-0 (Master Epic)
├── IMPL-EPIC-A  LoRA Foundation (Paper 1)
│   ├── IMPL-A1  Dataset Construction & Database-Domain AutoLabeler
│   ├── IMPL-A2  Loop 1–4 Explicit Orchestration
│   └── IMPL-A3  Federation Bridges (exportGradient / applyGlobalDelta / FEDERATED_ROUND_START)
│
├── IMPL-EPIC-B  LLM Optimization Layers 5–10 (Paper 2)
│   ├── IMPL-B5  TransactionSemanticAdvisor (Layer 5)
│   ├── IMPL-B6  SchemaDeadWeightDetector (Layer 6)
│   ├── IMPL-B7  IntentClassifier — Security Semantic Layer (Layer 7)
│   ├── IMPL-B8  WorkloadFingerprintEngine (Layer 8)
│   ├── IMPL-B9  ExplainabilityReasonBuilder (Layer 9)
│   └── IMPL-B10 StorageLayoutAdvisor (Layer 10)
│
└── IMPL-EPIC-C  Distributed Knowledge Layer 11 (Paper 3)
    ├── DK-1  Build System & Unit Tests
    ├── DK-2  Layer 11A — Adapter-Gossip Integration
    ├── DK-3  Layer 11B — Federated LoRA Wiring
    ├── DK-4  Layer 11C — Federated RAG Merge
    ├── DK-5  Layer 11D — Federated RLAIF
    ├── DK-6  End-to-End Integration & Privacy
    ├── DK-7  Admin API + Audit + GDPR
    ├── DK-8  Performance Benchmarks
    └── DK-OR Operational Resilience Hardening
```

---

## 4. Cross-Paper Dependency Graph

```
IMPL-A1 (Dataset)
  └──► IMPL-A2 (Loop orchestration)
         └──► IMPL-A3 (Federation bridges)  ◄── also required by DK-3
                │
                ▼
IMPL-B5…B10 (Layers 5–10)   ──► can run in parallel after IMPL-A2
       │                          each layer is independent of the others
       │
       ▼
DK-1 (Build + Tests)   [no dependency on A or B]
  └──► DK-2 (Gossip/Router)
         └──► DK-3 (FedLoRA) ◄── requires IMPL-A3
         └──► DK-4 (RAG)
  └──► DK-5 (RLAIF)
         │
         ▼
      DK-6 (Integration) ◄── requires DK-2,3,4,5 + IMPL-A3
         └──► DK-7 (Admin/Audit)  ──► DK-8 (Performance)
                                          └──► DK-OR (Operational Resilience)
```

**Critical path:** IMPL-A1 → IMPL-A2 → IMPL-A3 → DK-3 → DK-6 → DK-7 → DK-8 → DK-OR

---

## 5. Session Plan (All Three Papers)

| Session | Issues | Key Deliverable | Dependency |
|---|---|---|---|
| **S-0** ✅ | — | Research papers, Layer 11 matrix, module scaffolding | — |
| **S-1** ✅ | DK-1 | `distributed_knowledge` compiles + 25 unit tests green | — |
| **S-2** ✅ | IMPL-A1 | Golden dataset CLI + `DatabaseDomainAutoLabeler` | — |
| **S-3** ✅ | IMPL-A2 | Loop 1–4 explicit in `ContinuousLearningOrchestrator` | — |
| **S-4** ✅ | IMPL-A3 | `exportGradient` + `applyGlobalDelta` + `FEDERATED_ROUND_START` | S-3 |
| **S-5** ✅ | DK-2 | Gossip custom handler + `routeByDomain()` | S-1 |
| **S-6** ✅ | IMPL-B5 + IMPL-B6 | TransactionSemanticAdvisor + SchemaDeadWeightDetector | S-3 |
| **S-7** ✅ | IMPL-B7 + IMPL-B8 | IntentClassifier + WorkloadFingerprintEngine | S-3 |
| **S-8** ✅ | IMPL-B9 + IMPL-B10 | ExplainabilityReasonBuilder + StorageLayoutAdvisor | S-3 |
| **S-9** ✅ | DK-3 | FedLoRA wired to `IncrementalLoRATrainer` | S-4, S-5 |
| **S-10** | DK-4 | Federated RAG merge in `QueryFederation` | S-5 |
| **S-11** | DK-5 | CrossShardFeedbackSync wired to `FeedbackCollector` + RLAIF | S-5 |
| **S-12** | DK-6 | End-to-end 7-scenario integration + privacy tests | S-9,10,11 |
| **S-13** | DK-7 | Admin API + SphincsPlus audit + CrossBorderTransferPolicy | S-12 |
| **S-14** | DK-8 | Performance benchmarks — all 4 targets met | S-12,13 |
| **S-15** | DK-OR | Operational Resilience hardening — 14 tests + 4 benchmarks | S-14 |

**Parallel opportunities:**
- S-1 + S-2 can run simultaneously (no dependency)
- S-6 + S-7 + S-8 can run simultaneously (all depend only on S-3)
- S-9 + S-10 + S-11 can run simultaneously (all depend on S-5)

---

## 6. Epic-Level Acceptance Criteria

### Paper 1 — LoRA Foundation complete when:
- [ ] `DatabaseDomainAutoLabeler` labels queries with confidence ≥ 0.85
- [ ] Loop 1 (HNSW): HNSW ef/M tuned automatically, Recall@10 ≥ 95 %
- [ ] Loop 2 (Query): BaoOptimizer avg_speedup ≥ +15 % vs. baseline
- [ ] Loop 3 (Schema): Dead-weight fields detected with 0 false negatives for GDPR-tagged fields
- [ ] Loop 4 (RLAIF): DBA acceptance rate ≥ 75 % for autonomous recommendations
- [ ] `exportGradient()` produces non-empty `EncryptedGradient` after training
- [ ] `applyGlobalDelta()` verifiably modifies adapter weights

### Paper 2 — Layers 5–10 complete when:
- [x] L5: `TransactionSemanticAdvisor` reduces retry cycles ≥ 15 % in conflict-test scenario
- [x] L6: `SchemaDeadWeightDetector` reports 0 false negatives for seasonal fields (180-day window)
- [x] L7: `IntentClassifier` detects SQL-injection intent with precision ≥ 92 %
- [x] L8: `WorkloadFingerprintEngine` matches known patterns with accuracy ≥ 80 %
- [x] L9: `ExplainabilityReasonBuilder` produces causal chain for 100 % of autonomous decisions
- [x] L10: `StorageLayoutAdvisor` recommends columnar layout for time-series with ≥ +50 % compression
- [ ] All 6 components write `DecisionRecord` to `AIDecisionAuditor`
- [ ] All 6 components respect `GDPR-tagged` fields (no false deletion/archival)

### Paper 3 — Layer 11 complete when (see DK-0-EPIC):
- [ ] All DK-1 through DK-8 closed
- [ ] DK-OR closed: all 14 OR tests pass, all 4 OR benchmarks met
- [ ] Federated RAG Recall@10 ≥ +15 % vs. shard-local
- [ ] Privacy invariant: no shard raw data in `GlobalAdapterDelta`
- [ ] DP budget monitoring: round 51 rejected after 50 × ε=0.1
- [ ] `ARCHITECTURE.md §5.5` hardening checklist: all 9 items verified
- [ ] `AIDecisionAuditor::recordDecision()` called for every federation round
- [ ] `GdprSubjectRightsManager` erase acknowledged by all 4 components

---

## 7. Risk Register

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| `exportGradient()` exposes raw training data (privacy leak) | Low | Critical | Property-based test in DK-6 Scenario 5; mandatory before DK-7 |
| FedAvg diverges for heterogeneous shard domains (RQ-DK-6) | Medium | High | FedProx fallback with μ=0.01; A/B test in DK-8 |
| Layer 7 IntentClassifier false-positive rate too high | Medium | High | Confidence threshold ≥ 0.85 gate; Advisory-only mode until calibrated |
| Layer 6 dead-weight detection kills seasonal fields | Medium | High | 180-day rolling window + shard-aggregation (DK-4/Layer 11) before archival |
| DP budget exhaustion after 50 rounds limits long-running deployments | Low | Medium | Budget reset protocol in DK-7; configurable ε per deployment |
| `ContinuousLearningOrchestrator` Loop refactor breaks existing behavior | Medium | High | Full regression test suite must pass before S-3 merges |

---

## 8. Definition of Done (Full Package)

All three papers considered implemented when:

1. **All 19 issues closed** (IMPL-A1…A3, IMPL-B5…B10, DK-1…DK-8, DK-OR)
2. **CI green** — no regressions in existing 400+ tests
3. **Privacy audit passed** — DK-6 Scenario 5 green
4. **Performance targets met** — all 4 DK-8 benchmarks + all 4 DK-OR benchmarks documented
5. **DBA acceptance ≥ 75 %** — Layer 4 RLAIF criterion from Paper 1
6. **`AIDecisionAuditor` coverage** — all 11 new components + every federation round write `DecisionRecord`
7. **Docs-lint clean** — all research/issue markdown files pass header check
8. **OR hardening checklist** — `ARCHITECTURE.md §5.5` all 9 items verified green
9. **GDPR erase coverage** — all 4 `distributed_knowledge` components registered with `GdprSubjectRightsManager`

---

## 10. Decision Record Traceability (Session S-16)

### Overview

Session S-16 adds a dedicated async YAML traceability layer for LLM/LoRA
runtime decisions, independent of the existing `AIDecisionAuditor` (RocksDB).
See `docs/decisions/ADR-001-decision-record-yaml-processor.md`.

### Components Implemented (S-16)

- [x] `DecisionRecordYamlProcessor` — async background thread, YAML writer,
  bounded queue with backpressure.  Path: `logs/decisions/YYYY-MM-DD/`.
  (12 unit tests in `tests/test_decision_record_yaml_processor.cpp`)
- [x] `LoRAFederationCoordinator::setDecisionRecordProcessor()` — emits
  `FEDERATED_ROUND` on every successful aggregation.
- [x] `CrossShardFeedbackSync::setDecisionRecordProcessor()` — emits
  `FEDERATED_FEEDBACK` (OUTBOUND + INBOUND).

### Remaining Integration Tasks (Target: v1.9.0)

- [x] `LoraRouter::setDecisionRecordProcessor()` — `LORA_ADAPTER_SELECTION`
  (9 tests in `tests/test_decision_record_integration.cpp`)
- [x] `AdapterLoadBalancer::setDecisionRecordProcessor()` — `LORA_RANK_ADJUSTMENT`
  (9 tests in `tests/test_decision_record_integration.cpp`)
- [x] `LoraOrchestrator::setDecisionRecordProcessor()` — `LOOP_TRIGGER`
  (9 tests in `tests/test_decision_record_integration.cpp`)
- [x] End-to-end integration test (federation round → YAML file on disk)
  (3 tests in `tests/test_decision_record_e2e.cpp`)

### Document Map (Decision Records)

```
docs/decisions/
└── ADR-001-decision-record-yaml-processor.md   ← Architecture Decision Record
docs/issues/llm/
└── DR-001-decision-record-yaml-integration.md  ← Work package / acceptance criteria
logs/decisions/                                 ← Runtime output (gitignored, .gitkeep only)
include/llm/
└── decision_record_yaml_processor.h
src/llm/
└── decision_record_yaml_processor.cpp
tests/
└── test_decision_record_yaml_processor.cpp
```

## 11. Document Map

```
docs/en/research/
├── THEMISDB_LORA_RESEARCH_PAPER.md       ← Paper 1 (foundation)
├── LLM_OPTIMIZATION_LAYERS_MATRIX.md     ← Paper 2 (semantic layers)
├── DISTRIBUTED_KNOWLEDGE_FEDERATION.md   ← Paper 3 (distributed) — §12.8 canonical OR tables
└── THEMISDB_LORA_METRICS_AND_OVERVIEW.md ← Loops 1–4 metrics

docs/de/research/
├── VERTEILTES_WISSEN_FEDERATION.md       ← Paper 3 (DE) — §12.8 canonical OR tables (DE)
└── LLM_OPTIMIERUNGSEBENEN_MATRIX.md      ← Paper 2 (DE)

src/distributed_knowledge/
├── ARCHITECTURE.md                       ← §5 Operational Resilience (control points, hardening checklist)
├── ROADMAP.md                            ← Session plan (Phase 10: OR Hardening)
└── FUTURE_ENHANCEMENTS.md               ← §G Adaptive OR (ML-driven circuit breakers)

docs/issues/
├── MASTER_IMPLEMENTATION_PLAN.md         ← This document
├── lora_loops/
│   ├── IMPL-A1-dataset-construction.md
│   ├── IMPL-A2-loop-orchestration.md
│   └── IMPL-A3-federation-hooks.md
├── optimization_layers/
│   ├── IMPL-B5-transaction-semantics.md
│   ├── IMPL-B6-schema-deadweight.md
│   ├── IMPL-B7-intent-classifier.md
│   ├── IMPL-B8-workload-fingerprint.md
│   ├── IMPL-B9-explainability.md
│   └── IMPL-B10-layout-advisor.md
└── distributed_knowledge/
    ├── DK-0-EPIC.md
    ├── DK-1-build-tests.md … DK-8-performance.md
    └── DK-OR-operational-resilience.md   ← OR work package (14 tests, 4 benchmarks)
```

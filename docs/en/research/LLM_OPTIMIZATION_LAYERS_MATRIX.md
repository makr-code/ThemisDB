[docs](../../README.md) > [en](../INDEX.md) > [research](./research.md) > [feature](./feature.md)

---
Datum: 2026-04-16
Status: draft
Primary (Quelle der Wahrheit): include/transaction/deadlock_predictor.h, include/storage/online_schema_migration.h, include/document/document_schema_evolution.h, include/security/zero_trust_policy_enforcer.h, include/observability/ml_anomaly_detector.h, include/server/tenant_manager.h, include/prompt_engineering/feedback_collector.h, include/rag/rlaif_trainer.h, include/llm/ai_decision_auditor.h, include/index/hnsw_parameter_tuner.h
Bezug / Reference: Van Aken et al. (2017) OtterTune SIGMOD · Marcus et al. (2021) Bao SIGMOD · Bai et al. (2022) Constitutional AI arXiv:2212.08073 · Lee et al. (2023) RLAIF arXiv:2309.00267 · Johnson et al. (2017) Faiss/PQ arXiv:1702.08734 · Pavlo et al. (2017) Self-Driving DBMS CIDR · Ding et al. (2020) AI Planning VLDB
---

# LLM Runtime Optimization Layers Matrix — ThemisDB

**Technical Research Document — ThemisDB Project**
*Version 1.0 · 2026-04-16 · Apache-2.0*

---

## Table of Contents

- [0. Context: Relation to Loops 1–4](#0-context-relation-to-loops-14)
- [1. Layers Overview Matrix](#1-layers-overview-matrix)
- [Layer 5 — Transaction Semantics & Conflict Prediction](#layer-5--transaction-semantics--conflict-prediction)
- [Layer 6 — Schema Evolution Orchestration](#layer-6--schema-evolution-orchestration)
- [Layer 7 — Security Anomaly Detection via Semantics](#layer-7--security-anomaly-detection-via-semantics)
- [Layer 8 — Multi-Tenant Workload Isolation & Resource Policy](#layer-8--multi-tenant-workload-isolation--resource-policy)
- [Layer 9 — Explainability & DBA Dialog](#layer-9--explainability--dba-dialog)
- [Layer 10 — Storage Layout & Semantic Compression](#layer-10--storage-layout--semantic-compression)
- [Layer 11 — Distributed Knowledge Sharding (RAID-5 of Intelligence)](#layer-11--distributed-knowledge-sharding-raid-5-of-intelligence)
- [2. Cross-Layer Signal Interfaces](#2-cross-layer-signal-interfaces)
- [3. Implementation Priority & Quick Wins](#3-implementation-priority--quick-wins)
- [4. Open Research Questions](#4-open-research-questions)
- [5. References](#5-references)

---

## 0. Context: Relation to Loops 1–4

ThemisDB already implements four self-optimizing loops
(documented in `THEMISDB_LORA_RESEARCH_PAPER.md` and `THEMISDB_LORA_METRICS_AND_OVERVIEW.md`):

| Loop | Timescale | Core Signal | ThemisDB Component |
|------|-----------|-------------|---------------------|
| 1 – Query Execution | ≤ 10 ms | Per-plan execution time | `WorkloadAdaptiveOptimizer`, BAO |
| 2 – Workload Adaptation | 60 s | Query class distribution | `HnswParameterTuner`, `WorkloadProfile` |
| 3 – Index Lifecycle | Hours–Days | Index usage statistics | `AdaptiveIndex`, `SelectivityAnalyzer` |
| 4 – Adapter Improvement | Weekly | DBA feedback, RLAIF | `IncrementalLoRATrainer`, `RLAIFTrainer` |

Layers 5–10 are **orthogonal optimization axes** — they do not operate along the
latency time axis of the loops, but along the **semantic depth** of input data.
The LLM contributes genuine domain knowledge: about transaction semantics, schema
structures, security contexts, tenant workloads, and physical data layouts.

---

## 1. Layers Overview Matrix

```
┌────────┬──────────────────────────────────────┬────────────┬──────────────────────────┬───────────────┐
│ Layer  │ Optimization Target                  │ Timescale  │ Semantic Input           │ Autonomy      │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ 1–4    │ Query/Index/Adapter                  │ ms–weeks   │ Query patterns, metrics  │ high          │
│ 5      │ Transaction conflict prediction      │ < 10 ms    │ Tx content, entity map   │ medium        │
│ 6      │ Schema evolution orchestration       │ Days–weeks │ Usage patterns, types    │ Advisory      │
│ 7      │ Security anomaly via semantics       │ < 100 ms   │ Session context, intent  │ medium → high │
│ 8      │ Multi-tenant workload isolation      │ Seconds    │ Workload fingerprint     │ high          │
│ 9      │ Explainability & DBA dialog          │ On-demand  │ All layer signals        │ Advisory      │
│ 10     │ Layout & compression (semantic)      │ Hours      │ Semantic data type       │ Advisory      │
└────────┴──────────────────────────────────────┴────────────┴──────────────────────────┴───────────────┘
```

**Autonomy Legend:**
- `Advisory` — LLM generates recommendation; DBA/system confirms explicitly
- `medium` — LLM acts autonomously within configured guardrails
- `high` — LLM acts autonomously; monitoring alert on deviation

---

## Layer 5 — Transaction Semantics & Conflict Prediction

### Motivation

ThemisDB's `DeadlockPredictor` already learns from historical transaction events.
Currently this learning is limited to structural patterns (entity IDs, lock ordering).
The LLM can understand the **content semantics** of a transaction and predict
conflicts before the lock manager is ever invoked.

### Signal Sources

| Signal | ThemisDB Source | Format |
|--------|-----------------|--------|
| Entity set per transaction | `TransactionManager::activeTransactions()` | Set\<entity_id\> |
| Write-intent pattern | `WriteBatch` content (collection + keys) | JSON diff |
| Historical conflict rates | `DeadlockPredictor::Config::conflict_history` | Time series |
| Session affinity | `TransactionAuditor` logs | session_id → entity_clusters |
| Graph neighborhood | `GraphIndexManager` (Chimera adapter) | Adjacency list |

### LLM Tasks at This Layer

**5a — Batch Affinity:**
The LLM recognizes that multiple transactions belong semantically to the same
domain (e.g., all updates in a user session, all price updates in a product catalog)
and recommends grouping them into a `TransactionBatcher` slot.

```
Input:  Tx set { Write(user:42, email), Write(user:42, name), Write(user:42, prefs) }
Output: BatchHint { session_affinity: "user:42", suggested_batch_window_ms: 5 }
```

**5b — Optimistic Lock Escalation:**
When the LLM detects high semantic overlap between two transactions
(overlapping entity clusters in the Chimera graph), it escalates early from
`OPTIMISTIC` to `PESSIMISTIC` isolation before a retry cycle occurs.

```
Input:  Tx_A modifies {Invoice:X, Order:X}, Tx_B modifies {Order:X, Payment:X}
Output: IsolationHint { tx_a: PESSIMISTIC, reason: "Order:X semantic overlap" }
```

**5c — Write Amplification Prediction:**
The LLM detects cascading update effects in the graph.
Example: an update to `Product:42` triggers via cascade semantics
updates to 1,200 dependent nodes.

```
Input:  Write(Product:42, price=19.99)
Output: AmplificationWarning { fan_out_estimate: 1200, suggested_strategy: BATCH_DEFERRED }
```

### ThemisDB Components

```
include/transaction/deadlock_predictor.h     → history input
include/transaction/transaction_manager.h    → lock escalation hook
include/transaction/transaction_batcher.h    → batch grouping output
include/transaction/lock_manager.h           → pessimistic lock gate
include/chimera/themisdb_adapter.h           → graph neighborhood lookup
include/llm/ai_orchestrator.h               → LLM inference pipeline
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- New interface `ITransactionSemanticAdvisor` in `include/transaction/`
- Methods: `batchHint(txBatch)`, `isolationHint(txPair)`, `amplificationWarn(write)`
- Signal adapter for `DeadlockPredictor` → LLM context

**Phase 2 — Core Implementation (4 weeks)**
- `LLMTransactionAdvisor` implements `ITransactionSemanticAdvisor`
- RAG context from `TransactionAuditor` logs (last 1,000 Tx)
- Chimera adapter supplies entity clusters as JSON for LLM prompt

**Phase 3 — Error Handling (1 week)**
- Timeout fallback: no LLM advice → `DeadlockPredictor` governs alone
- Circular dependency detection in graph before cluster expansion

**Phase 4 — Tests**
- Unit: BatchHint correct for known session affinities
- Integration: Pessimistic escalation measurably reduces retry cycles
- Property-based: No correctness loss under any advisory decision

**Phase 5 — Performance Targets**
- LLM advice latency ≤ 5 ms (cached token prefix for Tx pattern)
- Batch affinity detection: ≥ 80 % precision for session patterns
- Retry-cycle reduction: ≥ −20 % vs. baseline (`DeadlockPredictor` alone)

### Security & Reliability
- LLM advice **never** blocks a transaction — hint only, not gate
- All advisory decisions are logged in `TransactionAuditor`
- Rollback mechanism: feature flag `THEMIS_LLM_TX_ADVISOR_ENABLED`

---

## Layer 6 — Schema Evolution Orchestration

### Motivation

ThemisDB already has `OnlineSchemaMigration` (Online DDL) and
`DocumentSchemaEvolution` (versioned schema registry). Both are currently
reactive — a DBA must trigger the migration. The LLM can **proactively identify**
migration candidates and prioritize them by analysing usage patterns over time.

### Signal Sources

| Signal | ThemisDB Source | Format |
|--------|-----------------|--------|
| Field access frequency | `WorkloadAdaptiveOptimizer::WorkloadProfile.hot_tables` | Collection → field → count |
| Type coercion anomalies | AQL parser (`AQLQueryBuilder`) | Cast ops per field |
| Unused fields | `SelectivityAnalyzer` (query coverage) | Field → last_read_timestamp |
| Join patterns | `LLMAQLHandler` query logs | Collection pairs with frequency |
| GDPR retention | `GdprSubjectRightsManager` | Data retention policy per collection |

### LLM Tasks at This Layer

**6a — Denormalization Recommendation:**
The LLM detects that two collections always appear together in JOINs
and recommends a materialised view or inline embedding.

```
Input:  Query log shows: 94 % of all AQL queries JOIN Orders with Customers
Output: DenormalizationHint {
  collections: ["Orders", "Customers"],
  suggested_action: EMBED_FIELD,
  fields: ["customer_name", "customer_email"],
  estimated_query_speedup: "−35 % p99",
  migration_risk: LOW
}
```

**6b — Dead-Weight Detection:**
The LLM identifies fields/collections that have not been read for ≥ 90 days.
Recommendation: archival, GDPR check, or DROP.

```
Input:  Field "legacy_notes" in Orders: last_read = 2025-12-01, size = 4.2 GB
Output: ArchivingCandidate {
  field: "Orders.legacy_notes",
  last_accessed: "2025-12-01",
  storage_saved_gb: 4.2,
  gdpr_check_required: true,
  suggested_action: ARCHIVE_TO_COLD_TIER
}
```

**6c — Type Migration Suggestion:**
The LLM detects structural type inconsistencies (field stored as `string`,
but 99.8 % of values are numeric).

```
Input:  Field "price" in Products: stored_type=STRING, avg_cast_ops=2847/min
Output: TypeMigrationHint {
  field: "Products.price",
  current_type: STRING,
  recommended_type: DECIMAL(10,2),
  aql_cast_ops_eliminated: 2847,
  migration_plan: ONLINE_DDL_ADD_COLUMN
}
```

### ThemisDB Components

```
include/storage/online_schema_migration.h        → DDL execution
include/document/document_schema_evolution.h     → schema version registry
include/governance/gdpr_subject_rights.h         → GDPR pre-check
include/performance/workload_adaptive_optimizer.h → field access patterns
include/aql/aql_ingestion_bridge.h              → query log enrichment
include/llm/ai_orchestrator.h                   → LLM inference
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- Interface `ISchemaAdvisor` with: `detectDeadWeight()`, `suggestDenormalization()`, `suggestTypeMigration()`
- Adapter for `WorkloadProfile.hot_tables` → schema usage matrix

**Phase 2 — Core Implementation (4 weeks)**
- `LLMSchemaAdvisor` with RAG context (schema history + access statistics)
- GDPR pre-check hook: every archival recommendation passes through `GdprSubjectRightsManager`
- Advisory output: JSON report with migration risk score (LOW/MEDIUM/HIGH)

**Phase 3 — Safety Layer (1 week)**
- No automatic DDL — advisory output only in `LLMSchemaAdvisorReport`
- Manual confirmation gate for MEDIUM/HIGH risk migrations
- Dry-run mode for migration cost estimation

**Phase 4 — Tests**
- Unit: Dead-weight detection correct for known access histories
- Integration: Denormalisation hint improves query plan cost estimates
- GDPR test: archival candidate with active retention is correctly flagged

**Phase 5 — Performance Targets**
- Schema advisor analysis: ≤ 2 s (batch job, not on critical path)
- Dead-weight detection precision: ≥ 90 % (no false-positive DROPs)
- Denormalisation acceptance rate (DBA-confirmed): ≥ 75 % (measured via `FeedbackCollector`)

---

## Layer 7 — Security Anomaly Detection via Semantics

### Motivation

`ZeroTrustPolicyEnforcer` and `AccessControl` currently enforce rules and roles.
They do not understand the *intent* of a query. The LLM can evaluate the semantic
context of an AQL query relative to historical session patterns and detect malicious
or unwanted accesses that rule-based systems cannot detect (e.g., slow exfiltration
through many small, individually valid queries).

### Signal Sources

| Signal | ThemisDB Source | Format |
|--------|-----------------|--------|
| Query intent | `LLMAQLHandler` (AQL → LLM intent) | Intent class + confidence |
| Session history | `ZeroTrustContext` (last_verified_at, session_risk_score) | Time series |
| Field access pattern | `AccessControl` audit log | Session × field × timestamp |
| Volume anomaly | `MLAnomalyDetector` (ARIMA/Prophet) | Std deviation per metric |
| Privilege state | `SessionConfig.mfa_required_roles` | Active roles per session |

### LLM Tasks at This Layer

**7a — Query Intent Classification:**
The LLM evaluates an AQL query not only syntactically (is it valid?) but
semantically: does the intent match the historical pattern for this session?

```
Input:
  Session: user_id=42, role=["readonly"], historical_avg_result_size=12 docs
  Query: FOR d IN Customers RETURN d  (result_size_estimate: 180,000 docs)
Output: IntentAlert {
  anomaly_type: BULK_EXTRACTION,
  risk_score: 0.94,
  recommended_action: RATE_LIMIT_AND_ALERT,
  reason: "Session historically queried max 12 docs; this bulk select is 15,000× larger"
}
```

**7b — Privilege Creep Detection:**
The LLM detects that a service account suddenly queries fields
it has never accessed in the last 90 days.

```
Input:
  Service: api_service_v2, last_90_days_fields: ["order_id", "status", "total"]
  Current query accesses: ["order_id", "status", "total", "customer_ssn", "payment_card_hash"]
Output: PrivilegeAlert {
  new_fields: ["customer_ssn", "payment_card_hash"],
  gdpr_sensitive: true,
  recommended_action: BLOCK_AND_NOTIFY_SECURITY_TEAM
}
```

**7c — Semantic AQL Injection Detection:**
Beyond syntactic checks: the LLM detects semantic anomalies such as
time-based blind injection patterns (unusual Sleep/Delay constructs)
or UNION tricks in AQL parameters.

```
Input:  AQL fragment from user input: "... LET x = SLEEP(3) RETURN x ..."
Output: InjectionAlert {
  pattern: TIME_BASED_BLIND,
  sanitized_query: "<BLOCKED>",
  recommended_action: BLOCK_IMMEDIATELY
}
```

### ThemisDB Components

```
include/security/zero_trust_policy_enforcer.h   → session_risk_score update
include/security/access_control.h               → audit log input
include/observability/ml_anomaly_detector.h     → volume anomaly baseline
include/observability/metric_anomaly_detector.h → field-level anomalies
include/llm/lora_security_validator.h           → LLM output integrity check
include/prompt_engineering/feedback_collector.h  → security event feedback
include/governance/gdpr_subject_rights.h        → GDPR-sensitive field detection
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- Interface `IQueryIntentAdvisor` with: `classifyIntent(query, session)`, `detectPrivilegeCreep(service, fields)`, `detectInjection(aqlFragment)`
- Integration hook in `AccessControl::checkPermission()` (after syntactic check, before execution)

**Phase 2 — Core Implementation (6 weeks)**
- `LLMQueryIntentClassifier`: LoRA adapter `DomainType::SECURITY_MONITOR` (new)
- Baseline profile from 90-day window via `ZeroTrustContext` history
- GDPR-sensitive fields lookup via `GdprSubjectRightsManager::getSensitiveFields()`

**Phase 3 — Error Handling (1 week)**
- Fail-open vs. fail-closed configurable: `THEMIS_LLM_SECURITY_FAIL_CLOSED=true`
- Latency budget: > 50 ms → fallback to rule-based `AccessControl`
- False-positive rate ≤ 0.1 % (avoid excessive DBA alerts)

**Phase 4 — Tests**
- Unit: Intent classification correct for exfiltration, normal, and analytics patterns
- Red-team: 20 known AQL injection patterns are detected
- Regression: no false positives for legitimate bulk exports (backup jobs)

**Phase 5 — Performance Targets**
- Classification latency ≤ 30 ms (p99) in the critical query path
- Precision (anomaly detection) ≥ 95 %
- Recall (no false negative for critical GDPR fields) ≥ 99 %

### Security & Reliability
- LLM output verified for integrity via `LoRASecurityValidator`
- All security alerts written to `AuditLogger` (tamper-evident)
- Post-quantum audit log protection via `SphincsPlus`

---

## Layer 8 — Multi-Tenant Workload Isolation & Resource Policy

### Motivation

`TenantManager` currently manages static quotas
(max_storage_bytes, requests_per_second, max_concurrent_queries).
The LLM can identify **dynamic workload identities** and adapt resource
allocation at runtime — without manual quota changes.

### Signal Sources

| Signal | ThemisDB Source | Format |
|--------|-----------------|--------|
| Workload type | `WorkloadAdaptiveOptimizer::WorkloadType` | OLTP/OLAP/GRAPH/VECTOR |
| Query burst pattern | `TenantConfig.requests_per_second` metric | Time series per tenant |
| Resource consumption | Prometheus metrics (CPU, IO, VRAM) | Counter/Gauge per tenant |
| Shard affinity | `AdaptiveShardRouter` routing log | Tenant → shard distribution |
| Anomaly signal | `MLAnomalyDetector` | Deviation from expected profile |

### LLM Tasks at This Layer

**8a — Workload Fingerprint Classification:**
The LLM receives the current query mix of a tenant and classifies it
into one of four workload classes with specific resource profiles.

```
Input:  Tenant A: 90 % OLAP scans > 1M docs, 10 % OLTP writes
Output: WorkloadFingerprint {
  tenant: "A",
  class: ANALYTICAL_BATCH,
  recommended_profile: {
    thread_pool_size: 16,
    cache_size_mb: 2048,
    io_priority: LOW,          // yields IO to OLTP tenants
    vector_search_disabled: false,
    query_timeout_ms: 30000
  }
}
```

**8b — Cross-Tenant Resource Arbitration:**
When Tenant B starts an OLTP burst while Tenant A runs an OLAP background
scan, the LLM proactively reduces A's IO priority without requiring manual
quota adjustments.

```
Input:
  Tenant A: OLAP scan (in progress, 45 % complete, low urgency)
  Tenant B: OLTP burst (SLA: p99 < 10 ms, currently at 9.8 ms)
Output: ResourceArbitration {
  reduce_io_for: "A",
  boost_io_for: "B",
  duration_estimate_s: 12,
  rationale: "Tenant B SLA critical, Tenant A batch tolerant"
}
```

**8c — Privacy-Safe Cross-Tenant Pattern Transfer:**
The LLM learns from optimisation successes on Tenant A (e.g., a specific
HNSW efSearch for embedding search) and transfers the **pattern** (not the data)
to Tenant B with a similar workload fingerprint.

```
Input:  Tenant A: efSearch=128 → p99 reduction 22 ms→14 ms (VECTOR workload)
        Tenant B: VECTOR workload, efSearch currently=64
Output: TransferHint {
  tenant_target: "B",
  recommendation: "Increase efSearch to 128",
  basis: "Pattern-transfer from similar-fingerprint tenant (anonymised)",
  privacy_guarantee: "NO_DATA_SHARED"
}
```

### ThemisDB Components

```
include/server/tenant_manager.h                   → quota runtime update
include/performance/workload_adaptive_optimizer.h  → WorkloadType classification
include/sharding/adaptive_shard_router.h           → tenant → shard routing
include/index/hnsw_parameter_tuner.h              → efSearch cross-tenant transfer
include/observability/ml_anomaly_detector.h        → workload anomaly detection
include/llm/ai_orchestrator.h                     → inference pipeline
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- Interface `IMultiTenantWorkloadAdvisor` in `include/server/`
- Methods: `fingerprint(tenantId)`, `arbitrate(tenantPair)`, `transferPattern(src, dst)`
- Privacy guard: `transferPattern` operates exclusively on anonymised metrics

**Phase 2 — Core Implementation (5 weeks)**
- `LLMTenantWorkloadAdvisor` with Prometheus metric adapter
- `TenantConfig` runtime update via `TenantManager::updateConfig()` (already implemented)
- Privacy layer: tenant IDs replaced by UUID tokens before LLM inference

**Phase 3 — Tests**
- Unit: Fingerprint classification correct for OLTP, OLAP, VECTOR, GRAPH
- Integration: OLTP SLA maintained while OLAP scan runs
- Privacy: Zero-knowledge test — no tenant plain-text name appears in LLM prompt

**Phase 4 — Performance Targets**
- Arbitration response time ≤ 2 s (not in critical query path)
- SLA violation rate for OLTP tenants: ≥ −40 % vs. static quotas
- Cross-tenant transfer acceptance rate (DBA-confirmed): ≥ 70 %

---

## Layer 9 — Explainability & DBA Dialog

### Motivation

Layers 1–8 produce optimisation decisions — but without explanation, the DBA loses
trust in the system. The LLM is the only layer that can **contextualise all prior
decisions in natural language**. Layer 9 is not a standalone optimiser but the
**explainability bridge** between all other layers and the human administrator.

### Signal Sources

Layer 9 aggregates **all signals from Layers 1–10**:

| Signal Type | Source |
|-------------|--------|
| Loop 1–4 decisions | `WorkloadAdaptiveOptimizer`, `AdaptiveIndex`, `LoRATrainer` |
| Tx advisor decisions (L5) | `LLMTransactionAdvisor` |
| Schema hints (L6) | `LLMSchemaAdvisor` |
| Security alerts (L7) | `LLMQueryIntentClassifier` |
| Tenant arbitration (L8) | `LLMTenantWorkloadAdvisor` |
| Layout hints (L10) | `LLMStorageLayoutAdvisor` |
| DBA feedback | `FeedbackCollector`, `RLAIFTrainer` |

### LLM Tasks at This Layer

**9a — Decision Rationale:**
Every autonomous decision from all layers receives a `reason` field
in structured English/German.

```
Decision: "Index 'idx_order_customer' was automatically dropped"
Reason: "This index was used in 0 out of 284,193 queries over the last 47 days.
         Dropping it saves 2.1 GB storage and eliminates write overhead on inserts
         (~12 ms/insert). A similar decision was made on 2026-03-12 for
         'idx_product_legacy' (DBA accepted)."
```

**9b — Causal Explanation for Performance Problems:**

```
DBA question: "Why did p99 of query Q42 rise from 8 ms to 31 ms?"
LLM answer: "Three causes identified:
  1. [Main cause, 68 %] Cache eviction: Tenant C ran a 3 GB OLAP scan yesterday,
     which flushed the shared buffer pool. Layer 8 did not trigger an arbitration
     event because C had not yet been classified as OLAP.
  2. [Secondary cause, 22 %] HNSW efSearch was reduced from 64 to 32 (Loop 2,
     2026-04-15 03:12 UTC) to save VRAM. Q42 is a vector lookup query.
  3. [Background, 10 %] Elevated write concurrency due to backup job (Monday 03:00)."
```

**9c — DBA Chat & Counter-Proposal Workflow:**
The DBA can reject proposals in natural language dialogue with reasons,
which flow directly as RLAIF training signal into the next adapter version.

```
DBA: "The proposal to drop idx_order_status is wrong —
      this index is used by our monitoring job."
LLM: "Understood. I'm updating the profile for idx_order_status:
      'Batch-job usage outside AQL query log'. Should I include
      batch-job queries in usage statistics in future?
      [Yes / No / Only for this collection]"
→ DBA feedback stored via FeedbackCollector as PreferencePair in RLAIFTrainer.
```

### ThemisDB Components

```
include/prompt_engineering/feedback_collector.h  → DBA feedback storage
include/rag/rlaif_trainer.h                      → preference-pair training
include/llm/ai_orchestrator.h                    → dialog management
include/llm/ai_decision_auditor.h                → decision log (tamper-evident)
include/observability/metric_anomaly_detector.h  → causality signals
include/rag/continuous_learning_orchestrator.h   → RLAIF feedback → adapter
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- `DecisionRecord` struct: `{layer, timestamp, action, reason, confidence, dba_accepted}`
- `IExplainabilityBridge` with: `explain(decisionId)`, `askCausal(metricName, timerange)`, `submitFeedback(decisionId, accepted, rationale)`

**Phase 2 — Core Implementation (6 weeks)**
- Decision log aggregator: all layers write to `AIDecisionAuditor`
- Causality graph: LLM builds a causal tree for performance questions
- DBA chat API: REST endpoint `POST /api/llm/dba-dialog` (streaming response)

**Phase 3 — Tests**
- Unit: `explain(decisionId)` returns correct fields for all decision types
- Integration: DBA feedback flows into new LoRA adapter within 24 h
- Acceptance: DBA user study — ≥ 80 % rate explanations as "helpful"

**Phase 4 — Performance Targets**
- Explanation latency: ≤ 500 ms for simple decisions
- Causal analysis: ≤ 3 s for a 7-day window
- RLAIF loop: DBA feedback → adapter update ≤ 24 h

---

## Layer 10 — Storage Layout & Semantic Compression

### Motivation

ThemisDB currently selects storage backends (RocksDB/LSM, B-Tree, Columnar, HNSW)
based on static configuration and simple heuristics. The LLM can understand the
**semantics of stored data** and recommend data-type-specific optimisations:
partitioning scheme, compression algorithm, hot/cold-tier assignment.

### Signal Sources

| Signal | ThemisDB Source | Format |
|--------|-----------------|--------|
| Data type distribution | AQL schema analysis | Collection → field type histogram |
| Compression ratios | RocksDB `CompactionStats` | Collection → compression ratio |
| Access pattern | `WorkloadAdaptiveOptimizer` | Read/write mix per collection |
| Embedding dimensions | `VectorIndexManager` | Dimension × distance metric |
| Retention policy | `GdprSubjectRightsManager` | Mandatory retention per field |
| Time-series granularity | `TimeSeriesManager` | Tick interval × window size |

### LLM Tasks at This Layer

**10a — Semantic Partitioning:**
The LLM recognises that certain data types should be stored columnar
rather than row-oriented.

```
Input:
  Collection "Metrics" (1.2 TB):
    Fields: timestamp (int64), sensor_id (string), value (float64), unit (string)
    Query pattern: 98 % range scans on [timestamp] with FILTER sensor_id
Output: LayoutHint {
  collection: "Metrics",
  recommended_layout: COLUMNAR,
  partition_key: "timestamp",
  secondary_cluster_key: "sensor_id",
  estimated_compression_improvement: "+340 %",
  query_speedup_estimate: "−45 % scan time"
}
```

**10b — Domain-Specific Compression:**
The LLM recommends the optimal compression algorithm per collection type.

```
Collection type                  → Algorithm                  → Rationale
─────────────────────────────────────────────────────────────────────────────
Float embedding vectors         → Product Quantization (PQ)  → 4–16× compression, ~95 % recall
Timestamps                      → Delta + Gorilla encoding   → 10–90× for monotonic time series
Free-text fields                → Zstandard (Zstd-19)        → 2–3× without structural loss
JSON documents                  → Snappy + schema-aware      → 1.5–2× with field indexing
Log messages                    → LZ4 + rotation dictionary  → 2–4× for repetitive text
```

**10c — Hot/Cold Tier Semantics:**
Beyond last-access timestamps: the LLM evaluates the *semantic value*
of data for tier placement.

```
Input:
  Collection "LegalCases" (500 GB): last_access = 18 months, case_status = "CLOSED"
  GDPR tag: "data_retention_required_7_years"
Output: TierHint {
  collection: "LegalCases",
  recommended_tier: COLD_WRITE_ONCE,  // not DELETE — GDPR active
  rationale: "Legal retention obligation active for 7 years. Cold-tier move saves ~380 GB warm storage.",
  gdpr_policy: "DO_NOT_DELETE_BEFORE_2032-04-16",
  allowed_action: MOVE_TO_COLD_TIER
}
```

### ThemisDB Components

```
include/storage/online_schema_migration.h         → layout migration execution
include/index/hnsw_parameter_tuner.h             → vector PQ recommendation
include/governance/gdpr_subject_rights.h         → retention policy check
include/timeseries/anomaly_detection.h           → time-series patterns
include/sharding/gpu_erasure_coder.h             → erasure coding for cold tier
include/llm/ai_orchestrator.h                    → LLM inference
```

### Implementation Phases

**Phase 1 — API Contract (2 weeks)**
- Interface `IStorageLayoutAdvisor` with: `analyzeLayout(collection)`, `suggestCompression(collection)`, `suggestTierPlacement(collection)`
- `LayoutHint` struct with: `recommended_layout`, `compression_algo`, `tier`, `gdpr_gate`, `risk_level`

**Phase 2 — Core Implementation (6 weeks)**
- `LLMStorageLayoutAdvisor` with schema analysis adapter
- GDPR gate: every tier recommendation passes through `GdprSubjectRightsManager::canArchive()`
- Compression ratings: benchmark data from `BENCHMARK_ANALYSIS.md` as RAG context

**Phase 3 — Tests**
- Unit: columnar recommendation correct for time-series collections
- Integration: PQ recommendation for embedding collections without > 2 % recall loss
- GDPR test: cold-tier move with active retention is correctly blocked

**Phase 4 — Performance Targets**
- Layout analysis ≤ 10 s (batch job, not on critical path)
- Compression improvement after advisory ≥ +50 % for time-series collections
- Zero erroneous DELETEs of GDPR-protected data (zero-error target)

---

## Layer 11 — Distributed Knowledge Sharding (RAID-5 of Intelligence)

> **Full documentation:** `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`

### Motivation

Layers 5–10 operate **shard-locally** — each shard optimises in isolation.
Layer 11 is the infrastructure that propagates optimisation insights **across shard
boundaries** without raw data ever crossing those boundaries.

**Analogy:** Layers 5–10 are the data in a RAID array.
Layer 11 is the RAID-5 controller that manages the parity (= distributed knowledge).

### Position in the Matrix

```
┌────────┬──────────────────────────────────────┬────────────┬──────────────────────────┬───────────────┐
│ Layer  │ Optimization Target                  │ Timescale  │ Semantic Input           │ Autonomy      │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ 1–4    │ Query/Index/Adapter (local)          │ ms–weeks   │ Query patterns, metrics  │ high          │
│ 5      │ Transaction conflict prediction      │ < 10 ms    │ Tx content, entity map   │ medium        │
│ 6      │ Schema evolution orchestration       │ Days–weeks │ Usage patterns, types    │ Advisory      │
│ 7      │ Security anomaly via semantics       │ < 100 ms   │ Session context, intent  │ medium → high │
│ 8      │ Multi-tenant workload isolation      │ Seconds    │ Workload fingerprint     │ high          │
│ 9      │ Explainability & DBA dialog          │ On-demand  │ All layer signals        │ Advisory      │
│ 10     │ Layout & compression (semantic)      │ Hours      │ Semantic data type       │ Advisory      │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ **11** │ **Distributed Knowledge Sharding**   │ **Hours–** │ **Gradients, embeddings**│ **Infra-**    │
│        │ **RAID-5 for intelligence**           │ **Days**   │ **anon. metrics**        │ **structure** │
└────────┴──────────────────────────────────────┴────────────┴──────────────────────────┴───────────────┘
```

### The Four Connection Layers

| Connection Layer | Mechanism | New Component | Base Component |
|---|---|---|---|
| **A — Adapter Discovery** | Gossip payload | `AdapterCapabilityAnnouncement` | `GossipProtocol` |
| **B — Federated LoRA** | FedAvg + DP | `LoRAFederationCoordinator` | `FederatedAggregator` |
| **C — Federated RAG** | RRF merge | `FederatedRAGMerger` | `QueryFederation` + `RAGIngestionBridge` |
| **D — Federated RLAIF** | Embedding gossip | `CrossShardFeedbackSync` | `FeedbackCollector` + `RLAIFTrainer` |

### Cross-Shard Extension of Layers 5–10

| Layer | Shard-local (today) | Cross-Shard with Layer 11 |
|---|---|---|
| L5 Tx-Semantics | Batch hints per shard | `CrossShardTransaction` hints via `QueryFederation` |
| L6 Schema | Dead-weight report per shard | Aggregated across all shards — no seasonal field loss |
| **L7 Security** | IntentAlert per shard | **Gossip propagation: anomaly shard warns all immediately** |
| L8 Multi-Tenant | WorkloadFingerprint per shard | Cross-shard transfer for similar tenant fingerprints |
| L9 Explainability | AIDecisionAuditor per shard | `FederatedAIDecisionAuditor` — global timeline of all shards |
| L10 Layout | LayoutHint per shard | LayoutHint via Gossip — cross-shard compression strategy |

### Differential Privacy Core

Federated LoRA (Layer 11B) applies the **Gaussian mechanism**
(Dwork & Roth 2014):

```
σ = Δf · √(2·ln(1.25/δ)) / ε
```

Recommended configuration: `ε = 0.1`, `δ = 1e-5`, max. `T = 50` rounds
→ `ε_total = 5.0` (practically acceptable, Dwork & Roth §3.5).

### New Acceptance Criteria for Layer 11

| Criterion | Threshold |
|---|---|
| Gradient accuracy delta after round | ≥ +0 % (no regression) |
| DP budget consumption per round | ε_round ≤ 0.1 |
| Adapter routing quality (Precision@3) | ≥ 80 % for domain_hint queries |
| Federated RAG recall | ≥ +15 % vs. shard-local |
| DBA feedback propagation latency | ≤ 2 × gossip interval |

---

## 2. Cross-Layer Signal Interfaces

The **seven** layers (5–11) are not isolated. They share signal sources and produce
mutual inputs:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                  LLM Optimization Layers — Signal Flow                   │
│                                                                         │
│  Layer 5 (Tx semantics)         ──►  Layer 9 (Explainability)           │
│  Layer 6 (Schema evolution)     ──►  Layer 9                            │
│  Layer 7 (Security)             ──►  Layer 9                            │
│  Layer 8 (Multi-tenant)         ──►  Layer 9                            │
│  Layer 10 (Layout)              ──►  Layer 9                            │
│                                                                         │
│  Layer 7 (Security)             ──►  Layer 5 (Tx block on anomaly)      │
│  Layer 6 (Schema)               ──►  Layer 10 (Type → compression)      │
│  Layer 8 (Tenant fingerprint)   ──►  Layer 5 (Batch affinity per tenant)│
│  Layer 9 (DBA feedback)         ──►  Loop 4 (RLAIF)                     │
│                                                                         │
│  Loops 1–4 (existing)           ──►  Layer 9 (all decisions explainable)│
└─────────────────────────────────────────────────────────────────────────┘
```

**Shared AIDecisionAuditor:**
All layers write structured `DecisionRecord` entries to `AIDecisionAuditor`.
Layer 9 reads from it for explanations. Loop 4 (RLAIF) reads from it for training.

**Shared Guardrail Layer:**
All layers with `medium`/`high` autonomy pass through the **Autonomy Gate**
(documented in `THEMISDB_LORA_METRICS_AND_OVERVIEW.md` §5):
- ECE < 0.05
- Hot-pattern coverage ≥ 85 %
- DBA acceptance rate ≥ 75 %

---

## 3. Implementation Priority & Quick Wins

**Priority by ROI / Effort:**

| Priority | Layer | Rationale | Quick Win |
|----------|-------|-----------|-----------|
| 1 | **L7 Security** | Existing signals (`ZeroTrustContext`, `MLAnomalyDetector`), no new training signal needed | Intent classification with existing LLM adapter |
| 2 | **L9 Explainability** | High DBA trust gain, no autonomy risk, supplies RLAIF data | Decision log + causality API |
| 3 | **L8 Multi-tenant** | `TenantManager` already exists, workload profiles already via `WorkloadAdaptiveOptimizer` | Workload fingerprint for best tenant isolation |
| 4 | **L6 Schema** | Online DDL already exists, advisory-only = no risk | Dead-weight report |
| 5 | **L5 Transaction** | `DeadlockPredictor` as foundation | Batch affinity for session patterns |
| 6 | **L10 Layout** | Long-term ROI (storage costs), more complex migration | Columnar recommendation for time series |

---

## 4. Open Research Questions

**RQ-L5-1** — What is the actual retry-cycle gain from semantic conflict prediction
vs. `DeadlockPredictor` alone?
*(Hypothesis: +15–25 % for graph-intensive workloads)*

**RQ-L6-1** — What minimum usage lifetime allows safe classification of a field as
"dead weight" without violating seasonal patterns?
*(Hypothesis: 180-day rolling window with seasonality correction via ARIMA)*

**RQ-L7-1** — What is the false-positive rate of LLM-based intent classification
for legitimate bulk-export scenarios (backup jobs)?
*(Hypothesis: < 0.1 % with session-context conditioning)*

**RQ-L8-1** — Does privacy-safe cross-tenant pattern transfer violate GDPR principles
(data separation, Article 32 GDPR)?
*(Hypothesis: No, provided only patterns — not raw data — are transferred)*

**RQ-L9-1** — Does explainability increase DBA acceptance rates for autonomous
decisions?
*(Hypothesis: +20–35 pp vs. recommendation without reason field)*

**RQ-L10-1** — What is the actual recall loss from Product Quantization for
high-dimensional embeddings (1536 dim)?
*(Hypothesis: < 2 % recall loss at 8× compression — based on Johnson et al. 2017)*

---

## 5. References

- Van Aken, D. et al. (2017). Automatic Database Management System Tuning Through Large-scale Machine Learning. ACM SIGMOD.
- Marcus, R. et al. (2021). Bao: Learning to Steer Query Optimizers. SIGMOD.
- Bai, Y. et al. (2022). Constitutional AI: Harmlessness from AI Feedback. arXiv:2212.08073.
- Lee, H. et al. (2023). RLAIF: Scaling Reinforcement Learning from Human Feedback with AI Feedback. arXiv:2309.00267.
- Johnson, J. et al. (2017). Billion-scale similarity search with GPUs. arXiv:1702.08734.
- Ding, J. et al. (2020). Self-Managing Database Systems with AI Planning. VLDB.
- Pavlo, A. et al. (2017). Self-Driving Database Management Systems. CIDR.
- Negi, P. et al. (2023). Robust Query Driven Cardinality Estimation. VLDB.
- ThemisDB: `docs/de/research/LLM_OPTIMIERUNGSEBENEN_MATRIX.md` (German version)
- ThemisDB: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
- ThemisDB: `docs/en/research/THEMISDB_LORA_METRICS_AND_OVERVIEW.md`
- ThemisDB: `docs/de/research/MULTI_LAYER_FEEDBACK_LEARNING.md`
- ThemisDB: `docs/de/research/HYBRID_KONZEPT_THEMISDB.md`

---

## 6. Runtime Influence Mechanisms: 7 Classes

> **Cross-reference:** `PERFORMANCE_EXPECTATIONS.md §14.1` ·
> `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12` ·
> `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12`

These seven classes classify every mechanism by which LLM infrastructure and
AdaLoRA affect ThemisDB SLOs (Layers 5–11) at **runtime** — without restart
or recompile.

| # | Class | Semantics | Examples (Layers 5–11) |
|---|---|---|---|
| 1 | **Switch** | Binary ON/OFF — deterministic code-path flip | `enable_draft_kv_cache`, `hot_swap.enabled`, `importance_pruning.enabled` |
| 2 | **Fader** | Continuous signed −x…0…+x — hot-reloadable via SIGHUP | `acceptance_threshold` (0.6–0.75–0.9), `total_rank_budget` (128–512–1024), `speculative_tokens` (3–6–10) |
| 3 | **Optimizer** | Solves objective function (min/max) — no environment perception | `WorkloadFingerprintEngine`, FedAvg rank aggregation, TIES-Merge SVD |
| 4 | **Agentic Solver** | Perception → Decision → Action — autonomous | `SelfImprovementModule`, LLM Intent Classifier (Layer 7), `CrossShardFeedbackSync` |
| 5 | **Closed Loop** | Output measured → fed back as correction signal | AdaLoRA rank allocation, CI SLO gate, RLAIF quality loop |
| 6 | **Open Loop** | Action triggered by input; no feedback path to sender | SIGHUP hot-reload, gossip broadcast, LoRA hot-swap |
| 7 | **Causal Chain** | Directed multi-step cause-effect sequence; no return path | WorkloadFingerprintEngine → rank budget → FedAvg → TTFT P99↓ |

Full tables with ThemisDB instances: `PERFORMANCE_EXPECTATIONS.md §14.1` and
`docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12`.

**Operational Resilience — Cross-Cutting Dimensions**

The five dimensions are not independent classes — they instantiate the seven
classes above with concrete resilience patterns. They apply orthogonally across
all six semantic optimization layers (L5–L10) and the shared LoRA/AdaLoRA stack.
Canonical full tables:
`DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.8` · `VERTEILTES_WISSEN_FEDERATION.md §12.8`.

### Backpressure

| Mechanism | Class | Downstream signal | Upstream reaction | SLO |
|---|---|---|---|---|
| Inference request queue | **Fader** | `max_pending_requests` exceeded | ingestion throttled | Dispatch latency P99 |
| Kafka semantic-layer event lag | **Closed Loop** | topic-lag metric | consumer rate adjusted | Throughput |
| HTTP 429 from inference endpoint | **Open Loop** | 429 response | exponential backoff | TTFT |
| LLM queue hard-drop | **Switch** | queue full | request rejected (503) | Availability |

### Timeout / Circuit Breaker

| Mechanism | Class | Trigger | Action | Config key |
|---|---|---|---|---|
| Inference timeout | **Fader** | deadline exceeded | request aborted | `inference_timeout_ms` |
| LoRA hot-swap timeout | **Switch** | swap > 5 s | rollback to previous adapter | `hot_swap.timeout_ms` |
| Circuit Breaker OPEN | **Closed Loop** | `failure_rate ≥ failure_threshold` | path blocked | `circuit_breaker.failure_threshold` |
| gRPC deadline propagation | **Causal Chain** | client sets deadline | propagated through all layers | gRPC metadata |

### Errors / Warnings

| Signal | Class | Source | Consumer | Effect |
|---|---|---|---|---|
| L5 transaction conflict WARN | **Causal Chain** | `TransactionSemanticAdvisor` | `DeadlockPredictor` → re-index | Conflict graph updated |
| L6 schema dead-weight WARN | **Causal Chain** | `SchemaDeadWeightDetector` | `DocumentSchemaEvolution` → advisory | Archival candidate flagged |
| L7 IntentClassifier risk=HIGH | **Causal Chain** | `IntentClassifier` | ZeroTrust → AuditLog → SIEM | Session revoked |
| Importance-score NaN | **Causal Chain** | AdaLoRA layer | PruningEngine → pruning disabled | Rank budget fixed |
| P99 > baseline + 20 % | **Closed Loop** | SLO monitor | CI gate | Deployment blocked |

### Security

| Mechanism | Class | ThemisDB instance | Reference |
|---|---|---|---|
| Enforce TLS | **Switch** | `tls.enforce` | `docker/admin-ui/nginx.ssl.conf` |
| MFA for admin/operator | **Switch** | `mfa_required_roles: [admin, operator]` | `include/security/access_control.h` |
| RBAC policy strictness | **Fader** | `rbac.policy_version` | `src/security/access_control.cpp` |
| Login rate-limiting | **Fader** | 5 r/m → 30 r/m (nginx) | `docker/admin-ui/nginx.conf` |
| ZeroTrust session-risk loop | **Closed Loop** | `session_risk_score` → continuous re-auth | `include/security/zero_trust_policy_enforcer.h` |
| Security anomaly → SIEM (L7) | **Causal Chain** | `IntentClassifier` → ZeroTrust → SIEM | `DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.7` |
| CSRF nonce validation | **Switch** | `csrf_validation.enabled` | `docker/admin-ui/nginx.conf` |

### Hardening

| Measure | Class | Mechanism | Activation |
|---|---|---|---|
| Reject plaintext API | **Switch** | `security.deny_plaintext_api` | ON in production |
| Audit log verbosity | **Fader** | `audit.log_level` (INFO → DEBUG → TRACE) | SIGHUP |
| Dependency pinning + SBOM | **Open Loop** | CI scan on every build | GitHub Actions |
| IPv6 CIDR allowlist | **Fader** | `network_policy.cidr_allowlist` | `include/security/zero_trust_policy_enforcer.h` |
| Secret scanning gate | **Closed Loop** | alert → PR blocked | GitHub Actions |
| GDPR erase-target for L5–L10 decisions | **Closed Loop** | `GdprSubjectRightsManager` → per-module ACK | `include/governance/gdpr_subject_rights.h` |
| AIDecisionAuditor coverage (all 6 layers) | **Open Loop** | L5–L10 write `DecisionRecord` | `include/llm/ai_decision_auditor.h` |

> **Implementation work package:** `docs/issues/distributed_knowledge/DK-OR-operational-resilience.md`

[docs](../../README.md) > [en](../INDEX.md) > [research](./README.md)

---
Datum: 2026-04-17
Status: draft
Primary (Quelle der Wahrheit): include/distributed_knowledge/lora_federation_coordinator.h, include/distributed_knowledge/federated_rag_merger.h, include/distributed_knowledge/cross_shard_feedback_sync.h, include/distributed_knowledge/adapter_capability_announcement.h, include/sharding/gossip_protocol.h, include/importers/federated_learning.h, include/training/incremental_lora_trainer.h, include/rag/rlaif_trainer.h
Bezug / Reference: McMahan et al. (2017) FedAvg AISTATS · Dwork & Roth (2014) DP Foundations · Cormack et al. (2009) RRF SIGIR · Bai et al. (2022) Constitutional AI arXiv:2212.08073 · Lee et al. (2023) RLAIF arXiv:2309.00267 · Li et al. (2020) FedProx MLSys
---

# Distributed Knowledge — RAID-Sharding of ThemisDB Intelligence

**Technical Research Document — ThemisDB Project**  
*Version 1.0 · 2026-04-17 · Apache-2.0*

---

## Table of Contents

- [0. Problem Statement](#0-problem-statement)
- [1. The RAID Analogy Applied to Knowledge](#1-the-raid-analogy-applied-to-knowledge)
- [2. Existing Building Blocks & Connection Gaps](#2-existing-building-blocks--connection-gaps)
- [3. Layer A — Gossip-Based Adapter Discovery](#3-layer-a--gossip-based-adapter-discovery)
- [4. Layer B — Federated LoRA Gradient Aggregation (RAID-5 Core)](#4-layer-b--federated-lora-gradient-aggregation-raid-5-core)
- [5. Layer C — Cross-Shard RAG Federation](#5-layer-c--cross-shard-rag-federation)
- [6. Layer D — Federated RLAIF (Distributed DBA Feedback)](#6-layer-d--federated-rlaif-distributed-dba-feedback)
- [7. Security & Privacy Guarantees](#7-security--privacy-guarantees)
- [8. Connection to Optimization Layers 5–10](#8-connection-to-optimization-layers-510)
- [9. Full Architecture Diagram](#9-full-architecture-diagram)
- [10. Implementation Order by ROI](#10-implementation-order-by-roi)
- [11. Open Research Questions](#11-open-research-questions)
- [12. Runtime Influence Mechanisms: Switches · Levers · Optimizers](#12-runtime-influence-mechanisms-switches--levers--optimizers)
- [13. References](#13-references)

---

## 0. Problem Statement

ThemisDB has fully implemented RAID-sharding for **data**:
`ConsistentHash`, `RaftShardManager`, `QueryFederation`, `CrossShardTransaction`.
Each shard manages its data redundantly, in a distributed and fault-tolerant manner.

The learning layers — `LoRA adapters`, `RLAIF`, `ContinuousLearningOrchestrator` — are
however still **shard-local**: each shard trains alone and has no knowledge of
the insights gained on other shards.

**The problem:** Shard 7 detects a novel attack pattern. Shard 12 develops a better
schema compression strategy. Shard 3 receives valuable DBA feedback about
denormalisation errors. None of the other shards benefit from any of this.

**The solution:** RAID-sharding of intelligence — a federated learning system that
propagates optimisation insights between shards **without raw data ever crossing shard
boundaries**.

---

## 1. The RAID Analogy Applied to Knowledge

Classic RAID protects data through redundancy and parity information.
The same logic applies to *learning states*:

| RAID Level | Data Sharding (implemented) | Knowledge Sharding (new) |
|---|---|---|
| **RAID-0** | Striping without redundancy | Each shard trains in isolation — no transfer |
| **RAID-1** | Full replication | Full LoRA adapter synchronisation across all shards → prohibitively expensive |
| **RAID-5** | Striping + distributed parity | **FedAvg** — gradient aggregation with Differential Privacy |
| **RAID-6** | Double parity | Hierarchical aggregation: Shard → Region → Global |
| **RAID-10** | Mirror + Stripe | Specialised + shared adapters combined |

**Target strategy: RAID-5 for knowledge** — FedAvg-based LoRA adapter federation
where no shard sees the raw data of any other shard. The "parity" is the
DP-protected global gradient vector.

### Information-Theoretic Justification

RAID-5 works because parity information can be *derived* without knowing the original
data of all participants. Analogously for FedAvg (McMahan et al. 2017):

```
θ_global = Σ_k (n_k / N) · θ_k
```

where `n_k` is the sample count and `θ_k` are the local model weights of shard `k`.
The global sum is a sufficient statistic for the gradient space — analogous to the
XOR parity block in RAID-5.

With Differential Privacy (Dwork & Roth 2014, Gaussian mechanism):

```
σ = Δf · √(2 · ln(1.25/δ)) / ε
```

the aggregation step guarantees (ε, δ)-DP: no observer can reconstruct the
training content of any individual shard from the global delta.

---

## 2. Existing Building Blocks & Connection Gaps

### 2.1 Data Layer (complete)

| Component | File | Purpose |
|---|---|---|
| `GossipProtocol` | `include/sharding/gossip_protocol.h` | Cluster membership, heartbeat propagation |
| `AdaptiveShardRouter` | `include/sharding/adaptive_shard_router.h` | Capability-based query routing |
| `QueryFederation` | `include/query/query_federation.h` | Fan-out + merge for AQL queries |
| `CrossShardTransaction` | `include/sharding/cross_shard_transaction.h` | 2PC across shard boundaries |

### 2.2 Learning Layer (complete, but shard-local)

| Component | File | Purpose |
|---|---|---|
| `ContinuousLearningOrchestrator` | `include/rag/continuous_learning_orchestrator.h` | Loop-4 coordination |
| `IncrementalLoRATrainer` | `include/training/incremental_lora_trainer.h` | LoRA training + versioning |
| `FederatedAggregator` | `include/importers/federated_learning.h` | FedAvg/FedProx/Median **← KEY** |
| `DifferentialPrivacyManager` | `include/importers/federated_learning.h` | DP noise with ε/δ budget |
| `RLAIFTrainer` | `include/rag/rlaif_trainer.h` | Constitutional AI + RLAIF |
| `FeedbackCollector` | `include/prompt_engineering/feedback_collector.h` | DBA feedback capture |
| `RAGIngestionBridge` | `include/rag/rag_ingestion_bridge.h` | Document indexing + entity extraction |
| `AdapterRegistry` | `include/llm/adapter_registry.h` | Adapter versioning + A/B routing |

### 2.3 Connection Gaps (new components)

| Required | Basis | New File |
|---|---|---|
| `AdapterCapabilityAnnouncement` | `GossipProtocol` | `include/distributed_knowledge/adapter_capability_announcement.h` |
| `GossipAdapterPublisher` | `GossipProtocol` | (same) |
| `ILoRAFederationCoordinator` | `FederatedAggregator` + `IncrementalLoRATrainer` | `include/distributed_knowledge/lora_federation_coordinator.h` |
| `LoRAFederationCoordinator` | (same) | `src/distributed_knowledge/lora_federation_coordinator.cpp` |
| `FederatedRAGMerger` | `QueryFederation` + `RAGIngestionBridge` | `include/distributed_knowledge/federated_rag_merger.h` |
| `CrossShardFeedbackSync` | `FeedbackCollector` + `GossipProtocol` | `include/distributed_knowledge/cross_shard_feedback_sync.h` |

---

## 3. Layer A — Gossip-Based Adapter Discovery

### 3.1 Motivation

Without discovery, `AdaptiveShardRouter` does not know which shard holds the
most domain-specialised LoRA adapter. Queries are distributed naively.

With discovery: Shard 7 has `domain_type = SECURITY_MONITOR` and
`performance_delta_p99_ms = -8.4 ms`. Security-relevant queries are preferentially
routed there — no training required, immediate gain.

### 3.2 Data Structure: AdapterCapabilityAnnouncement

```cpp
struct AdapterCapabilityAnnouncement {
    std::string       shard_id;
    std::string       adapter_version;          // "v1.3.0"
    AdapterDomainType domain_type;              // SECURITY_MONITOR, SCHEMA_ADVISOR, ...
    double            performance_delta_p99_ms; // -8.4 = 8.4 ms faster
    double            accuracy_delta;           // +0.03 = 3% better
    size_t            training_samples;         // 14200
    uint64_t          federation_round;         // last federated round
};
```

### 3.3 Transport

The announcement is embedded as JSON in `GossipMessage::payload`
(`message_type = "adapter_capability"`). No protocol changes required.

```
GossipProtocol::sendHeartbeat()
   ↓ payload += AdapterCapabilityAnnouncement.toJson()
AdaptiveShardRouter::onPeerDiscovered()
   ↓ updateCapabilityScore(shard_id, announcement)
Query with domain_hint="SECURITY_MONITOR"
   ↓ AdaptiveShardRouter selects shard with highest domain_score
```

### 3.4 Security

`GossipProtocol::verifyMessage()` checks the HMAC signature before processing.
`ZeroTrustPolicyEnforcer::evaluateRequest()` verifies shard identity via
mTLS certificate.

---

## 4. Layer B — Federated LoRA Gradient Aggregation (RAID-5 Core)

### 4.1 Mathematical Foundation

**FedAvg** (McMahan et al. 2017):

Each shard `k` runs `E` local training epochs and exports its weight vector `w_k`:

```
Global update:
  w_{t+1} = Σ_k (n_k / N) · w_k^t

Differential Privacy (Gaussian mechanism):
  w̃_{t+1} = w_{t+1} + N(0, σ²I)
  σ = Δ_f · √(2·ln(1.25/δ)) / ε
```

**FedProx** (Li et al. 2020) for heterogeneous workloads — minimises:

```
min_w  F_k(w) + (μ/2)·‖w - w_t‖²
```

The proximal term `μ/2·‖w - w_t‖²` limits drift of local models under heterogeneous
shard distributions — relevant when Shard 1 handles primarily transaction workloads
and Shard 7 handles security workloads.

### 4.2 Flow per Federation Round

```
┌─────────────────────────────────────────────────────────────────┐
│  Federated LoRA Round (every 24h or manually triggered)         │
│                                                                 │
│  Shard 1  ─┐                                                    │
│  Shard 2  ─┤─ EncryptedGradient(shard_k, round, data) ──────►  │
│  Shard N  ─┘                                                    │
│                    LoRAFederationCoordinator                     │
│                    .submitGradient(gradient_k)                  │
│                    ↓  (after min_participants submissions)       │
│                    FedAvg/FedProx Aggregation                   │
│                    ↓                                            │
│                    DifferentialPrivacyManager(ε=0.1, δ=1e-5)    │
│                    ↓                                            │
│                    GlobalAdapterDelta("global-v42")             │
│                    ↓  (broadcast via GossipProtocol)            │
│  Shard 1  ◄─── applyGlobalDelta(delta)                         │
│  Shard N  ◄─── applyGlobalDelta(delta)                         │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 Privacy Budget Management

The DP budget accumulates: after `T` rounds, `ε_total = T · ε_round`.
`DifferentialPrivacyManager::verifyPrivacyBudget()` monitors the budget.
When exceeded, the next round is skipped.

**Recommended configuration:**
- `ε_round = 0.1`, `δ = 1e-5` per round
- `T_max = 50` rounds before budget reset → `ε_total = 5.0` (acceptable for
  many practical applications per Dwork & Roth §3.5)

### 4.4 Configuration

```json
{
  "federation": {
    "min_participants": 2,
    "aggregation_algorithm": "FedAvg",
    "weight_by_sample_count": true,
    "dp_epsilon": 0.1,
    "dp_delta": 1e-5,
    "dp_sensitivity": 1.0,
    "federation_interval_hours": 24,
    "round_timeout_minutes": 60
  }
}
```

---

## 5. Layer C — Cross-Shard RAG Federation

### 5.1 Motivation

Currently `RAGIngestionBridge` only evaluates the local shard. A question like
*"What are the most frequent transaction errors?"* only receives Shard 3's
perspective — even though Shard 7 may hold the most relevant documents.

With federation: `QueryFederation::fanOut()` calls `RAGIngestionBridge` on
**all** shards in parallel; `FederatedRAGMerger` combines the results into a
global context.

### 5.2 Reciprocal Rank Fusion

RRF (Cormack et al. 2009) is the standard method for multi-source re-ranking:

```
score(d) = Σ_{q ∈ Shards}  1 / (k + rank(d, q))
```

Advantages:
- No calibration of shard scores required (independent retrieval systems)
- Robust against outliers (logarithmic dampening of high ranks)
- Deterministic tie-breaking via shard ID

**Specialisation boost:** When `adapter_accuracy_delta > 0` (shard has a
domain-specialised adapter), the RRF score is multiplied by `specialisation_boost`
(default: 1.2) — documents from the best adapter dominate the merge.

### 5.3 Flow

```
AQL Query: "Most frequent transaction errors"
    ↓
QueryFederation::fanOut(queryPlan)  [parallel]
    ↓
    Shard 1: RAGIngestionBridge::enrichRetrievedDocuments(docs_1)
    Shard 2: RAGIngestionBridge::enrichRetrievedDocuments(docs_2)
    Shard N: RAGIngestionBridge::enrichRetrievedDocuments(docs_N)
    ↓
FederatedRAGMerger::merge(shard_results)
    [MergeStrategy::RECIPROCAL_RANK_FUSION, top_k=20, deduplicate=true]
    ↓
MergedRAGContext::buildPromptContext(max_docs=10, max_chars=4000)
    ↓
LLM Prompt: "[Shard: shard-7] ... [Shard: shard-3] ..."
```

### 5.4 Merge Strategy Comparison

| Strategy | Strength | Weakness | Recommendation |
|---|---|---|---|
| `RECIPROCAL_RANK_FUSION` | Robust, no score calibration | Ignores absolute scores | **Default** |
| `SCORE_WEIGHTED` | Uses adapter_accuracy_delta | Requires calibrated scores | Homogeneous shards |
| `ROUND_ROBIN` | Maximum diversity | No relevance sorting | Exploratory queries |

---

## 6. Layer D — Federated RLAIF (Distributed DBA Feedback)

### 6.1 Motivation

A DBA on Shard 3 rejects a denormalisation recommendation. This experience
currently stays on Shard 3. All other shards continue issuing the same flawed
recommendation.

With federated RLAIF: the feedback is anonymised and propagated; all shards
learn from local DBA expertise — **without privacy compromise**.

### 6.2 Privacy Through Embedding Propagation

```
DBA provides feedback on query "ALTER TABLE orders ADD COLUMN..."
    ↓
FeedbackCollector::recordFeedback(entry)
    ↓
Embedding model: reason_embedding = embed(entry.query)
    [only the embedding vector, no raw text]
    ↓
CrossShardFeedbackSync::publishFeedback({
    feedback_type_label: "USER_NEGATIVE",
    reason_embedding: [0.12, -0.34, ...],  // 384-dim, no raw text
    shard_origin: "ANON"
})
    ↓
GossipProtocol propagates FeedbackSummary
    ↓
Receiving shards: RLAIFTrainer::addPreferencePair(
    buildPairFromSummary(summary)
)
    ↓
Next IncrementalLoRATrainer round incorporates global DBA knowledge
```

### 6.3 Deduplication & Idempotency

Gossip echoes can deliver the same `FeedbackSummary` multiple times.
`CrossShardFeedbackSync` maintains an LRU cache of recently seen
`summary_id` hashes (default: 10,000 entries). Duplicates are discarded
without triggering the callback.

### 6.4 RLAIF Preference Pair Construction from Embeddings

Since the raw text is unavailable, preference pairs are constructed synthetically
from the embedding:

```python
# Pseudo-code — actual implementation is the caller's responsibility
chosen   = nearest_neighbors(summary.reason_embedding, polarity=+1)
rejected = nearest_neighbors(summary.reason_embedding, polarity=-1)
pair = PreferencePair(prompt=context, chosen=chosen, rejected=rejected)
rlaif_trainer.addPreferencePair(pair)
```

*Note: The nearest-neighbour construction is an approximation. For production
deployment, a dedicated feedback embedding model should be trained
(see Research Question RQ-DK-3).*

---

## 7. Security & Privacy Guarantees

| Security Requirement | Implementation |
|---|---|
| **Zero-Knowledge Constraint** | No raw data crosses shard boundaries — only DP gradients, embeddings, anonymous metrics |
| **mTLS for all shard communication** | `MTLSClient` already in `GossipProtocol`; `ZeroTrustPolicyEnforcer` verifies |
| **Differential Privacy** | `LoRAFederationCoordinator` applies Gaussian mechanism: σ = Δ·√(2·ln(1.25/δ))/ε |
| **Post-Quantum Audit** | `SphincsPlus` secures the global federation audit log |
| **GDPR Compliance** | `CrossBorderTransferPolicy` checks EU adequacy boundary before gradient aggregation |
| **Embedding Anonymisation** | `CrossShardFeedbackSync` enforces `shard_origin = "ANON"`, no raw text in payload |
| **Privacy Budget Monitoring** | `DifferentialPrivacyManager::verifyPrivacyBudget(ε_total, δ)` prevents budget exhaustion |

---

## 8. Connection to Optimization Layers 5–10

The six LLM optimization layers from `LLM_OPTIMIZATION_LAYERS_MATRIX.md` are
qualitatively extended in distributed mode:

| Layer | Shard-local (today) | Cross-Shard (new) |
|---|---|---|
| **L5 Tx-Semantics** | Batch hints per shard | `CrossShardTransaction` hints via `QueryFederation` |
| **L6 Schema** | Dead-weight report per shard | Aggregated dead-weight across all shards — prevents misclassification of seasonal fields |
| **L7 Security** | IntentAlert per shard | Gossip propagation: Shard A detects anomaly → all shards raise `session_risk_score` immediately |
| **L8 Multi-Tenant** | WorkloadFingerprint per shard | Cross-shard WorkloadFingerprint transfer: Shard B learns from Shard A for similar tenants |
| **L9 Explainability** | AIDecisionAuditor per shard | `FederatedAIDecisionAuditor`: DBA sees decisions from **all shards** in a single global timeline |
| **L10 Layout** | LayoutHint per shard | LayoutHint propagated via Gossip — cross-shard compression strategy |

### New Layer 11 — Distributed Knowledge Sharding

Layer 11 is not analogous to Layers 5–10 (which describe orthogonal semantic
dimensions) but is the **infrastructure layer** that makes Layers 5–10
*effective across shard boundaries*.

```
Layers 5–10 (semantics, local)  +  Layer 11 (transport)
=  Distributed Self-Optimizing ThemisDB
```

---

## 9. Full Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│          ThemisDB Distributed Knowledge Architecture — Layer 11          │
│                                                                         │
│  Shard 1          Shard 2          Shard N         Global Coordinator   │
│  ─────────        ─────────        ─────────       ─────────────────    │
│                                                                         │
│  [Layer A: Adapter Discovery]                                           │
│  AdapterReg   ─── Gossip ───►  AdapterReg   ─────► CapabilityScore     │
│       ▲             │                                     │             │
│       └─────────────┴─────────── AdaptiveShardRouter ◄───┘             │
│                                                                         │
│  [Layer B: Federated LoRA]                                              │
│  LoRATrainer  ──► Gradient₁                                             │
│  LoRATrainer  ──► Gradient₂ ──► LoRAFederationCoordinator              │
│  LoRATrainer  ──► GradientN        │ FedAvg + DP(ε,δ)                  │
│       ◄──────── GlobalDelta ◄──────┘                                   │
│                                                                         │
│  [Layer C: Federated RAG]                                               │
│  RAGBridge    ─── QueryFed ──►  RAGBridge   ──► FederatedRAGMerger     │
│                                                     │ RRF + top_k=20   │
│                              LLM Prompt ◄───────────┘                  │
│                                                                         │
│  [Layer D: Federated RLAIF]                                             │
│  FeedbackColl ─── Gossip ───►  FeedbackColl  ──► CrossShardFeedbackSync│
│  RLAIFTrainer ◄── Summary  ◄──────────────────────────────────────────  │
│                                                                         │
│  [Security throughout]                                                  │
│  ZeroTrust ─────── mTLS ──── SphincsPlus ──── CrossBorderTransferPolicy│
│                                                                         │
│  [Layers 5–10 in distributed mode]                                      │
│  L7 IntentAlert  ──►  Gossip  ──►  session_risk_score (all shards)     │
│  L6 Dead-Weight  ──►  Federation ──►  global dead-weight report         │
│  L9 AIDecision   ──►  Audit Log  ──►  FederatedAIDecisionAuditor        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Implementation Order by ROI

| Priority | Layer | Estimated Effort | Immediate Benefit |
|---|---|---|---|
| **1** | A — Adapter Gossip | 2 weeks | Domain-aware routing without training |
| **2** | C — Federated RAG Merge | 3 weeks | LLM sees knowledge from all shards |
| **3** | B — Federated LoRA | 6 weeks | Core mechanism for distributed learning |
| **4** | D — Cross-Shard RLAIF | 3 weeks | DBA feedback propagated globally |

**Rationale:**
- Layer A is purely configurative, no training required, immediate routing gain
- Layer C reuses existing `QueryFederation` + `RAGIngestionBridge`
- Layer B is the most complex core but is prepared by Layers A and C
  (correct routing basis → better gradients)
- Layer D is independent, but full potential only unfolds when Layer B is
  complete (better adapter foundation → better preference pairs)

---

## 11. Open Research Questions

**RQ-DK-1** — How many training samples does a shard need at minimum before its
gradient contribution does not degrade the global model?
*(Hypothesis: n_k ≥ 500 based on FedAvg convergence analysis McMahan §4)*

**RQ-DK-2** — How large is the information loss from DP noise at ε = 0.1, δ = 1e-5
for LoRA adapters with rank=8?
*(Hypothesis: < 3 % accuracy delta, as LoRA updates are inherently low-norm
vectors)*

**RQ-DK-3** — Is the nearest-neighbour construction of RLAIF preference pairs
from embeddings (without raw text) sufficient for measurable learning improvement?
*(Hypothesis: +8–15 % compared to no cross-shard propagation; requires empirical
validation)*

**RQ-DK-4** — Does gradient aggregation across EU/non-EU shard boundaries violate
GDPR requirements (Art. 44 GDPR — third-country transfer)?
*(Hypothesis: No, as only anonymous numerical gradients are transferred, no
personal data — but: `CrossBorderTransferPolicy` must explicitly handle the
edge case)*

**RQ-DK-5** — How large is the specialisation loss in Layer C when RRF mixes
domain-specialised shard results with generic shard results?
*(Hypothesis: With 1.2× specialisation boost, relevance is dominated by the
specialised shard; generic contributions supplement rather than distort)*

**RQ-DK-6** — Does FedAvg converge for strongly heterogeneous shard specialisations
(Security shard vs. Schema shard) or does the global model diverge?
*(Hypothesis: FedProx with μ = 0.01 prevents divergence at up to 5×
heterogeneity — based on Li et al. 2020 §4.3)*

---

## 12. Runtime Influence Mechanisms: Switches · Levers · Optimizers

> **Cross-reference:** `PERFORMANCE_EXPECTATIONS.md §14.1` · Paper 1
> `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12` ·
> `config/lora/adalora_optimization_strategy.yaml` ·
> `config/ai_ml/llm/llm_optimization_strategy.yaml`

The insights from Paper 1 (FedAvg rank federation) and this paper
(Semantic Advisors B5–B10) yield three clearly separable influence classes
through which LLM infrastructure and AdaLoRA control ThemisDB performance
at **runtime** — without restart, recompile, or model reload.

### 12.1 Class 1 — Switches

Switches are **binary code-path decisions**. Activation takes effect immediately
and deterministically changes runtime behaviour.

| Switch | Activation condition | Affected SLO |
|---|---|---|
| `bypass_dedup_cache_for_streaming` | Streaming request detected | TTFT −10 ms (L-1) |
| `enable_draft_kv_cache` | Draft model present | Speculative decoding active (L-6/L-8) |
| `hot_swap.enabled` | `THEMIS_ENABLE_LLM=1` set | LoRA swap without restart (L-3 ≤ 5 s) |
| `importance_pruning.enabled` | AdaLoRA active | Rank-budget compression (§39.20) |
| `federation.broadcast_importance_scores` | Feature flag IMPL-A3 active | Cross-shard pruning knowledge |

### 12.2 Class 2 — Levers

Levers are **numeric configuration parameters** with a measurable, continuous
trade-off. They can be rewritten at runtime via hot-reload (SIGHUP).

| Lever | Value range | Trade-off |
|---|---|---|
| `speculative_tokens` | 3 – 10 | TTFT ↔ acceptance rate (L-6) |
| `total_rank_budget` | 128 – 1024 | memory footprint ↔ model quality |
| `acceptance_threshold` | 0.6 – 0.9 | inference speed ↔ correctness |
| `pruning_interval_steps` | 50 – 500 | pruning overhead ↔ adaptivity |
| `worker_threads` | 2 – 16 | dispatch latency P99 ↔ CPU overhead (L-5) |
| `chunked_prefill_size` | 512 – 2048 tokens | TTFT reduction ↔ decode interleave overhead |

### 12.3 Class 3 — Optimizers

Optimizers are **self-adapting feedback loops**. They observe the current
runtime state and autonomously rewrite levers or switches.

| Optimizer | Acts on | Interval | Linked issue |
|---|---|---|---|
| `WorkloadFingerprintEngine` (B8) | AdaLoRA `total_rank_budget` | every 100 queries | IMPL-B8 |
| FedAvg rank aggregation (`lora_federation_coordinator`) | Importance-score distribution across all shards | per pruning step | IMPL-A3 |
| `SelfImprovementModule` | Quality thresholds (acceptance, confidence) | continuous | DK-4 |
| TIES-Merge SVD (`LoRAAdapterMerger`) | Adapter merging without checkpoint | at adapter switch | PR #4405 |
| CI SLO gate (P99 > 20 % regression) | Deployment release | every CI run | §23 SLO Monitor |

### 12.4 Effect Chain

```
WorkloadFingerprintEngine (B8)
  └─ detects current query mix
       └─ adjusts total_rank_budget
            └─ AdaLoRA redistributes rank budget optimally per layer
                 └─ lora_federation_coordinator propagates
                    importance scores across all shards (FedAvg)
                         └─ TTFT P99 L-1 decreases
                            throughput L-8 increases
                            — without manual intervention
```

The Differential Privacy guarantee (Dwork & Roth 2014, Gaussian mechanism,
Layer B §4) is fully preserved throughout: only anonymised gradient
statistics cross shard boundaries.

---

## 13. References

**Federated Learning & Differential Privacy:**
- McMahan, H.B. et al. (2017). Communication-Efficient Learning of Deep Networks from Decentralised Data. *AISTATS 2017*.
- Li, T. et al. (2020). Federated Optimization in Heterogeneous Networks (FedProx). *MLSys 2020*.
- Dwork, C. & Roth, A. (2014). The Algorithmic Foundations of Differential Privacy. *Foundations and Trends in Theoretical Computer Science*.
- Geyer, R.C. et al. (2017). Differentially Private Federated Learning: A Client Level Perspective. *arXiv:1712.07557*.

**RLAIF & Constitutional AI:**
- Bai, Y. et al. (2022). Constitutional AI: Harmlessness from AI Feedback. *arXiv:2212.08073*.
- Lee, H. et al. (2023). RLAIF: Scaling Reinforcement Learning from Human Feedback with AI Feedback. *arXiv:2309.00267*.

**Retrieval & Re-Ranking:**
- Cormack, G.V., Clarke, C.L.A., Buettcher, S. (2009). Reciprocal Rank Fusion outperforms Condorcet and individual Rank Learning Methods. *ACM SIGIR 2009*.

**Gossip & Distributed Systems:**
- Demers, A. et al. (1987). Epidemic Algorithms for Replicated Database Maintenance. *ACM PODC 1987*.
- Jelasity, M. et al. (2007). Gossip-based Peer Sampling. *ACM TOCS 2007*.

**Self-Driving DBMS:**
- Pavlo, A. et al. (2017). Self-Driving Database Management Systems. *CIDR 2017*.
- Van Aken, D. et al. (2017). Automatic Database Management System Tuning Through Large-scale Machine Learning. *ACM SIGMOD*.

**ThemisDB internal documents:**
- `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md` — German version of this document
- `docs/de/research/LLM_OPTIMIERUNGSEBENEN_MATRIX.md` — Optimization Layers 5–10 (DE)
- `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` — Optimization Layers 5–10 (EN)
- `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md` — LoRA fundamentals
- `docs/en/research/THEMISDB_LORA_METRICS_AND_OVERVIEW.md` — Metrics & Loops 1–4
- `docs/en/sharding/RAID_LORA_IMPLEMENTATION_REPORT.md` — RAID data-layer status

# distributed_knowledge Module — Architecture

**Version:** 0.1.0  
**Last Updated:** 2026-04-17  
**Module Path:** `src/distributed_knowledge/`  
**Header Path:** `include/distributed_knowledge/`

---

## 1. Overview

The `distributed_knowledge` module implements **RAID-5-style knowledge sharding**
for ThemisDB: it propagates optimisation insights (LoRA gradients, RAG retrieval
results, DBA feedback) across shard boundaries **without raw data ever leaving a
shard**.

It connects the existing, isolated learning stack — `IncrementalLoRATrainer`,
`ContinuousLearningOrchestrator`, `RLAIFTrainer`, `RAGIngestionBridge` — to the
existing, production-ready distributed transport stack — `GossipProtocol`,
`QueryFederation`, `AdaptiveShardRouter` — through four well-defined integration
layers (A–D).

Full scientific rationale: `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`

---

## 2. Design Principles

- **Zero raw-data egress** — no shard-local training data or query content crosses
  shard boundaries; only DP-noised gradients, fixed-length embeddings, and
  anonymous metrics travel over the wire.
- **Incremental integration** — each of the four layers (A–D) is independently
  deployable and testable; disabling one layer does not break the others.
- **Reuse existing infrastructure** — `FederatedAggregator`, `DifferentialPrivacyManager`,
  `GossipProtocol`, `QueryFederation` are used as-is; no forking.
- **Configurability over magic** — all thresholds (ε, δ, top_k, min_participants)
  are runtime-configurable via `FederationConfig` / `FederatedRAGMergerConfig` /
  `FeedbackSyncConfig`.
- **Thread safety** — all public APIs acquire an internal `std::mutex`; callers need
  no external synchronisation.

---

## 3. Component Architecture

### 3.1 Module Components

| File | Role | Layer |
|---|---|---|
| `adapter_capability_announcement.h/cpp` | Gossip payload struct + publisher | A |
| `lora_federation_coordinator.h/cpp` | FedAvg/DP gradient aggregation | B |
| `federated_rag_merger.h/cpp` | Cross-shard RAG result merge (RRF) | C |
| `cross_shard_feedback_sync.h/cpp` | Anonymised DBA feedback propagation | D |

### 3.2 External Dependencies

```
distributed_knowledge module
        │
        ├── [A] include/sharding/gossip_protocol.h          (transport)
        ├── [A] include/sharding/adaptive_shard_router.h    (routing consumer)
        ├── [B] include/importers/federated_learning.h      (FedAvg + DP)
        ├── [B] include/training/incremental_lora_trainer.h (gradient producer)
        ├── [B] include/rag/continuous_learning_orchestrator.h (trigger)
        ├── [C] include/query/query_federation.h            (fan-out transport)
        ├── [C] include/rag/rag_ingestion_bridge.h          (per-shard enrichment)
        ├── [D] include/prompt_engineering/feedback_collector.h (feedback source)
        └── [D] include/rag/rlaif_trainer.h                 (RLAIF consumer)
```

### 3.3 Layer Interaction Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│                distributed_knowledge — Internal Flows                 │
│                                                                      │
│  [Layer A]  AdapterCapabilityAnnouncement                            │
│             GossipAdapterPublisher ──────────► GossipProtocol        │
│             GossipProtocol (inbound) ──────────► AdaptiveShardRouter │
│                                                                      │
│  [Layer B]  EncryptedGradient (per shard)                            │
│             IncrementalLoRATrainer ─────► LoRAFederationCoordinator  │
│             LoRAFederationCoordinator                                │
│               ├── FedAvg / FedProx aggregation                       │
│               ├── DifferentialPrivacyManager (Gaussian σ)            │
│               └── GlobalAdapterDelta ─────► all shards              │
│                                                                      │
│  [Layer C]  ShardRetrievalResult (per shard)                         │
│             QueryFederation::fanOut ───────► FederatedRAGMerger      │
│             FederatedRAGMerger                                       │
│               ├── RRF / ScoreWeighted / RoundRobin                   │
│               ├── Deduplication                                      │
│               └── MergedRAGContext ────────► LLM prompt builder      │
│                                                                      │
│  [Layer D]  FeedbackSummary (embedding-only, no raw text)            │
│             FeedbackCollector ─────────► CrossShardFeedbackSync      │
│             CrossShardFeedbackSync ────► GossipProtocol (outbound)   │
│             GossipProtocol (inbound) ──► CrossShardFeedbackSync      │
│             CrossShardFeedbackSync ────► RLAIFTrainer                │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 4. Privacy & Security Architecture

| Concern | Mechanism | Location |
|---|---|---|
| No raw training data egress | Only `EncryptedGradient::data` (numeric delta map) transmitted | `lora_federation_coordinator.h` |
| (ε, δ)-Differential Privacy | Gaussian mechanism σ = Δ·√(2·ln(1.25/δ))/ε applied before distribution | `lora_federation_coordinator.cpp::applyDifferentialPrivacy()` |
| No raw DBA query text egress | `CrossShardFeedbackSync::publishFeedback()` enforces `shard_origin="ANON"`, only `reason_embedding` | `cross_shard_feedback_sync.cpp` |
| Gossip message authentication | `GossipProtocol::verifyMessage()` HMAC + mTLS per peer | external (gossip_protocol) |
| GDPR cross-border check | Caller integrates `CrossBorderTransferPolicy` before triggering federation round | external (governance) |
| Post-quantum audit log | `SphincsPlus` audit log written by coordinator after each round | external (post_quantum_crypto) |
| Privacy budget exhaustion guard | `DifferentialPrivacyManager::verifyPrivacyBudget()` called before each round | external (federated_learning) |

---

## 5. Configuration Reference

### Layer B — FederationConfig

```cpp
struct FederationConfig {
    size_t min_participants      = 2;
    size_t max_participants      = 64;
    std::chrono::hours federation_interval{24};
    std::string aggregation_algorithm = "FedAvg"; // "FedAvg" | "FedProx" | "median"
    bool   weight_by_sample_count = true;
    double dp_epsilon             = 0.1;
    double dp_delta               = 1e-5;
    double dp_sensitivity         = 1.0;
    std::chrono::minutes round_timeout{60};
};
```

### Layer C — FederatedRAGMergerConfig

```cpp
struct FederatedRAGMergerConfig {
    MergeStrategy strategy          = MergeStrategy::RECIPROCAL_RANK_FUSION;
    size_t        top_k             = 20;
    bool          deduplicate       = true;
    double        rrf_constant      = 60.0;   // Cormack 2009
    bool          boost_specialised = true;
    double        specialisation_boost = 1.2;
};
```

### Layer D — FeedbackSyncConfig

```cpp
struct FeedbackSyncConfig {
    size_t max_embedding_dim      = 384;
    size_t dedup_cache_size       = 10000;
    bool   validate_embedding_dim = true;
};
```

---

## 6. Error Handling

| Situation | Behaviour |
|---|---|
| `submitGradient()` for wrong round | Silently ignored (idempotent) |
| `triggerAggregation()` below min_participants | Throws `std::runtime_error` |
| `publishFeedback()` with wrong embedding dim | Throws `std::invalid_argument` |
| Shard returns `ok=false` in `ShardRetrievalResult` | Skipped in merge; `shards_responded` not incremented |
| `handleInboundSummary()` duplicate `summary_id` | Silently deduplicated, callback not invoked |
| DP budget exceeded | Round skipped; `verifyPrivacyBudget()` returns false |
| gossip_message_fn is null | `announce()` / `publishFeedback()` skip dispatch (no crash) |

---

## 7. Testing Strategy

| Test Type | Target | Location |
|---|---|---|
| Unit | All 4 components, 25+ cases | `tests/test_distributed_knowledge.cpp` |
| Privacy | DP noise is non-zero; gradient anonymity | `tests/test_distributed_knowledge.cpp` (DK-B-*) |
| Merge | RRF correctness, dedup, top_k cut | `tests/test_distributed_knowledge.cpp` (DK-C-*) |
| Dedup | FeedbackSummary round-trip, dedup | `tests/test_distributed_knowledge.cpp` (DK-D-*) |
| Integration | Full round A→B→C→D with mock gossip | `tests/test_distributed_knowledge_integration.cpp` (Session 7) |

---

## 8. Module Conventions

- Namespace: `themis::distributed_knowledge`
- All classes use PIMPL or member `std::mutex mutex_` for thread safety
- No raw pointers for owned resources; `std::unique_ptr` / `std::shared_ptr`
- Config structs validated via `isValid()` before construction
- Serialisation: `nlohmann::json` (same as rest of codebase)
- Error reporting: exceptions for unrecoverable errors, silent-skip for idempotent ops

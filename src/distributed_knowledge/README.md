> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# distributed_knowledge Module — Implementation Overview

**Version:** v1.1.0
**Status:** ✅ Production-ready
**Last Updated:** 2026-05-11

---

## Purpose

`distributed_knowledge` implements ThemisDB Layer-11 federation flows:

- Layer A: adapter capability gossip (`AdapterCapabilityAnnouncement`)
- Layer B: federated LoRA gradient aggregation (`LoRAFederationCoordinator`)
- Layer C: cross-shard RAG result merge (`FederatedRAGMerger`)
- Layer D: cross-shard feedback synchronisation for RLAIF (`CrossShardFeedbackSync`)

The module enforces zero raw-data egress and privacy-aware cross-shard exchange.

---

## Source Layout

| File | Role |
|---|---|
| `adapter_capability_announcement.h/.cpp` | Gossip payload + publisher/consumer callback wiring |
| `lora_federation_coordinator.h/.cpp` | Federated gradient aggregation + DP controls + audit hooks |
| `federated_rag_merger.h/.cpp` | RRF / weighted / round-robin merge + dedup + timeout handling |
| `cross_shard_feedback_sync.h/.cpp` | Anonymised feedback publication, dedup, policy/ZeroTrust gate |
| `federated_distillation_coordinator.h/.cpp` | Teacher→student soft-label distillation with DP budget and policy gates |

## Public API (Entry Points)

Primary headers in `include/distributed_knowledge/`:

- [`adapter_capability_announcement.h`](../../include/distributed_knowledge/adapter_capability_announcement.h)
  - `AdapterCapabilityAnnouncement`, `GossipAdapterPublisher`
- [`lora_federation_coordinator.h`](../../include/distributed_knowledge/lora_federation_coordinator.h)
  - `ILoRAFederationCoordinator`, `LoRAFederationCoordinator`, `FederationConfig`
- [`federated_rag_merger.h`](../../include/distributed_knowledge/federated_rag_merger.h)
  - `FederatedRAGMerger`, `FederatedRAGMergerConfig`, `MergedRAGContext`
- [`cross_shard_feedback_sync.h`](../../include/distributed_knowledge/cross_shard_feedback_sync.h)
  - `CrossShardFeedbackSync`, `FeedbackSyncConfig`, `FeedbackSummary`
- [`federated_distillation_coordinator.h`](../../include/distributed_knowledge/federated_distillation_coordinator.h)
  - `FederatedDistillationCoordinator`, `DistillationConfig`, `DistillationRound`

---

## Installation

No standalone installation is required. The module is built as part of the
regular ThemisDB CMake build.

## Usage

The module is consumed via its integration points in sharding, training, query,
and prompt/RLAIF components; see the verification references below for concrete
entry points and tests.

### Minimal usage snippets

```cpp
using namespace themis::distributed_knowledge;

FederationConfig cfg{};
cfg.min_participants = 3;
cfg.dp_epsilon = 0.1;

LoRAFederationCoordinator coordinator(cfg);
coordinator.setGlobalDeltaCallback(
    [](const GlobalAdapterDelta& delta) { /* applyGlobalDelta(...) */ });
```

```cpp
FederatedRAGMergerConfig merge_cfg{};
merge_cfg.strategy = MergeStrategy::RECIPROCAL_RANK_FUSION;
merge_cfg.top_k = 20;
FederatedRAGMerger merger(merge_cfg);
```

## Configuration options (high-impact)

| Config type | Key options | Runtime impact |
|---|---|---|
| `FederationConfig` | `min_participants`, `aggregation_algorithm`, `dp_epsilon`, `max_rounds`, `round_timeout_ms` | Aggregation cadence, privacy budget consumption, timeout/failure behavior |
| `FederatedRAGMergerConfig` | `strategy`, `top_k`, `deduplicate`, `shard_timeout_ms` | Recall/latency trade-offs in cross-shard RAG merge |
| `FeedbackSyncConfig` | `max_embedding_dim`, `dedup_cache_size`, `validate_embedding_dim` | Input validation strictness and memory profile for dedup cache |
| `DistillationConfig` | `dp_epsilon`, `dp_delta`, `temperature`, `min_utility_threshold`, `require_dp` | Distillation quality, privacy cost, and rollback sensitivity |

## Runtime behavior, failure modes, and limits

- No raw query text, raw data, or model weights are exchanged across shards.
- LoRA federation:
  - duplicate gradient submissions for the same `(shard_id, round)` are ignored
  - aggregation throws on insufficient participants or exhausted privacy budget
- RAG merge:
  - failed/timed-out shards are skipped, unless all shards time out (`runtime_error`)
  - `top_k`, deduplication, and merge strategy determine final ranking behavior
- Feedback sync:
  - invalid embedding dimensions can throw `std::invalid_argument`
  - repeated `summary_id` payloads are deduplicated and not reprocessed
- Distillation:
  - policy gate can block broadcast (`runtime_error`)
  - utility below threshold can trigger rollback callback path

---

## Troubleshooting

- **No federated LoRA aggregation happens**
  - Check `min_participants`, `round_timeout_ms`, and whether gradients are submitted for the current round.
- **RAG results look incomplete**
  - Inspect shard timeout behavior (`shard_timeout_ms`) and failed shard responses (`ok=false`).
- **Feedback summaries are dropped**
  - Validate embedding dimension (`max_embedding_dim`) and inbound policy/ZeroTrust checks.
- **Distillation broadcast fails**
  - Verify policy gate outcome and DP configuration (`dp_epsilon`, `dp_delta`, `require_dp`).

## Verification References

- Unit tests: `tests/test_distributed_knowledge.cpp`
- Integration tests: `tests/test_distributed_knowledge_integration.cpp`
- OR hardening tests: `tests/test_distributed_knowledge_or.cpp`
- Admin/privacy tests: `tests/test_federation_admin.cpp`
- Benchmarks: `benchmarks/bench_distributed_knowledge.cpp`, `benchmarks/bench_distributed_knowledge_or.cpp`

---

## Primary Documentation

- [Architecture](./ARCHITECTURE.md)
- [Roadmap](./ROADMAP.md)
- [Future Enhancements](./FUTURE_ENHANCEMENTS.md)
- [Changelog](./CHANGELOG.md)
- [Security notes](./SECURITY.md)
- [Global module map entry](../MODULE_FUNCTION_USAGE_MAP.md#module-distributed_knowledge)

## Related docs

- [Distributed knowledge federation research (EN)](../../docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md)
- [Distributed knowledge federation research (DE)](../../docs/de/research/VERTEILTES_WISSEN_FEDERATION.md)
- [Secondary module overview (EN)](../../docs/en/distributed_knowledge/README.md)
- [Secondary module overview (DE)](../../docs/de/distributed_knowledge/README.md)

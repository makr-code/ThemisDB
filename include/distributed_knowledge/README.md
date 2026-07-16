> **Build:** `cmake --preset release && cmake --build build/release`

# distributed_knowledge — Public Headers

**Module Path:** `include/distributed_knowledge/`
**Implementation:** `../../src/distributed_knowledge/`
**Last Updated:** 2026-05-11

## Purpose

Public interfaces for ThemisDB's distributed knowledge federation — cross-shard model feedback, federated RAG merging, and LoRA adapter coordination.

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `adapter_capability_announcement.h` | `AdapterCapabilityAnnouncement` — broadcasts adapter capabilities across shards |
| `cross_shard_feedback_sync.h` | `CrossShardFeedbackSync` — synchronises model feedback across shards |
| `federated_rag_merger.h` | `FederatedRAGMerger` — merges RAG results from multiple federated nodes |
| `lora_federation_coordinator.h` | `LoRAFederationCoordinator` — coordinates LoRA adapter distribution |
| `federated_distillation_coordinator.h` | `FederatedDistillationCoordinator` — coordinates teacher-student soft-label federation |

## Public API groups

- **Layer A (Capability Gossip):**
  - `AdapterCapabilityAnnouncement`, `GossipAdapterPublisher`
- **Layer B (Federated LoRA):**
  - `ILoRAFederationCoordinator`, `LoRAFederationCoordinator`, `FederationConfig`
- **Layer C (Federated RAG):**
  - `FederatedRAGMerger`, `FederatedRAGMergerConfig`, `MergedRAGContext`
- **Layer D (Feedback Sync / RLAIF):**
  - `CrossShardFeedbackSync`, `FeedbackSyncConfig`, `FeedbackSummary`
- **Distillation extension:**
  - `FederatedDistillationCoordinator`, `DistillationConfig`, `DistillationRound`

## Configuration quick reference

| Struct | Important fields | Typical checks |
|---|---|---|
| `FederationConfig` | `min_participants`, `dp_epsilon`, `dp_delta`, `max_rounds`, `round_timeout_ms` | `isValid()`, privacy budget exhaustion, participant threshold |
| `FederatedRAGMergerConfig` | `strategy`, `top_k`, `deduplicate`, `shard_timeout_ms` | `isValid()`, timeout coverage |
| `FeedbackSyncConfig` | `max_embedding_dim`, `dedup_cache_size`, `validate_embedding_dim` | embedding-size mismatch and dedup behavior |
| `DistillationConfig` | `dp_epsilon`, `dp_delta`, `temperature`, `min_utility_threshold`, `require_dp` | policy gate behavior, rollback threshold |

## Runtime behavior and limits

- Raw training data and raw prompt/query text are not exchanged between shards.
- Federation and distillation enforce DP-budget and policy-gate checks; invalid settings can block round execution.
- RAG merge may skip failed/timed-out shards; merged output remains bounded by `top_k`.
- Feedback sync rejects malformed embeddings when validation is enabled and deduplicates repeated `summary_id`s.

## Build

```cmake
cmake --preset linux-release && cmake --build --preset linux-release
```

## Usage

```cpp
#include "distributed_knowledge/lora_federation_coordinator.h"

themis::distributed_knowledge::FederationConfig cfg{};
cfg.min_participants = 3;
cfg.dp_epsilon = 0.1;
themis::distributed_knowledge::LoRAFederationCoordinator coordinator(cfg);
```

```cpp
#include "distributed_knowledge/federated_rag_merger.h"

themis::distributed_knowledge::FederatedRAGMerger merger(
    themis::distributed_knowledge::FederatedRAGMergerConfig{});
```

## Troubleshooting

- **`FederationConfig::isValid()` fails**
  - Ensure `dp_epsilon > 0`, `dp_delta > 0`, and `max_participants >= min_participants`.
- **No merged RAG context returned**
  - Verify shard responses, timeout configuration, and `top_k > 0`.
- **Feedback sync throws on publish**
  - Check embedding dimension vs. `FeedbackSyncConfig::max_embedding_dim`.
- **Distillation broadcast rejected**
  - Inspect configured policy gate and DP parameters.

## See Also

- [`../../src/distributed_knowledge/README.md`](../../src/distributed_knowledge/README.md) — implementation details
- [`../../src/distributed_knowledge/ROADMAP.md`](../../src/distributed_knowledge/ROADMAP.md)
- [`../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`](../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md)
- [`../../docs/en/distributed_knowledge/README.md`](../../docs/en/distributed_knowledge/README.md)
- [`../../docs/de/distributed_knowledge/README.md`](../../docs/de/distributed_knowledge/README.md)

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

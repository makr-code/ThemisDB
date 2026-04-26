> **Build:** `cmake --preset release && cmake --build build/release`

# distributed_knowledge — Public Headers

**Module Path:** `include/distributed_knowledge/`
**Implementation:** `../../src/distributed_knowledge/`

## Purpose

Public interfaces for ThemisDB's distributed knowledge federation — cross-shard model feedback, federated RAG merging, and LoRA adapter coordination.

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `adapter_capability_announcement.h` | `AdapterCapabilityAnnouncement` — broadcasts adapter capabilities across shards |
| `cross_shard_feedback_sync.h` | `CrossShardFeedbackSync` — synchronises model feedback across shards |
| `federated_rag_merger.h` | `FederatedRAGMerger` — merges RAG results from multiple federated nodes |
| `lora_federation_coordinator.h` | `LoRAFederationCoordinator` — coordinates LoRA adapter distribution |

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-distributed-knowledge
```

## See Also

- [`../../src/distributed_knowledge/README.md`](../../src/distributed_knowledge/README.md) — implementation details

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

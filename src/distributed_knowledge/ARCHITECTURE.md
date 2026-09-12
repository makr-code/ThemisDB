# Architecture - Distributed Knowledge Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The distributed_knowledge module composes cross-shard federation and coordination paths into a bounded runtime contract. It covers capability exchange, federated aggregation, retrieval merge, feedback propagation, and distillation workflows under policy and privacy controls.

## Main Execution Planes

1. Capability and routing plane
- capability announcement handling across shard boundaries
- domain/capability-informed routing support surfaces

2. Federation and aggregation plane
- federated contribution intake and round coordination
- aggregation lifecycle with bounded round and timeout semantics

3. Retrieval merge plane
- cross-shard retrieval fusion with configurable merge behavior
- partial response handling and deterministic merge constraints

4. Feedback and distillation plane
- cross-shard feedback synchronization and dedup behavior
- distillation governance paths under policy/privacy controls

## Core Contracts

| Contract | Behavior |
|---|---|
| capability contract | explicit capability announcement and consumption flow |
| federation contract | bounded contribution intake and aggregation semantics |
| merge contract | deterministic cross-shard merge behavior under partial failures |
| feedback/distillation contract | privacy-aware cross-shard sync and distillation coordination |

## Failure Semantics

- policy/privacy violations fail closed before federation propagation.
- timeout/failed-shard paths degrade gracefully where contractually allowed.
- invalid federation payloads fail with explicit structured error behavior.

## Sourcecode Verification (Module: distributed_knowledge/architecture)

- Verified files:
  - src/distributed_knowledge/adapter_capability_announcement.cpp
  - src/distributed_knowledge/lora_federation_coordinator.cpp
  - src/distributed_knowledge/federated_rag_merger.cpp
  - src/distributed_knowledge/cross_shard_feedback_sync.cpp
  - src/distributed_knowledge/federated_distillation_coordinator.cpp
- Verified architecture claims:
  - explicit capability/federation/merge/sync planes
  - bounded failure semantics for policy and timeout paths
  - module-local coordination ownership for distributed knowledge workflows

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| graph | `include/graph/` | Graph traversal used for federated knowledge graph expansions |
| sharding | `include/sharding/` | Shard routing and capability advertisement across cluster shards |
| llm | `include/llm/` | Federated LoRA coordination uses LLM adapter interfaces |
| rag | `include/rag/` | RAG module provides cross-shard retrieval and feedback sync inputs |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| query | `include/distributed_knowledge/` (federated_rag_merger.h) | Query uses federated RAG merger for cross-shard result fusion |
| sharding | `include/distributed_knowledge/` (adapter_capability_announcement.h) | Sharding module announces shard capabilities through distributed knowledge interfaces |
| rag | `include/distributed_knowledge/` (cross_shard_feedback_sync.h) | RAG synchronises cross-shard relevance feedback via this module |

## Integration Points

### Critical Integration: Query Federated RAG Merger
**Files:** `src/distributed_knowledge/federated_rag_merger.cpp` ↔ `query/federated_rag_merger.h`
**Contract:** Query issues a federated merge request with per-shard result lists; merger returns a ranked unified result respecting policy/privacy controls.
**Thread Safety:** Each merge operation is stateless and re-entrant; concurrent merge calls are safe.

### Critical Integration: Cross-Shard Feedback Sync
**Files:** `src/distributed_knowledge/cross_shard_feedback_sync.cpp` ↔ `rag/`
**Contract:** RAG pushes feedback events; dedup and sync logic serialises delivery to avoid duplicate propagation across shards.
**Thread Safety:** Feedback dedup state is protected by an internal mutex; concurrent push calls are safe.
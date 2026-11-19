---
Author: "ThemisDB Contributors"
Created: "2026-09-09"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Architecture - Distributed Knowledge Module

<!-- Status: evidence reconciliation | source review: 2026-11-19; no real shard-topology validation -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The distributed_knowledge module provides in-process coordination components for capability announcements, federated aggregation, retrieval merge, feedback callbacks, and distillation callbacks. It does not implement a graph store, graph synchronization protocol, peer transport, durable replication, consensus, or restart recovery.

## Main Execution Planes

1. Capability announcement plane
- local announcements are serialized and passed to a supplied gossip callback
- inbound announcements require a configured trust policy and are dispatched only after it permits them
- the module does not authenticate the transport; `AlwaysPermitTrustPolicy` is permissive, not an authentication mechanism

2. Federation and aggregation plane
- in-memory contribution intake and round coordination
- policy/privacy checks occur in the local coordinator; submissions from non-current rounds and duplicate shard IDs are ignored

3. Retrieval merge plane
- merges already-fetched shard results using the configured strategy
- failed/timed-out shard results are skipped if another response is usable; an all-timeout set fails with an exception
- no request fan-out, deadline enforcement, retry, or version/causal guard is performed

4. Feedback and distillation plane
- callback-based feedback publication with local bounded in-memory dedup state
- distillation uses in-memory rounds and invokes registered student callbacks synchronously

## Core Contracts

| Contract | Behavior |
|---|---|
| capability contract | unknown domain names fail deserialization; inbound delivery fails closed without an explicit trust policy |
| federation contract | configured participant, round, and privacy limits are checked locally; durability and peer agreement are external |
| merge contract | deterministic local merge over supplied responses; freshness is eventual and partial results are exposed through response counts |
| feedback/distillation contract | policy/privacy checks and callback dispatch are local; no durable delivery or cross-process state is provided |

## Failure Semantics

- a missing/rejecting capability trust policy and an unsupported capability domain do not dispatch inbound announcements.
- policy/privacy violations in aggregation and distillation reject the local operation.
- failed RAG shards can produce a partial result; all supplied shards timed out produces an exception.
- feedback policy rejection is counted and dropped; publish callback exceptions are counted as skipped publishes, not delivered.
- LoRA timeout overloads do not cancel unfinished work and are not hard wall-clock deadlines.
- all coordination state and counters are process-local; restart does not recover rounds, privacy budgets, or dedup state.

The consistency descriptions below apply only to these local APIs. They do not
establish graph consistency or cross-shard convergence:

| Surface | Local consistency | Degraded/invalid outcome |
|---|---|---|
| Capability announcement | callback order only; no version arbitration | invalid domain throws; missing/rejecting trust policy throws before callback |
| LoRA aggregation | current-round intake; in-memory round updates | stale/future/duplicate submissions are ignored; insufficient participants and privacy/policy violations throw |
| RAG merge | stateless eventual read merge | partial response counts remain visible; all-timeout input throws |
| Feedback sync | eventual, deduplicated within bounded local cache | policy rejection increments a counter and drops; callback exception increments skipped-publish count |
| Distillation | in-memory causal round counter | invalid config or exhausted limits/policy gate reject; callback failures can leave delivery partial |

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
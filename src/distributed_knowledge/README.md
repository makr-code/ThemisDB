# ThemisDB Distributed Knowledge Module

<!-- Status: PRODUCTION_CANDIDATE (Wave B Distribution) | Phase 1-3 complete | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- Wave Context: Wave B (Performance Consolidation Q3-Q4 2026) — Partition Tolerance + Convergence Guarantees + Cross-Shard Consistency -->

## Module Purpose

Production-candidate cross-shard knowledge federation runtime surfaces for ThemisDB, including adapter capability exchange, federated aggregation, cross-shard retrieval merge, and privacy-aware feedback synchronization. **Batch 5 enhancement focus: Partition tolerance under shard failures, convergence guarantees, cross-shard consistency validation**.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| adapter_capability_announcement.cpp | adapter capability announcement and gossip integration |
| lora_federation_coordinator.cpp | federated LoRA aggregation coordination |
| federated_rag_merger.cpp | cross-shard RAG result merge orchestration |
| cross_shard_feedback_sync.cpp | cross-shard feedback sync and dedup paths |
| federated_distillation_coordinator.cpp | federated distillation coordination with policy/DP guards |

## Scope

In scope:
- cross-shard capability and federation coordination workflows
- federated merge and synchronization behavior across shard boundaries
- privacy-aware federation controls and observability surfaces

Out of scope:
- local-only shard internals outside federation interfaces
- unrelated query/training subsystem ownership not exposed via module contracts
- non-distributed single-node control-plane behavior

## Runtime Behavior and Limits

- behavior depends on federation policy settings, timeouts, and shard response quality.
- partial shard failures are expected and handled by bounded merge/sync logic.
- privacy and policy gates can reject federation actions before propagation.

## Sourcecode Verification (Module: distributed_knowledge/readme)

- Verified files:
  - src/distributed_knowledge/adapter_capability_announcement.cpp
  - src/distributed_knowledge/lora_federation_coordinator.cpp
  - src/distributed_knowledge/federated_rag_merger.cpp
  - src/distributed_knowledge/cross_shard_feedback_sync.cpp
  - src/distributed_knowledge/federated_distillation_coordinator.cpp
- Verified behavior surfaces:
  - capability announcement and federation routing surfaces
  - federated aggregation and cross-shard merge behavior
  - cross-shard feedback synchronization and privacy-aware gates
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
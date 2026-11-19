---
Author: "ThemisDB Contributors"
Created: "2026-08-14"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Distributed Knowledge Module

<!-- Status: HARDENING / EVIDENCE INCOMPLETE | source review: 2026-11-19; runtime validation remains open -->
<!-- Links: API.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PERFORMANCE_EXPECTATIONS.md -->

## Module Purpose

In-process coordination components for capability announcements, federated aggregation, merging caller-fetched retrieval responses, feedback callbacks, and distillation callbacks. These APIs do not implement a graph store, graph synchronization protocol, peer transport, durable state, consensus, or restart recovery.

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
- callback-based capability announcement, aggregation, retrieval merge, feedback, and distillation coordination
- local validation, policy/privacy gates, partial-result accounting, and process-local observability

Out of scope:
- graph synchronization, remote fan-out, peer authentication, durable delivery, retry/cancellation protocols, and restart recovery
- local-only shard internals outside the callback interfaces

## Runtime Behavior and Limits

- Inbound capability dispatch requires an explicit trust policy; missing or rejecting policies throw before callback dispatch. Unknown domain strings throw rather than mapping to `GENERAL`.
- RAG merge skips failed/timed-out inputs when usable responses remain and throws when all supplied responses time out. The merger does not fetch responses or enforce remote deadlines.
- Feedback is best effort; publish callback exceptions are counted as skipped, and dedup state is bounded and in-memory.
- LoRA and distillation rounds, counters, and privacy accounting are process-local. Callback failure may leave partial delivery; no module retry or recovery loop is provided.
- `AlwaysPermitTrustPolicy` is permissive and is not authentication or production authorization.

## Sourcecode Verification (Module: distributed_knowledge/readme)

- Verified files:
  - src/distributed_knowledge/adapter_capability_announcement.cpp
  - src/distributed_knowledge/lora_federation_coordinator.cpp
  - src/distributed_knowledge/federated_rag_merger.cpp
  - src/distributed_knowledge/cross_shard_feedback_sync.cpp
  - src/distributed_knowledge/federated_distillation_coordinator.cpp
- Verified behavior surfaces:
  - capability announcement, federation, merge, feedback, and distillation components
  - in-process partial/degraded behavior only; no real shard topology is exercised
  - explicit trust/policy gates with the limitations described above
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
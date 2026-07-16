> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/distributed_knowledge/ARCHITECTURE.md -->

# Distributed Knowledge Module — Public Header Architecture

**Module Path:** `include/distributed_knowledge/`  
**Implementation:** `../../src/distributed_knowledge/`  
**Canonical architecture doc:** [`../../src/distributed_knowledge/ARCHITECTURE.md`](../../src/distributed_knowledge/ARCHITECTURE.md)

---

## 1. Overview

`include/distributed_knowledge/` defines the **public federated RAG merging, LoRA federation, federated distillation, cross-shard feedback sync, and adapter capability announcements API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/distributed_knowledge/ARCHITECTURE.md`](../../src/distributed_knowledge/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Federated Learning

| Header | Public Type | Purpose |
|--------|------------|---------|
| `federated_distillation_coordinator.h` | `FederatedDistillationCoordinator` | Cross-node knowledge distillation coordination |
| `federated_rag_merger.h` | `FederatedRAGMerger` | Distributed RAG result merging |
| `lora_federation_coordinator.h` | `LoRAFederationCoordinator` | Multi-shard LoRA adapter federation |
### 2.2 Coordination and Sync

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adapter_capability_announcement.h` | `AdapterCapabilityAnnouncement` | Capability advertisement for remote adapters |
| `cross_shard_feedback_sync.h` | `CrossShardFeedbackSync` | Feedback-signal synchronisation across shards |

---

## 3. Namespace Layout

All public types reside in the `themis::distributed_knowledge` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/distributed_knowledge/` expose the **stable public API**; internal types live in `src/distributed_knowledge/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM/Tensor**.

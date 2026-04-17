# distributed_knowledge Module — Implementation Overview

**Version:** v1.0.0
**Status:** ✅ Production-ready
**Last Updated:** 2026-04-17

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

---

## Installation

No standalone installation is required. The module is built as part of the
regular ThemisDB CMake build.

## Usage

The module is consumed via its integration points in sharding, training, query,
and prompt/RLAIF components; see the verification references below for concrete
entry points and tests.

---

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

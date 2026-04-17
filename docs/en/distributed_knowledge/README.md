[docs](../../README.md) > [en](../README.md) > [distributed_knowledge](./README.md) > [reference](./README.md)
**Datum:** 2026-04-17
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/distributed_knowledge/README.md`
- `src/distributed_knowledge/ARCHITECTURE.md`
- `src/distributed_knowledge/ROADMAP.md`
- `src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`
- `src/distributed_knowledge/CHANGELOG.md`

**Bezug / Reference:**
- Issue: #4712
- Context: Secondary documentation update for the `distributed_knowledge` module migration and reality-check.

---

# distributed_knowledge — Module Overview (Secondary Docs)

## TL;DR

Implementation is production-wired across Layers A–D, including DK-OR hardening.
This page mirrors the validated state from primary sources.

## Validated implementation state

- **Layer A (gossip + capability routing):** available (`include/sharding/gossip_protocol.h`, `include/sharding/adaptive_shard_router.h`, `tests/test_gossip_custom_handler.cpp`)
- **Layer B (FedLoRA):** available (`include/training/incremental_lora_trainer.h`, `tests/test_incremental_lora_trainer.cpp`)
- **Layer C (federated RAG merge):** available (`include/query/query_federation.h`, `tests/test_query_federation.cpp`)
- **Layer D (federated feedback/RLAIF):** available (`include/prompt_engineering/feedback_collector.h`, `include/rag/rlaif_trainer.h`, `tests/test_feedback_collector.cpp`)
- **Integration/OR/Admin:** available (`tests/test_distributed_knowledge_integration.cpp`, `tests/test_distributed_knowledge_or.cpp`, `tests/test_federation_admin.cpp`)

## Installation

No standalone installation is required. The module is included in the standard
ThemisDB build.

## Usage

Usage is via the existing integration points in sharding, training, query, and
RLAIF/prompt-engineering components (see primary docs and referenced tests).

## Research notes and constraints

- Research baseline:
  - `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`
  - `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md`
- Mandatory constraints in primary docs: zero raw-data egress, DP budget limits, and custom gossip handlers instead of protocol-level changes (`src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`).

## Navigation

- Primary source index: [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)
- Gap report (DE): [`../../de/distributed_knowledge/missing-implementations.md`](../../de/distributed_knowledge/missing-implementations.md)

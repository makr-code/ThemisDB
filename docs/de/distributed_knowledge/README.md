[docs](../../README.md) > [de](../README.md) > [distributed_knowledge](./README.md) > [reference](./README.md)
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
- Kontext: Sekundärdokumentation für den Reality-Check und die Doku-Migration des Moduls `distributed_knowledge`.

---

# distributed_knowledge — Modulüberblick (Sekundärdoku)

## TL;DR

Das Modul ist auf Implementierungsseite produktiv verdrahtet (Layer A–D inkl. DK-OR-Hardening).
Diese Sekundärdoku spiegelt den verifizierten Stand aus den Primärquellen wider.

## Verifizierter Implementierungsstand

- **Layer A (Gossip/Capability Routing):** vorhanden (`include/sharding/gossip_protocol.h`, `include/sharding/adaptive_shard_router.h`, `tests/test_gossip_custom_handler.cpp`)
- **Layer B (FedLoRA):** vorhanden (`include/training/incremental_lora_trainer.h`, `tests/test_incremental_lora_trainer.cpp`)
- **Layer C (Federated RAG Merge):** vorhanden (`include/query/query_federation.h`, `tests/test_query_federation.cpp`)
- **Layer D (Federated Feedback/RLAIF):** vorhanden (`include/prompt_engineering/feedback_collector.h`, `include/rag/rlaif_trainer.h`, `tests/test_feedback_collector.cpp`)
- **Integration/OR/Admin:** vorhanden (`tests/test_distributed_knowledge_integration.cpp`, `tests/test_distributed_knowledge_or.cpp`, `tests/test_federation_admin.cpp`)

## Installation

Keine separate Installation notwendig. Das Modul wird über den regulären
ThemisDB-Build mitgebaut.

## Usage

Nutzung erfolgt über die vorhandenen Integrationspunkte in Sharding, Training,
Query und RLAIF/Prompt-Engineering (siehe Primärquellen und Testreferenzen).

## Research-Hinweise und Design-Constraints

- Forschungsbasis:
  - `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md`
  - `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`
- Verbindliche Constraints laut Primärdoku: kein Raw-Data-Egress, DP-Budget-Guardrails, Gossip Custom-Handler statt Protokollbruch (`src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`).

## Navigationslinks

- Primärquellenindex: [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)
- Gap-Report: [`missing-implementations.md`](./missing-implementations.md)

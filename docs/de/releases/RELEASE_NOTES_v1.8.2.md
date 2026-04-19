# ThemisDB v1.8.2 — Release-Aggregation

**Release Date:** 2026-04-19 (Aggregation)
**Type:** Patch / Stabilisierung
**Previous Version:** v1.8.0
**Milestone:** current (`milestone:current`)
**Aggregation Scope:** Retrospektive Zuordnung aller Feature-Wellen zwischen v1.8.0 und v1.8.2 auf Basis von `roadmap.md` und Merge-Liste.

---

## 🎯 Überblick

v1.8.2 konsolidiert die seit v1.8.0 ausgelieferten funktionalen Erweiterungen in zwei Blöcken:

1. **v1.8.1-rc1 Feature-Welle** (Geo/Search/Storage/Sharding Hardening)
   Referenz: [RELEASE_NOTES_v1.8.1-rc1.md](./RELEASE_NOTES_v1.8.1-rc1.md)
2. **Post-RC Stabilisierung bis v1.8.2** (RAG/LLM/Storage/Process/Voice/RPC Hardening)
   Referenz: Merge-Liste der gemergten PRs (unten)

---

## 📦 PR-Auflistung mit funktionaler Zuordnung (v1.8.0 → v1.8.2)

| PR | Bereich | Funktionale Erweiterung / Hinweis |
|---|---|---|
| [#4697](https://github.com/makr-code/ThemisDB/pull/4697) | rag / toolbox / ingestion | `RAGIngestionBridge`, neue Workflow-Profile (`pdf`, `office`, `audio`, `archive`), MIME-Erweiterungen |
| [#4708](https://github.com/makr-code/ThemisDB/pull/4708) | plugins / server / voice / rpc / ethics | Ersetzt No-op-Stubs durch produktive Implementierungen (u. a. Voice-Synthese, RPC-Differential-Chunks, Nutzerlisten) |
| [#4711](https://github.com/makr-code/ThemisDB/pull/4711) | storage | `IVectorIndexBackend` + `InMemoryVectorIndex`, `EncryptedBlobBackend` (AES-256-GCM) |
| [#4713](https://github.com/makr-code/ThemisDB/pull/4713) | process / sharding / cli | EPK/ARIS-AML Importer, `ProcessAgenticRag`, Replica-Validierungs-Wiring, `themisctl config validate` |
| [#4723](https://github.com/makr-code/ThemisDB/pull/4723) | llm / sharding | Adaptive Batch-Retry Telemetrie + RAID-Sharding-Interlock-Hints |
| [#4726](https://github.com/makr-code/ThemisDB/pull/4726) | llm / aql / sharding | Domain-aware `INFER` Routing, async Batch Fan-out, LLM Shard Telemetrie, Cross-Shard Runbook |

---

## 🗺️ Roadmap-Zuordnung

| Roadmap-Quelle | Zuordnung in dieser Aggregation |
|---|---|
| [`roadmap.md` — Milestone v1.8.0](../../../roadmap.md) | Basis-Feature-Stand aus v1.8.0 bleibt erhalten |
| [`docs/de/releases/RELEASE_NOTES_v1.8.1-rc1.md`](./RELEASE_NOTES_v1.8.1-rc1.md) | Vollständige RC1-Welle zwischen v1.8.0 und v1.8.2 referenziert |
| Modul-Roadmaps ([`src/rag/ROADMAP.md`](../../../src/rag/ROADMAP.md), [`src/storage/ROADMAP.md`](../../../src/storage/ROADMAP.md), [`src/process/ROADMAP.md`](../../../src/process/ROADMAP.md), [`src/llm/ROADMAP.md`](../../../src/llm/ROADMAP.md), [`src/sharding/ROADMAP.md`](../../../src/sharding/ROADMAP.md)) | Post-RC-Erweiterungen durch PRs #4697, #4711, #4713, #4723, #4726 zugeordnet |

---

## ✅ Blocker- und QA-Fortschritt

### QA-Kriterien (Issue)

| Kriterium | Status | Nachweis |
|---|---|---|
| Alle Blocker-PRs behoben | ✅ Done | Offene Punkte aus LLM+RAID-Blocker-Welle wurden in [#4726](https://github.com/makr-code/ThemisDB/pull/4726) abgeschlossen |
| Regressionstests passen | ✅ Done | In den gemergten Feature-PRs wurden fokussierte Tests ergänzt/registriert (u. a. RAG/Storage/LLM) |
| Release-Doku abgeschlossen | ✅ Done | Dieses Dokument + Update der Release-Übersicht in `docs/de/releases/README.md` |

### QA-Status je Feature-Gruppe

| Gruppe | Build/Test | Doku | Release-Zuordnung |
|---|---|---|---|
| RC1-Basis (Geo/Search/Storage/Config/Sharding) | ✅ | ✅ | ✅ |
| RAG + Ingestion Bridge | ✅ | ✅ | ✅ |
| Storage Backend Layer | ✅ | ✅ | ✅ |
| Process/Sharding Integrationen | ✅ | ✅ | ✅ |
| LLM+RAID Routing/Telemetry | ✅ | ✅ | ✅ |

---

## 🔗 Verwandte Dokumentation

- [RELEASE_NOTES_v1.8.0.md](./RELEASE_NOTES_v1.8.0.md) — vorherige Aggregation
- [RELEASE_NOTES_v1.8.1-rc1.md](./RELEASE_NOTES_v1.8.1-rc1.md) — RC1-Funktionswelle
- [roadmap.md](../../../roadmap.md) — Top-level Roadmap und Milestone-Zuordnung
- [CHANGELOG.md](../../../CHANGELOG.md) — fortlaufende Änderungsdokumentation

---

*Erstellt für die Release-Aggregation v1.8.2 (milestone:current). Stand: 2026-04-19.*

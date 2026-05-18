# Gossip-Driven LoRA Domain Routing for Distributed Inference Fabrics

**Projekt:** ThemisDB
**Kategorie:** Research Documentation
**Status:** Review-ready (überarbeitet)
**Datum:** 2026-05-18
**Version:** 1.0

---

## Abstract / Zusammenfassung

Dieses Dokument überprüft den Stand von gossip-getriebenem LoRA-Domain-Routing in ThemisDB anhand von repository-nahen Artefakten (Code, Tests, Benchmarks, Modul-Dokumentation). Kernbefund: ThemisDB enthält bereits die zentralen Bausteine für domain-basierte Routingentscheidungen ohne zentrale Registry-Abhängigkeit: (1) Capability-Ankündigungen über Gossip (`AdapterCapabilityAnnouncement`, `GossipAdapterPublisher`), (2) domain-spezifische Zielwahl über `AdaptiveShardRouter::routeByDomain()`, (3) Nutzung von Domain-Hints im AQL-LLM-Pfad (`LLMAQLHandler`), inklusive lokalem Fallback bei fehlendem Match oder zu geringem Accuracy-Delta.

Die vorhandene Evidenz erlaubt belastbare Aussagen zur Implementierungsfähigkeit und zu testbarer Funktionskorrektheit. Nicht belegt sind hingegen großskalige Konvergenzgarantien, produktionsnahe Multi-Fault-Kampagnen und allgemeingültige Parameterempfehlungen für TTL/Fanout/Schwellenwerte. Das Dokument trennt daher explizit zwischen belegten Claims und offenen Evaluationsfragen.

---

## Introduction / Einleitung

### Problemstellung

In verteilten Inferenz-Clustern sind LoRA-Adapter häufig domain-spezifisch auf einzelnen Shards konzentriert. Reines hash-/topologie-basiertes Routing berücksichtigt diese Spezialisierung nicht und kann zu Qualitätsverlusten oder unnötigen Weiterleitungen führen.

### Ziel des Reviews

1. Technische Aussagen des Drafts gegen den aktuellen ThemisDB-Stand verifizieren.
2. Terminologie an ThemisDB-Konventionen angleichen (AQL, Multi-Model, Konsistenzmodell, Komponenten-Namen).
3. Review-fähige Struktur mit klaren Claim-Grenzen und belastbarer Quellenlage herstellen.

### Terminologie (vereinheitlicht)

- **AQL** = *Advanced Query Language* (ThemisDB Query-Modul).
- **Multi-Model** = relational + graph + vector + document + geospatial + time-series (`README.md`).
- **Konsistenzmodell** = ACID mit MVCC/Snapshot-Isolation (`README.md`, `ARCHITECTURE.md`).
- **Domain Routing** = Auswahl eines Ziel-Shards basierend auf domain-spezifischem `accuracy_delta` (mit Last-Tie-Break).

---

## Methodik / Ansatz

### 1) Artefaktbasierter Faktencheck

Primäre Evidenzquellen:

- Implementierung: `include/sharding/adaptive_shard_router.h`, `src/sharding/adaptive_shard_router.cpp`
- Gossip-Payload + Publisher: `include/distributed_knowledge/adapter_capability_announcement.h`, `src/distributed_knowledge/adapter_capability_announcement.cpp`
- AQL-Integration: `src/aql/llm_aql_handler.cpp`
- Tests: `tests/test_adaptive_shard_router.cpp`, `tests/test_distributed_knowledge.cpp`, `tests/test_llm_aql_handler.cpp`, `tests/test_llm_raid_routing.cpp`, `tests/test_llm_raid_integration.cpp`
- Benchmark-Anker: `benchmarks/bench_llm_raid_pipeline.cpp`

### 2) Claim-Klassifizierung

- **Bestätigt:** Direkt durch Code + Testpfad belegbar.
- **Teilweise bestätigt:** Implementierungsanker vorhanden, aber ohne belastbare End-to-End-Messkampagne.
- **Nicht bestätigt:** Behauptung ohne nachweisbare Artefakte oder Messung.

### 3) Redaktionsregeln

- Unbelegte Leistungs- oder Konvergenzversprechen entfernt oder als offen gekennzeichnet.
- Terminologie auf ThemisDB-Dokumentation abgestimmt.
- Pflichtstruktur für Review ergänzt.

---

## Evaluation / Experimente

### A) Verifizierte Implementierungsbefunde (Problem → Ansatz → Evidenz)

| Problem | Ansatz in ThemisDB | Evidenz |
|---|---|---|
| Domain-spezifische Adapterfähigkeit muss clusterweit bekannt werden | Gossip-fähiges Announcement-Schema mit Domain-Typ und Qualitäts-/Latenz-Deltas | `include/distributed_knowledge/adapter_capability_announcement.h`, `src/distributed_knowledge/adapter_capability_announcement.cpp`, `tests/test_distributed_knowledge.cpp` |
| Router muss bestgeeigneten Domain-Shard wählen | `routeByDomain()` wählt bestes `accuracy_delta`; Gleichstand via `pending_llm_requests` | `src/sharding/adaptive_shard_router.cpp`, `tests/test_adaptive_shard_router.cpp`, `tests/test_llm_raid_routing.cpp` |
| Domain-Routing muss mit AQL-LLM-Aufrufen integrierbar sein | `LLMAQLHandler` verarbeitet Domain-Hints und nutzt adaptive Router-Entscheidung mit Fallback | `src/aql/llm_aql_handler.cpp`, `tests/test_llm_aql_handler.cpp` |
| Routing-Overhead braucht Benchmark-Anker | Benchmarkfall für `routeByDomain()` vorhanden | `benchmarks/bench_llm_raid_pipeline.cpp` |

### B) Gegenprüfung zentraler Claims aus dem Draft

| Claim | Ergebnis | Begründung |
|---|---|---|
| „Gossip-basierte Capability-Verteilung ist im Code verankert“ | **Bestätigt** | Schema + Publisher + Callback-Pfad sind implementiert und getestet. |
| „Domain-Routing ist im Anfragepfad nutzbar“ | **Bestätigt** | AQL-LLM-Handler nutzt Domain-Hint und Router-Auswahl mit klaren Fallback-Zweigen. |
| „Es gibt produktionsreife Parameterempfehlungen für alle Workloads“ | **Nicht bestätigt** | Code-/Teststand belegt Mechanik, aber keine allgemeingültige Kalibrierung über große Last- und Fehlerkampagnen. |
| „Großskalige Konvergenzbeweise (z. B. >32/64 Shards) liegen vor“ | **Nicht bestätigt** | In diesem Artefaktstand sind dafür keine belastbaren Ergebnisreihen dokumentiert. |

### C) Reproduzierbare Dokumentprüfung

Für diese Datei wurden die vorhandenen Research-Checks ausgeführt:

```bash
python3 scripts/docs-lint.py research/GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md
python3 scripts/link-check.py --internal-only research/GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md
```

### D) Claim Boundaries

**Supported claims:**

- Gossip-Payloads für Adapter-Capabilities sind implementiert und serialisierbar.
- Domain-basierte Shard-Auswahl über `accuracy_delta` ist implementiert und testbar.
- AQL-LLM-Aufrufe können Domain-Hints in Routing-Entscheidungen überführen.
- Ein Benchmark-Anker für Routing-Entscheidungskosten existiert.

**Deferred claims:**

- Allgemeingültige TTL-/Fanout-/Threshold-Empfehlungen über heterogene Produktionsumgebungen.
- Robuste Aussagen zu p99-Verhalten unter langlaufenden Multi-Fault-Szenarien.
- Skalierungsgrenzen für Konvergenz unter großen Clustergrößen.

---

## Limitations / Known Issues

1. **Keine vollständige, veröffentlichte Großskalierungs-Evaluation in diesem Dokument**
   Der Stand ist repository-grounded, aber keine vollständige Publikation mit belastbarer Ergebnis-Tabelle aus reproduzierter 4-64-Shard-Messkampagne.

2. **Benchmark-Anker ist kein Ersatz für Produktions-Fault-Campaigns**
   Vorhandene Benchmarks belegen Messbarkeit einzelner Pfade, aber nicht automatisch reale Betriebsrobustheit unter kombinierter Last, Drift und Failures.

3. **Routingqualität hängt von Upstream-Qualitätssignalen ab**
   `routeByDomain()` nutzt bereitgestellte Deltas; die Güte der Entscheidung hängt von Korrektheit/Aktualität dieser Signale ab.

4. **Dokument trennt bewusst zwischen Implementierungsfähigkeit und empirischer Generalisierung**
   Diese Trennung reduziert Over-Claiming, lässt aber offene Forschungsfragen explizit bestehen.

---

## Schlussfolgerung / Fazit

ThemisDB besitzt bereits die Kernmechanismen für gossip-getriebenes LoRA-Domain-Routing im aktuellen OSS-Stand: Capability-Announcements, domain-spezifische Shard-Selektion und AQL-Integration mit Fallback-Logik. Damit ist die Architekturhypothese technisch plausibel und testbar. Nicht abgeschlossen sind robuste, verallgemeinerbare Aussagen zu Konvergenz-, Robustheits- und Parameteroptima in großskaligen Produktionsszenarien. Für eine vollwertige Publikation ist als nächster Schritt eine reproduzierbare Benchmark- und Fault-Injection-Kampagne mit dokumentierter Umgebung und Ergebnisreihen notwendig.

---

## References / Quellen

### Interne ThemisDB-Artefakte

1. ThemisDB Repository (Hauptartefakt): https://github.com/makr-code/ThemisDB
2. Adaptive Router API: https://github.com/makr-code/ThemisDB/blob/main/include/sharding/adaptive_shard_router.h
3. Adaptive Router Implementierung: https://github.com/makr-code/ThemisDB/blob/main/src/sharding/adaptive_shard_router.cpp
4. Gossip Adapter Capability Schema: https://github.com/makr-code/ThemisDB/blob/main/include/distributed_knowledge/adapter_capability_announcement.h
5. AQL LLM Handler Routingpfad: https://github.com/makr-code/ThemisDB/blob/main/src/aql/llm_aql_handler.cpp
6. Benchmark-Anker für Domain Routing: https://github.com/makr-code/ThemisDB/blob/main/benchmarks/bench_llm_raid_pipeline.cpp

### Externe Literatur (DOI/URL)

1. Hu, E. J., et al. (2021). *LoRA: Low-Rank Adaptation of Large Language Models*. arXiv. URL: https://arxiv.org/abs/2106.09685
2. McMahan, B., et al. (2017). *Communication-Efficient Learning of Deep Networks from Decentralized Data*. AISTATS (PMLR). URL: https://proceedings.mlr.press/v54/mcmahan17a.html
3. Demers, A., et al. (1987). *Epidemic Algorithms for Replicated Database Maintenance*. DOI: https://doi.org/10.1145/41840.41841
4. Kwon, W., et al. (2023). *Efficient Memory Management for Large Language Model Serving with PagedAttention (vLLM)*. DOI: https://doi.org/10.1145/3600006.3613165
5. Leviathan, Y., et al. (2023). *Fast Inference from Transformers via Speculative Decoding*. arXiv. URL: https://arxiv.org/abs/2211.17192
6. Chen, C., et al. (2023). *Accelerating Large Language Model Decoding with Speculative Sampling*. arXiv. URL: https://arxiv.org/abs/2302.01318

---

## Changelog

| Datum | Version | Änderungen |
|---|---|---|
| 2026-05-18 | 1.0 | Vollständige Review-Überarbeitung: Pflichtstruktur ergänzt, Terminologie vereinheitlicht, unbelegte Claims entfernt/markiert, evidenzbasierte Evaluation + Referenzen konsolidiert |
| 2026-04-19 | 0.2 | Vorheriger Draft-Stand |

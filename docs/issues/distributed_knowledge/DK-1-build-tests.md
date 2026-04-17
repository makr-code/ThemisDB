---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "priority:critical", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 1
---

# [DK-1] distributed_knowledge: Build-System & Unit-Tests

## Aufgabe

Das neue Modul `distributed_knowledge` kompilierbar machen und mit 25+ gezielten
Unit-Tests absichern. Alle vier Komponenten (A–D) werden isoliert getestet — ohne
laufende Shards, ohne Netz. Erst danach sind alle weiteren Issues (DK-2…DK-8)
sicher implementierbar.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `src/distributed_knowledge/CMakeLists.txt` | Integration mit GossipProtocol (→ DK-2) |
| Root-`CMakeLists.txt` `add_subdirectory` | RLAIF-Wiring (→ DK-5) |
| `tests/test_distributed_knowledge.cpp` (25+ Fälle) | End-to-End-Tests (→ DK-6) |
| Compiler-Warnings frei (`-Wall -Wextra`) | Performance-Benchmarks (→ DK-8) |
| Fix: `#include <unordered_set>` in `federated_rag_merger.cpp` | Keine neuen Algorithmen |

## Idee / Konzept

Jede der vier Komponenten hat ein klar abgegrenztes Verantwortlichkeitsprofil:

| Komponente | Kern-Invariante die getestet wird |
|---|---|
| `AdapterCapabilityAnnouncement` | `toJson()` / `fromJson()` Round-Trip + Callback-Dispatch |
| `LoRAFederationCoordinator` | FedAvg-Korrektheit + DP-Noise ist nicht null |
| `FederatedRAGMerger` | RRF-Rang-Ordnung + Dedup + top_k-Limit |
| `CrossShardFeedbackSync` | `shard_origin = "ANON"` erzwungen + Dedup-Cache |

Tests sind schnell (< 50 ms gesamt, kein Netz, kein FS-I/O).

## Technische Details

### CMakeLists.txt (neu)

```cmake
add_library(themis_distributed_knowledge STATIC
    adapter_capability_announcement.cpp
    lora_federation_coordinator.cpp
    federated_rag_merger.cpp
    cross_shard_feedback_sync.cpp
)
target_include_directories(themis_distributed_knowledge PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(themis_distributed_knowledge
    PUBLIC nlohmann_json::nlohmann_json themis_core
)
```

### Bekannter Fix (vor Tests)

`federated_rag_merger.cpp` — fehlendes Include ergänzen:
```cpp
#include <unordered_set>
```

### Teststruktur

```
tests/test_distributed_knowledge.cpp
├── DK-A-01…DK-A-06  AdapterCapabilityAnnouncement (6 Fälle)
├── DK-B-01…DK-B-13  LoRAFederationCoordinator (13 Fälle)
├── DK-C-01…DK-C-10  FederatedRAGMerger (10 Fälle)
└── DK-D-01…DK-D-08  CrossShardFeedbackSync (8 Fälle)
```

Vollständige Fallbeschreibungen: `src/distributed_knowledge/ROADMAP.md → Session 1`

## Abhängigkeiten

- **Vorbedingung:** keine (erster Issue des Epics)
- **Blockiert:** DK-2, DK-3, DK-4, DK-5 (alle benötigen kompilierendes Modul)

## Erfolgskriterien

- [ ] `src/distributed_knowledge/CMakeLists.txt` existiert und kompiliert fehlerfrei
- [ ] `add_subdirectory(src/distributed_knowledge)` im Root-CMakeLists eingetragen
- [ ] `tests/test_distributed_knowledge.cpp` mit ≥ 25 Testfällen vorhanden
- [ ] Alle 25+ Tests grün (`ctest -R test_distributed_knowledge`)
- [ ] Keine Compiler-Warnings unter `-Wall -Wextra`
- [ ] `#include <unordered_set>` in `federated_rag_merger.cpp` vorhanden
- [ ] DK-B-08: DP-Noise-Test schlägt fehl wenn σ=0 (statistischer Nachweis)
- [ ] DK-C-03: Dedup-Test: gleiches `doc_id` aus zwei Shards → erscheint nur einmal
- [ ] DK-D-02: `shard_origin` ist immer `"ANON"` auch wenn anderer Wert übergeben wird
- [ ] Keine Regressions in bestehendem Test-Suite (`ctest --rerun-failed`)

## Definition of Done

Modul kompiliert, alle Unit-Tests grün, CI-Pipeline sauber. Der nächste
Entwickler kann DK-2 ohne Build-Fehler starten.

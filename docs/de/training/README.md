# Training-Modul

**Stand:** 6. April 2026
**Version:** 1.6.0
**Kategorie:** Training
**Validated:** 2026-04-06
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Training-Modul stellt Werkzeuge für den Aufbau und die Pflege domänenspezifischer
KI-Feinabstimmungsdatensätze und LoRA-Adapter innerhalb von ThemisDB bereit. Es ist
primär auf die juristische Fachsprache ausgerichtet (Deutsch als Hauptzielsprache) und
umfasst folgende Kernkomponenten:

- **LegalAutoLabeler** (`auto_labeler.cpp`) — automatische Extraktion strukturierter
  Trainingsproben aus juristischen Dokumenten mittels NLP-Modalitätserkennung
- **IncrementalLoRATrainer** (`incremental_lora_trainer.cpp`) — inkrementelles
  LoRA-Adapter-Training mit Checkpoint/Resume-Unterstützung
- **KnowledgeGraphEnricher** (`knowledge_graph_enricher.cpp`) — Anreicherung von
  Trainingsproben mit Wissensgraph-Kontext via AQL-Traversierung
- **LoRACheckpointManager** (`lora_checkpoint_manager.cpp`) — SHA-256-gesichertes
  Checkpoint-Management mit atomarer Rotation
- **ModalityParser** (`modality_parser.cpp`) — Multi-Modalitäts-Erkennung und -Extraktion
  (Fließtext, Tabellen, Zitate, OCR)
- **ProvenanceTracker** (`provenance_tracker.cpp`) — Herkunfts- und Abstammungsverfolgung
  für Trainingsproben
- **LoraDataSelection** (`lora_data_selection.cpp`) — Auswahl, Deduplizierung und
  Balancierung von Trainingsdaten
- **TrainingPipeline** (`training_pipeline.cpp`) — Ende-zu-Ende-Orchestrierung des
  Trainingsprozesses (ConfidenceCalibrator, ProvenanceTracker-Integration)

**Aktueller Status: 🟢 Production-Ready** — v1.6.0 vollständig implementiert. AdaLoRAAdapter, LoRAAdapterMerger und LoRA+ Dual-Optimizer integriert.

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/training/README.md`](../../../src/training/README.md) | Modulübersicht, APIs, Verwendungsbeispiele |
| [`src/training/ARCHITECTURE.md`](../../../src/training/ARCHITECTURE.md) | Systemarchitektur, Komponentendiagramm, Datenfluss |
| [`src/training/ROADMAP.md`](../../../src/training/ROADMAP.md) | Feature-Roadmap und Implementierungsphasen |
| [`src/training/FUTURE_ENHANCEMENTS.md`](../../../src/training/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen und Designbeschränkungen |
| [`include/training/README.md`](../../../include/training/README.md) | Öffentliche Header/APIs und Konfigurationsoptionen |

---

## Schnittstellenübersicht

| Komponente | Header | Source | Rolle |
|------------|--------|--------|-------|
| LegalAutoLabeler | `auto_labeler.h` | `auto_labeler.cpp` | NLP-Extraktions-Pipeline für juristische Dokumente |
| IncrementalLoRATrainer | `incremental_lora_trainer.h` | `incremental_lora_trainer.cpp` | LoRA-Lebenszyklus (Training, Deploy, Rollback); LoRA+ Dual-AdamOptimizer (B-Matrix: lr×λ, A-Matrix: lr) via `IncrementalTrainingConfig::lora_plus_lambda` |
| KnowledgeGraphEnricher | `knowledge_graph_enricher.h` | `knowledge_graph_enricher.cpp` | AQL-Graphtraversierung für Kontextanreicherung |
| LoRACheckpointManager | `lora_checkpoint_manager.h` | `lora_checkpoint_manager.cpp` | SHA-256-Checkpoint-Validierung und -Rotation |
| ModalityParser | `modality_parser.h` | `modality_parser.cpp` | Modalitätserkennung (Text, Tabelle, Zitat, OCR) |
| ProvenanceTracker | `provenance_tracker.h` | `provenance_tracker.cpp` | Proben-Herkunftsverfolgung |
| LoraDataSelection | `lora_data_selection.h` | `lora_data_selection.cpp` | Trainingsdaten-Auswahl und -Deduplizierung |
| TrainingPipeline | `training_pipeline.h` | `training_pipeline.cpp` | Ende-zu-Ende-Orchestrierung |
| AdaLoRAAdapter | `ada_lora_adapter.h` | `ada_lora_adapter.cpp` | Wichtigkeitsbasiertes Rank-Pruning für LoRA-Adapter (AdaLoRA) |
| LoRAAdapterMerger | `lora_adapter_merger.h` | `lora_adapter_merger.cpp` | Adapter-Zusammenführung (Linear, TIES, *All) mit Power-Iteration-SVD |

---

## Einstieg

```cpp
#include "training/auto_labeler.h"
#include "training/training_pipeline.h"

using namespace themis::training;

// Trainingsproben aus juristischen Dokumenten extrahieren
AutoLabelConfig config;
config.language_code    = "de";
config.min_confidence   = 0.6f;
config.flag_low_confidence = true;

LegalAutoLabeler labeler(config, "rocksdb://./data");
LabelingStats stats = labeler.labelAll(
    [](size_t done, size_t total, const std::string& msg) { /* Fortschritt */ }
);
```

Weitere Beispiele: [`src/training/README.md`](../../../src/training/README.md#usage-examples)

---

## Laufzeitverhalten, Fehlerfälle und Grenzen

- `LegalAutoLabeler` arbeitet DB-gestützt mit `QueryEngine*` oder offline/testweise ohne Engine.
- `IncrementalLoRATrainer` validiert Konfiguration strikt; ungültige Parameter führen zu `std::invalid_argument`.
- Checkpoint-Ladevorgänge nutzen Integritätsprüfung (SHA-256) via `LoRACheckpointManager`.
- Grenzen: Single-Node-Orchestrierung (optional Multi-GPU), Inferenz-Routing außerhalb des Moduls.

---

## Troubleshooting (Kurz)

- **Keine gelabelten Dokumente:** `source_collection`, AQL-Zugriff und `text`-Feld in Quelldokumenten prüfen.
- **Leere Similarity-Ergebnisse:** `VectorIndexManager` initialisieren und per `setVectorIndex(...)` verdrahten.
- **Checkpoint-Resume schlägt fehl:** Dateirechte im Checkpoint-Verzeichnis und Manifest/Checksumme prüfen.

---

## Bekannte Einschränkungen

- Verteiltes/Multi-GPU-Training noch nicht koordiniert (Single-Node)
- Adapter-Serving (Inferenz) wird durch das LLM-Integrationsmodul übernommen
- Höchste Modellqualität ist für juristische deutschsprachige Korpora validiert; andere Domänen/Sprachen benötigen eigene Evaluierung und Kalibrierung.

---

## Fehlende Implementierungen

Siehe [`MISSING_IMPLEMENTATIONS.md`](MISSING_IMPLEMENTATIONS.md) für den vollständigen
Reality-Check-Bericht.

---

## Installation

Das Modul wird als Teil von ThemisDB gebaut. Für Build-/Test-Details siehe:

- [`src/training/README.md`](../../../src/training/README.md)
- [`include/training/README.md`](../../../include/training/README.md)

---

## Navigation

- [Zurück zur Dokumentationsübersicht](../README.md)
- [Query-Modul](../query/README.md)
- [LLM-Modul](../llm/)
- [LoRA-Dokumentation](../lora/)

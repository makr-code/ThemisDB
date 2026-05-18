# ONNX-CLIP-Modul

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: index.md · PRIMARY_SOURCES.md · ../../../src/onnx_clip/README.md -->

**Stand:** 13. Mai 2026
**Version:** 1.0.0
**Kategorie:** Multimodales Embedding / CLIP
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Modul `onnx_clip` stellt einen `IImageAnalysisBackend` für deterministische
Bild- und Text-Embeddings bereit. Der aktuelle Build deckt Initialisierung,
Batch-Verarbeitung mit Obergrenze, Text-Embedding, Statistiken,
Health-Checks sowie optionale SHA-256-Prüfung von Modelldateien ab.

**Primäre Quelle:** [`src/onnx_clip/`](../../../src/onnx_clip/)

## Wichtige Entry-Points

| Einstiegspunkt | Zweck |
|---|---|
| [`src/onnx_clip/onnx_clip_plugin.h`](../../../src/onnx_clip/onnx_clip_plugin.h) | Deklariert `ONNXClipPlugin` und die öffentliche API |
| [`include/onnx_clip/README.md`](../../../include/onnx_clip/README.md) | Dokumentiert die öffentliche Oberfläche und erklärt die aktuelle Header-Lage |
| [`src/onnx_clip/onnx_clip_plugin.cpp`](../../../src/onnx_clip/onnx_clip_plugin.cpp) | Implementiert Embeddings, Tokenisierung, Statistik, Batch-Splitting und Integritätsprüfung |

## Öffentlich dokumentierte API

- Lifecycle: `initialize()`, `shutdown()`, `isReady()`
- Metadaten/Status: `getInfo()`, `getBackend()`, `healthCheck()`, `warmup()`, `getStatistics()`
- Kernfunktionen: `generateEmbedding()`, `generateEmbeddingBatch()`, `generateTextEmbedding()`
- Non-OpenSSL-Fallback: `setModelHashFn()` für injizierte Hash-Prüfung

## Konfigurationsoptionen

| Key | Standardwert | Verhalten |
|---|---|---|
| `model.name` | `clip-vit-base-patch32` | Modellname in Ergebnissen und Statistiken |
| `model.embedding_dim` | `512` | Ziel-Dimension; ungültige Werte werden auf `512` zurückgesetzt |
| `max_batch_size` | `16` auf CPU, sonst `64` | Begrenzung für Sub-Batches |
| `model.path` | leer | Optionaler Dateipfad für Integritätsprüfung |
| `model.expected_sha256` | leer | Aktiviert Hash-Prüfung bei gemeinsamer Nutzung mit `model.path` |

## Laufzeitverhalten, Fehlerfälle und Grenzen

- `BackendType::AUTO` fällt im aktuellen generischen Build deterministisch auf `CPU`.
- `generateEmbeddingBatch()` verarbeitet Eingaben in geordneten Sub-Batches statt in einem einzelnen nativen ONNX-Batch-Call.
- Leere Bilddaten führen zu `success=false` mit `"Image data is empty"`.
- Leerer Text führt zu `success=false` mit `"Text input is empty"`.
- Vor `initialize()` liefern Embedding-Aufrufe `"ONNXClipPlugin not initialized"`.
- `getStatistics()` enthält u. a. `ready`, `backend`, `model_name`, `max_batch_size`, Latenzen und die Zähler `clip_embeddings_total`, `clip_text_embeddings_total`, `clip_batch_embeddings_total`.

## Installation

Für In-Tree-Nutzung müssen `include/` und `src/` auf dem Include-Pfad liegen,
da der aktuelle Header in `src/onnx_clip/` lebt.

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Usage

```cpp
#include "onnx_clip/onnx_clip_plugin.h"
#include <nlohmann/json.hpp>

using namespace themis::plugins::image;

ONNXClipPlugin plugin;
nlohmann::json settings = {
    {"model", {{"embedding_dim", 768}}},
    {"max_batch_size", 4}
};

PluginConfig config(settings);
plugin.initialize(config, BackendType::CPU);

auto image = plugin.generateEmbedding(std::vector<uint8_t>{1, 2, 3, 4});
auto text = plugin.generateTextEmbedding("eine Katze auf einem Sofa");
auto stats = plugin.getStatistics();
```

## Troubleshooting

- **`AUTO` nutzt keine GPU**: aktueller Codepfad wählt absichtlich `CPU`; für andere Backends explizit `BackendType::CUDA`, `DIRECTML` oder `TENSORRT` setzen.
- **Batches sind langsamer als erwartet**: derzeitiges Verhalten ist Sub-Batch-Splitting; nativer Batch-Call ist Follow-up-Arbeit.
- **Hash-Check läuft lokal nicht**: `model.path` und `model.expected_sha256` setzen; ohne OpenSSL zusätzlich `setModelHashFn()` registrieren.

## Primäre Dokumentation

| Dokument | Beschreibung |
|---|---|
| [`src/onnx_clip/README.md`](../../../src/onnx_clip/README.md) | Modulübersicht mit API, Konfiguration, Fehlerfällen und Snippets |
| [`include/onnx_clip/README.md`](../../../include/onnx_clip/README.md) | Öffentliche Oberfläche / Header-Dokumentation |
| [`src/onnx_clip/ARCHITECTURE.md`](../../../src/onnx_clip/ARCHITECTURE.md) | Architektur und Ablauf |
| [`src/onnx_clip/ROADMAP.md`](../../../src/onnx_clip/ROADMAP.md) | Status und offene Arbeit |
| [`src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../../src/onnx_clip/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen |
| [`src/onnx_clip/SECURITY.md`](../../../src/onnx_clip/SECURITY.md) | Sicherheitsannahmen und Kontrollen |

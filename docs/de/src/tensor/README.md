# Tensor-Modul — Übersicht

**Stand:** 2026-05-13
**Version:** v1.0.0 (Phase 1 abgeschlossen)
**Status:** 🟡 Experimentell — Phase 1 vollständig, Phase 2 in Bearbeitung (Q4 2026)
**Kategorie:** 🧮 Tensorbasierte ANN-Indizierung

---

## 📑 Inhaltsverzeichnis

- [Zweck](#zweck)
- [Architektur](#architektur)
- [Hauptkomponenten](#hauptkomponenten)
- [Konfiguration](#konfiguration)
- [Schnellstart](#schnellstart)
- [Fehlerbehandlung und Grenzen](#fehlerbehandlung-und-grenzen)
- [Primärdokumentation](#primärdokumentation)
- [Sekundärdokumentation](#sekundärdokumentation)

---

## Zweck

Das Tensor-Modul implementiert **Tensor-Train (TT)- und Hierarchical-Tucker (HT)-basierte
approximierte Nächste-Nachbar-Suche (ANN)** für ThemisDB. Es ist ein vollwertiges, eigenständiges
Indexmodul parallel zu `src/index/` (HNSW, FAISS, ScaNN, DiskANN) und belegt eine eigene Nische:

| Modul         | Stärke                                        | Einsatz wenn                          |
|---------------|-----------------------------------------------|---------------------------------------|
| `src/index`   | Sub-ms-Anfragen, float32-Vektoren, n ≥ 1 M    | dim ≤ 4096, κ < 2×, Standard-ANN     |
| `src/tensor`  | Strukturierte Komprimierbarkeit, Zero-Copy GGML | dim > 4096, κ ≥ 2×, LLM / Wissenschaft |

**κ** bezeichnet die Komprimierbarkeit: κ = Anzahl dichte Parameter / Anzahl TT-Parameter.

Hauptziele:
- Komprimierte Vektorspeicherung im TT-Format (Oseledets 2011 TT-SVD)
- Anfragen im komprimierten Raum ohne Rekonstruktion (O(d·r²) pro Kandidat)
- Zero-Copy GGML-Brücke für llama.cpp/LLM-Inferenz (Phase 3)
- LoRA/PEFT-Adapter-Souveränität als TT-Graphen (Phase 3)
- Multi-modale Datenkonvertierung (Geodaten, tabellarisch, Bilder, Dokumente) als TT/HT (Phase 7)
- Adaptive Strukturabrundung (Hiss/TNSR) im Hintergrund (Phase 6)

---

## Architektur

```
ThemisDB Core Engine
        │
        ▼
TensorIndexManager (Routing: HNSW / HYBRID / TENSOR_TRAIN)
        │
   ┌────┴──────────────────────────────┐
   │                                   │
   ▼                                   ▼
FlatTensorIndex (Phase 1)       HnswTTBridge (Phase 2 HYBRID)
  Linearer Scan O(n·d·r²)         HNSW-Navigation (Skizze)
                                   + TT-Reranking O(C·d·r²)
        │
        ▼
TensorNetworkStorageEngine (RocksDB, Phase 2)
TensorTrainDecomposer (TT-SVD, Holtz-Rounding)

        │
        ▼ Ingestion-Pipeline
TensorIngestionBridge  →  κ-Gate  →  TensorTrainDecomposer
TensorCoreStorageBridge → ITensorStorageBackend (RocksDB, Phase 2)

        │
        ▼ Phase 3+
TensorMmapBridge    → Zero-Copy GGML-Injektion
AdapterRepository   → LoRA/PEFT als TT-Graphen
TensorFingerprintGraph → Adapter-Ähnlichkeit (Phase 4-Vorbereitung)

        │
        ▼ Phase 5+
IHierarchicalTuckerIndex / FlatHTIndex → HT-Format
HissStructuralSearchEngine / TNSRTask  → Adaptive Strukturoptimierung (Phase 6)
UTRConverter / HyperIndexBuilder       → Multi-modale Kodierung (Phase 7)
```

---

## Hauptkomponenten

### Phase 1 — Kern-TT-Index (✅ Abgeschlossen)

| Header | Beschreibung |
|--------|--------------|
| `include/tensor/tensor_index.h` | `ITensorIndex` — einheitliche Schnittstelle für alle TT-Backends |
| `include/tensor/tensor_index_manager.h` | `TensorIndexManager` — Lifecycle-Manager, Routing, Mandanten-Isolation |
| `include/tensor/tensor_ingestion_bridge.h` | `TensorIngestionBridge` — Ingestion-Pipeline-Bridge (κ-Gate + TT-SVD) |
| `include/tensor/tensor_core_bridge.h` | `TensorCoreStorageBridge` — TT-Core-Persistenz in `ITensorStorageBackend` |

### Phase 2 — HNSW-Hybrid-Bridge (🟡 Q4 2026)

| Header | Beschreibung |
|--------|--------------|
| `include/tensor/hnsw_tt_bridge.h` | `HnswTTBridge` — HNSW-Navigation über Skizzen + TT-Reranking |

### Phase 3 — Zero-Copy GGML & Adapter-Souveränität (🟡 Experimentell)

| Header | Beschreibung |
|--------|--------------|
| `include/tensor/tensor_mmap_bridge.h` | `TensorMmapBridge` — RAII mmap-gepinnte TT-Cores für GGML |
| `include/tensor/adapter_repository.h` | `AdapterRepository` — LoRA/PEFT-Adapter als TT-Graphen |
| `include/tensor/tensor_butterfly_operator.h` | `TensorButterflyOperator` — Oszillatorische Integraloperatoren |

### Phase 4–8 — Erweiterte Komponenten (🟡 Geplant)

| Header | Beschreibung | Ziel |
|--------|--------------|------|
| `include/tensor/tensor_fingerprint_graph.h` | Adapter-Ähnlichkeitsgraph (fingerprint-basiert) | Q3 2027 |
| `include/tensor/ht_train.h` | Hierarchical-Tucker-Typen und Kontraktions-Engine | Q1 2028 |
| `include/tensor/ht_index.h` | HT-Index-Schnittstelle und FlatHTIndex | Q1 2028 |
| `include/tensor/hiss_structural_search.h` | Hiss/TNSR-Topologiesuche | Q2 2028 |
| `include/tensor/tnsr_task.h` | Hintergrundaufgabe für strukturelle Abrundung | Q3 2028 |
| `include/tensor/utr_converter.h` | Multi-modale Datenkonvertierung (UTR) | Q3–Q4 2028 |
| `include/tensor/hyper_index_builder.h` | Tabellarischer Kookkurrenz-TT-Index | Q4 2028 |

---

## Konfiguration

### Routing-Schwellenwerte (TensorRouter)

| κ (Komprimierbarkeit) | dim | Entscheidung |
|-----------------------|-----|--------------|
| κ ≥ 1.7 und dim ≥ 256 | beliebig | `TENSOR_TRAIN` |
| κ ≥ 1.3 | beliebig | `HYBRID` |
| κ < 1.3 | beliebig | `HNSW` → `src/index` verwenden |

### HnswTTConfig (Phase 2)

| Parameter | Standard | Beschreibung |
|-----------|---------|--------------|
| `M` | 16 | HNSW-Nachbarn pro Knoten |
| `ef_construction` | 200 | HNSW-Aufbauqualität |
| `ef_search` | 50 | HNSW-Kandidaten-Pool bei Suche |
| `sketch_dim` | 64 | Skizzen-Dimension des ersten TT-Cores |
| `rerank_candidates` | 200 | Kandidaten für TT-Reranking |
| `max_tt_rank` | 32 | Maximaler TT-Bond-Rang |
| `epsilon` | 0.01 | Relativer Rekonstruktionsfehler |
| `kappa_min` | 2.0 | Minimale Komprimierbarkeit für HYBRID |
| `kappa_max` | 6.0 | Max. Komprimierbarkeit (darüber → reines TT) |

### TensorIngestionBridge-Konfiguration

| Methode | Standard | Beschreibung |
|---------|---------|--------------|
| `setEpsilon(eps)` | 0.01 | Rekonstruktionsfehlertoleranz (TT-SVD) |
| `setMaxRank(rank)` | 0 (kein Limit) | Bond-Dimension-Cap |
| `setMinKappa(kappa)` | 1.3 | κ-Gate-Schwellenwert |

---

## Schnellstart

### Vektoren einfügen und suchen

```cpp
#include "tensor/tensor_index_manager.h"

auto mgr = themis::tensor::TensorIndexManager::create(db);

// Routing-Entscheidung prüfen
auto route = mgr->routeFor("mandant1", "llm_gewichte", "attention_k", 4096, 1'000'000);
// route == TENSOR_TRAIN (κ ≈ 4.5 für dim=4096)

// TT-Index anlegen und Vektor einfügen
auto* idx = mgr->createIndex("mandant1", "llm_gewichte", "attention_k");
if (!idx) { /* route war HNSW — src/index verwenden */ }
idx->addFlat(42, attention_vektor.data(), 4096);

// Suche im komprimierten Raum (ohne Rekonstruktion)
auto ergebnisse = idx->searchFlat(anfrage.data(), 4096, /*k=*/10);
```

### Persistenz konfigurieren

```cpp
mgr->setDataDir("/var/themisdb/tensor");
mgr->flushAll();  // Alle In-Memory-Indizes auf Disk schreiben
```

### Ingestion-Pipeline einbinden

```cpp
#include "tensor/tensor_ingestion_bridge.h"
#include "tensor/tensor_core_bridge.h"
#include "ingestion/builtin_step_factories.h"

// Produktions-Bootstrap: RocksDB-Backend injizieren
TensorCoreStorageBridge::setDefaultBackendFactory([&] {
    return std::make_shared<RocksDBTensorBackend>(db_handle);
});

auto decomp  = std::make_shared<TensorIngestionBridge>(0.01, 64, 1.3);
auto storage = std::make_shared<TensorCoreStorageBridge>();

auto step1 = ingestion::builtin::createChunkTtDecomposeStep(decomp);
auto step2 = ingestion::builtin::createTensorCoreBridgeStep(storage);
// Steps nach chunk_embed in der WorkflowEngine registrieren
```

---

## Fehlerbehandlung und Grenzen

| Situation | Rückgabewert |
|-----------|--------------|
| `createIndex()` wenn Route == HNSW | `nullptr` |
| `getIndex()` wenn nicht gefunden | `nullptr` |
| `mapCores()` wenn Vektor-ID fehlt | `nullptr` (unique_ptr) |
| `add()` mit doppelter ID | `false` |
| `addFlat()` mit `dim == 0` | `false` |
| `remove()` wenn ID nicht vorhanden | `false` |
| `save()` / `load()` (Phase-1-Stubs) | `false` |
| `makeKey()` mit leerem/Schrägstrich-Argument | `std::invalid_argument` |
| `HyperIndexBuilder::fromSchema()` mit < 2 Spalten | `std::invalid_argument` |
| `TensorButterflyOperator::build(RADON, …)` ohne Bridge | `std::logic_error` |
| `HTTrain` Copy-Konstruktor | gelöscht — `clone()` verwenden |

### Bekannte Grenzen (Phase 1)

- `FlatTensorIndex` ist O(n) pro Anfrage — für n > 100 000 empfiehlt sich Phase-2-`HnswTTBridge`.
- `save()` / `load()` sind Stubs; Persistenz über `setDataDir()` + `flushAll()`.
- `mlock()` kann in unprivilegierten Containerumgebungen (`RLIMIT_MEMLOCK == 0`) stillschweigend fehlschlagen; Datenpuffer bleiben gültig.
- RADON- und GREENS_FUNCTION-Operatoren sind nicht implementiert (Ziel: Phase 3 Q3 2027).

---

## Primärdokumentation

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| **Public-Header-API** | [`include/tensor/README.md`](../../../../include/tensor/README.md) | Vollständige API-Referenz aller 15 Header |
| **Implementierungsübersicht** | [`src/tensor/README.md`](../../../../src/tensor/README.md) | Dieses Dokument |
| **Architektur** | [`src/tensor/ARCHITECTURE.md`](../../../../src/tensor/ARCHITECTURE.md) | Komponentendiagramm, Datenfluss, SOC-Grenzen |
| **Roadmap** | [`src/tensor/ROADMAP.md`](../../../../src/tensor/ROADMAP.md) | Phasenplan, offene Aufgaben, Akzeptanzkriterien |
| **Future Enhancements** | [`src/tensor/FUTURE_ENHANCEMENTS.md`](../../../../src/tensor/FUTURE_ENHANCEMENTS.md) | Detaillierte Anforderungen je Enhancement |
| **Audit** | [`src/tensor/AUDIT.md`](../../../../src/tensor/AUDIT.md) | Stub-Inventar, Sicherheitshinweise |

---

## Installation

Header werden durch den regulären ThemisDB-Build bereitgestellt:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
target_link_libraries(your_target PRIVATE themis_tensor)
```

Einbindung einzelner Header:

```cpp
#include "tensor/tensor_index_manager.h"    // Primärer Einstiegspunkt
#include "tensor/tensor_ingestion_bridge.h" // Ingestion-Pipeline
#include "tensor/hnsw_tt_bridge.h"          // HYBRID-Modus
```

---

## Verwendung / Usage

Siehe [Schnellstart](#schnellstart) für häufige Verwendungsmuster.
Die vollständige Public-API-Referenz findet sich in
[`include/tensor/README.md`](../../../../include/tensor/README.md).

---

## Sekundärdokumentation

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| Grenzanalyse TT vs. HNSW | `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md` | κ-Schwellenwerte, empirische Daten |
| ArXiv-Entwurf (intern) | `research/TENSOR_NETWORK_DATABASE_ARXIV_DRAFT.md` | Vollständige wissenschaftliche Beschreibung |
| AdaLoRA-TT-Bridge | `research/ADALORA_TT_BRIDGE_RESEARCH.md` | Zero-Copy-Rationale, GGUF-v3-Metadaten |
| TT-Best-Practices | `research/best_practices/tensor_train_storage.md` | Speicher- und Zugriffsempfehlungen |
| Storage-Layer-API | `include/storage/README.md` | TensorTrainDecomposer, TensorNetworkStorageEngine |
| Ingestion-Layer-API | `include/ingestion/README.md` | ITensorDecompositionBackend, ITensorCoreBridge |

[docs](../../index.md) > [de](../index.md) > [process](./index.md) > [PROCESS_ANALYTICS_ROADMAP](./PROCESS_ANALYTICS_ROADMAP.md)  
**Datum:** 2026-03-20  
**Status:** draft  
**Modul:** `src/process/`  
**Version:** 1.0.0  

---

# Multidimensionale Organisations- und Prozessanalyse – Implementierungs-Roadmap für ThemisDB

> **English Abstract:**  
> This roadmap consolidates scientific and industry findings on multidimensional
> organisational and process analysis (OCEL 2.0, OCPM, LightRAG/HippoRAG, PPR,
> percolation resilience) and derives a concrete implementation roadmap for ThemisDB.
> It covers the rationale for migrating to an object-centric event log model (OCEL 2.0),
> compares retrieval strategies (Baseline RAG, GraphRAG, LightRAG, HippoRAG) with
> cost/latency/recall trade-offs, and provides a phased engineering plan with APIs,
> storage keys, and background-job hooks for Q2 2026 – Q1 2027.

---

## 📑 Inhaltsverzeichnis

1. [Executive Summary](#1-executive-summary)  
2. [Rationale: Vom modellzentrierten zum objektzentrierten Datenmodell (OCEL 2.0)](#2-rationale-vom-modellzentrierten-zum-objektzentrierten-datenmodell-ocel-20)  
3. [Vergleich der Retrieval-Strategien](#3-vergleich-der-retrieval-strategien)  
4. [Modulplan: APIs und Speicher-Schlüssel](#4-modulplan-apis-und-speicher-schlüssel)  
5. [Phasenbasierter Implementierungsplan](#5-phasenbasierter-implementierungsplan)  
6. [Risiko- und Bereitschaftsbewertung](#6-risiko--und-bereitschaftsbewertung)  
7. [Offene Forschungsfragen und Evaluationsmetriken](#7-offene-forschungsfragen-und-evaluationsmetriken)  
8. [Anhang: Bibliographie](#8-anhang-bibliographie)  

---

## 1. Executive Summary

### Kontext und Ziel

ThemisDB verwaltet Verwaltungsvorgänge als strukturierte Graphen (BPMN 2.0, EPK,
VCC-VPB) und bietet semantisches Retrieval über `ProcessGraphRag`. Die derzeitige
Architektur ist *modellzentriert*: Ereignisse werden an einzelnen Prozessinstanzen
fixiert, und die Ähnlichkeitssuche operiert auf flachen Embeddings.

Die wissenschaftliche Entwicklung der Jahre 2023–2025 zeigt drei konvergente Trends:

1. **Objektzentrierte Ereignismodellierung (OCEL 2.0):** Einzelne Ereignisse verknüpfen
   sich gleichzeitig mit mehreren Objekten (Antrag, Sachbearbeiter, Dokument,
   Organisationseinheit). Dies ermöglicht präzisere Konformitäts- und
   Bottleneck-Analyse. [→ Berti et al. 2023, [8](#8-anhang-bibliographie)]

2. **Graph-basiertes Retrieval mit PPR:** LightRAG ([Guo et al. 2024, [4](#8-anhang-bibliographie)])
   und HippoRAG ([Gutierrez et al. 2024, [5](#8-anhang-bibliographie)]) zeigen, dass
   eine duale Retrieval-Strategie (lokal: Entitätsebene; global: Community-Ebene)
   kombiniert mit Personalized PageRank (PPR) Reranking die Recall-Rate gegenüber
   einfachem BFS signifikant verbessert.

3. **Prädiktives Process Monitoring mit Transformer/GNN:** ProcessTransformer
   ([Bukhsh et al. 2021, [9](#8-anhang-bibliographie)]) und neuere GNN-Ansätze (2024)
   ermöglichen die Vorhersage des nächsten Aktivitäts- und Ressourcenzuordnung mit
   > 85 % Genauigkeit auf Standard-Benchmarks.

### Empfehlungen auf einen Blick

| Priorität | Maßnahme | Target | Modul |
|-----------|----------|--------|-------|
| P1 🔴 | OCEL 2.0 Import/Export | Q2 2026 | `ProcessModelManager` |
| P1 🔴 | Incremental LightRAG Indexer | Q2 2026 | `ProcessGraphRag` |
| P1 🔴 | Async Embedding-Hooks in `save()` | Q2 2026 | `ProcessModelManager` |
| P1 🔴 | Hybrid BM25 + HNSW Suche | Q2 2026 | `ProcessModelManager` |
| P2 🟡 | Leiden Community Detection Service | Q3 2026 | `ProcessGraphRag` |
| P2 🟡 | PPR Scoring Service (Multi-Hop) | Q3 2026 | `ProcessGraphRag` |
| P2 🟡 | Perkolations-/Resilienz-Simulator | Q3 2026 | Analytics |
| P3 🟢 | Predictive Monitoring (Transformer/GNN) | Q1 2027 | Analytics |
| P3 🟢 | SHAP/Attention Explainability | Q1 2027 | Analytics |

**Querverweis:**
- Wissenschaftliche Grundlagen: [`STATE_OF_THE_ART.md`](./STATE_OF_THE_ART.md)
- Mehrdimensionale Graphdimensionen: [`PROCESS_GRAPH_DIMENSIONS.md`](./PROCESS_GRAPH_DIMENSIONS.md)
- Primärquellen des Moduls: [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)
- Modul-Roadmap: [`src/process/ROADMAP.md`](../../../src/process/ROADMAP.md)

---

## 2. Rationale: Vom modellzentrierten zum objektzentrierten Datenmodell (OCEL 2.0)

### 2.1 Einschränkungen des aktuellen Modells

Das bestehende Datenmodell von ThemisDB speichert Ereignisse unter dem Schlüsselschema
`proc:inst:<id>:events:<seq>`. Jedes Ereignis referenziert genau *eine* Instanz
(`instance_id`). Für typische Verwaltungsvorgänge führt dies zu folgenden Problemen:

| Problem | Ursache | Auswirkung |
|---------|---------|-----------|
| Doppelzählung bei Ressourcenteilung | Sachbearbeiter bearbeitet N Vorgänge parallel | Bottleneck-Analyse überschätzt Wartezeiten |
| Verlorene O2O-Kanten | Verknüpfte Objekte (Antrag ↔ Bescheid) in separaten Sammlungen | Cross-Case-Abfragen erfordern Joins über mehrere Keys |
| Schwache Konformitätsauswertung | Nur ein Objekt pro Event beobachtbar | DSGVO-Datenpfad-Analyse unvollständig |

### 2.2 OCEL 2.0 Kerndatenmodell

Die OCEL 2.0-Spezifikation ([Berti et al. 2023, [8](#8-anhang-bibliographie)]) definiert:

```
Event
  ├── event_id        : string
  ├── activity        : string          # mapped to ProcessNodeInfo.name
  ├── timestamp       : ISO 8601
  ├── e2o[]           : [{object_id, qualifier}]   # event-to-object
  └── attributes      : map<string, any>

Object
  ├── object_id       : string
  ├── object_type     : string          # "case", "document", "person", "org_unit"
  ├── attributes      : map<string, any>
  └── o2o[]           : [{object_id, qualifier, timestamp}]  # object-to-object
```

Kernunterschied zu XES (eXtensible Event Stream): Ein Event kann über `e2o` mit *N*
Objekten verknüpft sein. Dies deckt Ressourcenteilung, parallele Bearbeitung und
Dokumentenflüsse nativ ab.

### 2.3 Mapping auf ThemisDB-Speicherstruktur

| OCEL 2.0 Konzept | ThemisDB Schlüssel | Datentyp |
|-----------------|-------------------|---------|
| Event | `proc:ocel:evt:<event_id>` | JSON |
| Object | `proc:ocel:obj:<object_id>` | JSON |
| E2O-Relation | `proc:ocel:e2o:<event_id>:<object_id>` | JSON `{qualifier}` |
| O2O-Relation | `proc:ocel:o2o:<obj_a>:<obj_b>` | JSON `{qualifier, timestamp}` |
| Embedding (Objekt) | `proc:ocel:emb:<object_id>` | float32[] |
| HNSW-Index (Instanz) | `proc:ocel:hnsw:<model_id>` | serialisierter HNSW |

**Migrationspfad:**  
Bestehende `proc:inst:<id>:events:<seq>`-Einträge können über einen
Konversionsskript nach OCEL 2.0 migriert werden:

```
proc:inst:<id>:events:<seq>  →  proc:ocel:evt:<event_id>
                                + proc:ocel:e2o:<event_id>:<id>  (qualifier="case")
```

Dabei wird die `instance_id` als OCEL-Objekt vom Typ `"case"` erzeugt. Alle anderen
Objekte (Dokumente, Personen, Organisationseinheiten) werden aus den vorhandenen
`proc:link:*`-Einträgen extrahiert.

### 2.4 Vorteile für ThemisDB

1. **Prozessübergreifende Bottleneck-Analyse:** Durch die O2O-Kanten lassen sich
   Ressourcenengpässe über Instanzgrenzen hinweg korrekt attribuieren.

2. **Präzisere DSGVO-Datenpfad-Analyse:** Jeder Ereignispfad durch personenbezogene
   Datenobjekte ist explizit nachvollziehbar.

3. **Konformität mit PM4Py und Celonis:** OCEL 2.0 wird von pm4py ≥ 2.4 und Celonis
   nativ importiert, was Interoperabilität mit dem deutschen Public-Sector-Ökosystem
   sicherstellt.

---

## 3. Vergleich der Retrieval-Strategien

### 3.1 Übersicht

| Strategie | Indexstruktur | Recall (Multi-Hop) | Latenz (p95) | Kosten | Empfehlung |
|-----------|--------------|-------------------|--------------|--------|-----------|
| **Baseline RAG** | Flat vector store | Niedrig (< 55 %) | < 30 ms | 💚 Niedrig | ❌ Unzureichend für Process-Mining |
| **GraphRAG (Microsoft)** | KG + Leiden Communities | Hoch (> 75 %) | 80–200 ms | 🟡 Mittel | ✅ Global queries |
| **LightRAG** | Dual (entity + community) | Sehr hoch (> 80 %) | 40–100 ms | 🟡 Mittel | ✅ **Empfohlen** |
| **HippoRAG + PPR** | KG + Personalized PageRank | Sehr hoch (> 82 %) | 20–80 ms | 🟡 Mittel | ✅ **Empfohlen für Multi-Hop** |
| **Hybrid BM25 + HNSW** | Inverted index + HNSW | Mittel (65–75 %) | < 50 ms | 💚 Niedrig | ✅ Als Fallback / Reranking |

*Recall-Werte basieren auf den jeweiligen Paper-Benchmarks; direkte Vergleiche auf dem
VCC-VPB-Datensatz fehlen noch (→ offene Forschungsfrage, [Abschnitt 7](#7-offene-forschungsfragen-und-evaluationsmetriken)).*

### 3.2 Baseline RAG

**Prinzip:** Texte werden in Chunks aufgeteilt, einzeln eingebettet und in einem Flat
Vector Store (HNSW, Faiss o. Ä.) abgelegt. Anfragen werden per Cosine-Similarity
beantwortet.

**Schwächen für Prozessanalyse:**
- Kein strukturelles Kontextwissen (Kanten, Vorgänger-/Nachfolgerbeziehungen)
- Recall bricht bei Multi-Hop-Anfragen (≥ 2 Zwischenschritte) drastisch ein
- Keine Community-Aggregation → globale Fragen ("Wie läuft das gesamte Genehmigungsverfahren ab?") schlecht beantwortet

### 3.3 Microsoft GraphRAG

**Prinzip:** Entitäten und Beziehungen werden mittels LLM aus Dokumenten extrahiert und
in einem Knowledge Graph gespeichert. Der Leiden-Algorithmus bildet hierarchische
Communities. Jede Community erhält einen vorberechneten LLM-Report.

**Zwei Retrieval-Modi:**
- *Local Search*: Entitätszentriert, traversiert direkten Nachbarn
- *Global Search*: Community-zentriert, aggregiert über Reports

**Stärken:** Hohe Recall-Rate bei konzeptuellen Fragen; gut für "Was sind alle
Genehmigungsschritte in diesem Bereich?"

**Schwächen:** Hohe Indexierungskosten (LLM-Aufrufe pro Entität); Latenz bei globalen
Anfragen durch Report-Aggregation.

**Referenz:** Edge et al. 2024, arXiv:2404.16130 [[3](#8-anhang-bibliographie)]

### 3.4 LightRAG – Inkrementelles duales Retrieval

**Prinzip:** LightRAG ([Guo et al. 2024 [4](#8-anhang-bibliographie)]) implementiert
ein *duales* Retrieval-System:

- **Low-level (lokal):** Sucht spezifische Entitäten und ihre direkten Relationen.
  Geeignet für Faktenfragen: "Wer ist zuständig für Schritt X?"
- **High-level (global):** Sucht über globale Themen/Konzepte.
  Geeignet für konzeptuelle Fragen: "Wie läuft das Genehmigungsverfahren ab?"

**Schlüsselunterschied zu GraphRAG:**  
Inkrementelle Indexierung ohne vollständigen Rebuild. Neue Dokumente/Prozessknoten
werden ohne Re-Indexierung des gesamten Graphen eingefügt.

**Empfehlung für ThemisDB:**  
Implementierung als `ProcessLightRetriever` mit konfigurierbarer Mode-Auswahl:

```cpp
enum class Mode { LOW, HIGH, AUTO };

LightResult retrieve(const std::string& query,
                     const std::string& instance_id,
                     Mode mode = Mode::AUTO) const;
```

`AUTO` klassifiziert die Anfrage durch lexikalische Schlüsselwörter (z. B. "übersicht",
"ablauf", "alle" → HIGH; spezifische Namen/IDs → LOW).

### 3.5 HippoRAG + PPR – Multi-Hop Reranking

**Prinzip:** HippoRAG ([Gutierrez et al. 2024, [5](#8-anhang-bibliographie)]) ist
neurobiologisch inspiriert durch den Hippocampus-Retrieval-Mechanismus. Kernidee:
Personalized PageRank (PPR) verteilt Relevanzscores ausgehend von Seed-Knoten über
den gesamten Graphen, gewichtet nach Graphstruktur.

**Verfahren:**

1. Query-Knoten identifizieren (Embedding-Matching oder BM25)
2. PPR-Iteration: `r = α · Aᵀ · r + (1 − α) · personalization`, `α = 0.85`
3. Top-K Knoten nach konvergiertem PPR-Score als Subgraph-Kontext

**Überlegenheit gegenüber BFS:**  
BFS gewichtet alle Knoten innerhalb des Radius gleich. PPR bevorzugt Knoten mit hoher
Erreichbarkeit aus den Seeds über mehrere Pfade – entspricht der menschlichen
Erinnerungsassoziation bei komplexen Zusammenhängen.

**Aktuelle ThemisDB-Situation:**  
`ProcessGraphRag::extractSubgraph()` nutzt aktuell BFS mit festem Tiefenlimit.
PPR-Ersatz ist in [`src/process/ROADMAP.md`](../../../src/process/ROADMAP.md) Phase 7 geplant.

**Referenz:** Gutierrez et al. 2024, arXiv:2405.14831 [[5](#8-anhang-bibliographie)]

### 3.6 Empfehlung: LightRAG-Stil + PPR Reranking

Die empfohlene Architektur für ThemisDB kombiniert:

```
Query
  │
  ├─ [LOW]  → Entity-Matching (BM25 + Embedding)
  │           → PPR-Scoring ausgehend von gematchten Entitäten
  │           → Top-K Knoten → Subgraph-Kontext
  │
  └─ [HIGH] → Community-Reports (Leiden-Cluster-Zusammenfassungen)
              → PPR-Scoring auf Community-Graph
              → Aggregierter Kontext
              │
              └─ [HYBRID] → BM25 + HNSW Reranking als Fallback
```

**Begründung:**
- Niedrige inkrementelle Indexierungskosten (kein LLM-Aufruf pro Entität nötig)
- PPR liefert konsistent hohe Multi-Hop-Recall-Raten ohne zusätzliche LLM-Kosten
- Hybrid BM25 + HNSW als performanter Fallback wenn PPR-Graph noch nicht aufgebaut

---

## 4. Modulplan: APIs und Speicher-Schlüssel

### 4.1 Überblick der neuen Komponenten

| Komponente | Datei | Funktion |
|-----------|-------|---------|
| `OcelImporter` | `src/process/ocel_importer.cpp` | OCEL 2.0 JSON/XML Import |
| `OcelExporter` | `src/process/ocel_exporter.cpp` | OCEL 2.0 JSON Export |
| `ProcessEmbeddingService` | `src/process/process_embedding_service.cpp` | Async Embedding-Queue |
| `ProcessEmbeddingJobQueue` | `src/process/process_embedding_jobs.cpp` | Thread-Pool für Embedding-Jobs |
| `ProcessLightRetriever` | `src/process/process_light_retriever.cpp` | Duales LightRAG-Retrieval |
| `PprScoring` | `src/process/ppr_scoring.cpp` | Personalized PageRank |
| `ProcessCommunityDetector` | `src/process/process_community_detector.cpp` | Leiden-Clustering |
| `PercolationSimulator` | `src/process/percolation_simulator.cpp` | Resilienzanalyse |
| `ProcessTransformerPredictor` | `src/process/process_transformer_predictor.cpp` | Prädiktives Monitoring |

### 4.2 Speicher-Schlüssel (RocksDB)

```
// OCEL 2.0 Ereignislog
proc:ocel:evt:<event_id>              → JSON (Event)
proc:ocel:obj:<object_id>             → JSON (Object)
proc:ocel:e2o:<event_id>:<object_id>  → JSON {qualifier}
proc:ocel:o2o:<obj_a>:<obj_b>         → JSON {qualifier, timestamp}

// Embeddings
proc:emb:model:<model_id>             → float32[] (Prozessmodell-Embedding)
proc:emb:inst:<instance_id>           → float32[] (Instanz-Embedding bei Terminal-State)
proc:ocel:emb:<object_id>             → float32[] (OCEL-Objekt-Embedding)

// Indizes
proc:idx:hnsw:<model_id>              → serialisierter HNSW-Graph
proc:idx:bm25:<model_id>              → serialisierter BM25-Index (IDF-Vektoren)
proc:idx:community:<model_id>         → JSON {cluster_id → [node_ids], report}

// PPR / Graph
proc:ppr:adj:<model_id>               → serialisierte Adjazenzliste
proc:ppr:scores:<model_id>:<query_hash> → float32[] (gecachte PPR-Scores, TTL 5 min)

// Prädiktives Monitoring
proc:pred:next:<instance_id>          → JSON {activity, probability, timestamp}
proc:pred:resource:<instance_id>      → JSON {resource_id, confidence}
```

### 4.3 APIs der neuen Komponenten

#### 4.3.1 OCEL 2.0 Import/Export

```cpp
// include/process/ocel_importer.h
class OcelImporter {
public:
    // Importiert OCEL 2.0 JSON-Datei und schreibt in RocksDB
    // Throws: OcelParseError bei ungültigem Format
    ImportResult importJson(const std::string& json_path, RocksDBWrapper& db);

    // Migrationspfad: konvertiert bestehende proc:inst:*:events:* Einträge
    MigrationResult migrateFromLegacy(const std::string& instance_id, RocksDBWrapper& db);
};

// include/process/ocel_exporter.h
class OcelExporter {
public:
    // Exportiert alle Events eines Modells als OCEL 2.0 JSON
    std::string exportJson(const std::string& model_id, RocksDBWrapper& db);

    // Exportiert als pm4py-kompatibles OCEL 2.0 XML
    std::string exportXml(const std::string& model_id, RocksDBWrapper& db);
};
```

#### 4.3.2 Async Embedding-Hooks in ProcessModelManager::save()

```cpp
// include/process/process_embedding_service.h
class ProcessEmbeddingService {
public:
    using EmbedFn = std::function<std::vector<float>(const std::string&)>;

    explicit ProcessEmbeddingService(EmbedFn fn, int thread_count = 2);

    // Nicht-blockierend; Embedding wird im Hintergrund berechnet und gespeichert
    std::future<void> embedModelAsync(const std::string& model_id,
                                       const std::string& text);
    std::future<void> embedInstanceAsync(const std::string& instance_id,
                                          const std::string& summary_text);
    void stop();  // wartet auf alle laufenden Jobs
};

// Erweiterung in ProcessModelManager (include/process/process_model_manager.h)
// (Setter-Injection, kein Breaking Change)
void setEmbeddingService(ProcessEmbeddingService* svc);

// In ProcessModelManager::save() nach erfolgreichem RocksDB-Put:
// if (embedding_service_) {
//     embedding_service_->embedModelAsync(record.id,
//         record.name + " " + record.description);
// }
```

#### 4.3.3 Hybrid BM25 + HNSW Suche

```cpp
// Erweiterung in ProcessModelManager::search()
// score = alpha * bm25_score + (1 - alpha) * cosine_score
std::vector<ProcessModelRecord> search(const std::string& query,
                                        double alpha = 0.5,
                                        int top_k = 20);
```

#### 4.3.4 PPR Scoring

```cpp
// include/process/ppr_scoring.h
namespace themis::process {

struct AdjEdge { std::size_t target; double weight; };

struct PprConfig {
    double damping{0.85};
    int    max_iterations{50};
    double convergence_epsilon{1e-6};
    int    top_k{20};
};

// Kernfunktion: liefert PPR-Scores für alle Knoten
std::vector<double> personalizedPageRank(
    const std::vector<std::vector<AdjEdge>>& adj,
    const std::vector<std::size_t>& seed_node_ids,
    double damping = 0.85,
    int max_iter = 50,
    double epsilon = 1e-6);

} // namespace themis::process
```

#### 4.3.5 ProcessLightRetriever

```cpp
// include/process/process_light_retriever.h
namespace themis::process {

struct LightResult {
    std::string              mode;       // "LOW", "HIGH"
    std::vector<std::string> node_ids;
};

class ProcessLightRetriever {
public:
    enum class Mode { LOW, HIGH, AUTO };

    using RetrieveFn = std::function<
        std::vector<std::string>(const std::string& query,
                                  const std::string& instance_id)>;

    void setLowLevelFn(RetrieveFn fn);
    void setHighLevelFn(RetrieveFn fn);

    LightResult retrieve(const std::string& query,
                         const std::string& instance_id,
                         Mode mode = Mode::AUTO) const;
private:
    // AUTO-Klassifizierung: HIGH bei globalem Vokabular ("übersicht", "ablauf",
    // "alle", "zusammenfassung", "wie"), sonst LOW
    static Mode classifyQuery(const std::string& query);
};

} // namespace themis::process
```

#### 4.3.6 Leiden Community Detection

```cpp
// include/process/process_community_detector.h
namespace themis::process {

struct CommunityResult {
    std::unordered_map<std::string, std::vector<std::string>> clusters; // cluster_id → node_ids
    std::unordered_map<std::string, std::string> community_reports;     // cluster_id → LLM-Report
};

class ProcessCommunityDetector {
public:
    // Führt Leiden-Clustering auf dem Knowledge-Graph des Modells durch
    // resolution: Auflösungsparameter (höher → kleinere Cluster)
    CommunityResult detect(const std::string& model_id,
                            RocksDBWrapper& db,
                            double resolution = 1.0);
};

} // namespace themis::process
```

#### 4.3.7 Perkolations-/Resilienz-Simulator

```cpp
// include/process/percolation_simulator.h
namespace themis::process {

struct PercolationResult {
    double giant_component_fraction;  // Anteil des größten Clusters
    double critical_threshold;        // geschätzter Perkolationsschwellwert
    std::vector<std::string> critical_nodes;  // Knoten mit höchstem Einfluss
};

class PercolationSimulator {
public:
    // Simuliert zufällige Knotenentfernung und misst Graphzusammenhang
    // trials: Anzahl Monte-Carlo-Durchläufe
    PercolationResult simulate(const std::string& model_id,
                                RocksDBWrapper& db,
                                int trials = 1000,
                                double removal_fraction = 0.1);
};

} // namespace themis::process
```

### 4.4 GPU/Offline Job-Hooks

Rechenintensive Aufgaben werden als asynchrone Background-Jobs ausgeführt:

| Job | Trigger | Parallelisierung |
|-----|---------|-----------------|
| Modell-Embedding | Nach `ProcessModelManager::save()` | `ProcessEmbeddingJobQueue` (Thread-Pool) |
| Instanz-Embedding | Nach Terminal-State in `ProcessGraphManager` | `ProcessEmbeddingJobQueue` |
| HNSW-Rebuild | Täglicher Cron oder manuelle API | Offline-Batch, GPU-optional |
| Leiden-Clustering | Nach Import > N neuer Modelle | Offline-Batch |
| PPR-Cache Invalidierung | Nach Graph-Mutation | Lazy, TTL-basiert |
| Predictive Model Inference | Neue Instanz-Events | GPU-Job-Queue (CUDA optional) |

---

## 5. Phasenbasierter Implementierungsplan

> Dieser Plan ist mit der [`src/process/ROADMAP.md`](../../../src/process/ROADMAP.md) abgestimmt.
> Checkboxen werden dort synchron aktualisiert.

### Phase Q2 2026 – Datenbasis & inkrementeller Index

#### Ziele
- OCEL 2.0 als natives Speicherformat einführen
- Embedding-Pipeline automatisieren (non-blocking)
- Hybride Suche (BM25 + HNSW) produktionsbereit

#### Aufgaben

- [ ] **OCEL 2.0 Import:** `OcelImporter::importJson()` implementieren
  - Betroffene Subsysteme: `src/process/ocel_importer.cpp`, `include/process/ocel_importer.h`, `RocksDBWrapper`
  - Inputs: OCEL 2.0 JSON (Spec 2023/2025); max. Dateigröße 500 MB
  - Outputs: `proc:ocel:evt:*`, `proc:ocel:obj:*`, `proc:ocel:e2o:*`, `proc:ocel:o2o:*` Keys
  - Fehlerbehandlung: `OcelParseError` bei fehlendem `ocelVersion`-Feld, ungültigem ISO-8601-Timestamp oder doppelter `event_id`; ungültige E2O-Referenz → Warnung + Skip
  - Perf: Import von 100.000 Events in ≤ 10 s (single thread)
  - Tests: Import-Round-Trip mit pm4py-Beispieldatei; Event-Count == Erwartungswert; E2O-Kardinalität korrekt; Fehlerfall ungültiger Timestamp wirft `OcelParseError`

- [ ] **OCEL 2.0 Export:** `OcelExporter::exportJson()` und `exportXml()`
  - Kompatibilität: pm4py ≥ 2.4, Celonis OCEL-Konnektor
  - Tests: Export → pm4py-Import ohne Datenverlust

- [ ] **Legacy-Migration:** `OcelImporter::migrateFromLegacy()`
  - Konvertiert `proc:inst:<id>:events:<seq>` → OCEL 2.0 Keys
  - Idempotent (mehrfacher Aufruf sicher)
  - Tests: Migrationsskript auf den 17 vorinstallierten VCC-VPB-Modellen

- [ ] **Async Embedding-Hooks:** `ProcessEmbeddingService` + `ProcessEmbeddingJobQueue`
  - Betroffene Subsysteme: `src/process/process_embedding_service.cpp`, `src/process/process_embedding_jobs.cpp`, `ProcessModelManager::save()`, `ProcessGraphManager` (post-state-change hook)
  - Fehlerbehandlung: LLM-Endpoint-Timeout (> 5 s) → Job wird verworfen + Warnung geloggt; RocksDB-Write-Fehler → Future-Exception; Thread-Pool-Überlastung (Queue > 1000) → Ältester Job wird verworfen (FIFO-Drop)
  - Laufzeitverhalten: `save()` kehrt sofort zurück (non-blocking); `stop()` wartet max. 10 s auf alle laufenden Jobs und danach Forced-Shutdown
  - Perf: Embedding-Queue Durchsatz ≥ 50 Jobs/s bei 2 Worker-Threads; End-to-End (Job-Submit bis Storage-Write) ≤ 5 s unter Normallast
  - Tests: 100 Jobs → alle Futures erfüllt; Ergebnis korrekt in RocksDB; `stop()` terminiert Worker sauber; Queue-Drop bei Überlastung (> 1000 Jobs) verifiziert

- [ ] **Hybrid BM25 + HNSW Suche:** Erweiterung von `ProcessModelManager::search()`
  - `alpha`-Parameter für Score-Fusion (default 0.5)
  - BM25: TF-IDF über Node-Namen, Descriptions, Compliance-Tags
  - HNSW: Cosine-Similarity auf Modell-Embeddings
  - Perf: < 50 ms für 10.000 Modelle
  - Tests: Recall ≥ 0.85 auf 50-Query-Benchmark über VCC-VPB-Bibliothek

- [ ] **Inkrementeller LightRAG-Indexer:** `ProcessLightRetriever` Grundimplementierung
  - Betroffene Subsysteme: `src/process/process_light_retriever.cpp`, `include/process/process_light_retriever.h`
  - LOW-Mode: Entity-Matching (BM25 + Embedding-Nearest-Neighbour); gibt `std::vector<std::string>` der matched Node-IDs zurück
  - HIGH-Mode: Aggregation über Community-Reports aus `proc:idx:community:<model_id>`; für Q2 2026 mit Platzhalter-Report implementieren (leere Report-Map → HIGH-Mode gibt Top-5 BFS-Knoten zurück, bis Leiden in Q3 verfügbar); kein Stub ohne Rückgabewert
  - Fehlerbehandlung: Leere Query → `InvalidQueryError`; unbekannte `instance_id` → leere Ergebnisliste
  - Tests: AUTO-Klassifizierung: "fehlende dokumente" → LOW, "prozess übersicht" → HIGH; LOW-Mode gibt korrekte Node-IDs; HIGH-Mode gibt Platzhalter-Kontext zurück (nicht leer)

#### Akzeptanzkriterien

- OCEL 2.0 JSON Export besteht pm4py-Validierung
- `save()` kehrt sofort zurück; Embedding erscheint innerhalb 5 s im Storage
- `search()` p95-Latenz < 50 ms auf Entwicklungs-Hardware

---

### Phase Q3 2026 – Community Detection, PPR & Resilienz

#### Ziele
- Leiden-Clustering als Community-Backend aktivieren
- PPR-Scoring ersetzt BFS in `ProcessGraphRag::extractSubgraph()`
- Perkolations-Resilienzanalyse für kritische Infrastrukturprozesse

#### Aufgaben

- [ ] **Leiden Community Detection Service:** `ProcessCommunityDetector::detect()`
  - Betroffene Subsysteme: `src/process/process_community_detector.cpp`, `include/process/process_community_detector.h`, `ProcessGraphRag`
  - Inputs: `model_id` + `resolution` (float, Wertebereich 0.1–10.0; außerhalb → `InvalidResolutionError`); Graph aus `proc:ppr:adj:<model_id>` oder `KnowledgeGraph`-Struktur
  - Laufzeitverhalten: Leiden-Iteration bis Modularity-Delta < 1e-4 oder max. 100 Iterationen; Ergebnis idempotent für gleichen Eingabegraphen (deterministischer Seed)
  - Community-Reports: optionaler LLM-Aufruf offline-batchbar; ohne LLM-Integration → Report = komma-separierte Top-5-Node-Namen der Community
  - Fehlerbehandlung: Leerer Graph → leeres `CommunityResult`; Isolierte Knoten → eigene Einzel-Community; Verbindungsfehler zum LLM → Report-Feld leer, kein Fehler
  - Speicherung: `proc:idx:community:<model_id>` (JSON, atomic RocksDB-Put)
  - Perf: ≤ 500 ms für Graph mit 500 Knoten und 2000 Kanten
  - Tests: synthetischer Graph mit 3 bekannten Communities (Modularity ≥ 0.3); Einzel-Knoten-Graph (kein Absturz); Idempotenz-Test (gleicher Input → gleicher Output)

- [ ] **PPR Scoring Service:** `personalizedPageRank()` + Integration in `ProcessGraphRag`
  - Ersetzt `extractSubgraph()` BFS (BFS bleibt als Fallback für Baumgraphen)
  - `PprConfig`: damping=0.85, max_iter=50, epsilon=1e-6, top_k=20
  - Perf: ≤ 20 ms für 500-Knoten-Graph
  - Tests:
    - 4-Knoten-Graph mit handberechneten PPR-Werten, Toleranz 1e-5
    - 3-Hop-Anfrage auf Bauantrag-Modell: Zielknoten in Top-3
    - BFS-Regression: linearer Graph → PPR degeneriert zu BFS

- [ ] **PPR-Score-Caching:** `proc:ppr:scores:<model_id>:<query_hash>` mit TTL 5 min
  - Invalidierung bei Graph-Mutation
  - Tests: Cache-Hit senkt Latenz auf < 5 ms

- [ ] **Perkolations-/Resilienz-Simulator:** `PercolationSimulator::simulate()`
  - Monte-Carlo Knotenentfernung; misst Giant-Component-Fraction
  - Identifiziert kritische Knoten (höchster Einfluss auf Zusammenhang)
  - Referenz: Perkolationsstudien 2023–2025 [[12](#8-anhang-bibliographie),[13](#8-anhang-bibliographie),[14](#8-anhang-bibliographie)]
  - Tests: synthetisches Netzwerk mit bekanntem kritischem Schwellwert

- [ ] **HIGH-Mode LightRAG mit Leiden-Reports**
  - Integration Community-Reports in `ProcessLightRetriever::retrieve()` HIGH-Mode
  - Tests: konzeptuelle Anfrage liefert Community-Report-Kontext

#### Akzeptanzkriterien

- PPR liefert nachweislich bessere 3-Hop-Recall als BFS (Benchmark auf VCC-VPB)
- Perkolations-Simulator identifiziert Schlüsselknoten korrekt auf synthetischen Graphen
- Community-Reports für alle 17 VCC-VPB-Modelle verfügbar

---

### Phase Q1 2027 – Prädiktives Monitoring & Explainability

#### Ziele
- Transformer/GNN-basierte Vorhersage des nächsten Aktivitäts- und Ressourcenzuordnung
- SHAP/Attention-basierte Erklärbarkeit für Compliance-Audits

#### Aufgaben

- [ ] **ProcessTransformer Vorhersagemodell:**
  - Architektur: Transformer über OCEL 2.0 Ereignissequenz (analog ProcessTransformer, Bukhsh 2021)
  - Inputs: Bisherige Events einer Instanz (Aktivität, Timestamp, Ressource, Objekte)
  - Outputs: Nächste Aktivität + Wahrscheinlichkeit; erwartete Abschlusszeit; Ressourcenzuordnung
  - Speicherung: `proc:pred:next:<instance_id>`, `proc:pred:resource:<instance_id>`
  - Perf: Inferenz < 100 ms auf CPU; GPU-Batch-Modus für > 1000 aktive Instanzen
  - Tests: Genauigkeit ≥ 85 % auf BPIC 2020 Benchmark-Datensatz

- [ ] **GNN-basiertes Strukturmodell (Forschungsphase Q1 2027):**
  - Scope: Graph Neural Network (GNN) über O2O-Relationen für strukturelle Next-Activity-Vorhersage bei unvollständigen Ereignissequenzen (< 3 beobachtete Events)
  - Architektur: 2-Layer-GraphSAGE über O2O-Nachbargraph; Input: Node-Embeddings aus `proc:ocel:emb:*`; Output: Wahrscheinlichkeitsverteilung über Aktivitätsklassen
  - Betroffene Subsysteme: `src/process/process_transformer_predictor.cpp` (neuer GNN-Branch), `RocksDBWrapper` (O2O-Graph-Scan)
  - Fehlerbehandlung: Kein O2O-Graph vorhanden → Fallback auf Transformer-Vorhersage; Inferenz-Timeout (> 200 ms CPU) → Fallback
  - Perf: Inferenz ≤ 200 ms auf CPU für Graph mit ≤ 500 Knoten
  - Tests: Genauigkeit ≥ 70 % auf Instanzen mit < 3 Events (BPIC 2020 Subset); Fallback-Pfad bei fehlendem Graph wird ausgelöst

- [ ] **SHAP-Explainability:**
  - SHAP-Werte für Transformer-Vorhersage: welche bisherigen Events beeinflussen
    Vorhersage am stärksten?
  - Integration in Compliance-Check-Report
  - Tests: SHAP-Werte summieren korrekt zur Vorhersage (Additivity-Test)

- [ ] **Attention-Visualisierung:**
  - Export von Attention-Gewichten als JSON
  - Ermöglicht manuelle Revision durch Sachbearbeiter

#### Akzeptanzkriterien

- Vorhersage-API verfügbar unter `ProcessTransformerPredictor::predictNext(instance_id)`
- SHAP-Report wird in `ComplianceCheckResult` integriert
- Genauigkeit ≥ 85 % auf BPIC 2020; Latenz < 100 ms (CPU)

---

## 6. Risiko- und Bereitschaftsbewertung

### 6.1 Risikomatrix

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|--------|-------------------|------------|-----------|
| OCEL 2.0 Spec-Änderungen (2025 Draft) | Mittel | Mittel | Schema-Version im Key `proc:ocel:version` versionieren; Migrations-Shim |
| Embedding-Dimensionsmismatch | Hoch | Hoch | Strict Dimension Check in `ProcessEmbeddingService::embedModelAsync()`; `EmbeddingDimensionError` |
| PPR-Konvergenz bei dünn besetzten Graphen | Niedrig | Mittel | Fallback auf BFS wenn |V| < 10 oder Baum |
| LLM-Community-Report-Kosten | Hoch | Mittel | Offline-Batch-Generierung; Reports cachen unter `proc:idx:community:*` |
| Leiden-Algorithmus-Lizenz | Niedrig | Hoch | leidenalg (GPLv3) → kommerziell ggf. problematisch; Alternative: Louvain (MIT) |
| Transformer-Overfitting auf VCC-VPB | Mittel | Mittel | Cross-Validation; externe Benchmarks (BPIC 2020, BPIC 2019) |
| RocksDB Key-Space-Explosion durch OCEL | Niedrig | Mittel | Column Family für `proc:ocel:*`; Kompression aktivieren |

### 6.2 Bereitschaftsbewertung

| Komponente | Bereitschaft | Notizen |
|-----------|-------------|---------|
| `ProcessModelManager` CRUD | ✅ Produktionsreif | Basis für alle Erweiterungen vorhanden |
| `ProcessGraphRag` BFS-Retrieval | ✅ Beta | Wird durch PPR ersetzt |
| `ProcessLinker` | ✅ Beta | Stabil; wird für O2O-Migration genutzt |
| `RocksDBWrapper` | ✅ Produktionsreif | Neues Column Family für OCEL |
| HNSW-Index | 🟡 Vorhanden | Noch nicht für Prozessmodelle aktiviert |
| Embedding-Infrastruktur | 🟡 Schema vorhanden | Auto-Generierung fehlt |
| Leiden/Community | ❌ Nicht vorhanden | Neuimplementierung erforderlich |
| PPR-Scoring | ❌ Nicht vorhanden | Neuimplementierung erforderlich |
| OCEL 2.0 I/O | ❌ Nicht vorhanden | Neuimplementierung erforderlich |
| Prädiktives Monitoring | ❌ Nicht vorhanden | Forschungsprototyp erforderlich |

### 6.3 Migrationskonsiderationen

**Bestandsdaten:**  
Alle bestehenden `proc:inst:*:events:*`-Einträge können durch `OcelImporter::migrateFromLegacy()`
non-destruktiv in OCEL 2.0 überführt werden. Die Legacy-Keys bleiben bis zu einer
definierten Sunset-Version (vorgeschlagen: v2.0.0) erhalten.

**API-Kompatibilität:**  
`ProcessGraphRag::retrieve()`, `ProcessModelManager::save()` und `ProcessLinker::attachObject()`
behalten ihre bestehenden Signaturen. OCEL und PPR werden über optionale Parameter oder
Setter-Injection eingebracht – *kein Breaking Change*.

**Rollout-Strategie:**
1. Feature-Flag `THEMIS_ENABLE_OCEL` (default: off) für Early-Adopter-Testing
2. Parallelbetrieb Legacy + OCEL bis alle Modelle migriert
3. Feature-Flag `THEMIS_ENABLE_PPR` für PPR-Retrieval
4. Sunset Legacy-Keys nach Migrationsprüfung

---

## 7. Offene Forschungsfragen und Evaluationsmetriken

### 7.1 Offene Forschungsfragen

| # | Frage | Relevanz | Nächster Schritt |
|---|-------|---------|-----------------|
| F1 | Wie verhält sich PPR-Recall vs BFS auf dem VCC-VPB-Datensatz? | Hoch | Benchmark auf 50-Query-Set |
| F2 | Welche Community-Granularität (Leiden-Resolution) ist optimal für Verwaltungsprozesse? | Hoch | Grid-Search mit manuell gelabelten Communities |
| F3 | Ist LightRAG-LOW oder LightRAG-HIGH besser für Konformitätsanfragen? | Mittel | A/B-Test mit Sachbearbeiter-Feedback |
| F4 | Wie wirkt sich OCEL 2.0 auf die BFS-Tiefe und PPR-Scores aus? | Mittel | Synthese-Experiment mit O2O-Kanten |
| F5 | Generalisiert ProcessTransformer auf unbekannte Prozesstypen? | Hoch | Zero-Shot-Transfer auf BPIC 2019 → BPIC 2020 |
| F6 | Welche Perkolationsschwelle ist für kritische Verwaltungsprozesse akzeptabel? | Mittel | Stakeholder-Workshop mit BSI-Richtlinien |

### 7.2 Evaluationsmetriken

| Metrik | Zielwert | Messmethode |
|--------|---------|-------------|
| Multi-Hop Recall@5 (PPR) | ≥ 82 % | 50-Query-Benchmark, VCC-VPB |
| Multi-Hop Recall@5 (BFS) | Baseline | Identischer Benchmark |
| Search p95-Latenz (BM25 + HNSW) | < 50 ms | Lasttest 10.000 Modelle |
| Embedding-Queue Throughput | ≥ 50 Jobs/s | 1000-Job-Stresstest |
| PPR-Konvergenz (500 Knoten) | ≤ 20 ms | Unit-Benchmark |
| Next-Activity Accuracy | ≥ 85 % | BPIC 2020 Cross-Validation |
| OCEL Export Roundtrip | 100 % Datentreue | pm4py Import-Validierung |
| Community Detection F1 | ≥ 0.80 | Manuell gelabelte Gold-Partition |

---

## 8. Anhang: Bibliographie

> Alle Quellen werden paraphrasiert und zusammengefasst. Es wird kein schutzbedürftiger
> Originaltext übernommen. DOIs und arXiv-IDs dienen zur eindeutigen Identifikation.

### Prozess-Mining und Ereignismodellierung

**[1] van der Aalst, W.M.P.** (2016). *Process Mining: Data Science in Action.* 2. Auflage.
Springer. — Standardwerk zu Process Mining; definiert Konformitätsprüfung, Bottleneck-
und Leistungsanalyse auf Basis von Ereignislogs.

**[2] van der Aalst, W.M.P.** (2022). *Object-Centric Process Mining: Dealing with
Divergence and Convergence in Event Data.* IEEE Access, 10.
— Einführung des objektzentrierten Process-Mining-Paradigmas; zeigt Nachteile
klassischer XES-basierter Ansätze bei N:M-Objekt-Beziehungen.

**[8] Berti, A., Park, G., Sani, M.F., van der Aalst, W.M.P.** (2023). *OCEL 2.0
Specification.* Process Mining Group, RWTH Aachen. doi:10.5281/zenodo.8428111.  
— Formale Spezifikation von OCEL 2.0 (Object-Centric Event Log); definiert Event,
Object, E2O und O2O Relationen; Referenz für ThemisDB-Import/-Export.

### Graph-RAG und Retrieval-Systeme

**[3] Edge, D., Trinh, H., Cheng, N. et al.** (2024). *From Local to Global: A Graph RAG
Approach to Query-Focused Summarization.* Microsoft Research. arXiv:2404.16130.  
— Einführung von GraphRAG mit zweistufigem Local/Global Retrieval; Leiden-Community-
Detection; Community-Reports. Basis für Leiden-Integration in ThemisDB.

**[4] Guo, Z., Xia, L., Yu, Y. et al.** (2024). *LightRAG: Simple and Fast Retrieval-
Augmented Generation.* arXiv:2410.05779.  
— Duales inkrementelles Retrieval-System (Entity-Level + Community-Level). Geringere
Indexierungskosten als GraphRAG durch inkrementellen Update. Empfohlen für ThemisDB
als `ProcessLightRetriever`.

**[5] Gutierrez, B.J., Shu, Y., Laban, P. et al.** (2024). *HippoRAG: Neurobiologically
Inspired Long-Term Memory for Large Language Models.* NeurIPS 2024. arXiv:2405.14831.  
— Personalized PageRank (PPR) als Multi-Hop-Retrieval-Mechanismus; inspiriert durch
hippokampale Erinnerungskonsolidierung. Empfohlen für ThemisDB als PPR-Scoring-Service.

### Prädiktives Process Monitoring

**[9] Bukhsh, Z.A., Saeed, A., Dijkman, R.M.** (2021). *ProcessTransformer: Predictive
Business Process Monitoring with Transformer Network.* arXiv:2104.00721.  
— Transformer-Architektur für Next-Activity- und Completion-Time-Vorhersage auf BPIC-
Datensätzen; ≥ 85 % Genauigkeit auf BPIC 2020. Referenz für `ProcessTransformerPredictor`.

**[10] Rama-Maneiro, E., Vidal, J.C., Lama, M.** (2023). *Deep Learning for Predictive
Business Process Monitoring: Review and Benchmark.* IEEE Transactions on Services
Computing, 16(1).  
— Umfassender Vergleich von LSTM, GRU, Transformer und GNN-Ansätzen für prädiktives
Process Monitoring. GNN übertrifft Transformer bei strukturellen Vorhersagen.

**[11] Neu, D.A., Lahann, J., Fettke, P.** (2022). *A Systematic Literature Review on
State-of-the-Art Deep Learning Methods for Process Prediction.* Artif. Intell. Rev.  
— Meta-Studie zu Deep-Learning-Ansätzen; identifiziert Datenmangel als Haupthindernis
für Verwaltungsanwendungen.

### Netzwerk-Resilienz und Perkolation

**[12] Boccaletti, S. et al.** (2023). *Network Resilience Under Targeted Attack.*
Physical Review Letters, 131.  
— Analytische Perkolationstheorie für gerichtete Netzwerke; definiert kritischen
Schwellwert $p_c$ für Zusammenbruch des größten Clusters.

**[13] Gao, J., Bashan, A., Shekhtman, L., Havlin, S.** (2022). *Introduction to Networks
of Networks.* IOP Publishing. doi:10.1088/978-0-7503-1046-8. — Multilayer-Netzwerk-Resilienz;
relevant für ThemisDB-Szenarien mit vernetzten Verwaltungsprozessen.

**[14] Schneider, C.M., Moreira, A.A., Andrade, J.S., Havlin, S., Herrmann, H.J.** (2011,
aktualisiert 2025). *Mitigation of Malicious Attacks on Networks.* PNAS.  
— Methoden zur Identifikation kritischer Knoten ("Superhighways") in Prozessnetzwerken;
Grundlage für `PercolationSimulator::simulate()`.

### Prozessmodellierung und Standards

**[15] OMG** (2013). *Business Process Model and Notation (BPMN) 2.0.* ISO/IEC 19510:2013.  
— Normativer Standard für BPMN 2.0; Grundlage für `BpmnSerializer` in ThemisDB.

**[16] OMG** (2023). *Decision Model and Notation (DMN) 1.5.*  
— Entscheidungstabellen und FEEL-Ausdrücke; geplant für `ProcessModelRecord`-Erweiterung.

**[17] Traag, V.A., Waltman, L., van Eck, N.J.** (2019). *From Louvain to Leiden: guaranteeing
well-connected communities.* Scientific Reports, 9, 5233.  
— Leiden-Algorithmus für Community Detection; Verbesserung gegenüber Louvain.
Referenz für `ProcessCommunityDetector`.

**[18] Page, L., Brin, S., Motwani, R., Winograd, T.** (1999). *The PageRank Citation
Ranking: Bringing Order to the Web.* Stanford InfoLab Technical Report.  
— Grundlage für Personalized PageRank (PPR); `personalizedPageRank()`-Implementierung
basiert auf Power-Iteration-Variante.

---

*Dieses Dokument basiert auf dem Stand der Forschung bis März 2026. Für die primären
Modul-Quellen siehe [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md). Der wissenschaftliche
Hintergrund ist ausführlicher in [`STATE_OF_THE_ART.md`](./STATE_OF_THE_ART.md)
dokumentiert.*

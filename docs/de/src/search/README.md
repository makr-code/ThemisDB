# Search-Modul — Übersicht

**Stand:** 2026-05-13
**Version:** v2.4.0
**Status:** ✅ Production Ready
**Kategorie:** 🔍 Suche

---

## 📑 Inhaltsverzeichnis

- [Zweck](#zweck)
- [Architektur](#architektur)
- [Hauptkomponenten](#hauptkomponenten)
- [Konfiguration](#konfiguration)
- [Schnellstart](#schnellstart)
- [Primärdokumentation](#primärdokumentation)
- [Sekundärdokumentation](#sekundärdokumentation)

---

## Zweck

Das Search-Modul implementiert **Volltext- und hybride semantische Suche** für ThemisDB. Es verbindet BM25-Keyword-Suche mit HNSW-Vektorsuche über Reciprocal Rank Fusion (RRF) und bietet LLM-gestützte Query-Umschreibung, LLM-Re-Ranking, facettierte Suche, Fuzzy-Matching, Autovervollständigung, personalisiertes Ranking, Konversationssuche, föderierte Suche und Streaming-Ergebnislieferung.

Hauptziele:
- Hybride BM25+Vektor-Suche mit ≥ 85 % Recall@10
- LLM-Augmentierung für verbesserten Recall bei unterspecifizierten Abfragen
- Facettierte Suche mit dynamischer Bucket-Aggregation
- Mehrsprachige Suche über multilinguale Embedding-Modelle
- Personalisiertes Re-Ranking auf Basis von Nutzerinteraktionshistorie
- NOT-Operator-Filterung mit negativen Keywords
- Multi-Turn-Konversationssuche mit kontextbasierter Query-Reformulierung (v2.4.0)
- Tenant-isolierte föderierte Suche mit per-Tenant-Gewichtung (v2.4.0)
- Cursor-basiertes Streaming für große Ergebnismengen (v2.4.0)

---

## Architektur

```
Search API  (src/server/)
        │
        ▼
ConversationalSearch / FederatedSearch (v2.4.0 — optionale Wrapper)
        │
        ▼
HybridSearch (Hauptfassade)
  ┌────────────────────────────────────────────┐
  │  LlmQueryRewriter (optional, 200 ms Timeout)│
  └────────────────────────────────────────────┘
        │
  ┌─────┴──────────────────────────────────┐
  │  BM25Search          VectorSearch       │
  │  (InvertedIndex)     (HNSW)             │
  └─────┬──────────────────────────────────┘
        │
  QueryExpander → FuzzyMatcher → NeuralSparseRetrieval
        │
  Reciprocal Rank Fusion (RRF) → merged result list
        │
  FacetedSearch → MultiFieldSearch → NegativeKeywordFilter
        │
  PersonalizedRanker → LearningToRank → LlmReranker (optional)
        │
  SearchHighlighter → SearchAnalytics → SearchResultStream → return results
```

| Komponente | Funktion |
|---|---|
| HybridSearch | Zentraler Orchestrator: BM25 + Vektor + RRF-Fusion |
| BM25Search | Invertierter Index mit BM25-Scoring (k1=1.2, b=0.75) |
| VectorSearch | HNSW-ANN-Suche (Cosine, Dot Product, L2) |
| RRF | Reciprocal Rank Fusion (O(k log k), keine Score-Normalisierung) |
| LlmQueryRewriter | LLM-basierte Query-Umschreibung mit Fallback |
| LlmReranker | LLM-basiertes Re-Ranking der Top-N Ergebnisse |
| ConversationalSearch | Multi-Turn-Kontext-Reformulierung (v2.4.0) |
| FederatedSearch | Tenant-isolierte parallele Suche mit RRF-Fusion (v2.4.0) |
| SearchResultStream | Cursor-basiertes Streaming-Paging (v2.4.0) |

---

## Hauptkomponenten

| Datei | Rolle |
|-------|-------|
| `hybrid_search.h/cpp` | Hauptfassade: RRF-basierte Fusion von BM25 und Vektorsuche |
| `query_expander.h/cpp` | Synonym-Expansion, Rechtschreibkorrektur, Zero-Result-Relaxation |
| `fuzzy_matcher.h/cpp` | Levenshtein, Soundex, Metaphone, N-Gramm (Dice) Ähnlichkeit |
| `faceted_search.h/cpp` | Per-Feld-Wert-Facetten, numerische Range-Buckets, Drill-Down |
| `search_analytics.h/cpp` | Query-Log; avg/p95/p99 Latenz, Zero-Result-Rate, Top-20 Queries |
| `autocomplete.h/cpp` | Prefix-Index + Popular-Query-Suggestions, score-gerankt |
| `learning_to_rank.h/cpp` | Linearer Re-Ranker über 6D-Feature-Vektor; Online-Pairwise-Training |
| `multi_modal_search.h/cpp` | TEXT/IMAGE/AUDIO/CUSTOM Modalitäten; gewichtete RRF-Fusion |
| `llm_query_rewriter.h/cpp` | LLM-basierte Query-Umschreibung für verbesserten Recall |
| `llm_reranker.h/cpp` | LLM-basiertes Re-Ranking der Top-N Ergebnisse |
| `personalized_ranker.h/cpp` | Per-User-Interaktionshistorie mit zeitgedämpftem Scoring |
| `multi_field_search.h/cpp` | Per-Feld-Boosting (Titel > Body > Tags), konfigurierbare Gewichte |
| `neural_sparse_retrieval.h/cpp` | SPLADE-basierte Sparse-Neural-Retrieval mit Vokabular-Expansion |
| `cross_lingual_search.h/cpp` | Mehrsprachige Suche via multilinguale Embedding-Modelle |
| `search_highlighter.h/cpp` | Highlight- und Snippet-Generierung für gefundene Terme |
| `negative_keyword_filter.h/cpp` | NOT-Operator-Filterung negativer Keywords |
| `conversational_search.h/cpp` | Multi-Turn-kontextbewusste Query-Reformulierung mit History-Eviction |
| `federated_search.h/cpp` | Tenant-isolierte parallele Suche mit per-Tenant-Gewichtung und RRF-Fusion |
| `search_result_stream.h/cpp` | Cursor-basiertes Streaming-Paging für große Ergebnismengen |

---

## Konfiguration

```cpp
// HybridSearch konfigurieren (Felder aus HybridSearch::Config)
HybridSearch::Config config;

// RRF-Fusion-Gewichte
config.bm25_weight   = 0.5;    // Gewicht BM25-Ergebnisse (>= 0)
config.vector_weight = 0.5;    // Gewicht Vektor-Ergebnisse (>= 0)
config.use_rrf       = true;   // Reciprocal Rank Fusion verwenden (empfohlen)
config.rrf_k         = 60.0;   // RRF-Konstante

// Ergebniszählen
config.k        = 10;   // Finale Ergebnis-Anzahl
config.k_bm25   = 50;   // BM25-Kandidaten
config.k_vector = 50;   // Vektor-Kandidaten

// Performance-Limits (Sicherheitsschranken)
config.max_k          = 10000; // Obere Schranke für k
config.max_candidates = 10000; // Obere Schranke für k_bm25 / k_vector

// Index-Zuordnung
config.default_table  = "documents";
config.default_column = "content";

// Vektordistanzmetrik
config.vector_metric = VectorIndexManager::Metric::COSINE; // COSINE / DOT / L2

// Konstruktor wirft std::invalid_argument bei ungültiger Config
HybridSearch engine(fulltext_index, vector_index, config);
```

> **Hinweis BM25-Parameter**: k1 (= 1.2) und b (= 0.75) werden im BM25-Scorer
> (`src/index/inverted_index.cpp`) konfiguriert, nicht in `HybridSearch::Config`.

---

## Schnellstart

```cpp
#include "search/hybrid_search.h"
#include "search/personalized_ranker.h"
#include "search/faceted_search.h"

using namespace themis;

// 1. Hybride Suche durchführen
std::string text_query = "database performance tuning";
std::vector<float> query_vec = embed(text_query); // eigene Embedding-Funktion

HybridSearch::SearchStats stats;
auto results = engine.search(text_query, query_vec.data(), query_vec.size(), &stats);

if (stats.partial_result) {
    // Ein Backend schlug fehl; Ergebnisse sind degradiert
}

// 2. Personalisiertes Re-Ranking
PersonalizedRanker ranker;
ranker.recordInteraction(user_id, doc_id, InteractionType::CLICK);
auto reranked = ranker.rerank(user_id, results);

// 3. Facetten berechnen
FacetedSearch facets(&secondary_index);
auto facet_result = facets.computeFacet("documents", "category");
// facet_result.value_counts — Map<Wert, Anzahl Dokumente>
```

---

## Primärdokumentation

Die maßgebliche Entwicklerdokumentation befindet sich in `src/search/` und `include/search/`:

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| **README** | [`src/search/README.md`](../../../../src/search/README.md) | Modulübersicht, Komponenten, Konfigurationsreferenz |
| **Architektur** | [`src/search/ARCHITECTURE.md`](../../../../src/search/ARCHITECTURE.md) | Komponentendiagramm, Datenflüsse, Threading, Sicherheit |
| **Roadmap** | [`src/search/ROADMAP.md`](../../../../src/search/ROADMAP.md) | Implementierungsstatus, bekannte Einschränkungen |
| **Erweiterungen** | [`src/search/FUTURE_ENHANCEMENTS.md`](../../../../src/search/FUTURE_ENHANCEMENTS.md) | Geplante Features, Interface-Designs, wissenschaftliche Referenzen |
| **Header-README** | [`include/search/README.md`](../../../../include/search/README.md) | Übersicht der öffentlichen Header-Schnittstellen |

---

## Sekundärdokumentation

Erklärende Dokumentation und Reality-Check in `docs/de/`:

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| **Fehlende Implementierungen** | [`docs/de/src/search/MISSING_IMPLEMENTATIONS.md`](MISSING_IMPLEMENTATIONS.md) | Reality-Check: offene Punkte (nur Distributed Search, Issue #2280) |

---

## Installation

Das Search-Modul wird als Teil von ThemisDB gebaut. Ausführliche Anweisungen befinden sich in der primären Dokumentation:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Siehe [`src/search/README.md`](../../../../src/search/README.md) und das Root-`CMakeLists.txt` für vollständige Build-Konfiguration.

---

## Usage

Einstiegsbeispiel sowie vollständige API-Referenz sind in der primären Entwicklerdokumentation zu finden:

- **Schnellstart / Usage-Snippets:** Abschnitt [Schnellstart](#schnellstart) oben und [`include/search/README.md`](../../../../include/search/README.md)
- **Konfigurationsreferenz:** Abschnitt [Konfiguration](#konfiguration) oben und `HybridSearch::Config` in [`include/search/README.md`](../../../../include/search/README.md)
- **Troubleshooting:** [`include/search/README.md` — Troubleshooting](../../../../include/search/README.md)

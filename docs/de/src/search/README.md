# Search-Modul — Übersicht

**Stand:** 2026-03-10
**Version:** v2.2.0
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

Das Search-Modul implementiert **Volltext- und hybride semantische Suche** für ThemisDB. Es verbindet BM25-Keyword-Suche mit HNSW-Vektorsuche über Reciprocal Rank Fusion (RRF) und bietet LLM-gestützte Query-Umschreibung, LLM-Re-Ranking, facettierte Suche, Fuzzy-Matching, Autovervollständigung und personalisiertes Ranking.

Hauptziele:
- Hybride BM25+Vektor-Suche mit ≥ 85 % Recall@10
- LLM-Augmentierung für verbesserten Recall bei unterspecifizierten Abfragen
- Facettierte Suche mit dynamischer Bucket-Aggregation
- Mehrsprachige Suche über multilinguale Embedding-Modelle
- Personalisiertes Re-Ranking auf Basis von Nutzerinteraktionshistorie
- NOT-Operator-Filterung mit negativen Keywords

---

## Architektur

```
Search API  (src/server/)
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
  SearchHighlighter → SearchAnalytics → return results
```

| Komponente | Funktion |
|---|---|
| HybridSearch | Zentraler Orchestrator: BM25 + Vektor + RRF-Fusion |
| BM25Search | Invertierter Index mit BM25-Scoring (k1=1.5, b=0.75) |
| VectorSearch | HNSW-ANN-Suche (Cosine, Dot Product, L2) |
| RRF | Reciprocal Rank Fusion (O(k log k), keine Score-Normalisierung) |
| LlmQueryRewriter | LLM-basierte Query-Umschreibung mit Fallback |
| LlmReranker | LLM-basiertes Re-Ranking der Top-N Ergebnisse |

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

---

## Konfiguration

```cpp
HybridSearch::Config config;

// BM25-Parameter
config.bm25_k1 = 1.5;     // Term-Frequency-Sättigung
config.bm25_b  = 0.75;    // Längen-Normalisierung

// RRF-Fusion
config.rrf_k         = 60;  // RRF-Konstante
config.bm25_weight   = 0.5; // Gewicht BM25 in Fusion

// LLM-Augmentierung (optional)
config.llm_rewriter_enabled  = false; // Query-Umschreibung
config.llm_reranker_enabled  = false; // Re-Ranking
config.llm_timeout_ms        = 200;   // Timeout mit Fallback

// Performance-Limits
config.max_k          = 1000;  // Maximale Ergebnis-Anzahl
config.max_candidates = 10000; // Maximale HNSW-Kandidaten

HybridSearch engine(config);
```

---

## Schnellstart

```cpp
// Hybride Suche durchführen
HybridSearch::Request req;
req.query   = "database performance tuning";
req.top_k   = 10;
req.mode    = HybridSearch::Mode::HYBRID;
req.tenant  = "my-tenant";

auto result = engine.search(req);
// result.hits      — sortierte Treffer-Liste
// result.facets    — Facetten-Buckets
// result.total     — Gesamtanzahl Treffer
// result.highlights — Hervorhebungen pro Dokument

// Personalisiertes Re-Ranking
PersonalizedRanker ranker;
ranker.recordInteraction(user_id, doc_id, InteractionType::CLICK);
auto ranked = ranker.rerank(user_id, result.hits);

// Facetten-Drill-Down
FacetedSearch facets;
facets.addFilter("category", "database");
facets.addRangeFilter("year", 2020, 2026);
auto filtered = facets.apply(result.hits);
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
| **Fehlende Implementierungen** | [`docs/de/src/search/missing-implementations.md`](missing-implementations.md) | Reality-Check: offene Punkte (nur Distributed Search, Issue #2280) |

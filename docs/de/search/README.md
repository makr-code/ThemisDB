# Search Module – ThemisDB

**Stand:** 9. März 2026
**Version:** v1.8.0
**Kategorie:** Core Search
**Validated:** 2026-03-09
**Status:** current

---

## Übersicht

Das Search-Modul implementiert den vollständigen Suchstack von ThemisDB: Hybrid-Search (BM25 + Vektor-RRF), Fuzzy-Matching, Query-Expansion, Faceted-Search, Autocomplete, Multi-Modal-Search, Neural-Sparse-Retrieval, Cross-Lingual-Search, Personalisiertes Ranking und LLM-basiertes Re-Ranking.

**Primäre Dokumentation:** [`src/search/README.md`](../../src/search/README.md)  
**Roadmap:** [`src/search/ROADMAP.md`](../../src/search/ROADMAP.md)  
**Geplante Erweiterungen:** [`src/search/FUTURE_ENHANCEMENTS.md`](../../src/search/FUTURE_ENHANCEMENTS.md)  
**Fehlende Implementierungen:** [`missing-implementations.md`](missing-implementations.md)

---

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| HybridSearch | `hybrid_search.h` | `hybrid_search.cpp` | RRF-basierte Fusion von BM25 und Vektor-Suche |
| FuzzyMatcher | `fuzzy_matcher.h` | `fuzzy_matcher.cpp` | Levenshtein, Soundex, Metaphone, N-Gramm-Ähnlichkeit |
| QueryExpander | `query_expander.h` | `query_expander.cpp` | Synonym-Expansion, Rechtschreibkorrektur, Query-Relaxation |
| FacetedSearch | `faceted_search.h` | `faceted_search.cpp` | Wert-Facetten, Bereichs-Buckets, Drill-Down-Filter |
| SearchAnalytics | `search_analytics.h` | `search_analytics.cpp` | Thread-sicheres Query-Log, p95/p99-Latenz, Zero-Result-Rate |
| AutocompleteEngine | `autocomplete.h` | `autocomplete.cpp` | Präfix-Index + populäre Query-Vorschläge |
| LearningToRank | `learning_to_rank.h` | `learning_to_rank.cpp` | Linearer Re-Ranker, paarweises Gradient-Descent-Training |
| MultiModalSearch | `multi_modal_search.h` | `multi_modal_search.cpp` | TEXT/IMAGE/AUDIO/CUSTOM-Modalitäten RRF-Fusion |
| MultiFieldBoostedSearch | `multi_field_search.h` | `multi_field_search.cpp` | Multi-Feld-Boosting (Titel > Body > Tags) |
| NeuralSparseRetrieval | `neural_sparse_retrieval.h` | `neural_sparse_retrieval.cpp` | SPLADE/BERT-basiertes Neural-Sparse-Retrieval |
| CrossLingualSearch | `cross_lingual_search.h` | `cross_lingual_search.cpp` | Sprachübergreifende Suche via mehrsprachige Embeddings |
| PersonalizedRanker | `personalized_ranker.h` | `personalized_ranker.cpp` | Nutzer-spezifisches zeitgewichtetes Interaktions-Ranking |
| LlmQueryRewriter | `llm_query_rewriter.h` | `llm_query_rewriter.cpp` | LLM-basierte alternative Query-Generierung |
| LlmReranker | `llm_reranker.h` | `llm_reranker.cpp` | Batch-LLM-Scoring + LTR-Bridge für Closed-Loop-Training |

**Gesamt:** 14 Header, 14 Source-Dateien in `src/search/` / `include/search/`

---

## Search-Methoden

### 1. Full-Text Search (BM25)

**Best für:** Exakte Keyword-Matches, Boolean Queries, Phrase Search

- ✅ BM25 Ranking (k1, b konfigurierbar)
- ✅ Term-Frequency- und Document-Frequency-Analyse
- ✅ Feldlängen-Normalisierung
- ✅ Phrase Search (via SecondaryIndexManager)
- ✅ Fuzzy Matching (FuzzyMatcher)
- ✅ Stemming und Stop-Word-Filterung (utils-Modul)

### 2. Vector Search (HNSW)

**Best für:** Semantic Search, Ähnlichkeitssuche

- ✅ HNSW-Indexierung (Cosine, Dot Product, L2)
- ✅ k-NN-Suche
- ✅ Bis zu 10M+ Vektoren

### 3. Hybrid Search (RRF)

**Best für:** Best of Both Worlds – Keywords + Semantik

- ✅ Reciprocal Rank Fusion (RRF) mit konfigurierbaren Gewichten
- ✅ Score-Normalisierung
- ✅ Partial-Result-Erkennung (`SearchStats.partial_result`)
- ✅ Text-only, Vector-only, Hybrid-Modi
- ✅ Optionaler LLM-Re-Ranker (`LlmReranker`)

---

## Performance (Referenzwerte)

| Methode | Avg-Latenz | Durchsatz | Recall@10 |
|---------|-----------|-----------|-----------|
| Full-Text (BM25) | 1–10 ms | 1K–10K QPS | — |
| Vector (HNSW) | 1–10 ms | 1K–5K QPS | — |
| Hybrid (RRF) | 5–20 ms | 500–2K QPS | 85%+ |

---

## Bekannte Offene Punkte

| Item | ROADMAP-Status | Issue |
|------|---------------|-------|
| Highlight / Snippet-Generierung | `[I]` In Progress | #2457 |
| Negative Keyword Filtering (`NOT`) | `[I]` Geplant | #2003 |
| Distributed Search über Shards | `[I]` Geplant | #2280 |
| `QueryParser` (search-modul-spezifisch) | `[?]` Unklar | — |
| `ResultRanker` (search-modul-spezifisch) | `[?]` Unklar | — |

---

## Verwandte Dokumentation

- [Full-Text Search Guide](FULLTEXT_SEARCH_GUIDE.md)
- [Vector Search Guide](VECTOR_SEARCH_GUIDE.md)
- [Hybrid Search Guide](HYBRID_SEARCH_GUIDE.md)
- [Search Feature Matrix](SEARCH_FEATURE_MATRIX.md)
- [Performance Tuning](performance_tuning.md)
- [Migration Guide](migration_guide.md)
- [Fehlende Implementierungen](missing-implementations.md)

---

*Reviewed by: Copilot agent (2026-03-09)*  
*Nächste Prüfung: v1.9.0 Milestone*


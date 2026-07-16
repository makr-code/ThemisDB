# Hybrid Search – Implementierung (✅ Produktionsreif)

**Stand:** 6. April 2026  
**Version:** 2.3.0  
**Kategorie:** Search  
**Status:** ✅ Produktionsreif (seit v1.2.0; vollständig implementiert)

---

> **Hinweis:** Dieses Dokument beschreibt das ursprüngliche Design (Phase 4, Dez 2025).
> Die Implementierung ist vollständig abgeschlossen und produktionsreif.  
> Aktuelle technische Details: `include/search/hybrid_search.h`, `src/search/hybrid_search.cpp`
> (v0.0.35, PRODUCTION-READY, 0 TODOs, 0 Stubs).  
> Vollständiger Modul-Status: `include/search/ROADMAP.md` (v2.3.0).

---

## ✅ Implementierungsstatus (März 2026)

| Komponente | Status | Implementierung |
|------------|--------|-----------------|
| **BM25 Fulltext Search** | ✅ Produktionsreif | `src/search/hybrid_search.cpp` |
| **Vector ANN Search (HNSW)** | ✅ Produktionsreif | `include/index/vector_index.h` |
| **Reciprocal Rank Fusion (RRF)** | ✅ Produktionsreif | `src/search/hybrid_search.cpp` |
| **Score-Normalisierung** | ✅ Produktionsreif | `src/search/hybrid_search.cpp` |
| **LLM Re-Ranking (Phase 3)** | ✅ Produktionsreif | `include/search/llm_reranker.h` |
| **Distributed Hybrid Search (mTLS)** | ✅ Produktionsreif | `include/search/distributed_hybrid_search.h` |
| **NegativeKeywordFilter** | ✅ Produktionsreif | `include/search/negative_keyword_filter.h` |
| **SearchHighlighter** | ✅ Produktionsreif | `include/search/search_highlighter.h` |
| **MultiModalSearch** | ✅ Produktionsreif | `include/search/multi_modal_search.h` |
| **CrossLingualSearch** | ✅ Produktionsreif | `include/search/cross_lingual_search.h` |

**Implementierte API:** `POST /search/hybrid` (vollständig, inkl. Pagination und Filter)  
**Benchmarks:** `benchmarks/benchmark_hybrid_search.cpp`, `benchmarks/benchmark_distributed_hybrid_search.cpp`  
**Tests:** `tests/test_hybrid_search.cpp`, `tests/test_hybrid_search_integration.cpp`, `tests/test_distributed_hybrid_search.cpp`, `tests/test_http_hybrid_search.cpp`

---

## Design-Beschreibung (ursprünglich Phase 4, jetzt implementiert)

Kombiniert Vektorähnlichkeit (Chunks) mit BM25-Fulltext und optionalen Filtern, um robuste Ergebnisse über Content-Chunks zu liefern.

### Ziele ✅ Alle erreicht
- Semantische Suche (Vector Top-K) + BM25-Fulltext-Suche
- Score-Fusion via Reciprocal Rank Fusion (RRF) oder linearer Kombination
- Filterbarkeit (category, mime_type, metadata-*), Pagination
- Konfigurierbare Distanzmetrik (COSINE, DOT, L2)
- LLM-basiertes Re-Ranking via `setReranker()`
- Distribuierte Suche mit mTLS über mehrere Knoten

### Ablauf ✅ Implementiert
1) Query-Embedding (konfigurierbare Dimension)
2) BM25 Fulltext-Suche (Inverted Index, `SecondaryIndexManager`)
3) Vector Top-K ANN-Suche (HNSW, `VectorIndexManager`)
4) Score-Fusion (RRF Standard; Linear-Kombination Fallback mit Normalisierung)
5) Optional: LLM Re-Ranking via `LlmReranker`
6) Filter anwenden (serverseitig)
7) Sortierung + Pagination (limit/offset)

### Implementierte API
`POST /search/hybrid`
```json
{
  "query": "text or vector",
  "embedding": [..optional..],
  "k": 20,
  "filters": {"category": ["TEXT","IMAGE","GEO"], "mime_type": ["image/jpeg"], "metadata": {"dataset": "LSG"}},
  "scoring": {"alpha": 1.0, "beta": 0.2, "gamma": 0.1}
}
```
Response: Liste von Ergebnissen mit Score, ID und Metadaten

### Konfiguration (`HybridSearch::Config`)
```cpp
HybridSearch::Config cfg;
cfg.k = 20;
cfg.bm25_weight = 0.5;
cfg.vector_weight = 0.5;
cfg.fusion_method = FusionMethod::RRF;   // oder LINEAR_COMBINATION
cfg.vector_metric = VectorMetric::COSINE;
cfg.max_k = 1000;
cfg.max_candidates = 10000;
```

### Edge Cases ✅ Implementiert und getestet
- Leeres/kurzes Query → Fallback auf Fulltext-only
- Heterogene Dimensionen → getrennte Index-Namespaces
- Fehlende Index-Backends → graceful degradation (noexcept search())
- Ressourcengrenzen via `max_k` / `max_candidates`

### Tests ✅ Vollständig
- Top-K stabil, Fusion deterministisch bei fixierten Parametern
- Filter wirksam (before/after Fusion), Paginierung korrekt
- Distributed Hybrid Search getestet über 16-Shard-Cluster
- Integration-Tests mit echten BM25 und Vector-Indizes

### Siehe auch
- `include/search/ROADMAP.md` – vollständiger Modul-Roadmap (v2.3.0)
- `include/search/hybrid_search.h` – Public API
- `src/search/hybrid_search.cpp` – Implementierung
- `docs/de/apis/apis_hybrid_search.md` – HTTP API-Dokumentation

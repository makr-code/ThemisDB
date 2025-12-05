# Search Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Search

---

## Übersicht

ThemisDB bietet verschiedene Such-Modi für unterschiedliche Anwendungsfälle.

## Such-Modi

| Modus | Index | Beschreibung |
|-------|-------|--------------|
| **Fulltext** | Inverted Index | BM25 Ranking, Stemming |
| **Vector** | HNSW | Semantic Search, Embeddings |
| **Hybrid** | Fulltext + Vector | Reciprocal Rank Fusion |
| **Graph** | Adjacency List | Path Queries, Traversals |
| **Geo** | R-Tree, S2/H3 | Spatial Queries |

## Source-Code Referenz

| Komponente | Header | Source |
|------------|--------|--------|
| FulltextIndex | `include/index/fulltext_index.h` | `src/index/fulltext_index.cpp` |
| VectorIndex | `include/index/vector_index.h` | `src/index/vector_index.cpp` |
| GraphIndex | `include/index/graph_index.h` | `src/index/graph_index.cpp` |

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [content_search_summary.md](content_search_summary.md) | Content Search Overview |
| [fulltext_api.md](fulltext_api.md) | Fulltext Search API |
| [hybrid_fusion_api.md](hybrid_fusion_api.md) | Hybrid Fusion API |
| [hybrid_search_design.md](hybrid_search_design.md) | Hybrid Search Design |
| [pagination_benchmarks.md](pagination_benchmarks.md) | Pagination Performance |
| [performance_tuning.md](performance_tuning.md) | Search Performance Tuning |
| [stemming.md](stemming.md) | Stemming Algorithms |
| [migration_guide.md](migration_guide.md) | Migration Guide |

## Verwandte Dokumentation

- [Index Module](../index/README.md) - Index Implementations
- [Query Module](../query/README.md) - Query Engine
- [AQL Documentation](../aql/README.md) - Query Syntax

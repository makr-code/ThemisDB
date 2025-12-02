# Cache-Dokumentation

**Source Code:** `src/cache/`, `include/cache/`

Diese Dokumentation beschreibt die Caching-Komponenten von ThemisDB.

## Übersicht

ThemisDB implementiert mehrere Caching-Strategien für optimale Performance:
- Block Cache (RocksDB)
- Query Result Cache
- Graph Topology Cache
- HNSW Upper Layer Cache

## Dokumentation in diesem Ordner

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [cache_overview.md](./cache_overview.md) | Konzept und Architektur | 📋 TODO |
| [cache_implementation.md](./cache_implementation.md) | Implementierungsdetails | 📋 TODO |
| [cache_config.md](./cache_config.md) | Konfigurationsoptionen | 📋 TODO |
| [cache_performance.md](./cache_performance.md) | Benchmarks & Tuning | 📋 TODO |

## Verwandte Dokumentation

- [Architektur: Caching Data Structures](../architecture/architecture_caching_structures.md)
- [Architektur: Caching Lookup Patterns](../architecture/architecture_caching_patterns.md)
- [Architektur: Cache Invalidation Strategy](../architecture/architecture_cache_invalidation.md)
- [Performance: Memory Tuning](../performance/performance_memory.md)

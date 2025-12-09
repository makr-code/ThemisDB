# Cache Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Cache

---

## Übersicht

Das Cache-Modul bietet verschiedene Caching-Strategien für ThemisDB, einschließlich Semantic Cache für LLM-Responses und Query-Result-Caching.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| SemanticCache | `semantic_cache.h` | `semantic_cache.cpp` | LLM Response Cache |
| ResultCache | `result_cache.h` | - | Query Result Cache |
| CacheProvider | `cache_provider.h` | - | Cache Abstraction |

**Gesamt:** 6 Header, 1 Source-Datei, ~500 LOC

## Implementierte Klassen

### SemanticCache

LLM Response Cache mit TTL-Support:

```cpp
class SemanticCache {
    // Storage: RocksDB Column Family "semantic_cache"
    // Key: SHA256(prompt + params)
    // Value: JSON {response, metadata, timestamp, ttl_seconds}
    
    struct CacheEntry {
        std::string response;
        nlohmann::json metadata;
        int64_t timestamp_ms;
        int ttl_seconds;
    };
    
    struct Stats {
        uint64_t hit_count;
        uint64_t miss_count;
        uint64_t total_entries;
        uint64_t total_size_bytes;
        double hit_rate;
        double avg_latency_ms;
    };
    
    // API
    bool put(prompt, params, response, metadata, ttl_seconds);
    std::optional<CacheEntry> query(prompt, params);
    Stats getStats();
    uint64_t clearExpired();
    bool clear();
};
```

### ResultCache

Query-Ergebnis-Caching:

```cpp
class ResultCache {
    struct Key { std::string query_hash; };
    struct Entry { 
        nlohmann::json result;
        std::chrono::system_clock::time_point created;
        std::chrono::seconds ttl;
    };
    
    void put(const Key& key, const nlohmann::json& result, ttl);
    std::optional<nlohmann::json> get(const Key& key);
    void invalidate(const Key& key);
    void clear();
};
```

## HTTP API

### POST /cache/put
```json
{
  "prompt": "What is the capital of France?",
  "parameters": {"model": "gpt-4", "temperature": 0.7},
  "response": "The capital of France is Paris.",
  "metadata": {"tokens": 15, "cost_usd": 0.001},
  "ttl_seconds": 3600
}
```

### POST /cache/query
```json
{
  "prompt": "What is the capital of France?",
  "parameters": {"model": "gpt-4", "temperature": 0.7}
}
```

### GET /cache/stats
```json
{
  "hit_count": 42,
  "miss_count": 8,
  "hit_rate": 0.84,
  "avg_latency_ms": 1.2,
  "total_entries": 100,
  "total_size_bytes": 524288
}
```

## Verwandte Dokumentation

- [Features: Semantic Cache](../features/features_semantic_cache.md) - Feature-Details
- [Architektur: Caching Data Structures](../architecture/architecture_caching_structures.md)
- [Architektur: Caching Lookup Patterns](../architecture/architecture_caching_patterns.md)
- [Architektur: Cache Invalidation Strategy](../architecture/architecture_cache_invalidation.md)
- [Performance: Memory Tuning](../performance/performance_memory.md)

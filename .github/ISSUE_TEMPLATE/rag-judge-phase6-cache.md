---
name: RAG Judge - EvaluationCache Implementation
about: Implementierung des LRU-Cache mit TTL für Performance-Optimierung (2-3 Tage)
title: '[RAG-JUDGE-P6-CACHE] EvaluationCache Implementation - LRU + TTL + Warming'
labels: 'priority:P2, type:feature, area:performance, area:rag, effort:medium, phase:6'
assignees: ''
---

## 📋 Übersicht

Implementierung des `EvaluationCache` für hochperformante Evaluation-Caching mit LRU-Eviction, TTL-Expiration und Cache-Warming.

**Namespace:** `themis::rag::judge`  
**Header:** `include/rag/evaluation_cache.h` (✅ bereits vorhanden)  
**Implementation:** `src/rag/evaluation_cache.cpp` (🚧 zu implementieren)  
**Dokumentation:** `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

## 🎯 Ziele

- ✅ API-Header definiert (190 LOC)
- 🚧 LRU-Cache-Implementierung (~350 LOC)
- 🚧 TTL-basierte Expiration
- 🚧 Cache-Warming für Common-Queries
- 🚧 Thread-Safe-Operations
- 🚧 Comprehensive Statistics
- 🚧 Unit Tests (10 Tests)

## 📦 Arbeitspakete

### 1. LRU-Cache-Core (1 Tag)

**Zu implementieren:**
- [ ] `get(query, answer)` - Cache-Lookup
  - [ ] Compute Cache-Key (Hash von query + answer)
  - [ ] Check if entry exists und nicht expired
  - [ ] Update LRU-Position (move to front)
  - [ ] Increment access_count und cache_hits
  - [ ] Return pointer to cached result (oder nullptr)
- [ ] `put(query, answer, result)` - Cache-Insert
  - [ ] Compute Cache-Key
  - [ ] Check capacity: evict LRU if full
  - [ ] Insert entry mit timestamp
  - [ ] Add to LRU-List (front)
- [ ] `evictLRU()` - Evict Least Recently Used
  - [ ] Remove from LRU-List (back)
  - [ ] Remove from cache map
  - [ ] Update statistics (evictions++)
- [ ] `updateLRU(key)` - Move to front of LRU-List
  - [ ] Remove from current position
  - [ ] Insert at front

**Data Structures:**
```cpp
std::unordered_map<CacheKey, CacheEntry> cache_;
std::list<CacheKey> lru_list_;  // Most recent at front
std::unordered_map<CacheKey, std::list<CacheKey>::iterator> lru_map_;
```

**Acceptance Criteria:**
- LRU-Eviction funktioniert korrekt
- O(1) get/put-Operationen
- LRU-Ordnung korrekt maintained

---

### 2. TTL-Expiration (0.5 Tage)

**Zu implementieren:**
- [ ] `isExpired(entry)` - Check TTL
  - [ ] Compare current_time - timestamp > ttl
  - [ ] Return true if expired
- [ ] TTL-Check in `get()`
  - [ ] If expired: remove entry, return nullptr
  - [ ] Increment invalidations counter
- [ ] Lazy Expiration (on get)
- [ ] Optional: Background Thread für proaktive Expiration

**Acceptance Criteria:**
- Expired entries nicht zurückgegeben
- TTL konfigurierbar
- Expired entries entfernt bei next access

---

### 3. Cache-Warming (0.5 Tage)

**Zu implementieren:**
- [ ] `warmCache(judge, queries)` - Pre-compute Common Queries
  - [ ] Iterate über warming_queries
  - [ ] Judge evaluiert jede Query
  - [ ] Speichere Result in Cache
  - [ ] Log warming-progress
- [ ] Background-Warming (optional)
  - [ ] Thread für periodisches Re-Warming
  - [ ] Configurable warming_interval
- [ ] Warming-Status-Tracking

**Acceptance Criteria:**
- Cache-Warming funktioniert vor Production-Traffic
- Hit-Rate erhöht sich durch Warming
- Warming nicht blocking

---

### 4. Thread-Safety (0.5 Tage)

**Zu implementieren:**
- [ ] `std::mutex mutex_` - Protect all cache operations
- [ ] Lock in `get()` - Reader lock
- [ ] Lock in `put()` - Writer lock
- [ ] Lock in `clear()` und `invalidate()`
- [ ] Minimize lock-contention
  - [ ] Fine-grained locking (optional)
  - [ ] Lock-free reads (optional, advanced)

**Acceptance Criteria:**
- Thread-safe unter concurrent access
- No data races (Valgrind/ThreadSanitizer clean)
- Performance nicht signifikant beeinträchtigt

---

### 5. Invalidation & Statistics (0.5 Tage)

**Zu implementieren:**

#### Invalidation
- [ ] `invalidate(trigger, metadata)` - Selective Invalidation
  - [ ] MODEL_UPDATE: Clear all entries
  - [ ] CONFIG_CHANGE: Clear all entries
  - [ ] TTL_EXPIRED: Remove specific entry
  - [ ] MANUAL: Clear by pattern/metadata
- [ ] `clear()` - Complete cache flush
- [ ] `registerInvalidationCallback()` - Callback bei Invalidation

#### Statistics
- [ ] `getStatistics()` - Return CacheStatistics
  - [ ] total_requests, cache_hits, cache_misses
  - [ ] hit_rate = hits / (hits + misses)
  - [ ] current_size, max_size
  - [ ] evictions, invalidations
  - [ ] average_lookup_time
- [ ] `resetStatistics()` - Reset counters
- [ ] Track timing with `std::chrono`

**Acceptance Criteria:**
- Invalidation funktioniert für alle Triggers
- Statistics akkurat
- Callback-System funktioniert

---

### 6. Integration & Configuration (0.5 Tage)

**Zu implementieren:**
- [ ] `setConfig(config)` / `getConfig()` - Runtime Config Updates
  - [ ] Adjust max_entries (mit Eviction falls nötig)
  - [ ] Adjust TTL (existing entries behalten alte TTL)
  - [ ] Toggle warming on/off
- [ ] `contains(query, answer)` - Check existence
- [ ] `computeKey(query, answer)` - Hash function
  - [ ] std::hash oder custom hash
  - [ ] Collision-resistent

**Acceptance Criteria:**
- Config-Updates zur Runtime möglich
- Cache-Key eindeutig
- Integration mit RAGJudge einfach

---

### 7. Unit Tests (0.5 Tage)

**Test-Suite:** `tests/test_evaluation_cache.cpp`

**Zu implementieren (10 Tests):**
1. [ ] `TEST(EvaluationCache, BasicGetPut)`
   - Insert, retrieve, verify
2. [ ] `TEST(EvaluationCache, LRUEviction)`
   - Fill cache, verify LRU removed
3. [ ] `TEST(EvaluationCache, TTLExpiration)`
   - Insert, wait TTL, verify expired
4. [ ] `TEST(EvaluationCache, CacheWarming)`
   - Warm cache, verify entries present
5. [ ] `TEST(EvaluationCache, ThreadSafety)`
   - Concurrent gets/puts from multiple threads
6. [ ] `TEST(EvaluationCache, InvalidationTriggers)`
   - Test all trigger types
7. [ ] `TEST(EvaluationCache, Statistics)`
   - Verify hit rate, counters accurate
8. [ ] `TEST(EvaluationCache, ConfigUpdate)`
   - Change max_entries, verify eviction
9. [ ] `TEST(EvaluationCache, CacheClear)`
   - Clear cache, verify empty
10. [ ] `TEST(EvaluationCache, HighThroughput)`
    - Performance test with 10k+ entries

**Acceptance Criteria:**
- Alle 10 Tests bestehen
- ThreadSanitizer clean
- Performance-Test zeigt < 1ms lookup

---

## 🔗 Abhängigkeiten

**Code:**
- `include/rag/rag_judge.h` - EvaluationResult, RAGJudge
- `include/utils/logger.h` - Logging
- `<mutex>`, `<chrono>` - Threading, Timing

**Voraussetzungen:**
- Phase 1-4 Judge-Implementation
- C++20 Standard (für `<chrono>` features)

---

## 📊 Erfolgskriterien

- [ ] Alle Methoden in `evaluation_cache.h` implementiert
- [ ] 10 Unit Tests bestehen
- [ ] Cache-Hit-Rate > 80% für repeated queries
- [ ] Lookup-Time < 1ms (cache hit)
- [ ] Thread-safe unter concurrent load
- [ ] Dokumentation aktualisiert
- [ ] Code Review abgeschlossen
- [ ] Keine Compiler-Warnings

---

## 📝 Implementation Notes

**Performance-Targets:**
- Cache-Lookup (Hit): < 1ms
- Cache-Insert: < 2ms
- LRU-Eviction: < 1ms
- Cache-Warming: < 10s für 1000 Queries
- Thread-Contention: Minimal (< 5% overhead)

**Best Practices:**
- Use `std::unordered_map` für O(1) lookup
- Use `std::list` für O(1) LRU-Manipulation
- Minimize lock-hold-time
- Log cache-statistics periodisch
- Monitor eviction-rate (high = increase capacity)

**Memory Management:**
- Max 1000 entries default = ~5-10 MB (assuming 5-10 KB/entry)
- Configurable max_entries für memory control
- Consider serialization für persistent cache (future)

**Konfiguration:**
```yaml
cache:
  max_entries: 1000             # Maximum cached evaluations
  ttl_seconds: 3600             # 1 hour TTL
  enable_warming: true          # Pre-compute common queries
  warming_interval_seconds: 300 # Re-warm every 5 min
  warm_queries:                 # Queries to pre-compute
    - "What is machine learning?"
    - "Explain neural networks"
```

---

## 📚 Referenzen

- [LRU Cache Algorithm](https://en.wikipedia.org/wiki/Cache_replacement_policies#LRU)
- [C++ std::list Performance](https://en.cppreference.com/w/cpp/container/list)
- Phase 6 Progress: `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`
- Basic Cache Implementation: `src/rag/rag_judge.cpp` (lines 37-43, 121-128, 260-262)

---

**Labels:** `priority:P2`, `type:feature`, `area:performance`, `area:rag`, `effort:medium`, `phase:6`  
**Estimated Effort:** 2-3 Tage (1 Developer)  
**Dependencies:** Phase 1-4 Complete  
**Follow-up:** BatchEvaluator Implementation, Performance Benchmarking

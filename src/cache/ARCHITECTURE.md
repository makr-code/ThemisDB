> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Cache Module — Architecture Guide
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/src/cache/README.md -->

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/cache/`

---

## 1. Overview

The Cache module implements a multi-level, adaptive query result cache for ThemisDB. It sits
between the query engine and the storage layer, transparently serving repeated or semantically
similar queries from fast in-memory or compressed storage instead of hitting RocksDB.

The cache is designed to be **tenant-aware**, **fault-tolerant** (circuit breaker for L3),
and **semantically intelligent** (vector similarity matching for near-duplicate queries).

---

## 2. Design Principles

- **Multi-Level Pipeline** – L1 (in-memory LRU) → L2 (compressed in-memory) → L3
  (RocksDB-backed persistent). Each level provides progressively higher capacity at the
  cost of access latency.
- **Circuit Breaker** – RocksDB (L3) failures are isolated; the cache degrades to L1/L2
  without propagating storage errors to the caller.
- **Semantic Matching** – `semantic_cache.cpp` uses vector embeddings to match queries
  that are semantically equivalent but syntactically different.
- **Tenant Isolation** – all keys are namespaced per tenant; cross-tenant cache leaks are
  architecturally impossible.
- **Configuration Validation** – the cache constructor validates all parameters and throws
  `std::invalid_argument` on misconfiguration, failing fast at startup.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `adaptive_query_cache.cpp` | Main cache façade: L1→L2→L3 lookup and write pipeline |
| `bounded_lru_cache.cpp` | L1 in-memory LRU cache with per-entry size limits |
| `semantic_cache.cpp` | Vector similarity-based cache lookup for near-duplicate queries |
| `embedding_cache.cpp` | Dedicated cache for embedding vectors |
| `warmup.cpp` | Bulk cache warmup from pre-computed result sets or query log snapshots |
| `predictive_prefetcher.cpp` | Predictive pre-fetching based on query sequence history |
| `cache_hit_rate_slo_monitor.cpp` | Cache hit-rate SLO alerting and monitoring |
| `cache_replication.cpp` | Cache replication event handling for high-availability deployments |
| `cache_replication_coordinator.cpp` | In-process cache replication coordination |
| `distributed_cache_coordinator.cpp` | Distributed cache coordination (node-local bus, `InProcessCacheCoordinator`) |
| `redis_cache_coordinator.cpp` | Redis pub/sub backed distributed cache coordinator (`RedisCacheCoordinator`) |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Query Engine (src/query/)                   │
│                      cache.get(query_key)                       │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│               AdaptiveQueryCache (main façade)                   │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  SemanticCache: hash(embedding) → near-duplicate lookup    │  │
│  └────────────────────────────────────────────────────────────┘  │
│                           │                                      │
│  ┌───────────┐  ┌─────────▼───────┐  ┌────────────────────────┐ │
│  │    L1     │  │      L2         │  │         L3             │ │
│  │ BoundedLRU│→ │ Compressed LRU  │→ │  RocksDB Persistent   │ │
│  │ (in-mem)  │  │ (zstd/lz4)      │  │  (circuit breaker)    │ │
│  └───────────┘  └─────────────────┘  └────────────────────────┘ │
│                                                                  │
│  Token Bucket Rate Limiter │ Tenant Namespace Enforcer           │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Cache Read (Lookup)

```
cache.get(tenant_id, query_hash)
    │
    ├─ L1 BoundedLRU hit? → return immediately
    │
    ├─ SemanticCache: embedding similarity match? → return (promote to L1)
    │
    ├─ L2 hit? → decompress → return (promote to L1)
    │
    ├─ CircuitBreaker: CLOSED?
    │       ├─ yes → L3 RocksDB get → decompress → return (promote L1+L2)
    │       └─ no (OPEN/HALF_OPEN) → skip L3, return cache miss
    │
    └─ cache miss → caller executes query → cache.put(result)
```

### 4.2 Cache Write (Insert)

```
cache.put(tenant_id, query_hash, result)
    │
    ├─ validate entry size (reject oversized entries)
    ├─ check per-tenant quota
    ├─ write to L1 (with LRU eviction if full)
    ├─ compress and write to L2
    └─ CircuitBreaker CLOSED? → write to L3 RocksDB with retry
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/query/` | Query result caching |
| **Called by** | `src/llm/` | Embedding result caching via `embedding_cache.cpp` |
| **Uses** | `src/storage/` | RocksDB instance for L3 persistence |
| **Uses** | `src/index/` | Vector index for semantic similarity lookup |
| **Uses** | `src/observability/` | Cache hit/miss metrics and tracing |

---

## 6. Threading & Concurrency Model

- `BoundedLRUCache` (L1) uses a mutex; reads/writes serialize through the lock.
- `AdaptiveQueryCache` is thread-safe; multiple query threads can read/write concurrently.
- `SemanticCache` uses a shared_mutex (multiple readers, one writer).
- `EmbeddingCache` is independently thread-safe.
- Circuit breaker state transitions use an atomic state variable.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| L1 in-memory LRU | O(1) lookup via hash map + doubly-linked list |
| L2 compressed | zstd/lz4 compression reduces memory footprint by 3-10× |
| L3 RocksDB prefix scan | Pattern-based invalidation via iterator |
| Semantic matching | Embedding similarity avoids re-executing semantically identical queries |
| Circuit breaker | Prevents L3 latency spikes from degrading L1/L2 performance |
| Cache warmup | `warmup.cpp` pre-populates L1/L2 at startup for hot paths |

---

## 8. Security Considerations

- Tenant namespacing is enforced at the cache key level; cross-tenant access is
  structurally impossible.
- Per-tenant size quotas prevent cache exhaustion attacks.
- Token bucket rate limiter prevents cache flooding from a single tenant.
- Cache entries are not encrypted at rest (L3); enable RocksDB encryption at the storage
  layer for at-rest protection.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `cache.l1.max_entries` | 10000 | L1 LRU max entries |
| `cache.l1.max_entry_size_bytes` | 1024 | Max single L1 entry size |
| `cache.l2.max_entries` | 50000 | L2 compressed cache max entries |
| `cache.l2.max_entry_size_bytes` | 10240 | Max single L2 entry size |
| `cache.l3.enabled` | true | Enable L3 RocksDB persistence |
| `cache.circuit_breaker.failure_threshold` | 5 | Failures before OPEN |
| `cache.circuit_breaker.timeout_s` | 30 | OPEN → HALF_OPEN timeout |
| `cache.rate_limit.requests_per_second` | 10000 | Token bucket rate limit |
| `cache.semantic.similarity_threshold` | 0.95 | Min cosine similarity for hit |
| `cache.ttl_s` | 3600 | Default entry TTL in seconds |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Oversized entry | Reject with structured error; log metric |
| L3 RocksDB write failure | Circuit breaker records failure; retry with backoff |
| Circuit breaker OPEN | Skip L3; serve from L1/L2; log degradation |
| Tenant quota exceeded | Reject with 429-equivalent error |
| Compression failure | Fall back to uncompressed storage; log warning |

---

## 11. Known Limitations & Future Work

- Tenant management API (`/v1/admin/cache/tenants` PATCH endpoint) is in progress (Issue: #1579).
- Distributed cache coordination (`RedisCacheCoordinator`) requires an external Redis server; enable via `THEMIS_ENABLE_REDIS=ON` and link hiredis. Degrades gracefully when Redis is unavailable.
- True cluster bus variant (without Redis dependency) is deferred; `InProcessCacheCoordinator` covers single-binary deployments.
- Signed invalidation messages for distributed coordinator are planned to prevent unauthenticated cache-flush attacks (see `FUTURE_ENHANCEMENTS.md`).

---

## 12. References

- `src/cache/README.md` — module overview and phase summaries
- `src/cache/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/cache_roadmap.md` — cache roadmap
- `docs/de/architecture/architecture_cache_invalidation.md` — invalidation patterns
- `docs/de/architecture/architecture_caching_patterns.md` — caching strategy overview
- `ARCHITECTURE.md` (root) — full system architecture

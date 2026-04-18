# Cache Troubleshooting Guide

The `cache` module provides multi-tier caching for ThemisDB including adaptive LRU query result caches (L1/L2/L3), semantic embedding caches, workload-specific cache policies, tenant isolation, and cache warmup strategies.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Cache hit rate < 20% | Cache too small or TTL too short | Increase `cache.l1.max_entries`; tune `ttl_ms` |
| L3 cache OOM crash | No upper bound on L3 disk usage | Set `cache.l3.max_size_gb` |
| Stale results returned | TTL too long or invalidation disabled | Reduce `ttl_ms`; enable `invalidate_on_write` |
| Semantic cache returns wrong results | Similarity threshold too low | Increase `cache.semantic.similarity_threshold` |
| Tenant A sees Tenant B's cached data | Tenant isolation not enabled | Set `cache.tenant_isolation: true` |
| Cache warmup takes too long | Too many entries in warmup list | Reduce warmup set or run async |
| `BoundedLruCache: eviction rate too high` | Working set larger than cache | Increase cache size or use tiered caching |
| Embedding cache miss after model reload | Model change invalidates embeddings | Flush embedding cache after model update |
| `AdaptiveQueryCache: policy flip too frequent` | Workload classifier unstable | Increase `policy_stability_window_ms` |
| Cache writes blocked during compaction | L3 compaction not running async | Enable `cache.l3.async_compaction: true` |

## Common Issues

### Issue 1: Low Cache Hit Rate Under Read-Heavy Workload

**Description:** Despite a read-heavy workload, cache hit rate remains low.

**Symptoms:**
- Prometheus metric `themisdb_cache_hit_rate < 0.30`
- Log: `AdaptiveQueryCache: evicting entries too frequently`

**Cause:** Cache is too small for the working set; LRU evicts useful entries.

**Solution:**
```yaml
cache:
  l1:
    max_entries: 10000          # increase from 1000
    ttl_ms: 30000
    eviction_policy: lru
  l2:
    max_entries: 100000
    ttl_ms: 120000
    backend: in_memory
  l3:
    enabled: true
    backend: rocksdb
    max_size_gb: 20
    ttl_ms: 3600000             # 1 hour for cold data
```

---

### Issue 2: Stale Data Returned After Collection Write

**Description:** Cached query results are returned after the underlying data was modified.

**Symptoms:**
- Query returns a document that was deleted
- Log: `AdaptiveQueryCache: serving cached result (age=300s) for collection=orders`

**Cause:** `invalidate_on_write` is disabled; TTL is too long.

**Solution:**
```yaml
cache:
  invalidate_on_write: true         # invalidate affected cache entries on writes
  l1:
    ttl_ms: 5000                    # short TTL for frequently-written collections
  write_through:
    enabled: true                   # update cache on write path
```

---

### Issue 3: Semantic Cache Returns Semantically-Wrong Result

**Description:** The semantic cache serves a cached result for a query that has a different meaning.

**Symptoms:**
- Query "Show orders from Berlin" returns cached result from "Show orders from Munich"
- Log: `SemanticCache: similarity=0.72 (threshold=0.70) – cache hit`

**Cause:** Cosine similarity threshold is too low; semantically different queries with similar vocabulary match.

**Solution:**
```yaml
cache:
  semantic:
    enabled: true
    similarity_threshold: 0.92      # increase from 0.70
    embedding_model: text-embedding-3-small
    max_entries: 50000
    ttl_ms: 60000
```

---

### Issue 4: Tenant Data Leaking Between Cache Entries

**Description:** Tenant A retrieves cached results that belong to Tenant B.

**Symptoms:**
- Tenant A queries return documents from Tenant B's dataset
- Audit log shows cross-tenant cache hits

**Cause:** Cache keys do not include tenant ID; tenant isolation disabled.

**Solution:**
```yaml
cache:
  tenant_isolation: true            # prefix all cache keys with tenant_id
  l1:
    key_components: [tenant_id, collection, query_hash]
```

---

### Issue 5: Cache Warmup Blocks Server Startup

**Description:** Server startup takes minutes because cache warmup is synchronous.

**Symptoms:**
- Log: `CacheWarmup: loading 50000 entries... (blocking startup)`
- Health checks fail during startup window

**Cause:** Warmup is configured to run synchronously before the server accepts connections.

**Solution:**
```yaml
cache:
  warmup:
    enabled: true
    async: true                     # run warmup in background after startup
    max_entries: 10000              # limit warmup set size
    source: recent_queries          # "recent_queries" | "static_list" | "analytics"
    timeout_ms: 60000               # abort warmup if it takes too long
```

---

### Issue 6: L3 RocksDB Cache Runs Out of Disk Space

**Description:** The L3 disk-backed cache consumes all available disk space.

**Symptoms:**
- Log: `AdaptiveQueryCache: L3 write failed: ENOSPC`
- Server becomes read-only

**Cause:** No upper bound set on L3 cache size; TTL cleanup not running.

**Solution:**
```yaml
cache:
  l3:
    enabled: true
    max_size_gb: 50                 # hard cap
    ttl_ms: 86400000                # 24 hours
    compaction_schedule: "0 3 * * *"  # compact daily at 3am
    async_compaction: true
```

---

### Issue 7: Embedding Cache Invalidated After Model Reload

**Description:** After updating the embedding model, all embedding cache entries are invalid.

**Symptoms:**
- Log: `EmbeddingCache: model fingerprint changed; flushing 200000 entries`
- Semantic search performance degrades for hours after model update

**Cause:** New model produces different embeddings; old cache entries are incompatible.

**Solution:**
```bash
# Pre-warm embedding cache after model update
themisdb-admin cache embedding-warmup \
  --collection users \
  --field description \
  --batch-size 100

# Monitor warmup progress
curl -s http://localhost:9100/metrics | grep themisdb_cache_embedding_warmup
```
```yaml
cache:
  embedding:
    version_aware: true             # include model version in cache key
    pre_warm_on_model_change: true
    pre_warm_batch_size: 100
```

---

### Issue 8: Adaptive Policy Switches Too Frequently

**Description:** The `AdaptiveQueryCache` switches caching policy multiple times per minute.

**Symptoms:**
- Log: `AdaptiveQueryCache: policy changed from write_through to cache_aside (workload shift)`
- High overhead from policy evaluation

**Cause:** Workload classifier window is too short; minor fluctuations cause policy oscillation.

**Solution:**
```yaml
cache:
  adaptive:
    enabled: true
    policy_stability_window_ms: 60000   # require 60s stable signal before switching
    min_policy_duration_ms: 300000       # hold policy for at least 5 min
    workload_sample_size: 1000
```

## Diagnostic Commands

```bash
# Cache statistics (hit rates, sizes)
themisdb-admin cache stats

# Flush entire cache
themisdb-admin cache flush --tier all

# Flush cache for a specific collection
themisdb-admin cache flush --collection orders

# Inspect L3 cache size
du -sh /var/lib/themisdb/cache/l3/

# Show warmup status
themisdb-admin cache warmup-status

# Live cache metrics
curl -s http://localhost:9100/metrics | grep themisdb_cache

# Tail cache logs
journalctl -u themisdb -f | grep -E "cache|lru|evict|warmup|semantic"
```

## Configuration Reference

```yaml
cache:
  enabled: true
  tenant_isolation: true
  invalidate_on_write: true
  l1:
    max_entries: 5000
    ttl_ms: 10000
    eviction_policy: lru
  l2:
    max_entries: 50000
    ttl_ms: 60000
  l3:
    enabled: false
    backend: rocksdb
    path: /var/lib/themisdb/cache/l3
    max_size_gb: 10
    ttl_ms: 3600000
  semantic:
    enabled: false
    similarity_threshold: 0.92
    max_entries: 10000
    ttl_ms: 60000
  embedding:
    max_entries: 100000
    ttl_ms: 3600000
    version_aware: true
  warmup:
    enabled: false
    async: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `tenant_isolation` | `false` | `true` in multi-tenant deployments |
| `semantic.similarity_threshold` | `0.70` | `0.90–0.95` |
| `invalidate_on_write` | `false` | `true` for consistent reads |
| `l3.max_size_gb` | unset | Always set an explicit limit |

## Known Limitations

- Semantic cache requires an embedding model to be configured; adds latency to cache lookup.
- L3 RocksDB cache does not support point-in-time invalidation of a single key; only full-table invalidation.
- Adaptive policy switching is not cluster-aware; each node may use a different policy under split workloads.
- Embedding cache entries are invalidated on any model parameter change, including minor patch updates.

## Related Documentation

- [Cache Module ROADMAP](../../src/cache/ROADMAP.md)
- [Cache Roadmap](../cache_roadmap.md)
- [Workload-Specific Caching](../performance/WORKLOAD_SPECIFIC_CACHING.md)
- [Cache Optimization 1536D Summary](../performance/CACHE_OPTIMIZATION_1536D_SUMMARY.md)
- [Response Cache Metrics](../RESPONSE_CACHE_METRICS_COMPLETE.md)

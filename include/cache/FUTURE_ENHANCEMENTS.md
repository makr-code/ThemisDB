# Cache Module - Future Header Enhancements

## Scope

- `ICache<K,V>` interface extensions for pluggable eviction and write-through modes
- Eviction policy plug-in interface (`IEvictionPolicy`) supporting LFU, ARC, and custom strategies — **✅ Implemented in `include/cache/cache_interfaces.h`**
- Cache warmup API (`ICacheWarmup`) for pre-populating entries from query logs at startup — **✅ Implemented in `include/cache/cache_interfaces.h`**
- Admin operation interface (`ICacheAdminOps`) for runtime inspection, flush, and stats collection — **✅ Implemented in `include/cache/cache_interfaces.h`**
- GDPR purge hook (`IGDPRPurgeHook`) for synchronous, audited removal of PII-bearing cache entries — **✅ Implemented in `include/cache/cache_interfaces.h`**
- TTL adaptation interface (`ITTLAdapter`) for workload-driven TTL tuning within configured bounds — **✅ Implemented in `include/cache/cache_interfaces.h`**

## Design Constraints

- `[x]` `ICache<K,V>` interface is thread-safe; all methods safe to call concurrently without external locking
- `[x]` Eviction policies are hot-swappable at runtime; `IEvictionPolicy` must not hold references into `ICache<K,V>` internals
- `[x]` `ITTLAdapter` must never extend a TTL beyond the per-cache configured maximum; violations are a contract error
- `[x]` GDPR purge via `IGDPRPurgeHook` is synchronous and must produce an audit-log entry before returning
- `[x]` `ICacheAdminOps` methods are privileged; interface is only obtainable via an authenticated admin accessor, not from `ICache<K,V>` directly
- `[x]` All public header types are forward-declarable; no implementation-detail headers pulled in by `ICache<K,V>`

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ICache<K,V>` | `QueryEngine`, `EmbeddingLayer`, `SessionManager` | Base cache interface; `get/put/remove` are thread-safe and return `Result<T>` |
| `IEvictionPolicy` | `ICache<K,V>` implementation, `CacheManager` | Pure-virtual strategy; receives `EvictionEvent` notifications; must not access stored values |
| `ICacheWarmup` | `CacheManager`, `StartupCoordinator` | Batch-inserts entries from a `WarmupSource`; returns `WarmupStats` on completion |
| `ICacheAdminOps` | `AdminAPI`, `ObservabilityLayer` | Provides `flush()`, `stats()`, `resize()`, and `listKeys()`; gated by admin capability |
| `IGDPRPurgeHook` | `GDPRComplianceService`, `DataRetentionManager` | Synchronous purge of keys matching a `PurgeDescriptor`; writes to audit log before return |
| `ITTLAdapter` | `ICache<K,V>` implementation, `WorkloadAdaptationLayer` | Computes adapted TTL for a key given access patterns; bounded by `maxTTL` config |

## Planned Features

### Admin Cache Operations Interface

- `[x]` Define `ICacheAdminOps` with `flush()`, `stats() -> CacheStats`, `resize(size_t newCapacity)`, and `listKeys(KeyFilter) -> KeyList` — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Expose `CacheStats` as a plain-data struct: hit count, miss count, eviction count, current size, capacity — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Add `KeyFilter` value type supporting prefix, pattern, and TTL-range predicates — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Document that `flush()` is a blocking call; all in-flight `get/put` operations complete before flush returns — **documented in `cache_interfaces.h` doxygen**

### Cache Warmup API

- `[x]` Define `ICacheWarmup::warm(WarmupSource&) -> Result<WarmupStats>` in public header — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Expose `WarmupSource` as a pure-virtual interface with `nextBatch() -> std::vector<CacheEntry<K,V>>` — **`IWarmupSource` in `include/cache/cache_interfaces.h`**
- `[x]` Add `WarmupStats` value type: entries inserted, entries skipped (TTL expired), duration, error count — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Document that `warm()` respects existing TTL values from the source and does not override live entries — **documented in `cache_interfaces.h` doxygen**

### Adaptive TTL Interface

- `[x]` Define `ITTLAdapter::computeTTL(const K& key, AccessPattern) -> std::chrono::milliseconds` — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Expose `AccessPattern` as a plain-data struct: access frequency, last-access age, write ratio — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Add `ITTLAdapter::configure(TTLAdapterConfig)` to set min/max bounds and adaptation aggressiveness — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Document hard constraint: `computeTTL()` must never return a value exceeding `TTLAdapterConfig::maxTTL` — **documented in `cache_interfaces.h` doxygen**

### Pluggable Eviction Policy API

- `[x]` Define `IEvictionPolicy` with `onAccess(const K& key)`, `onInsert(const K& key)`, `onRemove(const K& key)`, and `evict() -> K` — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Expose `EvictionEvent` as a tagged-union value type covering access, insert, remove, and expiry events — **`EvictionEventType` enum + `EvictionEvent` struct in `include/cache/cache_interfaces.h`**
- `[x]` Add `IEvictionPolicy::name() -> std::string_view` for observability and admin display — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Document that `evict()` must return a valid key from the current key set; empty-cache behavior is undefined — **documented in `cache_interfaces.h` doxygen**

### GDPR-Aware Purge Hook

- `[x]` Define `IGDPRPurgeHook::purge(PurgeDescriptor) -> PurgeResult` — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Expose `PurgeDescriptor` with subject ID, key patterns, and purge reason (enum class) — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Add `PurgeResult` value type: purged key count, audit-log entry ID, and timestamp — **implemented in `include/cache/cache_interfaces.h`**
- `[x]` Document that `purge()` is synchronous; the calling thread blocks until all matching keys are removed and the audit entry is written — **documented in `cache_interfaces.h` doxygen**

### Phase 6: Distribution & Partitioning Headers (New — v2.0.0)

- `[x]` Define `IDistributedEviction` for cross-node coordinated eviction — `include/cache/distributed_eviction.h`; `evict()`, `evictByPattern()`, `evictByTenant()`, `flush()`, listener registration, `DistributedEvictionEvent`, `DistributedEvictionStats`
- `[x]` Define `ICachePartition` for sharded per-tenant cache partition management — `include/cache/cache_partition.h`; `assignTenant()`, `unassignTenant()`, `listTenants()`, `listPartitions()`, `resize()`, `evictPartition()`, `getStats()`, `getAllStats()`, `PartitionStats`
- `[x]` Define `IAdaptiveTTLPolicy` for stateful, history-driven TTL adaptation — `include/cache/adaptive_ttl_policy.h`; `recordAccess()`, `computeTTL()`, `getHistory()`, `evict()`, `pruneHistory()`, `flushHistory()`, `configure()`, `AdaptiveTTLSuggestion`, `AccessRecord`, `AdaptiveTTLPolicyConfig`

## Test Strategy

- Thread-safety tests spawn 64 concurrent threads executing `ICache<K,V>::get` and `put` with overlapping keys; assert no data races under TSan
- Eviction policy contract tests verify that `IEvictionPolicy::evict()` never returns a key absent from the live key set
- GDPR purge tests assert that the audit-log entry is written before `purge()` returns, even when the cache is empty
- Warmup source tests inject a `WarmupSource` with pre-expired TTLs and verify that `WarmupStats::skipped` reflects all expired entries
- Admin ops tests verify `listKeys(KeyFilter)` returns only keys matching the filter predicate, including edge cases (empty cache, full capacity)
- TTL adapter tests assert that `computeTTL()` never exceeds `maxTTL` regardless of `AccessPattern` input values

## Performance Targets

- `ICache<K,V>::get` and `put` latency ≤ 200 ns per operation at 99th percentile under 32-thread concurrent load
- `IEvictionPolicy::onAccess` and `evict()` combined overhead ≤ 500 ns per eviction cycle
- `ICacheWarmup::warm()` batch insert throughput ≤ 1 µs per entry (1M entries in ≤ 1 s on a single thread)
- `IGDPRPurgeHook::purge()` latency ≤ 10 ms per key including audit-log write
- `ICacheAdminOps::stats()` snapshot collection ≤ 100 µs regardless of cache size
- `ITTLAdapter::computeTTL()` computation ≤ 200 ns per key (stateless computation from `AccessPattern`)

## Security / Reliability

- GDPR purge is verified complete via an audit-log entry written before `purge()` returns; partial purge is treated as a fatal error
- Cache keys must never include raw PII; `ICache<K,V>` key type is required to implement a `redactForLog()` method when registered
- `IEvictionPolicy` callbacks receive only keys, never stored values; value access from eviction code is a contract violation
- `ICacheAdminOps` interface is only obtainable via a privileged admin accessor; `ICache<K,V>` does not expose it directly
- `IGDPRPurgeHook::purge()` accepts only structured `PurgeDescriptor` values; free-form string key patterns are rejected to prevent injection
- TTL adaptation is bounded; `ITTLAdapter` cannot extend TTLs beyond configured maximums, preventing unbounded data retention

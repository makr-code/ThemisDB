> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/src/cache/README.md -->

# Cache Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the Cache module (`src/cache/`), comprising `adaptive_query_cache.cpp` (multi-level L1/L2/L3 pipeline), `semantic_cache.cpp` (vector-similarity result caching), `bounded_lru_cache.cpp`, `embedding_cache.cpp`, `predictive_prefetcher.cpp`, `cache_hit_rate_slo_monitor.cpp`, `cache_replication.cpp`, `cache_replication_coordinator.cpp`, `redis_cache_coordinator.cpp`, and `warmup.cpp`. Enhancements to the underlying `storage/rocksdb_wrapper.h` (L3 backing store) and `utils/zstd_codec.h` (L2 compression) are out of scope except where the cache module controls their configuration.

---

## Design Constraints

- `[x]` L1 and L2 in-memory tiers must stay lock-free on the read path; no new `std::mutex` acquisitions may be introduced on `AdaptiveQueryCache::get()`. **Resolved (v1.9.0): `l1_mutex_` is now `std::shared_mutex`; `get()` holds only a `shared_lock`, allowing concurrent readers.**
- `[x]` The `cache::CircuitBreaker` protecting L3 RocksDB must remain the sole fault-isolation mechanism for the persistence tier; new L3 features must check breaker state before every operation. **Enforced: all L3 paths call `circuitBreaker_.allowRequest()` before RocksDB access.**
- `[x]` Per-tenant quotas enforced via `config_.per_tenant_max_bytes` must not be bypassable by any new Admin API write path. **Enforced: `PATCH /v1/admin/cache/tenant/{id}/quota` only raises the quota; the put path always checks `tenant_bytes_used_`.**
- `[x]` Serialization format for L2 compressed entries (`zstd_codec`) and L3 RocksDB keys (`QUERY_CACHE_PREFIX`) must remain stable across minor versions; breaking format changes require a cache flush on upgrade. **Stable since v1.0.0; documented in ARCHITECTURE.md.**
- `[x]` `RedisCacheCoordinator` must gracefully degrade to standalone operation (pub/sub disabled) when Redis is unreachable, without throwing exceptions into caller code. **Resolved (v1.7.0): subscriber thread catches all exceptions and schedules reconnect with exponential back-off.**

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AdaptiveQueryCache::get(key, tenant_id)` | AQL executor, server request handler | Must remain O(1) average on L1 hit path |
| `AdaptiveQueryCache::put(key, value, ttl, tenant_id)` | AQL executor | Rate limiter gating enforced before write |
| `cache::CircuitBreaker` state machine | `AdaptiveQueryCache` L3 path | CLOSED/OPEN/HALF_OPEN transitions observable via metrics |
| `SemanticCache::findSimilar(embedding, threshold)` | LLM query handler in `content/content_manager_llm.cpp` | Cosine threshold configurable per-call |
| `cache::RateLimiter` | `AdaptiveQueryCache::put()` | Token bucket; refill rate and burst configurable |
| `EmbeddingCache` | `SemanticCache`, vector search paths | Must evict LRU entries when capacity is reached |

---

## Planned Features

### [x] Admin HTTP API for Cache Operations
**Priority:** High
**Target Version:** v1.7.0
**Status:** Fully implemented — all 12 endpoints wired in `src/server/http_server.cpp`.

- `[x]` `GET /v1/admin/cache/stats`
- `[x]` `DELETE /v1/admin/cache/key/{encoded_key}`
- `[x]` `DELETE /v1/admin/cache/tenant/{tenant_id}`
- `[x]` `POST /v1/admin/cache/circuit-breaker/reset`
- `[x]` `GET /v1/admin/cache/circuit-breaker`
- `[x]` `GET /v1/admin/cache/health`
- `[x]` `POST /v1/admin/cache/warmup`
- `[x]` `POST /v1/admin/cache/snapshot`
- `[x]` `GET /v1/admin/cache/tenants`
- `[x]` `GET /v1/admin/cache/tenant/{tenant_id}/stats`
- `[x]` `PATCH /v1/admin/cache/tenant/{tenant_id}/quota`

---

### Lock-Free L1 Read Path
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented (v1.9.0)

`AdaptiveQueryCache::get()` previously took an exclusive `std::lock_guard<std::mutex>` on `l1_mutex_` on every read, serialising all reader threads under high concurrency.

**Implementation Notes:**
- `[x]` Replaced `l1_cache_` value type with `std::unordered_map<std::string, std::unique_ptr<L1Entry>>`; `L1Entry` fields (timestamps, TTL, counters) are now `std::atomic`. Copy/move constructors are deleted to prevent unintentional value-type copies.
- `[x]` `l1_mutex_` promoted to `std::shared_mutex`; `get()` acquires a `std::shared_lock` (concurrent readers), all write paths (`put()`, `invalidate()`, `clear()`, `clearExpired()`, warmup, replicated put/invalidate, `exportSnapshot()`) acquire `std::unique_lock`.
- `[x]` Expiry on read uses CAS on `expired_flag` (`std::atomic<bool>`) so only the first thread marks the entry; all readers fall through to L2 without erasing under the shared lock.
- `[x]` `l1_eviction_strategy_` calls protected by a dedicated `l1_eviction_mutex_` (`std::mutex`) so the eviction strategy is never called concurrently.
- `[x]` `onAccess()` removed from the hot read path; access frequency tracked via `access_count.fetch_add(1, relaxed)` per entry.

**Performance Targets:**
- L1 hit path throughput: ≥ 5 M ops/s per core under 16-thread contention.

---

### `RedisCacheCoordinator` Async Pub/Sub Subscription Loop
**Priority:** High
**Target Version:** v1.7.0

`redis_cache_coordinator.cpp` uses synchronous blocking `hiredis` calls (`redisCommand`) for both PUBLISH and SUBSCRIBE. The subscription thread blocks indefinitely on `redisGetReply`. On Redis disconnect, the thread silently exits without notifying the coordinator of the failure; reconnection is only triggered on the next PUBLISH call.

**Implementation Notes:**
- `[x]` Replace synchronous `hiredis` calls with `hiredis-async` + `libuv` or a dedicated async event loop thread to avoid blocking the coordinator's callers. **Delivered:** dedicated background subscriber thread with `redisSetTimeout`-bounded `redisGetReply` calls (200 ms poll interval); coordinator callers are never blocked. Full hiredis-async/libuv migration is a future enhancement.
- `[x]` Implement a reconnection health loop: if the subscription thread exits, schedule reconnect with exponential back-off (1 s, 2 s, 4 s, max 30 s) and emit a `cache.redis.reconnect` metric.
- `[x]` Expose `RedisCacheCoordinator::isConnected()` observable via `GET /v1/admin/cache/health`.
- `[x]` The Windows stub (line 80 of `distributed_cache_coordinator.cpp`) should be replaced with a proper compile-time feature flag; the warning log on every construction is noisy in tests.

---

### Predictive Prefetcher: ML-Based Access Pattern Model
**Priority:** Medium
**Target Version:** v1.8.0

`predictive_prefetcher.cpp` uses a simple frequency counter over a fixed candidate window (`config_.max_predictions`) to predict next accesses. There is no sequential-access pattern detection or time-of-day awareness. The model is not persistent across restarts.

**Implementation Notes:**
- `[x]` Replace frequency counter with a Markov chain transition matrix (order-1) keyed by the last `N` accessed fingerprints; serialize/deserialize the matrix to RocksDB under prefix `prefetch_model::`.
- `[x]` Add time-of-day bucketing (24 one-hour buckets) so prefetch probability is weighted by historical access at the current hour.
- `[x]` Emit `cache.prefetch.hit_rate` and `cache.prefetch.overhead_bytes` metrics via `MetricsCollector` to evaluate model effectiveness in production.
- `[x]` Add a prefetcher A/B test toggle: route 50 % of tenants to Markov model vs. frequency baseline; compare hit-rate improvement.

**Performance Targets:**
- Prefetch prediction latency: ≤ 100 µs per call.
- Prefetch overhead (bytes fetched but never hit): ≤ 10 % of total prefetch volume.

---

### SLO Monitor: Latency Percentile Tracking
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.9.0)

`cache_hit_rate_slo_monitor.cpp` monitors only hit-rate thresholds (`config_.warning_threshold`, `config_.critical_threshold`). It does not track cache operation latency (p50/p99). Latency regressions (e.g. L3 compaction slowing down `get()`) are invisible.

**Implementation Notes:**
- `[x]` Add a rolling HDRHistogram (or `utils/hdr_histogram.h` if available) to `CacheHitRateSloMonitor`; record latency per tier (L1/L2/L3) on each `get()` call. **Delivered: `LatencyHistogram` struct with 12 fixed buckets; `recordLatency(Tier, double)` API; per-tier histograms `latency_hist_[]`.**
- `[x]` Add `CacheSloConfig::p99_warn_ms` and `p99_critical_ms` thresholds; fire Alertmanager alerts when exceeded, similar to the existing hit-rate alert path. **Delivered: `Config::p99_warn_ms` and `p99_critical_ms` added; latency alert firing/resolving path identical to hit-rate path.**
- `[x]` Expose `p50_latency_ms`, `p95_latency_ms`, `p99_latency_ms` in the `/v1/admin/cache/stats` response. **Delivered: `EvaluationResult` carries `p50_latency_ms`, `p95_latency_ms`, `p99_latency_ms`; `getStatus()` JSON includes `latency.p50_ms`, `latency.p95_ms`, `latency.p99_ms` and per-tier breakdowns.**

---

### In-Process Replication Coordinator: Network-Backed Peer Discovery
**Priority:** Medium
**Target Version:** v1.8.0

`cache_replication_coordinator.cpp` uses an in-process `ReplicationBus` where peers are registered via direct pointer sharing (line 73: `for (auto* peer : bus_->peers)`). This only works within a single process. Cross-node cache invalidation (required for clustered deployments) is not implemented.

**Implementation Notes:**
- `[x]` Define a `IRemoteCachePeer` interface with `invalidate(key)` and `invalidateTenant(tenant_id)` methods.
- `[x]` Implement `GrpcRemoteCachePeer` backed by the existing gRPC transport in `src/network/grpc_transport.cpp`.
- `[x]` `CacheReplicationCoordinator` holds a `std::vector<IRemoteCachePeer*>`; populate from cluster membership (Raft or gossip) via a `ClusterView` injection.
- `[x]` Fanout invalidation to remote peers asynchronously (fire-and-forget with a bounded retry queue); do not block `put()` on remote acknowledgment.

---

### Warmup: Parallel Bulk Load
**Priority:** Low
**Target Version:** v1.8.0

`warmup.cpp` (`warmupFromLog`) processes warmup entries sequentially — one line at a time — which limits warmup throughput on startup when the log file has millions of entries.

**Implementation Notes:**
- `[x]` Partition the NDJSON warmup log into N chunks (one per CPU core) and spawn N `std::async` tasks to parse and insert in parallel; use per-shard L1 insertion to avoid contention.
- `[x]` Add `WarmupConfig::max_parallel_workers` (default: `std::thread::hardware_concurrency()`).
- `[x]` Report `warmup_duration_ms` and `warmup_entries_per_second` in the warmup result JSON.

**Performance Targets:**
- Warmup throughput: ≥ 500 K entries/s on a 4-core machine for a 5 M entry log.

---

## Test Strategy

- **Unit tests**: `get()` lock-free L1 path under 16-thread TSAN-enabled stress; circuit breaker state transitions; per-tenant quota enforcement.
- **Integration tests**: Redis coordinator reconnect after forced disconnect; cross-node invalidation via mock gRPC peers.
- **Performance benchmarks**: `get()` throughput before/after lock-free L1 (target ≥ 3× improvement); warmup bulk-load throughput.
- **SLO tests**: inject artificial L3 latency; verify latency-percentile SLO alert fires within one evaluation window.

## Performance Targets

- L1 hit path: ≥ 5 M ops/s per core under 16-thread contention (post lock-free migration).
- L2 hit path: ≥ 500 K ops/s (ZSTD decompress included).
- L3 hit path: ≤ 5 ms p99 under normal RocksDB load.
- Warmup: ≥ 500 K entries/s with parallel bulk load.
- Admin API endpoints: ≤ 5 ms response regardless of L1 cache size.

---

*Last Updated: 2026-03-22*
*Module Version: v1.9.0*

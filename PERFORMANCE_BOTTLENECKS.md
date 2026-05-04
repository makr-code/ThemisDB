# ThemisDB — Performance Bottlenecks & Optimization Potentials

**Version:** 1.0  
**Date:** 2026-05-04  
**Scope:** Systematic analysis of the ThemisDB C++ source tree for lock contention,
algorithmic inefficiencies, I/O overhead, string overhead, container misuse, and
stubs masquerading as functional code on critical paths.

---

## Legend

| Severity | Meaning |
|----------|---------|
| 🔴 **Critical** | Directly causes correctness failure or eliminates all concurrency on a hot path |
| 🟠 **High** | Measurable latency/throughput regression under typical load |
| 🟡 **Medium** | Overhead confined to infrequent paths or bounded cardinality |

---

## Table of Contents

1. [Lock Contention](#1-lock-contention)
2. [Memory Allocation Patterns](#2-memory-allocation-patterns)
3. [I/O Patterns](#3-io-patterns)
4. [String Handling](#4-string-handling)
5. [Container Choices](#5-container-choices)
6. [Algorithmic Inefficiencies](#6-algorithmic-inefficiencies)
7. [Stubs on Critical Paths](#7-stubs-on-critical-paths)
8. [Parallelism Gaps](#8-parallelism-gaps)
9. [Data Races](#9-data-races)
10. [SIMD / CPU Feature Detection](#10-simd--cpu-feature-detection)
11. [Summary Table](#11-summary-table)
12. [Recommended Fix Priority](#12-recommended-fix-priority)

---

## 1. Lock Contention

### F-001 ✅ · `BoundedLRUCache::get()` takes exclusive write-lock for reads
- **File:** `src/cache/bounded_lru_cache.cpp` · ~Line 40
- **Severity:** 🔴 Critical

`get()` acquires `std::unique_lock<std::shared_mutex>` even though the initial
map lookup is read-only. The `moveToFront()` mutation requires write access, but
this serialises **every** cache read under a full exclusive lock, making
concurrent reads impossible.

```cpp
// Every cache read blocks all other readers:
std::unique_lock<std::shared_mutex> lock(mutex_);
auto it = cache_.find(key);  // read-only; doesn't need exclusive lock
```

**Fix:** Acquire `shared_lock` for the lookup; upgrade to `unique_lock` only when
`moveToFront()` is required. Alternatively use an optimistic two-phase pattern.

---

### F-002 ✅ · `AccountLockoutManager::getLockoutInfo()` uses exclusive lock on a pure-read path
- **File:** `src/auth/auth_rate_limiter.cpp` · ~Line 142
- **Severity:** 🟠 High

The method is `const`, performs no mutations, yet takes `unique_lock`:

```cpp
std::optional<LockoutInfo> AccountLockoutManager::getLockoutInfo(
    const std::string& user_id) const
{
    std::unique_lock<std::shared_mutex> lock(mutex_);  // should be shared_lock
```

Every authentication request consults this path. All concurrent auth checks are
fully serialised.

**Fix:** Change to `std::shared_lock<std::shared_mutex>`.

---

### F-003 · `AuthRateLimiter` stats counters increment under an exclusive lock
- **File:** `src/auth/auth_rate_limiter.cpp` · Lines 298, 381, 436, 457, 596
- **Severity:** 🟠 High

`total_requests`, `rejected_requests`, and `total_latency_ms` are incremented
inside `std::unique_lock<std::shared_mutex>` on `stats_mutex_`. These are
write-hot on the critical auth path.

**Fix:** Declare counters as `std::atomic<uint64_t>` / `std::atomic<double>`.
Reserve the mutex solely for mutating the callback vector.

---

### F-004 · `EmbeddingCache::query()` holds single coarse mutex during full HNSW vector search
- **File:** `src/cache/embedding_cache.cpp` · ~Lines 108–200
- **Severity:** 🔴 Critical

```cpp
std::lock_guard<std::mutex> lock(impl_->mutex);

if (impl_->vector_index) {
    // HNSW graph traversal — can take 10s of ms — held under global mutex
    auto [status, results] = impl_->vector_index->searchKnn(query_embedding, 1);
```

Every `query()`, `store()`, and `clearExpired()` call serialises on the same
coarse mutex while the HNSW search (potentially 10–50 ms) is in flight. There is
zero read concurrency.

**Fix:** Separate the metadata-map mutex from the vector-index lock. Use
`shared_mutex` for the entry map; let HNSW's own internal locking guard the
index. After the vector search, acquire a brief lock for the entry map lookup.

---

### F-005 · `IntelligentPrefetcher` uses a single mutex for all operations including hot-path `record_access`
- **File:** `src/performance/intelligent_prefetcher.cpp` · Lines 116–369
- **Severity:** 🟡 Medium

All four public operations (`record_access`, `predict_next_accesses`,
`prefetch_predicted`, `current_pattern`) share one `std::mutex mu_`. On
multi-threaded workloads, every cache-access observation serialises.

**Fix:** Split into independently-locked domains: atomic EMA/confidence fields
updated per access; a coarser mutex only for the batch `prefetch_predicted` path.

---

### F-006 · `NUMAMemoryManager` acquires `stats_mutex_` on every single allocation/deallocation
- **File:** `src/performance/numa_memory_manager.cpp` · Lines 126, 187
- **Severity:** 🟡 Medium

`update_alloc_stats()` takes `stats_mutex_` on every `allocate_on_node()` call.
If the NUMA allocator is used as a hot-path memory manager, this is a scalability
bottleneck.

**Fix:** Use `thread_local` per-node counters; flush to the central stats struct
periodically.

---

### F-007 · `SseConnectionManager::backgroundPollTask()` holds an exclusive write-lock for the entire poll loop
- **File:** `src/server/sse_connection_manager.cpp` · ~Lines 258–290
- **Severity:** 🟠 High

`backgroundPollTask()` takes `unique_lock<shared_mutex>` at the top of its loop,
then iterates over all active connections and calls the changefeed for each one
while holding the lock. This means:
- New SSE connections (`addConnection`) are blocked for the full polling interval.
- All `broadcastEvent` callers are serialised.

```cpp
std::unique_lock<std::shared_mutex> lock(connections_mutex_);
for (auto& [id, conn] : connections_) {
    // ...changefeed.list() — potentially slow I/O — held under exclusive lock
}
```

**Fix:** Copy the connection list under the lock, then release it before making
any I/O calls. Apply events and sequence updates under a brief re-lock per
connection.

---

### F-008 · `TokenBucketRateLimiter` serialises all Redis calls through a single mutex
- **File:** `src/server/rate_limiter_v2.cpp` · Lines 116, 221, 301, 313
- **Severity:** 🟠 High

A single `redis_mutex_` guards every interaction with the Redis connection,
including the hot-path `checkAndConsume()`. Every concurrent rate-limit check
blocks on this lock even though the Redis Lua script is fully atomic on the
server side.

**Fix:** Use a connection pool (`hiredis-cluster` or a manual pool of
`redisContext*`). Each thread draws a connection, performs the EVALSHA call, and
returns it — allowing true concurrency against Redis.

---

## 2. Memory Allocation Patterns

### F-009 · `EmbeddingCache::store()` O(N) linear eviction scan
- **File:** `src/cache/embedding_cache.cpp` · ~Lines 243–261
- **Severity:** 🟠 High

```cpp
// O(N) scan every time the cache is full
for (auto it = impl_->entries.begin(); it != impl_->entries.end(); ++it) {
    if (it->second.timestamp_ms < oldest_time) {
        oldest_time = it->second.timestamp_ms;
        oldest_it   = it;
    }
}
```

With `max_entries` in the thousands, every `store()` when the cache is full
performs an O(N) scan while holding the global mutex.

**Fix:** Maintain a `std::priority_queue<{timestamp_ms, pk}>` (min-heap) for
O(log N) eviction, or use a doubly-linked LRU list (same pattern as
`BoundedLRUCache`).

---

### F-010 · `MVCC::getAtTimestamp()` allocates a fresh RocksDB iterator on every point-read
- **File:** `src/storage/mvcc_store.cpp` · ~Lines 158–176
- **Severity:** 🟠 High

```cpp
auto iter_result = db_->newSafeIterator();
```

Creating a RocksDB iterator involves a heap allocation and a snapshot
registration. For OLTP point-lookup workloads (`getLatest()` on the common path)
this is unnecessary overhead compared to a direct `db_->Get()`.

**Fix:** Add a `db_->Get()` fast path in `getLatest()` for the common case where
the key has only one version. Fall back to iterator-seek only for time-travel
reads. Alternatively, maintain a per-thread iterator pool.

---

### F-011 · `NUMAMemoryManager` bucket deallocation is O(bucket_depth) linear scan
- **File:** `src/performance/numa_memory_manager.cpp` · ~Lines 151–158
- **Severity:** 🟡 Medium

```cpp
// O(N) scan per deallocation within each hash bucket
for (auto it = v.begin(); it != v.end(); ++it) {
    if (it->first == ptr) { ... v.erase(it); return true; }
}
```

**Fix:** Replace the `std::vector<std::pair<void*,AllocInfo>>` bucket with
`std::unordered_map<void*, AllocInfo>` for O(1) amortised lookup.

---

## 3. I/O Patterns

### F-012 ✅ · `WALStorage::appendEntryLocked()` makes 4 separate `write()` syscalls per record
- **File:** `src/storage/wal_storage.cpp` · ~Lines 403–406
- **Severity:** 🟠 High

```cpp
write_all_fd(fd_, hdr, HEADER_SIZE)    // syscall 1
write_all_fd(fd_, key.data(), klen)    // syscall 2
write_all_fd(fd_, value.data(), vlen)  // syscall 3
write_all_fd(fd_, crc_buf, 4)          // syscall 4
```

At ~100–300 ns per `write()` kernel transition, high write-rate workloads pay
400–1 200 ns of pure syscall overhead per WAL record.

**Fix:** Assemble the full record into a stack/arena buffer and issue a single
`write_all_fd()`. For larger records use `writev()` (scatter-gather) to avoid
copying.

---

### F-013 · `SemanticCache::getStats()` performs a full RocksDB key scan on every call
- **File:** `src/cache/semantic_cache.cpp` · ~Lines 196–213
- **Severity:** 🟠 High

```cpp
// Full column-family scan on every stats request:
for (it->SeekToFirst(); it->Valid(); it->Next()) {
    count++;
    size += it->key().size() + it->value().size();
}
```

On a cache with thousands of entries this takes milliseconds and generates
significant read I/O.

**Fix:** Maintain `entry_count_` and `total_bytes_` as `std::atomic<uint64_t>`,
updated on every `put()` and TTL expiry. `getStats()` simply reads the atomics.

---

### F-014 · `SemanticCache::clearExpired()` full O(N) expiry sweep — no background task
- **File:** `src/cache/semantic_cache.cpp` · ~Lines 213–252
- **Severity:** 🟡 Medium

`clearExpired()` must be called explicitly; it scans and deserialises every
JSON entry in the column family. TTL entries linger until a manual sweep is
triggered.

**Fix:** Store the expiry timestamp as a key prefix to enable a range-scan.
Alternatively, use RocksDB's built-in TTL column-family (`rocksdb::DBWithTTL`)
to eliminate manual expiry entirely.

---

## 4. String Handling

### F-015 ✅ · `SemanticCache::computeKey()` uses `std::ostringstream` in a 32-iteration hex loop
- **File:** `src/cache/semantic_cache.cpp` · ~Lines 73–82
- **Severity:** 🟠 High

```cpp
std::string input = prompt + params.dump();  // 2 heap allocs + copy
// ...SHA256(...)
std::ostringstream oss;
for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i]; // 32 writes
}
return oss.str();
```

Every cache lookup rebuilds the key from scratch, concatenates `params.dump()`,
and formats the 32-byte hash via 32 `operator<<` calls on a heap-allocated
stream.

**Fix:** Replace the `ostringstream` hex loop with a static nibble-lookup table
and direct writes into a `char[64]`:

```cpp
static constexpr char HEX[] = "0123456789abcdef";
char result[64];
for (int i = 0; i < 32; ++i) {
    result[2*i]   = HEX[hash[i] >> 4];
    result[2*i+1] = HEX[hash[i] & 0xf];
}
return std::string(result, 64);
```

---

### F-016 ✅ · Same `ostringstream` SHA-256 pattern in `AdaptiveQueryCache::computeFingerprint()`
- **File:** `src/cache/adaptive_query_cache.cpp` · ~Lines 184–189
- **Severity:** 🟠 High

Identical problem; this runs on every cache `get()`, including cache-hit paths.

**Fix:** Same as F-015.

---

### F-017 · `ConsistentHashRing::addShard()` constructs `std::ostringstream` inside virtual-node loop
- **File:** `src/sharding/consistent_hash.cpp` · ~Lines 60–64
- **Severity:** 🟡 Medium

```cpp
for (size_t i = 0; i < virtual_nodes; ++i) {
    std::ostringstream oss;          // constructed + destroyed per iteration
    oss << shard_id << "#" << i;
    uint64_t token = hash(oss.str());
```

150+ stream objects are created/destroyed per `addShard()` call.

**Fix:** Use a pre-reserved `std::string` with `std::to_string` + `+` outside a
single stream, or `fmt::format_to` into a stack buffer.

---

### F-018 · Widespread `std::ostringstream` usage in RAG evaluation pipeline
- **File:** Multiple files in `src/rag/` (≥ 20 files)
- **Severity:** 🟡 Medium

At least 20 RAG evaluation files use `std::ostringstream` for prompt and
report assembly. While these paths are not as hot as the cache key paths,
they add measurable heap pressure during batch evaluation runs.

**Fix:** Prefer `std::string::reserve()` + `+=` / `append()` for simple
concatenation. Use `fmt::format` / `std::format` (C++20) for structured output.

---

## 5. Container Choices

### F-019 · `ConsistentHashRing::getSuccessors()` uses `std::set<std::string>` for deduplication
- **File:** `src/sharding/consistent_hash.cpp` · ~Lines 144, 211
- **Severity:** 🟡 Medium

```cpp
std::set<std::string> seen;  // tree-based, O(log n) per insert + heap alloc per string
```

For typical shard counts (< 32), a `std::unordered_set<std::string>` gives O(1)
average insert, or a `std::vector<std::string>` with linear scan beats both for
n < 10.

**Fix:** Use `std::unordered_set<std::string>`.

---

### F-020 · `HnswLayerOptimizer` uses `std::map<int, …>` for per-layer stats rebuilt on every search
- **File:** `src/index/hnsw_layer_optimizer.cpp` · ~Lines 83, 118
- **Severity:** 🟡 Medium

```cpp
std::map<int, std::pair<double, int>> entry_layer_performance;  // per-search rebuild
std::map<int, LayerStats> layer_stats_;                         // class member
```

HNSW graphs have ≤ 20 layers. `std::unordered_map<int,…>` gives O(1) access.
A `std::array<LayerStats, 32>` (indexed by layer id) is even faster.

**Fix:** Replace with `std::unordered_map<int,…>` or `std::array<LayerStats,32>`.

---

## 6. Algorithmic Inefficiencies

### F-021 · `EmbeddingCache` brute-force fallback exits on first element above threshold, not the best match
- **File:** `src/cache/embedding_cache.cpp` · ~Lines 170–200
- **Severity:** 🟡 Medium

```cpp
if (best_similarity >= EARLY_TERMINATION_THRESHOLD) {
    break;  // stops after first hit — does not guarantee global best
}
```

The loop is unordered; early termination at 0.99 does not guarantee the
globally best match is returned.

**Fix:** Either pre-sort candidates by a proxy score and apply early termination
correctly, or remove the `break` and keep a running maximum throughout the full
scan.

---

### F-022 ✅ · `GorillaSIMDDecoder` calls `gorilla_simd_has_avx2()` (full CPUID instruction) on every decode invocation
- **File:** `src/timeseries/gorilla_simd.cpp` · ~Lines 248–262
- **Severity:** 🟡 Medium

```cpp
if (!gorilla_simd_has_avx2()) {   // executes CPUID on every decode call
    GorillaDecoder fallback(data_);
```

`gorilla_simd_has_avx2()` executes a `CPUID` instruction on every call.
Although CPUID takes only ~20–100 ns and the CPU result never changes at
runtime, this is unnecessary work on every hot decode path.

**Fix:** Cache the result in a `static const bool` or `std::once_flag`-guarded
variable:

```cpp
static const bool kHasAVX2 = gorilla_simd_has_avx2();
if (!kHasAVX2) { ...
```

---

## 7. Stubs on Critical Paths

### F-023 · `OptimizerCostModel` table statistics are stubs returning zero row counts
- **File:** `src/query/optimizer_cost_model.cpp` · ~Lines 580–634
- **Severity:** 🔴 Critical

```cpp
// STUB/SIMULATION NOTE:
// Purpose: Creates an empty placeholder entry so that getTableStatistics()
//   always returns a well-initialised struct ... even before real stats are injected.
// Production Delta: A production implementation would query the storage ...
```

The query optimizer uses these statistics to choose join order, index selection,
and predicate push-down. With zero statistics, the optimizer always uses
worst-case cardinality estimates, potentially choosing full collection scans over
index lookups for every query.

**Fix:** Implement `refreshAllStatistics()` to sample RocksDB key counts via
`GetApproximateKeyCount()` or HyperLogLog sketches. Schedule background refresh.

---

### F-024 · `DefaultExpressionEvaluator` always returns `true` — all WHERE-clause predicates pass
- **File:** `src/storage/storage_engine.cpp` · ~Lines 67–86
- **Severity:** 🔴 Critical

```cpp
bool evaluate(const std::string& expression, ...) override {
    // Default implementation: always return true (no filtering)
    return true;
}
```

`createDefault()` is used by `QueryEngine::createDefault()`. Any query path that
reaches `createDefault()` skips all filter predicates and returns a full
collection scan as its result, silently.

**Fix:** `createDefault()` should throw `std::logic_error` if a non-empty
expression is passed. Make the no-filter evaluator opt-in.

---

### F-025 · Cross-shard joins (`QueryFederation::broadcast` / `shuffle`) return empty data arrays
- **File:** `src/sharding/adaptive_shard_router.cpp`, `src/query/query_federation.cpp` · ~Lines 369–395
- **Severity:** 🔴 Critical

```cpp
// STUB/SIMULATION NOTE:
// Production Delta: No actual data is returned (`result.data` is always an
//                   empty JSON array); `execution_time_ms = 0` suppresses
//                   latency accounting.
```

Any query that spans multiple shards silently returns zero results. No error or
warning is surfaced to the caller.

**Fix:** Return a proper error `Result<>` with `ERR_NOT_IMPLEMENTED` instead of
silently returning empty data. This prevents silent correctness failures in
production.

---

### F-026 · `fulltext_functions.cpp` — all full-text search functions are unconnected placeholders
- **File:** `src/query/functions/fulltext_functions.cpp` · (file-level comment)
- **Severity:** 🟠 High

The file header states:

```
// Most functions are placeholders that need to be wired to the SecondaryIndexManager.
```

AQL queries using `FULLTEXT()`, `PHRASE()`, `TOKENS()` etc. operate on
in-memory fallbacks rather than the actual secondary index. Results may be
incomplete or incorrect for any non-trivial full-text query.

**Fix:** Wire each function registration to `SecondaryIndexManager::search()` /
`SecondaryIndexManager::phraseSearch()`.

---

### F-027 · `process_mining_functions.cpp` — most functions immediately return `"not implemented"`
- **File:** `src/query/functions/process_mining_functions.cpp` · Lines 29–36
- **Severity:** 🟠 High

```cpp
json makeError(const std::string& msg) { ... }
// Line 36:
return makeError(name + " not implemented");
```

A catch-all registration path returns `"not implemented"` for most
process-mining AQL functions. These are surfaced as valid JSON results rather
than errors, making it hard for callers to detect that processing did not occur.

**Fix:** Return a proper AQL error code (not a JSON field). Implement the highest-priority
functions (`PM_DISCOVER_PROCESS`, conformance check, variant analysis) in
production code.

---

### F-028 · `ethics_functions.cpp` — multiple query functions return empty arrays as placeholders
- **File:** `src/query/functions/ethics_functions.cpp` · Lines 163, 190, 214
- **Severity:** 🟡 Medium

```cpp
// Line 163: Return empty array as placeholder until collection is populated
// Line 190: Return empty array as placeholder
// Line 214: Return empty array as placeholder
```

AQL queries invoking ethics-school listing, bias detection, or argument
decomposition functions silently receive empty arrays.

**Fix:** Either raise an AQL error (so callers know the function is unimplemented)
or connect to the `EthicsProfileRegistry` / `DiscourseMemoryStore` that are
already implemented.

---

## 8. Parallelism Gaps

### F-029 · `MultiStepRAGOrchestrator::runMapReduce()` map phase is fully sequential
- **File:** `src/rag/multi_step_rag.cpp` · ~Lines 237–248
- **Severity:** 🟠 High

```cpp
// Map phase — all batches are independent but processed sequentially:
for (const auto& batch : batches) {
    const std::string map_prompt = buildMapPrompt(batch, query);
    std::string partial = infer(map_prompt, map_max_tok);  // blocking call
    result.steps.push_back(partial);
}
```

With 3 batches and ~500 ms per inference call, the map phase takes 1.5 s instead
of 0.5 s.

**Fix:** Dispatch map tasks with `std::async(std::launch::async, ...)` and
collect futures before the reduce step. Limit concurrency with a semaphore to
avoid saturating the LLM endpoint.

---

## 9. Data Races

### F-030 ✅ · `SemanticCache` stats counters are bare non-atomic integers with no synchronisation
- **File:** `src/cache/semantic_cache.cpp` · ~Lines 152–172
- **Severity:** 🔴 Critical

```cpp
// No mutex anywhere — bare updates from concurrent threads:
miss_count_++;                          // UB: data race
hit_count_++;                           // UB: data race
total_query_latency_ms_ += latency_ms;  // UB: non-atomic double accumulation
```

Concurrent `put()` / `query()` calls produce undefined behaviour. The `double`
accumulation is particularly dangerous.

**Fix:** Declare all counters as `std::atomic<uint64_t>`. For `double`, use an
atomic integer in microseconds or a `std::mutex`-guarded stats struct.

---

### F-031 ✅ · `WALStorage::crc32_update()` double-checked initialisation uses a plain (non-atomic) `bool`
- **File:** `src/storage/wal_storage.cpp` · ~Lines 103–118
- **Severity:** 🟠 High

```cpp
static uint32_t table[256];
static bool initialized = false;   // not atomic
if (!initialized) {                // two threads can both see false
    for (uint32_t i = 0; i < 256; ++i) { ... }
    initialized = true;
}
```

Two threads calling `appendEntry` concurrently before the first write completes
will both initialise the table; the second will observe a partially-initialised
array. This is undefined behaviour in C++11+.

**Fix:** Replace with a `constexpr`-initialised table, or use C++11 guaranteed
thread-safe static local initialisation:

```cpp
static const auto& table = [] {
    static uint32_t t[256]; /* populate */ return t;
}();
```

---

## 10. SIMD / CPU Feature Detection

### F-022 (details in §6 above)

`gorilla_simd_has_avx2()` re-executes `CPUID` on every decode call. Cache with
a static `bool`.

---

## 11. Summary Table

| ID | File(s) | Category | Severity | Status |
|----|---------|----------|----------|--------|
| F-001 | `cache/bounded_lru_cache.cpp:40` | Lock Contention | 🔴 Critical | ✅ Fixed |
| F-002 | `auth/auth_rate_limiter.cpp:142` | Lock Contention | 🟠 High | ✅ Fixed |
| F-003 | `auth/auth_rate_limiter.cpp:298–596` | Lock Contention | 🟠 High | Open |
| F-004 | `cache/embedding_cache.cpp:108` | Lock Contention | 🔴 Critical | Open |
| F-005 | `performance/intelligent_prefetcher.cpp:116–369` | Lock Contention | 🟡 Medium | Open |
| F-006 | `performance/numa_memory_manager.cpp:126` | Lock Contention | 🟡 Medium | Open |
| F-007 | `server/sse_connection_manager.cpp:258` | Lock Contention / I/O | 🟠 High | Open |
| F-008 | `server/rate_limiter_v2.cpp:116–313` | Lock Contention | 🟠 High | Open |
| F-009 | `cache/embedding_cache.cpp:243` | Algorithmic / Memory | 🟠 High | Open |
| F-010 | `storage/mvcc_store.cpp:158` | Memory / I/O | 🟠 High | Open |
| F-011 | `performance/numa_memory_manager.cpp:151` | Container / Memory | 🟡 Medium | Open |
| F-012 | `storage/wal_storage.cpp:403–406` | I/O | 🟠 High | ✅ Fixed |
| F-013 | `cache/semantic_cache.cpp:196–213` | I/O / Algorithmic | 🟠 High | Open |
| F-014 | `cache/semantic_cache.cpp:213–252` | I/O / Algorithmic | 🟡 Medium | Open |
| F-015 | `cache/semantic_cache.cpp:73–82` | String / Memory | 🟠 High | ✅ Fixed |
| F-016 | `cache/adaptive_query_cache.cpp:184–189` | String / Memory | 🟠 High | ✅ Fixed |
| F-017 | `sharding/consistent_hash.cpp:60–64` | String / Memory | 🟡 Medium | Open |
| F-018 | `rag/*.cpp` (≥ 20 files) | String / Memory | 🟡 Medium | Open |
| F-019 | `sharding/consistent_hash.cpp:144,211` | Container | 🟡 Medium | Open |
| F-020 | `index/hnsw_layer_optimizer.cpp:83,118` | Container | 🟡 Medium | Open |
| F-021 | `cache/embedding_cache.cpp:170–200` | Algorithmic | 🟡 Medium | Open |
| F-022 | `timeseries/gorilla_simd.cpp:248` | CPU / SIMD | 🟡 Medium | ✅ Fixed |
| F-023 | `query/optimizer_cost_model.cpp:580–634` | Stub-in-critical-path | 🔴 Critical | Open |
| F-024 | `storage/storage_engine.cpp:67–86` | Stub-in-critical-path | 🔴 Critical | Open |
| F-025 | `sharding/adaptive_shard_router.cpp`, `query/query_federation.cpp:369–395` | Stub-in-critical-path | 🔴 Critical | Open |
| F-026 | `query/functions/fulltext_functions.cpp` | Stub-in-critical-path | 🟠 High | Open |
| F-027 | `query/functions/process_mining_functions.cpp:36` | Stub-in-critical-path | 🟠 High | Open |
| F-028 | `query/functions/ethics_functions.cpp:163,190,214` | Stub-in-critical-path | 🟡 Medium | Open |
| F-029 | `rag/multi_step_rag.cpp:237–248` | Parallelism | 🟠 High | Open |
| F-030 | `cache/semantic_cache.cpp:152–172` | Data Race | 🔴 Critical | ✅ Fixed |
| F-031 | `storage/wal_storage.cpp:103–118` | Data Race | 🟠 High | ✅ Fixed |

**Total:** 31 findings — 7 Critical, 14 High, 10 Medium  
**Fixed (this PR):** 7 (F-001, F-002, F-012, F-015, F-016, F-022, F-030, F-031)

---

## 12. Recommended Fix Priority

### Tier 1 — Fix immediately (correctness / UB)

| # | Item | Reason | Status |
|---|------|--------|--------|
| F-030 | SemanticCache data race on stats | Undefined behaviour in production | ✅ Fixed |
| F-031 | WAL CRC32 initialisation data race | UB; can corrupt WAL records | ✅ Fixed |
| F-024 | DefaultExpressionEvaluator always-true | Silent wrong query results | Open |
| F-025 | Cross-shard join stub returns empty data | Silent wrong results on distributed queries | Open |
| F-023 | Cost model statistics stubs | Causes suboptimal query plans for every query | Open |

### Tier 2 — High-impact, low-effort wins

| # | Item | Effort | Status |
|---|------|--------|--------|
| F-001 | `BoundedLRUCache::get()` shared_lock | 2-line change | ✅ Fixed |
| F-002 | `getLockoutInfo()` shared_lock | 1-line change | ✅ Fixed |
| F-003 | Auth stats → `std::atomic` | 3-line change | Open |
| F-015 | SHA-256 hex formatting via table | ~10-line change | ✅ Fixed |
| F-016 | Same fix in `AdaptiveQueryCache` | ~10-line change | ✅ Fixed |
| F-022 | Cache `gorilla_simd_has_avx2()` result | 1-line change | ✅ Fixed |
| F-012 | WAL: single write per record (buffer+1 syscall) | ~20-line change | ✅ Fixed |

### Tier 3 — Structural improvements

| # | Item |
|---|------|
| F-004 | Decouple EmbeddingCache mutex from HNSW index |
| F-007 | SSE background poll — copy connections before I/O |
| F-008 | Redis connection pool for rate limiter |
| F-009 | EmbeddingCache LRU/heap eviction |
| F-010 | MVCC fast path via `db_->Get()` |
| F-013 | SemanticCache atomic counters for `getStats()` |
| F-029 | Parallel map phase in `MultiStepRAGOrchestrator` |

### Tier 4 — Clean up stubs (required for full functionality)

| # | Item |
|---|------|
| F-026 | Wire `fulltext_functions` to `SecondaryIndexManager` |
| F-027 | Implement or error-out `process_mining_functions` |
| F-028 | Wire `ethics_functions` to `EthicsProfileRegistry` |
| F-014 | Replace SemanticCache manual TTL with RocksDB TTL CF |

---

*Generated by systematic static analysis of ThemisDB source tree (commit `c332cdd5a7`, 2026-05-04). Line numbers are approximate and should be verified against HEAD.*

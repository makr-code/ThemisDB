# ThemisDB Performance Optimization Action Plan
**Generated:** 2026-05-10  
**Based on:** Full Performance Logging Session + PERFORMANCE_BOTTLENECKS.md Analysis  
**Scope:** 35 Benchmark Tests, 5 Suites, Hyperscaler Edition

---

## Executive Summary

Performance logging completed across all benchmark suites reveals:
- ✅ **35/35 tests validated** (29 passed + 14 executed)
- ✅ **All SLOs met** (Scheduler, Wire, Ethics, Allocator)
- ✅ **Inference scaling measured** (1-4 threads: 2.21x improvement)
- ⚠️ **3 critical lock contention issues identified** (cache, auth, vector search)

---

## Security-/Audit-Guardrails für Optimierungen

Alle Maßnahmen in diesem Plan sind an folgende Root-Leitplanken gebunden:

1. Keine Optimierung darf bestehende Sicherheitskontrollen (RBAC, Audit-Logging, Transportschutz) deaktivieren oder abschwächen.
2. Änderungen mit Sicherheitsbezug müssen gegen `SECURITY.md` und `audit/AUDIT.md` nachvollziehbar bleiben.
3. Validierung erfolgt nicht nur über Benchmarks, sondern auch über die dokumentierten Verifikationspfade in `CTEST.md`.
4. Zielwerte müssen mit `PERFORMANCE_EXPECTATIONS.md` und den Bottleneck-Funden in `PERFORMANCE_BOTTLENECKS.md` konsistent bleiben.

---

## Optimization Roadmap

### Phase 1: Lock Contention Elimination (P0 — Week 1)

#### Issue 1.1: BoundedLRUCache Read Serialization 🔴 CRITICAL
**File:** `src/cache/bounded_lru_cache.cpp` ~Line 40  
**Impact:** Cache read throughput severely limited by exclusive lock  
**Current:** All `get()` calls take `unique_lock` even for read-only lookup  

```cpp
// BEFORE (Current - Wrong)
std::unique_lock<std::shared_mutex> lock(mutex_);
auto it = cache_.find(key);  // read-only; blocks all other readers
if (it != cache_.end()) {
    moveToFront(it);  // mutation requires write lock
}

// AFTER (Proposed)
{
    std::shared_lock<std::shared_mutex> read_lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // Found; need to promote to write lock for moveToFront
        read_lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(mutex_);
        // Recheck presence after lock release/reacquire
        auto it2 = cache_.find(key);
        if (it2 != cache_.end()) {
            moveToFront(it2);
            return it2->second;
        }
        // Raced out; return not found
        return {};
    }
}
// Not found; return nullopt
return {};
```

**Estimated Impact:** 3-5x improvement in cache read throughput under concurrent load  
**Test Strategy:** Add concurrency stress test (100+ reader threads vs 1 writer)  
**Target:** v1.10.0 (Q3 2026)

---

#### Issue 1.2: AuthRateLimiter Exclusive Lock on Read Path 🔴 CRITICAL
**File:** `src/auth/auth_rate_limiter.cpp` ~Line 142  
**Impact:** Every authentication request serializes on lock  
**Current:** `getLockoutInfo()` (const method) uses `unique_lock`

```cpp
// BEFORE
std::optional<LockoutInfo> AccountLockoutManager::getLockoutInfo(
    const std::string& user_id) const
{
    std::unique_lock<std::shared_mutex> lock(mutex_);  // ❌ WRONG
    // ... read-only operations ...
}

// AFTER
std::optional<LockoutInfo> AccountLockoutManager::getLockoutInfo(
    const std::string& user_id) const
{
    std::shared_lock<std::shared_mutex> read_lock(mutex_);  // ✅ CORRECT
    // ... read-only operations ...
}
```

**Estimated Impact:** 10-15x improvement in auth throughput  
**Test Strategy:** LoadTest with 1000+ concurrent auth checks  
**Target:** v1.10.0 (Q3 2026)

---

#### Issue 1.3: AuthRateLimiter Stats Under Exclusive Lock 🟠 HIGH
**File:** `src/auth/auth_rate_limiter.cpp` Lines 298, 381, 436, 457, 596  
**Impact:** Write-hot counters serialize all auth operations  
**Current:** `total_requests++`, `rejected_requests++` under `unique_lock`

```cpp
// BEFORE
{
    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    total_requests++;  // ❌ Serializes every auth
    rejected_requests++;
    total_latency_ms += elapsed;
}

// AFTER
std::atomic_fetch_add(&total_requests, 1UL);  // ✅ Lock-free
std::atomic_fetch_add(&rejected_requests, 1UL);
std::atomic_fetch_add((std::atomic<uint64_t>*)&total_latency_ns, elapsed_ns);
```

**Estimated Impact:** 2-3x improvement in auth latency (remove critical section)  
**Test Strategy:** Latency histogram comparison (before/after)  
**Target:** v1.10.0 (Q3 2026)

---

### Phase 2: Vector Search Lock Separation (P1 — Week 2)

#### Issue 2.1: EmbeddingCache Coarse Mutex During HNSW Search 🔴 CRITICAL
**File:** `src/cache/embedding_cache.cpp` ~Lines 108–200  
**Impact:** 10-50ms HNSW search holds global mutex → zero read concurrency  
**Current:** Single coarse `std::mutex` guards both metadata and vector index

```cpp
// BEFORE (Current - Wrong)
std::lock_guard<std::mutex> lock(impl_->mutex);
if (impl_->vector_index) {
    // HNSW search under global lock - 10-50ms hold time!
    auto [status, results] = impl_->vector_index->searchKnn(query_embedding, 1);
    // ...
}

// AFTER (Proposed - Two-phase locking)
std::shared_ptr<VectorIndex> vector_index;
{
    std::shared_lock<std::shared_mutex> meta_lock(impl_->metadata_mutex);
    vector_index = impl_->vector_index;  // Copy shared_ptr (cheap)
}
// Release metadata_lock; now perform long-running search without lock
auto [status, results] = vector_index->searchKnn(query_embedding, 1);
{
    std::lock_guard<std::shared_mutex> meta_lock(impl_->metadata_mutex);
    // Update results cache atomically
}
```

**Estimated Impact:** 5-10x improvement in vector search concurrency  
**Test Strategy:** Concurrent query benchmark (100+ threads)  
**Target:** v1.10.0 (Q3 2026)

---

### Phase 3: String Overhead Reduction (P2 — Week 3)

#### Issue 3.1: String Copies in Protocol Handlers 🟡 MEDIUM
**File:** `src/network/wire_protocol_v2.cpp` ~Lines 450–520  
**Impact:** Repeated string allocations on hot path  
**Current:** `std::string` copies in deserialize/serialize loops

**Fix Strategy:**
1. Use `std::string_view` for read-only string parameters
2. Pre-allocate buffers with reserve()
3. Use move semantics where lifetime allows

**Estimated Impact:** 5-10% throughput improvement  
**Test Strategy:** Wire protocol benchmark (throughput delta)  
**Target:** v1.10.0 (Q3 2026)

---

### Phase 4: Container Misuse Fixes (P2 — Week 3)

#### Issue 4.1: std::vector for Frequent Deletions 🟡 MEDIUM
**File:** `src/graph/constraint_manager.cpp` ~Lines 180–220  
**Impact:** O(n) deletion cost in constraint filtering  
**Current:** Uses `std::vector` + linear search for removal

```cpp
// BEFORE
std::vector<Constraint> active_constraints;
// ... frequently remove constraints ...
auto it = std::find(active_constraints.begin(), active_constraints.end(), constraint_id);
if (it != active_constraints.end()) {
    active_constraints.erase(it);  // ❌ O(n) operation
}

// AFTER
std::unordered_set<ConstraintId> active_constraint_ids;
// ... frequently remove constraints ...
active_constraint_ids.erase(constraint_id);  // ✅ O(1) operation
```

**Estimated Impact:** 20-50% improvement in constraint filtering  
**Test Strategy:** Graph query benchmark with many constraints  
**Target:** v1.10.0 (Q3 2026)

---

## Implementation Schedule

```
Week 1 (May 10-17):     Phase 1: Lock contention fixes (3 critical issues)
Week 2 (May 17-24):     Phase 2: Vector search lock separation  
Week 3 (May 24-31):     Phase 3 & 4: String / Container optimizations
Week 4 (May 31-Jun 7):  Integration testing + regression validation
Week 5 (Jun 7-14):      Performance benchmarking + documentation
Target Release:         v1.10.0 (Q3 2026)
```

---

## Validation Strategy

For each optimization:

1. **Unit Tests:** Lock-free atomics, shared_lock upgrades
2. **Concurrency Tests:** 100+ concurrent threads
3. **Benchmark Before/After:** Measure improvement percentage
4. **Regression Tests:** Ensure no functionality loss
5. **CI/CD Gates:** Performance regression thresholds

---

## Estimated Performance Gains

| Optimization | Current | Target | Gain |
|---|---|---|---|
| Cache read throughput | ~100K ops/s | ~300K ops/s | **3x** |
| Auth throughput | ~5K req/s | ~50K req/s | **10x** |
| Vector search concurrency | 0 (serialized) | Full | **5-10x** |
| Protocol throughput | 1M msg/s | 1.05M msg/s | **5-10%** |
| Graph constraint filtering | 100K ops/s | 150K ops/s | **20-50%** |

**Aggregate Estimated Impact:** 2-3x system-level throughput improvement

---

## Post-Optimization Validation

After all Phase 1-4 fixes:

1. **Re-run benchmark suite** (same 35 tests)
2. **Compare metrics** vs. current report
3. **Validate SLOs** still met (should exceed)
4. **Generate performance delta report**
5. **Document all optimizations** in ROADMAP.md

---

## References

- `PERFORMANCE_BOTTLENECKS.md` - Detailed issue analysis
- `PERFORMANCE_EXPECTATIONS.md` - SLO definitions
- `BENCHMARK_IMPLEMENTATION_REPORT.md` - Current metrics

---

**Status:** Ready for implementation  
**Owner:** Platform Performance Team  
**Review Date:** 2026-05-17 (one week sprint review)

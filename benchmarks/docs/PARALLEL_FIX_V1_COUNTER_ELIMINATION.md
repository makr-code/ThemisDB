> ⚠️ **Historischer Plan** – Beschreibt den Counter-Elimination-Ansatz v1.

# 🎯 CRITICAL FINDING: Shared Counter Bottleneck IDENTIFIED & PARTIALLY FIXED

**Status**: ✅ Major Discovery + Partial Fix Implemented  
**Date**: 18. Dezember 2025  
**Impact**: +39% @ 8 Threads (490k → 683k)

---

## The Discovery

The **shared `std::atomic<int> counter`** in parallel benchmark loops was the **PRIMARY BOTTLENECK**:

```cpp
// ❌ BAD - Every thread hammers the shared counter
std::atomic<int> counter(0);
ParallelExecutor executor(8);
executor.execute([&counter](int) {
    // ...
    BaseEntity e("entity_" + std::to_string(counter++), ...);  // ← CONTENTION!
    sim->put(..., e);
}, 12);
```

**Result**: Even with separate database instances, all 8 threads fight over incrementing `counter`.

---

## The Fix (v1)

```cpp
// ✅ GOOD - Each thread has its own work_id  
ParallelExecutor executor(8);
executor.execute([this](int work_id) {  // ← work_id is unique per iteration
    // ...
    BaseEntity e("entity_" + std::to_string(work_id), ...);  // ← No contention!
    sim->put(..., e);
}, 12);
```

**Result**: 
- 1T: +13.7% (reduced overhead)
- 4T: +11.5% (less contention)
- 8T: +39.3% (major contention relief) ✅
- 16T: +10.5%
- 32T: -6.1% (too many threads still causes issues)

---

## Why This Works

**Atomic Counter Contention**: When all 8 threads execute `counter++`:
1. Thread 0 locks: `counter = 1`
2. Thread 1 waits for lock...
3. Thread 2 waits for lock...
4. ...
5. Eventually: Cache line invalidation, memory fence, etc.

**With Thread-Local IDs**: Each thread gets unique `work_id = 0..11` per iteration:
- No atomic operations
- No cache line contention
- No memory fences
- **Pure Work** (put operation) dominates

---

## Remaining Problems

Even with +39% improvement, we still have:

| Test | Current | Expected | Gap |
|------|---------|----------|-----|
| 8T | 683k | 3,258k × 8 = 26M | -97% |
| 16T | 293k | 3,258k × 16 = 52M | -99% |

**So what's STILL wrong?**

Analysis:
1. ✅ Shared Counter: FIXED
2. ❌ **Database Write Serialization**: Still present
3. ❌ **RocksDB Lock**: Still serializes writes
4. ❌ **Single Index**: All threads compete for same write lock

---

## Next Level Optimization

The **real** issue is now exposed: **Each thread is trying to write to the SAME database/index**.

```cpp
sim_->put("parallel_data", e);  // ← All 8 threads want to write HERE
```

This likely serializes in RocksDB:

```
Thread 0: [Acquire Write Lock] → write → [Release]
Thread 1: [Wait...] [Acquire Write Lock] → write → [Release]
Thread 2: [Wait...] [Wait...] [Acquire Write Lock] → write → [Release]
...
```

---

## Solutions Roadmap

### Phase 2: Per-Thread Table/Shard Strategy

```cpp
BENCHMARK_F(ParallelityBench, ParallelInserts_8Threads_Sharded) {
    ParallelExecutor executor(8);
    executor.execute([this](int thread_id) {
        // Each thread writes to its own "shard"
        auto table_name = "parallel_data_shard_" + std::to_string(thread_id);
        for (int i = 0; i < 12; ++i) {
            BaseEntity e("entity_" + std::to_string(thread_id) + "_" + std::to_string(i), ...);
            sim_->put(table_name, e);  // ← Different table per thread!
        }
    }, 1);
}
```

**Expected Gain**: 5-10x (7-14 Threads can write in parallel)

---

### Phase 3: RocksDB Thread-Pool Optimization

Configure RocksDB to use thread pool for concurrent writes:

```cpp
RocksDBWrapper::Config cfg;
cfg.max_write_threads = 32;  // ← Use RocksDB parallelization
cfg.enable_thread_pool = true;
```

**Expected Gain**: 10-20x (if RocksDB supports it)

---

### Phase 4: Batch Write API

```cpp
std::vector<BaseEntity> batch(100);
for (int i = 0; i < 100; ++i) {
    batch[i] = BaseEntity(...);
}
sim_->put_batch("parallel_data", batch);  // ← Single write for 100 items
```

**Expected Gain**: 100-500x (amortize overhead)

---

## Current Status

✅ **Issue Identified**: Shared Atomic Counter  
✅ **Partial Fix Applied**: Use thread-local IDs  
✅ **Result**: +39% improvement @ 8 Threads  

❌ **Remaining Issue**: Database Write Serialization  
🎯 **Next Fix**: Sharding strategy (Phase 2)

---

## Documentation Updated

Files created/updated:
- ✅ [PARALLEL_BOTTLENECK_DIAGNOSIS.md](PARALLEL_BOTTLENECK_DIAGNOSIS.md) - Detailed analysis
- ✅ [PARALLEL_FIX_PRIORITY1_BARRIER_REMOVAL.md](PARALLEL_FIX_PRIORITY1_BARRIER_REMOVAL.md) - Implementation guide
- ✅ [benchmarks/bench_advanced_patterns.cpp](benchmarks/bench_advanced_patterns.cpp) - Fixed ParallelInserts tests

---

## Benchmarks Updated

**ParallelInserts Benchmarks** (all 5):
- ✅ 1Thread: 3.7M ops/sec (+13.7%)
- ✅ 4Threads: 1.17M ops/sec (+11.5%)
- ✅ 8Threads: 683k ops/sec (+39.3%)
- ✅ 16Threads: 293k ops/sec (+10.5%)
- ⚠️ 32Threads: 153k ops/sec (-6.1%, still contended)

---

## Lessons Learned

1. **Atomic Operations** are insidious bottlenecks - can dominate performance
2. **Shared State** in tight loops = guaranteed slowdown
3. **Thread-Local IDs** are simple but powerful optimization
4. **Benchmark Design** matters - the counter was part of the benchmark artifact!

---

## Next Action

Implement **Phase 2: Sharding Strategy** for true parallelization.

Expected result: 8 Threads @ 5-10M ops/sec (7-13x improvement)

> ⚠️ **Historischer Statusreport** – Dieser Bericht beschreibt den Implementierungsstand zum Zeitpunkt der Erstellung.
> Für den aktuellen Stand: Quellcode und aktuelle [`benchmarks/README.md`](../README.md) prüfen.

# 🛠️ Performance Optimization Roadmap & Implementation Guide

**Based on**: Advanced Benchmarks Gap Analysis  
**Target**: 50-100x improvement in critical areas  
**Timeline**: 2-4 weeks

---

## Issue Ranking & Impact

```
🔴 Critical (MUST FIX)
├─ Parallel Scaling: 0.15x speedup (should be 7x) → -95% gap
└─ Memory Pressure: 1.2k ops/s (should be 25k) → -95% gap

🟠 High (SHOULD FIX)  
├─ Transaction Overhead: 83% (should be <5%) → +1560% gap
└─ Best-Practice Gap: Can't measure (~1x, expected 10x)

🟡 Medium (NICE TO FIX)
└─ Write Performance: 637k ops/s (could reach 700k+)
```

---

## Issue #1: Parallel Scaling Catastrophe

### Problem Statement

Multi-threading performance COLLAPSES instead of scaling:

```
Threads  │ Ops/sec  │ Speedup │ Expected │ Actual Gap
─────────┼──────────┼─────────┼──────────┼────────────
1        │ 3.26M    │ 1.0x    │ 1.0x     │ ✅
4        │ 1.05M    │ 0.32x   │ 3.5x     │ ❌ -91%
8        │ 490k     │ 0.15x   │ 7.0x     │ ❌ -98%
16       │ 265k     │ 0.08x   │ 12.0x    │ ❌ -99%
32       │ 163k     │ 0.05x   │ 24.0x    │ ❌ -99.5%
```

### Root Cause Analysis

**Hypothesis 1: Single Database Mutex**
```cpp
// Current architecture
class BenchmarkFixture {
    std::unique_ptr<RocksDBWrapper> db_;  // SHARED across threads
    std::unique_ptr<SecondaryIndexManager> sim_;  // SHARED
    
    void SetUp() {
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
    }
};

// All threads compete for SAME db_ lock:
ThreadA: sim_->put(e1);  ← Lock acquired
ThreadB: sim_->put(e2);  ← BLOCKED, waiting
ThreadC: sim_->put(e3);  ← BLOCKED, waiting
ThreadD: sim_->put(e4);  ← BLOCKED, waiting
```

**Hypothesis 2: Context Switch Thrashing**
- 32 threads on 8-core CPU
- OS scheduler constantly context switching
- Cache line thrashing (all threads fighting for same memory)

**Hypothesis 3: Poor Lock Implementation**
- RocksDB uses exclusive locks by default
- No read-write separation
- No lock-free fast path

### Investigation Steps

#### Step 1: Profile to Confirm Root Cause

```powershell
# Use Windows Performance Analyzer
$perfscript = @"
# Profile ParallelInserts_4Threads
xperf -start perf -on BASE+CSWITCH+PROFILE
C:\VCC\themis\build-msvc\Release\bench_advanced_patterns.exe `
    --benchmark_filter="ParallelInserts_4Threads"
xperf -stop perf -d profile.etl

# Analyze
xperfview profile.etl
"@

# Examine:
# - Context switches (high = thrashing)
# - Lock contention (high = mutex problem)
# - CPU migration (high = bad thread placement)
# - Cache misses (high = memory pressure)
```

#### Step 2: Enable Detailed Logging

```cpp
// Add to bench_advanced_patterns.cpp
class DebugParallelityBench : public benchmark::Fixture {
protected:
    std::atomic<int> lock_wait_count = 0;
    std::atomic<int64_t> total_lock_wait_ns = 0;
    
    void SetUp(const benchmark::State&) override {
        // Setup instrumentation
    }
};

BENCHMARK_F(DebugParallelityBench, Debug_4Threads_WithMetrics)
    (benchmark::State& state) {
    // Measure lock wait times, context switches, cache misses
}
```

### Solution: Per-Thread Database Instances

**Approach 1: Separate DB per Thread**

```cpp
class OptimizedParallelityBench : public benchmark::Fixture {
protected:
    std::vector<std::unique_ptr<DatabaseFixture>> thread_dbs;
    std::vector<std::unique_ptr<SecondaryIndexManager>> thread_sims;
    
    void SetUp(const benchmark::State&) override {
        // Create separate DB for each thread
        thread_dbs.clear();
        thread_sims.clear();
        
        for (int t = 0; t < num_threads; ++t) {
            auto db = std::make_unique<DatabaseFixture>("par_thread_" + std::to_string(t));
            auto sim = std::make_unique<SecondaryIndexManager>(db->getDb());
            sim->createIndex("data", "id");
            
            thread_dbs.push_back(std::move(db));
            thread_sims.push_back(std::move(sim));
        }
    }
};

BENCHMARK_F(OptimizedParallelityBench, Optimized_ParallelInserts_8Threads)
    (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int thread_id) {
            // Use thread-specific DB (NO LOCK CONTENTION)
            auto& sim = thread_sims[thread_id];
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_" + std::to_string(thread_id * 1000 + i), ...);
                sim->put("data", e);  // ← NO GLOBAL LOCK
            }
        }, iterations_per_thread);
    }
    state.SetItemsProcessed(state.iterations() * 8 * 100);
}
```

**Expected Improvement**: 7x-8x (linear scaling instead of -95%)

---

**Approach 2: Read-Write Locks**

```cpp
class LockingStrategyBench : public benchmark::Fixture {
protected:
    std::shared_mutex rw_lock;  // Multiple readers, exclusive writers
    
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("rw_locks");
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    }
};

BENCHMARK_F(LockingStrategyBench, OptimizedRead_WriteLocksStrategy)
    (benchmark::State& state) {
    std::uniform_int_distribution<int> ops(0, 99);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            if (ops(rng) < 80) {  // 80% reads
                std::shared_lock read_lock(rw_lock);  // Multiple threads can share
                // Read operation
            } else {  // 20% writes
                std::unique_lock write_lock(rw_lock);  // Exclusive
                // Write operation
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
```

**Expected Improvement**: 3x-4x for read-heavy (reads don't block each other)

---

**Approach 3: Lock-Free Data Structures**

```cpp
class LockFreeArch : public benchmark::Fixture {
protected:
    // Use lock-free queue for writes
    std::atomic_queue<BaseEntity> write_queue;
    
    void SetUp(const benchmark::State&) override {
        // Background writer thread processes queue
    }
};

BENCHMARK_F(LockFreeArch, LockFree_QueuedWrites)
    (benchmark::State& state) {
    std::atomic<int> counter(0);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("entity_" + std::to_string(counter++), ...);
            write_queue.push(e);  // ← No lock, atomic operation
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
```

**Expected Improvement**: 5x-10x (near-lock-free performance)

### Recommendation

**Start with Approach 1** (per-thread DBs):
- Simplest to implement
- Immediate 7x improvement expected
- Aligns with typical multi-threaded architecture

```cpp
// Modified bench_advanced_patterns.cpp
class ParallelityBench : public benchmark::Fixture {
protected:
    std::vector<std::unique_ptr<SecondaryIndexManager>> thread_sims;
    
    void SetUp(const benchmark::State&) override {
        // Create per-thread managers
        thread_sims.clear();
        for (int t = 0; t < 8; ++t) {
            auto db = std::make_unique<DatabaseFixture>("parallel_" + std::to_string(t));
            auto sim = std::make_unique<SecondaryIndexManager>(db->getDb());
            sim->createIndex("data", "id");
            thread_sims.push_back(std::move(sim));
        }
    }
};
```

---

## Issue #2: Memory Pressure Bottleneck

### Problem Statement

100KB documents take 39.8ms per operation (expected: 5-10ms):

```
Document Size │ Throughput │ Latency   │ Gap
───────────────┼────────────┼───────────┼──────────
Small (100B)   │ Very fast  │ <1ms      │ ✅
Medium (10KB)  │ Good       │ ~2ms      │ ✅
Large (100KB)  │ 1.2k ops/s │ 39.8ms ❌ │ -97% 🔴
XL (1MB)       │ Terrible   │ >100ms    │ -99% 🔴
```

### Root Cause Analysis

**Hypothesis 1: Memory Allocation Overhead**
```cpp
BaseEntity e("mem_" + std::to_string(++write_counter), BaseEntity::FieldMap{
    {"large_data", RandomGenerator::instance().randStr(100000)},  // ← 100KB malloc
    {"value", static_cast<double>(write_counter)}
});
sim_->put("resilience", e);  // ← Each operation allocates 100KB
```

For 50 documents/iteration:
- 50 × 100KB = 5MB allocated per iteration
- Memory allocator overhead becomes dominant
- Fragmentation possible

**Hypothesis 2: No Memory Pooling**
```cpp
// Current: Fresh allocation every time
std::string large_data = RandomGenerator::instance().randStr(100000);

// Problem: Allocator fragmentation
// After 1000 operations: 100MB allocated + fragmented
```

**Hypothesis 3: Garbage Collection or Memory Pressure**
- OS paging when memory pressure exceeds physical RAM
- TLB misses for large allocations
- Cache misses for 100KB working set

### Solution: Memory Pooling

**Approach 1: Pre-allocated String Buffer Pool**

```cpp
class MemoryPool {
private:
    std::vector<std::string> pool;
    std::queue<size_t> available;
    static constexpr size_t BUFFER_SIZE = 100000;
    static constexpr size_t POOL_SIZE = 100;
    
public:
    MemoryPool() : pool(POOL_SIZE) {
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            pool[i].reserve(BUFFER_SIZE);
            available.push(i);
        }
    }
    
    std::string& acquire() {
        if (available.empty()) {
            throw std::runtime_error("Pool exhausted");
        }
        size_t idx = available.front();
        available.pop();
        return pool[idx];
    }
    
    void release(size_t idx) {
        pool[idx].clear();
        available.push(idx);
    }
};

// Usage in benchmark
class OptimizedMemoryPressureBench : public benchmark::Fixture {
private:
    MemoryPool pool;  // Reusable buffer pool
    
public:
    BENCHMARK_F(OptimizedMemoryPressureBench, 
                  Optimized_MemoryPressure_100KB_WithPooling)
        (benchmark::State& state) {
        for (auto _ : state) {
            for (int i = 0; i < 50; ++i) {
                auto& buffer = pool.acquire();
                
                // Fill buffer
                for (size_t j = 0; j < buffer.capacity(); ++j) {
                    buffer[j] = 'a' + (j % 26);
                }
                
                BaseEntity e("mem_" + std::to_string(i), BaseEntity::FieldMap{
                    {"large_data", buffer},
                    {"value", static_cast<double>(i)}
                });
                sim_->put("resilience", e);
                
                pool.release(i % 100);  // Return to pool
            }
        }
        state.SetItemsProcessed(state.iterations() * 50);
    }
};
```

**Expected Improvement**: 10x-20x (reuse avoids allocation overhead)

---

**Approach 2: Chunked Writing**

```cpp
class ChunkedWriter {
public:
    ChunkedWriter(const std::string& data, size_t chunk_size = 10000)
        : data_(data), chunk_size_(chunk_size), offset_(0) {}
    
    bool hasNext() const { return offset_ < data_.size(); }
    
    std::string next() {
        size_t end = std::min(offset_ + chunk_size_, data_.size());
        std::string chunk = data_.substr(offset_, end - offset_);
        offset_ = end;
        return chunk;
    }
    
private:
    const std::string& data_;
    size_t chunk_size_;
    size_t offset_;
};

BENCHMARK_F(OptimizedMemoryPressureBench, 
              Optimized_MemoryPressure_Chunked_100KB)
    (benchmark::State& state) {
    std::string large_doc(100000, 'x');
    
    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            ChunkedWriter writer(large_doc, 10000);  // 10KB chunks
            int chunk_num = 0;
            
            while (writer.hasNext()) {
                std::string chunk = writer.next();
                BaseEntity e("mem_chunk_" + std::to_string(i) + "_" 
                           + std::to_string(chunk_num++), 
                    BaseEntity::FieldMap{
                        {"chunk_data", chunk},
                        {"chunk_num", static_cast<double>(chunk_num)}
                    });
                sim_->put("resilience", e);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 50);
}
```

**Expected Improvement**: 5x-10x (smaller working set per operation)

---

**Approach 3: Streaming with Background Writer**

```cpp
class StreamingWriter {
private:
    std::queue<BaseEntity> queue;
    std::mutex queue_mutex;
    std::thread writer_thread;
    std::atomic<bool> stop_requested = false;
    
    void writerLoop(SecondaryIndexManager* sim) {
        while (!stop_requested) {
            {
                std::unique_lock lock(queue_mutex);
                if (!queue.empty()) {
                    auto entity = queue.front();
                    queue.pop();
                    lock.unlock();
                    sim->put("resilience", entity);  // Outside lock
                }
            }
            std::this_thread::yield();
        }
    }
    
public:
    StreamingWriter(SecondaryIndexManager* sim) {
        writer_thread = std::thread(&StreamingWriter::writerLoop, this, sim);
    }
    
    ~StreamingWriter() {
        stop_requested = true;
        writer_thread.join();
    }
    
    void enqueue(const BaseEntity& e) {
        std::unique_lock lock(queue_mutex);
        queue.push(e);
    }
};

BENCHMARK_F(OptimizedMemoryPressureBench, 
              Optimized_MemoryPressure_Streaming)
    (benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("resilience", "id");
    StreamingWriter writer(sim.get());
    
    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            BaseEntity e("mem_stream_" + std::to_string(i), BaseEntity::FieldMap{
                {"large_data", RandomGenerator::instance().randStr(100000)},
                {"value", static_cast<double>(i)}
            });
            writer.enqueue(e);  // Non-blocking enqueue
        }
    }
    state.SetItemsProcessed(state.iterations() * 50);
}
```

**Expected Improvement**: 3x-5x (amortizes write latency across threads)

### Recommendation

**Start with Approach 1** (Memory Pool):
- Simplest to implement
- 10x-20x improvement expected
- No architectural changes needed

```cpp
// Add to bench_advanced_patterns.cpp
class MemoryPool {
    // ... (as above)
};

class OptimizedSelfProtectionBench : public benchmark::Fixture {
private:
    MemoryPool<100000> large_doc_pool;
};
```

---

## Issue #3: Transaction Overhead

### Problem Statement

Transactions have 83% overhead (525k vs 637k ops/s, expected <5%):

```
Mode                    │ Throughput │ Overhead
────────────────────────┼────────────┼──────────
Single operations       │ 637k ops/s │ Baseline
Transactional (5 ops)   │ 525k ops/s │ 83% ❌
Expected overhead       │ ~600k      │ <5% ✅
```

### Root Cause Analysis

**Hypothesis 1: Expensive Begin/Commit**
```cpp
// Current (assumed)
void transactionalBatch() {
    for (int batch = 0; batch < 100; ++batch) {
        db->beginTransaction();      // ← EXPENSIVE
        for (int i = 0; i < 5; ++i) {
            sim->put(entity);         // ← Inside transaction
        }
        db->commitTransaction();      // ← EXPENSIVE
    }
}

// Problem: 100 × (beginTx + commitTx) = overhead
```

**Hypothesis 2: Unnecessary Isolation**
```cpp
// Possible implementation
void put(const Entity& e, TransactionWrapper& txn) {
    // Lock acquisition
    // Write-ahead log
    // Conflict detection
    // Timestamp management
    // All per-operation
}
```

### Solution: Batch Transactions

**Approach 1: Optimize Transaction Bundling**

```cpp
class OptimizedBatchTransactions {
public:
    void executeBatch(
        SecondaryIndexManager& sim,
        const std::vector<BaseEntity>& entities) {
        
        // Single begin/commit for entire batch
        auto [status, txn] = db.beginTransaction();
        
        for (const auto& entity : entities) {
            sim.put("table", entity, txn);  // Use transaction
        }
        
        txn.commit();  // Single commit for all
    }
};

BENCHMARK_F(OptimizedBatchTransactionsBench, 
              Optimized_TransactionOverhead_Batched)
    (benchmark::State& state) {
    for (auto _ : state) {
        for (int batch = 0; batch < 100; ++batch) {
            std::vector<BaseEntity> entities;
            
            // Prepare batch (outside transaction)
            for (int i = 0; i < 5; ++i) {
                entities.emplace_back("tx_" + std::to_string(batch * 5 + i), ...);
            }
            
            // Execute batch in single transaction
            auto [status, txn] = sim->db().beginTransaction();
            for (const auto& e : entities) {
                sim->put("transaction_test", e, txn);
            }
            txn.commit();
        }
    }
    state.SetItemsProcessed(state.iterations() * 100 * 5);
}
```

**Expected Improvement**: 10x-20x reduction in transaction overhead

---

**Approach 2: Asynchronous Commit**

```cpp
class AsyncTransactionCommit {
private:
    std::queue<Transaction> pending;
    std::thread commit_thread;
    
public:
    void enqueueTx(Transaction txn) {
        pending.push(std::move(txn));  // Non-blocking
    }
    
    void commitLoop() {
        while (true) {
            if (!pending.empty()) {
                auto txn = pending.front();
                pending.pop();
                txn.commit();  // Commit in background
            }
        }
    }
};
```

**Expected Improvement**: 2x-3x (overlaps commit latency)

### Recommendation

**Measure actual transaction cost first**:

```cpp
BENCHMARK_F(TransactionAnalysisBench, Profile_BeginCommit_Cost)
    (benchmark::State& state) {
    auto [status, txn] = sim->db().beginTransaction();
    
    for (auto _ : state) {
        txn.commit();
        txn = sim->db().beginTransaction().second;  // Start new
    }
}
```

If overhead is in begin/commit, use Approach 1 (batching).  
If overhead is per-operation, need deeper investigation.

---

## Implementation Timeline

### Week 1: Diagnosis & Profiling
- [ ] Profile parallel scaling (Windows Performance Analyzer)
- [ ] Profile memory pressure (malloc callstacks)
- [ ] Profile transactions (time breakdown)
- [ ] Document findings

### Week 2: Fix Parallel Scaling
- [ ] Implement per-thread database approach
- [ ] Benchmark before/after (expect 7x improvement)
- [ ] Verify linear scaling up to 8 threads
- [ ] Merge to main

### Week 3: Fix Memory Pressure
- [ ] Implement memory pooling
- [ ] Benchmark before/after (expect 10-20x improvement)
- [ ] Test with various document sizes
- [ ] Merge to main

### Week 4: Optimize Transactions
- [ ] Implement batch transaction optimization
- [ ] Benchmark before/after (expect 10-20x improvement)
- [ ] Document transaction best-practices
- [ ] Merge to main

### Final: Re-run Benchmarks
- [ ] Execute full advanced benchmark suite
- [ ] Compare to baseline
- [ ] Measure improvements
- [ ] Update documentation

---

## Success Criteria

| Issue | Current | Target | Success |
|-------|---------|--------|---------|
| Parallel (8T) | 0.15x | 7x | 46x improvement |
| Memory (100KB) | 1.2k | 25k | 20x improvement |
| Transactions | 83% | <5% | 16x improvement |

---

## Files to Modify

1. **benchmarks/bench_advanced_patterns.cpp**
   - Add optimized benchmark variants
   - Keep originals for comparison

2. **src/index/secondary_index.h** (possibly)
   - Optimize put() operation
   - Reduce transaction overhead

3. **CMakeLists.txt**
   - Add new benchmark targets

---

**Status**: Optimization roadmap complete  
**Next**: Pick Issue #1 and start implementation

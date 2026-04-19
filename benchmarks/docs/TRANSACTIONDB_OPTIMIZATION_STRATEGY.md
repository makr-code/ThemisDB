> ⚠️ **Historische Strategie** – Dokument beschreibt den Optimierungsansatz zum Zeitpunkt der Erstellung.

# TransactionDB Optimization Strategy

## Executive Summary

**Discovery**: RocksDB offers **WritePrepared** policy that improves TransactionDB performance by 30-68% (Facebook benchmarks).

**Current Implementation**: We use default **WriteCommitted** policy.

**Recommendation**: Switch to WritePrepared policy for significant parallel performance gains.

---

## Problem with WriteCommitted (Our Current Policy)

### Architecture Bottleneck

```
WriteCommitted Flow:
┌──────────────┐
│ BeginTxn()   │
│              │
│ Put("k1")    │ ──> Buffer in memory (not yet written!)
│ Put("k2")    │ ──> Buffer in memory
│ Put("k3")    │ ──> Buffer in memory
│              │
│ Commit()     │ ──> NOW write everything to WAL + MemTable
└──────────────┘         ↑
                    BOTTLENECK: All work at commit time!
```

**Problem**: All writes happen at `Commit()` time
- Buffer grows in memory during transaction
- Commit latency increases with transaction size
- Serial commits in 2PC become bottleneck
- **Cannot provide weaker isolation levels** (READ UNCOMMITTED)

---

## Solution: WritePrepared Policy

### Architecture Overview

```
WritePrepared Flow:
┌──────────────┐
│ BeginTxn()   │
│              │
│ Put("k1")    │ ──> Buffer in memory
│ Put("k2")    │ ──> Buffer in memory
│              │
│ Prepare()    │ ──> Write to WAL + MemTable (with prepare_seq)
│              │     ↑ Heavy lifting done here!
│              │
│ Commit()     │ ──> Write commit marker (FAST!)
└──────────────┘     Just update CommitCache
```

**Benefits**:
- ✅ Commit phase is **lightweight** (just a marker)
- ✅ Parallel writes during Prepare phase
- ✅ Reduces memory pressure (written to memtable earlier)
- ✅ Better for 2PC (two-phase commit)
- ✅ Supports weaker isolation levels

---

## Performance Gains (Facebook Benchmarks)

From official WritePrepared documentation:

| Benchmark | TPS Improvement | P95 Latency | CPU/Query |
|-----------|----------------|-------------|-----------|
| **insert** | **+68%** | - | - |
| **update-noindex** | **+30%** | **-38%** | - |
| **update-index** | **+61%** | **-28%** | - |
| **read-write** | **+6%** | **-3.5%** | - |
| **read-only** | -1.2% | -1.8% | - |
| **linkbench** | +1.9% | - | -0.6% |

**Key Insight**: Write-heavy workloads see **30-68% improvement!**

---

## How WritePrepared Works

### 1. Sequence Numbers for Ordering

Instead of WAL providing ordering, WritePrepared uses:

```cpp
prepare_seq = 100;  // Assigned at Prepare()
commit_seq = 150;   // Assigned at Commit()

CommitCache[prepare_seq % CACHE_SIZE] = <prepare_seq, commit_seq>
```

**Lock-free CommitCache**:
- Fixed-size array of `std::atomic<uint64_t>`
- ~8M entries (50 seconds at 80K tps)
- O(1) lookup for conflict detection

---

### 2. MVCC Snapshot Isolation

```cpp
bool IsInSnapshot(prepare_seq, snapshot_seq) {
    if (snapshot_seq < prepare_seq) return false;
    if (prepare_seq < min_uncommitted) return true;
    
    if (prepare_seq in CommitCache) {
        return CommitCache[prepare_seq] <= snapshot_seq;
    }
    
    // Old commit (evicted from cache)
    if (max_evicted_seq < snapshot_seq) return true;
    
    // Check old_commit_map for very old snapshots
    return !overlapped;
}
```

**Optimizations**:
- Lock-free reads from CommitCache (x86_64 cache coherency)
- PreparedHeap tracks uncommitted transactions
- Lock-free snapshot list (first 128 in atomic array)

---

### 3. Two Write Queues

**Key Innovation**: Separate queues for prepare and commit

```cpp
TransactionDBOptions txn_options;
txn_options.write_policy = WRITE_PREPARED;
txn_options.two_write_queues = true;  // ← CRITICAL

// Queue 1: Prepare (writes to WAL + MemTable)
// Queue 2: Commit (writes commit marker to WAL only)
```

**Benefit**: Prepare and Commit can proceed in parallel!

---

## Implementation Changes Required

### 1. Change Write Policy

**File**: `src/storage/rocksdb_wrapper.cpp`

```cpp
// In configureOptions():
txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;
txn_db_options_->wp_commit_cache_bits = 23;  // 8M entries (default)
txn_db_options_->two_write_queues = true;    // Enable parallel queues
```

---

### 2. Update Config Struct

**File**: `include/storage/rocksdb_wrapper.h`

```cpp
struct Config {
    // ... existing fields ...
    
    // Transaction Performance (NEW)
    enum class WritePolicy {
        WriteCommitted,   // Default (current)
        WritePrepared,    // For performance
        WriteUnprepared   // For large transactions
    };
    
    WritePolicy write_policy = WritePolicy::WriteCommitted;
    bool two_write_queues = false;           // Parallel prepare/commit
    uint64_t wp_commit_cache_bits = 23;      // 8M entries
    uint64_t wp_snapshot_cache_bits = 7;     // 128 entries
};
```

---

### 3. Use Prepare() in Transactions

**Current Code** (WriteCommitted):
```cpp
Transaction* txn = db_->BeginTransaction(write_options, txn_options);
txn->Put("key", "value");
txn->Commit();  // ← All work happens here
```

**New Code** (WritePrepared):
```cpp
Transaction* txn = db_->BeginTransaction(write_options, txn_options);
txn->Put("key", "value");
txn->Prepare();  // ← Heavy lifting (WAL + MemTable)
txn->Commit();   // ← Fast (just marker)
```

**CRITICAL**: Must call `Prepare()` to benefit from WritePrepared!

---

### 4. Benchmark Integration

**New Fixture**: ParallelityBenchPhase2G (WritePrepared)

```cpp
class ParallelityBenchPhase2G : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("phase2g_writeprepared");
        
        // Configure WritePrepared policy
        RocksDBWrapper::Config cfg;
        cfg.db_path = fixture_->getPath() + "_custom";
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = true;
        cfg.wp_commit_cache_bits = 23;  // 8M entries
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("phase2g_test", "id");
    }
    
    void BenchmarkWriteThread(benchmark::State& state, int num_threads) {
        for (auto _ : state) {
            auto txn = db_->beginTransaction();
            
            for (int i = 0; i < 100; i++) {
                std::string key = "key_" + std::to_string(i);
                std::string value = "value_" + std::to_string(i);
                txn->put(key, value);
            }
            
            // CRITICAL: Call Prepare() for WritePrepared benefit!
            txn->prepare();  // ← Heavy work
            txn->commit();   // ← Fast
        }
    }
};

BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_8Threads)(benchmark::State& state) {
    BenchmarkWriteThread(state, 8);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_8Threads)->Threads(8)->UseRealTime();
```

---

## Expected Results

### Based on Facebook Benchmarks

**Our workload is write-heavy** (insert/update pattern):

| Threads | Current (WriteCommitted) | Expected (WritePrepared) | Gain |
|---------|-------------------------|-------------------------|------|
| 1T | 4.0M | **4.0M** | 0% (single-threaded) |
| 4T | 1.17M | **1.6-2.0M** | +37-71% |
| 8T | 683k | **1.0-1.2M** | +46-76% |
| 16T | 387k | **620-700k** | +60-81% |
| 32T | 125k | **200-250k** | +60-100% |

**Reasoning**:
- Facebook saw +30-68% on insert/update workloads
- Our workload: 100 writes per transaction (similar to "insert")
- Expected: +50-80% improvement at high thread counts

---

## Implementation Plan: Phase 2G

### Step 1: Add Write Policy Support (5 min)

**File**: `include/storage/rocksdb_wrapper.h`

```cpp
struct Config {
    // ... existing fields ...
    
    enum class WritePolicy {
        WriteCommitted,   // Current default
        WritePrepared,    // For performance (NEW)
        WriteUnprepared   // For very large transactions
    };
    
    WritePolicy write_policy = WritePolicy::WriteCommitted;
    bool two_write_queues = false;
    uint64_t wp_commit_cache_bits = 23;  // 8M entries (default)
};
```

---

### Step 2: Configure WritePrepared Policy (5 min)

**File**: `src/storage/rocksdb_wrapper.cpp`

```cpp
void RocksDBWrapper::configureOptions() {
    // ... existing code ...
    
    // Configure Write Policy
    switch (config_.write_policy) {
        case Config::WritePolicy::WriteCommitted:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_COMMITTED;
            break;
        case Config::WritePolicy::WritePrepared:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;
            txn_db_options_->wp_commit_cache_bits = config_.wp_commit_cache_bits;
            break;
        case Config::WritePolicy::WriteUnprepared:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_UNPREPARED;
            txn_db_options_->wp_commit_cache_bits = config_.wp_commit_cache_bits;
            break;
    }
    
    // Enable two write queues for WritePrepared/WriteUnprepared
    if (config_.write_policy != Config::WritePolicy::WriteCommitted) {
        options_->two_write_queues = config_.two_write_queues;
    }
}
```

---

### Step 3: Add Prepare() Support to TransactionWrapper (10 min)

**File**: `include/storage/rocksdb_wrapper.h`

```cpp
class TransactionWrapper {
public:
    // ... existing methods ...
    
    // NEW: Prepare transaction (for WritePrepared policy)
    bool prepare();
};
```

**File**: `src/storage/rocksdb_wrapper.cpp`

```cpp
bool RocksDBWrapper::TransactionWrapper::prepare() {
    if (!active_ || !txn_) {
        THEMIS_ERROR("Cannot prepare inactive transaction");
        return false;
    }
    
    rocksdb::Status status = txn_->Prepare();
    if (!status.ok()) {
        THEMIS_ERROR("Transaction prepare failed: {}", status.ToString());
        return false;
    }
    
    prepared_ = true;  // Track prepare state
    return true;
}
```

---

### Step 4: Create Phase 2G Benchmark (20 min)

**File**: `benchmarks/bench_advanced_patterns.cpp`

```cpp
class ParallelityBenchPhase2G : public benchmark::Fixture {
protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("phase2g_writeprepared");
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = fixture_->getPath() + "_custom";
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = true;
        cfg.wp_commit_cache_bits = 23;
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("phase2g_test", "id");
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_->close();
        db_.reset();
        fixture_.reset();
    }
    
    void BenchmarkWriteThread(benchmark::State& state, int num_threads) {
        int thread_id = state.thread_index();
        
        for (auto _ : state) {
            auto txn = db_->beginTransaction();
            
            for (int i = 0; i < 100; i++) {
                std::string key = "key_" + std::to_string(thread_id * 10000 + i);
                std::string value = "value_" + std::to_string(i);
                txn->put(key, value);
            }
            
            // CRITICAL: Prepare before commit
            txn->prepare();  // Heavy work (WAL + MemTable)
            txn->commit();   // Lightweight (marker only)
        }
        
        state.SetItemsProcessed(state.iterations() * 100);
    }
};

// Benchmark methods for 1/4/8/16/32 threads
BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_1Thread)(benchmark::State& state) {
    BenchmarkWriteThread(state, 1);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_1Thread)->Threads(1)->UseRealTime();

BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_4Threads)(benchmark::State& state) {
    BenchmarkWriteThread(state, 4);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_4Threads)->Threads(4)->UseRealTime();

BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_8Threads)(benchmark::State& state) {
    BenchmarkWriteThread(state, 8);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_8Threads)->Threads(8)->UseRealTime();

BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_16Threads)(benchmark::State& state) {
    BenchmarkWriteThread(state, 16);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_16Threads)->Threads(16)->UseRealTime();

BENCHMARK_DEFINE_F(ParallelityBenchPhase2G, Phase2G_32Threads)(benchmark::State& state) {
    BenchmarkWriteThread(state, 32);
}
BENCHMARK_REGISTER_F(ParallelityBenchPhase2G, Phase2G_32Threads)->Threads(32)->UseRealTime();
```

---

### Step 5: Build and Execute (10 min)

```powershell
# Build benchmark
cd C:\VCC\themis\build-msvc
cmake --build . --target bench_advanced_patterns --config Release --parallel 8

# Run Phase 2G benchmarks
.\Release\bench_advanced_patterns.exe --benchmark_filter="Phase2G" --benchmark_format=json --benchmark_out=C:\tmp\phase2g_writeprepared.json

# Compare with Phase 1F
# Expected: +50-80% improvement at 8/16/32 threads!
```

---

## Why WritePrepared Will Work (Unlike disableWAL)

### disableWAL Failed Because:
1. ❌ Broke transaction ordering
2. ❌ Removed synchronization points
3. ❌ Caused lock manager breakdown
4. ❌ Violated MVCC guarantees

### WritePrepared Succeeds Because:
1. ✅ **Preserves transaction ordering** (via prepare_seq/commit_seq)
2. ✅ **Maintains synchronization** (lock-free CommitCache)
3. ✅ **Distributes work** (Prepare does heavy lifting, Commit is fast)
4. ✅ **Keeps MVCC intact** (IsInSnapshot algorithm)
5. ✅ **Officially supported** (Facebook production code)

---

## Migration Path

### Backward Compatibility

**IMPORTANT**: WAL format is **not compatible** between policies.

**Safe Migration**:
1. Flush all memtables: `db->FlushWAL(true)`
2. Close database
3. Change write_policy to WritePrepared
4. Reopen database

**Database files (SST) are compatible** - only WAL format differs.

---

### Production Deployment Strategy

**Phase 2G (Benchmark)**:
1. Test WritePrepared in benchmarks
2. Validate +50-80% improvement
3. Document any issues

**Phase 2H (Staging)**:
1. Test in dev environment
2. Verify correctness
3. Load testing

**Phase 2I (Production)**:
1. Scheduled maintenance window
2. Flush + Close + Reopen with WritePrepared
3. Monitor performance

---

## Limitations and Considerations

### Current Limitations (from RocksDB docs)

1. **~1% overhead for read workloads** - Extra work to check committed vs uncommitted
2. **WAL format incompatible** - Must flush before switching policies
3. **Iterator::Refresh not supported** - Can be added if needed
4. **Non-2PC transactions** - May incur two writes with two_write_queues

### Mitigation

1. **Read overhead**: Acceptable trade-off for +50-80% write improvement
2. **WAL incompatibility**: One-time migration during maintenance
3. **Iterator::Refresh**: Not used in our codebase
4. **Non-2PC**: Benchmark uses 2PC (Prepare + Commit), so no issue

---

## Summary

### Why This Will Work

1. **Official Solution**: WritePrepared is Facebook's recommended optimization for TransactionDB
2. **Proven Results**: +30-68% in Facebook benchmarks
3. **Correct Approach**: Unlike disableWAL, this preserves all ACID guarantees
4. **Architectural Match**: Our workload (write-heavy) aligns with WritePrepared benefits
5. **Simple Implementation**: ~50 lines of code changes

---

### Next Steps

1. ✅ **Phase 2G Implementation** (40 minutes)
   - Add WritePolicy config
   - Implement prepare() method
   - Create benchmark fixture
   - Compile and test

2. ⏭️ **Phase 2G Execution** (10 minutes)
   - Run benchmarks
   - Collect results
   - Compare with Phase 1F

3. ⏭️ **Phase 2G Analysis** (if successful)
   - Document improvements
   - Create deployment guide
   - Recommend for production

---

### Expected Outcome

**Conservative Estimate**: +50% @ 8T (683k → 1.0M ops/sec)  
**Optimistic Estimate**: +76% @ 8T (683k → 1.2M ops/sec)

**This would SOLVE the parallel scaling problem!**

---

## References

1. **WritePrepared Transactions**: https://github.com/facebook/rocksdb/wiki/WritePrepared-Transactions
2. **Transactions**: https://github.com/facebook/rocksdb/wiki/Transactions
3. **Facebook Benchmarks**: Sysbench + Linkbench results showing +30-68%
4. **Our Analysis**: TRANSACTIONDB_VS_DB_ANALYSIS.md
5. **Phase 2F Failure**: PHASE2F_ROOT_CAUSE_ANALYSIS.md (disableWAL incompatible)

---

**Status**: Ready for implementation  
**Confidence**: HIGH (official solution with proven results)  
**Risk**: LOW (fully supported by RocksDB)  
**Expected Gain**: +50-80% parallel write performance

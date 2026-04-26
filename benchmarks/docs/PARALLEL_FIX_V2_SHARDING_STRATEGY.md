> ⚠️ **Historischer Plan** – Strategie-Dokument beschreibt den Stand zum Zeitpunkt der Erstellung.

# 🚀 Phase 2: Sharding Strategy for True Parallelization

**Goal**: Eliminate Database Write Serialization  
**Expected Gain**: 5-10x (from 683k → 5-7M @ 8 Threads)  
**Timeline**: 1-2 Hours  

---

## The Strategy

Instead of all threads writing to the **same table**, each thread writes to its **own shard**:

```cpp
Thread 0 writes to: parallel_data_shard_0
Thread 1 writes to: parallel_data_shard_1
Thread 2 writes to: parallel_data_shard_2
...
Thread 7 writes to: parallel_data_shard_7

// No contention! Each has its own lock domain.
```

---

## Implementation: Sharded Benchmark Fixtures

### Step 1: Create ParallelityBenchSharded Fixture

Add to [benchmarks/bench_advanced_patterns.cpp](benchmarks/bench_advanced_patterns.cpp) after ParallelityBench:

```cpp
// ============================================================================
// SHARDED PARALLEL BENCHMARKS (TRUE PARALLELIZATION)
// ============================================================================

/**
 * Sharded parallel benchmark - each thread writes to own table/shard
 * This tests true parallelization without write serialization
 */
class ParallelityBenchSharded : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_sharded");
        
        // Create one index per potential thread (32)
        db_wrapper_ = &fixture_->getDb();
        for (int i = 0; i < 32; ++i) {
            auto sim = std::make_unique<SecondaryIndexManager>(*db_wrapper_);
            sim->createIndex("parallel_shard_" + std::to_string(i), "id");
            shards_.push_back(std::move(sim));
        }
    }
    
    void TearDown(const benchmark::State&) override {
        shards_.clear();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    RocksDBWrapper* db_wrapper_;
    std::vector<std::unique_ptr<SecondaryIndexManager>> shards_;
};

// Sharded: 1 thread (baseline)
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i), 
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 4 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 8 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 16 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 32 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
```

---

## Step 2: Compile and Test

```bash
cd C:\VCC\themis\build-msvc
cmake --build . --target bench_advanced_patterns --config Release

# Run sharded benchmarks
.\Release\bench_advanced_patterns.exe --benchmark_filter="ShardedParallel" \
    --benchmark_format=json --benchmark_out=C:\tmp\sharded_results.json
```

---

## Step 3: Analysis Script

```powershell
$baseline = Get-Content "C:\tmp\counter_fix_results.json" | ConvertFrom-Json
$sharded = Get-Content "C:\tmp\sharded_results.json" | ConvertFrom-Json

Write-Host "`n=== SHARDING IMPACT (Baseline vs Sharded) ===" -ForegroundColor Cyan
Write-Host ("`n{0,-15} | {1,12} | {2,12} | {3,10}" -f "Test", "Baseline", "Sharded", "Change")
Write-Host "-" * 55

@("1Thread", "4Threads", "8Threads", "16Threads", "32Threads") | ForEach-Object {
    $t = $_
    $b = $baseline.benchmarks | Where-Object { $_.name -match "ParallelInserts_$t" } | Select -First 1
    $s = $sharded.benchmarks | Where-Object { $_.name -match "ShardedParallel_$t" } | Select -First 1
    
    $b_val = [Math]::Round($b.items_per_second, 0)
    $s_val = [Math]::Round($s.items_per_second, 0)
    $change = [Math]::Round(($s_val - $b_val) / $b_val * 100, 1)
    $indicator = if ($change -gt 0) { "✅" } else { "❌" }
    
    Write-Host ("{0,-15} | {1,12} | {2,12} | {3,9}% {4}" -f $t, $b_val, $s_val, $change, $indicator)
}
```

---

## Expected Results

| Test | Baseline (Counter-Fixed) | Sharded | Improvement | Speedup vs 1T |
|------|---|---|---|---|
| 1 Thread | 3.7M | 3.7M | 0% | 1x |
| 4 Threads | 1.17M | 4.4M | +275% | 1.2x |
| 8 Threads | 683k | 8.2M | +1100% | 2.2x |
| 16 Threads | 293k | 12.0M | +3900% | 3.2x |
| 32 Threads | 153k | 15.0M | +9700% | 4x |

**Note**: These are aspirational. Real results depend on RocksDB internals.
Realistic expectation: 50-300% improvement (not 1000%)

---

## Key Insights

### Why Sharding Works:

1. **No Write Contention**: Each thread has exclusive access to its shard
2. **Parallel I/O**: 8 threads can write to 8 different SST files simultaneously
3. **CPU Efficiency**: Each thread does 100% work (no lock waiting)
4. **Cache Locality**: Each thread's data stays in its own cache

### Limitations:

1. **Memory Overhead**: 32 separate indexes × overhead
2. **Merge Complexity**: Reading requires merging all shards
3. **RocksDB Internals**: May still have global locks we don't know about
4. **Not Production**: Sharding is test artifact, not real design

---

## Real-World Translation

In production, sharding looks like:

```
// Instead of:
shared_db.put("users", user);  // ← Single writer
shared_db.put("users", user);
shared_db.put("users", user);

// You'd do:
user_shard[hash(user.id) % num_shards].put(user);  // ← Parallel writes
```

This is how databases scale:
- **RocksDB**: LSM tree with multiple column families
- **MongoDB**: Sharding by key range
- **Cassandra**: Consistent hashing
- **Kafka**: Multiple partitions

---

## Implementation Checklist

- [ ] Add ParallelityBenchSharded fixture
- [ ] Implement 5 ShardedParallel benchmark methods
- [ ] Compile successfully
- [ ] Run benchmarks with --benchmark_filter="ShardedParallel"
- [ ] Export JSON results
- [ ] Analyze improvement
- [ ] Document findings
- [ ] Compare with baseline

---

## Debugging Tips

**Problem**: Compilation error with `shards_` vector
**Solution**: Ensure `std::unique_ptr<SecondaryIndexManager>` is properly initialized

```cpp
// In SetUp():
auto sim = std::make_unique<SecondaryIndexManager>(*db_wrapper_);
sim->createIndex(table_name, "id");
shards_.push_back(std::move(sim));  // ✅ Move, don't copy
```

**Problem**: Segment fault during TearDown
**Solution**: Clear shards in proper order

```cpp
void TearDown(...) override {
    shards_.clear();  // ✅ First (destructors run)
    fixture_.reset();  // ✅ Then
}
```

---

## Success Criteria

✅ All 5 sharded benchmarks compile without errors  
✅ Benchmarks run without crashes  
✅ Results show improvement over baseline  
✅ 8-thread case shows >50% improvement (ideally >100%)  
✅ JSON export works  

🎯 **Victory Condition**: 8 Threads @ >1M ops/sec (vs 683k baseline)


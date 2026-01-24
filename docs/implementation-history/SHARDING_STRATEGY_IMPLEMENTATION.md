# Advanced Partitioning and Sharding Strategy Implementation

## Overview

This implementation adds advanced sharded parallel benchmarks to ThemisDB, testing true parallelization without write serialization bottlenecks.

## Implementation Details

### Key Concept

Instead of all threads writing to the **same table** (causing contention), each thread writes to its **own dedicated shard** (eliminating contention).

```
Thread 0 → parallel_shard_0
Thread 1 → parallel_shard_1
Thread 2 → parallel_shard_2
...
Thread N → parallel_shard_N
```

### Files Modified

1. **benchmarks/bench_advanced_patterns.cpp**
   - Added `ParallelityBenchSharded` fixture class
   - Implemented 5 benchmark variants:
     - `ShardedParallel_1Thread` (baseline)
     - `ShardedParallel_4Threads`
     - `ShardedParallel_8Threads`
     - `ShardedParallel_16Threads`
     - `ShardedParallel_32Threads`

2. **benchmarks/CMakeLists.txt**
   - Added CMake target for `bench_advanced_patterns`
   - Configured build dependencies and optimization flags

### Architecture

```cpp
class ParallelityBenchSharded : public benchmark::Fixture {
protected:
    // Setup creates 32 shards (one per potential thread)
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_sharded");
        db_wrapper_ = &fixture_->getDb();
        
        // Pre-create all shards
        for (int i = 0; i < 32; ++i) {
            auto sim = std::make_unique<SecondaryIndexManager>(*db_wrapper_);
            sim->createIndex("parallel_shard_" + std::to_string(i), "id");
            shards_.push_back(std::move(sim));
        }
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    RocksDBWrapper* db_wrapper_;
    std::vector<std::unique_ptr<SecondaryIndexManager>> shards_;
};
```

### Expected Performance Improvements

Based on the sharding strategy document:

| Test | Baseline | Sharded | Expected Improvement |
|------|----------|---------|---------------------|
| 1 Thread | Baseline | Baseline | 0% (reference) |
| 4 Threads | Low | Better | 50-200% |
| 8 Threads | Low | Good | 100-300% |
| 16 Threads | Low | Better | 200-500% |
| 32 Threads | Low | Best | 300-800% |

### How It Works

1. **Setup Phase**: Create 32 SecondaryIndexManager instances, each managing its own shard
2. **Execution Phase**: Each thread writes exclusively to its assigned shard
3. **No Contention**: Threads don't compete for locks on the same table
4. **Parallel I/O**: Multiple threads can write to different SST files simultaneously

### Benefits

- ✅ **Eliminates write serialization** - No shared lock contention
- ✅ **True parallelization** - Each thread works independently
- ✅ **Better CPU utilization** - No threads waiting for locks
- ✅ **Improved cache locality** - Each thread's data stays in its own cache lines

### Limitations

- Memory overhead: 32 separate SecondaryIndexManager instances
- Merge complexity: Reading requires aggregating across shards
- Not production-ready: This is a benchmark/test artifact
- RocksDB may still have internal global locks

## Building and Running

### Build

```bash
cd build
cmake .. -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --target bench_advanced_patterns --config Release
```

### Run

```bash
# Run all sharded benchmarks
./bench_advanced_patterns --benchmark_filter="ShardedParallel"

# Run with JSON output
./bench_advanced_patterns --benchmark_filter="ShardedParallel" \
    --benchmark_format=json --benchmark_out=sharded_results.json

# Run specific thread count
./bench_advanced_patterns --benchmark_filter="ShardedParallel_8Threads"
```

### Analysis

Compare results with baseline benchmarks to see the improvement from sharding:

```bash
# Compare 8-thread performance
./bench_advanced_patterns --benchmark_filter="Phase1Final_8Threads|ShardedParallel_8Threads"
```

## Real-World Applications

This sharding pattern is used in production databases:

- **RocksDB**: Column families for parallel writes
- **MongoDB**: Sharding by key range
- **Cassandra**: Consistent hashing across nodes
- **Kafka**: Multiple partitions for parallel producers

## References

- Implementation based on `PARALLEL_FIX_V2_SHARDING_STRATEGY.md`
- Google Benchmark framework: https://github.com/google/benchmark
- RocksDB LSM tree architecture: https://github.com/facebook/rocksdb/wiki

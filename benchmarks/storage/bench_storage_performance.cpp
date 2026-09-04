/**
 * @file bench_storage_performance.cpp
 * @brief Real Google Benchmark performance tests for storage components
 * 
 * Tests storage performance with:
 * - mimalloc vs system allocator comparison
 * - Huge pages performance (2MB/1GB)
 * - RCU index read/write performance
 * - Memory allocation patterns
 * - Baseline vs optimized variants
 * 
 * Output: JSON format for CI regression tracking
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "performance/allocator.h"
#include "performance/huge_pages.h"
#include "performance/rcu.h"
#include "performance/rcu_hash_table.h"
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <thread>
#include <chrono>
#include <rocksdb/write_batch.h>
#include <rocksdb/utilities/transaction_db.h>

using namespace themis;
using namespace themis::memory;
using namespace themis::rcu;

namespace {

// ═══════════════════════════════════════════════════════════
// Test Data Generators
// ═══════════════════════════════════════════════════════════

std::vector<size_t> generateAllocSizes(size_t count) {
    std::vector<size_t> sizes;
    sizes.reserve(count);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(64, 4096);
    for (size_t i = 0; i < count; ++i) {
        sizes.push_back(dist(rng));
    }
    return sizes;
}

std::vector<std::string> generateKeys(size_t count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        keys.push_back("key_" + std::to_string(i));
    }
    return keys;
}

// ═══════════════════════════════════════════════════════════
// Memory Allocator Benchmarks (mimalloc vs system)
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: System allocator for small allocations
 * Target: <100ns per allocation
 */
static void BM_Allocator_System_Small(benchmark::State& state) {
    const size_t alloc_size = 128;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 1000; ++i) {
            void* ptr = ::operator new(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            ::operator delete(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel("system_alloc");
}
BENCHMARK(BM_Allocator_System_Small);

/**
 * Optimized: ThemisDB allocator (may use mimalloc) for small allocations
 * Target: <80ns per allocation (20% improvement)
 */
static void BM_Allocator_Themis_Small(benchmark::State& state) {
    const size_t alloc_size = 128;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 1000; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Themis_Small);

/**
 * Baseline: System allocator for large allocations
 * Target: <1us per allocation
 */
static void BM_Allocator_System_Large(benchmark::State& state) {
    const size_t alloc_size = 1024 * 1024; // 1MB
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 100; ++i) {
            void* ptr = ::operator new(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            ::operator delete(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetBytesProcessed(state.iterations() * 100 * alloc_size);
    state.SetLabel("system_alloc");
}
BENCHMARK(BM_Allocator_System_Large);

/**
 * Optimized: ThemisDB allocator for large allocations
 * Target: <800ns per allocation (20% improvement)
 */
static void BM_Allocator_Themis_Large(benchmark::State& state) {
    const size_t alloc_size = 1024 * 1024; // 1MB
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 100; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetBytesProcessed(state.iterations() * 100 * alloc_size);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Themis_Large);

/**
 * Mixed allocation pattern (realistic workload)
 */
static void BM_Allocator_Mixed(benchmark::State& state) {
    auto sizes = generateAllocSizes(1000);
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (size_t size : sizes) {
            void* ptr = allocate(size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Mixed);

// ═══════════════════════════════════════════════════════════
// Huge Pages Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Regular pages memory access
 * Target: Baseline for comparison
 */
static void BM_Memory_RegularPages_Sequential(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    void* mem = allocate(size);
    if (!mem) {
        state.SkipWithError("Memory allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    for (auto _ : state) {
        // Sequential access pattern
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        size_t count = size / sizeof(uint64_t);
        
        for (size_t i = 0; i < count; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    state.SetLabel("regular_pages");
    deallocate(mem);
}
BENCHMARK(BM_Memory_RegularPages_Sequential);

/**
 * Optimized: Huge pages memory access (if available)
 * Target: 5-10% improvement due to better TLB utilization
 */
static void BM_Memory_HugePages_Sequential(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    
    if (!huge_pages_available()) {
        state.SkipWithError("Huge pages not available");
        return;
    }
    
    void* mem = allocate_huge_pages(size);
    if (!mem) {
        state.SkipWithError("Huge pages allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    for (auto _ : state) {
        // Sequential access pattern
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        size_t count = size / sizeof(uint64_t);
        
        for (size_t i = 0; i < count; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    state.SetLabel("huge_pages");
    deallocate(mem);
}
BENCHMARK(BM_Memory_HugePages_Sequential);

/**
 * Random access pattern - shows TLB benefits more clearly
 */
static void BM_Memory_RegularPages_Random(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    void* mem = allocate(size);
    if (!mem) {
        state.SkipWithError("Memory allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    // Generate random indices
    std::vector<size_t> indices;
    indices.reserve(10000);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, (size / sizeof(uint64_t)) - 1);
    for (int i = 0; i < 10000; ++i) {
        indices.push_back(dist(rng));
    }
    
    for (auto _ : state) {
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        
        for (size_t idx : indices) {
            sum += data[idx];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.SetLabel("regular_pages_random");
    deallocate(mem);
}
BENCHMARK(BM_Memory_RegularPages_Random);

/**
 * Random access with huge pages
 */
static void BM_Memory_HugePages_Random(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    
    if (!huge_pages_available()) {
        state.SkipWithError("Huge pages not available");
        return;
    }
    
    void* mem = allocate_huge_pages(size);
    if (!mem) {
        state.SkipWithError("Huge pages allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    // Generate random indices
    std::vector<size_t> indices;
    indices.reserve(10000);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, (size / sizeof(uint64_t)) - 1);
    for (int i = 0; i < 10000; ++i) {
        indices.push_back(dist(rng));
    }
    
    for (auto _ : state) {
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        
        for (size_t idx : indices) {
            sum += data[idx];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.SetLabel("huge_pages_random");
    deallocate(mem);
}
BENCHMARK(BM_Memory_HugePages_Random);

// ═══════════════════════════════════════════════════════════
// RCU Index Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * RCU Read Performance - Single Thread
 * Target: <50ns per read
 */
static void BM_RCU_Read_SingleThread(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    // Simulate shared data structure
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        ReadLock lock;
        
        // Read operations
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += (*data)[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetLabel("rcu_read");
}
BENCHMARK(BM_RCU_Read_SingleThread);

/**
 * RCU Write Performance - Update and synchronize
 * Target: <1ms per write (including synchronization)
 */
static void BM_RCU_Write_WithSync(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    GracePeriodManager& mgr = GracePeriodManager::instance();
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        // Copy-modify-update pattern
        auto new_data = std::make_shared<std::vector<int>>(*data);
        (*new_data)[0] = 100;
        
        // Atomic update (in real code)
        data = new_data;
        
        // Wait for readers to finish
        mgr.synchronize_rcu();
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("rcu_write");
}
BENCHMARK(BM_RCU_Write_WithSync);

/**
 * RCU Multi-threaded Read Performance
 * Target: Linear scalability up to 8 threads
 */
static void BM_RCU_Read_MultiThread(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        ReadLock lock;
        
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += (*data)[i % data->size()];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetLabel("rcu_read_mt");
}
BENCHMARK(BM_RCU_Read_MultiThread)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);

// ═══════════════════════════════════════════════════════════
// Memory Usage Tracking
// ═══════════════════════════════════════════════════════════

/**
 * Measure memory overhead of allocator
 */
static void BM_Memory_Overhead(benchmark::State& state) {
    const size_t num_allocations = state.range(0);
    const size_t alloc_size = 128;
    
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocations);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (size_t i = 0; i < num_allocations; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
        }
        
        benchmark::ClobberMemory();
        
        // Cleanup
        state.PauseTiming();
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        state.ResumeTiming();
    }
    
    state.SetBytesProcessed(state.iterations() * num_allocations * alloc_size);
    state.SetLabel(std::string(allocator_name()) + "_overhead");
}
BENCHMARK(BM_Memory_Overhead)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// ═══════════════════════════════════════════════════════════
// Sustained Write Benchmarks (SLO: ≥ 100 k ops/s)
//
// These are the canonical 1:1 benchmarks for the "Storage Sustained Write"
// SLO documented in PERFORMANCE_EXPECTATIONS.md (previously showing only
// 1.276 k/s due to per-write fsync from write_options_->sync=true).
//
// Three configurations are measured:
//  (a) NoSync   – WAL written, no per-write fsync (target: ≥ 100 k ops/s)
//  (b) Batched  – RocksDB WriteBatch of 1024 entries, one sync per batch
//  (c) FullSync – force_sync_on_write=true; characterises max-durability cost
// ═══════════════════════════════════════════════════════════

} // namespace

#include "storage/rocksdb_wrapper.h"
#include "storage/wal_storage.h"
#include <filesystem>
#include <cassert>

namespace {

// ── Fixture helpers ──────────────────────────────────────────────────────────

struct SustainedWriteFixture {
    std::string path = {};
    std::unique_ptr<themis::RocksDBWrapper> db;

    explicit SustainedWriteFixture(themis::RocksDBWrapper::Config cfg) {
        path = (std::filesystem::temp_directory_path() /
                ("themis_swbench_" + std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count())))
               .string();
        cfg.db_path = path;
        cfg.enable_statistics = false;   // reduce micro-benchmark overhead
        db = std::make_unique<themis::RocksDBWrapper>(cfg);
        if (!db->open()) {
            throw std::runtime_error("SustainedWriteFixture: failed to open RocksDB");
        }
    }

    ~SustainedWriteFixture() {
        db.reset();
        std::error_code ec = {};
        std::filesystem::remove_all(path, ec);
    }
};

// ── (a) No per-write fsync — canonical SLO benchmark ─────────────────────────

/**
 * BM_Storage_SustainedWrite_NoSync
 *
 * WAL is written on every put() but NOT fsynced per write.  This is the
 * standard production setting (force_sync_on_write=false).
 *
 * SLO target: ≥ 100 000 ops/s  (10 µs/op)
 * Payload:    256-byte value, 32-byte key
 */
static void BM_Storage_SustainedWrite_NoSync(benchmark::State& state) {
    themis::RocksDBWrapper::Config cfg;
    cfg.force_sync_on_write    = false;
    cfg.disable_wal_for_benchmark = false;   // WAL written, not fsynced
    cfg.memtable_size_mb       = 64;
    cfg.block_cache_size_mb    = 64;

    SustainedWriteFixture fix(cfg);

    const std::string value(256, 'v');
    uint64_t counter = 0;

    for (auto _ : state) {
        std::string key = "sw_nosync_" + std::to_string(counter++);
        fix.db->put(key, value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(value.size() + 20));
    state.SetLabel("nosync_wal");
}
BENCHMARK(BM_Storage_SustainedWrite_NoSync)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ── (b) Batched writes — group commit via WriteBatch ─────────────────────────

/**
 * BM_Storage_SustainedWrite_Batched
 *
 * Groups kBatchSize writes into a single rocksdb::WriteBatch and commits
 * with sync=false.  Shows the throughput benefit of batching.
 *
 * SLO target: ≥ 500 000 ops/s (2 µs/op amortised)
 */
static void BM_Storage_SustainedWrite_Batched(benchmark::State& state) {
    const int kBatchSize = static_cast<int>(state.range(0));

    themis::RocksDBWrapper::Config cfg;
    cfg.force_sync_on_write       = false;
    cfg.disable_wal_for_benchmark = false;
    cfg.memtable_size_mb          = 128;
    cfg.block_cache_size_mb       = 64;

    SustainedWriteFixture fix(cfg);

    const std::string value(256, 'b');
    uint64_t counter = 0;

    rocksdb::WriteOptions wo;
    wo.sync       = false;
    wo.disableWAL = false;

    for (auto _ : state) {
        rocksdb::WriteBatch batch;
        for (int i = 0; i < kBatchSize; ++i) {
            std::string key = "sw_batch_" + std::to_string(counter++);
            batch.Put(rocksdb::Slice(key), rocksdb::Slice(value));
        }
        rocksdb::Status s = fix.db->getRawDB()->Write(wo, &batch);
        if (!s.ok()) {
            state.SkipWithError(("WriteBatch failed: " + s.ToString()).c_str());
            return;
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize *
                            static_cast<int64_t>(value.size() + 20));
    state.SetLabel("batched_wal_batch" + std::to_string(kBatchSize));
}
BENCHMARK(BM_Storage_SustainedWrite_Batched)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ── (c) Full sync — characterise max-durability cost ─────────────────────────

/**
 * BM_Storage_SustainedWrite_FullSync
 *
 * force_sync_on_write=true: every write fsyncs the WAL.  This was the
 * unintentional default behaviour before the sync/enable_wal fix and is
 * the reason for the observed 1.276 k/s baseline.  Retained here so the
 * durability cost is explicitly visible in the benchmark report.
 */
static void BM_Storage_SustainedWrite_FullSync(benchmark::State& state) {
    themis::RocksDBWrapper::Config cfg;
    cfg.force_sync_on_write       = true;    // per-write fsync (max durability)
    cfg.disable_wal_for_benchmark = false;
    cfg.memtable_size_mb          = 64;
    cfg.block_cache_size_mb       = 64;

    SustainedWriteFixture fix(cfg);

    const std::string value(256, 'f');
    uint64_t counter = 0;

    for (auto _ : state) {
        std::string key = "sw_fullsync_" + std::to_string(counter++);
        fix.db->put(key, value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.SetLabel("fullsync_wal");
}
BENCHMARK(BM_Storage_SustainedWrite_FullSync)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ── (d) WAL group-commit — appendBatch vs N × appendPut ──────────────────────

/**
 * BM_WAL_GroupCommit_Batch
 *
 * Measures throughput of WALStorage::appendBatch() vs the same number of
 * individual appendPut() calls, demonstrating the group-commit speedup.
 */
static void BM_WAL_GroupCommit_Batch(benchmark::State& state) {
    const int kBatchSize = static_cast<int>(state.range(0));

    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string dir = (std::filesystem::temp_directory_path() /
                       ("themis_wal_gc_" + std::to_string(ts))).string();
    std::filesystem::create_directories(dir);

    themis::WALStorage::Config cfg;
    cfg.dir                     = dir;
    cfg.fsync_on_write          = true;   // make the group-commit benefit visible
    cfg.rotation_threshold_bytes = 64 * 1024 * 1024;

    auto wal_res = themis::WALStorage::open(cfg);
    if (!wal_res) {
        state.SkipWithError("WALStorage::open failed");
        return;
    }
    auto& wal = *wal_res;

    const std::string value(128, 'g');
    uint64_t counter = 0;

    for (auto _ : state) {
        std::vector<themis::WALStorage::BatchEntry> entries;
        entries.reserve(static_cast<size_t>(kBatchSize));
        std::vector<std::string> keys(static_cast<size_t>(kBatchSize));
        for (int i = 0; i < kBatchSize; ++i) {
            keys[static_cast<size_t>(i)] = "gc_" + std::to_string(counter++);
            entries.push_back({themis::WALStorage::EntryType::PUT,
                               keys[static_cast<size_t>(i)],
                               value});
        }
        auto res = wal->appendBatch(std::move(entries));
        if (!res) {
            state.SkipWithError(("appendBatch failed: " + res.error().context()).c_str());
            return;
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
    state.SetLabel("wal_group_commit_batch" + std::to_string(kBatchSize));

    wal.reset();
    std::error_code ec = {};
    std::filesystem::remove_all(dir, ec);
}
BENCHMARK(BM_WAL_GroupCommit_Batch)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

} // namespace (sustained write benchmarks)

// ═══════════════════════════════════════════════════════════
// Main - Configure JSON output for CI
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();

// v1.3.0 Phase 2: Async I/O MultiScan Benchmarks
// Performance benchmarks for asynchronous I/O with prefetching

#include <benchmark/benchmark.h>
#include "storage/rocksdb_wrapper.h"
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include <future>

using namespace themis;

// Global test database for benchmarks
static std::unique_ptr<RocksDBWrapper> g_db_sync;
static std::unique_ptr<RocksDBWrapper> g_db_async;
static const std::string test_db_path_sync = "/tmp/bench_async_io_sync";
static const std::string test_db_path_async = "/tmp/bench_async_io_async";
static bool g_initialized = false;

// Initialize databases with test data
static void InitializeDatabases() {
    if (g_initialized) return;

    // Clean up
    std::filesystem::remove_all(test_db_path_sync);
    std::filesystem::remove_all(test_db_path_async);

    // Sync I/O database
    RocksDBWrapper::Config config_sync;
    config_sync.db_path = test_db_path_sync;
    config_sync.memtable_size_mb = 256;
    config_sync.block_cache_size_mb = 512;
    config_sync.enable_async_io = false;  // Sync I/O

    g_db_sync = std::make_unique<RocksDBWrapper>(config_sync);
    if (!g_db_sync->open()) {
        throw std::runtime_error("Failed to open sync database");
    }

    // Async I/O database
    RocksDBWrapper::Config config_async;
    config_async.db_path = test_db_path_async;
    config_async.memtable_size_mb = 256;
    config_async.block_cache_size_mb = 512;
    config_async.enable_async_io = true;  // Async I/O enabled
    config_async.async_io_readahead_size_mb = 64;  // 64MB prefetch buffer

    g_db_async = std::make_unique<RocksDBWrapper>(config_async);
    if (!g_db_async->open()) {
        throw std::runtime_error("Failed to open async database");
    }

    // Insert test data into both databases
    const int num_records = 100000;  // 100K records
    for (int i = 0; i < num_records; ++i) {
        std::string key = "benchmark_key_" + std::to_string(i);
        std::vector<uint8_t> value(2048, static_cast<uint8_t>(i % 256));  // 2KB values
        
        g_db_sync->put(key, value);
        g_db_async->put(key, value);
    }

    g_initialized = true;
}

// Benchmark 1: Sequential Scan - Sync I/O
static void BM_SequentialScan_Sync(benchmark::State& state) {
    InitializeDatabases();
    
    int num_records = state.range(0);
    for (auto _ : state) {
        auto results = g_db_sync->scanWithAsyncIO("benchmark_key_", num_records);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_records);
    state.SetBytesProcessed(state.iterations() * num_records * 2048);
}
BENCHMARK(BM_SequentialScan_Sync)->Arg(1000)->Arg(10000)->Arg(50000);

// Benchmark 2: Sequential Scan - Async I/O
static void BM_SequentialScan_Async(benchmark::State& state) {
    InitializeDatabases();
    
    int num_records = state.range(0);
    for (auto _ : state) {
        auto results = g_db_async->scanWithAsyncIO("benchmark_key_", num_records);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_records);
    state.SetBytesProcessed(state.iterations() * num_records * 2048);
}
BENCHMARK(BM_SequentialScan_Async)->Arg(1000)->Arg(10000)->Arg(50000);

// Benchmark 3: MultiGet - Sync I/O
static void BM_MultiGet_Sync(benchmark::State& state) {
    InitializeDatabases();
    
    int num_keys = state.range(0);
    std::vector<std::string> keys;
    for (int i = 0; i < num_keys; ++i) {
        keys.push_back("benchmark_key_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto results = g_db_sync->multiGet(keys);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_keys);
}
BENCHMARK(BM_MultiGet_Sync)->Arg(100)->Arg(500)->Arg(1000);

// Benchmark 4: MultiGet - Async I/O
static void BM_MultiGet_Async(benchmark::State& state) {
    InitializeDatabases();
    
    int num_keys = state.range(0);
    std::vector<std::string> keys;
    for (int i = 0; i < num_keys; ++i) {
        keys.push_back("benchmark_key_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto results = g_db_async->multiGetWithAsyncIO(keys);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_keys);
}
BENCHMARK(BM_MultiGet_Async)->Arg(100)->Arg(500)->Arg(1000);

// Benchmark 5: Iterator Throughput - Sync I/O
static void BM_IteratorThroughput_Sync(benchmark::State& state) {
    InitializeDatabases();
    
    int num_records = state.range(0);
    for (auto _ : state) {
        auto it = g_db_sync->newIterator();
        int count = 0;
        it->SeekToFirst();
        while (it->Valid() && count < num_records) {
            benchmark::DoNotOptimize(it->key());
            benchmark::DoNotOptimize(it->value());
            it->Next();
            count++;
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_records);
}
BENCHMARK(BM_IteratorThroughput_Sync)->Arg(1000)->Arg(10000);

// Benchmark 6: Iterator Throughput - Async I/O
static void BM_IteratorThroughput_Async(benchmark::State& state) {
    InitializeDatabases();
    
    int num_records = state.range(0);
    for (auto _ : state) {
        auto it = g_db_async->newAsyncIterator();
        int count = 0;
        it->SeekToFirst();
        while (it->Valid() && count < num_records) {
            benchmark::DoNotOptimize(it->key());
            benchmark::DoNotOptimize(it->value());
            it->Next();
            count++;
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_records);
}
BENCHMARK(BM_IteratorThroughput_Async)->Arg(1000)->Arg(10000);

// Benchmark 7: Range Query - Sync I/O
static void BM_RangeQuery_Sync(benchmark::State& state) {
    InitializeDatabases();
    
    std::string start_key = "benchmark_key_10000";
    std::string end_key = "benchmark_key_20000";
    
    for (auto _ : state) {
        auto results = g_db_sync->rangeQueryWithAsyncIO(start_key, end_key);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_RangeQuery_Sync);

// Benchmark 8: Range Query - Async I/O
static void BM_RangeQuery_Async(benchmark::State& state) {
    InitializeDatabases();
    
    std::string start_key = "benchmark_key_10000";
    std::string end_key = "benchmark_key_20000";
    
    for (auto _ : state) {
        auto results = g_db_async->rangeQueryWithAsyncIO(start_key, end_key);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_RangeQuery_Async);

// Benchmark 9: Prefetch Buffer Size Impact
static void BM_PrefetchBufferSize(benchmark::State& state) {
    std::filesystem::remove_all("/tmp/bench_prefetch_test");
    
    int buffer_size_mb = state.range(0);
    
    RocksDBWrapper::Config config;
    config.db_path = "/tmp/bench_prefetch_test";
    config.enable_async_io = true;
    config.async_io_readahead_size_mb = buffer_size_mb;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    
    // Insert test data
    for (int i = 0; i < 10000; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::vector<uint8_t> value(2048, static_cast<uint8_t>(i % 256));
        db->put(key, value);
    }
    
    for (auto _ : state) {
        auto results = db->scanWithAsyncIO("", 10000);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetLabel("Buffer: " + std::to_string(buffer_size_mb) + "MB");
    std::filesystem::remove_all("/tmp/bench_prefetch_test");
}
BENCHMARK(BM_PrefetchBufferSize)->Arg(16)->Arg(32)->Arg(64)->Arg(128);

// Benchmark 10: Reverse Scan - Async I/O
static void BM_ReverseScan_Async(benchmark::State& state) {
    InitializeDatabases();
    
    int num_records = state.range(0);
    for (auto _ : state) {
        auto results = g_db_async->reverseScanWithAsyncIO("benchmark_key_99999", num_records);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_records);
}
BENCHMARK(BM_ReverseScan_Async)->Arg(1000)->Arg(5000);

// Benchmark 11: Large Value Scan - Async I/O (1MB values)
static void BM_LargeValueScan_Async(benchmark::State& state) {
    std::filesystem::remove_all("/tmp/bench_large_values");
    
    RocksDBWrapper::Config config;
    config.db_path = "/tmp/bench_large_values";
    config.enable_async_io = true;
    config.async_io_readahead_size_mb = 128;
    config.enable_blobdb = true;  // Use BlobDB for large values
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    
    // Insert large values (1MB each)
    const int num_records = 100;
    const int value_size = 1024 * 1024;
    for (int i = 0; i < num_records; ++i) {
        std::string key = "large_key_" + std::to_string(i);
        std::vector<uint8_t> value(value_size, static_cast<uint8_t>(i % 256));
        db->put(key, value);
    }
    
    for (auto _ : state) {
        auto results = db->scanWithAsyncIO("", num_records);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetBytesProcessed(state.iterations() * num_records * value_size);
    std::filesystem::remove_all("/tmp/bench_large_values");
}
BENCHMARK(BM_LargeValueScan_Async);

// Benchmark 12: Concurrent Async Scans
static void BM_ConcurrentAsyncScans(benchmark::State& state) {
    InitializeDatabases();
    
    int num_threads = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::future<std::vector<std::pair<std::string, std::vector<uint8_t>>>>> futures;
        
        for (int i = 0; i < num_threads; ++i) {
            futures.push_back(std::async(std::launch::async, [i]() {
                int start = i * 1000;
                std::string prefix = "benchmark_key_" + std::to_string(start);
                return g_db_async->scanWithAsyncIO(prefix, 500);
            }));
        }
        
        for (auto& future : futures) {
            auto results = future.get();
            benchmark::DoNotOptimize(results);
        }
    }
    
    state.SetLabel("Threads: " + std::to_string(num_threads));
}
BENCHMARK(BM_ConcurrentAsyncScans)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();

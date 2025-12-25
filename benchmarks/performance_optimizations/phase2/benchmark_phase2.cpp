// Performance benchmarks for Phase 2 Optimizations
// Based on scientific research from PR #156/#157
// Uses Google Benchmark for consistent performance measurement

#include <benchmark/benchmark.h>
#include "performance/wisckey.h"
#include "performance/dostoevsky.h"
#include "performance/cicada.h"
#include "performance/ligra.h"
#include "performance/rabitq.h"
#include <random>
#include <filesystem>

using namespace themis::performance;

// ==================== WiscKey Benchmarks ====================

static void BM_WiscKey_SmallValueWrite(benchmark::State& state) {
    auto log_path = std::filesystem::temp_directory_path() / "wisckey_bench_small.log";
    WiscKeyStorage storage(log_path.string());
    
    std::string key = "test_key";
    std::string small_value = "small_value_less_than_1kb";
    
    for (auto _ : state) {
        auto encoded = storage.put(key, small_value);
        benchmark::DoNotOptimize(encoded);
    }
    
    std::filesystem::remove(log_path);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WiscKey_SmallValueWrite);

static void BM_WiscKey_LargeValueWrite(benchmark::State& state) {
    auto log_path = std::filesystem::temp_directory_path() / "wisckey_bench_large.log";
    WiscKeyStorage storage(log_path.string());
    
    std::string key = "test_key";
    std::string large_value(10000, 'x');  // 10KB value
    
    for (auto _ : state) {
        auto encoded = storage.put(key, large_value);
        benchmark::DoNotOptimize(encoded);
    }
    
    std::filesystem::remove(log_path);
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * large_value.size());
}
BENCHMARK(BM_WiscKey_LargeValueWrite);

static void BM_WiscKey_MixedWorkload(benchmark::State& state) {
    auto log_path = std::filesystem::temp_directory_path() / "wisckey_bench_mixed.log";
    WiscKeyStorage storage(log_path.string());
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<> size_dist(100, 5000);
    
    for (auto _ : state) {
        int size = size_dist(gen);
        std::string value(size, 'x');
        auto encoded = storage.put("key", value);
        benchmark::DoNotOptimize(encoded);
    }
    
    std::filesystem::remove(log_path);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WiscKey_MixedWorkload);

// ==================== Dostoevsky Benchmarks ====================

static void BM_Dostoevsky_PolicyComputation(benchmark::State& state) {
    DostoevskeyLSM lsm(10);
    WorkloadStats stats;
    
    // Simulate workload
    for (int i = 0; i < 1000; i++) {
        stats.record_read();
        if (i % 3 == 0) stats.record_write();
    }
    
    for (auto _ : state) {
        auto policy = lsm.compute_optimal_policy(0, stats);
        benchmark::DoNotOptimize(policy);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dostoevsky_PolicyComputation);

static void BM_Dostoevsky_WorkloadMonitoring(benchmark::State& state) {
    WorkloadMonitor monitor;
    std::mt19937 gen(42);
    std::uniform_int_distribution<> op_dist(0, 1);
    
    for (auto _ : state) {
        if (op_dist(gen) == 0) {
            monitor.record_read();
        } else {
            monitor.record_write();
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dostoevsky_WorkloadMonitoring);

// ==================== Cicada Benchmarks ====================

static void BM_Cicada_RecordLocking(benchmark::State& state) {
    CicadaRecord record;
    
    for (auto _ : state) {
        bool locked = record.try_lock();
        benchmark::DoNotOptimize(locked);
        if (locked) {
            record.unlock_and_increment_version();
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Cicada_RecordLocking);

static void BM_Cicada_TransactionCommit(benchmark::State& state) {
    std::vector<CicadaRecord> records(10);
    
    for (auto _ : state) {
        CicadaTransaction txn;
        
        // Read phase
        for (auto& record : records) {
            txn.record_read(&record, record.get_version());
        }
        
        // Write phase
        txn.record_write(&records[0]);
        
        // Commit
        bool success = txn.commit();
        benchmark::DoNotOptimize(success);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Cicada_TransactionCommit);

static void BM_Cicada_HighContention(benchmark::State& state) {
    CicadaRecord shared_record;
    
    for (auto _ : state) {
        CicadaTransaction txn;
        txn.record_read(&shared_record, shared_record.get_version());
        txn.record_write(&shared_record);
        
        bool success = txn.commit();
        benchmark::DoNotOptimize(success);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Cicada_HighContention)->Threads(1)->Threads(4)->Threads(8);

// ==================== Ligra Benchmarks ====================

static void BM_Ligra_FrontierOperations(benchmark::State& state) {
    Frontier f(10000);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dist(0, 9999);
    
    for (auto _ : state) {
        NodeID node = dist(gen);
        f.add(node);
        benchmark::DoNotOptimize(f.contains(node));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ligra_FrontierOperations);

static void BM_Ligra_BFS(benchmark::State& state) {
    int num_nodes = state.range(0);
    
    // Create chain graph: 0 -> 1 -> 2 -> ... -> n
    std::vector<std::vector<NodeID>> adj_list(num_nodes);
    for (int i = 0; i < num_nodes - 1; i++) {
        adj_list[i].push_back(i + 1);
    }
    
    LigraProcessor processor(num_nodes);
    
    for (auto _ : state) {
        auto distances = processor.parallel_bfs(0, adj_list);
        benchmark::DoNotOptimize(distances);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_Ligra_BFS)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_Ligra_PageRank(benchmark::State& state) {
    int num_nodes = 100;
    
    // Create small graph
    std::vector<std::vector<NodeID>> adj_list(num_nodes);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dist(0, num_nodes - 1);
    
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < 5; j++) {  // 5 edges per node
            adj_list[i].push_back(dist(gen));
        }
    }
    
    LigraProcessor processor(num_nodes);
    
    for (auto _ : state) {
        auto ranks = processor.parallel_pagerank(adj_list, 5);
        benchmark::DoNotOptimize(ranks);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_Ligra_PageRank);

// ==================== RaBitQ Benchmarks ====================

static void BM_RaBitQ_Encoding(benchmark::State& state) {
    int dimension = state.range(0);
    RaBitQEncoder encoder(dimension);
    
    // Train encoder
    std::vector<std::vector<float>> training(100, std::vector<float>(dimension, 1.0f));
    encoder.train(training);
    
    std::vector<float> vec(dimension, 0.5f);
    
    for (auto _ : state) {
        auto quantized = encoder.encode(vec);
        benchmark::DoNotOptimize(quantized);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * dimension * sizeof(float));
}
BENCHMARK(BM_RaBitQ_Encoding)->Arg(128)->Arg(512)->Arg(1024);

static void BM_RaBitQ_Decoding(benchmark::State& state) {
    int dimension = state.range(0);
    RaBitQEncoder encoder(dimension);
    
    std::vector<std::vector<float>> training(100, std::vector<float>(dimension, 1.0f));
    encoder.train(training);
    
    std::vector<float> vec(dimension, 0.5f);
    auto quantized = encoder.encode(vec);
    
    for (auto _ : state) {
        auto decoded = encoder.decode(quantized);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RaBitQ_Decoding)->Arg(128)->Arg(512)->Arg(1024);

static void BM_RaBitQ_DistanceComputation(benchmark::State& state) {
    int dimension = 128;
    RaBitQEncoder encoder(dimension);
    
    std::vector<std::vector<float>> training(100, std::vector<float>(dimension, 1.0f));
    encoder.train(training);
    
    std::vector<float> query(dimension, 0.5f);
    std::vector<float> db_vec(dimension, 0.6f);
    auto quantized_db = encoder.encode(db_vec);
    
    for (auto _ : state) {
        float dist = encoder.asymmetric_distance(query, quantized_db);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RaBitQ_DistanceComputation);

static void BM_RaBitQ_IndexSearch(benchmark::State& state) {
    int num_vectors = state.range(0);
    int dimension = 128;
    
    RaBitQIndex index(dimension);
    
    // Train and add vectors
    std::vector<std::vector<float>> training(100, std::vector<float>(dimension));
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (auto& vec : training) {
        for (auto& val : vec) val = dist(gen);
    }
    index.train(training);
    
    for (int i = 0; i < num_vectors; i++) {
        std::vector<float> vec(dimension);
        for (auto& val : vec) val = dist(gen);
        index.add(i, vec);
    }
    
    std::vector<float> query(dimension);
    for (auto& val : query) val = dist(gen);
    
    for (auto _ : state) {
        auto results = index.search(query, 10);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * num_vectors);
}
BENCHMARK(BM_RaBitQ_IndexSearch)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_RaBitQ_MemoryCompression(benchmark::State& state) {
    int dimension = 128;
    RaBitQIndex index(dimension);
    
    std::vector<std::vector<float>> training(100, std::vector<float>(dimension, 1.0f));
    index.train(training);
    
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<float> vec(dimension);
        for (auto& val : vec) val = dist(gen);
        state.ResumeTiming();
        
        index.add(state.iterations(), vec);
        
        state.PauseTiming();
        auto stats = index.get_memory_stats();
        benchmark::DoNotOptimize(stats);
        state.ResumeTiming();
    }
}
BENCHMARK(BM_RaBitQ_MemoryCompression);

BENCHMARK_MAIN();

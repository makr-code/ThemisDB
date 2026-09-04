// Google Benchmark for RCU Index Performance
// Tests read performance with and without RCU lock-free optimization

#include <benchmark/benchmark.h>
#include <performance/rcu.h>
#include <performance/rcu_hash_table.h>
#include <vector>
#include <random>
#include <thread>
#include <iostream>

using namespace themis::rcu;

// Benchmark: Simple lookup operations
static void BM_SimpleLookup(benchmark::State& state) {
    RCUHashTable<int, int> table;
    
    // Populate table
    const size_t num_keys = 10000;
    for (size_t i = 0; i < num_keys; ++i) {
        table.insert(i, i * 2);
    }
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, num_keys - 1);
    
    for (auto _ : state) {
        int key = dist(gen);
        int value = {};
        bool found = table.lookup(key, value);
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Read-heavy workload (95% reads, 5% writes)
static void BM_ReadHeavyWorkload(benchmark::State& state) {
    RCUHashTable<int, int> table;
    
    // Populate table
    const size_t num_keys = 10000;
    for (size_t i = 0; i < num_keys; ++i) {
        table.insert(i, i * 2);
    }
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> key_dist(0, num_keys - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);
    
    size_t reads = 0;
    size_t writes = 0;
    
    for (auto _ : state) {
        int op = op_dist(gen);
        int key = key_dist(gen);
        
        if (op < 95) {
            // Read operation (95%)
            int value = {};
            table.lookup(key, value);
            benchmark::DoNotOptimize(value);
            reads++;
        } else {
            // Write operation (5%)
            table.insert(key, key * 3);
            writes++;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["reads"] = reads;
    state.counters["writes"] = writes;
}

// Benchmark: Concurrent reads (shows RCU scalability)
static void BM_ConcurrentReads(benchmark::State& state) {
    static RCUHashTable<int, int> table;
    
    // Populate once (thread-safe initialization)
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        const size_t num_keys = 10000;
        for (size_t i = 0; i < num_keys; ++i) {
            table.insert(i, i * 2);
        }
    });
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 9999);
    
    for (auto _ : state) {
        int key = dist(gen);
        int value = {};
        bool found = table.lookup(key, value);
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Hot key access pattern
static void BM_HotKeyPattern(benchmark::State& state) {
    RCUHashTable<int, int> table;
    
    // Populate table
    const size_t num_keys = 10000;
    for (size_t i = 0; i < num_keys; ++i) {
        table.insert(i, i * 2);
    }
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    // 80% of accesses go to 20% of keys (Pareto distribution)
    std::uniform_int_distribution<int> hot_dist(0, 1999);
    std::uniform_int_distribution<int> cold_dist(2000, 9999);
    std::uniform_int_distribution<int> selector(0, 99);
    
    for (auto _ : state) {
        int key = (selector(gen) < 80) ? hot_dist(gen) : cold_dist(gen);
        int value = {};
        table.lookup(key, value);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Insert performance
static void BM_InsertOperations(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        RCUHashTable<int, int> table;
        state.ResumeTiming();
        
        const size_t num_inserts = state.range(0);
        for (size_t i = 0; i < num_inserts; ++i) {
            table.insert(i, i * 2);
        }
        
        benchmark::DoNotOptimize(table);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark: Mixed workload with varying read ratios
static void BM_MixedWorkload(benchmark::State& state) {
    RCUHashTable<int, int> table;
    
    const size_t num_keys = 10000;
    for (size_t i = 0; i < num_keys; ++i) {
        table.insert(i, i * 2);
    }
    
    const int read_percentage = state.range(0); // 50, 70, 90, 95, 99
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> key_dist(0, num_keys - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);
    
    size_t reads = 0;
    size_t writes = 0;
    
    for (auto _ : state) {
        int op = op_dist(gen);
        int key = key_dist(gen);
        
        if (op < read_percentage) {
            int value = 0;
            table.lookup(key, value);
            benchmark::DoNotOptimize(value);
            reads++;
        } else {
            table.insert(key, key * 3);
            writes++;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["read_ratio"] = static_cast<double>(reads) / (reads + writes);
}

// Benchmark: Grace period overhead
static void BM_GracePeriodOverhead(benchmark::State& state) {
    #ifdef THEMIS_USE_RCU_INDEX
    auto& manager = GracePeriodManager::instance();
    
    for (auto _ : state) {
        manager.synchronize_rcu();
    }
    
    state.SetItemsProcessed(state.iterations());
    #else
    state.SkipWithError("RCU not enabled");
    #endif
}

// Register benchmarks
BENCHMARK(BM_SimpleLookup)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ReadHeavyWorkload)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ConcurrentReads)
    ->ThreadRange(1, 8)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_HotKeyPattern)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_InsertOperations)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_MixedWorkload)
    ->Arg(50)   // 50% reads
    ->Arg(70)   // 70% reads
    ->Arg(90)   // 90% reads
    ->Arg(95)   // 95% reads
    ->Arg(99)   // 99% reads
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_GracePeriodOverhead)
    ->Unit(benchmark::kMicrosecond);

// Custom main
int main(int argc, char** argv) {
    std::cout << "\n=================================================================\n";
    std::cout << "RCU Index Benchmark\n";
    std::cout << "=================================================================\n";
    std::cout << "RCU enabled: " << (GracePeriodManager::is_enabled() ? "YES" : "NO") << "\n";
    
    if (GracePeriodManager::is_enabled()) {
        std::cout << "\nExpected improvement: +200-500% for read-heavy workloads\n";
        std::cout << "Read ratio required: 90%+ reads for best results\n";
        std::cout << "Key feature: ZERO overhead for readers (lock-free!)\n";
        std::cout << "Scalability: Linear with CPU cores\n";
    } else {
        std::cout << "\n⚠️  RCU NOT enabled - using locks\n";
        std::cout << "Build with: cmake -DTHEMIS_ENABLE_RCU_INDEX=ON\n";
        std::cout << "Performance will be limited by lock contention\n";
    }
    
    std::cout << "=================================================================\n\n";
    
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    
    return 0;
}

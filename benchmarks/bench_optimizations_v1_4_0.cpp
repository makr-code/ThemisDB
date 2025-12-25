// Google Benchmark suite for ThemisDB Optimizations (v1.4.0)
// Benchmarks for: HNSW parameter tuning, WriteBatch API, gRPC protocol
// Date: December 25, 2024

#include <benchmark/benchmark.h>
#include "rocksdb_wrapper.h"
#include <memory>
#include <random>
#include <string>
#include <vector>

// =============================================================================
// Benchmark Fixtures
// =============================================================================

class WriteBatchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "/tmp/themis_bench_writebatch_" + std::to_string(state.thread_index());
        db_ = std::make_shared<RocksDBWrapper>(db_path_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::system(("rm -rf " + db_path_).c_str());
    }
    
    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// =============================================================================
// WriteBatch Benchmarks
// =============================================================================

// Benchmark: Individual PUT operations (baseline)
static void BM_IndividualPuts(benchmark::State& state) {
    std::string db_path = "/tmp/themis_bench_individual";
    auto db = std::make_shared<RocksDBWrapper>(db_path);
    
    int op_count = 0;
    for (auto _ : state) {
        std::string key = "key_" + std::to_string(op_count++);
        std::string value = "value_" + std::to_string(op_count);
        db->put(key, value);
    }
    
    state.SetItemsProcessed(state.iterations());
    db.reset();
    std::system(("rm -rf " + db_path).c_str());
}
BENCHMARK(BM_IndividualPuts)->Threads(1)->Threads(4)->Threads(8);

// Benchmark: WriteBatch operations (optimized)
BENCHMARK_DEFINE_F(WriteBatchFixture, BatchPuts)(benchmark::State& state) {
    const int batch_size = state.range(0);
    int op_count = 0;
    
    for (auto _ : state) {
        auto batch = db_->createWriteBatch();
        
        for (int i = 0; i < batch_size; ++i) {
            std::string key = "batch_key_" + std::to_string(op_count++);
            std::string value = "batch_value_" + std::to_string(op_count);
            batch->put(key, value);
        }
        
        benchmark::DoNotOptimize(batch->commit());
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK_REGISTER_F(WriteBatchFixture, BatchPuts)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Threads(1)
    ->Threads(4);

// Benchmark: Mixed PUT/DELETE operations
BENCHMARK_DEFINE_F(WriteBatchFixture, MixedOperations)(benchmark::State& state) {
    const int batch_size = state.range(0);
    
    // Pre-populate some keys for deletion
    for (int i = 0; i < batch_size; ++i) {
        db_->put("delete_key_" + std::to_string(i), "value");
    }
    
    int op_count = 0;
    for (auto _ : state) {
        auto batch = db_->createWriteBatch();
        
        // 70% PUTs, 30% DELETEs
        for (int i = 0; i < batch_size; ++i) {
            if (i < batch_size * 0.7) {
                batch->put("new_key_" + std::to_string(op_count++), "value");
            } else {
                batch->delete_key("delete_key_" + std::to_string(i));
            }
        }
        
        benchmark::DoNotOptimize(batch->commit());
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK_REGISTER_F(WriteBatchFixture, MixedOperations)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000);

// =============================================================================
// HNSW Parameter Benchmarks
// =============================================================================

// Simulate HNSW search with different ef_search parameters
static void BM_HNSW_SearchSpeed(benchmark::State& state) {
    const int ef_search = state.range(0);
    const int num_queries = 100;
    
    // Simulate search with different ef_search values
    // In real implementation, this would call actual HNSW search
    for (auto _ : state) {
        for (int q = 0; q < num_queries; ++q) {
            // Simulate search cost: O(ef_search × log(N))
            // Simplified simulation
            int operations = ef_search * 10;  // Simplified
            benchmark::DoNotOptimize(operations);
            
            // Simulate computation
            volatile int sum = 0;
            for (int i = 0; i < operations; ++i) {
                sum += i;
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_queries);
    state.counters["ef_search"] = ef_search;
}
BENCHMARK(BM_HNSW_SearchSpeed)
    ->Arg(16)   // Fast (low recall)
    ->Arg(32)   // Speed preset
    ->Arg(48)   // Speed-balanced
    ->Arg(64)   // Balanced preset
    ->Arg(96)   // Production preset
    ->Arg(128)  // Quality preset
    ->Arg(256); // Maximum quality

// Benchmark: HNSW memory access patterns
static void BM_HNSW_MemoryFootprint(benchmark::State& state) {
    const int M = state.range(0);
    const int num_vectors = 10000;
    
    // Simulate memory allocation for HNSW graph
    size_t bytes_per_vector = M * 4 * 2;  // M × sizeof(int) × 2 (bidirectional)
    size_t total_bytes = bytes_per_vector * num_vectors;
    
    for (auto _ : state) {
        // Simulate memory access
        std::vector<char> memory(total_bytes);
        benchmark::DoNotOptimize(memory.data());
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * total_bytes);
    state.counters["M"] = M;
    state.counters["memory_MB"] = static_cast<double>(total_bytes) / (1024 * 1024);
}
BENCHMARK(BM_HNSW_MemoryFootprint)
    ->Arg(8)   // Memory preset
    ->Arg(12)  // Speed preset
    ->Arg(16)  // Balanced preset
    ->Arg(24)  // Production preset
    ->Arg(32)  // Quality preset
    ->Arg(48); // Maximum quality

// =============================================================================
// Protocol Comparison Benchmarks
// =============================================================================

// Simulate JSON serialization (HTTP/REST)
static void BM_JSON_Serialization(benchmark::State& state) {
    const int entity_size = state.range(0);
    
    // Generate sample data
    std::string entity_data(entity_size, 'x');
    
    for (auto _ : state) {
        // Simulate JSON encoding
        std::string json = "{\"uuid\":\"doc_001\",\"data\":\"" + entity_data + "\"}";
        benchmark::DoNotOptimize(json);
        
        // JSON parsing simulation
        size_t pos = json.find("data");
        benchmark::DoNotOptimize(pos);
    }
    
    state.SetBytesProcessed(state.iterations() * entity_size);
    state.counters["protocol"] = 0; // 0 = JSON
}
BENCHMARK(BM_JSON_Serialization)
    ->Arg(100)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400);

// Simulate Protocol Buffers serialization (gRPC)
static void BM_Protobuf_Serialization(benchmark::State& state) {
    const int entity_size = state.range(0);
    
    // Generate sample data
    std::string entity_data(entity_size, 'x');
    
    for (auto _ : state) {
        // Simulate Protobuf encoding (typically 30% of JSON overhead)
        std::vector<uint8_t> protobuf;
        protobuf.reserve(entity_size + 20);  // Protobuf overhead is minimal
        
        // Simplified serialization
        protobuf.insert(protobuf.end(), entity_data.begin(), entity_data.end());
        benchmark::DoNotOptimize(protobuf.data());
    }
    
    state.SetBytesProcessed(state.iterations() * entity_size);
    state.counters["protocol"] = 1; // 1 = Protobuf
}
BENCHMARK(BM_Protobuf_Serialization)
    ->Arg(100)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400);

// =============================================================================
// Multi-Agent Workflow Benchmarks
// =============================================================================

// Benchmark: Multi-agent result aggregation with WriteBatch
static void BM_MultiAgent_ResultAggregation(benchmark::State& state) {
    const int num_agents = state.range(0);
    std::string db_path = "/tmp/themis_bench_multiagent";
    auto db = std::make_shared<RocksDBWrapper>(db_path);
    
    for (auto _ : state) {
        auto batch = db->createWriteBatch();
        
        // Each agent writes a result
        for (int i = 0; i < num_agents; ++i) {
            std::string key = "agent_result:" + std::to_string(i);
            std::string value = "{\"agent_id\":" + std::to_string(i) + 
                               ",\"result\":\"analysis_complete\"}";
            batch->put(key, value);
        }
        
        // Update task metadata
        batch->put("task:status", "completed");
        batch->put("task:agent_count", std::to_string(num_agents));
        
        benchmark::DoNotOptimize(batch->commit());
    }
    
    state.SetItemsProcessed(state.iterations() * (num_agents + 2));
    
    db.reset();
    std::system(("rm -rf " + db_path).c_str());
}
BENCHMARK(BM_MultiAgent_ResultAggregation)
    ->Arg(3)   // Small team
    ->Arg(5)   // Typical team
    ->Arg(10)  // Large team
    ->Arg(20); // Very large team

// Benchmark: LoRA adapter batch loading
static void BM_MultiAgent_LoRABatchLoad(benchmark::State& state) {
    const int num_adapters = state.range(0);
    std::string db_path = "/tmp/themis_bench_lora";
    auto db = std::make_shared<RocksDBWrapper>(db_path);
    
    for (auto _ : state) {
        auto batch = db->createWriteBatch();
        
        // Load multiple LoRA adapters atomically
        for (int i = 0; i < num_adapters; ++i) {
            std::string key = "lora:adapter_" + std::to_string(i);
            std::string value = std::string(10240, 'x');  // 10KB adapter data
            batch->put(key, value);
            
            // Metadata
            std::string meta_key = "lora_meta:adapter_" + std::to_string(i);
            batch->put(meta_key, "{\"version\":1,\"size\":10240}");
        }
        
        // Update registry
        batch->put("lora:registry:version", std::to_string(state.iterations()));
        
        benchmark::DoNotOptimize(batch->commit());
    }
    
    state.SetItemsProcessed(state.iterations() * (num_adapters * 2 + 1));
    state.SetBytesProcessed(state.iterations() * num_adapters * 10240);
    
    db.reset();
    std::system(("rm -rf " + db_path).c_str());
}
BENCHMARK(BM_MultiAgent_LoRABatchLoad)
    ->Arg(1)
    ->Arg(5)
    ->Arg(10);

// =============================================================================
// Throughput Comparison Benchmarks
// =============================================================================

// Comprehensive throughput test: Individual vs Batched
static void BM_Throughput_Comparison(benchmark::State& state) {
    const bool use_batch = state.range(0);
    const int ops_per_iteration = 100;
    
    std::string db_path = "/tmp/themis_bench_throughput";
    auto db = std::make_shared<RocksDBWrapper>(db_path);
    
    int op_count = 0;
    
    for (auto _ : state) {
        if (use_batch) {
            // Batched approach
            auto batch = db->createWriteBatch();
            for (int i = 0; i < ops_per_iteration; ++i) {
                batch->put("key_" + std::to_string(op_count++), "value");
            }
            batch->commit();
        } else {
            // Individual approach
            for (int i = 0; i < ops_per_iteration; ++i) {
                db->put("key_" + std::to_string(op_count++), "value");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * ops_per_iteration);
    state.counters["approach"] = use_batch ? 1 : 0;  // 0=individual, 1=batched
    
    db.reset();
    std::system(("rm -rf " + db_path).c_str());
}
BENCHMARK(BM_Throughput_Comparison)
    ->Arg(0)  // Individual
    ->Arg(1)  // Batched
    ->Threads(1)
    ->Threads(4)
    ->Threads(8);

// =============================================================================
// Main
// =============================================================================

BENCHMARK_MAIN();

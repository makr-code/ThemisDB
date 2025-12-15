/**
 * @file bench_auto_buffers.cpp
 * @brief Performance benchmarks for AutoBuffer components
 * 
 * Compares buffered vs direct operations:
 * - TSAutoBuffer vs TSStore direct put
 * - VectorAutoBuffer vs VectorIndex direct add
 * - GraphAutoBuffer vs PropertyGraph direct add
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <filesystem>

#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"
#include "index/vector_index.h"
#include "index/vector_auto_buffer.h"
#include "index/graph_auto_buffer.h"

// Global setup
struct BenchmarkFixture {
    std::shared_ptr<themis::RocksDBWrapper> storage;
    std::shared_ptr<themis::TSStore> tsstore;
    std::shared_ptr<themis::VectorIndexManager> vector_index;
    
    BenchmarkFixture() {
        const std::string db_path = "data/themis_benchmark_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 256;
        cfg.block_cache_size_mb = 512;
        
        storage = std::make_shared<themis::RocksDBWrapper>(cfg);
        storage->open();
        
        tsstore = std::make_shared<themis::TSStore>(*storage);
        vector_index = std::make_shared<themis::VectorIndexManager>(*storage);
    }
    
    ~BenchmarkFixture() {
        storage->close();
        const std::string db_path = "data/themis_benchmark_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
    }
};

static BenchmarkFixture* g_fixture = nullptr;

// ===== Time Series Benchmarks =====

static void BM_TSStore_DirectPut(benchmark::State& state) {
    auto& tsstore = g_fixture->tsstore;
    int64_t timestamp = 1700000000;
    
    for (auto _ : state) {
        themis::TSStore::DataPoint point;
        point.metric = "benchmark.metric";
        point.entity = "entity_" + std::to_string(state.iterations());
        point.timestamp = timestamp++;
        point.value = 42.0 + (state.iterations() % 100);
        
        auto status = tsstore->putDataPoint(point);
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSStore_DirectPut)->Threads(1)->Iterations(1000);

static void BM_TSAutoBuffer_BufferedPut(benchmark::State& state) {
    auto& tsstore = g_fixture->tsstore;
    
    themis::timeseries::TSAutoBufferConfig config;
    config.max_points_per_buffer = 10000;  // Large buffer to avoid flushes during benchmark
    config.flush_interval = std::chrono::seconds(3600);
    config.async_flush = false;  // Synchronous for deterministic timing
    
    themis::timeseries::TSAutoBuffer buffer(tsstore.get(), config);
    int64_t timestamp = 1700000000;
    
    for (auto _ : state) {
        themis::TSStore::DataPoint point;
        point.metric = "benchmark.metric";
        point.entity = "entity_" + std::to_string(state.iterations());
        point.timestamp = timestamp++;
        point.value = 42.0 + (state.iterations() % 100);
        
        auto status = buffer.add(point);
        benchmark::DoNotOptimize(status);
    }
    
    // Final flush
    buffer.flush();
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSAutoBuffer_BufferedPut)->Threads(1)->Iterations(1000);

static void BM_TSAutoBuffer_Concurrent(benchmark::State& state) {
    auto& tsstore = g_fixture->tsstore;
    
    themis::timeseries::TSAutoBufferConfig config;
    config.max_points_per_buffer = 10000;
    config.flush_interval = std::chrono::seconds(3600);
    config.async_flush = true;
    
    static themis::timeseries::TSAutoBuffer buffer(tsstore.get(), config);
    static std::once_flag start_flag;
    std::call_once(start_flag, [&]() { buffer.start(); });
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (auto _ : state) {
        themis::TSStore::DataPoint point;
        point.metric = "benchmark.metric";
        point.entity = "entity_" + std::to_string(state.thread_index());
        point.timestamp = 1700000000 + state.iterations();
        point.value = dis(gen);
        
        auto status = buffer.add(point);
        benchmark::DoNotOptimize(status);
    }
    
    if (state.thread_index() == 0) {
        buffer.flush();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSAutoBuffer_Concurrent)->Threads(4)->Iterations(1000);

// ===== Vector Index Benchmarks =====

static void BM_VectorIndex_DirectAdd(benchmark::State& state) {
    auto& vector_index = g_fixture->vector_index;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        themis::BaseEntity entity;
        entity.setPrimaryKey("vec_" + std::to_string(state.iterations()));
        
        // Simulate 384-dim embedding
        nlohmann::json embedding = nlohmann::json::array();
        for (int i = 0; i < 384; ++i) {
            embedding.push_back(dis(gen));
        }
        
        nlohmann::json data = {
            {"embedding", embedding},
            {"metadata", {{"source", "benchmark"}}}
        };
        entity.setData(data);
        
        auto status = vector_index->add(entity);
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VectorIndex_DirectAdd)->Threads(1)->Iterations(100);

static void BM_VectorAutoBuffer_BufferedAdd(benchmark::State& state) {
    auto& vector_index = g_fixture->vector_index;
    
    themis::index::VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 10000;
    config.flush_interval = std::chrono::seconds(3600);
    config.async_flush = false;
    
    themis::index::VectorAutoBuffer buffer(vector_index.get(), config);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        themis::BaseEntity entity;
        entity.setPrimaryKey("vec_" + std::to_string(state.iterations()));
        
        // Simulate 384-dim embedding
        nlohmann::json embedding = nlohmann::json::array();
        for (int i = 0; i < 384; ++i) {
            embedding.push_back(dis(gen));
        }
        
        nlohmann::json data = {
            {"embedding", embedding},
            {"metadata", {{"source", "benchmark"}}}
        };
        entity.setData(data);
        
        auto status = buffer.add(entity);
        benchmark::DoNotOptimize(status);
    }
    
    buffer.flush();
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VectorAutoBuffer_BufferedAdd)->Threads(1)->Iterations(100);

static void BM_VectorAutoBuffer_Concurrent(benchmark::State& state) {
    auto& vector_index = g_fixture->vector_index;
    
    themis::index::VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 10000;
    config.flush_interval = std::chrono::seconds(3600);
    config.async_flush = true;
    
    static themis::index::VectorAutoBuffer buffer(vector_index.get(), config);
    static std::once_flag start_flag;
    std::call_once(start_flag, [&]() { buffer.start(); });
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        themis::BaseEntity entity;
        entity.setPrimaryKey("vec_" + std::to_string(state.thread_index()) + 
                           "_" + std::to_string(state.iterations()));
        
        // Simulate 384-dim embedding
        nlohmann::json embedding = nlohmann::json::array();
        for (int i = 0; i < 384; ++i) {
            embedding.push_back(dis(gen));
        }
        
        nlohmann::json data = {
            {"embedding", embedding},
            {"metadata", {{"source", "benchmark"}}}
        };
        entity.setData(data);
        
        auto status = buffer.add(entity);
        benchmark::DoNotOptimize(status);
    }
    
    if (state.thread_index() == 0) {
        buffer.flush();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VectorAutoBuffer_Concurrent)->Threads(4)->Iterations(100);

// ===== Throughput Benchmarks =====

static void BM_TSAutoBuffer_Throughput(benchmark::State& state) {
    auto& tsstore = g_fixture->tsstore;
    
    themis::timeseries::TSAutoBufferConfig config;
    config.max_points_per_buffer = 10000;
    config.flush_interval = std::chrono::seconds(5);
    config.async_flush = true;
    
    themis::timeseries::TSAutoBuffer buffer(tsstore.get(), config);
    buffer.start();
    
    int64_t timestamp = 1700000000;
    int64_t total_points = 0;
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            themis::TSStore::DataPoint point;
            point.metric = "throughput.test";
            point.entity = "entity_" + std::to_string(i % 10);
            point.timestamp = timestamp++;
            point.value = i * 0.1;
            
            buffer.add(point);
            total_points++;
        }
    }
    
    buffer.stop();
    
    state.SetItemsProcessed(total_points);
    state.SetBytesProcessed(total_points * sizeof(themis::TSStore::DataPoint));
}
BENCHMARK(BM_TSAutoBuffer_Throughput)->Iterations(10);

// ===== Main =====

int main(int argc, char** argv) {
    // Initialize global fixture
    g_fixture = new BenchmarkFixture();
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        delete g_fixture;
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    // Cleanup
    delete g_fixture;
    
    return 0;
}

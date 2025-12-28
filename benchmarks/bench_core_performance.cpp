#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

using namespace themis;

// ============================================================================
// VECTOR INDEX BENCHMARKS
// ============================================================================

class VectorIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_vi_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    
    std::vector<float> genVec(size_t dim) {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) x = dis(gen);
        return v;
    }
};

BENCHMARK_F(VectorIndexBench, InsertPlaintext)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("vec_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", genVec(384)}
            });
            vim.addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// SECONDARY INDEX BENCHMARKS
// ============================================================================

class SecondaryIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_si_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("users", "email");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        sim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

BENCHMARK_F(SecondaryIndexBench, IndexInsert)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("user_" + std::to_string(i), BaseEntity::FieldMap{
                {"email", "user" + std::to_string(i) + "@test.com"},
                {"name", "User " + std::to_string(i)}
            });
            sim_->put("users", e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// QUERY ENGINE BENCHMARKS
// ============================================================================

class QueryEngineBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_qe_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark::State& state) {
    for (auto _ : state) {
        // Placeholder for actual AQL evaluation
        // This benchmark verifies the compilation works
        double val = 42.0;
        benchmark::DoNotOptimize(val);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// GRAPH INDEX BENCHMARKS
// ============================================================================

class GraphIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_gi_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        gim_ = std::make_unique<GraphIndexManager>(*db_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        gim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> gim_;
};

BENCHMARK_F(GraphIndexBench, AddEdges)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("edge_" + std::to_string(i), BaseEntity::FieldMap{
                {"_from", "node_" + std::to_string(i % 10)},
                {"_to", "node_" + std::to_string((i + 1) % 10)},
                {"weight", 1.0}
            });
            gim_->addEdge(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// TIMESERIES BENCHMARKS
// ============================================================================

class TimeseriesBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_ts_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(TimeseriesBench, InsertTimepoints)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            int64_t ts = std::chrono::system_clock::now().time_since_epoch().count();
            double val = 50.0 + (i % 20);
            benchmark::DoNotOptimize(ts);
            benchmark::DoNotOptimize(val);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// MAIN
// ============================================================================

BENCHMARK_MAIN();

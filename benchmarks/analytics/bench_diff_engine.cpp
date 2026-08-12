#include <benchmark/benchmark.h>
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <random>

using namespace themis;
using namespace themis::analytics;

class DiffEngineBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        test_db_path_ = "./data/themis_diff_engine_bench";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        db_->open();
        
        auto txn_db = db_->getRawDB();
        changefeed_ = std::make_unique<Changefeed>(txn_db);
        diff_engine_ = std::make_unique<DiffEngine>(*changefeed_);
        
        // Pre-populate with data for benchmarks
        populateData(state.range(0));
    }
    
    void TearDown(const ::benchmark::State& state) override {
        diff_engine_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void populateData(int64_t num_changes) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);
        
        for (int64_t i = 0; i < num_changes; ++i) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "users:" + std::to_string(i);
            event.value = "User " + std::to_string(i) + " data " + std::to_string(dis(gen));
            // Use proper chrono conversion for timestamp
            auto now = std::chrono::system_clock::now();
            event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            changefeed_->recordEvent(event);
        }
        
        latest_sequence_ = changefeed_->getLatestSequence();
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<DiffEngine> diff_engine_;
    uint64_t latest_sequence_ = 0;
};

// Benchmark: Diff with 100 changes
BENCHMARK_DEFINE_F(DiffEngineBenchmark, Diff100Changes)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 100 ? latest_sequence_ - 100 : 0;
    uint64_t to_seq = latest_sequence_;
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark: Diff with 1K changes
BENCHMARK_DEFINE_F(DiffEngineBenchmark, Diff1KChanges)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 1000 ? latest_sequence_ - 1000 : 0;
    uint64_t to_seq = latest_sequence_;
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Benchmark: Diff with 10K changes
BENCHMARK_DEFINE_F(DiffEngineBenchmark, Diff10KChanges)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 10000 ? latest_sequence_ - 10000 : 0;
    uint64_t to_seq = latest_sequence_;
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
}

// Benchmark: Diff with 100K changes (target: <1s)
BENCHMARK_DEFINE_F(DiffEngineBenchmark, Diff100KChanges)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 100000 ? latest_sequence_ - 100000 : 0;
    uint64_t to_seq = latest_sequence_;
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100000);
}

// Benchmark: Diff with table filter
BENCHMARK_DEFINE_F(DiffEngineBenchmark, DiffWithTableFilter)(benchmark::State& state) {
    uint64_t from_seq = 0;
    uint64_t to_seq = latest_sequence_;
    
    DiffEngine::DiffOptions options;
    options.table_filter = "users";
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * latest_sequence_);
}

// Benchmark: Diff with pagination
BENCHMARK_DEFINE_F(DiffEngineBenchmark, DiffWithPagination)(benchmark::State& state) {
    uint64_t from_seq = 0;
    uint64_t to_seq = latest_sequence_;
    
    DiffEngine::DiffOptions options;
    options.limit = 100;
    options.offset = 0;
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark: Diff with caching (should be much faster on subsequent calls)
BENCHMARK_DEFINE_F(DiffEngineBenchmark, DiffWithCaching)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 1000 ? latest_sequence_ - 1000 : 0;
    uint64_t to_seq = latest_sequence_;
    
    DiffEngine::DiffOptions options;
    options.enable_caching = true;
    
    // First call to populate cache
    diff_engine_->computeDiff(from_seq, to_seq, options);
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiff(from_seq, to_seq, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Benchmark: Diff by timestamp
BENCHMARK_DEFINE_F(DiffEngineBenchmark, DiffByTimestamp)(benchmark::State& state) {
    auto now = std::chrono::system_clock::now();
    auto ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    auto ts_past = ts_now - 60000; // 1 minute ago
    
    for (auto _ : state) {
        auto result = diff_engine_->computeDiffByTimestamp(ts_past, ts_now);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: JSON serialization
BENCHMARK_DEFINE_F(DiffEngineBenchmark, JsonSerialization)(benchmark::State& state) {
    uint64_t from_seq = latest_sequence_ > 100 ? latest_sequence_ - 100 : 0;
    uint64_t to_seq = latest_sequence_;
    
    auto result = diff_engine_->computeDiff(from_seq, to_seq);
    
    for (auto _ : state) {
        auto json = result.toJson();
        benchmark::DoNotOptimize(json);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

// Register benchmarks with different dataset sizes
BENCHMARK_REGISTER_F(DiffEngineBenchmark, Diff100Changes)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, Diff1KChanges)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, Diff10KChanges)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(20);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, Diff100KChanges)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, DiffWithTableFilter)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, DiffWithPagination)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, DiffWithCaching)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, DiffByTimestamp)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

BENCHMARK_REGISTER_F(DiffEngineBenchmark, JsonSerialization)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_MAIN();

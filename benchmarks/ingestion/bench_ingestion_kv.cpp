// Benchmark: Key-Value Ingestion Throughput
// Measures single and batch ingestion performance into RocksDBWrapper

#include <benchmark/benchmark.h>
#include "../benchmark_cleanup.h"
#include "storage/rocksdb_wrapper.h"
#include <array>
#include <filesystem>
#include <random>
#include <mutex>

using namespace themis;

namespace {

std::mutex ingestion_db_open_mutex;

class IngestionBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const auto thread_index = static_cast<size_t>(state.thread_index());
        if (thread_index >= dbs_.size()) {
            throw std::runtime_error("Unsupported ingestion benchmark thread index");
        }
        db_paths_[thread_index] = "./data/bench_ingestion_kv_t" +
            std::to_string(state.thread_index());
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_paths_[thread_index], cleanup_error)) {
            throw std::runtime_error("Failed to clear ingestion benchmark DB: " +
                                     cleanup_error.message());
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_paths_[thread_index];
        cfg.enable_wal = false; // benchmark mode
        cfg.disable_wal_for_benchmark = true;
        cfg.memtable_size_mb = 128;
        cfg.block_cache_size_mb = 512;
        cfg.enable_high_parallel_tuning = true;
        cfg.max_background_jobs = 8;
        auto db = std::make_unique<RocksDBWrapper>(cfg);
        std::lock_guard<std::mutex> open_lock(ingestion_db_open_mutex);
        if (!db->open()) {
            throw std::runtime_error("Failed to open RocksDB for ingestion benchmark");
        }
        dbs_[thread_index] = std::move(db);
    }

    void TearDown(const ::benchmark::State& state) override {
        const auto thread_index = static_cast<size_t>(state.thread_index());
        if (thread_index >= dbs_.size() || !dbs_[thread_index]) {
            return;
        }
        dbs_[thread_index]->close();
        dbs_[thread_index].reset();
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_paths_[thread_index], cleanup_error)) {
            throw std::runtime_error("Ingestion benchmark DB cleanup failed: " +
                                     cleanup_error.message());
        }
    }

    RocksDBWrapper& dbForThread(const ::benchmark::State& state) {
        return *dbs_.at(static_cast<size_t>(state.thread_index()));
    }

protected:
    std::array<std::string, 8> db_paths_;
    std::array<std::unique_ptr<RocksDBWrapper>, 8> dbs_;
};

BENCHMARK_DEFINE_F(IngestionBenchFixture, SingleIngest)(benchmark::State& state) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (auto _ : state) {
        auto id = dist(rng);
        std::string key = "doc_" + std::to_string(id);
        std::string value(256, 'a');
        bool ok = dbForThread(state).put(key, value);
        benchmark::DoNotOptimize(ok);
        if (!ok) {
            state.SkipWithError("put failed");
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["writes_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(IngestionBenchFixture, SingleIngest)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(IngestionBenchFixture, BatchIngest)(benchmark::State& state) {
    const int batch = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (auto _ : state) {
        for (int i = 0; i < batch; ++i) {
            auto id = dist(rng);
            std::string key = "batch_" + std::to_string(id) + "_" + std::to_string(i);
            std::string value(512, 'b');
            bool ok = dbForThread(state).put(key, value);
            benchmark::DoNotOptimize(ok);
            if (!ok) {
                state.SkipWithError("put failed");
                break;
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * batch);
    state.counters["batch_size"] = static_cast<double>(batch);
    state.counters["writes_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * batch), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(IngestionBenchFixture, BatchIngest)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

} // namespace

BENCHMARK_MAIN();

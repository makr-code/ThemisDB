// Benchmark: Key-Value Ingestion Throughput
// Measures single and batch ingestion performance into RocksDBWrapper

#include <benchmark/benchmark.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <random>

using namespace themis;

namespace {

class IngestionBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        db_path_ = "./data/bench_ingestion_kv";
        std::filesystem::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.enable_wal = false; // benchmark mode
        cfg.disable_wal_for_benchmark = true;
        cfg.memtable_size_mb = 128;
        cfg.block_cache_size_mb = 512;
        cfg.enable_high_parallel_tuning = true;
        cfg.max_background_jobs = 8;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
       if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        if (!db_->open()) {
            throw std::runtime_error("Failed to open RocksDB for ingestion benchmark");
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        db_->close();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_DEFINE_F(IngestionBenchFixture, SingleIngest)(benchmark::State& state) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (auto _ : state) {
        auto id = dist(rng);
        std::string key = "doc_" + std::to_string(id);
        std::string value(256, 'a');
        bool ok = db_->put(key, value);
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
            bool ok = db_->put(key, value);
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

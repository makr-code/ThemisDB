/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_hotspots_micro.cpp                           ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>
#include <filesystem>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

namespace fs = std::filesystem;
using namespace themis;

namespace {

// Simple thread pool for benchmark iterations (per benchmark call)
class ParallelExecutor {
public:
    explicit ParallelExecutor(int threads) : threads_(threads) {}

    template <class Fn>
    void run(Fn&& fn) {
        std::vector<std::thread> workers;
        workers.reserve(threads_);
        for (int t = 0; t < threads_; ++t) {
            workers.emplace_back([&, t]() { fn(t); });
        }
        for (auto& w : workers) w.join();
    }

private:
    int threads_;
};

std::string uniquePath(const std::string& name) {
    return "C:\\tmp\\" + name + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

std::string makeValue(size_t bytes) {
    return std::string(bytes, 'x');
}

void runRawWriteBench(benchmark::State& state, bool wal_on, bool high_parallel) {
    const int threads = static_cast<int>(state.range(0));
    const std::string db_path = uniquePath(high_parallel ? "bench_hot_raw_hp" : "bench_hot_raw");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = wal_on;
    cfg.disable_wal_for_benchmark = !wal_on;
    cfg.enable_high_parallel_tuning = high_parallel;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    std::atomic<int64_t> counter{0};
    const std::string value = makeValue(64);

    for (auto _ : state) {
        ParallelExecutor exec(threads);
        exec.run([&](int) {
            for (int i = 0; i < 64; ++i) {
                int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                std::string key = "k" + std::to_string(id);
                db.put(key, value);
            }
        });
    }

    state.SetItemsProcessed(state.iterations() * threads * 64);
    db.close();
    fs::remove_all(db_path);
}

void runMixedRWBench(benchmark::State& state, bool high_parallel) {
    const int threads = static_cast<int>(state.range(0));
    const std::string db_path = uniquePath(high_parallel ? "bench_hot_mixed_hp" : "bench_hot_mixed");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.enable_high_parallel_tuning = high_parallel;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    std::atomic<int64_t> counter{0};
    const std::string value = makeValue(128);

    for (auto _ : state) {
        ParallelExecutor exec(threads);
        exec.run([&](int) {
            for (int i = 0; i < 32; ++i) {
                int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                std::string key = "mk" + std::to_string(id);
                db.put(key, value);
                std::string out;
                db.get(key, out);
            }
        });
    }

    state.SetItemsProcessed(state.iterations() * threads * 32 * 2); // put + get
    db.close();
    fs::remove_all(db_path);
}

void runSecondaryIndexBench(benchmark::State& state, bool high_parallel) {
    const int threads = static_cast<int>(state.range(0));
    const std::string db_path = uniquePath(high_parallel ? "bench_hot_sec_hp" : "bench_hot_sec");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.enable_high_parallel_tuning = high_parallel;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    SecondaryIndexManager sim(db);
    sim.createIndex("sec_hot", "id");

    std::atomic<int64_t> counter{0};
    const std::string value = makeValue(96);

    for (auto _ : state) {
        ParallelExecutor exec(threads);
        exec.run([&](int) {
            for (int i = 0; i < 24; ++i) {
                int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                BaseEntity e("sk" + std::to_string(id), BaseEntity::FieldMap{
                    {"data", value},
                    {"value", static_cast<double>(id)}
                });
                sim.put("sec_hot", e);
            }
        });
    }

    state.SetItemsProcessed(state.iterations() * threads * 24);
    db.close();
    fs::remove_all(db_path);
}

} // namespace

// Registration

static void BM_RawWrite_WAL_On(benchmark::State& state) { runRawWriteBench(state, true, false); }
static void BM_RawWrite_WAL_Off(benchmark::State& state) { runRawWriteBench(state, false, false); }
static void BM_RawWrite_WAL_On_Hybrid(benchmark::State& state) { runRawWriteBench(state, true, true); }
static void BM_MixedRW(benchmark::State& state) { runMixedRWBench(state, false); }
static void BM_MixedRW_Hybrid(benchmark::State& state) { runMixedRWBench(state, true); }
static void BM_SecondaryIndex_Write(benchmark::State& state) { runSecondaryIndexBench(state, false); }
static void BM_SecondaryIndex_Write_Hybrid(benchmark::State& state) { runSecondaryIndexBench(state, true); }

BENCHMARK(BM_RawWrite_WAL_On)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_RawWrite_WAL_Off)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_RawWrite_WAL_On_Hybrid)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_MixedRW)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_MixedRW_Hybrid)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_SecondaryIndex_Write)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_SecondaryIndex_Write_Hybrid)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();

BENCHMARK_MAIN();

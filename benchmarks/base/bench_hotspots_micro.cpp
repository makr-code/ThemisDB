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
        for (auto& w : workers) {
          w.join();
        }
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

void runStorageInsertBench(benchmark::State& state) {
    const std::string db_path = uniquePath("bench_crud_ins");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.enable_high_parallel_tuning = false;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    const std::string value = makeValue(256);
    std::atomic<int64_t> counter{0};

    for (auto _ : state) {
        int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string key = "ins" + std::to_string(id);
        benchmark::DoNotOptimize(db.put(key, value));
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(256 + 16));
    db.close();
    fs::remove_all(db_path);
}

void runStorageReadBench(benchmark::State& state) {
    constexpr int kPrefill = 10000;
    const std::string db_path = uniquePath("bench_crud_read");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = false;
    cfg.disable_wal_for_benchmark = true;
    cfg.enable_high_parallel_tuning = false;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    const std::string value = makeValue(256);
    for (int i = 0; i < kPrefill; ++i) {
        db.put("rk" + std::to_string(i), value);
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, kPrefill - 1);

    for (auto _ : state) {
        std::string key = "rk" + std::to_string(dist(rng));
        std::string out;
        benchmark::DoNotOptimize(db.get(key, out));
    }

    state.SetItemsProcessed(state.iterations());
    db.close();
    fs::remove_all(db_path);
}

void runStorageUpdateBench(benchmark::State& state) {
    constexpr int kNumKeys = 1000;
    const std::string db_path = uniquePath("bench_crud_upd");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.enable_high_parallel_tuning = false;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    const std::string value_v1 = makeValue(256);
    for (int i = 0; i < kNumKeys; ++i) {
        db.put("uk" + std::to_string(i), value_v1);
    }

    const std::string value_v2 = makeValue(256);
    std::atomic<int64_t> counter{0};

    for (auto _ : state) {
        int64_t id = counter.fetch_add(1, std::memory_order_relaxed) % kNumKeys;
        std::string key = "uk" + std::to_string(id);
        benchmark::DoNotOptimize(db.put(key, value_v2));
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(256 + 16));
    db.close();
    fs::remove_all(db_path);
}

void runSustainedWriteBench(benchmark::State& state, bool high_parallel) {
    const int threads = static_cast<int>(state.range(0));
    const std::string db_path = uniquePath(high_parallel ? "bench_sust_hp" : "bench_sust");
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
    const std::string value = makeValue(256);

    for (auto _ : state) {
        ParallelExecutor exec(threads);
        exec.run([&](int) {
            for (int i = 0; i < 128; ++i) {
                int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                std::string key = "sw" + std::to_string(id);
                db.put(key, value);
            }
        });
    }

    state.SetItemsProcessed(state.iterations() * threads * 128);
    state.SetBytesProcessed(state.iterations() * threads * 128 * static_cast<int64_t>(256 + 16));
    db.close();
    fs::remove_all(db_path);
}

void runPointReadP99Bench(benchmark::State& state) {
    constexpr int kPrefill = 50000;
    const std::string db_path = uniquePath("bench_p99");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = false;
    cfg.disable_wal_for_benchmark = true;
    cfg.enable_high_parallel_tuning = false;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    const std::string value = makeValue(256);
    for (int i = 0; i < kPrefill; ++i) {
        db.put("p99k" + std::to_string(i), value);
    }

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, kPrefill - 1);
    std::vector<int64_t> latencies_us;
    latencies_us.reserve(25000);

    for (auto _ : state) {
        std::string key = "p99k" + std::to_string(dist(rng));
        std::string out;
        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(db.get(key, out));
        auto t1 = std::chrono::steady_clock::now();
        latencies_us.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    if (!latencies_us.empty()) {
        std::sort(latencies_us.begin(), latencies_us.end());
        const size_t p99_idx = static_cast<size_t>(latencies_us.size() * 99 / 100);
        const int64_t p99_us = latencies_us[p99_idx];
        state.SetLabel("P99=" + std::to_string(p99_us) + "us");
        state.counters["P99_us"] = static_cast<double>(p99_us);
    }

    state.SetItemsProcessed(state.iterations());
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

static void BM_StorageInsert(benchmark::State& state) { runStorageInsertBench(state); }
static void BM_StorageRead(benchmark::State& state) { runStorageReadBench(state); }
static void BM_StorageUpdate(benchmark::State& state) { runStorageUpdateBench(state); }
static void BM_SustainedWrite(benchmark::State& state) { runSustainedWriteBench(state, false); }
static void BM_SustainedWrite_Hybrid(benchmark::State& state) { runSustainedWriteBench(state, true); }
static void BM_PointReadP99(benchmark::State& state) { runPointReadP99Bench(state); }

BENCHMARK(BM_StorageInsert)->UseRealTime();
BENCHMARK(BM_StorageRead)->UseRealTime();
BENCHMARK(BM_StorageUpdate)->UseRealTime();
BENCHMARK(BM_SustainedWrite)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_SustainedWrite_Hybrid)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->UseRealTime();
BENCHMARK(BM_PointReadP99)->Iterations(20000)->UseRealTime();

BENCHMARK_MAIN();

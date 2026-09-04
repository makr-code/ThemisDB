#include <benchmark/benchmark.h>
#include <filesystem>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;
using namespace themis;

namespace {

std::string uniquePath(const std::string& name) {
    return "C:\\tmp\\" + name + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

void runWalStress(benchmark::State& state, bool sync_on) {
    const int threads = static_cast<int>(state.range(0));
    const int batch = static_cast<int>(state.range(1));

    const std::string db_path = uniquePath(sync_on ? "bench_wal_sync" : "bench_wal_nosync");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = sync_on;
    cfg.disable_wal_for_benchmark = !sync_on;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    std::atomic<int64_t> counter{0};
    const std::string value(128, 'v');

    for (auto _ : state) {
        std::vector<std::thread> workers;
        workers.reserve(threads);
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t]() {
                for (int i = 0; i < batch; ++i) {
                    int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                    std::string key = "w" + std::to_string(id);
                    db.put(key, value);
                }
            });
        }
        for (auto& w : workers) {
          w.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * threads * batch);
    db.close();
    fs::remove_all(db_path);
}

} // namespace

static void BM_WAL_Sync(benchmark::State& state) { runWalStress(state, true); }
static void BM_WAL_NoSync(benchmark::State& state) { runWalStress(state, false); }

BENCHMARK(BM_WAL_Sync)->Args({1,64})->Args({4,64})->Args({8,64})->Args({16,64})->UseRealTime();
BENCHMARK(BM_WAL_NoSync)->Args({1,64})->Args({4,64})->Args({8,64})->Args({16,64})->UseRealTime();

BENCHMARK_MAIN();

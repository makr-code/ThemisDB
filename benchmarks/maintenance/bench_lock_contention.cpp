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

void runContentionBench(benchmark::State& state, bool overlapping_keys) {
    const int threads = static_cast<int>(state.range(0));
    const int key_domain = overlapping_keys ? 16 : threads * 64; // small domain causes lock contention

    const std::string db_path = uniquePath(overlapping_keys ? "bench_lock_overlap" : "bench_lock_disjoint");
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
    cfg.allow_concurrent_memtable_write = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }

    std::atomic<int64_t> counter{0};
    const std::string value(64, 'x');

    for (auto _ : state) {
        std::vector<std::thread> workers;
        workers.reserve(threads);
        std::atomic<bool> failed{false};

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t]() {
                try {
                    auto txn = db.beginTransaction();
                    for (int i = 0; i < 64; ++i) {
                        int64_t id = counter.fetch_add(1, std::memory_order_relaxed);
                        int key_id = overlapping_keys ? (id % key_domain) : (t * 1000000 + i);
                        std::string key = "lc_" + std::to_string(key_id);
                        txn->put(key, std::vector<uint8_t>(value.begin(), value.end()));
                    }
                    txn->prepare();
                    txn->commit();
                } catch (...) {
                    failed.store(true);
                }
            });
        }
        for (auto& w : workers) w.join();
        if (failed.load()) state.SkipWithError("Thread failed");
    }

    state.SetItemsProcessed(state.iterations() * threads * 64);
    db.close();
    fs::remove_all(db_path);
}

} // namespace

static void BM_LockContention_Disjoint(benchmark::State& state) { runContentionBench(state, false); }
static void BM_LockContention_Overlapping(benchmark::State& state) { runContentionBench(state, true); }

BENCHMARK(BM_LockContention_Disjoint)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)->UseRealTime();
BENCHMARK(BM_LockContention_Overlapping)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)->UseRealTime();

BENCHMARK_MAIN();

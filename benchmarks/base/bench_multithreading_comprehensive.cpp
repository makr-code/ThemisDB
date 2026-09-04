/**
 * @file bench_multithreading_comprehensive.cpp
 * @brief Comprehensive multi-threading benchmarks for ThemisDB
 * 
 * Tests thread scaling, contention, and performance across different concurrency levels.
 * Follows BENCHMARK_BEST_PRACTICES.md and SCIENTIFIC_STANDARDS_README.md guidelines.
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <thread>
#include <atomic>
#include <mutex>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

using namespace themis;

// ============================================================================
// Deterministic Random Number Generation
// ============================================================================

class DeterministicRNG {
private:
    std::mt19937_64 gen_;
    mutable std::mutex mutex_;

public:
    explicit DeterministicRNG(uint64_t seed = 42) : gen_(seed) {}

    uint64_t next() {
        std::lock_guard<std::mutex> lock(mutex_);
        return gen_();
    }

    std::string generateString(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[next() % (sizeof(charset) - 1)];
        }
        return result;
    }

    int64_t generateInt(int64_t min, int64_t max) {
        return min + (next() % (max - min + 1));
    }
};

// ============================================================================
// Shared Test Environment with Warmup
// ============================================================================

class MultiThreadBenchEnv {
private:
    std::shared_ptr<RocksDBWrapper> db_;
    std::filesystem::path db_path_;
    DeterministicRNG rng_{42}; // Deterministic seed
    bool initialized_ = false;
    std::mutex init_mutex_;

public:
    static MultiThreadBenchEnv& instance() {
        static MultiThreadBenchEnv env;
        return env;
    }

    void initialize() {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (initialized_) {
          return;
        }

        db_path_ = std::filesystem::temp_directory_path() / ("bench_multithread_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        cfg.memtable_size_mb = 256;
        cfg.block_cache_size_mb = 512;
        cfg.disable_wal_for_benchmark = true;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;
        auto hc = std::thread::hardware_concurrency();
        cfg.max_background_jobs = (hc == 0) ? 1u : hc;

        db_ = std::make_shared<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database for benchmark");
        }

        // Warmup: Pre-populate with data
        warmup();
        initialized_ = true;
    }

    void warmup() {
        // Insert 10000 entities for warmup
        for (int i = 0; i < 10000; ++i) {
            BaseEntity entity("warmup_" + std::to_string(i), BaseEntity::FieldMap{
                {"value", static_cast<int64_t>(i)},
                {"data", rng_.generateString(100)}
            });
            db_->put("entity:" + entity.getPrimaryKey(), entity.serialize());
        }
    }

    std::shared_ptr<RocksDBWrapper> getDB() {
        if (!initialized_) {
          initialize();
        }
        return db_;
    }

    DeterministicRNG& getRNG() { return rng_; }

    ~MultiThreadBenchEnv() {
        db_.reset();
        if (!db_path_.empty()) {
            std::filesystem::remove_all(db_path_);
        }
    }
};

// ============================================================================
// Thread Scaling Benchmarks
// ============================================================================

static void BM_ConcurrentWrites_ThreadScaling(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    const int ops_per_thread = 100;

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> completed{0};

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t); // Deterministic per-thread seed
                for (int i = 0; i < ops_per_thread; ++i) {
                    std::string key = "thread_" + std::to_string(t) + "_" + std::to_string(i);
                    BaseEntity entity(key, BaseEntity::FieldMap{
                        {"value", thread_rng.generateInt(0, 1000000)},
                        {"data", thread_rng.generateString(100)}
                    });
                    db->put("entity:" + key, entity.serialize());
                }
                completed++;
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        benchmark::DoNotOptimize(completed.load());
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
    state.SetBytesProcessed(state.iterations() * num_threads * ops_per_thread * 100);
}
BENCHMARK(BM_ConcurrentWrites_ThreadScaling)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseRealTime();

static void BM_ConcurrentReads_ThreadScaling(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    const int ops_per_thread = 100;

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                for (int i = 0; i < ops_per_thread; ++i) {
                    int warmup_id = thread_rng.generateInt(0, 9999);
                    std::string key = "entity:warmup_" + std::to_string(warmup_id);
                    auto value = db->get(key);
                    benchmark::DoNotOptimize(value);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}
BENCHMARK(BM_ConcurrentReads_ThreadScaling)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseRealTime();

static void BM_MixedReadWrite_ThreadScaling(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    const int ops_per_thread = 100;
    const double read_ratio = 0.7; // 70% reads, 30% writes

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                for (int i = 0; i < ops_per_thread; ++i) {
                    double op = static_cast<double>(thread_rng.next() % 100) / 100.0;
                    if (op < read_ratio) {
                        // Read operation
                        int warmup_id = thread_rng.generateInt(0, 9999);
                        std::string key = "entity:warmup_" + std::to_string(warmup_id);
                        auto value = db->get(key);
                        benchmark::DoNotOptimize(value);
                    } else {
                        // Write operation
                        std::string key = "mixed_" + std::to_string(t) + "_" + std::to_string(i);
                        BaseEntity entity(key, BaseEntity::FieldMap{
                            {"value", thread_rng.generateInt(0, 1000000)}
                        });
                        db->put("entity:" + key, entity.serialize());
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}
BENCHMARK(BM_MixedReadWrite_ThreadScaling)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseRealTime();

// ============================================================================
// Lock Contention Benchmarks
// ============================================================================

static void BM_HighContentionWrites(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    
    // All threads write to the same small key space to maximize contention
    const int key_space_size = 10;

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                for (int i = 0; i < 100; ++i) {
                    int key_id = thread_rng.generateInt(0, key_space_size - 1);
                    std::string key = "contention_" + std::to_string(key_id);
                    BaseEntity entity(key, BaseEntity::FieldMap{
                        {"counter", thread_rng.generateInt(0, 1000000)}
                    });
                    db->put("entity:" + key, entity.serialize());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_threads * 100);
}
BENCHMARK(BM_HighContentionWrites)
    ->Unit(benchmark::kMillisecond)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime();

static void BM_LowContentionWrites(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    
    // Each thread writes to its own key space to minimize contention
    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                for (int i = 0; i < 100; ++i) {
                    std::string key = "lowcontention_" + std::to_string(t) + "_" + std::to_string(i);
                    BaseEntity entity(key, BaseEntity::FieldMap{
                        {"value", thread_rng.generateInt(0, 1000000)}
                    });
                    db->put("entity:" + key, entity.serialize());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_threads * 100);
}
BENCHMARK(BM_LowContentionWrites)
    ->Unit(benchmark::kMillisecond)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime();

// ============================================================================
// Batch vs Individual Operations
// ============================================================================

static void BM_BatchInsert_SingleThread(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int batch_size = state.range(0);

    for (auto _ : state) {
        DeterministicRNG rng(42);
        std::vector<std::pair<std::string, BaseEntity::Blob>> batch;
        batch.reserve(batch_size);

        for (int i = 0; i < batch_size; ++i) {
            std::string key = "batch_" + std::to_string(i);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)},
                {"data", rng.generateString(100)}
            });
            batch.emplace_back("entity:" + key, entity.serialize());
        }

        // Batch write
        for (const auto& [key, value] : batch) {
            db->put(key, value);
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchInsert_SingleThread)
    ->Unit(benchmark::kMillisecond)
    ->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_BatchInsert_MultiThread(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    const int num_threads = state.range(0);
    const int batch_size = 100;

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                std::vector<std::pair<std::string, BaseEntity::Blob>> batch;
                batch.reserve(batch_size);

                for (int i = 0; i < batch_size; ++i) {
                    std::string key = "batchmt_" + std::to_string(t) + "_" + std::to_string(i);
                    BaseEntity entity(key, BaseEntity::FieldMap{
                        {"value", thread_rng.generateInt(0, 1000000)}
                    });
                    batch.emplace_back("entity:" + key, entity.serialize());
                }

                for (const auto& [key, value] : batch) {
                    db->put(key, value);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_threads * batch_size);
}
BENCHMARK(BM_BatchInsert_MultiThread)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->UseRealTime();

// ============================================================================
// Stress Tests with Maximum Threads
// ============================================================================

static void BM_MaxThreadStress(benchmark::State& state) {
    auto& env = MultiThreadBenchEnv::instance();
    auto db = env.getDB();
    auto hc = std::thread::hardware_concurrency();
    const int num_threads = (hc == 0) ? 1 : static_cast<int>(hc);

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> total_ops{0};

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                DeterministicRNG thread_rng(42 + t);
                int ops = 0;
                for (int i = 0; i < 50; ++i) {
                    std::string key = "stress_" + std::to_string(t) + "_" + std::to_string(i);
                    BaseEntity entity(key, BaseEntity::FieldMap{
                        {"value", thread_rng.generateInt(0, 1000000)},
                        {"data", thread_rng.generateString(200)}
                    });
                    db->put("entity:" + key, entity.serialize());
                    ops++;

                    // Read some data
                    int warmup_id = thread_rng.generateInt(0, 9999);
                    auto value = db->get("entity:warmup_" + std::to_string(warmup_id));
                    benchmark::DoNotOptimize(value);
                    ops++;
                }
                total_ops += ops;
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        benchmark::DoNotOptimize(total_ops.load());
    }

    state.SetItemsProcessed(state.iterations() * num_threads * 100);
}
BENCHMARK(BM_MaxThreadStress)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

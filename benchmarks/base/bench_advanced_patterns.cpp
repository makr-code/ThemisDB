/**
 * @file bench_advanced_patterns.cpp
 * @brief Advanced benchmark patterns following OOP and best-practices
 * 
 * Tests:
 * - Read/Write ratio combinations (20R/80W, 50R/50W, 80R/20W, 100R, 100W)
 * - Parallel scalability (1, 4, 8, 16, 32 threads)
 * - Self-protection mechanisms (burst load, sustained load, concurrent connections)
 * - Best-practice patterns vs anti-patterns
 * - Gap analysis against documented capabilities and internet standards
 * 
 * Build: cmake --build . --target bench_advanced_patterns --config Release
 * Run:   ./bench_advanced_patterns.exe
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>

#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "utils/logger.h"

namespace fs = std::filesystem;
using namespace themis;

// ============================================================================
// UTILITIES & HELPERS
// ============================================================================

namespace {

/**
 * Random number generation utilities using thread-local state
 */
class RandomGenerator {
public:
    static RandomGenerator& instance() {
        static thread_local RandomGenerator gen;
        return gen;
    }
    
    std::vector<float> genVec(size_t dim) {
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) {
          x = dis(rng_);
        }
        return v;
    }
    
    std::string randStr(size_t len) {
        static const std::string charset = 
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<size_t> dis(0, charset.length() - 1);
        std::string s(len, ' ');
        for (char& c : s) {
          c = charset[dis(rng_)];
        }
        return s;
    }
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dis(min, max);
        return dis(rng_);
    }

private:
    RandomGenerator() : rng_(std::random_device{}()) {}
    std::mt19937 rng_;
};

/**
 * Database lifecycle management with RAII pattern
 */
class DatabaseFixture {
public:
    explicit DatabaseFixture(const std::string& name, bool use_optimized_config = false, bool use_best_practices = false) {
        db_path_ = "C:\\tmp\\bench_" + name + "_" + 
                  std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        
        // Phase 1 (Final): Apply best practices from RocksDB Wiki
        if (use_best_practices) {
            cfg.allow_concurrent_memtable_write = true;  // Multiple threads write to different memtables
            cfg.enable_pipelined_write = true;           // Pipeline writes for parallelism
            cfg.allow_unordered_write = true;            // Unordered writes = better concurrency
            cfg.enable_wal = false;                      // Disable WAL for benchmarks
            cfg.max_write_buffer_number = 8;
        }
        
        // Phase 3: Optimized RocksDB configuration for parallel workloads
        if (use_optimized_config) {
            cfg.max_background_jobs = 16;              // More background threads (default: 4)
            cfg.max_write_buffer_number = 8;           // More write buffers (default: 3)
            cfg.memtable_size_mb = 512;                // LARGER memtables (default: 256)
            cfg.block_cache_size_mb = 2048;            // LARGER block cache (default: 1024)
            cfg.min_write_buffer_number_to_merge = 2;  // Batch merges
            cfg.enable_wal = false;                    // Disable WAL for benchmarks
        }
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        
        // CRITICAL FIX: Must call open() after construction
        if (!db_->open()) {
            throw std::runtime_error("Failed to open RocksDB in DatabaseFixture");
        }
    }
    
    ~DatabaseFixture() {
        db_.reset();
        fs::remove_all(db_path_);
    }
    
    RocksDBWrapper& getDb() { return *db_; }
    const std::string& getPath() const { return db_path_; }

private:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * Multi-threaded workload executor
 */
/**
 * Parallel task executor with improved thread synchronization
 * Uses epoch barrier to reduce join() overhead
 */
class ParallelExecutor {
public:
    explicit ParallelExecutor(int num_threads) : num_threads_(num_threads) {}
    
    template<typename Callable>
    void execute(Callable&& work, int iterations_per_thread) {
        THEMIS_TRACE("ParallelExecutor::execute start: threads={}, iterations_per_thread={}",
                     num_threads_, iterations_per_thread);
        std::vector<std::thread> threads;
        
        // Create all threads
        for (int t = 0; t < num_threads_; ++t) {
            threads.emplace_back([&work, iterations_per_thread, t]() {
                // Each thread works independently - no shared counter
                for (int i = 0; i < iterations_per_thread; ++i) {
                    work(t * iterations_per_thread + i);
                }
            });
        }
        
        // Join with yield to reduce OS context switch penalties
        for (auto& t : threads) {
            t.join();
        }
        THEMIS_TRACE("ParallelExecutor::execute done: threads={}, total_iterations={}",
                     num_threads_, num_threads_ * iterations_per_thread);
    }

private:
    int num_threads_;
};

} // anonymous namespace

// ============================================================================
// PHASE 1 FINAL: COUNTER ELIMINATION + ROCKSDB BEST PRACTICES
// ============================================================================

/**
 * Phase 1 Final: Combines counter elimination with RocksDB best practices
 * 
 * Key optimizations:
 * - Thread-local work_id (no shared counter)
 * - allow_concurrent_memtable_write = true
 * - enable_pipelined_write = true  
 * - allow_unordered_write = true
 * - WAL disabled for benchmarks
 * 
 * Expected: Better scaling than baseline
 */
class ParallelityBenchPhase1Final : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_phase1_final", false, true);
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("parallel_final", "id");
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Phase 1 Final: 1 thread
BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_final_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_final", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 1 Final: 4 threads
BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_final_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 25 + i)}
                });
                sim_->put("parallel_final", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 1 Final: 8 threads (KEY TEST)
BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_final_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 12 + i)}
                });
                sim_->put("parallel_final", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 1 Final: 16 threads
BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_final_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 6 + i)}
                });
                sim_->put("parallel_final", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 1 Final: 32 threads
BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_final_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 3 + i)}
                });
                sim_->put("parallel_final", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2F: WriteOptions::disableWAL BENCHMARK
// ============================================================================

class ParallelityBenchPhase2Final : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> fixture_phase2_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    
    void SetUp(const benchmark::State&) override {
        // Phase 2F: Enable WriteOptions::disableWAL for benchmark mode
        // CRITICAL: Configure disableWAL for Phase 2F
        RocksDBWrapper::Config cfg;
        cfg.db_path = "C:\\tmp\\bench_phase2_final_" + 
                     std::to_string(reinterpret_cast<uintptr_t>(this));
        cfg.disable_wal_for_benchmark = true;  // ← KEY: Phase 2F setting!
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;
        cfg.enable_wal = false;
        cfg.max_write_buffer_number = 8;
        
        fs::remove_all(cfg.db_path);
        fs::create_directories(cfg.db_path);
        
        fixture_phase2_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!fixture_phase2_->open()) {
            throw std::runtime_error("Failed to open RocksDB in Phase2Fixture");
        }
        fixture_phase2_->open();
        
        sim_ = std::make_unique<SecondaryIndexManager>(*fixture_phase2_);
        sim_->createIndex("parallel_p2f", "id");
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_phase2_.reset();
    }
};

// Phase 2F: 1 thread (baseline)
BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_p2f_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_p2f", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2F: 4 threads
BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_p2f_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 25 + i)}
                });
                sim_->put("parallel_p2f", e);
            }
        }, 4);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2F: 8 threads (KEY TEST)
BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_p2f_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 12 + i)}
                });
                sim_->put("parallel_p2f", e);
            }
        }, 8);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2F: 16 threads
BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_p2f_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 6 + i)}
                });
                sim_->put("parallel_p2f", e);
            }
        }, 16);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2F: 32 threads
BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_p2f_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 3 + i)}
                });
                sim_->put("parallel_p2f", e);
            }
        }, 32);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G: TransactionDB WritePrepared Policy BENCHMARK
// ============================================================================

class ParallelityBenchPhase2G : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_wp_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        // WritePrepared policy (keep WAL enabled)
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false; // try without dual queues to reduce overhead
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnWrites(int thread_id, int records) {
        auto txn = db_->beginTransaction();
        for (int i = 0; i < records; ++i) {
            BaseEntity e(
                std::string("entity_p2g_") + std::to_string(thread_id) + "_" + std::to_string(i),
                BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * records + i)}
                }
            );
            sim_->put("parallel_p2g", e, *txn);
        }
        // WritePrepared benefit: heavy work during prepare
        txn->prepare();
        txn->commit();
    }
};

// ============================================================================
// PHASE 2G+2H: Same workload as Phase 2G, but with Phase 2H background tuning
// ============================================================================

class ParallelityBenchPhase2G_2H : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g2h_wp_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        // WritePrepared policy (keep WAL enabled); same as Phase 2G
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        // Phase 2H background & L0 tuning (A/B against Phase 2G)
        cfg.max_background_compactions = 8;
        cfg.max_background_flushes = 2;
        cfg.background_threads_low = 8;
        cfg.background_threads_high = 2;
        cfg.max_subcompactions = 2;
        cfg.level0_file_num_compaction_trigger = 2;
        cfg.level0_slowdown_writes_trigger = 8;
        cfg.level0_stop_writes_trigger = 16;
        cfg.block_cache_shard_bits = 6;           // 64 shards
        cfg.db_write_buffer_size_mb = 512;        // total memtable cap

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g2h", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnWrites(int thread_id, int records) {
        auto txn = db_->beginTransaction();
        for (int i = 0; i < records; ++i) {
            BaseEntity e(
                std::string("entity_p2g2h_") + std::to_string(thread_id) + "_" + std::to_string(i),
                BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * records + i)}
                }
            );
            sim_->put("parallel_p2g2h", e, *txn);
        }
        txn->prepare();
        txn->commit();
    }
};

// Phase 2G+2H: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 100);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G+2H: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 25);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G+2H: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 12);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G+2H: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 6);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G+2H: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 3);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 100);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 25);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 12);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 6);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnWrites(work_id, 3);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G (A/B1): WritePrepared ohne Transaktionen (Non-Tx)
// ============================================================================

class ParallelityBenchPhase2G_NonTxn : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_ntx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        // Enable WritePrepared policy but do not use Transaction API in workload
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;              // reduce overhead
        cfg.disable_wal_for_benchmark = false;     // keep WAL (required for WP)
        cfg.enable_wal = false;                    // no fsync for benchmark
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2gntx", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }
};

// Phase 2G Non-Tx: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_p2gntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_p2gntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G Non-Tx: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_p2gntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 25 + i)}
                });
                sim_->put("parallel_p2gntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G Non-Tx: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_p2gntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 12 + i)}
                });
                sim_->put("parallel_p2gntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G Non-Tx: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_p2gntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 6 + i)}
                });
                sim_->put("parallel_p2gntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Phase 2G Non-Tx: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_p2gntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 3 + i)}
                });
                sim_->put("parallel_p2gntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G (A/B2): WriteUnprepared ohne Transaktionen (Non-Tx)
// ============================================================================

class ParallelityBenchPhase2G_Unprepared_NonTxn : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_unprep_ntx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        // Enable WriteUnprepared policy; workload uses no Transaction API
        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WriteUnprepared;
        cfg.two_write_queues = false;
        cfg.disable_wal_for_benchmark = false;    // keep WAL
        cfg.enable_wal = false;                   // no fsync for bench
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g_unprep_ntx", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }
};

// A/B2 Non-Tx: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_p2gunprepntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_p2g_unprep_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B2 Non-Tx: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_p2gunprepntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 25 + i)}
                });
                sim_->put("parallel_p2g_unprep_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B2 Non-Tx: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_p2gunprepntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 12 + i)}
                });
                sim_->put("parallel_p2g_unprep_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B2 Non-Tx: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_p2gunprepntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 6 + i)}
                });
                sim_->put("parallel_p2g_unprep_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B2 Non-Tx: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_p2gunprepntx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 3 + i)}
                });
                sim_->put("parallel_p2g_unprep_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G (A/B3): WritePrepared mit kleinen Transaktionen (max 10 Ops/Txn)
// ============================================================================

class ParallelityBenchPhase2G_Txn10 : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_txn10_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g_txn10", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            auto txn = db_->beginTransaction();
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                BaseEntity e(
                    std::string("entity_p2g_txn10_") + std::to_string(thread_id) + "_" + std::to_string(idx),
                    BaseEntity::FieldMap{
                        {"data", RandomGenerator::instance().randStr(100)},
                        {"value", static_cast<double>(thread_id * total_records + idx)}
                    }
                );
                sim_->put("parallel_p2g_txn10", e, *txn);
            }
            txn->prepare();
            txn->commit();
            produced += batch;
        }
    }
};

// A/B3 Txn10: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 100, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B3 Txn10: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 25, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B3 Txn10: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 12, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B3 Txn10: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 6, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B3 Txn10: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 3, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G (A/B4): WritePrepared mit sehr kleinen Transaktionen (max 5 Ops/Txn)
// ============================================================================

class ParallelityBenchPhase2G_Txn5 : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_txn5_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g_txn5", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            auto txn = db_->beginTransaction();
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                BaseEntity e(
                    std::string("entity_p2g_txn5_") + std::to_string(thread_id) + "_" + std::to_string(idx),
                    BaseEntity::FieldMap{
                        {"data", RandomGenerator::instance().randStr(100)},
                        {"value", static_cast<double>(thread_id * total_records + idx)}
                    }
                );
                sim_->put("parallel_p2g_txn5", e, *txn);
            }
            txn->prepare();
            txn->commit();
            produced += batch;
        }
    }
};

// A/B4 Txn5: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 100, 5);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B4 Txn5: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 25, 5);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B4 Txn5: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 12, 5);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B4 Txn5: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 6, 5);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B4 Txn5: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 3, 5);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 2G (A/B5): WritePrepared Txn10 mit two_write_queues=true
// ============================================================================

class ParallelityBenchPhase2G_Txn10_DualQueue : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_phase2g_txn10dq_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = true; // enable dual queues
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("parallel_p2g_txn10dq", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            auto txn = db_->beginTransaction();
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                BaseEntity e(
                    std::string("entity_p2g_txn10dq_") + std::to_string(thread_id) + "_" + std::to_string(idx),
                    BaseEntity::FieldMap{
                        {"data", RandomGenerator::instance().randStr(100)},
                        {"value", static_cast<double>(thread_id * total_records + idx)}
                    }
                );
                sim_->put("parallel_p2g_txn10dq", e, *txn);
            }
            txn->prepare();
            txn->commit();
            produced += batch;
        }
    }
};

// A/B5 Txn10 + dual queue: 1 thread
BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 100, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B5 Txn10 + dual queue: 4 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 25, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B5 Txn10 + dual queue: 8 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 12, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B5 Txn10 + dual queue: 16 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 6, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// A/B5 Txn10 + dual queue: 32 threads
BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 3, 10);
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// RAW ROCKSDB BASELINES (TransactionDB directly)
// ============================================================================

// Non-transactional baseline using TransactionDB::Put (WritePrepared policy) to gauge wrapper/secondary index overhead.
class RocksDBRaw_NonTxn : public benchmark::Fixture {
protected:
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::string db_path_ = {};
    rocksdb::WriteOptions write_opts_;
    std::atomic<bool> failed_{false};
    std::string first_error_;

    void SetUp(const benchmark::State&) override {
        db_path_ = "C:\\tmp\\bench_raw_ntx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);

        rocksdb::Options options;
        options.create_if_missing = true;
        options.allow_concurrent_memtable_write = true;
        options.enable_pipelined_write = false; // disable for compatibility with current RocksDB build

        rocksdb::TransactionDBOptions txn_opts;
        txn_opts.write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;

        rocksdb::TransactionDB* db_raw = nullptr;
        auto status = rocksdb::TransactionDB::Open(options, txn_opts, db_path_, &db_raw);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open raw TransactionDB: " + status.ToString());
        }
        db_.reset(db_raw);

        write_opts_.disableWAL = false; // keep WAL (comparable to WP configs with WAL on)
    }

    void TearDown(const benchmark::State&) override {
        db_.reset();
        fs::remove_all(db_path_);
    }
};

// Raw Non-Tx: 1 thread
BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                std::string key = "raw_ntx_" + std::to_string(work_id) + "_" + std::to_string(i);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = db_->Put(write_opts_, key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                }
            }
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawNTX Put failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Non-Tx: 4 threads
BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                std::string key = "raw_ntx_" + std::to_string(work_id) + "_" + std::to_string(i);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = db_->Put(write_opts_, key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                }
            }
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawNTX Put failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Non-Tx: 8 threads
BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                std::string key = "raw_ntx_" + std::to_string(work_id) + "_" + std::to_string(i);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = db_->Put(write_opts_, key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                }
            }
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawNTX Put failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Non-Tx: 16 threads
BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                std::string key = "raw_ntx_" + std::to_string(work_id) + "_" + std::to_string(i);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = db_->Put(write_opts_, key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                }
            }
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawNTX Put failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Non-Tx: 32 threads
BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                std::string key = "raw_ntx_" + std::to_string(work_id) + "_" + std::to_string(i);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = db_->Put(write_opts_, key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                }
            }
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawNTX Put failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Transactional baseline using TransactionDB::BeginTransaction (WritePrepared policy) with chunked commits.
class RocksDBRaw_Txn10 : public benchmark::Fixture {
protected:
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::string db_path_ = {};
    rocksdb::WriteOptions write_opts_;
    std::atomic<bool> failed_{false};
    std::string first_error_;

    void SetUp(const benchmark::State&) override {
        db_path_ = "C:\\tmp\\bench_raw_txn10_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);

        rocksdb::Options options;
        options.create_if_missing = true;
        options.allow_concurrent_memtable_write = true;
        options.enable_pipelined_write = false; // disable for compatibility with current RocksDB build

        rocksdb::TransactionDBOptions txn_opts;
        txn_opts.write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;

        rocksdb::TransactionDB* db_raw = nullptr;
        auto status = rocksdb::TransactionDB::Open(options, txn_opts, db_path_, &db_raw);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open raw TransactionDB: " + status.ToString());
        }
        db_.reset(db_raw);

        write_opts_.disableWAL = false; // comparable to WP configs
    }

    void TearDown(const benchmark::State&) override {
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            std::unique_ptr<rocksdb::Transaction> txn(db_->BeginTransaction(write_opts_));
            if (!txn) {
                if (!failed_.load(std::memory_order_relaxed)) {
                    first_error_ = "BeginTransaction returned null";
                }
                failed_.store(true, std::memory_order_relaxed);
                return;
            }
            txn->SetName("raw_txn10_" + std::to_string(thread_id) + "_" + std::to_string(produced));
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                std::string key = "raw_txn10_" + std::to_string(thread_id) + "_" + std::to_string(idx);
                std::string val = RandomGenerator::instance().randStr(100);
                auto s = txn->Put(key, val);
                if (!s.ok()) {
                    if (!failed_.load(std::memory_order_relaxed)) {
                        first_error_ = s.ToString();
                    }
                    failed_.store(true, std::memory_order_relaxed);
                    return;
                }
            }
            auto ps = txn->Prepare();
            if (!ps.ok()) {
                if (!failed_.load(std::memory_order_relaxed)) {
                    first_error_ = ps.ToString();
                }
                failed_.store(true, std::memory_order_relaxed);
                return;
            }
            auto cs = txn->Commit();
            if (!cs.ok()) {
                if (!failed_.load(std::memory_order_relaxed)) {
                    first_error_ = cs.ToString();
                }
                failed_.store(true, std::memory_order_relaxed);
                return;
            }
            produced += batch;
        }
    }
};

// Raw Txn10: 1 thread
BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 100, 10);
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawTxn10 failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Txn10: 4 threads
BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 25, 10);
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawTxn10 failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Txn10: 8 threads
BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 12, 10);
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawTxn10 failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Txn10: 16 threads
BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 6, 10);
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawTxn10 failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw Txn10: 32 threads
BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            doTxnChunked(work_id, 3, 10);
        }, 1);
    }
    if (failed_.load(std::memory_order_relaxed)) {
        state.SkipWithError(first_error_.empty() ? "RocksRawTxn10 failed" : first_error_.c_str());
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// THEMIS NoPipelined BASELINES (pipelined_write=false)
// ============================================================================
// Zum Vergleich mit Raw RocksDB Baselines ohne pipelined_write

class ThemisNoPipe_NonTxn : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_themis_npipe_ntx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.disable_wal_for_benchmark = false;
        cfg.enable_wal = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = false;  // disabled for raw comparison

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("themis_npipe_ntx", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }
};

BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_thnpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 100 + i)}});
                sim_->put("themis_npipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_thnpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 25 + i)}});
                sim_->put("themis_npipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_thnpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 12 + i)}});
                sim_->put("themis_npipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_thnpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 6 + i)}});
                sim_->put("themis_npipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_thnpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 3 + i)}});
                sim_->put("themis_npipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

class ThemisNoPipe_Txn10 : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_themis_npipe_txn10_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("themis_npipe_txn10", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            auto txn = db_->beginTransaction();
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                BaseEntity e(std::string("entity_thnpipetxn10_") + std::to_string(thread_id) + "_" + std::to_string(idx),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(thread_id * total_records + idx)}});
                sim_->put("themis_npipe_txn10", e, *txn);
            }
            txn->prepare();
            txn->commit();
            produced += batch;
        }
    }
};

BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 100, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 25, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 12, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 6, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 3, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// THEMIS WITH PIPELINED WRITE (pipelined_write=true)
// ============================================================================
// Zum Vergleich des Pipelined-Write-Effekts auf Themis

class ThemisWithPipe_NonTxn : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_themis_pipe_ntx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.disable_wal_for_benchmark = false;
        cfg.enable_wal = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;  // ENABLED

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("themis_pipe_ntx", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }
};

BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_thpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 100 + i)}});
                sim_->put("themis_pipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_thpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 25 + i)}});
                sim_->put("themis_pipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_thpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 12 + i)}});
                sim_->put("themis_pipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_thpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 6 + i)}});
                sim_->put("themis_pipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_thpipentx_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(work_id * 3 + i)}});
                sim_->put("themis_pipe_ntx", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

class ThemisWithPipe_Txn10 : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::string db_path_ = {};

    void SetUp(const benchmark::State&) override {
        RocksDBWrapper::Config cfg;
        db_path_ = "C:\\tmp\\bench_themis_pipe_txn10_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);
        cfg.db_path = db_path_;

        cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
        cfg.two_write_queues = false;
        cfg.enable_wal = true;
        cfg.disable_wal_for_benchmark = false;
        cfg.allow_concurrent_memtable_write = true;
        cfg.enable_pipelined_write = true;  // ENABLED

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("themis_pipe_txn10", "id");
    }

    void TearDown(const benchmark::State&) override {
        sim_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void doTxnChunked(int thread_id, int total_records, int chunk) {
        int produced = 0;
        while (produced < total_records) {
            int batch = std::min(chunk, total_records - produced);
            auto txn = db_->beginTransaction();
            for (int i = 0; i < batch; ++i) {
                int idx = produced + i;
                BaseEntity e(std::string("entity_thpipetxn10_") + std::to_string(thread_id) + "_" + std::to_string(idx),
                           BaseEntity::FieldMap{{"data", RandomGenerator::instance().randStr(100)},
                                              {"value", static_cast<double>(thread_id * total_records + idx)}});
                sim_->put("themis_pipe_txn10", e, *txn);
            }
            txn->prepare();
            txn->commit();
            produced += batch;
        }
    }
};

BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 100, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 25, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 12, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 6, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) { doTxnChunked(work_id, 3, 10); }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// READ/WRITE RATIO BENCHMARKS
// ============================================================================

/**
 * Base fixture for read/write ratio testing
 */
class ReadWriteRatioBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("rw_ratio");
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("test_data", "id");
        
        // Pre-populate dataset
        populateDataset(10000);
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }
    
    void populateDataset(int count) {
        for (int i = 0; i < count; ++i) {
            BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
            sim_->put("test_data", e);
        }
    }
    
    void performRead(int entity_id) {
        auto results = sim_->scanKeysEqual("test_data", "id",
                                          "entity_" + std::to_string(entity_id));
        benchmark::DoNotOptimize(results);
    }
    
    void performWrite(int entity_id) {
        BaseEntity e("entity_new_" + std::to_string(entity_id), BaseEntity::FieldMap{
            {"data", RandomGenerator::instance().randStr(100)},
            {"value", static_cast<double>(entity_id)}
        });
        sim_->put("test_data", e);
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::mt19937 rng_{std::random_device{}()};
};

// Write Heavy: 80% Write, 20% Read
BENCHMARK_F(ReadWriteRatioBench, WriteHeavy_80W_20R)(benchmark::State& state) {
    std::uniform_int_distribution<int> ops(0, 99);
    std::uniform_int_distribution<int> entity_id(0, 9999);
    
    int write_id = 10000;
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            if (ops(rng_) < 80) {
                performWrite(write_id++);
            } else {
                performRead(entity_id(rng_));
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["write_ratio"] = benchmark::Counter(80.0, benchmark::Counter::kAvgIterations);
}

// Balanced: 50% Write, 50% Read
BENCHMARK_F(ReadWriteRatioBench, Balanced_50W_50R)(benchmark::State& state) {
    std::uniform_int_distribution<int> ops(0, 99);
    std::uniform_int_distribution<int> entity_id(0, 9999);
    
    int write_id = 10000;
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            if (ops(rng_) < 50) {
                performWrite(write_id++);
            } else {
                performRead(entity_id(rng_));
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["write_ratio"] = benchmark::Counter(50.0, benchmark::Counter::kAvgIterations);
}

// Read Heavy: 20% Write, 80% Read
BENCHMARK_F(ReadWriteRatioBench, ReadHeavy_20W_80R)(benchmark::State& state) {
    std::uniform_int_distribution<int> ops(0, 99);
    std::uniform_int_distribution<int> entity_id(0, 9999);
    
    int write_id = 10000;
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            if (ops(rng_) < 20) {
                performWrite(write_id++);
            } else {
                performRead(entity_id(rng_));
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["write_ratio"] = benchmark::Counter(20.0, benchmark::Counter::kAvgIterations);
}

// Read Only: 100% Read
BENCHMARK_F(ReadWriteRatioBench, ReadOnly_0W_100R)(benchmark::State& state) {
    std::uniform_int_distribution<int> entity_id(0, 9999);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            performRead(entity_id(rng_));
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["write_ratio"] = benchmark::Counter(0.0, benchmark::Counter::kAvgIterations);
}

// Write Only: 100% Write
BENCHMARK_F(ReadWriteRatioBench, WriteOnly_100W_0R)(benchmark::State& state) {
    int write_id = 10000;
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            performWrite(write_id++);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["write_ratio"] = benchmark::Counter(100.0, benchmark::Counter::kAvgIterations);
}

// ============================================================================
// PARALLELITY & CONCURRENCY SCALING
// ============================================================================

/**
 * Base fixture for parallelity testing
 */
class ParallelityBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity");
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("parallel_data", "id");
        
        // Pre-populate
        for (int i = 0; i < 10000; ++i) {
            BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
            sim_->put("parallel_data", e);
        }
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Parallel inserts - 1 thread baseline
BENCHMARK_F(ParallelityBench, ParallelInserts_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            BaseEntity e("entity_par_" + std::to_string(work_id), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(work_id)}
            });
            sim_->put("parallel_data", e);
        }, 100);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Parallel inserts - 4 threads
BENCHMARK_F(ParallelityBench, ParallelInserts_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            BaseEntity e("entity_par_" + std::to_string(work_id), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(work_id)}
            });
            sim_->put("parallel_data", e);
        }, 25);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Parallel inserts - 8 threads
BENCHMARK_F(ParallelityBench, ParallelInserts_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            BaseEntity e("entity_par_" + std::to_string(work_id), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(work_id)}
            });
            sim_->put("parallel_data", e);
        }, 12);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Parallel inserts - 16 threads
BENCHMARK_F(ParallelityBench, ParallelInserts_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            BaseEntity e("entity_par_" + std::to_string(work_id), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(work_id)}
            });
            sim_->put("parallel_data", e);
        }, 6);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Parallel inserts - 32 threads
BENCHMARK_F(ParallelityBench, ParallelInserts_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            BaseEntity e("entity_par_" + std::to_string(work_id), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(work_id)}
            });
            sim_->put("parallel_data", e);
        }, 3);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// OPTIMIZED PARALLELITY BENCHMARKS (WITH PER-THREAD DB)
// ============================================================================

/**
 * Optimized parallel benchmark fixture - each thread gets its own DB instance
 * This eliminates lock contention and tests actual parallelizability
 */
class ParallelityBenchOptimized : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        // Setup shared shared metadata (but not actual DB)
        base_path_ = "C:\\tmp\\bench_parallelity_opt_" + 
                    std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::remove_all(base_path_);
        fs::create_directories(base_path_);
    }
    
    void TearDown(const benchmark::State&) override {
        fs::remove_all(base_path_);
    }

protected:
    std::string base_path_ = {};
};

// Optimized: 1 thread with dedicated DB
BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int thread_id) {
            // OPTIMIZED: Per-thread DB instance + thread-local counters
            std::string db_path = base_path_ + "\\db_" + std::to_string(thread_id);
            RocksDBWrapper::Config cfg;
            cfg.db_path = db_path;
            RocksDBWrapper db(cfg);
            SecondaryIndexManager sim(db);
            sim.createIndex("parallel_data", "id");
            
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_opt_" + std::to_string(thread_id) + "_" + std::to_string(i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim.put("parallel_data", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Optimized: 4 threads with dedicated DBs
BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int thread_id) {
            // OPTIMIZED: Per-thread DB instance + thread-local counters
            std::string db_path = base_path_ + "\\db_" + std::to_string(thread_id);
            RocksDBWrapper::Config cfg;
            cfg.db_path = db_path;
            RocksDBWrapper db(cfg);
            SecondaryIndexManager sim(db);
            sim.createIndex("parallel_data", "id");
            
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_opt_" + std::to_string(thread_id) + "_" + std::to_string(i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim.put("parallel_data", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Optimized: 8 threads with dedicated DBs
BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int thread_id) {
            // OPTIMIZED: Per-thread DB instance + thread-local counters
            std::string db_path = base_path_ + "\\db_" + std::to_string(thread_id);
            RocksDBWrapper::Config cfg;
            cfg.db_path = db_path;
            RocksDBWrapper db(cfg);
            SecondaryIndexManager sim(db);
            sim.createIndex("parallel_data", "id");
            
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_opt_" + std::to_string(thread_id) + "_" + std::to_string(i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim.put("parallel_data", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Optimized: 16 threads with dedicated DBs
BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int thread_id) {
            // OPTIMIZED: Per-thread DB instance + thread-local counters
            std::string db_path = base_path_ + "\\db_" + std::to_string(thread_id);
            RocksDBWrapper::Config cfg;
            cfg.db_path = db_path;
            RocksDBWrapper db(cfg);
            SecondaryIndexManager sim(db);
            sim.createIndex("parallel_data", "id");
            
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_opt_" + std::to_string(thread_id) + "_" + std::to_string(i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim.put("parallel_data", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Optimized: 32 threads with dedicated DBs
BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int thread_id) {
            // OPTIMIZED: Per-thread DB instance + thread-local counters
            std::string db_path = base_path_ + "\\db_" + std::to_string(thread_id);
            RocksDBWrapper::Config cfg;
            cfg.db_path = db_path;
            RocksDBWrapper db(cfg);
            SecondaryIndexManager sim(db);
            sim.createIndex("parallel_data", "id");
            
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_opt_" + std::to_string(thread_id) + "_" + std::to_string(i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim.put("parallel_data", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// SHARDED PARALLEL BENCHMARKS (TRUE PARALLELIZATION)
// ============================================================================

/**
 * Sharded parallel benchmark - each thread writes to own table/shard
 * This tests true parallelization without write serialization
 */
class ParallelityBenchSharded : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_sharded");
        
        // Create one SecondaryIndexManager per potential thread (32)
        db_wrapper_ = &fixture_->getDb();
        for (int i = 0; i < 32; ++i) {
            auto sim = std::make_unique<SecondaryIndexManager>(*db_wrapper_);
            sim->createIndex("parallel_shard_" + std::to_string(i), "id");
            shards_.push_back(std::move(sim));
        }
    }
    
    void TearDown(const benchmark::State&) override {
        shards_.clear();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    RocksDBWrapper* db_wrapper_;
    std::vector<std::unique_ptr<SecondaryIndexManager>> shards_;
};

// Sharded: 1 thread (baseline)
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i), 
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 4 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 8 threads (KEY TEST)
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 16 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Sharded: 32 threads
BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int thread_id) {
            auto& sim = shards_[thread_id];
            std::string table = "parallel_shard_" + std::to_string(thread_id);
            
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_shard_" + std::to_string(thread_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(thread_id * 100 + i)}
                });
                sim->put(table, e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 3: ROCKSDB CONFIGURATION OPTIMIZATION
// ============================================================================

/**
 * Phase 3: Test RocksDB configuration tuning for parallel workloads
 * 
 * Strategy: Use optimized RocksDB settings to improve parallel write throughput
 * 
 * Key optimizations:
 * - max_background_jobs = 16 (more compaction/flush threads)
 * - max_write_buffer_number = 8 (more memtables for concurrent writes)
 * - memtable_size_mb = 128 (larger buffers reduce flush frequency)
 * - enable_wal = false (remove WAL overhead for benchmarks)
 */
class ParallelityBenchOptimizedConfig : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_config_opt", true);
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("parallel_config", "id");
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Config Optimized: 1 thread (baseline)
BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_1Thread) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 100; ++i) {
                BaseEntity e("entity_cfg_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_config", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Config Optimized: 4 threads
BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_4Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 25; ++i) {
                BaseEntity e("entity_cfg_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_config", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Config Optimized: 8 threads (KEY TEST)
BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_8Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 12; ++i) {
                BaseEntity e("entity_cfg_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_config", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Config Optimized: 16 threads
BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_16Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 6; ++i) {
                BaseEntity e("entity_cfg_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_config", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Config Optimized: 32 threads
BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_32Threads) (benchmark::State& state) {
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this](int work_id) {
            for (int i = 0; i < 3; ++i) {
                BaseEntity e("entity_cfg_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * 100 + i)}
                });
                sim_->put("parallel_config", e);
            }
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// PHASE 4: BATCH WRITE API OPTIMIZATION
// ============================================================================

/**
 * Phase 4: Test WriteBatch API for amortized lock overhead
 * 
 * Strategy: Batch multiple writes together to reduce lock acquisitions
 * 
 * Key insight:
 * - Individual Put(): 1 record = 1 lock acquisition
 * - WriteBatch: 100 records = 1 lock acquisition
 * - Amortization factor: 100x fewer lock contentions!
 * 
 * Expected Gain: +100-500% (linear with batch size)
 */
class ParallelityBenchBatchWrite : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("parallelity_batch", false);
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("parallel_batch", "id");
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Batch Write: 1 thread (baseline)
BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_1Thread) (benchmark::State& state) {
    const int batch_size = 100;
    for (auto _ : state) {
        ParallelExecutor executor(1);
        executor.execute([this, batch_size](int work_id) {
            auto batch = fixture_->getDb().createWriteBatch();
            for (int i = 0; i < batch_size; ++i) {
                BaseEntity e("entity_batch_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * batch_size + i)}
                });
                // Put into batch (no lock yet!)
                std::vector<uint8_t> serialized = e.serialize();
                batch->put(e.getPrimaryKey(), serialized);
            }
            batch->commit();  // 1 lock for 100 records!
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// Batch Write: 4 threads
BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_4Threads) (benchmark::State& state) {
    const int batch_size = 100;
    for (auto _ : state) {
        ParallelExecutor executor(4);
        executor.execute([this, batch_size](int work_id) {
            auto batch = fixture_->getDb().createWriteBatch();
            for (int i = 0; i < batch_size / 4; ++i) {
                BaseEntity e("entity_batch_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * (batch_size / 4) + i)}
                });
                std::vector<uint8_t> serialized = e.serialize();
                batch->put(e.getPrimaryKey(), serialized);
            }
            batch->commit();  // 1 lock per thread per iteration
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// Batch Write: 8 threads (KEY TEST)
BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_8Threads) (benchmark::State& state) {
    const int batch_size = 100;
    for (auto _ : state) {
        ParallelExecutor executor(8);
        executor.execute([this, batch_size](int work_id) {
            auto batch = fixture_->getDb().createWriteBatch();
            for (int i = 0; i < batch_size / 8; ++i) {
                BaseEntity e("entity_batch_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * (batch_size / 8) + i)}
                });
                std::vector<uint8_t> serialized = e.serialize();
                batch->put(e.getPrimaryKey(), serialized);
            }
            batch->commit();  // 1 lock per thread per iteration
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// Batch Write: 16 threads
BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_16Threads) (benchmark::State& state) {
    const int batch_size = 100;
    for (auto _ : state) {
        ParallelExecutor executor(16);
        executor.execute([this, batch_size](int work_id) {
            auto batch = fixture_->getDb().createWriteBatch();
            for (int i = 0; i < batch_size / 16; ++i) {
                BaseEntity e("entity_batch_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * (batch_size / 16) + i)}
                });
                std::vector<uint8_t> serialized = e.serialize();
                batch->put(e.getPrimaryKey(), serialized);
            }
            batch->commit();
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// Batch Write: 32 threads
BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_32Threads) (benchmark::State& state) {
    const int batch_size = 100;
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this, batch_size](int work_id) {
            auto batch = fixture_->getDb().createWriteBatch();
            for (int i = 0; i < batch_size / 32; ++i) {
                BaseEntity e("entity_batch_" + std::to_string(work_id) + "_" + std::to_string(i),
                           BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(work_id * (batch_size / 32) + i)}
                });
                std::vector<uint8_t> serialized = e.serialize();
                batch->put(e.getPrimaryKey(), serialized);
            }
            batch->commit();
        }, 1);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// ============================================================================
// SELF-PROTECTION & RESILIENCE TESTS
// ============================================================================

/**
 * Base fixture for self-protection testing
 */
class SelfProtectionBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("self_protect");
        sim_ = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim_->createIndex("resilience", "id");
        
        // Pre-populate
        for (int i = 0; i < 5000; ++i) {
            BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(500)},
                {"value", static_cast<double>(i)}
            });
            sim_->put("resilience", e);
        }
    }
    
    void TearDown(const benchmark::State&) override {
        sim_.reset();
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Sustained high load - tests throughput under pressure
BENCHMARK_F(SelfProtectionBench, SustainedLoad_70W_30R)(benchmark::State& state) {
    std::atomic<int> write_counter(0);
    std::uniform_int_distribution<int> ops(0, 99);
    std::uniform_int_distribution<int> read_id(0, 4999);
    std::mt19937 rng{std::random_device{}()};
    
    for (auto _ : state) {
        for (int i = 0; i < 500; ++i) {
            if (ops(rng) < 70) {
                BaseEntity e("sustained_" + std::to_string(write_counter++), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(500)},
                    {"value", static_cast<double>(write_counter)}
                });
                sim_->put("resilience", e);
            } else {
                auto results = sim_->scanKeysEqual("resilience", "id",
                                                   "entity_" + std::to_string(read_id(rng)));
                benchmark::DoNotOptimize(results);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 500);
}

// Burst load - sudden spike in traffic
BENCHMARK_F(SelfProtectionBench, BurstLoad_NormalThen10xSpike)(benchmark::State& state) {
    std::atomic<int> write_counter(0);
    
    for (auto _ : state) {
        // Normal load phase
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("burst_" + std::to_string(write_counter++), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(500)},
                {"value", static_cast<double>(write_counter)}
            });
            sim_->put("resilience", e);
        }
        
        // Burst phase - 10x traffic
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("burst_spike_" + std::to_string(write_counter++), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(500)},
                {"value", static_cast<double>(write_counter)}
            });
            sim_->put("resilience", e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1100);
}

// Connection exhaustion - many concurrent threads
BENCHMARK_F(SelfProtectionBench, ConcurrentConnections_32Threads)(benchmark::State& state) {
    std::atomic<int> write_counter(0);
    
    for (auto _ : state) {
        ParallelExecutor executor(32);
        executor.execute([this, &write_counter](int) {
            BaseEntity e("conn_" + std::to_string(write_counter++), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(500)},
                {"value", static_cast<double>(write_counter)}
            });
            sim_->put("resilience", e);
        }, 10);
    }
    state.SetItemsProcessed(state.iterations() * 32 * 10);
}

// Memory pressure - large documents
BENCHMARK_F(SelfProtectionBench, MemoryPressure_100KB_Documents)(benchmark::State& state) {
    std::atomic<int> write_counter(0);
    
    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            BaseEntity e("mem_" + std::to_string(write_counter++), BaseEntity::FieldMap{
                {"large_data", RandomGenerator::instance().randStr(100000)},
                {"value", static_cast<double>(write_counter)}
            });
            sim_->put("resilience", e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 50);
}

// ============================================================================
// BEST-PRACTICE VS ANTI-PATTERN COMPARISON
// ============================================================================

/**
 * Best-practice pattern testing
 */
class BestPracticeBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("best_practice");
    }
    
    void TearDown(const benchmark::State&) override {
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
};

// Anti-pattern: Creating new index for each operation
BENCHMARK_F(BestPracticeBench, AntiPattern_NewIndex_PerOperation)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
            sim->createIndex("antipattern_" + std::to_string(i), "id");
            
            BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
            sim->put("antipattern_" + std::to_string(i), e);
            sim.reset();
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Best-practice: Reuse index manager and connections
BENCHMARK_F(BestPracticeBench, BestPractice_ReuseIndex_Manager)(benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("best_practice", "id");
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
            sim->put("best_practice", e);
        }
    }
    
    sim.reset();
    state.SetItemsProcessed(state.iterations() * 100);
}

// Best-practice: Batch vs individual operations
BENCHMARK_F(BestPracticeBench, BestPractice_Batch_1000Items)(benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("batch", "id");
    
    for (auto _ : state) {
        std::vector<BaseEntity> batch = {};

        for (int i = 0; i < 1000; ++i) {
            batch.emplace_back("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
        }
        
        // Apply batch (simulated as individual puts)
        for (const auto& entity : batch) {
            sim->put("batch", entity);
        }
    }
    
    sim.reset();
    state.SetItemsProcessed(state.iterations() * 1000);
}

// ============================================================================
// GAP ANALYSIS - COMPARING WITH STANDARDS
// ============================================================================

/**
 * Gap analysis fixture
 * Compares actual performance vs documented/expected performance
 */
class GapAnalysisBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        fixture_ = std::make_unique<DatabaseFixture>("gap_analysis");
    }
    
    void TearDown(const benchmark::State&) override {
        fixture_.reset();
    }

protected:
    std::unique_ptr<DatabaseFixture> fixture_;
};

/**
 * RocksDB baseline: Sequential writes
 * Expected: ~1M writes/sec on SSD (per RocksDB documentation)
 * Actual: Measure to identify gap
 */
BENCHMARK_F(GapAnalysisBench, Gap_RocksDB_SequentialWrites_vs_Expected)
    (benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("rocksdb_baseline", "id");
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("seq_" + std::to_string(i), BaseEntity::FieldMap{
                {"data", RandomGenerator::instance().randStr(100)},
                {"value", static_cast<double>(i)}
            });
            sim->put("rocksdb_baseline", e);
        }
    }
    
    sim.reset();
    // Expected: 1M ops/sec, actual measurement shows gap
    state.SetItemsProcessed(state.iterations() * 1000);
}

/**
 * Random access performance
 * Expected: 10-50% of sequential throughput
 * Actual: Measure to identify gap
 */
BENCHMARK_F(GapAnalysisBench, Gap_RandomVsSequential_AccessPattern)
    (benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("random_access", "id");
    
    // Pre-populate
    for (int i = 0; i < 10000; ++i) {
        BaseEntity e("entity_" + std::to_string(i), BaseEntity::FieldMap{
            {"data", RandomGenerator::instance().randStr(100)},
            {"value", static_cast<double>(i)}
        });
        sim->put("random_access", e);
    }
    
    std::uniform_int_distribution<int> dis(0, 9999);
    std::mt19937 rng(std::random_device{}());
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            int idx = dis(rng);
            auto results = sim->scanKeysEqual("random_access", "id",
                                             "entity_" + std::to_string(idx));
            benchmark::DoNotOptimize(results);
        }
    }
    
    sim.reset();
    state.SetItemsProcessed(state.iterations() * 100);
}

/**
 * Concurrent operations efficiency
 * Expected: Near-linear scaling up to CPU cores (then sublinear)
 * Actual: Measure to identify bottlenecks
 */
BENCHMARK_F(GapAnalysisBench, Gap_ConcurrencyScaling_8Threads) (benchmark::State& state) {
    std::atomic<int> counter(0);
    
    for (auto _ : state) {
        std::vector<std::thread> threads = {};

        for (int t = 0; t < 8; ++t) {
            threads.emplace_back([this, &counter, t]() {
                auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
                sim->createIndex("concurrent_" + std::to_string(t), "id");
                
                for (int i = 0; i < 100; ++i) {
                    BaseEntity e("conc_" + std::to_string(counter++), BaseEntity::FieldMap{
                        {"data", RandomGenerator::instance().randStr(100)},
                        {"value", static_cast<double>(counter)}
                    });
                    sim->put("concurrent_" + std::to_string(t), e);
                }
                sim.reset();
            });
        }
        for (auto& t : threads) {
            t.join();
        }
    }
    state.SetItemsProcessed(state.iterations() * 8 * 100);
}

/**
 * Transaction overhead
 * Expected: <5% overhead vs non-transactional
 * Actual: Measure overhead
 */
BENCHMARK_F(GapAnalysisBench, Gap_TransactionOverhead_MultiOp)
    (benchmark::State& state) {
    auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
    sim->createIndex("transaction_test", "id");
    
    for (auto _ : state) {
        for (int batch = 0; batch < 100; ++batch) {
            // Simulate transaction: 5 related operations
            for (int i = 0; i < 5; ++i) {
                BaseEntity e("tx_" + std::to_string(batch * 5 + i), BaseEntity::FieldMap{
                    {"data", RandomGenerator::instance().randStr(100)},
                    {"value", static_cast<double>(i)}
                });
                sim->put("transaction_test", e);
            }
        }
    }
    
    sim.reset();
    state.SetItemsProcessed(state.iterations() * 100 * 5);
}

/**
 * Index creation overhead
 * Expected: O(1) for empty index, O(n*log(n)) for populated
 * Actual: Measure to identify gaps
 */
BENCHMARK_F(GapAnalysisBench, Gap_IndexCreation_NewIndexCost)
    (benchmark::State& state) {
    int counter = 0;
    
    for (auto _ : state) {
        auto sim = std::make_unique<SecondaryIndexManager>(fixture_->getDb());
        sim->createIndex("gap_idx_" + RandomGenerator::instance().randStr(10), "id");
        sim.reset();
        counter++;
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// PHASE 2H: BACKGROUND THREAD OPTIMIZATIONS (Simple Write Test)
// Testing impact of max_background_compactions on high parallelism workloads
// ============================================================================

static void BM_Phase2H_BgThreads(benchmark::State& state) {
    int num_threads = state.range(0);
    int bg_compactions = state.range(1);
    
    std::string path = "C:\\tmp\\bench_phase2h_bgthreads_" + std::to_string(num_threads) + "_" + std::to_string(bg_compactions);
    fs::remove_all(path);
    fs::create_directories(path);
    
    RocksDBWrapper::Config config;
    config.db_path = path;
    config.memtable_size_mb = 128;
    config.block_cache_size_mb = 512;
    config.max_write_buffer_number = 4;
    config.db_write_buffer_size_mb = 512;
    
    config.max_background_compactions = bg_compactions;
    config.max_background_flushes = std::max(1, bg_compactions / 2);
    config.background_threads_low = bg_compactions;
    config.background_threads_high = std::max(1, bg_compactions / 2);
    config.max_subcompactions = std::max(1, bg_compactions / 2);
    
    config.allow_concurrent_memtable_write = true;
    config.enable_pipelined_write = false;
    config.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        state.SkipWithError("Failed to open DB");
        return;
    }
    
    auto sim = std::make_unique<SecondaryIndexManager>(*db);
    sim->createIndex("phase2h_test", "id");
    
    std::atomic<int64_t> work_counter{0};
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::atomic<bool> failed{false};
        state.ResumeTiming();
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                try {
                    for (int i = 0; i < 10; ++i) {
                        int64_t work_id = work_counter.fetch_add(1, std::memory_order_relaxed);
                        BaseEntity e("key_" + std::to_string(work_id), BaseEntity::FieldMap{
                            {"data", "value_" + std::to_string(work_id)},
                            {"value", static_cast<double>(work_id)}
                        });
                        sim->put("phase2h_test", e);
                    }
                } catch (...) {
                    failed.store(true);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (failed.load()) {
            state.SkipWithError("Thread failed");
        }
    }
    
    sim.reset();
    db->close();
    
    state.SetItemsProcessed(state.iterations() * num_threads * 10);
}

BENCHMARK(BM_Phase2H_BgThreads)
    ->Args({1, 1})->Args({4, 1})->Args({8, 1})->Args({16, 1})
    ->Args({1, 2})->Args({4, 2})->Args({8, 2})->Args({16, 2})
    ->Args({1, 4})->Args({4, 4})->Args({8, 4})->Args({16, 4})
    ->Args({1, 8})->Args({4, 8})->Args({8, 8})->Args({16, 8})
    ->Unit(benchmark::kMillisecond)->UseRealTime();

// ============================================================================
// PHASE 2H: FULL OPTIMIZED CONFIG
// All optimizations combined for best high-parallelism performance
// ============================================================================

static void BM_Phase2H_FullOptimized(benchmark::State& state) {
    int num_threads = state.range(0);
    
    std::string path = "C:\\tmp\\bench_phase2h_full_" + std::to_string(num_threads);
    fs::remove_all(path);
    fs::create_directories(path);
    
    RocksDBWrapper::Config config;
    config.db_path = path;
    
    // Write buffer optimizations
    config.memtable_size_mb = 128;
    config.max_write_buffer_number = 4;
    config.db_write_buffer_size_mb = 512;
    
    // Cache optimizations
    config.block_cache_size_mb = 512;
    config.block_cache_shard_bits = 6;  // 64 shards
    
    // Background thread optimizations
    config.max_background_compactions = 8;
    config.max_background_flushes = 2;
    config.background_threads_low = 8;
    config.background_threads_high = 2;
    config.max_subcompactions = 4;
    
    // Level0 optimizations
    config.level0_file_num_compaction_trigger = 2;
    config.level0_slowdown_writes_trigger = 8;
    config.level0_stop_writes_trigger = 16;
    
    // Parallel write optimizations
    config.allow_concurrent_memtable_write = true;
    config.enable_pipelined_write = false;
    config.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        state.SkipWithError("Failed to open DB");
        return;
    }
    
    auto sim = std::make_unique<SecondaryIndexManager>(*db);
    sim->createIndex("phase2h_full", "id");
    
    std::atomic<int64_t> work_counter{0};
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::atomic<bool> failed{false};
        state.ResumeTiming();
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                try {
                    for (int i = 0; i < 10; ++i) {
                        int64_t work_id = work_counter.fetch_add(1, std::memory_order_relaxed);
                        BaseEntity e("key_" + std::to_string(work_id), BaseEntity::FieldMap{
                            {"data", "value_" + std::to_string(work_id)},
                            {"value", static_cast<double>(work_id)}
                        });
                        sim->put("phase2h_full", e);
                    }
                } catch (...) {
                    failed.store(true);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (failed.load()) {
            state.SkipWithError("Thread failed");
        }
    }
    
    sim.reset();
    db->close();
    
    state.SetItemsProcessed(state.iterations() * num_threads * 10);
}

BENCHMARK(BM_Phase2H_FullOptimized)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond)->UseRealTime();

// ============================================================================
// NOTE: duplicate sharded benchmark block removed (definitions already exist above). Keeping single BENCHMARK_MAIN below.

BENCHMARK_MAIN();

// Benchmark: Transaction Throughput
// Measures ACID transaction performance for different workload patterns
//
// TX-4 (2PC Latency, 5 Shards) — direct benchmark via TwoPhaseCommitFixture:
//   Uses TwoPhaseCommitCoordinator + 5 in-process TwoPhaseCommitParticipant mocks.
//   No WAL, null storage callbacks → measures pure protocol latency.
//   SLO: ≤ 5 ms per cross-shard commit.
//
// TX-6 (Deadlock Detection Overhead) — direct benchmark via DeadlockDetectionFixture:
//   Pre-trains DeadlockPredictor with 1 000 lock patterns (5 % deadlock rate),
//   then measures predictDeadlockProbability() and recommendLockOrder() overhead
//   per transaction decision. SLO: ≤ 1 % overhead delta vs. baseline.

#include "transaction/transaction_manager.h"
#include "transaction/deadlock_predictor.h"
#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/two_phase_commit_participant.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <sstream>
#include <chrono>

using namespace themis;

// ============================================================================
// Test Setup
// ============================================================================

class TransactionBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Clean up any existing test database
         const auto unique_id = static_cast<unsigned long long>(
             std::chrono::steady_clock::now().time_since_epoch().count());
        std::ostringstream suffix = {};
        suffix << "bench_transaction_tmp_t"
               << state.thread_index()
             << "_" << unique_id;
        const auto db_dir = std::filesystem::absolute(std::filesystem::path("data") / suffix.str());
        test_db_path_ = db_dir.string();
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.wal_dir = (db_dir / "wal").string();
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;
        config.max_write_buffer_number = 3; // entfernt: write_buffer_size (nicht mehr vorhanden)
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Create index managers
        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);
        
        // Create transaction manager
        tx_manager_ = std::make_unique<TransactionManager>(
            *db_, *secondary_index_, *graph_index_, *vector_index_);
        
        // Pre-populate with some data for read tests
        populateTestData(1000);
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void populateTestData(size_t count) {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> age_dist(18, 80);
        std::uniform_real_distribution<double> balance_dist(0.0, 100000.0);
        
        for (size_t i = 0; i < count; i++) {
            auto txn_id = tx_manager_->beginTransaction();
            auto txn = tx_manager_->getTransaction(txn_id);
            
            BaseEntity entity("user_" + std::to_string(i));
            entity.setField("name", std::string("User_") + std::to_string(i));
            entity.setField("age", static_cast<int64_t>(age_dist(rng)));
            entity.setField("balance", balance_dist(rng));
            entity.setField("active", true);
            
            txn->putEntity("users", entity);
            tx_manager_->commitTransaction(txn_id);
        }
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
    std::unique_ptr<GraphIndexManager> graph_index_;
    std::unique_ptr<VectorIndexManager> vector_index_;
    std::unique_ptr<TransactionManager> tx_manager_;
};

// ============================================================================
// Benchmark: Read-Only Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, ReadOnlyTransaction)(benchmark::State& state) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 999);
    
    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        
        // Simulierter Read: keine direkte getEntity API mehr – wir lassen den Körper leer
        for (int i = 0; i < 10; i++) {
            benchmark::DoNotOptimize(i);
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }
    
    auto stats = tx_manager_->getStats();
    state.SetItemsProcessed(state.iterations() * 10); // 10 reads per transaction
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    state.counters["total_committed"] = stats.total_committed;
    state.counters["total_aborted"] = stats.total_aborted;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, ReadOnlyTransaction)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Write-Only Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, WriteOnlyTransaction)(benchmark::State& state) {
    size_t counter = 0;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> balance_dist(0.0, 100000.0);
    
    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        
        // Write multiple entities
        for (int i = 0; i < 10; i++) {
            std::string key = "new_user_" + std::to_string(counter++);
            
            BaseEntity entity(key);
            entity.setField("name", std::string("NewUser_") + std::to_string(counter));
            entity.setField("balance", balance_dist(rng));
            entity.setField("created_at", std::chrono::system_clock::now().time_since_epoch().count());
            
            auto put_status = txn->putEntity("users", entity);
            if (!put_status.ok) {
                state.SkipWithError("Put entity failed");
                break;
            }
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10); // 10 writes per transaction
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, WriteOnlyTransaction)
    ->Threads(1)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Mixed Read/Write Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, MixedTransaction)(benchmark::State& state) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> user_dist(0, 999);
    std::uniform_real_distribution<double> amount_dist(-1000.0, 1000.0);
    
    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        
        // Read-Modify-Write pattern (e.g., account balance update)
        for (int i = 0; i < 5; i++) {
            std::string key = "user_" + std::to_string(user_dist(rng));
            
            // Vereinfachtes Read-Modify-Write ohne getEntity: wir erzeugen neuen Entity mit aktualisiertem balance
            BaseEntity entity(key);
            double new_balance = std::max(0.0, amount_dist(rng));
            entity.setField("balance", new_balance);
            txn->putEntity("users", entity);
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 5); // 5 RMW operations per transaction
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, MixedTransaction)
    ->Threads(1)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Commit Latency Distribution
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, CommitLatency)(benchmark::State& state) {
    const int ops_per_txn = state.range(0);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> value_dist(0.0, 1000.0);
    size_t counter = 0;
    
    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        
        // Perform operations
        for (int i = 0; i < ops_per_txn; i++) {
            BaseEntity entity("temp_" + std::to_string(counter++));
            entity.setField("value", value_dist(rng));
            txn->putEntity("temp", entity);
        }
        
        // Measure commit time
        auto start = std::chrono::high_resolution_clock::now();
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
        
        auto commit_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        benchmark::DoNotOptimize(commit_time);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["ops_per_txn"] = ops_per_txn;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, CommitLatency)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Abort Performance
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, AbortTransaction)(benchmark::State& state) {
    size_t counter = 0;
    
    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        
        // Perform some operations
        for (int i = 0; i < 10; i++) {
            BaseEntity entity("abort_test_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            txn->putEntity("temp", entity);
        }
        
        // Rollback statt Commit (abort API entfernt)
        tx_manager_->rollbackTransaction(txn_id);
    }
    
    auto stats = tx_manager_->getStats();
    state.SetItemsProcessed(state.iterations());
    state.counters["aborts_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    state.counters["total_aborted"] = stats.total_aborted;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, AbortTransaction)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Savepoint Create and Rollback
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointCreateAndRollback)(benchmark::State& state) {
    const int writes_before = state.range(0);
    const int writes_after  = state.range(1);
    size_t counter = 0;

    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        // Writes committed to the transaction before the savepoint
        for (int i = 0; i < writes_before; ++i) {
            BaseEntity entity("sp_before_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            txn->putEntity("bench", entity);
        }

        // Create a named savepoint
        txn->createSavepoint("bench_sp");

        // Writes that will be rolled back
        for (int i = 0; i < writes_after; ++i) {
            BaseEntity entity("sp_after_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            txn->putEntity("bench", entity);
        }

        // Partial rollback to the savepoint (discards the writes_after entities)
        txn->rollbackToSavepoint("bench_sp");

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }

    // Count both kept and rolled-back writes to reflect total work performed
    // (including savepoint overhead and rollback cost).
    state.SetItemsProcessed(state.iterations() * (writes_before + writes_after));
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointCreateAndRollback)
    ->Args({0, 5})    // 0 writes before, 5 rolled back
    ->Args({5, 5})    // 5 writes kept,   5 rolled back
    ->Args({5, 20})   // 5 writes kept,  20 rolled back
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Nested Savepoints with Rollback
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointNested)(benchmark::State& state) {
    const int depth = state.range(0); // number of nested savepoints
    size_t counter  = 0;

    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        // Create `depth` nested savepoints, each with one write
        for (int d = 0; d < depth; ++d) {
            BaseEntity entity("sp_nest_" + std::to_string(counter++));
            entity.setField("depth", static_cast<int64_t>(d));
            txn->putEntity("bench", entity);
            txn->createSavepoint("sp_" + std::to_string(d));
        }

        // Write after the last savepoint (will be rolled back)
        BaseEntity last("sp_last_" + std::to_string(counter++));
        last.setField("depth", static_cast<int64_t>(depth));
        txn->putEntity("bench", last);

        // Rollback to the first savepoint: discards writes made after "sp_0" was
        // created (depth-1 writes + the final write).  The write at d=0 is kept
        // because it was committed to the transaction before "sp_0" was set.
        txn->rollbackToSavepoint("sp_0");

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }

    state.SetItemsProcessed(state.iterations() * (depth + 1));
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    state.counters["savepoint_depth"] = depth;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointNested)
    ->Arg(2)
    ->Arg(5)
    ->Arg(10)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Savepoint Release (no rollback)
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointRelease)(benchmark::State& state) {
    const int num_savepoints = state.range(0);
    size_t counter = 0;

    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        // Create several savepoints and release them without rolling back
        for (int i = 0; i < num_savepoints; ++i) {
            BaseEntity entity("sp_rel_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(i));
            txn->putEntity("bench", entity);
            txn->createSavepoint("sp_" + std::to_string(i));
        }

        // Release the first savepoint (also releases all newer ones)
        txn->releaseSavepoint("sp_0");

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }

    state.SetItemsProcessed(state.iterations() * num_savepoints);
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointRelease)
    ->Arg(1)
    ->Arg(5)
    ->Arg(10)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Optimistic Concurrency Control (OCC) – new entity creation
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticPut)(benchmark::State& state) {
    size_t counter = 0;

    for (auto _ : state) {
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        // Create a new entity with expected_version=0 (OCC insert)
        std::string pk = "occ_new_" + std::to_string(counter++);
        BaseEntity entity(pk);
        entity.setField("value", static_cast<int64_t>(counter));

        auto st = txn->optimisticPut("occ_bench", entity, 0);
        if (!st.ok) {
            state.SkipWithError("optimisticPut failed");
            break;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccOptimisticPut)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: OCC Read-Modify-Write pattern (getEntityVersion + optimisticPut)
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccReadVersionAndUpdate)(benchmark::State& state) {
    // Pre-insert a set of entities that the benchmark will update
    const int num_entities = 100;
    for (int i = 0; i < num_entities; ++i) {
        auto setup_id = tx_manager_->beginTransaction();
        auto setup_txn = tx_manager_->getTransaction(setup_id);
        BaseEntity e("occ_rmw_" + std::to_string(i));
        e.setField("counter", static_cast<int64_t>(0));
        auto setup_st = setup_txn->optimisticPut("occ_rmw", e, 0);
        if (!setup_st.ok) {
            state.SkipWithError("OccReadVersionAndUpdate setup: optimisticPut failed");
            return;
        }
        auto commit_st = tx_manager_->commitTransaction(setup_id);
        if (!commit_st.ok) {
            state.SkipWithError("OccReadVersionAndUpdate setup: commit failed");
            return;
        }
    }

    // Fixed seed for reproducible benchmark results across threads
    std::mt19937 rng(42u + static_cast<unsigned>(state.thread_index()));
    std::uniform_int_distribution<int> key_dist(0, num_entities - 1);
    uint64_t conflicts = 0;

    for (auto _ : state) {
        std::string pk = "occ_rmw_" + std::to_string(key_dist(rng));

        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        // OCC read-modify-write: read version, then write with version check
        auto ver = txn->getEntityVersion("occ_rmw", pk);
        if (!ver.has_value() || *ver == 0) {
            tx_manager_->rollbackTransaction(txn_id);
            continue;
        }

        BaseEntity updated(pk);
        updated.setField("counter", static_cast<int64_t>(*ver));

        auto st = txn->optimisticPut("occ_rmw", updated, *ver);
        if (!st.ok) {
            // OCC conflict: another writer changed the version
            ++conflicts;
            tx_manager_->rollbackTransaction(txn_id);
            continue;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            ++conflicts;
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["tps"]       = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.counters["conflicts"] = benchmark::Counter(static_cast<double>(conflicts));
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccReadVersionAndUpdate)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: OCC erase (optimisticErase)
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticErase)(benchmark::State& state) {
    size_t counter = 0;

    for (auto _ : state) {
        state.PauseTiming();
        // Create entity to be erased
        std::string pk = "occ_erase_" + std::to_string(counter++);
        {
            auto ins_id = tx_manager_->beginTransaction();
            auto ins_txn = tx_manager_->getTransaction(ins_id);
            BaseEntity e(pk);
            e.setField("v", static_cast<int64_t>(1));
            ins_txn->optimisticPut("occ_erase", e, 0);
            tx_manager_->commitTransaction(ins_id);
        }
        state.ResumeTiming();

        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);

        auto st = txn->optimisticErase("occ_erase", pk, 1);
        if (!st.ok) {
            state.SkipWithError("optimisticErase failed");
            break;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccOptimisticErase)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Concurrent Transaction Contention
// ============================================================================

static void BM_TransactionContention(benchmark::State& state) {
    // Setup
    const auto unique_id = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::ostringstream suffix = {};
    suffix << "bench_transaction_contention_tmp_t"
           << state.thread_index()
           << "_" << unique_id;
    const auto db_dir = std::filesystem::absolute(std::filesystem::path("data") / suffix.str());
    std::string test_db_path = db_dir.string();
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.wal_dir = (db_dir / "wal").string();
    config.memtable_size_mb = 128;
    config.block_cache_size_mb = 256;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    
    auto secondary_index = std::make_unique<SecondaryIndexManager>(*db);
    auto graph_index = std::make_unique<GraphIndexManager>(*db);
    auto vector_index = std::make_unique<VectorIndexManager>(*db);
    auto tx_manager = std::make_unique<TransactionManager>(
        *db, *secondary_index, *graph_index, *vector_index);
    
    // All threads contend on same key
    const std::string contended_key = "contended_resource";
    
    uint64_t conflicts = 0;
    for (auto _ : state) {
        auto txn_id = tx_manager->beginTransaction();
        auto txn = tx_manager->getTransaction(txn_id);
        
        // Vereinfachter Contention-Test ohne getEntity: atomare Ersetzung
        static int64_t local_counter = 0;
        local_counter++;
        BaseEntity entity(contended_key);
        entity.setField("counter", local_counter);
        txn->putEntity("resources", entity);
        
        // Try to commit (may fail due to contention)
        auto commit_status = tx_manager->commitTransaction(txn_id);
        if (!commit_status.ok) {
            // Track conflicts as a numeric counter
            ++conflicts;
        }
    }
    
    // Cleanup
    tx_manager.reset();
    vector_index.reset();
    graph_index.reset();
    secondary_index.reset();
    db->close();
    db.reset();
    
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    state.SetItemsProcessed(state.iterations());
    // Report total conflicts encountered during the benchmark
    state.counters["conflicts"] = benchmark::Counter(static_cast<double>(conflicts));
}

BENCHMARK(BM_TransactionContention)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TX-4: Two-Phase Commit Latency (5 Shards) — direct SLO benchmark
//
// Measures end-to-end 2PC commit latency across N in-process shard mocks.
// Each participant uses null storage callbacks (accepts every PREPARE/COMMIT),
// so the benchmark isolates protocol overhead — i.e. the Phase-1 fan-out and
// Phase-2 broadcast cost — without storage I/O noise.
//
// SLO (TX-4): ≤ 5 ms per commit across 5 shards.
// ============================================================================

/// @brief Fixture for two-phase commit latency benchmarks (TX-4).
///
/// Creates one TwoPhaseCommitCoordinator and @p num_shards in-process
/// TwoPhaseCommitParticipant instances. WAL is disabled so the benchmark
/// measures pure protocol (PREPARE fan-out + COMMIT broadcast) latency.
class TwoPhaseCommitFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const auto num_shards = static_cast<int>(state.range(0));

        // Build participants first (null callbacks → accept every operation).
        for (int i = 0; i < num_shards; ++i) {
            auto shard_id = "bench_shard_" + std::to_string(i);
            participants_.emplace(
                shard_id,
                std::make_unique<themis::sharding::TwoPhaseCommitParticipant>(shard_id)
            );
        }

        // Coordinator: WAL disabled to measure protocol latency only.
        themis::sharding::TwoPhaseCommitCoordinator::Config cfg;
        cfg.wal_directory   = "";    // disable durable logging
        cfg.sync_wal_writes = false;
        coordinator_ = std::make_unique<themis::sharding::TwoPhaseCommitCoordinator>(
            "bench_coordinator", cfg
        );
        for (auto& [id, p] : participants_) {
            coordinator_->registerParticipant(id, p.get());
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        coordinator_.reset();
        participants_.clear();
    }

private:
    std::map<std::string,
             std::unique_ptr<themis::sharding::TwoPhaseCommitParticipant>> participants_;
    std::unique_ptr<themis::sharding::TwoPhaseCommitCoordinator>            coordinator_;

protected:
    /// Access from benchmark body
    themis::sharding::TwoPhaseCommitCoordinator& coordinator() { return *coordinator_; }
    const std::map<std::string,
                   std::unique_ptr<themis::sharding::TwoPhaseCommitParticipant>>&
    participants() const { return participants_; }
};

/// @brief Benchmark: end-to-end 2PC commit latency across N in-process shards.
///
/// Each iteration commits one transaction via the full two-phase protocol:
/// PREPARE to all N shards, then COMMIT (or ABORT on failure). The operation
/// payload is a single JSON string per shard — enough to exercise the
/// protocol framing without adding storage overhead.
///
/// @param state  Benchmark state; range(0) = number of shards.
/// @note  SLO TX-4: ≤ 5 ms per commit for 5 shards.
BENCHMARK_DEFINE_F(TwoPhaseCommitFixture, TwoPhaseCommitLatency)(benchmark::State& state) {
    const auto num_shards = static_cast<int>(state.range(0));
    uint64_t   txn_seq    = 0;
    uint64_t   aborts     = 0;

    // Shared per-shard payload: a single write operation per shard per commit.
    nlohmann::json shard_ops = nlohmann::json::array();
    shard_ops.push_back("SET bench_key bench_value");

    for (auto _ : state) {
        std::map<std::string, nlohmann::json> ops_per_shard = {};

        for (int i = 0; i < num_shards; ++i) {
            ops_per_shard["bench_shard_" + std::to_string(i)] = shard_ops;
        }

        const auto outcome = coordinator().commit(
            "bench_2pc_" + std::to_string(txn_seq++), ops_per_shard
        );
        if (!outcome.committed()) {
            ++aborts;
        }
    }

    state.counters["tps"]    = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
    state.counters["aborts"] = benchmark::Counter(static_cast<double>(aborts));
    state.counters["shards"] = static_cast<double>(num_shards);
}

// Arg(5) = TX-4 SLO target (5 shards, ≤ 5 ms); Arg(3) sanity baseline.
BENCHMARK_REGISTER_F(TwoPhaseCommitFixture, TwoPhaseCommitLatency)
    ->Arg(3)
    ->Arg(5)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// TX-6: Deadlock Detection Overhead — direct SLO benchmark
//
// Measures the per-transaction cost of calling DeadlockPredictor in both
// prediction and lock-ordering modes, using a pre-trained predictor that
// mirrors a realistic workload (5 % observed deadlock rate, 20 hot keys,
// 3..5 locks per transaction).
//
// Two cases are provided:
//   1. DeadlockDetectionOverhead_Predict   — predictDeadlockProbability()
//   2. DeadlockDetectionOverhead_LockOrder — recommendLockOrder()
//
// SLO (TX-6): overhead of either call ≤ 1 % of a typical transaction's
//             wall-clock cost (baseline ~0.1 ms).
// ============================================================================

/// @brief Fixture for deadlock-detection overhead benchmarks (TX-6).
///
/// Pre-trains DeadlockPredictor with 1 000 realistic lock patterns including
/// a 5 % deadlock event rate before any benchmark iteration runs.
class DeadlockDetectionFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        predictor_ = std::make_unique<DeadlockPredictor>();

        // Warm up with 1 000 transactions over a 20-key space (3..5 keys each).
        constexpr int kWarmupCount = 1000;
        constexpr int kKeySpace    = 20;
        constexpr int kDeadlockEvery = 20; // ~5 % deadlock rate

        std::mt19937 rng(42);
        std::uniform_int_distribution<int> key_dist(0, kKeySpace - 1);
        std::uniform_int_distribution<int> num_keys_dist(3, 5);
        std::uniform_int_distribution<int> hold_us_dist(50, 950);

        for (int i = 0; i < kWarmupCount; ++i) {
            std::vector<std::string> keys;
            const int nk = num_keys_dist(rng);
            keys.reserve(static_cast<size_t>(nk));
            for (int k = 0; k < nk; ++k) {
                keys.push_back("lock_key_" + std::to_string(key_dist(rng)));
            }
            predictor_->recordTransaction(
                static_cast<uint64_t>(i),
                keys,
                std::chrono::microseconds(hold_us_dist(rng))
            );
            if (i % kDeadlockEvery == 0) {
                predictor_->recordDeadlock(keys);
            }
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        predictor_.reset();
    }

protected:
    DeadlockPredictor& predictor() { return *predictor_; }

private:
    std::unique_ptr<DeadlockPredictor> predictor_;
};

/// @brief Benchmark: predictDeadlockProbability() call overhead (TX-6 direct).
///
/// Each iteration queries the pre-trained predictor for a 5-key lock set drawn
/// from the hot 20-key space. The result is consumed via DoNotOptimize to
/// prevent the call from being elided.
///
/// @note  SLO TX-6: this call should consume ≤ 1 % of a typical transaction's
///        wall-clock budget (~0.1 ms base → ≤ 1 µs target).
BENCHMARK_DEFINE_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_Predict)(
    benchmark::State& state)
{
    constexpr int kKeySpace = 20;
    std::mt19937  rng(99);
    std::uniform_int_distribution<int> key_dist(0, kKeySpace - 1);
    const std::set<DeadlockPredictor::TransactionId> active_txns = {1u, 2u, 3u};

    for (auto _ : state) {
        std::vector<std::string> proposed = {
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
        };
        const double prob = predictor().predictDeadlockProbability(proposed, active_txns);
        benchmark::DoNotOptimize(prob);
    }

    state.counters["calls_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
    state.counters["trained_patterns"] = static_cast<double>(
        predictor().recordedTransactionCount());
}

BENCHMARK_REGISTER_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_Predict)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

/// @brief Benchmark: recommendLockOrder() call overhead (TX-6 direct).
///
/// Each iteration asks the pre-trained predictor to sort a 5-key candidate
/// set into a safe acquisition order. This exercises the pair-conflict matrix
/// traversal that dominates the predictor's steady-state cost.
///
/// @note  SLO TX-6: same overhead budget as DeadlockDetectionOverhead_Predict.
BENCHMARK_DEFINE_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_LockOrder)(
    benchmark::State& state)
{
    constexpr int kKeySpace = 20;
    std::mt19937  rng(77);
    std::uniform_int_distribution<int> key_dist(0, kKeySpace - 1);

    for (auto _ : state) {
        std::vector<std::string> candidates = {
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
            "lock_key_" + std::to_string(key_dist(rng)),
        };
        auto order = predictor().recommendLockOrder(candidates);
        benchmark::DoNotOptimize(order);
    }

    state.counters["calls_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_LockOrder)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

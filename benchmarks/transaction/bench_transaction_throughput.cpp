// Benchmark: Transaction Throughput
// Measures ACID transaction performance for different workload patterns
//
// TX-4 (2PC Latency, 5 Shards) — direct benchmark via TwoPhaseCommitFixture:
//   Uses TwoPhaseCommitCoordinator + 5 in-process TwoPhaseCommitParticipant mocks.
//   A unique coordinator/participant WAL and valid per-shard fencing tokens
//   exercise the complete durable in-process protocol with null storage callbacks.
//   SLO: ≤ 5 ms per cross-shard commit.
//
// TX-6 (Deadlock Detection Call Latency) — direct benchmark via DeadlockDetectionFixture:
//   Pre-trains DeadlockPredictor with 1 000 lock patterns (5 % deadlock rate),
//   then measures predictDeadlockProbability() and recommendLockOrder() overhead
//   per call. SLO: ≤ 1 us.

#include "transaction/transaction_manager.h"
#include "transaction/deadlock_predictor.h"
#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/two_phase_commit_participant.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "../benchmark_cleanup.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <array>
#include <barrier>
#include <cmath>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <optional>
#include <string>
#include <thread>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <vector>

using namespace themis;

namespace {

void publishOutcomeCounters(benchmark::State& state,
                            uint64_t attempted,
                            uint64_t committed,
                            uint64_t aborted) {
    state.counters["attempted"] = static_cast<double>(attempted);
    state.counters["committed"] = static_cast<double>(committed);
    state.counters["aborted"] = static_cast<double>(aborted);
    state.counters["retried"] = 0.0;
}

std::array<double, 3> nearestRankPercentilesUs(std::vector<double>& samples) {
    if (samples.empty()) {
        return {};
    }

    std::sort(samples.begin(), samples.end());
    const auto nearest_rank = [&samples](double percentile) {
        const auto rank = static_cast<size_t>(
            std::ceil(percentile * static_cast<double>(samples.size())));
        return samples[std::max<size_t>(1, rank) - 1];
    };
    return {nearest_rank(0.50), nearest_rank(0.95), nearest_rank(0.99)};
}

}  // namespace

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
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_dir, cleanup_error)) {
            throw std::runtime_error("Failed to clear transaction benchmark DB: " + cleanup_error.message());
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
        
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(test_db_path_, cleanup_error)) {
            std::fprintf(stderr, "Transaction benchmark DB cleanup failed: %s\n",
                         cleanup_error.message().c_str());
        }
    }
    
    void populateTestData(size_t count) {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> age_dist(18, 80);
        std::uniform_real_distribution<double> balance_dist(0.0, 100000.0);
        
        for (size_t i = 0; i < count; i++) {
            auto txn_id = tx_manager_->beginTransaction();
            auto txn = tx_manager_->getTransaction(txn_id);
            if (!txn) {
                throw std::runtime_error("Failed to start transaction while populating benchmark data");
            }
            
            BaseEntity entity("user_" + std::to_string(i));
            entity.setField("name", std::string("User_") + std::to_string(i));
            entity.setField("age", static_cast<int64_t>(age_dist(rng)));
            entity.setField("balance", balance_dist(rng));
            entity.setField("active", true);
            
            const auto put_status = txn->putEntity("users", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                throw std::runtime_error("Failed to populate transaction benchmark data: " +
                                         put_status.message);
            }

            const auto commit_status = tx_manager_->commitTransaction(txn_id);
            if (!commit_status.ok) {
                throw std::runtime_error("Failed to commit transaction benchmark setup data: " +
                                         commit_status.message);
            }
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

// Shared across Google Benchmark worker threads so these cases exercise
// transactions against the same RocksDB instance and transaction manager.
class ConcurrentTransactionBenchmarkContext {
public:
    ConcurrentTransactionBenchmarkContext() {
        const auto unique_id = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        db_dir_ = std::filesystem::temp_directory_path() /
            ("bench_transaction_concurrency_" + std::to_string(unique_id));

        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_dir_, cleanup_error)) {
            throw std::runtime_error(
                "Failed to clear concurrent transaction benchmark DB: " +
                cleanup_error.message());
        }

        RocksDBWrapper::Config config;
        config.db_path = db_dir_.string();
        config.wal_dir = (db_dir_ / "wal").string();
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;
        config.max_write_buffer_number = 3;

        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open concurrent transaction benchmark DB");
        }

        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);
        tx_manager_ = std::make_unique<TransactionManager>(
            *db_, *secondary_index_, *graph_index_, *vector_index_);

        seedEntity("txn_hot_key");
        for (size_t i = 0; i < kUniformKeyCount; ++i) {
            seedEntity("txn_uniform_" + std::to_string(i));
        }
        for (size_t i = 0; i < kWorkerCount; ++i) {
            seedEntity("txn_disjoint_" + std::to_string(i));
            seedEntity("txn_abort_" + std::to_string(i));
        }
    }

    ~ConcurrentTransactionBenchmarkContext() {
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }

        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_dir_, cleanup_error)) {
            std::fprintf(stderr,
                         "Concurrent transaction benchmark DB cleanup failed: %s\n",
                         cleanup_error.message().c_str());
        }
    }

    static constexpr size_t kUniformKeyCount = 64;
    static constexpr size_t kWorkerCount = 32;

    TransactionManager& transactionManager() {
        return *tx_manager_;
    }

private:
    void seedEntity(const std::string& key) {
        const auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            throw std::runtime_error(
                "Failed to begin transaction while seeding concurrent benchmark data");
        }

        BaseEntity entity(key);
        entity.setField("counter", static_cast<int64_t>(0));
        const auto put_status = txn->optimisticPut("txn_concurrency", entity, 0);
        if (!put_status.ok) {
            if (!tx_manager_->rollbackTransaction(txn_id)) {
                throw std::runtime_error(
                    "Failed to roll back concurrent benchmark seed transaction");
            }
            throw std::runtime_error(
                "Failed to seed concurrent transaction benchmark data: " +
                put_status.message);
        }

        const auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            throw std::runtime_error(
                "Failed to commit concurrent transaction benchmark setup data: " +
                commit_status.message);
        }
    }

    std::filesystem::path db_dir_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
    std::unique_ptr<GraphIndexManager> graph_index_;
    std::unique_ptr<VectorIndexManager> vector_index_;
    std::unique_ptr<TransactionManager> tx_manager_;
};

namespace {

ConcurrentTransactionBenchmarkContext& concurrentTransactionContext() {
    static ConcurrentTransactionBenchmarkContext context;
    return context;
}

enum class ConcurrentKeyProfile {
    Uniform,
    HotKey,
    Disjoint
};

void publishWorkerLatencyPercentiles(
    benchmark::State& state,
    const std::array<double, 3>& latency_percentiles_us) {
    state.counters["mean_worker_p50_us"] = benchmark::Counter(
        latency_percentiles_us[0], benchmark::Counter::kAvgThreads);
    state.counters["mean_worker_p95_us"] = benchmark::Counter(
        latency_percentiles_us[1], benchmark::Counter::kAvgThreads);
    state.counters["mean_worker_p99_us"] = benchmark::Counter(
        latency_percentiles_us[2], benchmark::Counter::kAvgThreads);
}

void runConcurrentCommitProfile(benchmark::State& state,
                                ConcurrentKeyProfile profile) {
    auto& tx_manager = concurrentTransactionContext().transactionManager();
    const auto worker_index = static_cast<size_t>(state.thread_index());
    std::mt19937 rng(42u + static_cast<unsigned>(worker_index));
    std::uniform_int_distribution<size_t> uniform_key(
        0, ConcurrentTransactionBenchmarkContext::kUniformKeyCount - 1);
    uint64_t attempted = 0;
    uint64_t committed = 0;
    uint64_t aborted = 0;
    std::vector<double> latency_samples_us;

    for (auto _ : state) {
        ++attempted;
        std::string key;
        switch (profile) {
            case ConcurrentKeyProfile::Uniform:
                key = "txn_uniform_" + std::to_string(uniform_key(rng));
                break;
            case ConcurrentKeyProfile::HotKey:
                key = "txn_hot_key";
                break;
            case ConcurrentKeyProfile::Disjoint:
                key = "txn_disjoint_" + std::to_string(worker_index);
                break;
        }

        const auto start = std::chrono::steady_clock::now();
        const auto txn_id = tx_manager.beginTransaction();
        auto txn = tx_manager.getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Concurrent commit benchmark failed to begin transaction");
            return;
        }

        const auto version = txn->getEntityVersion("txn_concurrency", key);
        const auto entity_json = txn->readEntityJson("txn_concurrency", key);
        if (!version.has_value() || *version == 0 || !entity_json.has_value()) {
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent commit benchmark failed to roll back missing-entity transaction");
                return;
            }
            state.SkipWithError("Concurrent commit benchmark lost a seeded entity");
            return;
        }

        auto entity = BaseEntity::fromJson(key, *entity_json);
        const auto current_counter = entity.getFieldAsInt("counter");
        if (!current_counter.has_value() ||
            *current_counter == std::numeric_limits<int64_t>::max()) {
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent commit benchmark failed to roll back invalid-counter transaction");
                return;
            }
            state.SkipWithError("Concurrent commit benchmark counter is invalid or exhausted");
            return;
        }
        entity.setField("counter", *current_counter + 1);

        const auto put_status =
            txn->optimisticPut("txn_concurrency", entity, *version);
        if (!put_status.ok) {
            const bool is_conflict =
                put_status.message.find("OCC version conflict") != std::string::npos ||
                put_status.message.find("MVCC conflict") != std::string::npos;
            if (!is_conflict) {
                if (!tx_manager.rollbackTransaction(txn_id)) {
                    state.SkipWithError(
                        "Concurrent commit benchmark failed to roll back failed write");
                    return;
                }
                const auto message =
                    "Concurrent commit benchmark optimisticPut failed: " +
                    put_status.message;
                state.SkipWithError(message.c_str());
                return;
            }
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent commit benchmark conflict rollback failed");
                return;
            }
            ++aborted;
            latency_samples_us.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - start).count());
            continue;
        }

        const auto commit_status = tx_manager.commitTransaction(txn_id);
        if (!commit_status.ok) {
            if (commit_status.conflict_set_id.empty()) {
                const auto message =
                    "Concurrent commit benchmark commit failed: " +
                    commit_status.message;
                state.SkipWithError(message.c_str());
                return;
            }
            ++aborted;
            latency_samples_us.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - start).count());
            continue;
        }

        ++committed;
        latency_samples_us.push_back(
            std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - start).count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["tps"] = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, aborted);
    publishWorkerLatencyPercentiles(
        state, nearestRankPercentilesUs(latency_samples_us));
}

void runConcurrentAbortProfile(benchmark::State& state) {
    auto& tx_manager = concurrentTransactionContext().transactionManager();
    const auto worker_index = static_cast<size_t>(state.thread_index());
    const std::string key = "txn_abort_" + std::to_string(worker_index);
    uint64_t attempted = 0;
    uint64_t aborted = 0;
    std::vector<double> latency_samples_us;

    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        const auto txn_id = tx_manager.beginTransaction();
        auto txn = tx_manager.getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Concurrent abort benchmark failed to begin transaction");
            return;
        }

        const auto version = txn->getEntityVersion("txn_concurrency", key);
        const auto entity_json = txn->readEntityJson("txn_concurrency", key);
        if (!version.has_value() || *version == 0 || !entity_json.has_value()) {
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent abort benchmark failed to roll back missing-entity transaction");
                return;
            }
            state.SkipWithError("Concurrent abort benchmark lost a seeded entity");
            return;
        }

        auto entity = BaseEntity::fromJson(key, *entity_json);
        const auto current_counter = entity.getFieldAsInt("counter");
        if (!current_counter.has_value() ||
            *current_counter == std::numeric_limits<int64_t>::max()) {
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent abort benchmark failed to roll back invalid-counter transaction");
                return;
            }
            state.SkipWithError("Concurrent abort benchmark counter is invalid or exhausted");
            return;
        }
        entity.setField("counter", *current_counter + 1);
        const auto put_status =
            txn->optimisticPut("txn_concurrency", entity, *version);
        if (!put_status.ok) {
            if (!tx_manager.rollbackTransaction(txn_id)) {
                state.SkipWithError(
                    "Concurrent abort benchmark failed to roll back failed write");
                return;
            }
            const auto message =
                "Concurrent abort benchmark optimisticPut failed: " +
                put_status.message;
            state.SkipWithError(message.c_str());
            return;
        }

        if (!tx_manager.rollbackTransaction(txn_id)) {
            state.SkipWithError("Concurrent abort benchmark rollback failed");
            return;
        }
        ++aborted;
        latency_samples_us.push_back(
            std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - start).count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(aborted));
    state.counters["aborts_per_second"] = benchmark::Counter(
        static_cast<double>(aborted), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, 0, aborted);
    publishWorkerLatencyPercentiles(
        state, nearestRankPercentilesUs(latency_samples_us));
}

class LockWaitRun {
public:
    LockWaitRun(LockManager& lock_manager, size_t worker_count, int64_t iterations)
        : lock_manager_(lock_manager),
          worker_count_(worker_count),
          iterations_(iterations),
          holder_barrier_(static_cast<std::ptrdiff_t>(worker_count + 1)),
          release_barrier_(static_cast<std::ptrdiff_t>(worker_count + 1)),
          holder_thread_([this] { holdAndReleasePerIteration(); }) {}

    ~LockWaitRun() {
        if (holder_thread_.joinable()) {
            holder_thread_.join();
        }
    }

    void joinHolder() {
        if (holder_thread_.joinable()) {
            holder_thread_.join();
        }
    }

    size_t workerCount() const {
        return worker_count_;
    }

    void synchronizeBeforeAcquire() {
        holder_barrier_.arrive_and_wait();
    }

    void synchronizeAfterAcquire() {
        release_barrier_.arrive_and_wait();
    }

    bool failed() const {
        return failed_.load(std::memory_order_relaxed);
    }

private:
    void holdAndReleasePerIteration() {
        constexpr auto kLockTimeout = std::chrono::seconds(10);
        constexpr auto kWaitForQueue = std::chrono::seconds(2);
        constexpr LockManager::TransactionId kHolderId =
            std::numeric_limits<LockManager::TransactionId>::max();
        const std::string key = "txn_benchmark_blocking_lock";

        for (int64_t iteration = 0; iteration < iterations_; ++iteration) {
            const auto result = lock_manager_.acquireLock(
                kHolderId, key, LockType::EXCLUSIVE, kLockTimeout);
            if (result.status != LockStatus::GRANTED) {
                failed_ = true;
            }

            holder_barrier_.arrive_and_wait();
            if (result.status == LockStatus::GRANTED) {
                const auto deadline = std::chrono::steady_clock::now() + kWaitForQueue;
                while (lock_manager_.getStats().current_waiting < worker_count_ &&
                       std::chrono::steady_clock::now() < deadline) {
                    std::this_thread::yield();
                }
                if (lock_manager_.getStats().current_waiting < worker_count_) {
                    failed_ = true;
                }
                if (!lock_manager_.releaseLock(kHolderId, key)) {
                    failed_ = true;
                }
            }

            release_barrier_.arrive_and_wait();
        }
    }

    LockManager& lock_manager_;
    size_t worker_count_;
    int64_t iterations_;
    std::barrier<> holder_barrier_;
    std::barrier<> release_barrier_;
    std::atomic<bool> failed_{false};
    std::thread holder_thread_;
};

struct LockWaitRunRegistry {
    std::mutex mutex;
    std::condition_variable ready;
    std::shared_ptr<LockWaitRun> current;
};

LockWaitRunRegistry& lockWaitRunRegistry() {
    static LockWaitRunRegistry registry;
    return registry;
}

std::shared_ptr<LockWaitRun> getLockWaitRun(benchmark::State& state) {
    auto& registry = lockWaitRunRegistry();
    std::unique_lock lock(registry.mutex);
    if (state.thread_index() == 0) {
        registry.current = std::make_shared<LockWaitRun>(
            concurrentTransactionContext().transactionManager().getLockManager(),
            static_cast<size_t>(state.threads()),
            state.max_iterations);
        registry.ready.notify_all();
    } else {
        registry.ready.wait(lock, [&registry, &state] {
            return registry.current &&
                   static_cast<size_t>(state.threads()) ==
                       registry.current->workerCount();
        });
    }
    return registry.current;
}

void clearLockWaitRun(const std::shared_ptr<LockWaitRun>& run) {
    auto& registry = lockWaitRunRegistry();
    std::lock_guard lock(registry.mutex);
    if (registry.current == run) {
        registry.current.reset();
    }
}

}  // namespace

static void BM_TransactionConcurrentCommitUniform(benchmark::State& state) {
    runConcurrentCommitProfile(state, ConcurrentKeyProfile::Uniform);
}

static void BM_TransactionConcurrentCommitHotKey(benchmark::State& state) {
    runConcurrentCommitProfile(state, ConcurrentKeyProfile::HotKey);
}

static void BM_TransactionConcurrentCommitDisjoint(benchmark::State& state) {
    runConcurrentCommitProfile(state, ConcurrentKeyProfile::Disjoint);
}

BENCHMARK(BM_TransactionConcurrentCommitUniform)
    ->Threads(1)->Threads(8)->Threads(32)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TransactionConcurrentCommitHotKey)
    ->Threads(1)->Threads(8)->Threads(32)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TransactionConcurrentCommitDisjoint)
    ->Threads(1)->Threads(8)->Threads(32)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

static void BM_TransactionConcurrentAbortDisjoint(benchmark::State& state) {
    runConcurrentAbortProfile(state);
}

BENCHMARK(BM_TransactionConcurrentAbortDisjoint)
    ->Threads(1)->Threads(8)->Threads(32)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

static void BM_TransactionConcurrentLockWait(benchmark::State& state) {
    auto run = getLockWaitRun(state);
    const auto worker_index = static_cast<size_t>(state.thread_index());
    constexpr auto kLockTimeout = std::chrono::seconds(10);
    constexpr LockManager::TransactionId kWorkerIdBase =
        std::numeric_limits<LockManager::TransactionId>::max() - 64;
    const std::string key = "txn_benchmark_blocking_lock";
    auto& lock_manager =
        concurrentTransactionContext().transactionManager().getLockManager();
    uint64_t attempted = 0;
    uint64_t acquired = 0;
    uint64_t timeouts = 0;
    uint64_t denied = 0;
    bool worker_failed = false;
    std::vector<double> latency_samples_us;

    for (auto _ : state) {
        run->synchronizeBeforeAcquire();
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        const auto result = lock_manager.acquireLock(
            kWorkerIdBase + worker_index, key, LockType::EXCLUSIVE, kLockTimeout);
        latency_samples_us.push_back(
            std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - start).count());
        if (result.status == LockStatus::GRANTED) {
            ++acquired;
            if (!lock_manager.releaseLock(kWorkerIdBase + worker_index, key)) {
                worker_failed = true;
            }
        } else if (result.status == LockStatus::TIMEOUT) {
            ++timeouts;
        } else {
            ++denied;
            worker_failed = true;
        }
        run->synchronizeAfterAcquire();
    }

    if (worker_failed) {
        state.SkipWithError(
            "Concurrent lock-wait benchmark failed to complete a lock attempt");
    }
    if (worker_index == 0) {
        run->joinHolder();
        if (run->failed()) {
            state.SkipWithError(
                "Concurrent lock-wait benchmark could not establish every blocking waiter");
        }
        clearLockWaitRun(run);
    }

    state.SetItemsProcessed(static_cast<int64_t>(acquired));
    state.counters["lock_attempted"] = static_cast<double>(attempted);
    state.counters["lock_acquired"] = static_cast<double>(acquired);
    state.counters["lock_timeouts"] = static_cast<double>(timeouts);
    state.counters["lock_denied"] = static_cast<double>(denied);
    state.counters["lock_blocked"] =
        run->failed() ? 0.0 : static_cast<double>(attempted);
    state.counters["lock_acquisitions_per_second"] = benchmark::Counter(
        static_cast<double>(acquired), benchmark::Counter::kIsRate);
    publishWorkerLatencyPercentiles(
        state, nearestRankPercentilesUs(latency_samples_us));
}

BENCHMARK(BM_TransactionConcurrentLockWait)
    ->Threads(1)->Threads(8)->Threads(32)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Read-Only Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, ReadOnlyTransaction)(benchmark::State& state) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 999);
    uint64_t attempted = 0;
    uint64_t committed = 0;
    
    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start read transaction");
            return;
        }
        
        for (int i = 0; i < 10; i++) {
            const auto entity_json = txn->readEntityJson(
                "users", "user_" + std::to_string(dist(rng)));
            if (!entity_json.has_value()) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Read transaction could not find a pre-populated user");
                return;
            }
            benchmark::DoNotOptimize(*entity_json);
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }
    
    auto stats = tx_manager_->getStats();
    state.SetItemsProcessed(state.iterations() * 10); // 10 reads per transaction
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
    state.counters["total_committed"] = stats.total_committed;
    state.counters["total_aborted"] = stats.total_aborted;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, ReadOnlyTransaction)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Write-Only Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, WriteOnlyTransaction)(benchmark::State& state) {
    size_t counter = 0;
    std::mt19937 rng(99);
    std::uniform_real_distribution<double> balance_dist(0.0, 100000.0);
    uint64_t attempted = 0;
    uint64_t committed = 0;
    std::vector<BaseEntity> entities;
    entities.reserve(10);
    for (auto _ : state) {
        state.PauseTiming();
        entities.clear();
        for (int i = 0; i < 10; ++i) {
            std::string key = "new_user_" + std::to_string(counter++);
            BaseEntity entity(key);
            entity.setField("name", std::string("NewUser_") + std::to_string(counter));
            entity.setField("balance", balance_dist(rng));
            entity.setField("created_at",
                            std::chrono::system_clock::now().time_since_epoch().count());
            entities.push_back(std::move(entity));
        }
        state.ResumeTiming();

        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start write transaction");
            return;
        }
        
        for (const auto& entity : entities) {
            auto put_status = txn->putEntity("users", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Put entity failed");
                return;
            }
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }
    
    state.SetItemsProcessed(state.iterations()); // One processed item represents one committed transaction.
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, WriteOnlyTransaction)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Mixed Read/Write Transactions
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, MixedTransaction)(benchmark::State& state) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> user_dist(0, 999);
    std::uniform_real_distribution<double> amount_dist(-1000.0, 1000.0);
    uint64_t attempted = 0;
    uint64_t committed = 0;
    
    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start mixed transaction");
            return;
        }
        
        for (int i = 0; i < 5; i++) {
            std::string key = "user_" + std::to_string(user_dist(rng));
            const auto entity_json = txn->readEntityJson("users", key);
            if (!entity_json.has_value()) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Mixed transaction could not find a pre-populated user");
                return;
            }

            auto entity = BaseEntity::fromJson(key, *entity_json);
            const auto balance = entity.getFieldAsDouble("balance");
            if (!balance.has_value()) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Mixed transaction user has no numeric balance");
                return;
            }

            entity.setField("balance", std::max(0.0, *balance + amount_dist(rng)));
            const auto put_status = txn->putEntity("users", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Mixed transaction update failed");
                return;
            }
        }
        
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }
    
    state.SetItemsProcessed(state.iterations() * 5); // 5 RMW operations per transaction
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, MixedTransaction)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Commit Latency Distribution
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, CommitLatency)(benchmark::State& state) {
    const int ops_per_txn = state.range(0);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> value_dist(0.0, 1000.0);
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    std::vector<double> commit_latencies_us;
    
    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start commit-latency transaction");
            return;
        }
        
        // Perform operations
        for (int i = 0; i < ops_per_txn; i++) {
            BaseEntity entity("temp_" + std::to_string(counter++));
            entity.setField("value", value_dist(rng));
            const auto put_status = txn->putEntity("temp", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Commit-latency transaction write failed");
                return;
            }
        }
        
        // Measure commit time
        const auto start = std::chrono::steady_clock::now();
        auto commit_status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        
        commit_latencies_us.push_back(
            std::chrono::duration<double, std::micro>(end - start).count());
        ++committed;
    }
    
    if (commit_latencies_us.empty()) {
        state.SkipWithError("Commit-latency benchmark produced no commit samples");
        return;
    }

    std::sort(commit_latencies_us.begin(), commit_latencies_us.end());
    const auto percentile_us = [&commit_latencies_us](double percentile) {
        const auto rank = static_cast<size_t>(
            std::ceil(percentile * static_cast<double>(commit_latencies_us.size())));
        return commit_latencies_us[std::max<size_t>(1, rank) - 1];
    };
    state.SetItemsProcessed(state.iterations());
    publishOutcomeCounters(state, attempted, committed, 0);
    state.counters["ops_per_txn"] = ops_per_txn;
    state.counters["p50_real_time"] = percentile_us(0.50);
    state.counters["p99_real_time"] = percentile_us(0.99);
    state.counters["commit_samples"] =
        static_cast<double>(commit_latencies_us.size());
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, CommitLatency)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Abort Performance
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, AbortTransaction)(benchmark::State& state) {
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t aborted = 0;
    
    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start abort transaction");
            return;
        }
        
        // Perform some operations
        for (int i = 0; i < 10; i++) {
            BaseEntity entity("abort_test_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            const auto put_status = txn->putEntity("temp", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Abort transaction write failed");
                return;
            }
        }
        
        // Rollback statt Commit (abort API entfernt)
        if (!tx_manager_->rollbackTransaction(txn_id)) {
            state.SkipWithError("Abort transaction rollback failed");
            return;
        }
        ++aborted;
    }
    
    auto stats = tx_manager_->getStats();
    state.SetItemsProcessed(state.iterations());
    state.counters["aborts_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, 0, aborted);
    state.counters["total_aborted"] = stats.total_aborted;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, AbortTransaction)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Savepoint Create and Rollback
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointCreateAndRollback)(benchmark::State& state) {
    const int writes_before = state.range(0);
    const int writes_after  = state.range(1);
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start savepoint transaction");
            return;
        }

        // Writes committed to the transaction before the savepoint
        for (int i = 0; i < writes_before; ++i) {
            BaseEntity entity("sp_before_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            const auto put_status = txn->putEntity("bench", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Savepoint pre-write failed");
                return;
            }
        }

        // Create a named savepoint
        const auto savepoint_status = txn->createSavepoint("bench_sp");
        if (!savepoint_status.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("Savepoint creation failed");
            return;
        }

        // Writes that will be rolled back
        for (int i = 0; i < writes_after; ++i) {
            BaseEntity entity("sp_after_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(counter));
            const auto put_status = txn->putEntity("bench", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Savepoint post-write failed");
                return;
            }
        }

        // Partial rollback to the savepoint (discards the writes_after entities)
        const auto rollback_status = txn->rollbackToSavepoint("bench_sp");
        if (!rollback_status.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("Rollback to savepoint failed");
            return;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }

    // Count both kept and rolled-back writes to reflect total work performed
    // (including savepoint overhead and rollback cost).
    state.SetItemsProcessed(state.iterations() * (writes_before + writes_after));
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointCreateAndRollback)
    ->Args({0, 5})    // 0 writes before, 5 rolled back
    ->Args({5, 5})    // 5 writes kept,   5 rolled back
    ->Args({5, 20})   // 5 writes kept,  20 rolled back
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Nested Savepoints with Rollback
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointNested)(benchmark::State& state) {
    const int depth = state.range(0); // number of nested savepoints
    size_t counter  = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start nested-savepoint transaction");
            return;
        }

        // Create `depth` nested savepoints, each with one write
        for (int d = 0; d < depth; ++d) {
            BaseEntity entity("sp_nest_" + std::to_string(counter++));
            entity.setField("depth", static_cast<int64_t>(d));
            const auto put_status = txn->putEntity("bench", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Nested-savepoint write failed");
                return;
            }
            const auto savepoint_status = txn->createSavepoint("sp_" + std::to_string(d));
            if (!savepoint_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Nested savepoint creation failed");
                return;
            }
        }

        // Write after the last savepoint (will be rolled back)
        BaseEntity last("sp_last_" + std::to_string(counter++));
        last.setField("depth", static_cast<int64_t>(depth));
        const auto last_write_status = txn->putEntity("bench", last);
        if (!last_write_status.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("Post-savepoint write failed");
            return;
        }

        // Rollback to the first savepoint: discards writes made after "sp_0" was
        // created (depth-1 writes + the final write).  The write at d=0 is kept
        // because it was committed to the transaction before "sp_0" was set.
        const auto rollback_status = txn->rollbackToSavepoint("sp_0");
        if (!rollback_status.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("Nested rollback to savepoint failed");
            return;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }

    state.SetItemsProcessed(state.iterations() * (depth + 1));
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
    state.counters["savepoint_depth"] = depth;
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointNested)
    ->Arg(2)
    ->Arg(5)
    ->Arg(10)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Savepoint Release (no rollback)
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointRelease)(benchmark::State& state) {
    const int num_savepoints = state.range(0);
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start savepoint-release transaction");
            return;
        }

        // Create several savepoints and release them without rolling back
        for (int i = 0; i < num_savepoints; ++i) {
            BaseEntity entity("sp_rel_" + std::to_string(counter++));
            entity.setField("value", static_cast<int64_t>(i));
            const auto put_status = txn->putEntity("bench", entity);
            if (!put_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Savepoint-release write failed");
                return;
            }
            const auto savepoint_status = txn->createSavepoint("sp_" + std::to_string(i));
            if (!savepoint_status.ok) {
                tx_manager_->rollbackTransaction(txn_id);
                state.SkipWithError("Savepoint release setup failed");
                return;
            }
        }

        // Release the first savepoint (also releases all newer ones)
        const auto release_status = txn->releaseSavepoint("sp_0");
        if (!release_status.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("Savepoint release failed");
            return;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }

    state.SetItemsProcessed(state.iterations() * num_savepoints);
    state.counters["tps"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, SavepointRelease)
    ->Arg(1)
    ->Arg(5)
    ->Arg(10)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Optimistic Concurrency Control (OCC) – new entity creation
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticPut)(benchmark::State& state) {
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start optimistic-put transaction");
            return;
        }

        // Create a new entity with expected_version=0 (OCC insert)
        std::string pk = "occ_new_" + std::to_string(counter++);
        BaseEntity entity(pk);
        entity.setField("value", static_cast<int64_t>(counter));

        auto st = txn->optimisticPut("occ_bench", entity, 0);
        if (!st.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("optimisticPut failed");
            return;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }

    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["tps"] = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
    state.counters["attempts"] = static_cast<double>(attempted);
    state.counters["success_rate_percent"] = attempted == 0 ? 0.0 :
        100.0 * static_cast<double>(committed) / static_cast<double>(attempted);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccOptimisticPut)
    ->Threads(1)
    ->UseRealTime()
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
        if (!setup_txn) {
            state.SkipWithError("OccReadVersionAndUpdate setup: failed to start transaction");
            return;
        }
        BaseEntity e("occ_rmw_" + std::to_string(i));
        e.setField("counter", static_cast<int64_t>(0));
        auto setup_st = setup_txn->optimisticPut("occ_rmw", e, 0);
        if (!setup_st.ok) {
            if (!tx_manager_->rollbackTransaction(setup_id)) {
                state.SkipWithError("OccReadVersionAndUpdate setup: rollback failed");
                return;
            }
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
    uint64_t attempted = 0;
    uint64_t conflicts = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        ++attempted;
        std::string pk = "occ_rmw_" + std::to_string(key_dist(rng));

        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("OccReadVersionAndUpdate: failed to start transaction");
            return;
        }

        auto ver = txn->getEntityVersion("occ_rmw", pk);
        const auto entity_json = txn->readEntityJson("occ_rmw", pk);
        if (!ver.has_value() || *ver == 0 || !entity_json.has_value()) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("OccReadVersionAndUpdate: seeded entity or version is missing");
            return;
        }

        auto entity = BaseEntity::fromJson(pk, *entity_json);
        const auto current_counter = entity.getFieldAsInt("counter");
        if (!current_counter.has_value() ||
            *current_counter == std::numeric_limits<int64_t>::max()) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("OccReadVersionAndUpdate: invalid or exhausted counter");
            return;
        }
        entity.setField("counter", *current_counter + 1);

        auto st = txn->optimisticPut("occ_rmw", entity, *ver);
        if (!st.ok) {
            if (st.message.find("OCC version conflict") == std::string::npos) {
                tx_manager_->rollbackTransaction(txn_id);
                const auto message = "OccReadVersionAndUpdate: optimisticPut failed: " +
                                     st.message;
                state.SkipWithError(message.c_str());
                return;
            }
            if (!tx_manager_->rollbackTransaction(txn_id)) {
                state.SkipWithError("OccReadVersionAndUpdate: conflict rollback failed");
                return;
            }
            ++conflicts;
            continue;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            if (commit_status.conflict_set_id.empty()) {
                const auto message = "OccReadVersionAndUpdate: commit failed: " +
                                     commit_status.message;
                state.SkipWithError(message.c_str());
                return;
            }
            ++conflicts;
            continue;
        }
        ++committed;
    }

    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["tps"] = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    state.counters["conflicts"] = benchmark::Counter(static_cast<double>(conflicts));
    publishOutcomeCounters(state, attempted, committed, conflicts);
    state.counters["attempts"] = static_cast<double>(attempted);
    state.counters["false_conflict_rate_percent"] =
        attempted == 0 ? 0.0 : 100.0 * static_cast<double>(conflicts) /
                                   static_cast<double>(attempted);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccReadVersionAndUpdate)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: OCC erase (optimisticErase)
// ============================================================================

BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticErase)(benchmark::State& state) {
    size_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;

    for (auto _ : state) {
        state.PauseTiming();
        // Create entity to be erased
        std::string pk = "occ_erase_" + std::to_string(counter++);
        {
            auto ins_id = tx_manager_->beginTransaction();
            auto ins_txn = tx_manager_->getTransaction(ins_id);
            if (!ins_txn) {
                state.ResumeTiming();
                state.SkipWithError("Failed to start optimistic-erase setup transaction");
                return;
            }
            BaseEntity e(pk);
            e.setField("v", static_cast<int64_t>(1));
            const auto insert_status = ins_txn->optimisticPut("occ_erase", e, 0);
            if (!insert_status.ok) {
                tx_manager_->rollbackTransaction(ins_id);
                state.ResumeTiming();
                state.SkipWithError("Optimistic-erase setup insert failed");
                return;
            }
            const auto insert_commit_status = tx_manager_->commitTransaction(ins_id);
            if (!insert_commit_status.ok) {
                state.ResumeTiming();
                state.SkipWithError("Optimistic-erase setup commit failed");
                return;
            }
        }
        state.ResumeTiming();

        ++attempted;
        auto txn_id = tx_manager_->beginTransaction();
        auto txn    = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start optimistic-erase transaction");
            return;
        }

        auto st = txn->optimisticErase("occ_erase", pk, 1);
        if (!st.ok) {
            tx_manager_->rollbackTransaction(txn_id);
            state.SkipWithError("optimisticErase failed");
            return;
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            state.SkipWithError("Transaction commit failed");
            return;
        }
        ++committed;
    }

    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["tps"] = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK_REGISTER_F(TransactionBenchmarkFixture, OccOptimisticErase)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Single-thread hot-key overwrite
// ============================================================================

static void BM_TransactionHotKeyOverwrite(benchmark::State& state) {
    // Setup
    const auto unique_id = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::ostringstream suffix = {};
    suffix << "bench_transaction_hot_key_tmp_t"
           << state.thread_index()
           << "_" << unique_id;
    const auto db_dir = std::filesystem::absolute(std::filesystem::path("data") / suffix.str());
    std::string test_db_path = db_dir.string();
    std::error_code cleanup_error;
    if (!themis::bench::removeBenchmarkDirectory(db_dir, cleanup_error)) {
        throw std::runtime_error("Failed to clear hot-key overwrite benchmark DB: " + cleanup_error.message());
    }
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.wal_dir = (db_dir / "wal").string();
    config.memtable_size_mb = 128;
    config.block_cache_size_mb = 256;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        themis::bench::removeBenchmarkDirectory(db_dir, cleanup_error);
        throw std::runtime_error("Failed to open hot-key overwrite benchmark database");
    }
    
    auto secondary_index = std::make_unique<SecondaryIndexManager>(*db);
    auto graph_index = std::make_unique<GraphIndexManager>(*db);
    auto vector_index = std::make_unique<VectorIndexManager>(*db);
    auto tx_manager = std::make_unique<TransactionManager>(
        *db, *secondary_index, *graph_index, *vector_index);
    
    const std::string hot_key = "contended_resource";
    uint64_t counter = 0;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        auto txn_id = tx_manager->beginTransaction();
        auto txn = tx_manager->getTransaction(txn_id);
        if (!txn) {
            state.SkipWithError("Failed to start hot-key overwrite transaction");
            break;
        }

        BaseEntity entity(hot_key);
        entity.setField("counter", static_cast<int64_t>(++counter));
        const auto put_status = txn->putEntity("resources", entity);
        if (!put_status.ok) {
            if (!tx_manager->rollbackTransaction(txn_id)) {
                state.SkipWithError("Hot-key overwrite write failed and rollback failed");
                break;
            }
            state.SkipWithError("Hot-key overwrite write failed");
            break;
        }

        auto commit_status = tx_manager->commitTransaction(txn_id);
        if (!commit_status.ok) {
            const auto message = "Hot-key overwrite commit failed: " +
                                 commit_status.message;
            state.SkipWithError(message.c_str());
            break;
        }
        ++committed;
    }
    
    // Cleanup
    tx_manager.reset();
    vector_index.reset();
    graph_index.reset();
    secondary_index.reset();
    db->close();
    db.reset();
    
    if (!themis::bench::removeBenchmarkDirectory(test_db_path, cleanup_error)) {
        std::fprintf(stderr, "Transaction contention benchmark DB cleanup failed: %s\n",
                     cleanup_error.message().c_str());
    }
    
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["tps"] = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    publishOutcomeCounters(state, attempted, committed, 0);
}

BENCHMARK(BM_TransactionHotKeyOverwrite)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// TX-4: Two-Phase Commit Latency (5 Shards) — direct SLO benchmark
//
// Measures end-to-end 2PC commit latency across N in-process participants,
// including coordinator and participant WALs, fencing, PREPARE fan-out, and
// COMMIT broadcast. Null storage callbacks exclude application storage work.
//
// SLO (TX-4): ≤ 5 ms per commit across 5 shards.
// ============================================================================

/// @brief Fixture for two-phase commit latency benchmarks (TX-4).
///
/// Creates one TwoPhaseCommitCoordinator and @p num_shards in-process
/// TwoPhaseCommitParticipant instances with unique WALs and scoped fencing.
class TwoPhaseCommitFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const auto num_shards = static_cast<int>(state.range(0));
        const auto unique_id = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        wal_dir_ = std::filesystem::absolute(
            std::filesystem::path("data") /
            ("bench_2pc_wal_t" + std::to_string(state.thread_index()) + "_" +
             std::to_string(unique_id)));
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(wal_dir_, cleanup_error)) {
            throw std::runtime_error(
                "Failed to clear 2PC benchmark WAL: " + cleanup_error.message());
        }

        // Build durable participants first (null callbacks → no storage application).
        for (int i = 0; i < num_shards; ++i) {
            const auto shard_id = "bench_shard_" + std::to_string(i);
            themis::sharding::TwoPhaseCommitParticipant::Config participant_cfg;
            participant_cfg.wal_directory =
                (wal_dir_ / ("participant_" + std::to_string(i))).string();
            themis::sharding::EpochFencingConfig fencing_cfg;
            fencing_cfg.shard_id = shard_id;
            fencing_cfg.node_id = "participant-" + shard_id;
            fencing_cfg.auto_stonith = false;
            participant_cfg.fencing_manager =
                std::make_shared<themis::sharding::EpochFencingManager>(
                    std::move(fencing_cfg));
            participants_.emplace(
                shard_id,
                std::make_unique<themis::sharding::TwoPhaseCommitParticipant>(
                    shard_id, participant_cfg)
            );
        }

        // 2PC writes fail closed without coordinator WAL and current shard tokens.
        const std::string coordinator_id = "bench_coordinator";
        themis::sharding::TwoPhaseCommitCoordinator::Config cfg;
        cfg.wal_directory   = (wal_dir_ / "coordinator").string();
        cfg.sync_wal_writes = false;
        cfg.fencing_token_provider = [coordinator_id](
            const std::string& shard_id)
            -> std::optional<themis::sharding::EpochToken> {
            themis::sharding::EpochToken token;
            token.epoch = 1;
            token.issuer = coordinator_id;
            token.shard_id = shard_id;
            token.issued_at = std::chrono::system_clock::now();
            return token;
        };
        coordinator_ = std::make_unique<themis::sharding::TwoPhaseCommitCoordinator>(
            coordinator_id, cfg
        );
        for (auto& [id, p] : participants_) {
            coordinator_->registerParticipant(id, p.get());
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        coordinator_.reset();
        participants_.clear();
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(wal_dir_, cleanup_error)) {
            std::fprintf(stderr, "2PC benchmark WAL cleanup failed: %s\n",
                         cleanup_error.message().c_str());
        }
    }

private:
    std::filesystem::path wal_dir_;
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
/// @note  SLO TX-4: ≤ 5 ms per commit for 5 durable in-process participants.
BENCHMARK_DEFINE_F(TwoPhaseCommitFixture, TwoPhaseCommitLatency)(benchmark::State& state) {
    const auto num_shards = static_cast<int>(state.range(0));
    uint64_t   txn_seq    = 0;
    uint64_t   aborts     = 0;
    uint64_t   committed  = 0;
    uint64_t   attempted  = 0;

    // Shared per-shard payload: a single write operation per shard per commit.
    nlohmann::json shard_ops = nlohmann::json::array();
    shard_ops.push_back("SET bench_key bench_value");

    for (auto _ : state) {
        ++attempted;
        std::map<std::string, nlohmann::json> ops_per_shard = {};

        for (int i = 0; i < num_shards; ++i) {
            ops_per_shard["bench_shard_" + std::to_string(i)] = shard_ops;
        }

        const auto outcome = coordinator().commit(
            "bench_2pc_" + std::to_string(txn_seq++), ops_per_shard
        );
        if (outcome.result == themis::sharding::CoordinatorTxnResult::COMMITTED) {
            ++committed;
        } else if (outcome.result == themis::sharding::CoordinatorTxnResult::ABORTED) {
            ++aborts;
        } else {
            const auto message = "2PC coordinator returned FAILURE: " + outcome.reason;
            state.SkipWithError(message.c_str());
            return;
        }
    }

    publishOutcomeCounters(state, attempted, committed, aborts);
    state.counters["tps"]    = benchmark::Counter(
        static_cast<double>(committed), benchmark::Counter::kIsRate);
    state.counters["aborts"] = benchmark::Counter(static_cast<double>(aborts));
    state.counters["shards"] = static_cast<double>(num_shards);
}

// Arg(5) = TX-4 SLO target (5 shards, ≤ 5 ms); Arg(3) sanity baseline.
BENCHMARK_REGISTER_F(TwoPhaseCommitFixture, TwoPhaseCommitLatency)
    ->Arg(3)
    ->Arg(5)
    ->UseRealTime()
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
// SLO (TX-6): predictor call latency ≤ 1 us.
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
/// @note  SLO TX-6: direct call latency ≤ 1 us.
BENCHMARK_DEFINE_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_Predict)(
    benchmark::State& state)
{
    constexpr int kKeySpace = 20;
    std::mt19937  rng(99);
    std::uniform_int_distribution<int> key_dist(0, kKeySpace - 1);
    constexpr size_t kPreparedQueryCount = 256;
    std::vector<std::vector<std::string>> proposed_sets;
    proposed_sets.reserve(kPreparedQueryCount);
    for (size_t sample = 0; sample < kPreparedQueryCount; ++sample) {
        std::vector<std::string> proposed;
        proposed.reserve(5);
        for (int key = 0; key < 5; ++key) {
            proposed.push_back("lock_key_" + std::to_string(key_dist(rng)));
        }
        proposed_sets.push_back(std::move(proposed));
    }

    const std::set<DeadlockPredictor::TransactionId> active_txns = {1u, 2u, 3u};
    size_t query_index = 0;

    for (auto _ : state) {
        const auto& proposed = proposed_sets[query_index++ % proposed_sets.size()];
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
/// @note  SLO TX-6: direct call latency ≤ 1 us.
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

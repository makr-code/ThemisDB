#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <memory>
#include <queue>
#include <string>

namespace themis::process::benchmark {

// ============================================================================
// Constants and Types
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kLargeRevisions = 10;

/**
 * @brief Simulated process model revision record
 */
struct ProcessModelRevision {
    std::string id;
    int revision_num{0};
    std::string content_hash;
    int64_t timestamp_ms{0};
    std::string author;
};

/**
 * @brief Simulated conflict record
 */
struct ProcessConflict {
    std::string model_id;
    int conflict_type{0};  // 0=concurrent_edit, 1=circular_ref, 2=invalid_state
    std::vector<ProcessModelRevision> conflicting_revisions;
    int64_t detected_ms{0};
};

/**
 * @brief Simulated conflict resolution engine
 */
class ConflictResolver {
private:
    std::vector<ProcessModelRevision> revision_history_;
    std::vector<ProcessConflict> resolved_conflicts_;

public:
    ConflictResolver() = default;

    /**
     * @brief Simulate conflict detection and resolution
     */
    bool resolveConflict(const ProcessConflict& conflict) {
        auto start = std::chrono::high_resolution_clock::now();

        // Simulate conflict analysis
        std::vector<int> scores(conflict.conflicting_revisions.size(), 0);
        for (size_t i = 0; i < conflict.conflicting_revisions.size(); ++i) {
            // Score based on timestamp (most recent wins by default)
            scores[i] = static_cast<int>(conflict.conflicting_revisions[i].timestamp_ms);
        }

        // Find winner
        int winner_idx = 0;
        int max_score = scores[0];
        for (size_t i = 1; i < scores.size(); ++i) {
            if (scores[i] > max_score) {
                max_score = scores[i];
                winner_idx = i;
            }
        }

        // Record resolution
        resolved_conflicts_.push_back(conflict);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        benchmark::DoNotOptimize(duration);

        return true;
    }

    /**
     * @brief Get resolution statistics
     */
    size_t resolvedCount() const { return resolved_conflicts_.size(); }
};

/**
 * @brief Simulated revision store with rollback capability
 */
class RevisionStore {
private:
    std::vector<ProcessModelRevision> revisions_;

public:
    RevisionStore() = default;

    /**
     * @brief Add a new revision
     */
    void addRevision(const ProcessModelRevision& rev) {
        revisions_.push_back(rev);
    }

    /**
     * @brief Get all revisions for a model
     */
    std::vector<ProcessModelRevision> getRevisions(const std::string& model_id) const {
        std::vector<ProcessModelRevision> result = {};

        for (const auto& rev : revisions_) {
            if (rev.id == model_id) {
                result.push_back(rev);
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const ProcessModelRevision& a, const ProcessModelRevision& b) {
                      return a.revision_num < b.revision_num;
                  });
        return result;
    }

    /**
     * @brief Simulate single-model rollback
     */
    bool rollbackToRevision(const std::string& model_id, int target_revision) {
        auto revs = getRevisions(model_id);
        if (revs.empty() || target_revision < 1 || target_revision > static_cast<int>(revs.size())) {
            return false;
        }

        // Simulate removing revisions after target
        for (auto it = revisions_.begin(); it != revisions_.end();) {
            if (it->id == model_id && it->revision_num > target_revision) {
                it = revisions_.erase(it);
            } else {
                ++it;
            }
        }

        return true;
    }

    /**
     * @brief Simulate batch rollback
     */
    int batchRollback(const std::vector<std::string>& model_ids, int target_revision) {
        int rolled_back = 0;
        for (const auto& model_id : model_ids) {
            if (rollbackToRevision(model_id, target_revision)) {
                rolled_back++;
            }
        }
        return rolled_back;
    }

    size_t totalRevisions() const { return revisions_.size(); }
};

/**
 * @brief Simulate transaction for serialization testing
 */
struct Transaction {
    std::string txn_id;
    std::vector<ProcessModelRevision> operations;
    int64_t start_ms{0};
    int64_t end_ms{0};
};

/**
 * @brief Transaction serializer for deterministic ordering
 */
class TransactionSerializer {
private:
    std::vector<Transaction> committed_txns_;

public:
    TransactionSerializer() = default;

    /**
     * @brief Serialize and commit a transaction
     */
    bool commitTransaction(const Transaction& txn) {
        auto start = std::chrono::high_resolution_clock::now();

        // Simulate serialization (ordering verification, WAL write, etc.)
        std::vector<uint8_t> serialized;
        serialized.reserve(1000);

        for (const auto& op : txn.operations) {
            // Simulate serialization of each operation
            std::string op_str = op.id + "|" + std::to_string(op.revision_num);
            serialized.insert(serialized.end(), op_str.begin(), op_str.end());
        }

        // Verify ordering is deterministic
        if (!serialized.empty()) {
            committed_txns_.push_back(txn);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        benchmark::DoNotOptimize(duration);

        return true;
    }

    size_t committedCount() const { return committed_txns_.size(); }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate synthetic conflicts
 */
static std::vector<ProcessConflict> generateConflicts(int count) {
    std::vector<ProcessConflict> conflicts;
    std::mt19937 gen(kCanonicalRngSeed);
    std::uniform_int_distribution<> type_dist(0, 2);

    for (int i = 0; i < count; ++i) {
        ProcessConflict conflict;
        conflict.model_id = "model_" + std::to_string(i % 10);
        conflict.conflict_type = type_dist(gen);
        conflict.detected_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

        // Generate conflicting revisions
        ProcessModelRevision rev1, rev2;
        rev1.id = conflict.model_id;
        rev1.revision_num = i;
        rev1.content_hash = "hash_" + std::to_string(i);
        rev1.timestamp_ms = conflict.detected_ms - 100;

        rev2.id = conflict.model_id;
        rev2.revision_num = i + 1;
        rev2.content_hash = "hash_" + std::to_string(i + 1);
        rev2.timestamp_ms = conflict.detected_ms - 50;

        conflict.conflicting_revisions = {rev1, rev2};
        conflicts.push_back(conflict);
    }

    return conflicts;
}

/**
 * @brief Generate revision history
 */
static std::vector<ProcessModelRevision> generateRevisionHistory(
    const std::string& model_id, int num_revisions) {
    std::vector<ProcessModelRevision> revisions;
    int64_t start_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    for (int i = 1; i <= num_revisions; ++i) {
        ProcessModelRevision rev;
        rev.id = model_id;
        rev.revision_num = i;
        rev.content_hash = "hash_v" + std::to_string(i);
        rev.timestamp_ms = start_ms + (i * 1000);
        rev.author = (i % 2 == 0) ? "alice" : "bob";
        revisions.push_back(rev);
    }

    return revisions;
}

// ============================================================================
// DP-01: Conflict Resolution (100 conflicts)
// ============================================================================

static void BM_DP01_ConflictResolution(benchmark::State& state) {
    const int num_conflicts = kSmallDatasetSize;
    auto resolver = std::make_unique<ConflictResolver>();
    auto conflicts = generateConflicts(num_conflicts);

    std::vector<double> latencies;
    latencies.reserve(num_conflicts);

    for (auto _ : state) {
        state.PauseTiming();
        resolver = std::make_unique<ConflictResolver>();
        latencies.clear();
        state.ResumeTiming();

        for (const auto& conflict : conflicts) {
            auto start = std::chrono::high_resolution_clock::now();
            resolver->resolveConflict(conflict);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));
        }
    }

    state.SetItemsProcessed(num_conflicts * static_cast<int64_t>(state.iterations()));
    state.counters["p99_ms"] = benchmark::Counter(
        latencies.empty() ? 0.0 : *std::max_element(latencies.begin(), latencies.end()),
        benchmark::Counter::kAvgIterations);
}

BENCHMARK(BM_DP01_ConflictResolution)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// DP-02: Rollback Single (10 revisions)
// ============================================================================

static void BM_DP02_RollbackSingle(benchmark::State& state) {
    const std::string model_id = "test_model";
    const int num_revisions = kLargeRevisions;
    auto store = std::make_unique<RevisionStore>();

    for (auto _ : state) {
        state.PauseTiming();
        store = std::make_unique<RevisionStore>();
        auto revisions = generateRevisionHistory(model_id, num_revisions);
        for (const auto& rev : revisions) {
            store->addRevision(rev);
        }
        state.ResumeTiming();

        // Rollback to revision 5 (out of 10)
        auto start = std::chrono::high_resolution_clock::now();
        store->rollbackToRevision(model_id, 5);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        state.counters["rollback_ms"] = benchmark::Counter(
            static_cast<double>(duration_ms), benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_revisions * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_DP02_RollbackSingle)
    ->Iterations(20)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// DP-03: Rollback Batch (100 models)
// ============================================================================

static void BM_DP03_RollbackBatch(benchmark::State& state) {
    const int num_models = kSmallDatasetSize;
    const int revisions_per_model = 5;
    auto store = std::make_unique<RevisionStore>();

    for (auto _ : state) {
        state.PauseTiming();
        store = std::make_unique<RevisionStore>();

        std::vector<std::string> model_ids = {};

        for (int i = 0; i < num_models; ++i) {
            std::string model_id = "model_" + std::to_string(i);
            model_ids.push_back(model_id);
            auto revisions = generateRevisionHistory(model_id, revisions_per_model);
            for (const auto& rev : revisions) {
                store->addRevision(rev);
            }
        }

        state.ResumeTiming();

        // Batch rollback all models to revision 2
        auto start = std::chrono::high_resolution_clock::now();
        int rolled_back = store->batchRollback(model_ids, 2);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        benchmark::DoNotOptimize(rolled_back);
        state.counters["rollback_ms"] = benchmark::Counter(
            static_cast<double>(duration_ms), benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_models * revisions_per_model * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_DP03_RollbackBatch)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// DP-04: Transaction Serialization
// ============================================================================

static void BM_DP04_TransactionSerialization(benchmark::State& state) {
    const int num_transactions = 100;
    auto serializer = std::make_unique<TransactionSerializer>();

    for (auto _ : state) {
        state.PauseTiming();
        serializer = std::make_unique<TransactionSerializer>();

        // Generate synthetic transactions
        std::vector<Transaction> transactions = {};

        for (int i = 0; i < num_transactions; ++i) {
            Transaction txn;
            txn.txn_id = "txn_" + std::to_string(i);
            txn.start_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

            // Each transaction has 3-5 operations
            int num_ops = 3 + (i % 3);
            for (int j = 0; j < num_ops; ++j) {
                ProcessModelRevision op;
                op.id = "model_" + std::to_string(i % 10);
                op.revision_num = j + 1;
                op.content_hash = "hash_" + std::to_string(i) + "_" + std::to_string(j);
                op.timestamp_ms = txn.start_ms + j;
                txn.operations.push_back(op);
            }

            transactions.push_back(txn);
        }

        state.ResumeTiming();

        // Commit all transactions and measure serialization latency
        std::vector<double> latencies = {};

        for (const auto& txn : transactions) {
            auto start = std::chrono::high_resolution_clock::now();
            serializer->commitTransaction(txn);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));
        }

        if (!latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());
            double p99 = latencies[std::min(size_t(99), latencies.size() - 1)];
            state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
        }
    }

    state.SetItemsProcessed(num_transactions * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_DP04_TransactionSerialization)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// DP-05: Deterministic Output Verification
// ============================================================================

static void BM_DP05_DeterministicOutputVerification(benchmark::State& state) {
    const int num_models = 100;
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> content_dist(0, 9999);

    std::vector<std::string> model_contents = {};

    for (int i = 0; i < num_models; ++i) {
        std::string content = "model_" + std::to_string(i) + "_content_" + 
                             std::to_string(content_dist(rng));
        model_contents.push_back(content);
    }

    for (auto _ : state) {
        state.PauseTiming();
        std::string first_output = {};
        state.ResumeTiming();

        // First pass: generate output deterministically
        for (const auto& content : model_contents) {
            auto hash = std::hash<std::string>{}(content);
            first_output += std::to_string(hash) + ",";
        }

        state.PauseTiming();
        std::string second_output = {};
        state.ResumeTiming();

        // Second pass: verify output is identical
        for (const auto& content : model_contents) {
            auto hash = std::hash<std::string>{}(content);
            second_output += std::to_string(hash) + ",";
        }

        // Verify outputs match
        benchmark::DoNotOptimize(first_output == second_output);
    }

    state.SetItemsProcessed(num_models * 2 * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_DP05_DeterministicOutputVerification)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// DP-06: Version Clock Operations
// ============================================================================

struct VersionClock {
    uint64_t logical_clock{0};
    int64_t wall_clock_ms{0};
    std::string node_id = {};

    void increment() {
        logical_clock++;
        wall_clock_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    }

    bool isAfter(const VersionClock& other) const {
        if (logical_clock != other.logical_clock) {
            return logical_clock > other.logical_clock;
        }
        return wall_clock_ms > other.wall_clock_ms;
    }
};

static void BM_DP06_VersionClockOperations(benchmark::State& state) {
    const int num_clocks = 1000;
    std::vector<VersionClock> clocks;
    
    for (int i = 0; i < num_clocks; ++i) {
        VersionClock vc;
        vc.node_id = "node_" + std::to_string(i);
        vc.logical_clock = i;
        clocks.push_back(vc);
    }

    for (auto _ : state) {
        state.PauseTiming();
        auto test_clock = clocks[0];
        state.ResumeTiming();

        // Perform clock comparisons
        for (const auto& clock : clocks) {
            benchmark::DoNotOptimize(test_clock.isAfter(clock));
        }

        // Perform clock increments
        for (auto& clock : clocks) {
            clock.increment();
        }
    }

    state.SetItemsProcessed(num_clocks * 2 * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_DP06_VersionClockOperations)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();

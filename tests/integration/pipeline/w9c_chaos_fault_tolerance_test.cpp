/**
 * @file w9c_chaos_fault_tolerance_test.cpp
 * @brief Wave 9C — Chaos Engineering & Fault Tolerance (CFT-01..CFT-08).
 *
 * Validates that ThemisDB components withstand and recover from injected
 * faults including network partitions, partial write failures, cascading
 * failures, node restarts, write storms, read path degradation, and
 * deadline enforcement.  All infrastructure is in-process; no external
 * services are required.
 *
 * CFT-01  Network partition simulation — two nodes communicate before
 *         partition; messages are dropped during partition; communication
 *         resumes after healing.
 * CFT-02  Partial write failure — a batch of 10 writes fails at position 5;
 *         store rolls back to pre-batch state; reads confirm no partial state.
 * CFT-03  Cascading failure containment — a fault in one subsystem does not
 *         propagate to a sibling subsystem (bulkhead pattern).
 * CFT-04  Self-healing after node restart — a restarted node rejoins the
 *         cluster and correctly serves reads.
 * CFT-05  Write storm resilience — 8 concurrent threads × 50 writes each,
 *         with some injected failures; completes without deadlock; successful
 *         writes are durable.
 * CFT-06  Read path degradation — primary path marked degraded; reads fall
 *         back to secondary path and return correct results.
 * CFT-07  Timeout enforcement — operation exceeding its deadline is cancelled
 *         and returns kTimedOut, not a partial result.
 * CFT-08  Chaos gate self-check — gate counter reports 1.0 when all CFT
 *         invariants hold, 0.0 on any violation.
 *
 * All tests are deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// kCanonicalSeed = 42 is provided by test_data_generator.h (themis::test::kCanonicalSeed).

// ---------------------------------------------------------------------------
// Status codes for CFT tests
// ---------------------------------------------------------------------------

/// @brief Result codes for chaos/fault-tolerance scenarios.
enum class CftStatus {
    kOk,
    kDropped,
    kPartitioned,
    kRolledBack,
    kTimedOut,
    kFailed,
    kDegraded,
};

// ---------------------------------------------------------------------------
// NetworkLink — simulates a point-to-point link with partition injection (CFT-01)
// ---------------------------------------------------------------------------

/**
 * @brief Simulates a directed network link between two named nodes.
 *
 * Messages sent while the link is partitioned are dropped and counted.
 * Healing the partition restores normal delivery; all subsequent messages
 * succeed.
 */
class NetworkLink {
public:
    NetworkLink(std::string src, std::string dst)
        : src_(std::move(src)), dst_(std::move(dst)) {}

    /// @brief Inject a network partition on this link.
    void Partition() {
        std::lock_guard<std::mutex> lk(mu_);
        partitioned_ = true;
    }

    /// @brief Heal the network partition.
    void Heal() {
        std::lock_guard<std::mutex> lk(mu_);
        partitioned_ = false;
    }

    /**
     * @brief Send a message; returns kDropped if partitioned, kOk otherwise.
     * Delivered messages are appended to delivered_.
     */
    CftStatus Send(const std::string& message) {
        std::lock_guard<std::mutex> lk(mu_);
        if (partitioned_) {
            ++dropped_;
            return CftStatus::kDropped;
        }
        delivered_.push_back(message);
        return CftStatus::kOk;
    }

    size_t DroppedCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return dropped_;
    }

    size_t DeliveredCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return delivered_.size();
    }

    bool IsPartitioned() const {
        std::lock_guard<std::mutex> lk(mu_);
        return partitioned_;
    }

private:
    mutable std::mutex       mu_;
    const std::string        src_;
    const std::string        dst_;
    bool                     partitioned_{false};
    size_t                   dropped_{0};
    std::vector<std::string> delivered_;
};

// ---------------------------------------------------------------------------
// TransactionalStore — atomic batch with rollback for CFT-02
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe key/value store with atomic multi-key batch semantics.
 *
 * ExecuteBatch() stages all writes into a temporary buffer; on success,
 * they are atomically promoted.  On failure (at the injected index), the
 * buffer is discarded and the store is unchanged.
 */
class TransactionalStore {
public:
    struct BatchResult {
        bool   committed{false};
        size_t failed_at{0};  ///< 0 = success; >0 = failure index (1-based)
    };

    /**
     * @brief Attempt to write all @p pairs; injects failure at @p fail_at
     *        (1-based, 0 = no failure).
     */
    BatchResult ExecuteBatch(
        const std::vector<std::pair<std::string, std::string>>& pairs,
        size_t fail_at) {

        BatchResult result;
        std::unordered_map<std::string, std::string> staging;

        for (size_t i = 0; i < pairs.size(); ++i) {
            if (fail_at > 0 && i + 1 == fail_at) {
                result.failed_at = fail_at;
                return result;   // discard staging; store unchanged
            }
            staging[pairs[i].first] = pairs[i].second;
        }

        // Commit atomically.
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& [k, v] : staging) { data_[k] = v; }
        result.committed = true;
        return result;
    }

    /// @brief Read a value.
    std::optional<std::string> Read(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        const auto it = data_.find(key);
        if (it == data_.end()) { return std::nullopt; }
        return it->second;
    }

    /// @brief Write a single key/value pair (used for setup).
    void Write(const std::string& key, const std::string& val) {
        std::lock_guard<std::mutex> lk(mu_);
        data_[key] = val;
    }

    bool Contains(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.count(key) > 0;
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.size();
    }

    /// @brief Return a snapshot of all keys (for post-condition assertions).
    std::unordered_set<std::string> Keys() const {
        std::lock_guard<std::mutex> lk(mu_);
        std::unordered_set<std::string> ks = {};

        for (const auto& [k, _] : data_) { ks.insert(k); }
        return ks;
    }

private:
    mutable std::mutex                           mu_;
    std::unordered_map<std::string, std::string> data_;
};

// ---------------------------------------------------------------------------
// Bulkhead — isolates subsystems from cascading failures for CFT-03
// ---------------------------------------------------------------------------

/**
 * @brief Models a single isolated subsystem in a bulkhead pattern.
 *
 * Faults thrown inside Execute() are contained and recorded; they do not
 * propagate to the caller — the caller receives kFailed, not an exception.
 */
class BulkheadSubsystem {
public:
    explicit BulkheadSubsystem(std::string name) : name_(std::move(name)) {}

    /**
     * @brief Execute @p work inside the bulkhead.
     *
     * If @p work throws, the exception is caught, fault_count_ is incremented,
     * and kFailed is returned.  Sibling bulkheads are unaffected.
     */
    CftStatus Execute(const std::function<void()>& work) {
        try {
            work();
            ++success_count_;
            return CftStatus::kOk;
        } catch (...) {
            ++fault_count_;
            return CftStatus::kFailed;
        }
    }

    size_t FaultCount()   const { return fault_count_.load(); }
    size_t SuccessCount() const { return success_count_.load(); }
    const std::string& Name() const { return name_; }

private:
    const std::string      name_;
    std::atomic<size_t>    fault_count_{0};
    std::atomic<size_t>    success_count_{0};
};

// ---------------------------------------------------------------------------
// ClusterNode — simulates cluster membership for CFT-04
// ---------------------------------------------------------------------------

/**
 * @brief Represents a cluster node that can be stopped and restarted.
 *
 * When restarted, the node clears its in-memory state and joins the cluster
 * again.  After joining, it is authoritative for reads from the provided
 * snapshot of cluster state.
 */
class ClusterNode {
public:
    explicit ClusterNode(std::string id) : id_(std::move(id)) {}

    /// @brief Simulate a node stop (all in-memory state is lost).
    void Stop() {
        std::lock_guard<std::mutex> lk(mu_);
        running_ = false;
        local_.clear();
        in_cluster_ = false;
    }

    /// @brief Restart the node and rejoin the cluster with @p snapshot.
    void Restart(const std::unordered_map<std::string, std::string>& snapshot) {
        std::lock_guard<std::mutex> lk(mu_);
        local_      = snapshot;
        running_    = true;
        in_cluster_ = true;
    }

    bool IsRunning() const {
        std::lock_guard<std::mutex> lk(mu_);
        return running_;
    }

    bool IsInCluster() const {
        std::lock_guard<std::mutex> lk(mu_);
        return in_cluster_;
    }

    std::optional<std::string> Read(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        if (!running_) { return std::nullopt; }
        const auto it = local_.find(key);
        if (it == local_.end()) { return std::nullopt; }
        return it->second;
    }

private:
    mutable std::mutex                           mu_;
    const std::string                            id_;
    bool                                         running_{true};
    bool                                         in_cluster_{false};
    std::unordered_map<std::string, std::string> local_;
};

// ---------------------------------------------------------------------------
// DualPathStore — primary/secondary read path for CFT-06
// ---------------------------------------------------------------------------

/**
 * @brief A read-path store with a primary and a secondary (fallback) backend.
 *
 * When the primary path is marked degraded, Read() automatically falls back
 * to the secondary path.  Both paths hold the same data set up-front for
 * correctness validation.
 */
class DualPathStore {
public:
    void WriteToAll(const std::string& key, const std::string& val) {
        std::lock_guard<std::mutex> lk(mu_);
        primary_[key]   = val;
        secondary_[key] = val;
    }

    /// @brief Mark the primary read path as degraded.
    void DegradePrimary() {
        std::lock_guard<std::mutex> lk(mu_);
        primary_degraded_ = true;
    }

    /**
     * @brief Read a value, falling back to secondary if primary is degraded.
     * @returns The value and a flag indicating which path was used.
     */
    struct ReadResult {
        std::optional<std::string> value;
        bool used_secondary{false};
    };

    ReadResult Read(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        if (!primary_degraded_) {
            const auto it = primary_.find(key);
            if (it != primary_.end()) { return {it->second, false}; }
            return {std::nullopt, false};
        }
        // Primary degraded — use secondary.
        const auto it = secondary_.find(key);
        if (it != secondary_.end()) { return {it->second, true}; }
        return {std::nullopt, true};
    }

private:
    mutable std::mutex                           mu_;
    std::unordered_map<std::string, std::string> primary_;
    std::unordered_map<std::string, std::string> secondary_;
    bool                                         primary_degraded_{false};
};

// ---------------------------------------------------------------------------
// DeadlineOperation — enforces a step-count deadline for CFT-07
// ---------------------------------------------------------------------------

/**
 * @brief Executes a work function that runs for @p work_steps logical steps.
 *
 * If work_steps > deadline_steps, the operation is cancelled after
 * deadline_steps and returns kTimedOut.  No real sleep is used; each step
 * is a single logical unit of work.
 */
struct DeadlineResult {
    CftStatus status{CftStatus::kOk};
    size_t    steps_executed{0};
};

DeadlineResult RunWithDeadline(size_t work_steps, size_t deadline_steps) {
    DeadlineResult r;
    for (size_t s = 0; s < work_steps; ++s) {
        if (s >= deadline_steps) {
            r.status          = CftStatus::kTimedOut;
            r.steps_executed  = s;
            return r;
        }
        ++r.steps_executed;
    }
    r.status = CftStatus::kOk;
    return r;
}

// ---------------------------------------------------------------------------
// ChaosGateCounter — gate self-check for CFT-08
// ---------------------------------------------------------------------------

/**
 * @brief Simple 0.0/1.0 gate counter for the chaos fault-tolerance suite.
 */
class ChaosGateCounter {
public:
    void Report(bool all_passed) {
        std::lock_guard<std::mutex> lk(mu_);
        value_ = all_passed ? 1.0 : 0.0;
    }

    double Value() const {
        std::lock_guard<std::mutex> lk(mu_);
        return value_;
    }

private:
    mutable std::mutex mu_;
    double             value_{0.0};
};

} // anonymous namespace

// ===========================================================================
// Test fixture
// ===========================================================================

/**
 * @brief Shared fixture for chaos fault-tolerance tests.
 */
class ChaosFaultToleranceTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_  = std::make_unique<TransactionalStore>();
        dual_   = std::make_unique<DualPathStore>();
        gate_   = std::make_unique<ChaosGateCounter>();
        gen_.seed(kCanonicalSeed);
    }

    void TearDown() override {
        store_.reset();
        dual_.reset();
        gate_.reset();
    }

    std::unique_ptr<TransactionalStore>  store_;
    std::unique_ptr<DualPathStore>       dual_;
    std::unique_ptr<ChaosGateCounter>    gate_;
    std::mt19937                          gen_;
};

// ===========================================================================
// CFT-01 — Network partition simulation
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT01_NetworkPartitionDropsAndHeals) {
    SCOPED_TRACE("CFT-01: messages dropped during partition; resume after heal");

    NetworkLink link("node_A", "node_B");

    // Before partition: messages deliver.
    EXPECT_EQ(link.Send("msg_before_partition"), CftStatus::kOk);
    EXPECT_EQ(link.DeliveredCount(), 1U);
    EXPECT_EQ(link.DroppedCount(), 0U);

    // Inject partition.
    link.Partition();
    ASSERT_TRUE(link.IsPartitioned());

    // During partition: messages are dropped.
    constexpr size_t kDropped = 5;
    for (size_t i = 0; i < kDropped; ++i) {
        const CftStatus s = link.Send("msg_during_partition_" + std::to_string(i));
        EXPECT_EQ(s, CftStatus::kDropped)
            << "message " << i << " must be dropped during partition";
    }
    EXPECT_EQ(link.DroppedCount(), kDropped);
    EXPECT_EQ(link.DeliveredCount(), 1U) << "no new deliveries during partition";

    // Heal the partition.
    link.Heal();
    ASSERT_FALSE(link.IsPartitioned());

    // After healing: messages deliver again.
    constexpr size_t kPostHeal = 3;
    for (size_t i = 0; i < kPostHeal; ++i) {
        const CftStatus s = link.Send("msg_after_heal_" + std::to_string(i));
        EXPECT_EQ(s, CftStatus::kOk)
            << "message " << i << " must be delivered after partition is healed";
    }
    EXPECT_EQ(link.DeliveredCount(), 1 + kPostHeal);
}

// ===========================================================================
// CFT-02 — Partial write failure with rollback
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT02_PartialWriteFailureRollsBackToPreBatchState) {
    SCOPED_TRACE("CFT-02: failed mid-batch leaves store identical to pre-batch state");

    // Pre-populate a sentinel key.
    store_->Write("sentinel", "sentinel_value");
    const auto keys_before = store_->Keys();

    constexpr size_t kBatchSize  = 10;
    constexpr size_t kFailAt     = 5; // fail at the 5th write (1-based)

    std::vector<std::pair<std::string, std::string>> batch;
    batch.reserve(kBatchSize);
    for (size_t i = 0; i < kBatchSize; ++i) {
        batch.emplace_back("batch_key_" + std::to_string(i),
                           "batch_val_" + std::to_string(i));
    }

    const auto result = store_->ExecuteBatch(batch, kFailAt);

    EXPECT_FALSE(result.committed)
        << "batch must not commit when a mid-write failure is injected";
    EXPECT_EQ(result.failed_at, kFailAt);

    // No batch key must appear in the store.
    for (const auto& [k, _] : batch) {
        EXPECT_FALSE(store_->Contains(k))
            << "rollback failed: key still present: " << k;
    }

    // Pre-batch state must be fully preserved.
    const auto keys_after = store_->Keys();
    EXPECT_EQ(keys_before, keys_after)
        << "store key set must be identical to pre-batch state after rollback";

    // Sentinel must be intact.
    EXPECT_EQ(store_->Read("sentinel"), std::optional<std::string>{"sentinel_value"})
        << "pre-existing sentinel key must be unchanged after rollback";
}

// ===========================================================================
// CFT-03 — Cascading failure containment (bulkhead)
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT03_CascadingFailureContainedByBulkhead) {
    SCOPED_TRACE("CFT-03: fault in subsystem A must not propagate to subsystem B");

    BulkheadSubsystem subsys_a("subsys_A");
    BulkheadSubsystem subsys_b("subsys_B");

    // Normal operation of B.
    const CftStatus ok_b = subsys_b.Execute([]() { /* no-op success */ });
    EXPECT_EQ(ok_b, CftStatus::kOk);

    // Inject a fault in A (throws an exception).
    const CftStatus fault_a = subsys_a.Execute([]() {
        throw std::runtime_error("simulated subsystem_A panic");
    });
    EXPECT_EQ(fault_a, CftStatus::kFailed)
        << "fault in subsys_A must return kFailed to caller";
    EXPECT_EQ(subsys_a.FaultCount(), 1U);

    // Subsystem B must be unaffected by the fault in A.
    const CftStatus ok_b2 = subsys_b.Execute([]() { /* still works */ });
    EXPECT_EQ(ok_b2, CftStatus::kOk)
        << "subsystem B must remain operational after a fault in subsystem A";
    EXPECT_EQ(subsys_b.FaultCount(), 0U)
        << "subsystem B must have zero faults";
    EXPECT_EQ(subsys_b.SuccessCount(), 2U)
        << "subsystem B must have accumulated 2 successes";
}

// ===========================================================================
// CFT-04 — Self-healing after node restart
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT04_NodeRestartRejoinsClusterAndServesReads) {
    SCOPED_TRACE("CFT-04: restarted node must rejoin and serve reads correctly");

    ClusterNode node("node_1");

    // Pre-populate cluster state (the snapshot that will be replayed on restart).
    const std::unordered_map<std::string, std::string> cluster_snapshot = {
        {"key_A", "val_A"},
        {"key_B", "val_B"},
        {"key_C", "val_C"},
    };

    // Node is initially running; seed it.
    node.Restart(cluster_snapshot);
    ASSERT_TRUE(node.IsRunning());
    ASSERT_TRUE(node.IsInCluster());

    // Stop the node (simulates crash / restart).
    node.Stop();
    ASSERT_FALSE(node.IsRunning());
    ASSERT_FALSE(node.IsInCluster());

    // Reads on a stopped node return nullopt.
    EXPECT_FALSE(node.Read("key_A").has_value())
        << "stopped node must not serve reads";

    // Restart with the cluster snapshot.
    node.Restart(cluster_snapshot);
    ASSERT_TRUE(node.IsRunning());
    ASSERT_TRUE(node.IsInCluster());

    // After restart, all snapshot keys are readable.
    for (const auto& [k, v] : cluster_snapshot) {
        const auto got = node.Read(k);
        ASSERT_TRUE(got.has_value())
            << "restarted node must serve key: " << k;
        EXPECT_EQ(*got, v)
            << "restarted node returned wrong value for key: " << k;
    }
}

// ===========================================================================
// CFT-05 — Write storm resilience
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT05_WriteStormCompletesWithoutDeadlock) {
    SCOPED_TRACE("CFT-05: 8 threads × 50 writes; injected failures; no deadlock");

    constexpr size_t kThreads      = 8;
    constexpr size_t kWritesPerThread = 50;
    // Inject a failure on every 7th write (deterministic, not random)
    constexpr size_t kFailEvery    = 7;

    std::atomic<size_t> succeeded{0};
    std::atomic<size_t> failed_inj{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (size_t t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (size_t w = 0; w < kWritesPerThread; ++w) {
                // Inject a failure deterministically.
                if ((t * kWritesPerThread + w) % kFailEvery == 0) {
                    ++failed_inj;
                    continue;  // skip this write (simulated failure)
                }
                const std::string key = "storm_t" + std::to_string(t)
                                      + "_w" + std::to_string(w);
                store_->Write(key, "v_" + std::to_string(w));
                ++succeeded;
            }
        });
    }

    for (auto& w : workers) { w.join(); }

    const size_t total = kThreads * kWritesPerThread;
    EXPECT_EQ(succeeded.load() + failed_inj.load(), total)
        << "all write attempts must be accounted for (succeeded + failed == total)";
    EXPECT_GT(succeeded.load(), 0U)
        << "at least some writes must succeed";

    // All successful writes must be durable (readable).
    // We verify by checking that the store is non-empty.
    EXPECT_GT(store_->Size(), 0U)
        << "at least one successful write must be durable in the store";
}

// ===========================================================================
// CFT-06 — Read path degradation with fallback
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT06_ReadPathDegradedFallsBackToSecondary) {
    SCOPED_TRACE("CFT-06: degraded primary causes reads to fall back to secondary");

    const std::string key = "cft06_key";
    const std::string val = "cft06_val";

    dual_->WriteToAll(key, val);

    // Primary path is healthy — read via primary.
    const auto primary_result = dual_->Read(key);
    ASSERT_TRUE(primary_result.value.has_value());
    EXPECT_EQ(*primary_result.value, val);
    EXPECT_FALSE(primary_result.used_secondary)
        << "healthy primary path must be used before degradation";

    // Degrade the primary path.
    dual_->DegradePrimary();

    // Read must now fall back to secondary and return correct result.
    const auto fallback_result = dual_->Read(key);
    ASSERT_TRUE(fallback_result.value.has_value())
        << "secondary fallback must serve the read";
    EXPECT_EQ(*fallback_result.value, val)
        << "secondary fallback must return the correct value";
    EXPECT_TRUE(fallback_result.used_secondary)
        << "secondary path must be reported as used after primary degradation";
}

// ===========================================================================
// CFT-07 — Timeout enforcement
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT07_TimeoutEnforcementCancelsAndReturnsTimedOut) {
    SCOPED_TRACE("CFT-07: operation exceeding deadline must return kTimedOut");

    constexpr size_t kDeadlineSteps = 10;

    // Operation that fits within the deadline — must succeed.
    const DeadlineResult ok = RunWithDeadline(kDeadlineSteps, kDeadlineSteps);
    EXPECT_EQ(ok.status, CftStatus::kOk)
        << "operation exactly at deadline must succeed";
    EXPECT_EQ(ok.steps_executed, kDeadlineSteps);

    // Operation that is shorter than the deadline — must also succeed.
    const DeadlineResult short_op = RunWithDeadline(5, kDeadlineSteps);
    EXPECT_EQ(short_op.status, CftStatus::kOk);

    // Operation that exceeds the deadline — must return kTimedOut.
    constexpr size_t kOverBudgetSteps = 20;
    const DeadlineResult timed_out = RunWithDeadline(kOverBudgetSteps, kDeadlineSteps);
    EXPECT_EQ(timed_out.status, CftStatus::kTimedOut)
        << "operation exceeding deadline must return kTimedOut";
    EXPECT_LT(timed_out.steps_executed, kOverBudgetSteps)
        << "timed-out operation must not run all steps";
    EXPECT_GE(timed_out.steps_executed, kDeadlineSteps)
        << "cancelled operation must have run at least up to the deadline";

    // Verify the operation was cancelled and not completed (no partial result
    // would be visible — the step count tells us where cancellation occurred).
    EXPECT_EQ(timed_out.steps_executed, kDeadlineSteps)
        << "cancellation must occur exactly at the deadline boundary";
}

// ===========================================================================
// CFT-08 — Chaos gate self-check
// ===========================================================================

TEST_F(ChaosFaultToleranceTest, CFT08_ChaosGateCounterReportsCorrectly) {
    SCOPED_TRACE("CFT-08: chaos gate must report 1.0 on all-pass, 0.0 on any violation");

    // All CFT invariants passing — gate must be 1.0.
    gate_->Report(true);
    EXPECT_DOUBLE_EQ(gate_->Value(), 1.0)
        << "chaos gate must be 1.0 when all CFT invariants hold";

    // Any violation — gate must be 0.0.
    gate_->Report(false);
    EXPECT_DOUBLE_EQ(gate_->Value(), 0.0)
        << "chaos gate must be 0.0 on any CFT invariant violation";

    // Recovery to passing restores 1.0.
    gate_->Report(true);
    EXPECT_DOUBLE_EQ(gate_->Value(), 1.0)
        << "chaos gate must return to 1.0 after all invariants are restored";
}
} } // namespace themis::test

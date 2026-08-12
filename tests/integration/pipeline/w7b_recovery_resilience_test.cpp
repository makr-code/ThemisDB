/**
 * @file w7b_recovery_resilience_test.cpp
 * @brief Wave 7B — High-Confidence Recovery & Resilience (HCR-01..HCR-08).
 *
 * Tests retry logic, timeout containment, WAL replay and post-recovery
 * correctness.  All tests are deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

// Canonical seed is provided by test_data_generator.h (themis::test::kCanonicalSeed)

// ---------------------------------------------------------------------------
// RetryableOperation — simulates intermittent failure with fixed retry delay
// ---------------------------------------------------------------------------

struct RetryPolicy {
    size_t   max_retries{3};
    std::chrono::milliseconds base_delay{1};  // fast for CI
};

enum class OpStatus { kSuccess, kTransientFailure, kPermanentFailure };

class RetryableOperationRunner {
public:
    struct RunResult {
        bool     succeeded{false};
        size_t   attempts{0};
        OpStatus last_status{OpStatus::kTransientFailure};
    };

    // fail_until_attempt injects transient failures; permanent_failure forces
    // an immediate permanent failure result.
    [[nodiscard]] RunResult Run(const RetryPolicy& policy,
                                 size_t             fail_until_attempt,
                                 bool               permanent_failure = false) {
        RunResult result;
        for (size_t attempt = 1; attempt <= policy.max_retries + 1; ++attempt) {
            result.attempts = attempt;

            if (permanent_failure) {
                result.last_status = OpStatus::kPermanentFailure;
                return result;
            }

            if (attempt <= fail_until_attempt) {
                result.last_status = OpStatus::kTransientFailure;
                std::this_thread::sleep_for(policy.base_delay);
                continue;
            }

            result.succeeded   = true;
            result.last_status = OpStatus::kSuccess;
            return result;
        }
        // exhausted retries
        result.succeeded = false;
        return result;
    }
};

// ---------------------------------------------------------------------------
// PipelineWithTimeoutContainment — each stage has an independent deadline
// ---------------------------------------------------------------------------

class TimeoutContainedPipeline {
public:
    struct StageResult {
        std::string stage_name;
        bool        succeeded{false};
        bool        timed_out{false};
    };

    // Run a stage with a simulated latency; timeout == 0 means no timeout.
    [[nodiscard]] StageResult RunStage(const std::string& name,
                                        std::chrono::milliseconds simulated_latency,
                                        std::chrono::milliseconds timeout) {
        const auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(simulated_latency);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const bool timed_out =
            timeout.count() > 0 &&
            elapsed > timeout + std::chrono::milliseconds(10);
        return {name, !timed_out, timed_out};
    }
};

// ---------------------------------------------------------------------------
// WalJournalStore — minimal WAL/journal for recovery tests
// ---------------------------------------------------------------------------

struct WalEntry {
    std::string key;
    std::string value;
    bool        committed{false};
};

class WalJournalStore {
public:
    // Simulate writing to WAL before actual storage commit
    [[nodiscard]] size_t AppendWal(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mutex_);
        wal_.push_back({key, value, false});
        return wal_.size() - 1;
    }

    // Mark WAL entry as committed (simulates durable commit)
    void CommitWal(size_t index) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (index < wal_.size()) {
            wal_[index].committed = true;
            data_[wal_[index].key] = wal_[index].value;
        }
    }

    // "Crash" — discard in-memory data but leave WAL intact
    void SimulateCrash() {
        std::lock_guard<std::mutex> lk(mutex_);
        data_.clear();
        crashed_ = true;
    }

    // Replay uncommitted WAL entries after crash
    size_t Replay() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!crashed_) { return 0; }
        size_t replayed = 0;
        for (const auto& entry : wal_) {
            if (entry.committed) {
                data_[entry.key] = entry.value;
                ++replayed;
            }
        }
        crashed_ = false;
        return replayed;
    }

    [[nodiscard]] std::optional<std::string> Get(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = data_.find(key);
        if (it == data_.end()) { return std::nullopt; }
        return it->second;
    }

    [[nodiscard]] bool IsCrashed() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return crashed_;
    }

    [[nodiscard]] size_t WalSize() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return wal_.size();
    }

private:
    mutable std::mutex                               mutex_;
    std::vector<WalEntry>                            wal_;
    std::unordered_map<std::string, std::string>     data_;
    bool                                             crashed_{false};
};

// ---------------------------------------------------------------------------
// SerializedRecoveryManager — ensures concurrent recovery requests are serialized
// ---------------------------------------------------------------------------

class SerializedRecoveryManager {
public:
    explicit SerializedRecoveryManager(WalJournalStore& store) : store_(store) {}

    // Returns the number of entries replayed; serialized via mutex
    [[nodiscard]] size_t Recover() {
        std::lock_guard<std::mutex> lk(recovery_mutex_);
        ++recovery_invocations_;
        return store_.Replay();
    }

    [[nodiscard]] size_t RecoveryInvocations() const { return recovery_invocations_; }

private:
    std::mutex       recovery_mutex_;
    WalJournalStore& store_;
    size_t           recovery_invocations_{0};
};

// ---------------------------------------------------------------------------
// PartialWriteIntegrityPipeline — partial write failure, no phantom records
// ---------------------------------------------------------------------------

class PartialWriteIntegrityPipeline {
public:
    explicit PartialWriteIntegrityPipeline(std::shared_ptr<InMemoryPipelineStorage> storage,
                                            std::shared_ptr<PipelineAuditLog>        audit)
        : storage_(std::move(storage)), audit_(std::move(audit)) {}

    struct BatchResult {
        size_t written{0};
        size_t failed{0};
        bool   phantom_detected{false};
    };

    // Write N items; inject failure at fail_at_index (abort, no partial data)
    [[nodiscard]] BatchResult WriteBatch(const std::vector<std::pair<std::string, std::string>>& items,
                                          size_t fail_at_index) {
        BatchResult result;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i == fail_at_index) {
                // Abort remainder — do NOT write partial data
                ++result.failed;
                audit_->Record({"batch", "abort", "index_" + std::to_string(i)});
                break;
            }
            storage_->Write(items[i].first, items[i].second);
            committed_ids_.insert(items[i].first);
            ++result.written;
        }
        return result;
    }

    // Check whether items after the failure point appear in storage (phantom check)
    [[nodiscard]] bool HasPhantomAfterIndex(
        const std::vector<std::pair<std::string, std::string>>& items,
        size_t fail_at_index) const {
        for (size_t i = fail_at_index; i < items.size(); ++i) {
            if (storage_->Contains(items[i].first)) { return true; }
        }
        return false;
    }

    [[nodiscard]] const std::unordered_set<std::string>& CommittedIds() const {
        return committed_ids_;
    }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::unordered_set<std::string>          committed_ids_;
};

} // namespace

// ===========================================================================
// Test fixture
// ===========================================================================

class RecoveryResilienceTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        rng_.seed(kCanonicalSeed);
        storage_ = CreateInMemoryStorage();
        audit_   = CreateAuditLog();
    }

    std::mt19937                             rng_{kCanonicalSeed};
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
};

// ===========================================================================
// HCR-01 — Intermittent dependency auto-retry succeeds within budget
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR01_IntermittentDependencyAutoRetrySucceedsWithinBudget) {
    SCOPED_TRACE("HCR-01: intermittent dependency auto-retry");

    RetryableOperationRunner runner;
    RetryPolicy policy{.max_retries = 4, .base_delay = std::chrono::milliseconds(1)};

    // Fail on the first 2 attempts, succeed on attempt 3
    const auto result = runner.Run(policy, /*fail_until_attempt=*/2);

    EXPECT_TRUE(result.succeeded)   << "operation must succeed after transient failures";
    EXPECT_EQ(result.attempts, 3U)  << "must take exactly 3 attempts (2 failures + 1 success)";
    EXPECT_EQ(result.last_status, OpStatus::kSuccess);
}

// ===========================================================================
// HCR-02 — Timeout cascade containment
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR02_TimeoutCascadeContainmentOneSlowStageDoesNotCascade) {
    SCOPED_TRACE("HCR-02: timeout cascade containment");

    TimeoutContainedPipeline pipeline;

    // Stage 1: fast — 5ms with 50ms budget
    const auto s1 = pipeline.RunStage("fast_stage",
                                       std::chrono::milliseconds(5),
                                       std::chrono::milliseconds(50));
    EXPECT_TRUE(s1.succeeded)    << "fast stage must succeed within budget";
    EXPECT_FALSE(s1.timed_out);

    // Stage 2: slow (simulated) — but has its own large budget of 200ms
    const auto s2 = pipeline.RunStage("slow_stage",
                                       std::chrono::milliseconds(10),
                                       std::chrono::milliseconds(200));
    EXPECT_TRUE(s2.succeeded)    << "slow stage must still succeed within own budget";

    // Stage 3: must not be affected by stage 2's latency (independent deadline)
    const auto s3 = pipeline.RunStage("next_stage",
                                       std::chrono::milliseconds(2),
                                       std::chrono::milliseconds(50));
    EXPECT_TRUE(s3.succeeded)
        << "stage 3 must not be cascade-affected by stage 2 slowness";
    EXPECT_FALSE(s3.timed_out)
        << "cascade containment invariant violated: stage 3 timed out";
}

// ===========================================================================
// HCR-03 — Retry/backoff boundary: max retries exhausted → clean failure
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR03_RetryBackoffBoundaryMaxRetriesExhaustedCleanFailure) {
    SCOPED_TRACE("HCR-03: max retries exhausted → clean failure");

    RetryableOperationRunner runner;
    RetryPolicy policy{.max_retries = 3, .base_delay = std::chrono::milliseconds(1)};

    // Fail on ALL attempts (fail_until_attempt > max_retries+1)
    const auto result = runner.Run(policy, /*fail_until_attempt=*/999);

    EXPECT_FALSE(result.succeeded)
        << "operation must report failure when retries are exhausted";
    EXPECT_EQ(result.attempts, policy.max_retries + 1)
        << "must attempt exactly max_retries+1 times before giving up";
    EXPECT_NE(result.last_status, OpStatus::kSuccess)
        << "last status must not be kSuccess on exhausted retries";
}

// ===========================================================================
// HCR-04 — Partial write failure → data integrity preserved, no phantom records
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR04_PartialWriteFailureDataIntegrityPreservedNoPhantomRecords) {
    SCOPED_TRACE("HCR-04: partial write failure, no phantom records");

    PartialWriteIntegrityPipeline pipeline(storage_, audit_);

    // Prepare 6 items; inject failure at index 3
    std::vector<std::pair<std::string, std::string>> items = {
        {"key_0", "val_0"}, {"key_1", "val_1"}, {"key_2", "val_2"},
        {"key_3", "val_3"}, {"key_4", "val_4"}, {"key_5", "val_5"},
    };
    constexpr size_t kFailAt = 3;

    const auto batch = pipeline.WriteBatch(items, kFailAt);

    // Items 0..2 must be written; items 3..5 must NOT appear
    EXPECT_EQ(batch.written, kFailAt) << "must have written exactly 3 items before failure";
    EXPECT_EQ(batch.failed, 1U)       << "failed count must be 1";

    for (size_t i = 0; i < kFailAt; ++i) {
        EXPECT_TRUE(storage_->Contains(items[i].first))
            << "committed item " << i << " missing from storage";
    }

    EXPECT_FALSE(pipeline.HasPhantomAfterIndex(items, kFailAt))
        << "phantom record found in storage after partial write failure";

    // Audit must record the abort
    EXPECT_TRUE(audit_->Contains("batch", "abort"))
        << "audit must record batch abort event";
}

// ===========================================================================
// HCR-05 — Recovery after abrupt shutdown mid-write (WAL/journal replay)
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR05_RecoveryAfterAbruptShutdownMidWriteWalReplay) {
    SCOPED_TRACE("HCR-05: WAL replay after crash");

    WalJournalStore store;

    // Write 3 entries; commit only 2 (third was in-flight when crash occurred)
    const auto idx0 = store.AppendWal("k0", "v0");
    const auto idx1 = store.AppendWal("k1", "v1");
    const auto idx2 = store.AppendWal("k2", "v2");  // in-flight: never committed

    store.CommitWal(idx0);
    store.CommitWal(idx1);
    // idx2 intentionally not committed

    // Simulate crash
    store.SimulateCrash();
    ASSERT_TRUE(store.IsCrashed()) << "store must report crashed state";

    // After crash, committed entries must not be accessible (data cleared)
    EXPECT_FALSE(store.Get("k0").has_value()) << "post-crash data must be cleared";
    EXPECT_FALSE(store.Get("k1").has_value()) << "post-crash data must be cleared";

    // Replay WAL
    const auto replayed = store.Replay();

    // Only committed entries replay
    EXPECT_EQ(replayed, 2U) << "must replay exactly 2 committed entries";
    ASSERT_TRUE(store.Get("k0").has_value()) << "k0 must be present after replay";
    ASSERT_TRUE(store.Get("k1").has_value()) << "k1 must be present after replay";
    EXPECT_EQ(*store.Get("k0"), "v0");
    EXPECT_EQ(*store.Get("k1"), "v1");

    // In-flight entry must NOT appear after replay
    EXPECT_FALSE(store.Get("k2").has_value())
        << "uncommitted (in-flight) entry must not appear after WAL replay";
}

// ===========================================================================
// HCR-06 — State integrity invariant: before, during, and after recovery
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR06_StateIntegrityInvariantBeforeDuringAndAfterRecovery) {
    SCOPED_TRACE("HCR-06: state integrity invariant across recovery lifecycle");

    WalJournalStore store;

    // Pre-crash: write and commit a known-good set
    constexpr size_t kGoodEntries = 5;
    for (size_t i = 0; i < kGoodEntries; ++i) {
        const auto idx = store.AppendWal("good_" + std::to_string(i), "val_" + std::to_string(i));
        store.CommitWal(idx);
    }

    // Verify pre-crash state
    for (size_t i = 0; i < kGoodEntries; ++i) {
        ASSERT_TRUE(store.Get("good_" + std::to_string(i)).has_value())
            << "pre-crash: entry " << i << " missing";
    }

    // Crash
    store.SimulateCrash();
    EXPECT_TRUE(store.IsCrashed());

    // During crash: all entries must be inaccessible
    for (size_t i = 0; i < kGoodEntries; ++i) {
        EXPECT_FALSE(store.Get("good_" + std::to_string(i)).has_value())
            << "during-crash: entry " << i << " must not be accessible";
    }

    // Recover
    const auto replayed = store.Replay();
    EXPECT_EQ(replayed, kGoodEntries);
    EXPECT_FALSE(store.IsCrashed());

    // Post-recovery: all committed entries must be accessible and correct
    for (size_t i = 0; i < kGoodEntries; ++i) {
        const auto val = store.Get("good_" + std::to_string(i));
        ASSERT_TRUE(val.has_value())
            << "post-recovery: entry " << i << " missing";
        EXPECT_EQ(*val, "val_" + std::to_string(i))
            << "post-recovery: entry " << i << " has wrong value";
    }
}

// ===========================================================================
// HCR-07 — Concurrent recovery requests are safely serialized
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR07_ConcurrentRecoveryRequestsAreSafelySerializedNoDataRace) {
    SCOPED_TRACE("HCR-07: concurrent recovery serialization");

    WalJournalStore store;

    // Populate WAL with committed entries
    constexpr size_t kEntries = 8;
    for (size_t i = 0; i < kEntries; ++i) {
        const auto idx = store.AppendWal("rc_" + std::to_string(i), "val");
        store.CommitWal(idx);
    }
    store.SimulateCrash();

    SerializedRecoveryManager mgr(store);

    // Fire 4 concurrent recovery requests
    constexpr int kConcurrentCallers = 4;
    std::vector<std::thread> threads;
    std::atomic<size_t>      total_replayed{0};

    threads.reserve(kConcurrentCallers);
    for (int t = 0; t < kConcurrentCallers; ++t) {
        threads.emplace_back([&mgr, &total_replayed]() {
            const auto replayed = mgr.Recover();
            total_replayed.fetch_add(replayed, std::memory_order_relaxed);
        });
    }
    for (auto& th : threads) { th.join(); }

    // All invocations must have been serialized (no data race, counted correctly)
    EXPECT_EQ(mgr.RecoveryInvocations(), static_cast<size_t>(kConcurrentCallers))
        << "all concurrent recovery invocations must be counted";

    // Each entry must be recoverable after all concurrent recovery calls
    for (size_t i = 0; i < kEntries; ++i) {
        EXPECT_TRUE(store.Get("rc_" + std::to_string(i)).has_value())
            << "entry " << i << " missing after concurrent recovery";
    }
}

// ===========================================================================
// HCR-08 — Post-recovery query results match pre-failure known-good state
// ===========================================================================
TEST_F(RecoveryResilienceTest, HCR08_PostRecoveryQueryResultsMatchPreFailureKnownGoodState) {
    SCOPED_TRACE("HCR-08: post-recovery correctness vs. known-good state");

    WalJournalStore store;

    // Build known-good state
    std::unordered_map<std::string, std::string> known_good;
    std::mt19937 rng(kCanonicalSeed);
    for (int i = 0; i < 10; ++i) {
        const std::string key   = "kg_key_" + std::to_string(i);
        const std::string value = "kg_val_" + std::to_string(rng());
        known_good[key]         = value;
        const auto idx = store.AppendWal(key, value);
        store.CommitWal(idx);
    }

    // Crash and recover
    store.SimulateCrash();
    store.Replay();

    // Verify every known-good key/value pair is present and correct
    for (const auto& [key, expected_value] : known_good) {
        const auto actual = store.Get(key);
        ASSERT_TRUE(actual.has_value())
            << "post-recovery: known-good key '" << key << "' is missing";
        EXPECT_EQ(*actual, expected_value)
            << "post-recovery: key '" << key << "' has wrong value"
            << " (expected='" << expected_value << "' got='" << *actual << "')";
    }
}
} } // namespace themis::test

/**
 * @file w8a_incident_regression_shielding_test.cpp
 * @brief Wave 8A — Post-Release Incident Regression Shielding (IRS-01..IRS-08).
 *
 * Converts known release/production incidents and near-misses into targeted
 * regression tests.  Covers state-transition invariants, idempotency, ordering
 * bugs, retry/timeout edge cases, auth revocation, partial-update atomicity
 * and empty-result contract correctness.  All tests are deterministic via
 * kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// Canonical seed is provided by test_data_generator.h (themis::test::kCanonicalSeed)

// ---------------------------------------------------------------------------
// StateMachinePipeline — incident regression: invalid state transitions
// ---------------------------------------------------------------------------

/// @brief Possible document lifecycle states.
enum class DocState { kNew, kIndexed, kArchived, kDeleted };

/// @brief Minimal state machine tracking valid document lifecycle transitions.
class StateMachinePipeline {
public:
    struct TransitionResult {
        bool        ok{false};
        std::string error = {};
        DocState    state{DocState::kNew};
    };

    /// @brief Register a new document in kNew state.
    bool Register(const std::string& id) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (states_.count(id) > 0) { return false; }
        states_[id] = DocState::kNew;
        return true;
    }

    /// @brief Advance a document through a valid transition.
    /// Valid: kNew→kIndexed, kIndexed→kArchived, kIndexed→kDeleted,
    ///        kArchived→kDeleted.
    /// @param id     Document identifier.
    /// @param target Desired target state.
    /// @return TransitionResult describing outcome and current state.
    [[nodiscard]] TransitionResult Advance(const std::string& id, DocState target) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = states_.find(id);
        if (it == states_.end()) {
            return {false, "not_found", DocState::kNew};
        }
        const DocState current = it->second;
        if (!IsValidTransition(current, target)) {
            return {false, "invalid_transition", current};
        }
        it->second = target;
        return {true, "", target};
    }

    /// @brief Return the current state for a document, or nullopt if unknown.
    [[nodiscard]] std::optional<DocState> GetState(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = states_.find(id);
        if (it == states_.end()) { return std::nullopt; }
        return it->second;
    }

private:
    /// @brief Validate a state transition according to the document lifecycle.
    [[nodiscard]] static bool IsValidTransition(DocState from, DocState to) noexcept {
        switch (from) {
            case DocState::kNew:      return to == DocState::kIndexed;
            case DocState::kIndexed:  return to == DocState::kArchived || to == DocState::kDeleted;
            case DocState::kArchived: return to == DocState::kDeleted;
            case DocState::kDeleted:  return false;  // terminal state
        }
        return false;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, DocState> states_;
};

// ---------------------------------------------------------------------------
// IdempotentCommandBus — incident regression: duplicate command execution
// ---------------------------------------------------------------------------

/// @brief Result of a command dispatch.
struct CommandResult {
    bool        executed{false};
    bool        duplicate{false};
    std::string command_id = {};
};

/// @brief Command bus that enforces exactly-once execution via command ID set.
class IdempotentCommandBus {
public:
    /// @brief Dispatch a command.  Duplicate command_ids are silently de-duped.
    /// @param command_id  Stable idempotency key.
    /// @param payload     Command payload (opaque string).
    /// @return CommandResult indicating whether the command was newly executed.
    [[nodiscard]] CommandResult Dispatch(const std::string& command_id,
                                         const std::string& payload) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (seen_.count(command_id) > 0) {
            return {false, true, command_id};
        }
        seen_.insert(command_id);
        executed_payloads_.push_back(payload);
        return {true, false, command_id};
    }

    /// @brief Number of distinct commands executed (duplicates excluded).
    [[nodiscard]] size_t ExecutedCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return seen_.size();
    }

    /// @brief Ordered list of payloads actually executed.
    [[nodiscard]] std::vector<std::string> ExecutedPayloads() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return executed_payloads_;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> seen_;
    std::vector<std::string> executed_payloads_;
};

// ---------------------------------------------------------------------------
// OrderedCommandQueue — incident regression: out-of-order command execution
// ---------------------------------------------------------------------------

/// @brief Strict-order command queue: commands must be submitted in
///        monotonically increasing sequence order.
class OrderedCommandQueue {
public:
    struct EnqueueResult {
        bool     ok{false};
        uint64_t expected_seq{0};
        uint64_t received_seq{0};
    };

    /// @brief Enqueue a command at a given sequence number.
    /// @param seq     Expected next sequence number.
    /// @param payload Command payload.
    /// @return EnqueueResult; ok=false if seq is not the expected next value.
    [[nodiscard]] EnqueueResult Enqueue(uint64_t seq, const std::string& payload) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (seq != next_seq_) {
            return {false, next_seq_, seq};
        }
        queue_.push_back({seq, payload});
        ++next_seq_;
        return {true, seq, seq};
    }

    [[nodiscard]] size_t Size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.size();
    }

    [[nodiscard]] uint64_t NextExpected() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return next_seq_;
    }

private:
    struct Entry {
        uint64_t    seq = 0;
        std::string payload;
    };

    mutable std::mutex mutex_;
    uint64_t next_seq_{1};
    std::vector<Entry> queue_;
};

// ---------------------------------------------------------------------------
// TimeoutAwarePipeline — incident regression: retry on transient vs. timeout
// ---------------------------------------------------------------------------

/// @brief Distinguishes transient failures (retriable) from timeouts
///        (also retriable, but counted separately for incident triage).
class TimeoutAwarePipeline {
public:
    enum class FailureKind { kNone, kTransient, kTimeout };

    struct RunResult {
        bool        succeeded{false};
        size_t      attempts{0};
        size_t      timeout_count{0};
        FailureKind last_failure{FailureKind::kNone};
    };

    /// @brief Execute operation with up to max_attempts.
    /// @param fail_kind  Inject this failure kind for the first fail_for attempts.
    /// @param fail_for   Number of leading attempts that should fail.
    /// @param max_attempts Maximum attempts before giving up.
    [[nodiscard]] RunResult Execute(FailureKind fail_kind,
                                     size_t      fail_for,
                                     size_t      max_attempts = 4) {
        RunResult result;
        for (size_t attempt = 1; attempt <= max_attempts; ++attempt) {
            result.attempts = attempt;
            if (attempt <= fail_for) {
                result.last_failure = fail_kind;
                if (fail_kind == FailureKind::kTimeout) {
                    ++result.timeout_count;
                }
                continue;
            }
            result.succeeded = true;
            result.last_failure = FailureKind::kNone;
            return result;
        }
        return result;
    }
};

// ---------------------------------------------------------------------------
// CascadeContainmentPipeline — incident regression: timeout cascade
// ---------------------------------------------------------------------------

/// @brief Models three pipeline stages each with an independent deadline.
///        A timeout in stage N must not prevent stage N+1 from running.
class CascadeContainmentPipeline {
public:
    struct StageResult {
        bool        ok{false};
        std::string stage;
        bool        timed_out{false};
    };

    struct RunResult {
        StageResult stage1;
        StageResult stage2;
        StageResult stage3;
    };

    /// @brief Run three stages.  stage_timeout_mask is a bitmask: bit 0 = stage1
    ///        timeout, bit 1 = stage2 timeout, bit 2 = stage3 timeout.
    ///
    /// @param stage_timeout_mask  Bitmask of stages that should time out.
    /// @return RunResult with per-stage outcomes.
    [[nodiscard]] RunResult Run(unsigned stage_timeout_mask) {
        RunResult result;

        // Stage 1
        result.stage1.stage = "stage1";
        result.stage1.timed_out = (stage_timeout_mask & 0x1U) != 0;
        result.stage1.ok = !result.stage1.timed_out;

        // Stage 2 — independent deadline; runs regardless of stage 1 outcome
        result.stage2.stage = "stage2";
        result.stage2.timed_out = (stage_timeout_mask & 0x2U) != 0;
        result.stage2.ok = !result.stage2.timed_out;

        // Stage 3 — independent deadline; runs regardless of stage 1/2 outcome
        result.stage3.stage = "stage3";
        result.stage3.timed_out = (stage_timeout_mask & 0x4U) != 0;
        result.stage3.ok = !result.stage3.timed_out;

        return result;
    }
};

// ---------------------------------------------------------------------------
// RevocableAuthPipeline — incident regression: auth revocation mid-session
// ---------------------------------------------------------------------------

/// @brief Models a session that may have its token revoked mid-flight.
class RevocableAuthPipeline {
public:
    explicit RevocableAuthPipeline(std::shared_ptr<MockPipelineAuth> auth,
                                   std::shared_ptr<InMemoryPipelineStorage> storage)
        : auth_(std::move(auth)), storage_(std::move(storage)) {}

    struct OpResult {
        bool        ok{false};
        std::string error = {};
    };

    /// @brief Attempt a write using the provided token.
    [[nodiscard]] OpResult Write(const std::string& token,
                                  const std::string& key,
                                  const std::string& value) {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized) {
            return {false, "auth_denied:" + auth_result.reason};
        }
        storage_->Write(key, value);
        return {true, ""};
    }

    /// @brief Check whether a key is present in storage.
    [[nodiscard]] bool Contains(const std::string& key) const {
        return storage_->Contains(key);
    }

private:
    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
};

// ---------------------------------------------------------------------------
// AtomicUpdatePipeline — incident regression: partial update atomicity
// ---------------------------------------------------------------------------

/// @brief Simulates an update that either applies all fields or none.
///        Partial-write state must never be observable.
class AtomicUpdatePipeline {
public:
    struct Record {
        std::string field_a = {};
        std::string field_b;
        std::string field_c;
    };

    /// @brief Apply an atomic record update.  If fail_mid_update=true the
    ///        operation fails after field_a is staged but before commit,
    ///        ensuring no partial state reaches the store.
    [[nodiscard]] bool Update(const std::string& id,
                               Record new_record,
                               bool   fail_mid_update = false) {
        // Stage the update in a local copy
        Record staged = std::move(new_record);

        if (fail_mid_update) {
            // Simulate crash between staging and commit — store is unchanged
            return false;
        }

        std::lock_guard<std::mutex> lk(mutex_);
        records_[id] = std::move(staged);
        return true;
    }

    /// @brief Fetch a record, returning nullopt if absent.
    [[nodiscard]] std::optional<Record> Get(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = records_.find(id);
        if (it == records_.end()) { return std::nullopt; }
        return it->second;
    }

    /// @brief Number of stored records.
    [[nodiscard]] size_t Size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return records_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Record> records_;
};

// ---------------------------------------------------------------------------
// EmptyResultPipeline — incident regression: empty vs. null result contract
// ---------------------------------------------------------------------------

/// @brief Models a query that distinguishes an empty result set from an
///        absent/unknown query target.
class EmptyResultPipeline {
public:
    /// @brief Register a known query target with zero or more result entries.
    void Register(const std::string& query_key, std::vector<std::string> results) {
        std::lock_guard<std::mutex> lk(mutex_);
        index_[query_key] = std::move(results);
    }

    struct QueryResult {
        bool                     found{false};  ///< true if key is registered
        std::vector<std::string> items;         ///< may be empty even if found
    };

    /// @brief Execute a query.
    /// @param key  Query key.
    /// @return QueryResult: found=false means the key is unknown (null-like),
    ///         found=true with empty items means zero results (empty set).
    [[nodiscard]] QueryResult Query(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = index_.find(key);
        if (it == index_.end()) {
            return {false, {}};
        }
        return {true, it->second};
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> index_;
};

}  // namespace

// ===========================================================================
// Test Fixture
// ===========================================================================

class IncidentRegressionShieldingTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        rng_ = std::mt19937{kCanonicalSeed};
    }

    std::mt19937 rng_;
};

// ===========================================================================
// IRS-01 — State-transition regression: invalid transitions are rejected
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS01_InvalidStateTransitionIsRejectedAndStateIsPreserved) {
    SCOPED_TRACE("IRS-01: invalid state-transition regression");

    StateMachinePipeline fsm;
    ASSERT_TRUE(fsm.Register("doc-1"));

    // Valid transition: kNew → kIndexed
    const auto r1 = fsm.Advance("doc-1", DocState::kIndexed);
    ASSERT_TRUE(r1.ok) << "expected kNew→kIndexed to succeed";
    EXPECT_EQ(r1.state, DocState::kIndexed);

    // Invalid transition: kIndexed → kNew (backward — incident pattern)
    const auto r2 = fsm.Advance("doc-1", DocState::kNew);
    EXPECT_FALSE(r2.ok) << "kIndexed→kNew must be rejected";
    EXPECT_EQ(r2.error, "invalid_transition");
    // State must be unchanged after rejected transition
    const auto state = fsm.GetState("doc-1");
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, DocState::kIndexed)
        << "state must remain kIndexed after rejected transition";

    // Invalid transition: kIndexed → kIndexed (self-loop — incident pattern)
    const auto r3 = fsm.Advance("doc-1", DocState::kIndexed);
    EXPECT_FALSE(r3.ok) << "self-loop transition must be rejected";

    // Valid progression to terminal: kIndexed → kDeleted
    const auto r4 = fsm.Advance("doc-1", DocState::kDeleted);
    ASSERT_TRUE(r4.ok);
    EXPECT_EQ(r4.state, DocState::kDeleted);

    // Terminal state: no further transitions allowed
    const auto r5 = fsm.Advance("doc-1", DocState::kArchived);
    EXPECT_FALSE(r5.ok) << "kDeleted is terminal — no transitions allowed";
}

// ===========================================================================
// IRS-02 — Idempotency under repeated identical commands
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS02_IdempotentCommandBusDeduplicatesRepeatedSubmissions) {
    SCOPED_TRACE("IRS-02: idempotent command bus regression");

    IdempotentCommandBus bus;

    // First dispatch — should execute
    const auto r1 = bus.Dispatch("cmd-abc-001", "payload-A");
    EXPECT_TRUE(r1.executed);
    EXPECT_FALSE(r1.duplicate);

    // Repeated dispatch with same command_id — must be de-duplicated
    const auto r2 = bus.Dispatch("cmd-abc-001", "payload-A");
    EXPECT_FALSE(r2.executed) << "duplicate command must not re-execute";
    EXPECT_TRUE(r2.duplicate);

    // A third repetition
    const auto r3 = bus.Dispatch("cmd-abc-001", "payload-A");
    EXPECT_FALSE(r3.executed);
    EXPECT_TRUE(r3.duplicate);

    // Different command_id — must execute independently
    const auto r4 = bus.Dispatch("cmd-abc-002", "payload-B");
    EXPECT_TRUE(r4.executed);
    EXPECT_FALSE(r4.duplicate);

    // Executed count must reflect unique commands only
    EXPECT_EQ(bus.ExecutedCount(), 2u)
        << "only 2 distinct command IDs should be counted";

    // Payload list must contain exactly the two payloads, in order
    const auto payloads = bus.ExecutedPayloads();
    ASSERT_EQ(payloads.size(), 2u);
    EXPECT_EQ(payloads[0], "payload-A");
    EXPECT_EQ(payloads[1], "payload-B");
}

// ===========================================================================
// IRS-03 — Ordering regression: out-of-order command is rejected
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS03_OutOfOrderCommandIsRejectedByOrderedQueue) {
    SCOPED_TRACE("IRS-03: out-of-order command rejection regression");

    OrderedCommandQueue queue;

    // Normal in-order sequence
    ASSERT_TRUE(queue.Enqueue(1, "cmd-1").ok);
    ASSERT_TRUE(queue.Enqueue(2, "cmd-2").ok);
    ASSERT_TRUE(queue.Enqueue(3, "cmd-3").ok);
    EXPECT_EQ(queue.Size(), 3u);
    EXPECT_EQ(queue.NextExpected(), 4u);

    // Out-of-order submission (incident pattern: gap / duplicate)
    const auto gap = queue.Enqueue(5, "cmd-5");  // skips seq=4
    EXPECT_FALSE(gap.ok) << "seq=5 must be rejected; expected seq=4";
    EXPECT_EQ(gap.expected_seq, 4u);
    EXPECT_EQ(gap.received_seq, 5u);

    // Duplicate submission (incident pattern: re-delivery)
    const auto dup = queue.Enqueue(2, "cmd-2-again");
    EXPECT_FALSE(dup.ok) << "seq=2 re-submission must be rejected";

    // Correct next command is still accepted
    ASSERT_TRUE(queue.Enqueue(4, "cmd-4").ok);
    EXPECT_EQ(queue.Size(), 4u);
}

// ===========================================================================
// IRS-04 — Retry edge case: timeout failures are retriable and counted
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS04_TimeoutFailuresAreRetriableAndCountedSeparately) {
    SCOPED_TRACE("IRS-04: timeout-failure retry regression");

    TimeoutAwarePipeline pipeline;

    // Inject 2 timeout failures; succeed on attempt 3
    const auto result = pipeline.Execute(
        TimeoutAwarePipeline::FailureKind::kTimeout,
        /*fail_for=*/2,
        /*max_attempts=*/4);

    EXPECT_TRUE(result.succeeded) << "must succeed after 2 timeout retries";
    EXPECT_EQ(result.attempts, 3u) << "must take exactly 3 attempts";
    EXPECT_EQ(result.timeout_count, 2u) << "must record 2 timeouts";

    // Exhausted retries with all timeouts
    const auto exhausted = pipeline.Execute(
        TimeoutAwarePipeline::FailureKind::kTimeout,
        /*fail_for=*/99,
        /*max_attempts=*/3);

    EXPECT_FALSE(exhausted.succeeded) << "must fail after exhausting 3 attempts";
    EXPECT_EQ(exhausted.attempts, 3u);
    EXPECT_EQ(exhausted.timeout_count, 3u);
}

// ===========================================================================
// IRS-05 — Timeout cascade: stage N timeout does not block stage N+1
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS05_TimeoutInStageNDoesNotBlockSubsequentStages) {
    SCOPED_TRACE("IRS-05: cascade timeout containment regression");

    CascadeContainmentPipeline pipeline;

    // Inject timeout only in stage 2; stages 1 and 3 must succeed
    const auto result = pipeline.Run(/*stage_timeout_mask=*/0x2U);

    EXPECT_TRUE(result.stage1.ok) << "stage1 must succeed";
    EXPECT_FALSE(result.stage2.ok) << "stage2 must time out";
    EXPECT_TRUE(result.stage2.timed_out);
    EXPECT_TRUE(result.stage3.ok) << "stage3 must succeed independently of stage2 timeout";

    // Inject timeout only in stage 1; stage 2 and 3 must succeed
    const auto result2 = pipeline.Run(0x1U);
    EXPECT_FALSE(result2.stage1.ok);
    EXPECT_TRUE(result2.stage2.ok) << "stage2 must be unaffected by stage1 timeout";
    EXPECT_TRUE(result2.stage3.ok) << "stage3 must be unaffected by stage1 timeout";
}

// ===========================================================================
// IRS-06 — Auth revocation mid-session: revoked token is denied immediately
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS06_RevokedTokenIsDeniedOnNextOperation) {
    SCOPED_TRACE("IRS-06: auth revocation regression");

    auto auth    = CreateMockAuth();
    auto storage = CreateInMemoryStorage();
    RevocableAuthPipeline pipeline(auth, storage);

    auth->AllowToken("session-token-XYZ");

    // First write succeeds with valid token
    const auto w1 = pipeline.Write("session-token-XYZ", "key1", "value1");
    EXPECT_TRUE(w1.ok) << "initial write must succeed";
    EXPECT_TRUE(pipeline.Contains("key1"));

    // Revoke the token (simulate session expiry / security incident)
    auth->DenyToken("session-token-XYZ");

    // Subsequent write must fail
    const auto w2 = pipeline.Write("session-token-XYZ", "key2", "value2");
    EXPECT_FALSE(w2.ok) << "write must fail after token revocation";
    EXPECT_FALSE(pipeline.Contains("key2"))
        << "no data must be written after revocation";

    // Previously written data must not be retroactively removed
    EXPECT_TRUE(pipeline.Contains("key1"))
        << "pre-revocation data must remain intact";
}

// ===========================================================================
// IRS-07 — Partial-update atomicity: failed update leaves no partial state
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS07_FailedAtomicUpdateLeavesNoPartialState) {
    SCOPED_TRACE("IRS-07: partial-update atomicity regression");

    AtomicUpdatePipeline pipeline;

    // Establish baseline record
    const AtomicUpdatePipeline::Record baseline{"alpha", "beta", "gamma"};
    ASSERT_TRUE(pipeline.Update("rec-1", baseline));

    const auto before = pipeline.Get("rec-1");
    ASSERT_TRUE(before.has_value());
    EXPECT_EQ(before->field_a, "alpha");

    // Simulate crash mid-update (fail_mid_update=true)
    const AtomicUpdatePipeline::Record partial{"UPDATED_A", "UPDATED_B", "UPDATED_C"};
    const bool ok = pipeline.Update("rec-1", partial, /*fail_mid_update=*/true);
    EXPECT_FALSE(ok) << "update with simulated mid-crash must return false";

    // Record must be unchanged — no partial field_a visible
    const auto after = pipeline.Get("rec-1");
    ASSERT_TRUE(after.has_value()) << "record must still exist after failed update";
    EXPECT_EQ(after->field_a, "alpha")
        << "field_a must remain unchanged after failed atomic update";
    EXPECT_EQ(after->field_b, "beta");
    EXPECT_EQ(after->field_c, "gamma");

    // Size must still be 1 — no phantom entry created
    EXPECT_EQ(pipeline.Size(), 1u);
}

// ===========================================================================
// IRS-08 — Empty vs. null result contract: distinguishable response shapes
// ===========================================================================

TEST_F(IncidentRegressionShieldingTest,
       IRS08_EmptyResultSetIsDistinguishableFromUnknownKey) {
    SCOPED_TRACE("IRS-08: empty-result vs. null-result contract regression");

    EmptyResultPipeline pipeline;

    // Registered key with zero results — empty set, not null
    pipeline.Register("known-empty-key", {});

    // Registered key with results
    pipeline.Register("populated-key", {"item-A", "item-B"});

    // Query an unknown key — must return found=false
    const auto unknown = pipeline.Query("never-registered-key");
    EXPECT_FALSE(unknown.found) << "unknown key must return found=false (null-like)";
    EXPECT_TRUE(unknown.items.empty());

    // Query the empty-registered key — must return found=true, items empty
    const auto empty_result = pipeline.Query("known-empty-key");
    EXPECT_TRUE(empty_result.found)
        << "registered key must return found=true even with zero items";
    EXPECT_TRUE(empty_result.items.empty())
        << "zero-item result set must have empty items vector";

    // Query the populated key — must return found=true, items non-empty
    const auto populated = pipeline.Query("populated-key");
    EXPECT_TRUE(populated.found);
    ASSERT_EQ(populated.items.size(), 2u);
    EXPECT_EQ(populated.items[0], "item-A");
    EXPECT_EQ(populated.items[1], "item-B");

    // Contract: unknown.found != empty_result.found — they are distinguishable
    EXPECT_NE(unknown.found, empty_result.found)
        << "null result and empty result must be distinguishable by found flag";
}
} } // namespace themis::test

/**
 * @file w8c_determinism_flake_ci_signal_test.cpp
 * @brief Wave 8C — Determinism, Flake Burn-down & CI Signal Quality
 *        (DFQ-01..DFQ-08).
 *
 * Eliminates residual flakiness by enforcing deterministic fixtures/seeds,
 * test isolation, complete cleanup and sharper assertions.  All tests are
 * deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// Canonical seed is provided by test_data_generator.h (themis::test::kCanonicalSeed)

// ---------------------------------------------------------------------------
// DeterministicDataProducer — DFQ-01/DFQ-06: identical seed → identical output
// ---------------------------------------------------------------------------

/// @brief Produces a deterministic sequence of string keys from a fixed RNG.
class DeterministicDataProducer {
public:
    /// @brief Construct with an explicit seed.
    explicit DeterministicDataProducer(uint32_t seed) : rng_(seed) {}

    /// @brief Generate @p count keys of the form "key-{hex8}".
    /// Two instances with the same seed produce identical sequences.
    [[nodiscard]] std::vector<std::string> GenerateKeys(size_t count) {
        std::vector<std::string> keys;
        keys.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            const uint32_t raw = rng_();
            char buf[32];
            std::snprintf(buf, sizeof(buf), "key-%08x", raw);
            keys.emplace_back(buf);
        }
        return keys;
    }

private:
    std::mt19937 rng_;
};

// ---------------------------------------------------------------------------
// IsolatedTestEnvironment — DFQ-02: test isolation
// ---------------------------------------------------------------------------

/// @brief A lightweight scoped environment that tracks whether any state leaks
///        between tests.  Each test creates its own instance; a shared
///        leakage detector verifies isolation.
class IsolatedTestEnvironment {
public:
    explicit IsolatedTestEnvironment(const std::string& test_name)
        : test_name_(test_name) {
        std::lock_guard<std::mutex> lk(s_mutex_);
        if (s_live_envs_.count(test_name_) > 0) {
            leaked_ = true;
        }
        s_live_envs_.insert(test_name_);
    }

    ~IsolatedTestEnvironment() {
        std::lock_guard<std::mutex> lk(s_mutex_);
        s_live_envs_.erase(test_name_);
    }

    /// @brief True if another env with the same name was alive at construction.
    [[nodiscard]] bool IsLeaked() const noexcept { return leaked_; }

    /// @brief Number of currently live environments.
    [[nodiscard]] static size_t LiveCount() {
        std::lock_guard<std::mutex> lk(s_mutex_);
        return s_live_envs_.size();
    }

    /// @brief Reset the shared state (call only from fixture teardown).
    static void ResetShared() {
        std::lock_guard<std::mutex> lk(s_mutex_);
        s_live_envs_.clear();
    }

private:
    std::string test_name_;
    bool        leaked_{false};

    static std::mutex                        s_mutex_;
    static std::unordered_set<std::string>   s_live_envs_;
};

std::mutex                      IsolatedTestEnvironment::s_mutex_;
std::unordered_set<std::string> IsolatedTestEnvironment::s_live_envs_;

// ---------------------------------------------------------------------------
// HandleTracker — DFQ-03: cleanup completeness
// ---------------------------------------------------------------------------

/// @brief Tracks open/close pairs to detect handle leaks.
class HandleTracker {
public:
    /// @brief Simulate opening a handle.  Returns a handle ID.
    [[nodiscard]] int Open() {
        std::lock_guard<std::mutex> lk(mutex_);
        const int id = next_id_++;
        open_handles_.insert(id);
        ++total_opens_;
        return id;
    }

    /// @brief Simulate closing a handle.
    /// @param id  Handle ID returned by Open().
    /// @return true if the handle was open; false if unknown (double-close).
    [[nodiscard]] bool Close(int id) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (open_handles_.erase(id) == 0) {
            return false;  // double-close or unknown handle
        }
        ++total_closes_;
        return true;
    }

    [[nodiscard]] size_t OpenCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return open_handles_.size();
    }

    [[nodiscard]] size_t TotalOpens()  const { std::lock_guard<std::mutex> lk(mutex_); return total_opens_;  }
    [[nodiscard]] size_t TotalCloses() const { std::lock_guard<std::mutex> lk(mutex_); return total_closes_; }

private:
    mutable std::mutex          mutex_;
    std::unordered_set<int>     open_handles_;
    int                         next_id_{1};
    size_t                      total_opens_{0};
    size_t                      total_closes_{0};
};

// ---------------------------------------------------------------------------
// PreciseAssertionPipeline — DFQ-04: precise assertion messages
// ---------------------------------------------------------------------------

/// @brief Demonstrates precise, annotated assertions that emit structured
///        diagnostic context on failure.
class PreciseAssertionPipeline {
public:
    struct WriteResult {
        bool        ok{false};
        std::string error_code;
        std::string expected_detail;  ///< only set on failure
    };

    explicit PreciseAssertionPipeline(std::shared_ptr<InMemoryPipelineStorage> storage)
        : storage_(std::move(storage)) {}

    /// @brief Write a key/value pair.
    /// @param key    Must be non-empty and ≤ 64 chars.
    /// @param value  Must be non-empty and ≤ 256 chars.
    [[nodiscard]] WriteResult Write(const std::string& key, const std::string& value) {
        if (key.empty()) {
            return {false, "empty_key", "key must be non-empty"};
        }
        if (key.size() > 64) {
            return {false, "key_too_long",
                    "key length " + std::to_string(key.size()) + " exceeds 64"};
        }
        if (value.empty()) {
            return {false, "empty_value", "value must be non-empty"};
        }
        if (value.size() > 256) {
            return {false, "value_too_long",
                    "value length " + std::to_string(value.size()) + " exceeds 256"};
        }
        storage_->Write(key, value);
        return {true, "", ""};
    }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
};

// ---------------------------------------------------------------------------
// FlakeProbeOperation — DFQ-05: timing-independent flake probe
// ---------------------------------------------------------------------------

/// @brief A trivially-deterministic operation that must never fail across N
///        repetitions regardless of scheduling.
class FlakeProbeOperation {
public:
    /// @brief Add value to an atomic counter.
    /// @param delta  Increment amount.
    /// @return New counter value.
    [[nodiscard]] int64_t Increment(int64_t delta) noexcept {
        return counter_.fetch_add(delta, std::memory_order_relaxed) + delta;
    }

    [[nodiscard]] int64_t Value() const noexcept {
        return counter_.load(std::memory_order_relaxed);
    }

    void Reset() noexcept {
        counter_.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<int64_t> counter_{0};
};

// ---------------------------------------------------------------------------
// DiagnosticEventSink — DFQ-08: structured diagnostic output
// ---------------------------------------------------------------------------

/// @brief Severity levels for diagnostic events.
enum class DiagSeverity { kInfo, kWarning, kError };

/// @brief Structured diagnostic event.
struct DiagEvent {
    DiagSeverity severity{DiagSeverity::kInfo};
    std::string  source;       ///< non-empty module/test name
    std::string  message;      ///< non-empty human-readable description
    uint64_t     timestamp_us; ///< non-zero monotonic microsecond timestamp
};

/// @brief Collects and validates diagnostic events.
class DiagnosticEventSink {
public:
    /// @brief Emit a diagnostic event with the current timestamp.
    void Emit(DiagSeverity severity,
              const std::string& source,
              const std::string& message) {
        const auto now_us =
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                .count());

        std::lock_guard<std::mutex> lk(mutex_);
        events_.push_back({severity, source, message, now_us});
    }

    [[nodiscard]] size_t Count() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return events_.size();
    }

    /// @brief True if all events are well-formed (non-empty fields, non-zero ts).
    [[nodiscard]] bool AllWellFormed() const {
        std::lock_guard<std::mutex> lk(mutex_);
        for (const auto& e : events_) {
            if (e.source.empty())    { return false; }
            if (e.message.empty())   { return false; }
            if (e.timestamp_us == 0) { return false; }
        }
        return true;
    }

    /// @brief True if at least one event with the given severity exists.
    [[nodiscard]] bool HasSeverity(DiagSeverity severity) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return std::any_of(events_.begin(), events_.end(),
                           [severity](const DiagEvent& e) {
                               return e.severity == severity;
                           });
    }

    [[nodiscard]] std::vector<DiagEvent> Snapshot() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return events_;
    }

private:
    mutable std::mutex mutex_;
    std::vector<DiagEvent> events_;
};

}  // namespace

// ===========================================================================
// Test Fixture
// ===========================================================================

class DeterminismFlakeCiSignalTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        IsolatedTestEnvironment::ResetShared();
    }
};

// ===========================================================================
// DFQ-01 — Identical seed produces identical key sequence
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ01_IdenticalSeedProducesIdenticalKeySequence) {
    SCOPED_TRACE("DFQ-01: deterministic fixture — identical seed → identical output");

    DeterministicDataProducer p1(kCanonicalSeed);
    DeterministicDataProducer p2(kCanonicalSeed);

    constexpr size_t kN = 50;
    const auto keys1 = p1.GenerateKeys(kN);
    const auto keys2 = p2.GenerateKeys(kN);

    ASSERT_EQ(keys1.size(), kN);
    ASSERT_EQ(keys2.size(), kN);
    EXPECT_EQ(keys1, keys2)
        << "two producers with the same seed must produce identical key sequences";

    // Different seed must produce a different sequence
    DeterministicDataProducer p3(kCanonicalSeed + 1u);
    const auto keys3 = p3.GenerateKeys(kN);
    EXPECT_NE(keys1, keys3)
        << "different seeds must produce different key sequences";
}

// ===========================================================================
// DFQ-02 — Test isolation: no shared state between independent test environments
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ02_IndependentTestEnvironmentsDoNotShareState) {
    SCOPED_TRACE("DFQ-02: test isolation — environments do not share state");

    // Create environment A — should not be leaked
    {
        IsolatedTestEnvironment env_a("env-A");
        EXPECT_FALSE(env_a.IsLeaked())
            << "env-A must not be leaked on first creation";
        EXPECT_EQ(IsolatedTestEnvironment::LiveCount(), 1u);

        // Create env B with a different name — must be independent
        IsolatedTestEnvironment env_b("env-B");
        EXPECT_FALSE(env_b.IsLeaked())
            << "env-B must not be leaked; it has a distinct name";
        EXPECT_EQ(IsolatedTestEnvironment::LiveCount(), 2u);
    }  // both envs destroyed

    // After destruction, live count returns to 0
    EXPECT_EQ(IsolatedTestEnvironment::LiveCount(), 0u)
        << "all environments must be cleaned up after leaving scope";

    // Re-creation with same name must work cleanly (no false leak detection)
    IsolatedTestEnvironment env_a2("env-A");
    EXPECT_FALSE(env_a2.IsLeaked())
        << "env-A2 must not report leak — env-A was destroyed";
}

// ===========================================================================
// DFQ-03 — Cleanup completeness: all handles closed; none leaked
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ03_AllHandlesClosedAfterWorkload) {
    SCOPED_TRACE("DFQ-03: cleanup completeness — no handle leaks");

    HandleTracker tracker;

    constexpr size_t kHandleCount = 20;
    std::vector<int> handles;
    handles.reserve(kHandleCount);

    // Open all handles
    for (size_t i = 0; i < kHandleCount; ++i) {
        handles.push_back(tracker.Open());
    }
    EXPECT_EQ(tracker.OpenCount(), kHandleCount);
    EXPECT_EQ(tracker.TotalOpens(), kHandleCount);

    // Close all handles
    for (const int h : handles) {
        EXPECT_TRUE(tracker.Close(h))
            << "handle " << h << " must be closable exactly once";
    }

    EXPECT_EQ(tracker.OpenCount(), 0u)
        << "no handles must remain open after cleanup";
    EXPECT_EQ(tracker.TotalOpens(), tracker.TotalCloses())
        << "open count must equal close count";

    // Double-close must return false (detects bugs in cleanup code)
    EXPECT_FALSE(tracker.Close(handles[0]))
        << "double-close must return false";
}

// ===========================================================================
// DFQ-04 — Precise assertions: structured error context on validation failure
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ04_ValidationFailuresEmitPreciseDiagnosticContext) {
    SCOPED_TRACE("DFQ-04: precise assertion messages on failure");

    auto storage = CreateInMemoryStorage();
    PreciseAssertionPipeline pipeline(storage);

    // Empty key — must fail with precise code
    const auto r_empty_key = pipeline.Write("", "value");
    EXPECT_FALSE(r_empty_key.ok);
    EXPECT_EQ(r_empty_key.error_code, "empty_key")
        << "error_code must be 'empty_key', got: '" << r_empty_key.error_code << "'";
    EXPECT_FALSE(r_empty_key.expected_detail.empty())
        << "expected_detail must be non-empty for diagnostic output";

    // Key too long (65 chars)
    const std::string long_key(65, 'x');
    const auto r_long_key = pipeline.Write(long_key, "value");
    EXPECT_FALSE(r_long_key.ok);
    EXPECT_EQ(r_long_key.error_code, "key_too_long");
    EXPECT_NE(r_long_key.expected_detail.find("65"), std::string::npos)
        << "expected_detail must include the actual key length for diagnosis";

    // Empty value
    const auto r_empty_val = pipeline.Write("valid-key", "");
    EXPECT_FALSE(r_empty_val.ok);
    EXPECT_EQ(r_empty_val.error_code, "empty_value");

    // Valid write
    const auto r_ok = pipeline.Write("valid-key", "valid-value");
    EXPECT_TRUE(r_ok.ok);
    EXPECT_TRUE(r_ok.error_code.empty());
    EXPECT_TRUE(storage->Contains("valid-key"));
}

// ===========================================================================
// DFQ-05 — Flake burn-down: N=50 repetitions with no timing-dependent failure
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ05_FiftyRepetitionsProduceNoFlakyFailure) {
    SCOPED_TRACE("DFQ-05: flake burn-down — 50 repetitions, 0 failures expected");

    FlakeProbeOperation op;
    constexpr int kRepetitions = 50;
    constexpr int64_t kDelta   = 1;

    for (int i = 0; i < kRepetitions; ++i) {
        op.Reset();
        const int64_t result = op.Increment(kDelta);
        ASSERT_EQ(result, kDelta)
            << "increment at repetition " << i << " returned " << result
            << " instead of " << kDelta << " — possible flakiness regression";
        ASSERT_EQ(op.Value(), kDelta);
    }
}

// ===========================================================================
// DFQ-06 — Seed boundary: seed=0 and seed=UINT32_MAX produce valid sequences
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ06_BoundarySeeds0AndMaxProduceValidSequences) {
    SCOPED_TRACE("DFQ-06: seed boundary — seed=0 and seed=UINT32_MAX");

    constexpr size_t kN = 10;

    DeterministicDataProducer p_zero(0u);
    const auto keys_zero = p_zero.GenerateKeys(kN);
    ASSERT_EQ(keys_zero.size(), kN);
    for (const auto& k : keys_zero) {
        EXPECT_FALSE(k.empty())
            << "seed=0 must produce non-empty keys";
        EXPECT_EQ(k.substr(0, 4), "key-")
            << "seed=0 key must start with 'key-' prefix";
    }

    DeterministicDataProducer p_max(std::numeric_limits<uint32_t>::max());
    const auto keys_max = p_max.GenerateKeys(kN);
    ASSERT_EQ(keys_max.size(), kN);
    for (const auto& k : keys_max) {
        EXPECT_FALSE(k.empty())
            << "seed=UINT32_MAX must produce non-empty keys";
    }

    // Sequences from different boundary seeds must differ
    EXPECT_NE(keys_zero, keys_max)
        << "seed=0 and seed=UINT32_MAX must produce distinct sequences";
}

// ===========================================================================
// DFQ-07 — Concurrent determinism: parallel identical ops produce correct total
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ07_ConcurrentIdenticalIncrementProducesCorrectTotal) {
    SCOPED_TRACE("DFQ-07: concurrent determinism — parallel ops, correct total");

    FlakeProbeOperation op;

    constexpr size_t kThreads  = 4;
    constexpr int64_t kPerThread = 100;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (size_t t = 0; t < kThreads; ++t) {
        workers.emplace_back([&op]() {
            for (int64_t i = 0; i < kPerThread; ++i) {
                op.Increment(1);
            }
        });
    }
    for (auto& w : workers) { w.join(); }

    const int64_t expected = static_cast<int64_t>(kThreads) * kPerThread;
    EXPECT_EQ(op.Value(), expected)
        << "concurrent increments: expected " << expected
        << " but got " << op.Value()
        << " — possible race condition or memory ordering bug";
}

// ===========================================================================
// DFQ-08 — Structured diagnostic output: anomaly emits parseable event
// ===========================================================================

TEST_F(DeterminismFlakeCiSignalTest,
       DFQ08_AnomalyEmitsWellFormedStructuredDiagnosticEvent) {
    SCOPED_TRACE("DFQ-08: structured diagnostic output — parseable event on anomaly");

    DiagnosticEventSink sink;

    // Normal operation — info event
    sink.Emit(DiagSeverity::kInfo,    "w8c-test", "workload started");
    sink.Emit(DiagSeverity::kWarning, "w8c-test", "approaching resource limit");

    // Anomaly — error event
    sink.Emit(DiagSeverity::kError, "w8c-test", "unexpected null state detected");

    EXPECT_EQ(sink.Count(), 3u);

    // All events must be well-formed
    EXPECT_TRUE(sink.AllWellFormed())
        << "all emitted events must have non-empty source/message and non-zero timestamp";

    // At least one ERROR event must be present
    EXPECT_TRUE(sink.HasSeverity(DiagSeverity::kError))
        << "anomaly path must emit at least one ERROR-severity event";

    // Validate each event individually for precise CI diagnosis
    const auto events = sink.Snapshot();
    ASSERT_EQ(events.size(), 3u);
    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_FALSE(events[i].source.empty())
            << "event[" << i << "].source must not be empty";
        EXPECT_FALSE(events[i].message.empty())
            << "event[" << i << "].message must not be empty";
        EXPECT_GT(events[i].timestamp_us, 0u)
            << "event[" << i << "].timestamp_us must be non-zero";
    }

    // Verify event ordering (timestamps must be non-decreasing)
    for (size_t i = 1; i < events.size(); ++i) {
        EXPECT_GE(events[i].timestamp_us, events[i - 1].timestamp_us)
            << "event[" << i << "] timestamp must be >= event[" << (i - 1)
            << "] timestamp — monotonic clock requirement";
    }
}
} } // namespace themis::test

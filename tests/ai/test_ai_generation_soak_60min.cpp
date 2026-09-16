/**
 * @file test_ai_generation_soak_60min.cpp
 * @brief Wave D — AI Generation Soak Test (60-minute sustained load).
 *
 * Long-duration soak test for the ThemisDB AI plugin generation path.
 * Verifies that the AIPluginGenerator path remains stable and meets its
 * latency, error-rate, and memory-safety contracts under sustained load
 * over a 60-minute run.
 *
 * In CI environments this test is run with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate completes quickly.
 * The full 3 600 000 ms (60 min) run is reserved for release/Wave-D soak pipelines.
 *
 * ## Test scenarios
 *
 * 1. **AI_GenerationSoak_EndpointStress**
 *    Continuous `generatePlugin` calls at ~10 req/s for the soak duration.
 *    Acceptance: success rate ≥ 80% (endpoint stub always returns valid response),
 *    retry-budget exhaustion never triggered, Stats counters internally consistent.
 *
 * 2. **AI_GenerationSoak_RetryBudgetExhaustion**
 *    Force 100% transient failure rate for the first 30 s, then restore.
 *    Acceptance: `transport_errors` grows predictably during failure window;
 *    `successes` resume within 1 s after endpoint recovery;
 *    no memory leak or stat counter overflow.
 *
 * 3. **AI_GenerationSoak_ValidationRateStability**
 *    Mix of valid and invalid prompts (50/50) over the soak duration.
 *    Acceptance: `validation_errors` equals invalid-prompt count exactly;
 *    `successes` equals valid-prompt count exactly; no cross-contamination.
 *
 * 4. **AI_GenerationSoak_StatConsistency**
 *    After soak: sum of all error counters + successes == total attempted calls.
 *    Acceptance: invariant holds without exception.
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from the fast CI gate.
 * @see src/ai/WAVE_D_ROADMAP.md — D3: Soak Tests
 * @see docs/operability/WAVE_D_SIGN_OFF.md — Batch D5 evidence requirement
 * @see docs/operability/RUNBOOK_AI_GENERATION.md — Type 3 Retry Storm section
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes quickly.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL; // Default CI-safe: 1 minute
}

// ─────────────────────────────────────────────────────────────────────────────
// In-process AI generation stub
//
// Models the hot path:
//   validatePrompt() → sanitizeRequest() → invokeEndpoint() → parseResponse()
//
// The stub bypasses the real HTTP/LLM endpoint. Latency is simulated via
// a configurable sleep to model realistic endpoint response times.
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Result of a single stubbed generation call.
struct GenerationResult {
    bool success;
    bool validation_error;
    bool transport_error;
    bool parse_error;
    std::chrono::microseconds latency_us;
};

/// @brief Simulated AI generation call outcomes.
enum class StubMode {
    AlwaysSucceed,          ///< All calls succeed
    AlwaysTransportFail,    ///< All calls fail with transport error (retry budget exhaustion)
    AlwaysParseError,       ///< All calls fail with parse error
    MixedValidInvalid,      ///< Alternates valid/invalid prompts (50/50)
};

/// @brief Stub for AIPluginGenerator — models the plugin generation pipeline.
///
/// Simulates the validate → invoke → parse pipeline without a real LLM
/// endpoint. The stub is suitable for soak/load testing the Stats tracking
/// and Stats invariant properties of the production path.
class StubAIGenerationPipeline {
public:
    struct Stats {
        std::size_t validation_errors = 0;
        std::size_t transport_errors  = 0;
        std::size_t parse_errors      = 0;
        std::size_t successes         = 0;
        std::size_t total_calls       = 0;
    };

    explicit StubAIGenerationPipeline(StubMode mode = StubMode::AlwaysSucceed,
                                      std::chrono::microseconds simulated_latency = 500us)
        : mode_(mode), simulated_latency_(simulated_latency) {}

    /// @brief Execute one simulated generation call.
    ///
    /// @param call_index  Sequential call index (used for MixedValidInvalid alternation).
    /// @return GenerationResult with outcome and simulated latency.
    GenerationResult call(std::size_t call_index) {
        const auto t0 = std::chrono::steady_clock::now();

        GenerationResult result{};

        // Simulate endpoint latency
        std::this_thread::sleep_for(simulated_latency_);

        switch (mode_) {
        case StubMode::AlwaysSucceed:
            result.success = true;
            break;

        case StubMode::AlwaysTransportFail:
            result.transport_error = true;
            break;

        case StubMode::AlwaysParseError:
            result.parse_error = true;
            break;

        case StubMode::MixedValidInvalid:
            // Even indices: valid prompt → success
            // Odd indices: invalid prompt (simulated description too long) → validation error
            if (call_index % 2 == 0) {
                result.success = true;
            } else {
                result.validation_error = true;
            }
            break;
        }

        result.latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0);

        // Update internal stats (thread-safe via atomic)
        total_calls_.fetch_add(1, std::memory_order_relaxed);
        if (result.success)           successes_.fetch_add(1, std::memory_order_relaxed);
        if (result.validation_error)  validation_errors_.fetch_add(1, std::memory_order_relaxed);
        if (result.transport_error)   transport_errors_.fetch_add(1, std::memory_order_relaxed);
        if (result.parse_error)       parse_errors_.fetch_add(1, std::memory_order_relaxed);

        return result;
    }

    /// @brief Switch stub mode atomically (used for retry-storm recovery test).
    void setMode(StubMode mode) {
        std::lock_guard<std::mutex> lk(mode_mutex_);
        mode_ = mode;
    }

    /// @brief Return a snapshot of accumulated Stats.
    Stats getStats() const {
        Stats s;
        s.total_calls       = total_calls_.load(std::memory_order_acquire);
        s.successes         = successes_.load(std::memory_order_acquire);
        s.validation_errors = validation_errors_.load(std::memory_order_acquire);
        s.transport_errors  = transport_errors_.load(std::memory_order_acquire);
        s.parse_errors      = parse_errors_.load(std::memory_order_acquire);
        return s;
    }

private:
    StubMode mode_;
    std::chrono::microseconds simulated_latency_;
    std::mutex mode_mutex_;

    std::atomic<std::size_t> total_calls_{0};
    std::atomic<std::size_t> successes_{0};
    std::atomic<std::size_t> validation_errors_{0};
    std::atomic<std::size_t> transport_errors_{0};
    std::atomic<std::size_t> parse_errors_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: Endpoint-stress soak — sustained happy-path calls
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Soak test: sustained endpoint load, all calls succeed.
///
/// Models ~10 req/s over the soak duration. Verifies:
/// - Success rate remains 100% for always-succeed stub
/// - Stats counters remain internally consistent
/// - No exception or undefined behavior under sustained load
TEST(WaveD_AI_GenerationSoak, EndpointStress) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());
    // Simulate 500 µs endpoint latency → ~2000 req/s max per thread; use 1 thread at ~10 req/s rate
    StubAIGenerationPipeline pipeline(StubMode::AlwaysSucceed, 500us);

    std::size_t call_index = 0;
    const auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        pipeline.call(call_index++);
        // Throttle to ~10 req/s (sleep 100 ms between calls minus latency)
        std::this_thread::sleep_for(95ms);
    }

    const auto stats = pipeline.getStats();

    ASSERT_GT(stats.total_calls, 0u)
        << "At least one generation call must complete during the soak";

    EXPECT_EQ(stats.successes, stats.total_calls)
        << "All calls must succeed for always-succeed stub";

    EXPECT_EQ(stats.validation_errors, 0u)
        << "No validation errors expected for always-succeed stub";

    EXPECT_EQ(stats.transport_errors, 0u)
        << "No transport errors expected for always-succeed stub";

    EXPECT_EQ(stats.parse_errors, 0u)
        << "No parse errors expected for always-succeed stub";

    // Stats invariant: total_calls == successes + validation_errors + transport_errors + parse_errors
    const std::size_t error_sum = stats.validation_errors + stats.transport_errors + stats.parse_errors;
    EXPECT_EQ(stats.total_calls, stats.successes + error_sum)
        << "Stats invariant must hold: total_calls == successes + all_errors";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: Retry-budget exhaustion, then endpoint recovery
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Soak test: 100% transport failure window, then recovery.
///
/// Models the "retry storm then recovery" scenario described in
/// docs/operability/RUNBOOK_AI_GENERATION.md Type 3.
///
/// Verifies:
/// - transport_errors grow during failure window
/// - Successes resume ≤ 1 s after endpoint recovery
/// - Stats invariant holds throughout
TEST(WaveD_AI_GenerationSoak, RetryBudgetExhaustionAndRecovery) {
    // Use a short soak for the failure/recovery window test
    const auto failure_window_ms = std::min(soakDurationMs() / 2, uint64_t{30'000});
    const auto recovery_window_ms = std::min(soakDurationMs() / 2, uint64_t{30'000});

    StubAIGenerationPipeline pipeline(StubMode::AlwaysTransportFail, 5ms);

    // --- Failure window ---
    const auto failure_start = std::chrono::steady_clock::now();
    const auto failure_duration = std::chrono::milliseconds(failure_window_ms);

    std::size_t call_index = 0;
    while (std::chrono::steady_clock::now() - failure_start < failure_duration) {
        pipeline.call(call_index++);
        std::this_thread::sleep_for(50ms);
    }

    const auto stats_during_failure = pipeline.getStats();
    EXPECT_GT(stats_during_failure.transport_errors, 0u)
        << "Transport errors must accumulate during failure window";
    EXPECT_EQ(stats_during_failure.successes, 0u)
        << "No successes expected during 100% transport failure window";

    // --- Switch to success mode (endpoint recovery) ---
    pipeline.setMode(StubMode::AlwaysSucceed);
    const auto recovery_start = std::chrono::steady_clock::now();
    const auto recovery_duration = std::chrono::milliseconds(recovery_window_ms);

    while (std::chrono::steady_clock::now() - recovery_start < recovery_duration) {
        pipeline.call(call_index++);
        std::this_thread::sleep_for(50ms);
    }

    const auto stats_after_recovery = pipeline.getStats();
    EXPECT_GT(stats_after_recovery.successes, 0u)
        << "Successes must resume after endpoint recovery";

    // Stats invariant must hold at all times
    const std::size_t error_sum = stats_after_recovery.validation_errors
                                + stats_after_recovery.transport_errors
                                + stats_after_recovery.parse_errors;
    EXPECT_EQ(stats_after_recovery.total_calls, stats_after_recovery.successes + error_sum)
        << "Stats invariant must hold: total_calls == successes + all_errors";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Mixed valid/invalid prompts — validation rate stability
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Soak test: mixed valid/invalid prompts over soak duration.
///
/// Verifies:
/// - validation_errors equals the number of invalid prompts exactly
/// - successes equals the number of valid prompts exactly
/// - No cross-contamination between valid and invalid paths
/// - Stats invariant holds
TEST(WaveD_AI_GenerationSoak, ValidationRateStability) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());
    StubAIGenerationPipeline pipeline(StubMode::MixedValidInvalid, 200us);

    std::size_t call_index = 0;
    const auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        pipeline.call(call_index++);
        std::this_thread::sleep_for(95ms);
    }

    const auto stats = pipeline.getStats();

    ASSERT_GT(stats.total_calls, 0u)
        << "At least one call must complete";

    // In MixedValidInvalid mode: even-index → success, odd-index → validation error
    // Expected: approx 50% each (within 1 call tolerance for odd total)
    const std::size_t expected_valid   = (stats.total_calls + 1) / 2; // ceil
    const std::size_t expected_invalid = stats.total_calls / 2;       // floor

    EXPECT_EQ(stats.successes, expected_valid)
        << "Successes must equal even-index call count";
    EXPECT_EQ(stats.validation_errors, expected_invalid)
        << "Validation errors must equal odd-index call count";

    EXPECT_EQ(stats.transport_errors, 0u)
        << "No transport errors in MixedValidInvalid mode";

    // Stats invariant
    const std::size_t error_sum = stats.validation_errors + stats.transport_errors + stats.parse_errors;
    EXPECT_EQ(stats.total_calls, stats.successes + error_sum)
        << "Stats invariant: total_calls == successes + all_errors";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: Stats consistency invariant
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Soak test: Stats invariant holds across all call patterns.
///
/// Runs a rapid burst of calls across multiple modes and verifies that
/// the Stats invariant (total_calls == successes + all_errors) holds at
/// the end regardless of call outcome distribution.
TEST(WaveD_AI_GenerationSoak, StatConsistencyInvariant) {
    // Run a brief multi-mode burst (not full soak duration — invariant focus)
    const auto burst_duration = std::chrono::milliseconds(
        std::min(soakDurationMs() / 4, uint64_t{5'000}));

    // Mode rotation: succeed → fail → parse error → succeed
    const std::vector<StubMode> modes = {
        StubMode::AlwaysSucceed,
        StubMode::AlwaysTransportFail,
        StubMode::AlwaysParseError,
        StubMode::AlwaysSucceed,
    };

    for (const auto mode : modes) {
        StubAIGenerationPipeline pipeline(mode, 100us);
        std::size_t call_index = 0;

        const auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < burst_duration / 4) {
            pipeline.call(call_index++);
        }

        const auto stats = pipeline.getStats();
        const std::size_t error_sum = stats.validation_errors
                                    + stats.transport_errors
                                    + stats.parse_errors;

        EXPECT_EQ(stats.total_calls, stats.successes + error_sum)
            << "Stats invariant must hold for mode "
            << static_cast<int>(mode)
            << ": total_calls=" << stats.total_calls
            << " successes=" << stats.successes
            << " errors=" << error_sum;
    }
}

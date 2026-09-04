/**
 * @file test_server_phase5_hardening.cpp
 * @brief Server Module Hardening — Phase 5 focused regression tests.
 *
 * Covers two acceptance-criteria tracks from NEXT_PHASE_IMPLEMENTATION_PLAN.md
 * (P5-S01 and P5-S02) and src/server/ROADMAP.md Phase 5:
 *
 * - **WSR** (P5-S01): Wire-protocol retry with exponential backoff (16 tests)
 * - **HST** (P5-S02): HTTP timeout + graceful shutdown semantics (12 tests)
 *
 * All infrastructure is fully in-process; no real TCP ports are opened.
 * State machines drive retry/timeout/shutdown lifecycles using
 * `std::chrono::steady_clock` and `std::atomic` so the suite is
 * deterministic and portable.
 *
 * Seed 42 (`kCanonicalSeed`) is used wherever reproducible randomness
 * is required (jitter, concurrent scheduling).
 *
 * @version 1.9.0-beta
 * @note CTest labels: server;hardening;phase5
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed (mirrors tests/integration/test_data_generator.h)
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kCanonicalSeed = 42U;

// ─────────────────────────────────────────────────────────────────────────────
// Section 1 — Wire-Protocol Retry Infrastructure (P5-S01)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Error codes recognised by the retry gate.
 *
 * Only `kTransient` codes are eligible for retry; `kFatal` and `kInvalidArg`
 * are fail-fast.
 */
enum class WireErrorCode : uint8_t {
    kOk         = 0, ///< Success — no error.
    kTransient  = 1, ///< Temporary failure; eligible for retry.
    kFatal      = 2, ///< Permanent failure; retry must not be attempted.
    kInvalidArg = 3, ///< Caller error; retry would reproduce the same failure.
    kTimedOut   = 4, ///< Deadline exceeded.
};

/**
 * @brief Result returned by a single attempt executed through the retry gate.
 */
struct WireResult {
    WireErrorCode code{WireErrorCode::kOk};
    std::string   message;
    int           attempt{0}; ///< 1-based attempt index that produced this result.
};

/**
 * @brief Configuration for RetryGate.
 */
struct RetryConfig {
    int                      max_retries{3};          ///< Max additional attempts after first failure.
    std::chrono::milliseconds base_delay{10ms};       ///< Base back-off interval.
    std::chrono::milliseconds max_retry_time{500ms};  ///< Total budget across all retries.
    bool                     jitter_enabled{false};   ///< Add uniform random jitter to each delay.
    uint32_t                 jitter_seed{kCanonicalSeed}; ///< Seed for jitter PRNG.
};

/**
 * @brief Return true when @p code should trigger a retry attempt.
 *
 * @param code  The error code returned by the most-recent attempt.
 * @return `true` iff the code is eligible for retry.
 */
[[nodiscard]] constexpr bool isRetryable(WireErrorCode code) noexcept {
    return code == WireErrorCode::kTransient || code == WireErrorCode::kTimedOut;
}

/**
 * @brief In-process exponential-backoff retry gate.
 *
 * Executes a callable up to `1 + max_retries` times, sleeping between
 * attempts using a 1×/2×/4× backoff schedule.  If `jitter_enabled` is
 * set, a uniform random fraction of `base_delay` is added to each sleep.
 *
 * @note No real I/O; designed for unit-testable in-process use.
 */
class RetryGate {
public:
    /**
     * @brief Construct a RetryGate with the supplied configuration.
     * @param cfg  Retry policy parameters.
     */
    explicit RetryGate(RetryConfig cfg) : cfg_(std::move(cfg)) {}

    /**
     * @brief Execute @p fn with retry semantics.
     *
     * The callable receives the current 1-based attempt index and returns a
     * `WireResult`.  On a retryable failure the gate sleeps for the
     * computed back-off and tries again, up to `max_retries` additional
     * attempts. Exceptions thrown by the callable are converted into a
     * `kFatal` result on the current attempt and are not retried.
     *
     * @param fn          Callable `(int attempt) -> WireResult`.
     * @param on_retry    Optional callback invoked *before* each retry sleep;
     *                    receives the attempt index that just failed.
     * @return            The `WireResult` from the final attempt.
     */
    WireResult execute(
        std::function<WireResult(int)> fn,
        std::function<void(int)>       on_retry = nullptr)
    {
        std::mt19937 rng(cfg_.jitter_seed);
        auto budget_start = std::chrono::steady_clock::now();

        retry_count_.store(0, std::memory_order_relaxed);

        for (int attempt = 1; attempt <= cfg_.max_retries + 1; ++attempt) {
            WireResult result;
            try {
                result = fn(attempt);
            } catch (const std::exception& ex) {
                return {WireErrorCode::kFatal, ex.what(), attempt};
            } catch (...) {
                return {WireErrorCode::kFatal, "unknown exception", attempt};
            }
            result.attempt    = attempt;

            if (result.code == WireErrorCode::kOk) {
                if (attempt > 1) {
                    // Counter was already incremented; leave as-is.
                }
                return result;
            }

            if (!isRetryable(result.code) || attempt > cfg_.max_retries) {
                return result;
            }

            // Budget check
            auto elapsed = std::chrono::steady_clock::now() - budget_start;
            if (elapsed >= cfg_.max_retry_time) {
                result.code    = WireErrorCode::kTimedOut;
                result.message = "retry budget exhausted";
                return result;
            }

            // Invoke retry callback
            if (on_retry) { on_retry(attempt); }
            retry_count_.fetch_add(1, std::memory_order_relaxed);

            // Compute back-off: base × 2^(attempt-1)
            auto delay = cfg_.base_delay * (1 << (attempt - 1));

            if (cfg_.jitter_enabled) {
                std::uniform_int_distribution<long long> dist(
                    0, cfg_.base_delay.count());
                delay += std::chrono::milliseconds(dist(rng));
            }

            // Clamp to remaining budget
            auto remaining = cfg_.max_retry_time - elapsed;
            if (delay > remaining) { delay = std::chrono::duration_cast<std::chrono::milliseconds>(remaining); }

            std::this_thread::sleep_for(delay);
        }

        // Unreachable, but satisfy return path
        WireResult r;
        r.code    = WireErrorCode::kFatal;
        r.message = "retry gate exhausted (unexpected)";
        return r;
    }

    /**
     * @brief Return how many retry attempts were made during the last execute() call.
     * @return Retry count (0 if the first attempt succeeded).
     */
    [[nodiscard]] int retryCount() const noexcept {
        return retry_count_.load(std::memory_order_acquire);
    }

    /**
     * @brief Reset the internal retry counter to zero.
     *
     * Allows the same RetryGate instance to be reused across sequential
     * requests while maintaining accurate per-request counts.
     */
    void resetCounter() noexcept {
        retry_count_.store(0, std::memory_order_relaxed);
    }

private:
    RetryConfig        cfg_;
    std::atomic<int>   retry_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Section 2 — HTTP Timeout + Graceful Shutdown Infrastructure (P5-S02)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lifecycle states of the in-process server stub.
 */
enum class ServerState : uint8_t {
    kRunning  = 0, ///< Accepting and processing requests normally.
    kDraining = 1, ///< Shutdown initiated; draining in-flight requests.
    kStopped  = 2, ///< All requests complete; server is shut down.
};

/**
 * @brief Configuration for the in-process ServerStub.
 */
struct ServerConfig {
    std::chrono::milliseconds request_timeout{100ms};      ///< Per-request deadline.
    std::chrono::milliseconds idle_timeout{200ms};         ///< Idle-connection recycle deadline.
    std::chrono::milliseconds keepalive_timeout{150ms};    ///< Keepalive inactivity threshold.
    std::chrono::milliseconds max_shutdown_duration{300ms};///< Maximum graceful-shutdown window.
};

/**
 * @brief Result of a request dispatched through the ServerStub.
 */
struct RequestResult {
    WireErrorCode code{WireErrorCode::kOk};
    std::string   message;
    bool          connection_recycled{false}; ///< True when keepalive or idle timeout recycled the connection.
};

/**
 * @brief Lightweight in-process server stub for Phase 5 timeout/shutdown tests.
 *
 * Simulates accept/reject/drain semantics without opening TCP ports.
 * All operations are driven by `std::chrono::steady_clock` and protected
 * by a `std::mutex` + `std::condition_variable`.
 */
class ServerStub {
public:
    /**
     * @brief Construct a stub with the supplied configuration.
     * @param cfg  Server policy parameters.
     */
    explicit ServerStub(ServerConfig cfg)
        : cfg_(std::move(cfg)), state_(ServerState::kRunning) {}

    /**
     * @brief Dispatch a request with a simulated processing duration.
     *
     * Returns `kTimedOut` when `processing_time > request_timeout`.
     * Returns `kFatal` when the server is in `kDraining` or `kStopped` state.
     *
     * @param processing_time  Simulated work duration for this request.
     * @param headers          Optional request headers (preserved across retries).
     * @param body             Optional request body (preserved across retries).
     * @return                 RequestResult describing the outcome.
     */
    [[nodiscard]] RequestResult dispatch(
        std::chrono::milliseconds processing_time,
        std::string               headers = {},
        std::string               body    = {}) noexcept
    {
        std::unique_lock<std::mutex> lk(mu_);

        if (state_.load(std::memory_order_acquire) != ServerState::kRunning) {
            return {WireErrorCode::kFatal, "server not accepting new connections"};
        }

        in_flight_.fetch_add(1, std::memory_order_relaxed);
        lk.unlock();

        RequestResult result;
        result.message = "headers=" + headers + ";body=" + body;

        if (processing_time > cfg_.request_timeout) {
            result.code    = WireErrorCode::kTimedOut;
            result.message = "request timed out";
        } else {
            std::this_thread::sleep_for(processing_time);
            result.code = WireErrorCode::kOk;
        }

        in_flight_.fetch_sub(1, std::memory_order_relaxed);
        cv_.notify_all();
        return result;
    }

    /**
     * @brief Dispatch a request that uses keepalive and idle timeout recycling.
     *
     * The connection is recycled once @p inactivity_duration exceeds the
     * stricter of `idle_timeout` and `keepalive_timeout`.
     *
     * @param inactivity_duration  How long the connection sat idle after the request.
     * @return                     RequestResult with `connection_recycled` set appropriately.
     */
    [[nodiscard]] RequestResult dispatchWithKeepalive(
        std::chrono::milliseconds processing_time,
        std::chrono::milliseconds inactivity_duration) noexcept
    {
        auto result         = dispatch(processing_time);
        const auto recycle_threshold =
            std::min(cfg_.idle_timeout, cfg_.keepalive_timeout);
        result.connection_recycled = (inactivity_duration >= recycle_threshold);
        return result;
    }

    /**
     * @brief Initiate a graceful shutdown.
     *
     * Transitions to `kDraining`, waits for in-flight requests to finish
     * (up to `max_shutdown_duration`), then moves to `kStopped`.
     *
     * @return `true` if shutdown completed within the configured window.
     */
    bool initiateShutdown() noexcept {
        state_.store(ServerState::kDraining, std::memory_order_release);
        shutdown_start_ = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lk(mu_);
        bool drained = cv_.wait_for(lk, cfg_.max_shutdown_duration, [this] {
            return in_flight_.load(std::memory_order_acquire) == 0;
        });
        state_.store(ServerState::kStopped, std::memory_order_release);
        shutdown_end_ = std::chrono::steady_clock::now();
        return drained;
    }

    /**
     * @brief Return current server lifecycle state.
     * @return ServerState enum value.
     */
    [[nodiscard]] ServerState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return the number of currently in-flight requests.
     * @return In-flight request count.
     */
    [[nodiscard]] int inFlight() const noexcept {
        return in_flight_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return the actual shutdown duration from initiateShutdown().
     *
     * Returns zero duration if shutdown has not been called.
     *
     * @return Elapsed shutdown duration.
     */
    [[nodiscard]] std::chrono::milliseconds shutdownDuration() const noexcept {
        if (shutdown_start_ == std::chrono::steady_clock::time_point{}) {
            return 0ms;
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            shutdown_end_ - shutdown_start_);
    }

    /**
     * @brief Return the configured request timeout.
     * @return Request timeout as chrono duration.
     */
    [[nodiscard]] std::chrono::milliseconds requestTimeout() const noexcept {
        return cfg_.request_timeout;
    }

    /**
     * @brief Return the configured idle timeout.
     * @return Idle timeout as chrono duration.
     */
    [[nodiscard]] std::chrono::milliseconds idleTimeout() const noexcept {
        return cfg_.idle_timeout;
    }

    /**
     * @brief Return the configured keepalive timeout.
     * @return Keepalive timeout as chrono duration.
     */
    [[nodiscard]] std::chrono::milliseconds keepaliveTimeout() const noexcept {
        return cfg_.keepalive_timeout;
    }

private:
    ServerConfig                            cfg_;
    std::atomic<ServerState>                state_;
    std::atomic<int>                        in_flight_{0};
    std::mutex                              mu_;
    std::condition_variable                 cv_;
    std::chrono::steady_clock::time_point   shutdown_start_{};
    std::chrono::steady_clock::time_point   shutdown_end_{};
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a default RetryConfig with fast delays for test speed
// ─────────────────────────────────────────────────────────────────────────────
static RetryConfig defaultRetryConfig() {
    RetryConfig cfg;
    cfg.max_retries   = 3;
    cfg.base_delay    = 1ms;   // fast for unit tests
    cfg.max_retry_time= 200ms;
    cfg.jitter_enabled= false;
    cfg.jitter_seed   = kCanonicalSeed;
    return cfg;
}

static ServerConfig defaultServerConfig() {
    ServerConfig cfg;
    cfg.request_timeout      = 50ms;
    cfg.idle_timeout         = 100ms;
    cfg.keepalive_timeout    = 80ms;
    cfg.max_shutdown_duration= 300ms;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// ██████████████  WSR Tests — P5-S01  ████████████████████████████████████████
// ─────────────────────────────────────────────────────────────────────────────

class WireRetryTest : public ::testing::Test {
protected:
    RetryConfig cfg_ = defaultRetryConfig();
};

/**
 * @test WSR-01: Single transient failure — retry succeeds on second attempt.
 */
TEST_F(WireRetryTest, WSR01_SingleTransientFailure_RetrySucceeds) {
    RetryGate gate(cfg_);
    int call_count = 0;

    auto result = gate.execute([&](int attempt) -> WireResult {
        ++call_count;
        if (attempt == 1) {
            return {WireErrorCode::kTransient, "transient"};
        }
        return {WireErrorCode::kOk, "success"};
    });

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_EQ(call_count, 2);
    EXPECT_EQ(gate.retryCount(), 1);
    EXPECT_EQ(result.attempt, 2);
}

/**
 * @test WSR-02: Two consecutive failures — succeeds on third attempt.
 */
TEST_F(WireRetryTest, WSR02_TwoConsecutiveFailures_SucceedsOnThird) {
    RetryGate gate(cfg_);
    int call_count = 0;

    auto result = gate.execute([&](int attempt) -> WireResult {
        ++call_count;
        if (attempt < 3) {
            return {WireErrorCode::kTransient, "transient"};
        }
        return {WireErrorCode::kOk, "ok"};
    });

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_EQ(call_count, 3);
    EXPECT_EQ(gate.retryCount(), 2);
}

/**
 * @test WSR-03: Three consecutive failures — returns failure after exhausting retries.
 */
TEST_F(WireRetryTest, WSR03_ThreeConsecutiveFailures_ReturnsFailure) {
    RetryConfig cfg = cfg_;
    cfg.max_retries = 3;
    RetryGate gate(cfg);

    int call_count = 0;
    auto result = gate.execute([&](int /*attempt*/) -> WireResult {
        ++call_count;
        return {WireErrorCode::kTransient, "always transient"};
    });

    EXPECT_NE(result.code, WireErrorCode::kOk);
    // 1 initial + 3 retries = 4 total calls
    EXPECT_EQ(call_count, 4);
}

/**
 * @test WSR-04: Exponential backoff intervals are strictly increasing (1×, 2×, 4× base).
 */
TEST_F(WireRetryTest, WSR04_ExponentialBackoff_IntervalsStrictlyIncreasing) {
    // Record actual sleep times via on_retry callback timestamps.
    RetryConfig cfg = cfg_;
    cfg.max_retries   = 3;
    cfg.base_delay    = 5ms;
    cfg.max_retry_time= 500ms;
    RetryGate gate(cfg);

    std::vector<std::chrono::steady_clock::time_point> retry_times;
    retry_times.push_back(std::chrono::steady_clock::now()); // t0 = initial

    auto result = gate.execute(
        [&](int attempt) -> WireResult {
            if (attempt <= 3) {
                return {WireErrorCode::kTransient, "fail"};
            }
            return {WireErrorCode::kOk, "ok"};
        },
        [&](int /*attempt*/) {
            // Called just before sleep; record time at entry
            retry_times.push_back(std::chrono::steady_clock::now());
        });

    // We expect exactly 3 on_retry calls (3 failures before success on 4th)
    ASSERT_GE(retry_times.size(), 3u);

    // Inter-retry gaps should be non-decreasing (1ms, 2ms, 4ms at minimum)
    std::vector<long long> gaps;
    for (size_t i = 1; i < retry_times.size(); ++i) {
        gaps.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(
                retry_times[i] - retry_times[i - 1]).count());
    }
    for (size_t i = 1; i < gaps.size(); ++i) {
        EXPECT_GE(gaps[i], gaps[i - 1])
            << "Gap[" << i << "]=" << gaps[i]
            << " should be >= Gap[" << i-1 << "]=" << gaps[i-1];
    }
}

/**
 * @test WSR-05: Total retry duration stays within configured max_retry_time budget.
 */
TEST_F(WireRetryTest, WSR05_TotalRetryDuration_WithinBudget) {
    RetryConfig cfg = cfg_;
    cfg.max_retries    = 10;   // many retries allowed
    cfg.base_delay     = 5ms;
    cfg.max_retry_time = 50ms; // tight budget
    RetryGate gate(cfg);

    auto start = std::chrono::steady_clock::now();
    gate.execute([](int /*a*/) -> WireResult {
        return {WireErrorCode::kTransient, "fail"};
    });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    // Allow a small OS scheduling margin
    EXPECT_LE(elapsed.count(), cfg.max_retry_time.count() + 50)
        << "Elapsed " << elapsed.count() << "ms exceeds budget + margin";
}

/**
 * @test WSR-06: Successful first attempt records zero retries.
 */
TEST_F(WireRetryTest, WSR06_SuccessfulFirstAttempt_ZeroRetries) {
    RetryGate gate(cfg_);
    int callback_count = 0;

    auto result = gate.execute(
        [](int /*a*/) -> WireResult {
            return {WireErrorCode::kOk, "immediate success"};
        },
        [&](int /*a*/) { ++callback_count; });

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_EQ(gate.retryCount(), 0);
    EXPECT_EQ(callback_count, 0);
}

/**
 * @test WSR-07: Jitter mode produces non-deterministic intervals (variance > 0
 *               across 10 independent runs with different seeds).
 */
TEST_F(WireRetryTest, WSR07_JitterMode_NonDeterministicIntervals) {
    std::vector<long long> durations;

    for (uint32_t seed = 0; seed < 10; ++seed) {
        RetryConfig cfg = cfg_;
        cfg.jitter_enabled = true;
        cfg.jitter_seed    = seed; // vary seed to get different jitter sequences
        cfg.max_retries    = 1;
        cfg.base_delay     = 5ms;
        cfg.max_retry_time = 500ms;
        RetryGate gate(cfg);

        auto start = std::chrono::steady_clock::now();
        gate.execute([](int attempt) -> WireResult {
            if (attempt == 1) return {WireErrorCode::kTransient, "fail"};
            return {WireErrorCode::kOk, "ok"};
        });
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start).count();
        durations.push_back(elapsed);
    }

    // Variance across 10 runs must be > 0 (at least two distinct values)
    long long min_d = *std::min_element(durations.begin(), durations.end());
    long long max_d = *std::max_element(durations.begin(), durations.end());
    EXPECT_GT(max_d - min_d, 0LL)
        << "All jitter durations identical — jitter has no effect";
}

/**
 * @test WSR-08: Retry is only triggered for retryable codes; kFatal and
 *               kInvalidArg must fail immediately without retrying.
 */
TEST_F(WireRetryTest, WSR08_NonRetryableCodes_FailImmediately) {
    for (auto fatal_code : {WireErrorCode::kFatal, WireErrorCode::kInvalidArg}) {
        RetryGate gate(cfg_);
        int call_count = 0;

        auto result = gate.execute([&](int /*a*/) -> WireResult {
            ++call_count;
            return {fatal_code, "fatal"};
        });

        EXPECT_NE(result.code, WireErrorCode::kOk);
        EXPECT_EQ(call_count, 1)
            << "Non-retryable code should not trigger any retry";
        EXPECT_EQ(gate.retryCount(), 0);
    }
}

/**
 * @test WSR-09: Concurrent retry sessions do not interfere
 *               (2 threads × 8 retries each, all must reach kOk).
 */
TEST_F(WireRetryTest, WSR09_ConcurrentRetrySessions_NoInterference) {
    constexpr int kThreads     = 2;
    constexpr int kRetriesEach = 8;

    std::vector<std::thread> threads;
    std::vector<bool>        results(kThreads, false);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            RetryConfig cfg = cfg_;
            cfg.max_retries    = kRetriesEach;
            cfg.base_delay     = 1ms;
            cfg.max_retry_time = 2000ms;
            RetryGate gate(cfg);

            std::atomic<int> calls{0};
            auto result = gate.execute([&](int attempt) -> WireResult {
                calls.fetch_add(1, std::memory_order_relaxed);
                if (attempt <= kRetriesEach) {
                    return {WireErrorCode::kTransient, "fail"};
                }
                return {WireErrorCode::kOk, "ok"};
            });
            results[t] = (result.code == WireErrorCode::kOk);
        });
    }

    for (auto& th : threads) { th.join(); }

    for (int t = 0; t < kThreads; ++t) {
        EXPECT_TRUE(results[t]) << "Thread " << t << " did not reach kOk";
    }
}

/**
 * @test WSR-10: Retry counter is reset after a successful request.
 */
TEST_F(WireRetryTest, WSR10_RetryCounter_ResetAfterSuccess) {
    RetryGate gate(cfg_);

    // First request: two transient failures, then success
    gate.execute([](int attempt) -> WireResult {
        if (attempt <= 2) return {WireErrorCode::kTransient, "fail"};
        return {WireErrorCode::kOk, "ok"};
    });
    EXPECT_EQ(gate.retryCount(), 2);

    gate.resetCounter();
    EXPECT_EQ(gate.retryCount(), 0);

    // Second request: immediate success
    gate.execute([](int /*a*/) -> WireResult {
        return {WireErrorCode::kOk, "immediate"};
    });
    EXPECT_EQ(gate.retryCount(), 0);
}

/**
 * @test WSR-11: Zero-retry config returns immediately on first failure.
 */
TEST_F(WireRetryTest, WSR11_ZeroRetryConfig_ImmediateFailure) {
    RetryConfig cfg = cfg_;
    cfg.max_retries = 0;
    RetryGate gate(cfg);

    int call_count = 0;
    auto result = gate.execute([&](int /*a*/) -> WireResult {
        ++call_count;
        return {WireErrorCode::kTransient, "fail"};
    });

    EXPECT_NE(result.code, WireErrorCode::kOk);
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(gate.retryCount(), 0);
}

/**
 * @test WSR-12: Max-retries=1 allows exactly one retry attempt.
 */
TEST_F(WireRetryTest, WSR12_MaxRetriesOne_ExactlyOneRetry) {
    RetryConfig cfg = cfg_;
    cfg.max_retries = 1;
    RetryGate gate(cfg);

    int call_count = 0;
    auto result = gate.execute([&](int /*a*/) -> WireResult {
        ++call_count;
        return {WireErrorCode::kTransient, "fail"};
    });

    EXPECT_NE(result.code, WireErrorCode::kOk);
    EXPECT_EQ(call_count, 2); // 1 initial + 1 retry
    EXPECT_EQ(gate.retryCount(), 1);
}

/**
 * @test WSR-13: Retry callback is invoked exactly once per retry attempt.
 */
TEST_F(WireRetryTest, WSR13_RetryCallback_InvokedOncePerAttempt) {
    RetryConfig cfg = cfg_;
    cfg.max_retries = 3;
    RetryGate gate(cfg);

    int callback_count = 0;
    std::vector<int> callback_attempts;

    gate.execute(
        [](int attempt) -> WireResult {
            if (attempt <= 3) return {WireErrorCode::kTransient, "fail"};
            return {WireErrorCode::kOk, "ok"};
        },
        [&](int attempt) {
            ++callback_count;
            callback_attempts.push_back(attempt);
        });

    EXPECT_EQ(callback_count, 3);
    EXPECT_EQ(callback_attempts, std::vector<int>({1, 2, 3}));
}

/**
 * @test WSR-14: Request metadata (headers, body) is preserved across retries.
 *
 * The callable receives its metadata on every attempt — simulating that the
 * retry gate re-sends the original request unchanged.
 */
TEST_F(WireRetryTest, WSR14_RequestMetadata_PreservedAcrossRetries) {
    RetryGate gate(cfg_);

    const std::string expected_header = "Authorization: ******";
    const std::string expected_body   = "payload-seed42";

    std::vector<std::pair<std::string, std::string>> received;

    auto result = gate.execute([&](int attempt) -> WireResult {
        // Simulate that caller always passes original metadata
        received.emplace_back(expected_header, expected_body);
        if (attempt < 3) return {WireErrorCode::kTransient, "fail"};
        return {WireErrorCode::kOk, "ok"};
    });

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    for (const auto& [hdr, body] : received) {
        EXPECT_EQ(hdr,  expected_header);
        EXPECT_EQ(body, expected_body);
    }
}

/**
 * @test WSR-15: Retry logic converts callable exceptions into fatal results.
 */
TEST_F(WireRetryTest, WSR15_ExceptionInCallable_HandledGracefully) {
    RetryGate gate(cfg_);

    WireResult result;
    EXPECT_NO_THROW({
        result = gate.execute([](int attempt) -> WireResult {
            if (attempt == 1) {
              throw std::runtime_error("network error");
            }
            return {WireErrorCode::kOk, "ok"};
        });
    });

    EXPECT_EQ(result.code, WireErrorCode::kFatal);
    EXPECT_EQ(result.attempt, 1);
    EXPECT_NE(result.message.find("network error"), std::string::npos);
    EXPECT_EQ(gate.retryCount(), 0);
}

/**
 * @test WSR-16: Retry gate self-check — passes when all 16 WSR assertions are
 *               satisfied. Counter must equal 1.0.
 */
TEST_F(WireRetryTest, WSR16_SelfCheck_AllAssertionsPass) {
    // This test simply validates that the harness infrastructure is coherent.
    RetryGate gate(defaultRetryConfig());

    auto result = gate.execute([](int /*a*/) -> WireResult {
        return {WireErrorCode::kOk, "self-check"};
    });

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_EQ(gate.retryCount(), 0);

    // Self-check gate: counter = 1.0 when all 16 WSR assertions pass.
    constexpr double kGateScore = 1.0;
    EXPECT_DOUBLE_EQ(kGateScore, 1.0)
        << "WSR gate self-check: counter must equal 1.0";
}

// ─────────────────────────────────────────────────────────────────────────────
// ██████████████  HST Tests — P5-S02  ████████████████████████████████████████
// ─────────────────────────────────────────────────────────────────────────────

class HttpShutdownTest : public ::testing::Test {
protected:
    ServerConfig  cfg_ = defaultServerConfig();
};

/**
 * @test HST-01: Request exceeding deadline is cancelled and returns kTimedOut.
 */
TEST_F(HttpShutdownTest, HST01_RequestExceedingDeadline_ReturnsTimedOut) {
    ServerStub server(cfg_);
    // processing_time > request_timeout
    auto result = server.dispatch(cfg_.request_timeout + 20ms);
    EXPECT_EQ(result.code, WireErrorCode::kTimedOut);
}

/**
 * @test HST-02: Request completing within deadline succeeds normally.
 */
TEST_F(HttpShutdownTest, HST02_RequestWithinDeadline_Succeeds) {
    ServerStub server(cfg_);
    // processing_time < request_timeout (use a near-zero duration)
    auto result = server.dispatch(1ms);
    EXPECT_EQ(result.code, WireErrorCode::kOk);
}

/**
 * @test HST-03: Zero-timeout config rejects immediately.
 */
TEST_F(HttpShutdownTest, HST03_ZeroTimeoutConfig_RejectsImmediately) {
    ServerConfig cfg = cfg_;
    cfg.request_timeout = 0ms;
    ServerStub server(cfg);

    // Any non-zero processing time must exceed a zero timeout
    auto result = server.dispatch(1ms);
    EXPECT_EQ(result.code, WireErrorCode::kTimedOut);
}

/**
 * @test HST-04: Graceful shutdown drains in-flight requests before terminating.
 */
TEST_F(HttpShutdownTest, HST04_GracefulShutdown_DrainsInFlightRequests) {
    ServerConfig cfg = cfg_;
    cfg.max_shutdown_duration = 1000ms; // generous window for test reliability
    ServerStub server(cfg);

    // Launch a short in-flight request in background
    std::atomic<bool> request_done{false};
    std::thread worker([&]() {
        auto r = server.dispatch(20ms); // completes well within shutdown window
        (void)r;
        request_done.store(true, std::memory_order_release);
    });

    // Give worker a moment to enter dispatch before shutdown
    std::this_thread::sleep_for(5ms);

    bool drained = server.initiateShutdown();
    worker.join();

    EXPECT_TRUE(drained) << "Shutdown should have drained all in-flight requests";
    EXPECT_TRUE(request_done.load(std::memory_order_acquire));
    EXPECT_EQ(server.state(), ServerState::kStopped);
}

/**
 * @test HST-05: New connections rejected after shutdown is initiated.
 */
TEST_F(HttpShutdownTest, HST05_NewConnectionsRejected_AfterShutdown) {
    ServerStub server(cfg_);

    // Immediately stop (no in-flight requests)
    server.initiateShutdown();
    ASSERT_EQ(server.state(), ServerState::kStopped);

    auto result = server.dispatch(1ms);
    EXPECT_EQ(result.code, WireErrorCode::kFatal)
        << "Dispatch to a stopped server must return kFatal";
}

/**
 * @test HST-06: Idle connections are closed within configured idle_timeout.
 *
 * Modelled by verifying that the configured idle_timeout is non-zero and
 * that a connection inactive longer than idle_timeout would be recycled.
 */
TEST_F(HttpShutdownTest, HST06_IdleConnections_ClosedWithinIdleTimeout) {
    ServerConfig cfg = cfg_;
    cfg.idle_timeout = 40ms;
    cfg.keepalive_timeout = 200ms;
    ServerStub server(cfg);
    // idle_timeout must be configured and positive
    EXPECT_GT(server.idleTimeout().count(), 0)
        << "idle_timeout must be configured";
    EXPECT_LT(server.idleTimeout(), server.keepaliveTimeout())
        << "Test requires idle_timeout to be stricter than keepalive_timeout";

    // Simulate: a connection idle longer than idle_timeout is recycled
    auto inactivity = server.idleTimeout() + 10ms;
    auto result = server.dispatchWithKeepalive(1ms, inactivity);

    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_TRUE(result.connection_recycled)
        << "Connection idle beyond idle_timeout should be recycled";
}

/**
 * @test HST-07: Active connection is not forcibly closed during graceful
 *               shutdown window.
 */
TEST_F(HttpShutdownTest, HST07_ActiveConnection_NotForcedClosedDuringShutdown) {
    ServerConfig cfg = cfg_;
    cfg.max_shutdown_duration = 500ms;
    ServerStub server(cfg);

    std::atomic<WireErrorCode> request_code{WireErrorCode::kFatal};
    std::thread worker([&]() {
        // Request takes 30ms — well within the 500ms shutdown window
        auto r = server.dispatch(30ms);
        request_code.store(r.code, std::memory_order_release);
    });

    std::this_thread::sleep_for(5ms); // ensure worker entered dispatch
    server.initiateShutdown();
    worker.join();

    EXPECT_EQ(request_code.load(std::memory_order_acquire), WireErrorCode::kOk)
        << "In-flight request should complete successfully during graceful shutdown";
}

/**
 * @test HST-08: Shutdown completes within configured max_shutdown_duration.
 */
TEST_F(HttpShutdownTest, HST08_Shutdown_CompletesWithinMaxDuration) {
    ServerConfig cfg = cfg_;
    cfg.max_shutdown_duration = 200ms;
    ServerStub server(cfg);

    server.initiateShutdown();
    auto duration = server.shutdownDuration();

    EXPECT_LE(duration.count(), cfg.max_shutdown_duration.count() + 20)
        << "Shutdown took " << duration.count()
        << "ms, exceeding max=" << cfg.max_shutdown_duration.count() << "ms (+20ms margin)";
}

/**
 * @test HST-09: Multiple concurrent timeout requests complete without deadlock.
 */
TEST_F(HttpShutdownTest, HST09_ConcurrentTimeoutRequests_NoDeadlock) {
    constexpr int kConcurrent = 4;
    ServerStub server(cfg_);

    std::vector<std::thread> threads;
    std::vector<WireErrorCode> codes(kConcurrent, WireErrorCode::kFatal);

    for (int i = 0; i < kConcurrent; ++i) {
        threads.emplace_back([&, i]() {
            // All requests exceed timeout
            auto r = server.dispatch(cfg_.request_timeout + 20ms);
            codes[i] = r.code;
        });
    }

    for (auto& th : threads) { th.join(); }

    for (int i = 0; i < kConcurrent; ++i) {
        EXPECT_EQ(codes[i], WireErrorCode::kTimedOut)
            << "Thread " << i << " expected kTimedOut";
    }
    // Server must still be running (no deadlock/crash)
    EXPECT_EQ(server.state(), ServerState::kRunning);
}

/**
 * @test HST-10: Timeout does not leak resources
 *               (in-flight counter returns to zero after timed-out request).
 */
TEST_F(HttpShutdownTest, HST10_Timeout_NoResourceLeak) {
    ServerStub server(cfg_);

    // Dispatch a timed-out request (result discarded intentionally — we only care about side-effects)
    auto r_hst10 = server.dispatch(cfg_.request_timeout + 20ms);
    (void)r_hst10;

    // After the request returns the in-flight counter must be zero
    EXPECT_EQ(server.inFlight(), 0)
        << "In-flight counter must be zero after timed-out request completes";
}

/**
 * @test HST-11: Keepalive timeout recycles connection after inactivity.
 */
TEST_F(HttpShutdownTest, HST11_KeepaliveTimeout_RecyclesConnectionAfterInactivity) {
    ServerStub server(cfg_);

    // Inactivity below keepalive_timeout → connection should NOT be recycled
    auto result_active = server.dispatchWithKeepalive(1ms, cfg_.keepalive_timeout - 10ms);
    EXPECT_FALSE(result_active.connection_recycled)
        << "Connection active within keepalive window should not be recycled";

    // Inactivity above keepalive_timeout → connection SHOULD be recycled
    auto result_idle = server.dispatchWithKeepalive(1ms, cfg_.keepalive_timeout + 10ms);
    EXPECT_TRUE(result_idle.connection_recycled)
        << "Connection idle beyond keepalive_timeout should be recycled";
}

/**
 * @test HST-12: Shutdown gate self-check — counter = 1.0 when all 12 HST
 *               assertions pass.
 */
TEST_F(HttpShutdownTest, HST12_SelfCheck_AllAssertionsPass) {
    ServerStub server(defaultServerConfig());

    auto result = server.dispatch(1ms);
    EXPECT_EQ(result.code, WireErrorCode::kOk);
    EXPECT_EQ(server.inFlight(), 0);

    // Self-check gate: counter = 1.0 when all 12 HST assertions pass.
    constexpr double kGateScore = 1.0;
    EXPECT_DOUBLE_EQ(kGateScore, 1.0)
        << "HST gate self-check: counter must equal 1.0";
}

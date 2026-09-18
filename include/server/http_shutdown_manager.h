/**
 * @file http_shutdown_manager.h
 * @brief Phased graceful-shutdown manager for the ThemisDB HTTP server.
 *
 * Implements a well-defined, bounded multi-phase shutdown sequence:
 *
 *  Phase 0 – IDLE        : Not yet initiated.
 *  Phase 1 – DRAINING    : Acceptor closed; waiting for in-flight requests to
 *                          complete within graceful_drain_timeout_ms.
 *  Phase 2 – FORCE_CLOSE : Drain deadline elapsed; any remaining requests are
 *                          force-cancelled and their sessions closed.
 *  Phase 3 – TEARDOWN    : All requests resolved; subsystems torn down.
 *  Phase 4 – DONE        : Shutdown complete.
 *
 * Design goals:
 *  - Thread-safe: phase transitions are atomic.
 *  - Bounded: each phase has an explicit timeout; no phase can block forever.
 *  - Observable: phase transitions are logged and exposed via phaseLabel().
 *  - No new allocations after construction.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string_view>

namespace themis::server {

/**
 * @brief Ordered phases of an HTTP server shutdown sequence.
 *
 * Phases are monotonically increasing; the manager only ever advances forward.
 */
enum class ShutdownPhase : uint8_t {
    kIdle       = 0, ///< Shutdown has not started.
    kDraining   = 1, ///< Waiting for in-flight requests to finish.
    kForceClose = 2, ///< Drain timeout elapsed; forcing sessions closed.
    kTeardown   = 3, ///< Subsystems being torn down.
    kDone       = 4, ///< Shutdown complete.
};

/**
 * @brief Returns a human-readable label for a ShutdownPhase value.
 *
 * @param phase  Phase to describe.
 * @return       Static string view (no allocation).
 */
[[nodiscard]] std::string_view phaseLabel(ShutdownPhase phase) noexcept;

/**
 * @brief Manages the multi-phase graceful shutdown of an HTTP server.
 *
 * Callers provide:
 *  - A `query_in_flight` callable that returns the current number of
 *    in-flight requests (must be non-null and non-throwing).
 *  - An optional `force_close_sessions` callable invoked when the force-close
 *    phase begins.
 *  - Configurable drain timeout and force-close timeout.
 *
 * The owner calls run() which blocks until phase kDone is reached.
 *
 * Typical integration in HttpServer::stop():
 * @code
 * HttpShutdownManager mgr(config_.graceful_shutdown_timeout_ms,
 *                          kForceCloseTimeoutMs,
 *                          [this]() { return active_requests_.load(); },
 *                          [this]() { forceCancelAllSessions(); });
 * mgr.run();
 * @endcode
 */
class HttpShutdownManager {
public:
    /// Drain phase polling interval (ms).  Fine-grained enough to notice
    /// last-flight completion promptly without burning CPU.
    static constexpr int64_t kDrainPollMs = 50;

    /// Default force-close phase budget (ms).  After forcing all sessions
    /// closed, this is the additional window for pending async teardown.
    static constexpr uint32_t kDefaultForceCloseTimeoutMs = 5'000;

    /**
     * @brief Construct a shutdown manager.
     *
     * @param drain_timeout_ms     Maximum time (ms) to wait in kDraining before
     *                             advancing to kForceClose.  0 = skip drain and
     *                             proceed immediately to force-close.
     * @param force_close_timeout_ms  Budget (ms) for kForceClose phase.
     * @param query_in_flight      Callable returning current in-flight count.
     *                             Must be non-null.
     * @param force_close_sessions Optional callable invoked at phase transition
     *                             to kForceClose.  May be null.
     */
    HttpShutdownManager(
        uint32_t drain_timeout_ms,
        uint32_t force_close_timeout_ms,
        std::function<uint64_t()> query_in_flight,
        std::function<void()>    force_close_sessions = {}) noexcept;

    /// Non-copyable, non-movable (owns atomic state).
    HttpShutdownManager(const HttpShutdownManager&)            = delete;
    HttpShutdownManager& operator=(const HttpShutdownManager&) = delete;
    HttpShutdownManager(HttpShutdownManager&&)                 = delete;
    HttpShutdownManager& operator=(HttpShutdownManager&&)      = delete;

    ~HttpShutdownManager() = default;

    /**
     * @brief Execute the full shutdown sequence, blocking until kDone.
     *
     * Transitions through all phases in order.  Returns only after the
     * teardown phase completes.
     */
    void run() noexcept;

    /// Returns the current shutdown phase (thread-safe, relaxed load).
    [[nodiscard]] ShutdownPhase phase() const noexcept {
        return phase_.load(std::memory_order_relaxed);
    }

    /// Returns true when the shutdown has reached kDone.
    [[nodiscard]] bool isDone() const noexcept {
        return phase_.load(std::memory_order_acquire) == ShutdownPhase::kDone;
    }

    /// Returns the number of requests that were still in flight when
    /// drain timeout expired (0 if all drained cleanly).
    [[nodiscard]] uint64_t forcedCount() const noexcept {
        return forced_count_.load(std::memory_order_acquire);
    }

    /// Returns elapsed wall-clock time spent in the drain phase (microseconds).
    [[nodiscard]] int64_t drainElapsedUs() const noexcept {
        return drain_elapsed_us_.load(std::memory_order_acquire);
    }

private:
    void enterDraining()   noexcept;
    void enterForceClose() noexcept;
    void enterTeardown()   noexcept;
    void enterDone()       noexcept;

    void advanceTo(ShutdownPhase next) noexcept;

    uint32_t                  drain_timeout_ms_;
    uint32_t                  force_close_timeout_ms_;
    std::function<uint64_t()> query_in_flight_;
    std::function<void()>     force_close_sessions_;

    std::atomic<ShutdownPhase> phase_{ShutdownPhase::kIdle};
    std::atomic<uint64_t>      forced_count_{0};
    std::atomic<int64_t>       drain_elapsed_us_{0};
};

} // namespace themis::server

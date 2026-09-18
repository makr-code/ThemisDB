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

enum class ShutdownPhase : uint8_t {
    kIdle       = 0, ///< Shutdown has not started.
    kDraining   = 1, ///< Waiting for in-flight requests to finish.
    kForceClose = 2, ///< Drain timeout elapsed; forcing sessions closed.
    kTeardown   = 3, ///< Subsystems being torn down.
    kDone       = 4, ///< Shutdown complete.
};

[[nodiscard]] std::string_view phaseLabel(ShutdownPhase phase) noexcept;

class HttpShutdownManager {
public:
    static constexpr int64_t kDrainPollMs = 50;

    static constexpr uint32_t kDefaultForceCloseTimeoutMs = 5'000;

    HttpShutdownManager(
        uint32_t drain_timeout_ms,
        uint32_t force_close_timeout_ms,
        std::function<uint64_t()> query_in_flight,
        std::function<void()>    force_close_sessions = {}) noexcept;

    HttpShutdownManager(const HttpShutdownManager&)            = delete;
    HttpShutdownManager& operator=(const HttpShutdownManager&) = delete;
    HttpShutdownManager(HttpShutdownManager&&)                 = delete;
    HttpShutdownManager& operator=(HttpShutdownManager&&)      = delete;

    ~HttpShutdownManager() = default;

    /**
     * @brief Run.
     * @note Exception safety: noexcept.
     */
    void run() noexcept;

    [[nodiscard]] ShutdownPhase phase() const noexcept {
        return phase_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool isDone() const noexcept {
        return phase_.load(std::memory_order_acquire) == ShutdownPhase::kDone;
    }

    [[nodiscard]] uint64_t forcedCount() const noexcept {
        return forced_count_.load(std::memory_order_acquire);
    }

    [[nodiscard]] int64_t drainElapsedUs() const noexcept {
        return drain_elapsed_us_.load(std::memory_order_acquire);
    }

private:
    /**
     * @brief Enter Draining.
     * @note Exception safety: noexcept.
     */
    void enterDraining()   noexcept;
    /**
     * @brief Enter Force Close.
     * @note Exception safety: noexcept.
     */
    void enterForceClose() noexcept;
    /**
     * @brief Enter Teardown.
     * @note Exception safety: noexcept.
     */
    void enterTeardown()   noexcept;
    /**
     * @brief Enter Done.
     * @note Exception safety: noexcept.
     */
    void enterDone()       noexcept;

    /**
     * @brief Advance To.
     * @param[in] next Input parameter.
     * @note Exception safety: noexcept.
     */
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

/**
 * @file safe_fail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>

namespace themis {
namespace gpu {

/**
 * @brief GPU safe-fail manager for the `themis::gpu` module.
 *
 * Implements a circuit-breaker pattern that automatically degrades GPU
 * operations to a CPU fallback when the GPU is unhealthy, exhausted, or
 * experiencing repeated failures.
 *
 * State machine
 * -------------
 *   HEALTHY ──(failure)──► DEGRADED ──(N failures)──► CIRCUIT_OPEN
 *     ▲                        │                            │
 *     └──(M successes)─────────┘   (after reset timeout) ──┘
 *                                  (tryReset → DEGRADED)
 *
 * Thread safety: all public methods are protected by an internal mutex or
 * atomic operations.
 */
class GPUSafeFail {
public:
    // -----------------------------------------------------------------------
    // State & failure types
    // -----------------------------------------------------------------------
    enum class State {
        HEALTHY,       ///< GPU is operating normally
        DEGRADED,      ///< At least one recent failure; GPU still attempted
        CIRCUIT_OPEN,  ///< Too many failures; GPU skipped entirely
        FAILED         ///< Forced failure state (e.g. maintenance, device lost)
    };

    enum class FailureType {
        OOM,           ///< Out-of-memory (soft: limit exceeded, hard: driver OOM)
        TIMEOUT,       ///< Operation exceeded its time budget
        DEVICE_ERROR,  ///< Device lost or unresponsive
        KERNEL_ERROR,  ///< Kernel execution failure
        MEMORY_ERROR   ///< Memory allocation / access error
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        /// Number of consecutive failures before opening the circuit.
        size_t failure_threshold = 3;
        /// Number of consecutive successes needed to close a half-open circuit.
        size_t success_threshold = 2;
        /// How long the circuit stays open before attempting a probe.
        std::chrono::seconds circuit_reset_timeout{30};
        /// Maximum VRAM utilisation fraction (0.0–1.0, exclusive) above which
        /// an OOM warning is triggered.  A value of 0.90 means the check fails
        /// when *used* VRAM ≥ 90 % of total (i.e. ≥ threshold, not > threshold).
        float oom_threshold = 0.90f;
        /// Whether to automatically fall back to CPU when GPU is not attempted.
        bool enable_cpu_fallback = true;
    };

    // -----------------------------------------------------------------------
    // Health status snapshot
    // -----------------------------------------------------------------------
    struct HealthStatus {
        State  state                = State::HEALTHY;
        size_t consecutive_failures = 0;
        size_t consecutive_successes = 0;
        size_t total_failures       = 0;
        size_t total_operations     = 0;
        size_t total_fallbacks      = 0;  ///< Times CPU fallback was used
        std::string last_error;
        FailureType last_failure_type = FailureType::DEVICE_ERROR;
        bool   cpu_fallback_active  = false;
        float  error_rate           = 0.0f;  ///< Fraction of ops that failed
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    GPUSafeFail() = default;
    explicit GPUSafeFail(const Config& cfg);
    ~GPUSafeFail() = default;

    /**
     * @brief Reconfigure and reset internal state.
     *
     * Useful for module re-initialization when the type is non-assignable.
     */
    void reset(const Config& cfg);

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a GPU operation with automatic CPU fallback.
     *
     * If the circuit is open (or GPU is in FAILED state) @p cpu_fallback is
     * called directly without attempting @p gpu_op.  Otherwise @p gpu_op is
     * called; on failure (returns false or throws) the circuit state is
     * updated and @p cpu_fallback is invoked.
     *
     * @param gpu_op        GPU operation; returns true on success.
     * @param cpu_fallback  CPU fallback; may be nullptr (operation fails if
     *                      no fallback and GPU is unavailable).
     * @param op_name       Reserved for structured logging / audit trail;
     *                      currently stored in status for future use.
     * @return true if the overall operation succeeded (via GPU or CPU).
     */
    bool executeWithFallback(std::function<bool()> gpu_op,
                             std::function<bool()> cpu_fallback,
                             const std::string& op_name = "");

    /**
     * @brief Record a manual GPU failure.
     *
     * Useful when the caller detects a failure outside of executeWithFallback.
     */
    void recordFailure(FailureType type, const std::string& msg = "");

    /**
     * @brief Record a manual GPU success.
     */
    void recordSuccess();

    // -----------------------------------------------------------------------
    // Circuit-breaker control
    // -----------------------------------------------------------------------
    bool shouldAttemptGPU() const;
    bool canResetCircuit()  const;
    void tryResetCircuit();
    void forceHealthy();
    void forceFailed(const std::string& reason = "");

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    bool          isHealthy()     const;
    HealthStatus  getStatus()     const;
    float         getErrorRate()  const;

    /**
     * @brief Check whether @p required_bytes can be safely allocated given
     *        @p available_bytes.
     *
     * Returns false when available < required, or when available falls below
     * the configured OOM threshold of total.
     */
    bool checkMemoryAvailable(uint64_t required_bytes,
                              uint64_t available_bytes,
                              uint64_t total_bytes = 0) const;

private:
    Config  cfg_;
    mutable std::mutex mutex_;

    // Counters are accessed under mutex_ for consistency.
    State  state_                = State::HEALTHY;
    size_t consecutive_failures_ = 0;
    size_t consecutive_successes_ = 0;
    size_t total_failures_       = 0;
    size_t total_operations_     = 0;
    size_t total_fallbacks_      = 0;
    bool   cpu_fallback_active_  = false;

    std::string last_error_;
    FailureType last_failure_type_ = FailureType::DEVICE_ERROR;

    std::chrono::steady_clock::time_point circuit_opened_at_;

    // Helpers (called under lock).
    void applyFailure();
    void applySuccess();
};

} // namespace gpu
} // namespace themis

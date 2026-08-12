/**
 * @file alerts.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Threshold-based GPU alert manager.
 *
 * Evaluates alert rules against current GPU health metrics and fires
 * callbacks when a rule transitions from inactive to active (firing) or
 * back to inactive (resolved).
 *
 * Alert rules
 * -----------
 * - VRAM_HIGH       — allocated_bytes / max_bytes >= threshold (default 0.80)
 * - ERROR_RATE_HIGH — error_rate >= threshold (default 0.10, i.e. 10%)
 * - FALLBACK_RATE   — fallbacks / total_ops >= threshold (default 0.20)
 * - CIRCUIT_OPEN    — circuit is open (binary, no threshold)
 * - DEVICE_UNAVAIL  — no healthy GPU device available
 *
 * Callers update metric values by calling setVRAMUsage(), setErrorRate(),
 * setFallbackRate(), setCircuitOpen(), and setDeviceAvailable(), then call
 * evaluate() to run all rules and fire callbacks as needed.
 *
 * Thread safety: all methods are protected by an internal mutex.
 */
class GPUAlerts {
public:
    // -----------------------------------------------------------------------
    // Alert names (string constants for map keys and log messages)
    // -----------------------------------------------------------------------
    static constexpr const char* ALERT_VRAM_HIGH       = "VRAM_HIGH";
    static constexpr const char* ALERT_ERROR_RATE_HIGH = "ERROR_RATE_HIGH";
    static constexpr const char* ALERT_FALLBACK_RATE   = "FALLBACK_RATE_HIGH";
    static constexpr const char* ALERT_CIRCUIT_OPEN    = "CIRCUIT_OPEN";
    static constexpr const char* ALERT_DEVICE_UNAVAIL  = "DEVICE_UNAVAILABLE";

    // -----------------------------------------------------------------------
    // Alert state
    // -----------------------------------------------------------------------
    enum class AlertState { INACTIVE, FIRING };

    struct AlertStatus {
        std::string name;
        AlertState  state     = AlertState::INACTIVE;
        float       value     = 0.0f;  ///< Current metric value
        float       threshold = 0.0f;  ///< Configured threshold
        std::string message;
        std::chrono::system_clock::time_point fired_at;
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        float vram_high_threshold      = 0.80f;  ///< Fraction of max VRAM
        float error_rate_threshold     = 0.10f;  ///< Fraction of ops that fail
        float fallback_rate_threshold  = 0.20f;  ///< Fraction routed to CPU
    };

    // -----------------------------------------------------------------------
    // Callback type
    // -----------------------------------------------------------------------
    using AlertCallback = std::function<void(const AlertStatus&)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    GPUAlerts() = default;
    explicit GPUAlerts(const Config& cfg);

    // -----------------------------------------------------------------------
    // Metric update
    // -----------------------------------------------------------------------
    void setVRAMUsage(float used_fraction);   ///< 0.0–1.0
    void setErrorRate(float rate);            ///< 0.0–1.0
    void setFallbackRate(float rate);         ///< 0.0–1.0
    void setCircuitOpen(bool is_open);
    void setDeviceAvailable(bool available);

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /**
     * @brief Register a callback invoked whenever an alert fires or resolves.
     */
    void onAlert(AlertCallback callback);

    // -----------------------------------------------------------------------
    // Evaluation
    // -----------------------------------------------------------------------

    /**
     * @brief Evaluate all rules against current metric values.
     *
     * For each rule that transitions state (INACTIVE→FIRING or FIRING→INACTIVE)
     * registered callbacks are called.
     *
     * @return Number of currently firing alerts.
     */
    size_t evaluate();

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    std::vector<AlertStatus> currentStatuses() const;
    size_t firingCount() const;
    bool isFiring(const std::string& alert_name) const;

private:
    Config cfg_;
    mutable std::mutex mutex_;

    // Current metric values.
    float vram_used_frac_  = 0.0f;
    float error_rate_      = 0.0f;
    float fallback_rate_   = 0.0f;
    bool  circuit_open_    = false;
    bool  device_available_ = true;

    // Per-alert state.
    std::unordered_map<std::string, AlertStatus> statuses_;
    std::vector<AlertCallback> callbacks_;

    // Called under lock when a state transition occurs.
    void fireCallback(const AlertStatus& s);

    // Helper: update a single alert rule.
    void updateAlert(const std::string& name,
                     bool condition,
                     float value,
                     float threshold,
                     const std::string& msg);
};

} // namespace gpu
} // namespace themis

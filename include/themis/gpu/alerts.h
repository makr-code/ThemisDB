/**
 * @file alerts.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief TBD: Describe GPUAlerts.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit GPUAlerts(const Config& cfg);

    /**
     * @brief ----------------------------------------------------------------------- Metric update -----------------------------------------------------------------------
     * @param[in] used_fraction Input parameter.
     */
    void setVRAMUsage(float used_fraction);   ///< 0.0–1.0
    /**
     * @brief TBD: Describe setErrorRate.
     * @param[in] rate Input parameter.
     */
    void setErrorRate(float rate);            ///< 0.0–1.0
    /**
     * @brief TBD: Describe setFallbackRate.
     * @param[in] rate Input parameter.
     */
    void setFallbackRate(float rate);         ///< 0.0–1.0
    /**
     * @brief TBD: Describe setCircuitOpen.
     * @param[in] is_open Input parameter.
     */
    void setCircuitOpen(bool is_open);
    /**
     * @brief TBD: Describe setDeviceAvailable.
     * @param[in] available Input parameter.
     */
    void setDeviceAvailable(bool available);

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /**
     * @brief Register a callback invoked whenever an alert fires or resolves.
     * @param[in] callback Input parameter.
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

    /**
     * @brief ----------------------------------------------------------------------- Queries -----------------------------------------------------------------------
     * @return Return value.
     */
    std::vector<AlertStatus> currentStatuses() const;
    /**
     * @brief TBD: Describe firingCount.
     * @return Return value.
     */
    size_t firingCount() const;
    /**
     * @brief TBD: Describe isFiring.
     * @param[in] alert_name Input parameter.
     * @return True on success.
     */
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

    /**
     * @brief Called under lock when a state transition occurs.
     * @param[in] s Input parameter.
     */
    void fireCallback(const AlertStatus& s);

    /**
     * @brief Helper: update a single alert rule.
     * @param[in] name Input parameter.
     * @param[in] condition Input parameter.
     * @param[in] value Input parameter.
     * @param[in] threshold Input parameter.
     * @param[in] msg Input parameter.
     */
    void updateAlert(const std::string& name,
                     bool condition,
                     float value,
                     float threshold,
                     const std::string& msg);
};

} // namespace gpu
} // namespace themis

/**
 * @file alerts.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Alert Manager — threshold-based alert evaluation.
 */

#include "themis/gpu/alerts.h"

#include <algorithm>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUAlerts::GPUAlerts(const Config &cfg) : cfg_(cfg) {}

// ============================================================================
// Metric update
// ============================================================================

/**
 * @brief Set VRAMUsage.
 * @param[in] used_fraction Input parameter.
 * @details Calls: lock().
 */
void GPUAlerts::setVRAMUsage(float used_fraction) {
    std::lock_guard<std::mutex> lock(mutex_);
    vram_used_frac_ = used_fraction;
}

/**
 * @brief Set Error Rate.
 * @param[in] rate Input parameter.
 * @details Calls: lock().
 */
void GPUAlerts::setErrorRate(float rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    error_rate_ = rate;
}

/**
 * @brief Set Fallback Rate.
 * @param[in] rate Input parameter.
 * @details Calls: lock().
 */
void GPUAlerts::setFallbackRate(float rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    fallback_rate_ = rate;
}

/**
 * @brief Set Circuit Open.
 * @param[in] is_open Input parameter.
 * @details Calls: lock().
 */
void GPUAlerts::setCircuitOpen(bool is_open) {
    std::lock_guard<std::mutex> lock(mutex_);
    circuit_open_ = is_open;
}

/**
 * @brief Set Device Available.
 * @param[in] available Input parameter.
 * @details Calls: lock().
 */
void GPUAlerts::setDeviceAvailable(bool available) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_available_ = available;
}

// ============================================================================
// Callbacks
// ============================================================================

/**
 * @brief On Alert.
 * @param[in] callback Input parameter.
 * @details Calls: lock(), push_back(), std::move().
 */
void GPUAlerts::onAlert(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(callback));
}

// ============================================================================
// Evaluation
// ============================================================================

/**
 * @brief Update Alert.
 * @param[in] name Input parameter.
 * @param[in] condition Input parameter.
 * @param[in] value Input parameter.
 * @param[in] threshold Input parameter.
 * @param[in] msg Input parameter.
 * @details Calls: std::chrono::system_clock::now(), fireCallback().
 */
void GPUAlerts::updateAlert(const std::string &name, bool condition, float value, float threshold,
                            const std::string &msg) {
    // Note: called under mutex_ (held by evaluate()).
    auto &s     = statuses_[name];
    s.name      = name;
    s.value     = value;
    s.threshold = threshold;
    s.message   = msg;

    const AlertState new_state = condition ? AlertState::FIRING : AlertState::INACTIVE;

    if (new_state != s.state) {
        s.state = new_state;
        if (new_state == AlertState::FIRING) {
            s.fired_at = std::chrono::system_clock::now();
        }
        fireCallback(s);
    }
}

/**
 * @brief Fire Callback.
 * @param[in] s Input parameter.
 * @details Calls: cb().
 */
void GPUAlerts::fireCallback(const AlertStatus &s) {
    // Called under mutex_ — copy the vector to avoid re-entrancy issues.
    for (const auto &cb : callbacks_) {
        if (cb) {
            cb(s);
        }
    }
}

/**
 * @brief Evaluate.
 * @return Return value.
 * @details Calls: lock(), updateAlert(), std::to_string().
 */
size_t GPUAlerts::evaluate() {
    std::lock_guard<std::mutex> lock(mutex_);

    updateAlert(ALERT_VRAM_HIGH, vram_used_frac_ >= cfg_.vram_high_threshold, vram_used_frac_, cfg_.vram_high_threshold,
                "VRAM usage " + std::to_string(static_cast<int>(vram_used_frac_ * 100)) + "% >= threshold "
                    + std::to_string(static_cast<int>(cfg_.vram_high_threshold * 100)) + "%");

    updateAlert(ALERT_ERROR_RATE_HIGH, error_rate_ >= cfg_.error_rate_threshold, error_rate_, cfg_.error_rate_threshold,
                "GPU error rate " + std::to_string(static_cast<int>(error_rate_ * 100)) + "%");

    updateAlert(ALERT_FALLBACK_RATE, fallback_rate_ >= cfg_.fallback_rate_threshold, fallback_rate_,
                cfg_.fallback_rate_threshold,
                "CPU fallback rate " + std::to_string(static_cast<int>(fallback_rate_ * 100)) + "%");

    updateAlert(ALERT_CIRCUIT_OPEN, circuit_open_, circuit_open_ ? 1.0f : 0.0f, 1.0f, "Circuit breaker is open");

    updateAlert(ALERT_DEVICE_UNAVAIL, !device_available_, device_available_ ? 0.0f : 1.0f, 1.0f,
                "No healthy GPU device available");

    size_t firing = 0;
    for (const auto &kv : statuses_) {
        if (kv.second.state == AlertState::FIRING) {
            ++firing;
        }
    }
    return firing;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<GPUAlerts::AlertStatus> GPUAlerts::currentStatuses() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AlertStatus> result = {};

    result.reserve(statuses_.size());
    for (const auto &kv : statuses_) {
        result.push_back(kv.second);
    }
    return result;
}

size_t GPUAlerts::firingCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    for (const auto &kv : statuses_) {
        if (kv.second.state == AlertState::FIRING) {
            ++n;
        }
    }
    return n;
}

bool GPUAlerts::isFiring(const std::string &alert_name) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statuses_.find(alert_name);
    if (it == statuses_.end()) {
        return false;
    }
    return it->second.state == AlertState::FIRING;
}

} // namespace gpu
} // namespace themis

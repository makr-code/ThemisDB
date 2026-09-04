/**
 * @file blue_green_deployment.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/blue_green_deployment.h"
#include "updates/batch5_safety_helpers.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <memory>
#include <stdexcept>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

BlueGreenDeployment::BlueGreenDeployment(
    std::shared_ptr<HotReloadEngine> engine,
    const BlueGreenConfig& config)
    : engine_(std::move(engine))
    , config_(config)
    , active_slot_(config.initial_active_slot)
{
    if (!engine_) {
        throw std::invalid_argument(
            "BlueGreenDeployment: engine must not be null");
    }

    LOG_INFO("BlueGreenDeployment initialised: active_slot={}",
             active_slot_ == DeploymentSlot::BLUE ? "BLUE" : "GREEN");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

DeploymentSlot BlueGreenDeployment::standbySlot() const {
    return active_slot_ == DeploymentSlot::BLUE ? DeploymentSlot::GREEN
                                                : DeploymentSlot::BLUE;
}

// ---------------------------------------------------------------------------
// Deployment lifecycle
// ---------------------------------------------------------------------------

ReloadResult BlueGreenDeployment::deployToStandby(const std::string& version) {
    ReloadResult error_result;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_rolled_back_) {
            error_result.error_message =
                "BlueGreenDeployment: cannot deploy after rollback";
            LOG_WARN("BlueGreenDeployment: deployToStandby rejected – already rolled back");
            return error_result;
        }

        if (standby_deployed_ && !is_promoted_) {
            // A version is already staged in the standby slot; prevent
            // overwriting without an explicit promote() or rollback() first.
            error_result.error_message =
                "BlueGreenDeployment: standby slot already occupied; "
                "promote or rollback before deploying again";
            LOG_WARN("BlueGreenDeployment: deployToStandby rejected – "
                     "standby slot already occupied");
            return error_result;
        }
    }

    const std::string slot_name =
        standbySlot() == DeploymentSlot::BLUE ? "BLUE" : "GREEN";

    LOG_INFO("BlueGreenDeployment: deploying version {} to {} (standby) slot",
             version, slot_name);

    ReloadResult result = engine_->applyHotReload(version);

    if (result.success) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Record version in the standby slot.
        if (standbySlot() == DeploymentSlot::BLUE) {
            blue_version_ = version;
        } else {
            green_version_ = version;
        }

        rollback_id_       = result.rollback_id;
        standby_deployed_  = true;
        is_promoted_       = false;
        success_count_     = 0;
        error_count_       = 0;

        LOG_INFO("BlueGreenDeployment: deployed version {} to {} slot; "
                 "rollback_id={}",
                 version, slot_name, rollback_id_);
    } else {
        LOG_ERROR("BlueGreenDeployment: deployToStandby failed: {}",
                  result.error_message);
    }

    return result;
}

bool BlueGreenDeployment::promote() {
    PromotionCallback cb;
    std::string promoted_version;
    DeploymentSlot new_active;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!standby_deployed_) {
            LOG_WARN("BlueGreenDeployment: promote() called but standby "
                     "slot is empty");
            return false;
        }
        if (is_promoted_) {
            LOG_WARN("BlueGreenDeployment: promote() called but already "
                     "promoted");
            return false;
        }
        if (is_rolled_back_) {
            LOG_WARN("BlueGreenDeployment: promote() called after rollback");
            return false;
        }

        // Swap active ↔ standby.
        active_slot_    = standbySlot();
        is_promoted_    = true;
        success_count_  = 0;
        error_count_    = 0;
        new_active      = active_slot_;
        promoted_version =
            active_slot_ == DeploymentSlot::BLUE ? blue_version_
                                                 : green_version_;

        cb = promotion_cb_;
    }

    LOG_INFO("BlueGreenDeployment: promoted {} slot to active (version {})",
             new_active == DeploymentSlot::BLUE ? "BLUE" : "GREEN",
             promoted_version);

    if (cb) {
        try {
            cb(new_active, promoted_version);
        } catch (const std::exception& e) {
            LOG_WARN("BlueGreenDeployment: promotion callback threw: {}",
                     e.what());
        }
    }

    return true;
}

bool BlueGreenDeployment::rollback(const std::string& reason) {
    std::string rid;
    RollbackCallback cb;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_rolled_back_) {
            LOG_WARN("BlueGreenDeployment: already rolled back");
            return false;
        }

        rid              = rollback_id_;
        rollback_reason_ = reason.empty() ? "manual rollback" : reason;
        is_rolled_back_  = true;

        // Restore the original active slot.
        active_slot_     = config_.initial_active_slot;
        cb               = rollback_cb_;
    }

    LOG_WARN("BlueGreenDeployment: rolling back – reason: {}", rollback_reason_);

    bool ok = true;
    if (!rid.empty()) {
        ok = engine_->rollback(rid);
    } else {
        LOG_WARN("BlueGreenDeployment: no rollback_id available; "
                 "update may not have been applied");
    }

    if (cb) {
        try {
            cb(rollback_reason_);
        } catch (const std::exception& e) {
            LOG_WARN("BlueGreenDeployment: rollback callback threw: {}",
                     e.what());
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// Health tracking
// ---------------------------------------------------------------------------

void BlueGreenDeployment::reportSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++success_count_;
}

void BlueGreenDeployment::reportError() {
    bool trigger_rollback = false;
    std::string reason;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++error_count_;

        if (!is_rolled_back_ && is_promoted_) {
            const size_t total = success_count_ + error_count_;
            if (total >= config_.min_sample_count) {
                const double rate =
                    static_cast<double>(error_count_) /
                    static_cast<double>(total);
                if (rate > config_.error_rate_threshold) {
                    trigger_rollback = true;
                    reason = "error rate " +
                             std::to_string(static_cast<int>(rate * 100)) +
                             "% exceeds threshold " +
                             std::to_string(static_cast<int>(
                                 config_.error_rate_threshold * 100)) +
                             "%";
                }
            }
        }
    }

    if (trigger_rollback) {
        LOG_WARN("BlueGreenDeployment: auto-rollback triggered – {}", reason);
        rollback(reason);
    }
}

double BlueGreenDeployment::errorRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t total = success_count_ + error_count_;
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(error_count_) / static_cast<double>(total);
}

bool BlueGreenDeployment::shouldRollback() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_rolled_back_ || !is_promoted_) {
        return false;
    }
    const size_t total = success_count_ + error_count_;
    if (total < config_.min_sample_count) {
        return false;
    }
    const double rate =
        static_cast<double>(error_count_) / static_cast<double>(total);
    return rate > config_.error_rate_threshold;
}

// ---------------------------------------------------------------------------
// Status & accessors
// ---------------------------------------------------------------------------

BlueGreenStatus BlueGreenDeployment::status() const {
    std::lock_guard<std::mutex> lock(mutex_);

    BlueGreenStatus s;
    s.active_slot        = active_slot_;
    s.blue_version       = blue_version_;
    s.green_version      = green_version_;
    s.standby_is_deployed = standby_deployed_;
    s.is_promoted        = is_promoted_;
    s.is_rolled_back     = is_rolled_back_;
    s.rollback_reason    = rollback_reason_;
    s.rollback_id        = rollback_id_;

    const size_t total   = success_count_ + error_count_;
    s.sample_count       = total;
    s.observed_error_rate =
        total > 0
            ? static_cast<double>(error_count_) / static_cast<double>(total)
            : 0.0;

    return s;
}

DeploymentSlot BlueGreenDeployment::activeSlot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_slot_;
}

std::string BlueGreenDeployment::slotVersion(DeploymentSlot slot) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slot == DeploymentSlot::BLUE ? blue_version_ : green_version_;
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void BlueGreenDeployment::setPromotionCallback([[maybe_unused]] PromotionCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    promotion_cb_ = std::move(cb);
}

void BlueGreenDeployment::setRollbackCallback([[maybe_unused]] RollbackCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_cb_ = std::move(cb);
}

} // namespace updates
} // namespace themis


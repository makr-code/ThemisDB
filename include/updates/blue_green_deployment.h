/**
 * @file blue_green_deployment.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "updates/hot_reload_engine.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace themis {
namespace updates {

/**
 * @brief Identifies one of two deployment slots.
 *
 * At any point exactly one slot is *active* (serving live traffic) and the
 * other is *standby* (idle or running the new candidate version).
 */
enum class DeploymentSlot {
    BLUE,   ///< First slot – the default initial active slot.
    GREEN,  ///< Second slot – used for staged deployments.
};

/**
 * @brief Configuration for a blue/green deployment.
 *
 * Constraints (from module ROADMAP):
 *  - HotReloadEngine is used for the actual file-level update; callers must
 *    not run concurrent updates.
 *  - Cross-node coordination is NOT implemented here; each node manages its
 *    own slot state independently.
 */
struct BlueGreenConfig {
    /// Initial active slot (default: BLUE).
    DeploymentSlot initial_active_slot = DeploymentSlot::BLUE;

    /// Error rate (0.0 – 1.0) above which an automatic rollback is triggered
    /// when the standby slot is active (post-promotion).
    double error_rate_threshold = 0.05;

    /// Minimum number of health events required before the error rate threshold
    /// is evaluated (prevents premature rollback on small samples).
    size_t min_sample_count = 20;
};

/**
 * @brief Snapshot of a blue/green deployment's current state.
 */
struct BlueGreenStatus {
    /// Which slot is currently active (serving traffic).
    DeploymentSlot active_slot = DeploymentSlot::BLUE;

    /// Version string deployed to the blue slot (empty if not yet deployed).
    std::string blue_version;

    /// Version string deployed to the green slot (empty if not yet deployed).
    std::string green_version;

    /// True when the standby slot has had a version deployed to it.
    bool standby_is_deployed = false;

    /// True when the standby slot has been promoted to active.
    bool is_promoted = false;

    /// True when the deployment was rolled back.
    bool is_rolled_back = false;

    /// Reason supplied to the last rollback call (empty if no rollback).
    std::string rollback_reason;

    /// Rollback ID returned by HotReloadEngine::applyHotReload for the
    /// standby deployment; used to undo via HotReloadEngine::rollback.
    std::string rollback_id;

    /// Observed error rate since the last promotion (0.0 – 1.0).
    double observed_error_rate = 0.0;

    /// Number of health events (successes + errors) recorded since the last
    /// promotion.
    size_t sample_count = 0;
};

/**
 * @brief Blue/green deployment controller.
 *
 * Manages two deployment slots (BLUE and GREEN) so that a new version can be
 * deployed to the idle standby slot, validated, and then promoted to active
 * atomically.  If the promoted version develops problems the controller can
 * roll back to the previous active slot using the stored rollback ID from
 * HotReloadEngine.
 *
 * Usage example:
 * @code
 *   BlueGreenConfig cfg;
 *   BlueGreenDeployment bg(hot_reload_engine, cfg);
 *
 *   // 1. Deploy new version to standby (green) slot
 *   auto result = bg.deployToStandby("1.6.0");
 *   if (!result.success) { handle_error(); }
 *
 *   // 2. Run health checks on the standby slot
 *   // ...
 *
 *   // 3. Promote standby to active
 *   if (!bg.promote()) { handle_promotion_failure(); }
 *
 *   // 4. Observe post-promotion health events
 *   bg.reportSuccess();
 *   bg.reportError();
 *
 *   // 5. Rollback if needed
 *   if (bg.shouldRollback()) { bg.rollback("error rate too high"); }
 * @endcode
 */
class BlueGreenDeployment {
public:
    /// Callback invoked after a successful slot promotion.
    /// Receives the new active slot and the promoted version string.
    using PromotionCallback =
        std::function<void(DeploymentSlot /*active_slot*/,
                           const std::string& /*version*/)>;

    /// Callback invoked when a rollback is triggered.
    /// Receives a human-readable reason string.
    using RollbackCallback =
        std::function<void(const std::string& /*reason*/)>;

    /**
     * @brief Construct a blue/green deployment controller.
     * @param engine  Shared hot-reload engine (must outlive this object).
     * @param config  Deployment configuration.
     * @throws std::invalid_argument if engine is null.
     */
    BlueGreenDeployment(std::shared_ptr<HotReloadEngine> engine,
                        const BlueGreenConfig& config = {});

    ~BlueGreenDeployment() = default;

    // Non-copyable
    BlueGreenDeployment(const BlueGreenDeployment&) = delete;
    BlueGreenDeployment& operator=(const BlueGreenDeployment&) = delete;

    // ---------- Deployment lifecycle ----------

    /**
     * @brief Deploy a new version to the standby slot.
     *
     * Calls HotReloadEngine::applyHotReload for the given version and stores
     * the resulting rollback ID for future use.  The active slot is unchanged.
     *
     * @param version  Version string to deploy (e.g., "1.6.0").
     * @return Result from HotReloadEngine::applyHotReload.
     */
    ReloadResult deployToStandby(const std::string& version);

    /**
     * @brief Promote the standby slot to active.
     *
     * Swaps the active and standby slots.  Resets per-promotion health
     * counters and invokes the promotion callback.  Has no effect if the
     * standby has not been deployed, if the deployment was already promoted,
     * or if it was rolled back.
     *
     * @return true  if the promotion succeeded.
     * @return false if preconditions are not met (see above).
     */
    bool promote();

    /**
     * @brief Roll back to the previous active slot.
     *
     * Uses the stored rollback ID (from deployToStandby) to invoke
     * HotReloadEngine::rollback.  Sets the internal is_rolled_back flag and
     * invokes the rollback callback.
     *
     * @param reason  Human-readable reason for the rollback (optional).
     * @return true if rollback succeeded or no rollback ID was available.
     */
    bool rollback(const std::string& reason = "");

    // ---------- Health tracking ----------

    /**
     * @brief Record one successful operation after the standby was promoted.
     *
     * If shouldRollback() returns true after updating the counters the
     * automatic rollback is triggered.
     */
    void reportSuccess();

    /**
     * @brief Record one failed operation after the standby was promoted.
     *
     * Automatically triggers rollback when errorRate() exceeds the configured
     * threshold and the minimum sample count has been reached.
     */
    void reportError();

    /**
     * @brief Current error rate since the last promotion (0.0 – 1.0).
     *
     * Returns 0.0 when no events have been recorded yet.
     */
    double errorRate() const;

    /**
     * @brief Return true if the error rate exceeds the threshold and the
     *        minimum sample count has been reached.
     */
    bool shouldRollback() const;

    // ---------- Status & accessors ----------

    /**
     * @brief Get a snapshot of the current deployment state.
     */
    BlueGreenStatus status() const;

    /**
     * @brief Return the currently active slot.
     */
    DeploymentSlot activeSlot() const;

    /**
     * @brief Return the version string associated with a slot.
     *
     * Returns an empty string when no version has been deployed to the slot.
     */
    std::string slotVersion(DeploymentSlot slot) const;

    // ---------- Callbacks ----------

    /**
     * @brief Register a callback invoked after a successful promotion.
     * @param cb  Receives (new_active_slot, promoted_version).
     */
    void setPromotionCallback(PromotionCallback cb);

    /**
     * @brief Register a callback invoked when a rollback is triggered.
     * @param cb  Receives the human-readable rollback reason.
     */
    void setRollbackCallback(RollbackCallback cb);

private:
    /// Return the slot that is currently standby (opposite of active_slot_).
    DeploymentSlot standbySlot() const;

    mutable std::mutex mutex_;

    std::shared_ptr<HotReloadEngine> engine_;
    BlueGreenConfig config_;

    DeploymentSlot active_slot_;

    std::string blue_version_;
    std::string green_version_;

    bool standby_deployed_{false};
    bool is_promoted_{false};
    bool is_rolled_back_{false};
    std::string rollback_id_;
    std::string rollback_reason_;

    // Per-promotion health counters (reset on promote())
    size_t success_count_{0};
    size_t error_count_{0};

    PromotionCallback promotion_cb_;
    RollbackCallback  rollback_cb_;
};

} // namespace updates
} // namespace themis

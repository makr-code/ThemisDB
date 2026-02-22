/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            canary_rollout.h                                   ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "updates/hot_reload_engine.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief A single stage in a canary rollout.
 *
 * Nodes are selected deterministically by hashing the node_id with the
 * version string.  The @p percentage is cumulative: a node in stage 1
 * (10 %) is also in stage 2 (25 %), etc.
 */
struct CanaryStage {
    /// Fraction of the node population that receives the update (0.0 – 1.0).
    double percentage = 0.0;

    /// Minimum observation window before the rollout controller considers
    /// advancing to the next stage automatically.
    std::chrono::seconds observation_duration{300};
};

/**
 * @brief Configuration for a canary rollout.
 *
 * Constraints (from module ROADMAP):
 *  - Cross-node coordination is NOT implemented here; each node runs its
 *    own canary controller and the @p node_id hash determines membership.
 *  - HotReloadEngine is used for the actual file-level update; callers must
 *    not run concurrent updates.
 */
struct CanaryConfig {
    /// Target version string (e.g., "1.5.0").
    std::string version;

    /// Stable identifier for this node (hostname, UUID, …).
    std::string node_id;

    /// Ordered list of rollout stages.  The last stage must have
    /// percentage == 1.0 to guarantee full coverage.
    std::vector<CanaryStage> stages;

    /// Error rate (0.0 – 1.0) above which an automatic rollback is triggered.
    double error_rate_threshold = 0.05;

    /// Total number of events required before error_rate_threshold is evaluated.
    size_t min_sample_count = 20;

    /// Default: four canonical stages (1 % → 5 % → 25 % → 100 %).
    static CanaryConfig withDefaultStages(const std::string& version,
                                          const std::string& node_id);
};

/**
 * @brief Snapshot of a canary rollout's current state.
 */
struct CanaryStatus {
    /// 0-based index of the active stage.
    size_t current_stage = 0;

    /// Total number of configured stages.
    size_t total_stages = 0;

    /// Cumulative percentage of nodes in the active stage (0.0 – 1.0).
    double current_percentage = 0.0;

    /// Whether this node is included in the active stage.
    bool this_node_included = false;

    /// Whether the rollout has completed all stages successfully.
    bool is_complete = false;

    /// Whether the rollout was rolled back.
    bool is_rolled_back = false;

    /// Target version being rolled out.
    std::string version;

    /// Non-empty only when is_rolled_back is true.
    std::string rollback_reason;

    /// Current observed error rate (successes + errors tracked via
    /// reportSuccess() / reportError()).
    double observed_error_rate = 0.0;

    /// Number of events observed since the last stage advance.
    size_t sample_count = 0;

    /// Rollback ID from the last successful HotReloadEngine::applyHotReload,
    /// used to undo the update.
    std::string rollback_id;
};

/**
 * @brief Canary rollout controller.
 *
 * Manages a progressive, fraction-based rollout of a new version across a
 * fleet of nodes.  Membership in a stage is determined by a deterministic
 * hash of (version + node_id) so that the same node always makes the same
 * decision without requiring inter-node communication.
 *
 * Usage example:
 * @code
 *   auto cfg = CanaryConfig::withDefaultStages("1.5.0", hostname);
 *   CanaryRollout canary(hot_reload_engine, cfg);
 *
 *   if (canary.isNodeInCurrentStage()) {
 *       auto result = canary.applyIfIncluded();
 *       if (result.success) {
 *           // record rollback_id for later use
 *       }
 *   }
 *
 *   // After observing the stage, advance manually or set up a timer:
 *   canary.advanceStage();
 * @endcode
 */
class CanaryRollout {
public:
    using StageCompleteCallback =
        std::function<void(size_t /*stage*/, double /*percentage*/)>;
    using RollbackCallback =
        std::function<void(const std::string& /*reason*/)>;

    /**
     * @brief Construct a canary rollout controller.
     * @param engine  Shared hot-reload engine (must outlive this object).
     * @param config  Rollout configuration.
     */
    CanaryRollout(std::shared_ptr<HotReloadEngine> engine,
                  const CanaryConfig& config);

    ~CanaryRollout() = default;

    // Non-copyable
    CanaryRollout(const CanaryRollout&) = delete;
    CanaryRollout& operator=(const CanaryRollout&) = delete;

    // ---------- Node membership ----------

    /**
     * @brief Check whether this node belongs to a specific stage.
     *
     * The decision is deterministic: it depends only on the node_id and
     * version, not on any shared state.
     *
     * @param stage_index  0-based stage index.
     * @return true if the node hash falls within the stage percentage.
     */
    bool isNodeInStage(size_t stage_index) const;

    /**
     * @brief Check whether this node belongs to the *current* active stage.
     */
    bool isNodeInCurrentStage() const;

    // ---------- Stage management ----------

    /**
     * @brief Return the 0-based index of the currently active stage.
     */
    size_t currentStage() const;

    /**
     * @brief Advance to the next stage.
     *
     * Resets the per-stage event counters.  Has no effect once all stages
     * are complete or the rollout has been rolled back.
     *
     * @return true  if there was a next stage to advance to.
     * @return false if already at the final stage or rolled back.
     */
    bool advanceStage();

    // ---------- Update application ----------

    /**
     * @brief Apply the update via HotReloadEngine if this node is included
     *        in the current stage.
     *
     * If the node is not in the current stage, returns a ReloadResult with
     * success == false and a descriptive error_message.
     *
     * @return Result from HotReloadEngine::applyHotReload, or a skipped result.
     */
    ReloadResult applyIfIncluded();

    /**
     * @brief Roll back the update using the stored rollback_id.
     *
     * Sets the internal is_rolled_back flag and invokes the rollback callback.
     *
     * @param reason  Human-readable reason for the rollback.
     * @return true if rollback succeeded.
     */
    bool rollback(const std::string& reason = "");

    // ---------- Health tracking ----------

    /**
     * @brief Record one successful operation after the update was applied.
     *
     * If shouldRollback() returns true after updating the counters, the
     * automatic rollback is triggered.
     */
    void reportSuccess();

    /**
     * @brief Record one failed operation after the update was applied.
     *
     * Automatically triggers rollback when errorRate() exceeds the configured
     * threshold and the minimum sample count has been reached.
     */
    void reportError();

    /**
     * @brief Current error rate since the last stage advance (0.0 – 1.0).
     *
     * Returns 0.0 when no events have been recorded yet.
     */
    double errorRate() const;

    /**
     * @brief Return true if the error rate exceeds the threshold and the
     *        minimum sample count has been reached.
     */
    bool shouldRollback() const;

    // ---------- Status & callbacks ----------

    /**
     * @brief Get a snapshot of the current rollout state.
     */
    CanaryStatus status() const;

    /**
     * @brief Register a callback invoked when a stage completes.
     * @param cb  Receives (stage_index, cumulative_percentage).
     */
    void setStageCompleteCallback(StageCompleteCallback cb);

    /**
     * @brief Register a callback invoked when a rollback is triggered.
     * @param cb  Receives the human-readable rollback reason.
     */
    void setRollbackCallback(RollbackCallback cb);

private:
    /**
     * @brief Compute a deterministic hash value in [0, 1) for the node,
     *        given a specific stage percentage bucket.
     *
     * Uses std::hash on the concatenation of version + node_id.
     */
    double computeNodeHash() const;

    mutable std::mutex mutex_;

    std::shared_ptr<HotReloadEngine> engine_;
    CanaryConfig config_;

    size_t current_stage_{0};
    bool is_complete_{false};
    bool is_rolled_back_{false};
    std::string rollback_reason_;
    std::string rollback_id_;

    // Per-stage event counters (reset on advanceStage())
    size_t success_count_{0};
    size_t error_count_{0};

    StageCompleteCallback stage_complete_cb_;
    RollbackCallback rollback_cb_;
};

} // namespace updates
} // namespace themis

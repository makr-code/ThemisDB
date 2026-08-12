/**
 * @file canary_rollout.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "updates/hot_reload_engine.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
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
     * @brief Compute a deterministic hash value in [0, 1) for the node.
     *
     * Uses std::hash on the concatenation of version + node_id.
     * NOTE: std::hash<std::string> is consistent within a single binary but is
     * implementation-defined across different compilers/platforms.  Since all
     * nodes in a cluster run the same binary when evaluating canary membership,
     * this is sufficient for single-binary deployments.
     */
    double computeNodeHash() const;

    mutable std::mutex mutex_;

    std::shared_ptr<HotReloadEngine> engine_;
    CanaryConfig config_;

    size_t current_stage_{0};
    bool is_complete_{false};
    bool is_rolled_back_{false};
    bool is_applied_{false};         ///< Guard: prevent double-apply on the same node.
    std::string rollback_reason_;
    std::string rollback_id_;

    // Per-stage event counters (reset on advanceStage())
    size_t success_count_{0};
    size_t error_count_{0};

    StageCompleteCallback stage_complete_cb_;
    RollbackCallback rollback_cb_;
};

// ============================================================================
// CanaryDeployment – higher-level builder-pattern API (Issue #4046)
// ============================================================================

/**
 * @brief Percentile latency statistics computed from observed samples.
 */
struct LatencyStats {
    std::chrono::microseconds p50{0};
    std::chrono::microseconds p95{0};
    std::chrono::microseconds p99{0};
    size_t sample_count = 0;
};

/**
 * @brief A snapshot of all metrics tracked by CanaryDeployment.
 */
struct CanaryMetricsSnapshot {
    LatencyStats latency;

    /// Latest observed memory usage in bytes (0 if not reported).
    double memory_bytes = 0.0;

    /// Latest observed CPU utilisation fraction 0.0–1.0 (0 if not reported).
    double cpu_fraction = 0.0;

    /// Latest observed disk I/O throughput in bytes/s (0 if not reported).
    double disk_io_bytes_per_sec = 0.0;

    /// Registered custom metrics (name → latest value).
    std::unordered_map<std::string, double> custom_metrics;

    size_t error_count  = 0;
    size_t success_count = 0;
    double error_rate   = 0.0;

    /// True when the tracked p99 latency exceeds the configured threshold.
    bool latency_threshold_exceeded = false;
};

/**
 * @brief Configuration for A/B testing within a canary deployment.
 *
 * When A/B testing is enabled every incoming request (identified by a
 * string key) is deterministically assigned to either the *canary*
 * (new-version) bucket or the *control* (current-version) bucket.
 *
 * The split is stateless: hashing request_id + experiment_id gives a
 * consistent, per-request bucket without shared counters.
 */
struct ABTestConfig {
    /// Fraction of traffic routed to the canary (0.0–1.0).  Default 10 %.
    double canary_fraction = 0.10;

    /// Stable experiment identifier written into metrics/logs.
    std::string experiment_id;
};

/**
 * @brief A single stage specification for CanaryDeployment.
 *
 * Uses integer percentages (1, 5, 25, 100) and `std::chrono::seconds` /
 * `std::chrono::hours` durations, consistent with the API documented in
 * Issue #4046.
 */
struct CanaryDeploymentStage {
    /// Percentage of nodes that should receive the update (1–100).
    int percentage = 0;

    /// Observation window before the stage is eligible to advance.
    std::chrono::seconds duration{0};

    /// 0-based stage index (filled in automatically by CanaryDeployment).
    size_t stage_number = 0;
};

/**
 * @brief Higher-level canary deployment controller (Issue #4046).
 *
 * Wraps CanaryRollout with:
 *  - Builder-pattern configuration API (setVersion, setStages, …)
 *  - Latency percentile tracking (p50 / p95 / p99)
 *  - Scalar metric collection (memory, CPU, disk I/O)
 *  - Custom metric registration
 *  - A/B testing and traffic splitting
 *  - Automatic rollback on latency-threshold breach
 *
 * Usage:
 * @code
 *   CanaryDeployment canary;
 *   canary.setVersion("1.5.0");
 *   canary.setStages({
 *       {.percentage = 1,   .duration = std::chrono::hours(1)},
 *       {.percentage = 5,   .duration = std::chrono::hours(2)},
 *       {.percentage = 25,  .duration = std::chrono::hours(6)},
 *       {.percentage = 100, .duration = std::chrono::hours(0)},
 *   });
 *   canary.setErrorRateThreshold(0.05);
 *   canary.setLatencyThreshold(std::chrono::milliseconds(500));
 *   canary.setEngine(hot_reload_engine);
 *   canary.setNodeId(hostname);
 *
 *   auto result = canary.deploy();
 *
 *   canary.onStageComplete([](const CanaryDeploymentStage& stage) {
 *       LOG_INFO("Stage {} complete: {}% of nodes updated",
 *                stage.stage_number, stage.percentage);
 *   });
 *
 *   canary.onRollback([](const std::string& reason) {
 *       LOG_ERROR("Canary deployment rolled back: {}", reason);
 *   });
 * @endcode
 */
class CanaryDeployment {
public:
    using StageCompleteCallback =
        std::function<void(const CanaryDeploymentStage& /*stage*/)>;
    using RollbackCallback =
        std::function<void(const std::string& /*reason*/)>;

    CanaryDeployment();
    ~CanaryDeployment() = default;

    CanaryDeployment(const CanaryDeployment&) = delete;
    CanaryDeployment& operator=(const CanaryDeployment&) = delete;
    CanaryDeployment(CanaryDeployment&& other) noexcept;
    CanaryDeployment& operator=(CanaryDeployment&& other) noexcept;

    // -----------------------------------------------------------------------
    // Builder API
    // -----------------------------------------------------------------------

    /** @brief Set the version string to deploy (e.g., "1.5.0"). */
    void setVersion(const std::string& version);

    /**
     * @brief Set the rollout stages.
     *
     * stage_number is filled in automatically (0-based).
     * The last stage must have percentage == 100 to guarantee full coverage.
     */
    void setStages(std::vector<CanaryDeploymentStage> stages);

    /** @brief Set the error-rate threshold that triggers auto-rollback (0–1). */
    void setErrorRateThreshold(double threshold);

    /**
     * @brief Set the p99 latency threshold that triggers auto-rollback.
     *
     * Checked each time reportLatency() is called.
     */
    void setLatencyThreshold(std::chrono::milliseconds p99_limit);

    /** @brief Provide the HotReloadEngine used to apply / rollback the update. */
    void setEngine(std::shared_ptr<HotReloadEngine> engine);

    /**
     * @brief Set the stable node identifier used for canary-group membership.
     *
     * If not set, deploy() will throw std::invalid_argument.
     */
    void setNodeId(const std::string& node_id);

    // -----------------------------------------------------------------------
    // Deployment
    // -----------------------------------------------------------------------

    /**
     * @brief Start the canary deployment.
     *
     * Validates configuration, creates an internal CanaryRollout, and calls
     * applyIfIncluded() for this node.
     *
     * @return ReloadResult from HotReloadEngine::applyHotReload, or a skipped
     *         result when this node is not in the first canary stage.
     *
     * @throws std::invalid_argument if version, node_id, engine, or stages
     *         are missing / invalid.
     */
    ReloadResult deploy();

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /**
     * @brief Register a callback invoked when a stage completes.
     *
     * The callback receives a copy of the CanaryDeploymentStage that just
     * finished (including the stage_number and percentage).
     */
    void onStageComplete(StageCompleteCallback cb);

    /**
     * @brief Register a callback invoked when the rollout is rolled back.
     * @param cb Receives the human-readable reason string.
     */
    void onRollback(RollbackCallback cb);

    // -----------------------------------------------------------------------
    // Health / metric reporting
    // -----------------------------------------------------------------------

    /** @brief Record one successful operation. */
    void reportSuccess();

    /** @brief Record one failed operation (may trigger auto-rollback). */
    void reportError();

    /**
     * @brief Feed a latency sample.
     *
     * Samples are stored in a bounded reservoir (max 1000 entries).
     * Percentiles are recomputed lazily when getMetricsSnapshot() is called.
     * If the p99 of the current reservoir exceeds the configured latency
     * threshold, rollback is triggered automatically.
     */
    void reportLatency(std::chrono::microseconds latency);

    /** @brief Update the latest memory-usage reading. */
    void reportMemoryUsage(double bytes);

    /** @brief Update the latest CPU-utilisation reading (0.0–1.0). */
    void reportCpuUsage(double fraction);

    /** @brief Update the latest disk I/O throughput reading (bytes/s). */
    void reportDiskIO(double bytes_per_sec);

    /**
     * @brief Record (or update) a named custom metric.
     *
     * Custom metrics appear in the CanaryMetricsSnapshot and are useful for
     * domain-specific signals like query-error rates or transaction failures.
     */
    void recordCustomMetric(const std::string& name, double value);

    // -----------------------------------------------------------------------
    // A/B testing and traffic splitting
    // -----------------------------------------------------------------------

    /**
     * @brief Enable A/B testing.
     *
     * Once enabled, isCanaryRequest() / isControlRequest() can be used to
     * deterministically route individual requests.
     */
    void enableABTesting(const ABTestConfig& config);

    /**
     * @brief Return true if the given request key should be routed to the
     *        canary (new-version) path.
     *
     * Uses a deterministic hash of (request_id + experiment_id) so that the
     * same request always lands in the same bucket.
     *
     * Returns false when A/B testing is not enabled.
     */
    bool isCanaryRequest(const std::string& request_id) const;

    /**
     * @brief Return true if the given request key should be routed to the
     *        control (old-version) path.
     */
    bool isControlRequest(const std::string& request_id) const;

    /**
     * @brief Return true if this node is in the canary group for the current
     *        rollout stage (delegates to CanaryRollout::isNodeInCurrentStage).
     *
     * Returns false before deploy() is called.
     */
    bool isNodeInCanaryGroup() const;

    // -----------------------------------------------------------------------
    // Status and metrics
    // -----------------------------------------------------------------------

    /** @brief Get a snapshot of metrics collected since the last stage advance. */
    CanaryMetricsSnapshot getMetricsSnapshot() const;

    /** @brief Get a snapshot of the rollout state (delegates to CanaryRollout). */
    CanaryStatus status() const;

    /** @brief Manually advance to the next stage (delegates to CanaryRollout). */
    bool advanceStage();

    /** @brief Manually roll back the deployment. */
    bool rollback(const std::string& reason = "");

private:
    // Compute percentile from sorted latency reservoir (caller must hold mutex).
    LatencyStats computeLatencyStats() const;

    // Check latency threshold and trigger rollback if exceeded (must NOT hold mutex).
    void checkLatencyThreshold();

    mutable std::mutex mutex_;

    // Configuration
    std::string version_;
    std::string node_id_;
    std::vector<CanaryDeploymentStage> stages_;
    double error_rate_threshold_ = 0.05;
    std::chrono::microseconds latency_threshold_us_{0};  // 0 = disabled
    std::shared_ptr<HotReloadEngine> engine_;

    // Internal rollout controller (created by deploy())
    std::unique_ptr<CanaryRollout> rollout_;

    // Callbacks
    StageCompleteCallback stage_complete_cb_;
    RollbackCallback rollback_cb_;

    // Metrics
    std::deque<int64_t> latency_samples_us_;    ///< Bounded circular reservoir (≤ 1000, O(1) push/pop)
    double memory_bytes_{0.0};
    double cpu_fraction_{0.0};
    double disk_io_bytes_per_sec_{0.0};
    std::unordered_map<std::string, double> custom_metrics_;

    // A/B testing
    bool ab_testing_enabled_{false};
    ABTestConfig ab_config_;

    static constexpr size_t kMaxLatencySamples = 1000;
};

} // namespace updates
} // namespace themis

/**
 * @file index_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "storage/tiered_storage.h"
#include "utils/expected.h"

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

class RocksDBWrapper;
class CronExpression;

// ─────────────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Recommended maintenance action for an index as determined by analyze().
 */
enum class IndexRecommendation {
    NONE,            ///< Index is healthy – no maintenance needed
    UPDATE_STATS,    ///< Statistics are stale – refresh without structural change
    REORGANIZE,      ///< Light in-place defragmentation (online, minimal locking)
    PARTIAL_REBUILD, ///< Rebuild the most fragmented segments only
    FULL_REBUILD     ///< Discard and rebuild the entire index structure
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-tier threshold configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fragmentation and staleness thresholds for one storage tier.
 *
 * Thresholds are intentionally looser for warm/cold tiers because rebuild
 * overhead is amortised over longer access intervals.
 */
struct TierThresholds {
    double reorganize_pct  = 10.0;  ///< Fragmentation % triggering REORGANIZE
    double partial_rebuild_pct = 20.0; ///< Fragmentation % triggering PARTIAL_REBUILD
    double full_rebuild_pct  = 35.0;   ///< Fragmentation % triggering FULL_REBUILD
    uint32_t stats_stale_hours = 1;    ///< Hours after which statistics are considered stale

    /// Defaults suitable for a hot-tier (NVMe) index
    static TierThresholds hot() noexcept {
        return {10.0, 20.0, 35.0, 1};
    }
    /// Defaults suitable for a warm-tier (SATA SSD) index
    static TierThresholds warm() noexcept {
        return {18.0, 32.0, 50.0, 6};
    }
    /// Defaults suitable for a cold-tier (object storage / archive) index
    static TierThresholds cold() noexcept {
        return {30.0, 50.0, 70.0, 24};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-index entry in the YAML config
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration entry for a single managed index.
 */
struct IndexEntry {
    std::string name;                               ///< Index identifier
    storage::StorageTierLevel tier                  ///< Current storage tier
        = storage::StorageTierLevel::HOT;
    bool enabled = true;                            ///< Include in scheduled analysis
    std::optional<TierThresholds> overrides;        ///< Per-index threshold overrides (nullopt = use tier defaults)
};

// ─────────────────────────────────────────────────────────────────────────────
// Top-level YAML configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Full YAML-loadable configuration for IndexAnalyzer.
 *
 * Maps to the top-level "index_analyze" key in
 * config/index_analyze.yaml (or the main config.yaml).
 *
 * @code{.yaml}
 * index_analyze:
 *   enabled: true
 *   cron_expression: "0 2 * * *"
 *   thresholds:
 *     hot:   { reorganize_pct: 10, partial_rebuild_pct: 20, full_rebuild_pct: 35, stats_stale_hours: 1 }
 *     warm:  { reorganize_pct: 18, partial_rebuild_pct: 32, full_rebuild_pct: 50, stats_stale_hours: 6 }
 *     cold:  { reorganize_pct: 30, partial_rebuild_pct: 50, full_rebuild_pct: 70, stats_stale_hours: 24 }
 *   ai_advisor:
 *     enabled: false
 *     model: ""
 *   indices:
 *     - name: primary
 *       tier: hot
 *     - name: vectors
 *       tier: warm
 * @endcode
 */
struct IndexAnalyzeConfig {
    bool enabled = true;

    /// Cron expression controlling when scheduled analysis runs.
    /// Standard 5-field POSIX cron syntax; @daily etc. shortcuts supported.
    /// Empty string disables cron scheduling (manual-trigger only).
    std::string cron_expression = "0 2 * * *";  ///< Default: daily at 02:00

    TierThresholds hot_thresholds  = TierThresholds::hot();
    TierThresholds warm_thresholds = TierThresholds::warm();
    TierThresholds cold_thresholds = TierThresholds::cold();

    // AI/ML advisor settings
    bool ai_advisor_enabled   = false;
    std::string ai_advisor_model;  ///< Model identifier forwarded to registered advisor

    std::vector<IndexEntry> indices;

    /**
     * @brief Load configuration from a YAML file.
     *
     * The file is expected to contain an "index_analyze" root key.
     * Missing keys are filled with defaults; unknown keys are silently ignored.
     *
     * @param yaml_path Absolute or relative path to the YAML file.
     * @return Loaded config or an error string.
     */
    static Result<IndexAnalyzeConfig> fromYamlFile(const std::string& yaml_path);

    /**
     * @brief Return the threshold set that applies to the given tier.
     */
    const TierThresholds& thresholdsFor(storage::StorageTierLevel tier) const noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
// Analysis result
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result of a single-index analysis run.
 */
struct IndexAnalysisReport {
    std::string index_name;
    storage::StorageTierLevel tier     = storage::StorageTierLevel::HOT;
    double fragmentation_pct           = 0.0;
    uint64_t total_entries             = 0;
    uint64_t orphan_entries            = 0;
    uint64_t size_bytes                = 0;
    uint64_t stats_age_hours           = 0;  ///< Hours since last statistics update
    bool stats_stale                   = false;

    IndexRecommendation recommendation = IndexRecommendation::NONE;
    std::string reason;  ///< Human-readable explanation of the recommendation

    /// Optional override produced by the AI/ML advisor.
    /// When set, callers SHOULD prefer this over `recommendation`.
    std::optional<IndexRecommendation> ai_recommendation;
    std::string ai_reason;  ///< Explanation from the AI advisor

    std::chrono::system_clock::time_point analyzed_at;
};

// ─────────────────────────────────────────────────────────────────────────────
// AI/ML advisor hook
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Plugin interface for AI/ML-driven index maintenance decisions.
 *
 * Register an implementation via IndexAnalyzer::setAdvisor().
 * The advisor receives the preliminary analysis report (with the rule-based
 * recommendation already populated) and may return a different recommendation.
 * Returning std::nullopt leaves the rule-based recommendation unchanged.
 *
 * Thread safety: advise() must be safe to call from multiple threads.
 */
class IIndexAnalysisAdvisor {
public:
    virtual ~IIndexAnalysisAdvisor() = default;

    /**
     * @brief Inspect the preliminary analysis report and optionally override.
     *
     * @param report  Read-only preliminary report produced by rule-based logic.
     * @return Override recommendation + reason, or nullopt to keep rule-based result.
     */
    virtual std::optional<std::pair<IndexRecommendation, std::string>>
    advise(const IndexAnalysisReport& report) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IndexAnalyzer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-index analysis engine with cron scheduling and AI/ML advisor hook.
 *
 * ## Responsibilities
 * - Compute fragmentation, orphan entries, statistics staleness per index.
 * - Apply tier-aware thresholds to derive a maintenance recommendation
 *   (NONE / UPDATE_STATS / REORGANIZE / PARTIAL_REBUILD / FULL_REBUILD).
 * - Optionally forward the preliminary report to a registered IIndexAnalysisAdvisor
 *   (AI/ML model) which may override the recommendation.
 * - Run analysis on a cron schedule (background thread); results are retrievable
 *   via lastReports() without blocking the caller.
 *
 * ## Thread safety
 * All public methods are thread-safe.
 *
 * ## Usage example
 * @code{.cpp}
 * auto cfg = IndexAnalyzeConfig::fromYamlFile("config/index_analyze.yaml");
 * IndexAnalyzer analyzer(db_wrapper, *cfg);
 * analyzer.setAdvisor(my_ml_advisor);
 * analyzer.startScheduled();
 * // ...
 * auto reports = analyzer.analyzeAll();
 * analyzer.stopScheduled();
 * @endcode
 */
class IndexAnalyzer {
public:
    /**
     * @brief Construct with an existing RocksDB wrapper and loaded config.
     * @param db_wrapper  Non-null shared pointer to the active RocksDB instance.
     * @param config      Fully populated IndexAnalyzeConfig (from YAML or default).
     */
    explicit IndexAnalyzer(
        std::shared_ptr<RocksDBWrapper> db_wrapper,
        IndexAnalyzeConfig config = IndexAnalyzeConfig{});

    ~IndexAnalyzer();

    // Non-copyable, non-movable (owns mutex + thread)
    IndexAnalyzer(const IndexAnalyzer&)            = delete;
    IndexAnalyzer& operator=(const IndexAnalyzer&) = delete;
    IndexAnalyzer(IndexAnalyzer&&)                 = delete;
    IndexAnalyzer& operator=(IndexAnalyzer&&)      = delete;

    // ── Configuration ─────────────────────────────────────────────────────

    /**
     * @brief Replace the current configuration at runtime.
     *
     * If the background thread is running, the new cron expression and index
     * list take effect at the next scheduler wake-up.
     */
    void setConfig(IndexAnalyzeConfig config);

    const IndexAnalyzeConfig& config() const;

    // ── AI/ML advisor ─────────────────────────────────────────────────────

    /**
     * @brief Register an AI/ML advisor for post-analysis recommendation override.
     *
     * Pass nullptr to remove the current advisor (reverts to rule-based only).
     */
    void setAdvisor(std::shared_ptr<IIndexAnalysisAdvisor> advisor);

    // ── Manual analysis ───────────────────────────────────────────────────

    /**
     * @brief Analyse a single index and return the report.
     *
     * @param index_name   Name of the index to analyse.
     * @param tier         Storage tier the index currently resides on.
     * @param overrides    Optional per-call threshold overrides (nullopt = use config).
     */
    Result<IndexAnalysisReport> analyze(
        const std::string& index_name,
        storage::StorageTierLevel tier,
        std::optional<TierThresholds> overrides = std::nullopt);

    /**
     * @brief Analyse all indices registered in the config.
     *
     * Skips indices with `enabled = false`.
     * @return Vector of reports (one per enabled index).
     */
    std::vector<IndexAnalysisReport> analyzeAll();

    // ── Cron scheduling ───────────────────────────────────────────────────

    /**
     * @brief Start the background cron scheduler thread.
     *
     * The scheduler waits until the next time matching the configured
     * cron_expression, then calls analyzeAll() and stores the results.
     * Idempotent – calling start() twice has no effect.
     *
     * @return Error if cron_expression is invalid or already started.
     */
    Result<void> startScheduled();

    /**
     * @brief Stop the background cron scheduler thread.
     *
     * Blocks until the thread has exited.  Idempotent.
     */
    void stopScheduled();

    /// @return true if the background scheduler is currently running.
    bool isScheduled() const noexcept;

    // ── Result access ──────────────────────────────────────────────────────

    /**
     * @brief Return the reports from the most recent analyzeAll() run.
     *
     * Returns an empty vector if no analysis has been performed yet.
     */
    std::vector<IndexAnalysisReport> lastReports() const;

    /**
     * @brief Return the timestamp of the last scheduled analysis run.
     *
     * Returns nullopt if no scheduled run has completed yet.
     */
    std::optional<std::chrono::system_clock::time_point> lastRunTime() const;

private:
    // Background scheduler loop
    void schedulerLoop();

    // Core analysis logic (called from analyze() and the scheduler)
    IndexAnalysisReport computeReport(
        const std::string& index_name,
        storage::StorageTierLevel tier,
        const TierThresholds& thresholds);

    // Apply the AI advisor (if set) and update the report in-place
    void applyAdvisor(IndexAnalysisReport& report);

    // Derive recommendation from fragmentation metrics
    static IndexRecommendation classify(
        double frag_pct,
        bool stats_stale,
        const TierThresholds& thresholds);

    // Members
    std::shared_ptr<RocksDBWrapper>       db_wrapper_;
    mutable std::mutex                    mutex_;
    IndexAnalyzeConfig                    config_;
    std::shared_ptr<IIndexAnalysisAdvisor> advisor_;

    // Cron scheduler
    std::atomic<bool>                     running_{false};
    std::thread                           scheduler_thread_;
    std::condition_variable               cv_;

    // Last run results
    std::vector<IndexAnalysisReport>      last_reports_;
    std::optional<std::chrono::system_clock::time_point> last_run_time_;
};

} // namespace themis

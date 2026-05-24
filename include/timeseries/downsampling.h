/*
 * ThemisDB | File: downsampling.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file downsampling.h
 * @brief Multi-Tier Downsampling Pipeline for time-series data
 *
 * Implements a configurable pipeline that reduces raw time-series data through
 * successive resolution tiers (e.g. raw → 1 min → 1 hour → 1 day).  Each tier
 * is stored as a continuous aggregate in TSStore and can be governed by an
 * independent retention duration.
 *
 * The DownsamplingPipeline orchestrates:
 *  1. Automatic tier provisioning via ContinuousAggregateManager
 *  2. Watermark-driven incremental refresh per tier
 *  3. TierSelector for query_optimizer.cpp to route reads to the coarsest
 *     tier that satisfies the requested query resolution
 *  4. Integration with RetentionManager for per-tier data expiry
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "timeseries/continuous_agg.h"
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <unordered_map>

namespace themis {

class TSStore;
class RetentionManager;

// =========================================================================
// DownsamplingTier
// =========================================================================

/**
 * @brief Describes a single resolution tier in the downsampling pipeline.
 *
 * Each tier accumulates raw (or finer-grain) data into fixed-width windows
 * and stores the result as a continuous aggregate in TSStore.
 */
struct DownsamplingTier {
    std::string name;                       ///< Human-readable label (e.g. "1m", "1h", "1d")
    std::chrono::milliseconds resolution;   ///< Aggregation window width
    std::chrono::seconds     retention;     ///< How long to keep this tier's data (0 = forever)

    /// Convenience factory for common tier sizes
    static DownsamplingTier minutes(int n, std::chrono::seconds keep = std::chrono::seconds{0});
    static DownsamplingTier hours(int n, std::chrono::seconds keep = std::chrono::seconds{0});
    static DownsamplingTier days(int n, std::chrono::seconds keep = std::chrono::seconds{0});
};

// =========================================================================
// DownsamplingPolicy
// =========================================================================

/**
 * @brief Complete downsampling policy for a single metric.
 *
 * Declares the ordered list of resolution tiers from finest to coarsest.
 * The pipeline computes aggregates in that order: tier[0] reads raw data,
 * tier[1] reads tier[0] output, etc.
 *
 * Example – typical sensor/metrics pipeline:
 * @code
 * DownsamplingPolicy p = DownsamplingPolicy::defaultPolicy("cpu_usage");
 * // Tiers: 1m (7d), 1h (90d), 1d (2y)
 * @endcode
 */
struct DownsamplingPolicy {
    std::string metric;             ///< Source metric name
    std::optional<std::string> entity; ///< Optional entity filter (nullopt = all entities)
    std::vector<DownsamplingTier> tiers; ///< Ordered tiers, finest first

    /// Build a policy with three typical tiers: 1m (7d) → 1h (90d) → 1d (2y)
    static DownsamplingPolicy defaultPolicy(
        const std::string& metric,
        const std::optional<std::string>& entity = std::nullopt);
};

// =========================================================================
// TierSelector
// =========================================================================

/**
 * @brief Selects the coarsest downsampling tier that satisfies a query's resolution.
 *
 * Given a requested query window width (the granularity the caller wants data
 * at), TierSelector returns the tier whose resolution is ≤ requested_resolution
 * with the largest window size, i.e. the coarsest tier that still provides the
 * required detail.
 *
 * This is typically invoked by TSQueryOptimizer to route aggregate reads to
 * the smallest-data-volume tier.
 */
class TierSelector {
public:
    /**
     * @brief Register a downsampling policy so the selector knows which tiers exist.
     */
    void registerPolicy(const DownsamplingPolicy& policy);

    /**
     * @brief Find the best tier for the given metric and requested resolution.
     *
     * @param metric               Source metric name
     * @param requested_resolution The finest granularity the query needs.
     *                             Pass std::chrono::milliseconds{0} to request raw data.
     * @return The name of the selected tier's derived metric in TSStore,
     *         or std::nullopt if no registered tier fits (caller should use raw data).
     */
    std::optional<std::string> selectTier(
        const std::string& metric,
        std::chrono::milliseconds requested_resolution) const;

    /**
     * @brief Returns all registered tiers for a metric, ordered finest→coarsest.
     */
    std::vector<DownsamplingTier> tiersFor(const std::string& metric) const;

private:
    // metric → policy
    std::unordered_map<std::string, DownsamplingPolicy> policies_;
};

// =========================================================================
// DownsamplingPipeline
// =========================================================================

/**
 * @brief Orchestrates multi-tier downsampling for one or more metrics.
 *
 * The pipeline owns a ContinuousAggregateManager and per-metric watermarks.
 * Callers invoke refresh() periodically (e.g. from AggregateScheduler) to
 * process newly ingested data from the last watermark up to `now`.
 *
 * Thread safety: refresh() is NOT thread-safe.  External callers must
 * serialise concurrent refreshes for the same pipeline instance.
 */
class DownsamplingPipeline {
public:
    /**
     * @param store     TSStore to read raw/tier data from and write tier output to.
     *                  Not owned; must outlive the pipeline.
     */
    explicit DownsamplingPipeline(TSStore* store);
    ~DownsamplingPipeline() = default;

    DownsamplingPipeline(const DownsamplingPipeline&) = delete;
    DownsamplingPipeline& operator=(const DownsamplingPipeline&) = delete;

    /**
     * @brief Register a downsampling policy.
     *
     * Must be called before any refresh() for the given metric.
     * Registers the policy's tiers with the internal TierSelector.
     */
    void addPolicy(const DownsamplingPolicy& policy);

    /**
     * @brief Process all registered metrics from their stored watermark up to `to_ms`.
     *
     * For each registered metric, this method iterates through the tier list
     * from finest to coarsest.  Each tier reads from its input source
     * (raw data for tier[0], tier[i-1] output for tier[i]) over the window
     * [watermark, to_ms) and writes aggregate results back to TSStore under
     * the derived metric name produced by ContinuousAggregateManager::derivedMetricName().
     *
     * After a successful write the per-tier watermark is advanced to `to_ms`.
     *
     * @param to_ms  Upper bound of the refresh window (ms since epoch).
     *               Defaults to now() when 0.
     * @return Total number of aggregate data points written across all tiers.
     */
    size_t refresh(int64_t to_ms = 0);

    /**
     * @brief Refresh a single metric.
     *
     * @param metric  Metric name (must be registered via addPolicy()).
     * @param to_ms   Upper bound of refresh window (0 = now).
     * @return Number of aggregate points written.
     */
    size_t refreshMetric(const std::string& metric, int64_t to_ms = 0);

    /**
     * @brief Return the internal TierSelector for integration with TSQueryOptimizer.
     */
    const TierSelector& tierSelector() const { return tier_selector_; }

    /**
     * @brief Get the current watermark for a metric:tier combination.
     *
     * Returns 0 if no data has been processed yet for that tier.
     */
    int64_t getWatermark(const std::string& metric, const std::string& tier_name) const;

    /**
     * @brief Manually set a watermark (e.g. for backfill or disaster recovery).
     */
    void setWatermark(const std::string& metric, const std::string& tier_name, int64_t watermark_ms);

private:
    TSStore* store_;
    ContinuousAggregateManager agg_manager_;
    TierSelector tier_selector_;

    // Registered policies: metric → policy
    std::unordered_map<std::string, DownsamplingPolicy> policies_;

    // Per-tier watermarks: "metric:tier_name" → watermark_ms
    std::unordered_map<std::string, int64_t> watermarks_;

    static std::string watermarkKey(const std::string& metric, const std::string& tier_name);

    // Returns current epoch time in milliseconds
    static int64_t nowMs();

    // Refresh a single tier for a metric, reading from input_metric over [from_ms, to_ms)
    size_t refreshTier(const DownsamplingPolicy& policy,
                       const DownsamplingTier& tier,
                       const std::string& input_metric,
                       int64_t from_ms,
                       int64_t to_ms);
};

} // namespace themis

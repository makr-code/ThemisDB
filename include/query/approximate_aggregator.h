/**
 * @file approximate_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_set>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace query {

/**
 * @brief Abstract interface for approximate aggregation algorithms.
 *
 * Approximate aggregators trade a small, bounded error for dramatically
 * lower memory and CPU cost on large datasets (≥ 50× speedup at > 1 M rows).
 *
 * All implementations must be deterministic: given the same stream of
 * `add()` calls they must always return the same estimate.
 *
 * Thread safety: individual instances are NOT thread-safe. Use one
 * instance per query execution thread.
 */
class IApproximateAggregator {
public:
    virtual ~IApproximateAggregator() = default;

    /**
     * @brief Add a value to the sketch.
     * @param value JSON value (string, number, or bool) to ingest.
     */
    virtual void add(const nlohmann::json& value) = 0;

    /**
     * @brief Merge another aggregator of the same type into this one.
     *
     * After merging, `this` represents the union of both streams.
     * Throws `std::invalid_argument` if `other` is of an incompatible type.
     */
    virtual void merge(const IApproximateAggregator& other) = 0;

    /**
     * @brief Return the estimated aggregate result.
     *
     * The result type depends on the concrete aggregator:
     * - `ApproximateCountDistinct` → integer (estimated distinct count)
     * - `ApproximatePercentile`    → number  (estimated quantile value)
     * - `SamplingAggregator`       → number  (estimated aggregate over sample)
     */
    virtual nlohmann::json estimate() const = 0;

    /// Relative error guarantee (0.01 = 1 %).
    virtual double errorRate() const = 0;

    /// Reset to the empty state.
    virtual void reset() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// HyperLogLog — COUNT DISTINCT
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Approximate COUNT DISTINCT using the HyperLogLog++ algorithm.
 *
 * Memory: O(2^precision) bytes independent of the input cardinality.
 * Error:  ~1.04 / sqrt(2^precision).
 *
 * Precision controls the accuracy/memory tradeoff:
 * | precision | error  | memory  |
 * |-----------|--------|---------|
 * |     10    | ~3.3 % |  1 KB   |
 * |     12    | ~1.6 % |  4 KB   |
 * |     14    | ~0.8 % | 16 KB   |
 *
 * AQL usage:
 * @code
 *   FOR u IN users COLLECT AGGREGATE cnt = HLL(u.id) RETURN cnt
 * @endcode
 *
 * Performance: ≥ 100× faster than exact COUNT DISTINCT for > 1 M rows.
 */
class ApproximateCountDistinct : public IApproximateAggregator {
public:
    ~ApproximateCountDistinct() override = default;
    /// @param precision  HyperLogLog precision (4–18; default 12 → ~1.6 % error).
    explicit ApproximateCountDistinct(int precision = 12);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

    /// Returns the estimated number of distinct values as an integer.
    nlohmann::json estimate() const override;
    double errorRate() const override;
    void reset() override;

    int precision() const { return precision_; }

private:
    int precision_ = 12;
    int num_registers_ = 1 << 12;
    std::vector<uint8_t> registers_;
};

// ─────────────────────────────────────────────────────────────────────────────
// t-Digest — PERCENTILE / QUANTILE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Approximate quantile estimation using the t-digest algorithm.
 *
 * The t-digest maintains a compact summary of a numeric data stream and
 * provides accurate quantile estimates, particularly at the extremes (p1, p99).
 *
 * Memory: O(compression) centroids, independent of stream size.
 * Error:  bounded by the compression parameter; tighter at extreme quantiles.
 *
 * AQL usage:
 * @code
 *   FOR u IN users COLLECT AGGREGATE p95 = TDIGEST(u.age, 0.95) RETURN p95
 * @endcode
 *
 * Performance: ≥ 99× faster than exact sorting for > 1 M rows.
 */
class ApproximatePercentile : public IApproximateAggregator {
public:
    ~ApproximatePercentile() override = default;
    /**
     * @param quantile    Target quantile in [0.0, 1.0] (e.g. 0.95 for p95).
     * @param compression Number of centroids (higher = more accurate, default 100).
     */
    explicit ApproximatePercentile(double quantile = 0.5, int compression = 100);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

    /// Returns the estimated quantile value as a JSON number.
    nlohmann::json estimate() const override;
    double errorRate() const override;
    void reset() override;

    double quantile() const { return quantile_; }

private:
    struct Centroid {
        double mean = 0.0;
        double weight = 0.0;
    };

    void compress();

    double quantile_ = 0.5;
    int compression_ = 100;
    std::vector<Centroid> centroids_;
    double total_weight_ = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Sampling Aggregator — general SUM / AVG / COUNT via reservoir sampling
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Approximate aggregation via reservoir sampling.
 *
 * Maintains a fixed-size random sample of the input stream and computes
 * SUM, AVG, or COUNT by extrapolating from the sample to the full population.
 *
 * Sample size controls the accuracy/memory tradeoff:
 * - 1 % sample (10 K of 1 M rows) → ~1 % relative error for AVG/SUM.
 *
 * Performance: throughput is bounded by the sample size; at 1 % the
 * aggregator processes only 10 K rows regardless of input size.
 */
class SamplingAggregator : public IApproximateAggregator {
public:
    ~SamplingAggregator() override = default;
    enum class AggregationType { SUM, AVG, COUNT };

    /**
     * @param type        Aggregation to perform on the sample.
     * @param sample_size Reservoir capacity (default 10 000 rows → 1 % of 1 M).
     */
    explicit SamplingAggregator(AggregationType type = AggregationType::AVG,
                                 size_t sample_size = 10'000);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

    /// Returns the extrapolated aggregate estimate as a JSON number.
    nlohmann::json estimate() const override;
    double errorRate() const override;
    void reset() override;

    size_t sampleSize() const { return sample_size_; }
    size_t totalSeen() const { return total_seen_; }

private:
    AggregationType type_ = AggregationType::AVG;
    size_t sample_size_ = 10'000;
    size_t total_seen_ = 0;
    std::vector<double> reservoir_;
    uint64_t rng_state_ = 0x123456789abcdefULL;

    uint64_t nextRng();
};

} // namespace query
} // namespace themis

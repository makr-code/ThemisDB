/**
 * @file approximate_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class IApproximateAggregator {
public:
    /**
     * @brief IApproximate Aggregator.
     * @return Return value.
     */
    virtual ~IApproximateAggregator() = default;

    /**
     * @brief Add.
     * @param[in] value Input parameter.
     */
    virtual void add(const nlohmann::json& value) = 0;

    /**
     * @brief Merge.
     * @param[in] other Input parameter.
     */
    virtual void merge(const IApproximateAggregator& other) = 0;

    /**
     * @brief Estimate.
     * @return Return value.
     */
    virtual nlohmann::json estimate() const = 0;

    /**
     * @brief Error Rate.
     * @return Return value.
     */
    virtual double errorRate() const = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// HyperLogLog — COUNT DISTINCT
// ─────────────────────────────────────────────────────────────────────────────

class ApproximateCountDistinct : public IApproximateAggregator {
public:
    ~ApproximateCountDistinct() override = default;
    explicit ApproximateCountDistinct(int precision = 12);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

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

class ApproximatePercentile : public IApproximateAggregator {
public:
    ~ApproximatePercentile() override = default;
    explicit ApproximatePercentile(double quantile = 0.5, int compression = 100);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

    nlohmann::json estimate() const override;
    double errorRate() const override;
    void reset() override;

    double quantile() const { return quantile_; }

private:
    struct Centroid {
        double mean = 0.0;
        double weight = 0.0;
    };

    /**
     * @brief Compress.
     */
    void compress();

    double quantile_ = 0.5;
    int compression_ = 100;
    std::vector<Centroid> centroids_;
    double total_weight_ = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Sampling Aggregator — general SUM / AVG / COUNT via reservoir sampling
// ─────────────────────────────────────────────────────────────────────────────

class SamplingAggregator : public IApproximateAggregator {
public:
    ~SamplingAggregator() override = default;
    enum class AggregationType { SUM, AVG, COUNT };

    explicit SamplingAggregator(AggregationType type = AggregationType::AVG,
                                 size_t sample_size = 10'000);

    void add(const nlohmann::json& value) override;
    void merge(const IApproximateAggregator& other) override;

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

    /**
     * @brief Next Rng.
     * @return Return value.
     */
    uint64_t nextRng();
};

} // namespace query
} // namespace themis

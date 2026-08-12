/**
 * @file anomaly_detection.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "timeseries/tsstore.h"
#include <string>
#include <vector>

namespace themis {

// ============================================================================
// AnomalyPoint
// ============================================================================

/**
 * @brief A single detected anomaly in a time series.
 */
struct AnomalyPoint {
    int64_t     timestamp_ms{0};
    double      value{0.0};

    /// Detection score:
    ///   - For ZScore: |z-score| (higher = more anomalous)
    ///   - For IQR: signed distance from the nearest fence
    ///             (negative = below lower fence, positive = above upper fence)
    double      score{0.0};

    /// Detection method that flagged this point ("zscore" or "iqr").
    std::string method;
};

// ============================================================================
// AnomalyMethod / AnomalyConfig
// ============================================================================

enum class AnomalyMethod { ZScore, IQR, Both };

struct AnomalyConfig {
    AnomalyMethod method{AnomalyMethod::ZScore};

    /// Z-score threshold: a point is anomalous when |z| > zscore_threshold.
    double zscore_threshold{3.0};

    /// IQR multiplier k: fences are Q1 − k·IQR and Q3 + k·IQR (Tukey, 1977).
    double iqr_multiplier{1.5};

    /// Minimum number of points required to compute statistics.
    /// Fewer points → detect() returns an empty result.
    size_t min_samples{4};
};

// ============================================================================
// IAnomalyDetector
// ============================================================================

/**
 * @brief Common interface for time-series anomaly detectors.
 */
class IAnomalyDetector {
public:
    virtual ~IAnomalyDetector() = default;

    /**
     * @brief Detect anomalous data points in @p points.
     *
     * @param points  Input series (order is preserved; sorting not required).
     * @param cfg     Detection configuration.
     * @return        Subset of @p points that are considered anomalous,
     *                annotated with the detection score and method name.
     */
    virtual std::vector<AnomalyPoint> detect(
        const std::vector<TSStore::DataPoint>& points,
        const AnomalyConfig& cfg) const = 0;
};

// ============================================================================
// ZScoreDetector
// ============================================================================

/**
 * @brief Anomaly detector based on population Z-score.
 *
 * z(x) = (x − μ) / σ,  where μ = sample mean, σ = sample standard deviation.
 * A point is flagged as anomalous when |z(x)| > AnomalyConfig::zscore_threshold.
 *
 * When σ ≈ 0 (constant series) no anomalies are reported.
 */
class ZScoreDetector : public IAnomalyDetector {
public:
    std::vector<AnomalyPoint> detect(
        const std::vector<TSStore::DataPoint>& points,
        const AnomalyConfig& cfg) const override;

    /**
     * @brief Return the z-score for every input point.
     *
     * @return Vector of (timestamp_ms, z_score) pairs in input order.
     *         Returns an empty vector when @p points has fewer than 2 elements
     *         or when the standard deviation is effectively zero.
     */
    std::vector<std::pair<int64_t, double>> scoreAll(
        const std::vector<TSStore::DataPoint>& points) const;
};

// ============================================================================
// IQRDetector
// ============================================================================

/**
 * @brief Anomaly detector based on interquartile range (Tukey fences).
 *
 * Lower fence = Q1 − k·IQR,  upper fence = Q3 + k·IQR  (k = iqr_multiplier).
 * A point is flagged when it falls outside [lower_fence, upper_fence].
 *
 * The score field in AnomalyPoint is set to:
 *   - (value − upper_fence) when value > upper_fence (positive)
 *   - (value − lower_fence) when value < lower_fence (negative)
 */
class IQRDetector : public IAnomalyDetector {
public:
    std::vector<AnomalyPoint> detect(
        const std::vector<TSStore::DataPoint>& points,
        const AnomalyConfig& cfg) const override;
};

// ============================================================================
// AnomalyDetector (combined)
// ============================================================================

/**
 * @brief Configurable anomaly detector that delegates to ZScoreDetector,
 *        IQRDetector, or both.
 *
 * When AnomalyMethod::Both is selected, the union of anomalies detected by
 * either method is returned; duplicate timestamps are de-duplicated, keeping
 * the entry with the higher absolute score.
 *
 * ### Example
 * @code
 *   AnomalyConfig cfg;
 *   cfg.method             = AnomalyMethod::ZScore;
 *   cfg.zscore_threshold   = 3.0;
 *
 *   AnomalyDetector detector(cfg);
 *   auto anomalies = detector.detect(points);
 * @endcode
 */
class AnomalyDetector : public IAnomalyDetector {
public:
    explicit AnomalyDetector(AnomalyConfig cfg = {});

    /**
     * @brief Detect anomalies using the stored configuration.
     */
    std::vector<AnomalyPoint> detect(
        const std::vector<TSStore::DataPoint>& points) const;

    /**
     * @brief Detect anomalies using an explicit configuration (overrides stored).
     */
    std::vector<AnomalyPoint> detect(
        const std::vector<TSStore::DataPoint>& points,
        const AnomalyConfig& cfg) const override;

    void setConfig(const AnomalyConfig& cfg) { config_ = cfg; }
    const AnomalyConfig& config() const { return config_; }

private:
    AnomalyConfig config_;
};

} // namespace themis

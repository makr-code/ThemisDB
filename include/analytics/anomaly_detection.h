/**
 * @file anomaly_detection.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Real-Time Anomaly Detection Engine
 *
 * Provides offline training + online inference for several anomaly-detection
 * algorithms with no external ML dependencies.  Designed to integrate with
 * the CEP engine (event-stream anomaly detection) and the OLAP engine
 * (historical dataset anomaly detection).
 *
 * Supported algorithms:
 *   - Z_SCORE            – per-feature standardised score (Gaussian assumption)
 *   - MODIFIED_Z_SCORE   – robust variant using median absolute deviation (MAD)
 *   - IQR                – inter-quartile range fence method
 *   - ISOLATION_FOREST   – unsupervised approximate tree-based isolation
 *   - LOF                – Local Outlier Factor (density-based, k-NN)
 *   - ENSEMBLE           – weighted combination of the above detectors
 *
 * Thread-safety:
 *   - AnomalyDetector: train() is NOT thread-safe; predict/explain are.
 *   - StreamingAnomalyDetector: fully thread-safe.  Two independent mutexes:
 *     window_mu_ guards the deque/anomaly list; detector_mu_ guards the model.
 *     train() runs entirely off both locks; only a brief exclusive swap of the
 *     newly-trained model acquires detector_mu_.  Concurrent predict() calls
 *     share detector_mu_ and never block each other or window updates.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <variant>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class AnomalyDetector;
class StreamingAnomalyDetector;

// ============================================================================
// Value type (mirrors RecordValue from streaming_window.h)
// ============================================================================

using PointValue = std::variant<
    std::monostate,   // null
    bool,
    int64_t,
    double,
    std::string
>;

// ============================================================================
// Algorithms
// ============================================================================

enum class AnomalyMethod {
    Z_SCORE,          ///< Standardised z-score; simple, fast
    MODIFIED_Z_SCORE, ///< MAD-based robust z-score
    IQR,              ///< Interquartile-range fence
    ISOLATION_FOREST, ///< Approximate isolation forest
    LOF,              ///< Local Outlier Factor
    ENSEMBLE          ///< Weighted combination of all detectors
};

// ============================================================================
// DataPoint
// ============================================================================

struct DataPoint {
    std::string id = {};
    int64_t     timestamp_ms = 0;
    std::map<std::string, PointValue> fields;

    // Convenience helpers
    /**
     * @brief Set.
     * @param[in] name Input parameter.
     * @param[in] v Input parameter.
     * @details Implements set without additional internal calls.
     */
    void set(const std::string& name, double v)      { fields[name] = v; }
    /**
     * @brief Set.
     * @param[in] name Input parameter.
     * @param[in] v Input parameter.
     * @details Implements set without additional internal calls.
     */
    void set(const std::string& name, int64_t v)     { fields[name] = v; }
    /**
     * @brief Set.
     * @param[in] name Input parameter.
     * @param[in] v Input parameter.
     * @details Implements set without additional internal calls.
     */
    void set(const std::string& name, const std::string& v) { fields[name] = v; }
    /**
     * @brief Set.
     * @param[in] name Input parameter.
     * @param[in] v Input parameter.
     * @details Implements set without additional internal calls.
     */
    void set(const std::string& name, bool v)        { fields[name] = v; }

    template<typename T>
    std::optional<T> get(const std::string& name) const {
        auto it = fields.find(name);
        if (it == fields.end()) {
          return std::nullopt;
        }
        if (auto* p = std::get_if<T>(&it->second)) {
          return *p;
        }
        return std::nullopt;
    }

    /**
     * @brief Numeric Features.
     * @return Return value.
     */
    std::vector<double> numericFeatures() const;

    /**
     * @brief Numeric Field Names.
     * @return Return value.
     */
    std::vector<std::string> numericFieldNames() const;
};

// ============================================================================
// AnomalyResult
// ============================================================================

struct AnomalyResult {
    std::string   id;
    double        score      = 0.0;   ///< 0.0 = definitely normal; 1.0 = definite anomaly
    bool          is_anomaly = false; ///< score > threshold
    AnomalyMethod method     = AnomalyMethod::Z_SCORE;
    int64_t       timestamp_ms = 0;
    std::string   description;        ///< human-readable summary
};

// ============================================================================
// AnomalyExplanation
// ============================================================================

struct AnomalyExplanation {
    std::string id;
    double      score = 0.0;
    std::vector<std::pair<std::string, double>> feature_contributions; ///< sorted desc by abs contribution
    std::string description;
};

// ============================================================================
// DetectorConfig
// ============================================================================

struct DetectorConfig {
    AnomalyMethod method         = AnomalyMethod::Z_SCORE;
    double        contamination  = 0.1;   ///< expected fraction of anomalies in training data
    double        threshold      = 0.6;   ///< score ≥ threshold → is_anomaly = true
    int           n_estimators   = 100;   ///< Isolation Forest number of trees
    int           max_samples    = 256;   ///< Isolation Forest subsampling
    int           k_neighbors    = 5;     ///< LOF neighbourhood size
    bool          adaptive       = false; ///< update model online via update()
    std::vector<AnomalyMethod> ensemble_methods;
    std::vector<double>        ensemble_weights;
};

// ============================================================================
// AnomalyDetector
// ============================================================================

class AnomalyDetector {
public:
    // ---- Construction ----
    explicit AnomalyDetector(AnomalyMethod method = AnomalyMethod::Z_SCORE);
    /**
     * @brief Anomaly Detector.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AnomalyDetector(const DetectorConfig& config);
    ~AnomalyDetector();

    // Non-copyable; movable
    AnomalyDetector(const AnomalyDetector&)            = delete;
    AnomalyDetector& operator=(const AnomalyDetector&) = delete;
    AnomalyDetector(AnomalyDetector&&)                 noexcept;
    AnomalyDetector& operator=(AnomalyDetector&&)      noexcept;

    /**
     * @brief Train.
     * @param[in] data Input parameter.
     */
    void train(const std::vector<DataPoint>& data);
    /**
     * @brief Is Trained.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isTrained() const noexcept;

    /**
     * @brief Predict.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    AnomalyResult             predict(const DataPoint& point) const;
    /**
     * @brief Predict Batch.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<AnomalyResult> predictBatch(const std::vector<DataPoint>& data) const;

    /**
     * @brief Explain.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    AnomalyExplanation explain(const DataPoint& point) const;

    /**
     * @brief Update.
     * @param[in] point Input parameter.
     */
    void update(const DataPoint& point);

    /**
     * @brief Serialize.
     * @return Return value.
     */
    std::string serialize() const;
    /**
     * @brief Deserialize.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static AnomalyDetector deserialize(const std::string& data);

    // ---- Diagnostics ----
    struct ModelStats {
        size_t              training_samples = 0;
        double              contamination    = 0.1;
        size_t              n_features       = 0;
        std::vector<std::string> feature_names;
        std::vector<double> feature_means;
        std::vector<double> feature_stddevs;
        std::vector<double> feature_medians;
        std::vector<double> feature_mads;
        AnomalyMethod       method           = AnomalyMethod::Z_SCORE;
        bool                trained          = false;
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    ModelStats getStats() const;

    /**
     * @brief Config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const DetectorConfig& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// StreamingAnomalyDetector
// ============================================================================

class StreamingAnomalyDetector {
public:
    struct Config {
        AnomalyMethod method            = AnomalyMethod::Z_SCORE;
        double        threshold         = 0.6;
        size_t        window_size       = 1000; ///< max training window size
        bool          auto_train        = true;
        size_t        auto_train_after  = 100;  ///< start detecting after this many points
        bool          retrain_on_window = true; ///< retrain when window fills up
    };

    StreamingAnomalyDetector();
    /**
     * @brief Streaming Anomaly Detector.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit StreamingAnomalyDetector(const Config& config);

    ~StreamingAnomalyDetector();

    /**
     * @brief Process.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    std::optional<AnomalyResult> process(const DataPoint& point);

    /**
     * @brief Get Anomalies.
     * @return Return value.
     */
    std::vector<AnomalyResult> getAnomalies() const;

    /**
     * @brief Clear Anomalies.
     */
    void clearAnomalies();

    struct WindowStats {
        size_t window_size = 0;     ///< current window size (may be < config window_size during warm-up)
        size_t anomaly_count;
        double anomaly_rate;
        bool   trained;
    };
    /**
     * @brief Get Window Stats.
     * @return Return value.
     */
    WindowStats getWindowStats() const;

    const AnomalyDetector& detector() const noexcept { return detector_; }

private:
    /**
     * @brief Snapshot Window.
     * @return Return value.
     */
    std::vector<DataPoint> snapshotWindow() const;
    /**
     * @brief Make Detector Config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    DetectorConfig makeDetectorConfig() const noexcept;

    Config                           config_;
    AnomalyDetector                  detector_;
    mutable std::shared_mutex        window_mu_;   ///< guards window_, anomalies_, points_seen_
    mutable std::shared_mutex        detector_mu_; ///< guards detector_; held only for brief swap
    std::deque<DataPoint>            window_;
    std::vector<AnomalyResult>       anomalies_;
    size_t                           points_seen_ = 0;
    std::atomic<bool>                retraining_{false};
    std::atomic<bool>                stopping_{false};  ///< set in dtor to prevent new retrains
    std::future<void>                retrain_future_;
};

// ============================================================================
// Free helpers
// ============================================================================

inline const char* anomalyMethodName(AnomalyMethod m) noexcept {
    switch (m) {
        case AnomalyMethod::Z_SCORE:          return "Z_SCORE";
        case AnomalyMethod::MODIFIED_Z_SCORE: return "MODIFIED_Z_SCORE";
        case AnomalyMethod::IQR:              return "IQR";
        case AnomalyMethod::ISOLATION_FOREST: return "ISOLATION_FOREST";
        case AnomalyMethod::LOF:              return "LOF";
        case AnomalyMethod::ENSEMBLE:         return "ENSEMBLE";
        default:                              return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb

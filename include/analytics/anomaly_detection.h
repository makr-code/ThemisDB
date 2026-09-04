/**
 * @file anomaly_detection.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * A labelled feature vector fed into the anomaly detector.
 * Numeric fields (int64 / double) are used for detection; string fields are
 * treated as metadata and preserved in results / explanations.
 */
struct DataPoint {
    std::string id = {};
    int64_t     timestamp_ms = 0;
    std::map<std::string, PointValue> fields;

    // Convenience helpers
    void set(const std::string& name, double v)      { fields[name] = v; }
    void set(const std::string& name, int64_t v)     { fields[name] = v; }
    void set(const std::string& name, const std::string& v) { fields[name] = v; }
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

    /// Extract all numeric features in a deterministic (sorted-key) order.
    std::vector<double> numericFeatures() const;

    /// Return the sorted names of numeric features.
    std::vector<std::string> numericFieldNames() const;
};

// ============================================================================
// AnomalyResult
// ============================================================================

/**
 * Result for a single data point.
 */
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

/**
 * Per-feature contribution breakdown for one anomalous data point.
 */
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
    /// Methods used when method == ENSEMBLE (default: all except ENSEMBLE itself)
    std::vector<AnomalyMethod> ensemble_methods;
    /// Weights for ensemble methods (same order as ensemble_methods; uniform if empty)
    std::vector<double>        ensemble_weights;
};

// ============================================================================
// AnomalyDetector
// ============================================================================

/**
 * Offline-train + online-predict anomaly detector.
 *
 * Usage:
 * @code
 *   AnomalyDetector det(AnomalyMethod::ISOLATION_FOREST);
 *   det.train(normal_points);
 *
 *   auto result = det.predict(new_point);
 *   if (result.is_anomaly) {
 *       auto exp = det.explain(new_point);
 *       // exp.feature_contributions gives per-feature breakdown
 *   }
 * @endcode
 */
class AnomalyDetector {
public:
    // ---- Construction ----
    explicit AnomalyDetector(AnomalyMethod method = AnomalyMethod::Z_SCORE);
    explicit AnomalyDetector(const DetectorConfig& config);
    ~AnomalyDetector();

    // Non-copyable; movable
    AnomalyDetector(const AnomalyDetector&)            = delete;
    AnomalyDetector& operator=(const AnomalyDetector&) = delete;
    AnomalyDetector(AnomalyDetector&&)                 noexcept;
    AnomalyDetector& operator=(AnomalyDetector&&)      noexcept;

    // ---- Training ----
    /**
     * Train on a set of representative (ideally normal) data points.
     * Must be called before predict/explain.
     */
    void train(const std::vector<DataPoint>& data);
    bool isTrained() const noexcept;

    // ---- Inference ----
    AnomalyResult             predict(const DataPoint& point) const;
    std::vector<AnomalyResult> predictBatch(const std::vector<DataPoint>& data) const;

    // ---- Explanation ----
    AnomalyExplanation explain(const DataPoint& point) const;

    // ---- Adaptive learning (only when config.adaptive == true) ----
    /**
     * Incorporate a new observation into the model (sliding-window statistics).
     * For Isolation Forest / LOF this rebuilds a partial model efficiently.
     */
    void update(const DataPoint& point);

    // ---- Serialisation ----
    std::string serialize() const;
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
    ModelStats getStats() const;

    const DetectorConfig& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// StreamingAnomalyDetector
// ============================================================================

/**
 * Stateful, thread-safe wrapper for real-time stream anomaly detection.
 *
 * Maintains a sliding window of the most recent `window_size` data points.
 * When the window is full the model is (re-)trained automatically.
 *
 * Usage:
 * @code
 *   StreamingAnomalyDetector sad({
 *       .method          = AnomalyMethod::IQR,
 *       .window_size     = 500,
 *       .auto_train      = true,
 *       .auto_train_after = 100,
 *   });
 *
 *   while (auto point = stream.next()) {
 *       auto result = sad.process(*point);
 *       if (result && result->is_anomaly)
 *           alert(*result);
 *   }
 * @endcode
 */
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
    explicit StreamingAnomalyDetector(const Config& config);

    /**
     * Destructor: sets the stopping flag to prevent new async retrains from
     * launching, then waits for any in-flight background retrain to finish
     * before members (`mu_`, `detector_`, etc.) are destroyed.
     */
    ~StreamingAnomalyDetector();

    /**
     * Process a new data point.
     * Returns an AnomalyResult only when the detector is already trained.
     * Returns nullopt while warming up.
     * Lock-hold is bounded to ≤ 50 µs (window copy only); training runs
     * asynchronously on a background thread.
     */
    std::optional<AnomalyResult> process(const DataPoint& point);

    /** Retrieve all anomalies detected since construction (or last clear). */
    std::vector<AnomalyResult> getAnomalies() const;

    /** Clear stored anomaly history. */
    void clearAnomalies();

    struct WindowStats {
        size_t window_size = 0;     ///< current window size (may be < config window_size during warm-up)
        size_t anomaly_count;
        double anomaly_rate;
        bool   trained;
    };
    WindowStats getWindowStats() const;

    /** Direct access to the underlying detector (read-only). */
    const AnomalyDetector& detector() const noexcept { return detector_; }

private:
    /** Copy the current window under a brief shared lock and return it. */
    std::vector<DataPoint> snapshotWindow() const;
    /** Build a DetectorConfig from config_ (used when creating a fresh retrain detector). */
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

/**
 * Convert an anomaly method enum to a human-readable string.
 */
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

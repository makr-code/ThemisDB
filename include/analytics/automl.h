/**
 * @file automl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB AutoML Engine
 *
 * Automated Machine Learning for analytics tasks.  Implements automated
 * model selection, hyperparameter tuning, feature engineering, ensemble
 * generation, and SHAP-based model interpretation.
 *
 * Algorithms (pure C++17, no external ML dependencies):
 *   Classification:
 *     - LOGISTIC_REGRESSION  – L2-regularised logistic regression (SGD)
 *     - DECISION_TREE        – CART with Gini/entropy split criterion
 *     - RANDOM_FOREST        – Ensemble of CART trees (bagging)
 *     - GRADIENT_BOOSTING    – Gradient-boosted decision trees
 *     - KNN                  – k-Nearest Neighbours
 *   Regression:
 *     - LINEAR_REGRESSION    – OLS with optional L2 regularisation
 *     - DECISION_TREE        – CART with MSE split criterion
 *     - RANDOM_FOREST        – Ensemble of regression trees
 *     - GRADIENT_BOOSTING    – Gradient-boosted decision trees
 *     - KNN                  – k-Nearest Neighbours regression
 *
 * Hyperparameter search:
 *   - Random search with configurable budget (max_time_minutes / max_trials)
 *   - k-fold cross-validation for evaluation
 *
 * Feature engineering (optional, enabled via AutoMLConfig::feature_engineering):
 *   - Standard scaling (zero-mean, unit-variance)
 *   - Polynomial feature expansion (degree 2)
 *   - Categorical one-hot encoding
 *
 * Ensembling:
 *   - Soft-voting / mean-regression over best-k models
 *
 * Interpretation:
 *   - Per-sample feature contributions via permutation-based SHAP approximation
 *
 * Thread-safety:
 *   - AutoML::train* methods are NOT thread-safe (training modifies state).
 *   - AutoMLModel::predict / explain are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Re-use DataPoint from the anomaly-detection module for consistency.
#include "analytics/anomaly_detection.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class AutoMLModel;
class AutoML;

// ============================================================================
// Enumerations
// ============================================================================

/** ML task type. */
enum class AutoMLTask {
    CLASSIFICATION, ///< Predict a discrete class label
    REGRESSION      ///< Predict a continuous value
};

/** Candidate model algorithm. */
enum class ModelAlgorithm {
    LOGISTIC_REGRESSION, ///< L2-regularised logistic regression (classification)
    LINEAR_REGRESSION,   ///< OLS / ridge regression (regression)
    DECISION_TREE,       ///< CART decision tree (classification or regression)
    RANDOM_FOREST,       ///< Ensemble of CART trees via bagging
    GRADIENT_BOOSTING,   ///< Gradient-boosted decision trees
    KNN,                 ///< k-Nearest Neighbours
    ENSEMBLE             ///< AutoML-generated soft-voting ensemble
};

/** Primary evaluation metric. */
enum class AutoMLMetric {
    // Classification
    ACCURACY,   ///< Fraction of correctly classified samples
    F1,         ///< Macro-averaged F1
    PRECISION,  ///< Macro-averaged precision
    RECALL,     ///< Macro-averaged recall
    AUC_ROC,    ///< Area under the ROC curve (binary only)
    // Regression
    R2,         ///< Coefficient of determination
    RMSE,       ///< Root mean squared error (lower is better)
    MAE,        ///< Mean absolute error (lower is better)
    MAPE        ///< Mean absolute percentage error (lower is better)
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * Configuration for an AutoML training run.
 */
struct AutoMLConfig {
    std::string  target;                        ///< Name of the target field in DataPoint
    AutoMLTask   task          = AutoMLTask::CLASSIFICATION;
    AutoMLMetric metric        = AutoMLMetric::F1;
    int          max_time_minutes  = 5;         ///< Wall-clock budget
    int          max_trials        = 50;        ///< Maximum hyperparameter trials
    int          cv_folds          = 3;         ///< Cross-validation folds
    bool         feature_engineering = true;   ///< Enable automated feature engineering
    bool         ensemble          = true;      ///< Generate a voting/averaging ensemble
    int          ensemble_top_k    = 3;         ///< Top-k models used in ensemble
    int          random_seed       = 42;
    /// Subset of algorithms to consider (empty = all suitable algorithms)
    std::vector<ModelAlgorithm> algorithms;
};

// ============================================================================
// Evaluation metrics
// ============================================================================

/** Metrics computed after training / evaluation. */
struct EvalMetrics {
    double accuracy   = 0.0;
    double f1         = 0.0;
    double precision  = 0.0;
    double recall     = 0.0;
    double auc_roc    = 0.0;
    double r2         = 0.0;
    double rmse       = 0.0;
    double mae        = 0.0;
    double mape       = 0.0;

    /** Return the value of the requested primary metric. */
    double primary(AutoMLMetric m) const noexcept;
};

// ============================================================================
// Model explanation
// ============================================================================

/**
 * SHAP-approximated explanation for a single data-point prediction.
 */
struct ModelExplanation {
    std::string id;                                              ///< DataPoint id
    double      predicted_value = 0.0;                           ///< Raw model output
    std::string predicted_label;                                 ///< Class label (classification)
    double      confidence      = 0.0;                           ///< Confidence / probability
    /// Feature contributions sorted descending by absolute value.
    std::vector<std::pair<std::string, double>> feature_contributions;
    /// Comma-separated top feature names (convenience accessor).
    std::string top_features;
    std::string description;
};

// ============================================================================
// Trained model metadata
// ============================================================================

/** Information about one candidate model produced during the search. */
struct CandidateModelInfo {
    ModelAlgorithm algorithm    = ModelAlgorithm::DECISION_TREE;
    std::string    name;            ///< human-readable name
    EvalMetrics    cv_metrics;      ///< cross-validation metrics
    double         cv_score        = 0.0; ///< primary metric on CV
    double         train_time_ms   = 0.0;
    std::map<std::string, double> hyperparameters;
};

// ============================================================================
// AutoMLModel  (trained, predict-ready model)
// ============================================================================

/**
 * A trained AutoML model.
 *
 * Returned by AutoML::trainClassifier / trainRegressor.  Thread-safe after
 * construction; predict / explain may be called concurrently.
 *
 * @code
 *   AutoML automl;
 *   auto model = automl.trainClassifier(data, {
 *       .target  = "label",
 *       .metric  = AutoMLMetric::F1,
 *       .max_time_minutes = 10
 *   });
 *
 *   auto preds = model.predict(test_data);
 *   auto exps  = model.explain(test_data);
 * @endcode
 */
class AutoMLModel {
public:
    // ---- Construction (only AutoML creates these) ----
    AutoMLModel();
    ~AutoMLModel();

    AutoMLModel(const AutoMLModel&)            = delete;
    AutoMLModel& operator=(const AutoMLModel&) = delete;
    AutoMLModel(AutoMLModel&&)                 noexcept;
    AutoMLModel& operator=(AutoMLModel&&)      noexcept;

    // ---- Inference ----

    /**
     * Predict class labels or regression values for a batch of DataPoints.
     * Returns one string per input point:
     *   – Classification: the class label.
     *   – Regression:     std::to_string(value).
     */
    std::vector<std::string> predict(const std::vector<DataPoint>& data) const;

    /**
     * Predict a single DataPoint.
     */
    std::string predictOne(const DataPoint& point) const;

    /**
     * Return class probabilities for a batch (classification only).
     * Outer vector: one entry per data point.
     * Inner map: class label → probability in [0,1].
     */
    std::vector<std::map<std::string, double>>
    predictProba(const std::vector<DataPoint>& data) const;

    // ---- Explanation ----

    /**
     * Compute per-sample SHAP-approximated feature contributions.
     */
    std::vector<ModelExplanation> explain(const std::vector<DataPoint>& data) const;

    /**
     * Explain a single data point.
     */
    ModelExplanation explainOne(const DataPoint& point) const;

    // ---- Metadata ----

    AutoMLTask     task()      const noexcept;
    ModelAlgorithm algorithm() const noexcept;
    std::string    name()      const noexcept;
    EvalMetrics    metrics()   const noexcept;

    /** List of candidate models evaluated during search (sorted by cv_score desc). */
    std::vector<CandidateModelInfo> candidateModels() const;

    /** Feature importance (sum of |SHAP| over training set, normalised to [0,1]). */
    std::map<std::string, double> featureImportance() const;

    // ---- Serialisation ----
    std::string   serialize()   const;
    static AutoMLModel deserialize(const std::string& data);

    /**
     * Export the trained model to an ONNX-compatible text representation.
     *
     * Serialises the model weights, algorithm type, and feature schema into a
     * JSON-ONNX text file at @p path.  The output is loadable by
     * `MLServingClient` when the `THEMIS_HAS_ONNX_RUNTIME` flag is set; on
     * platforms without ONNX Runtime the file can be used for offline tooling.
     *
     * Supported algorithms (all others return Status::UNSUPPORTED_OPERATION):
     *   LinearRegression, LogisticRegression, DecisionTree, RandomForest,
     *   GradientBoosting, KNN (all exported as ONNX-JSON text format v0.1).
     *
     * @param path  Absolute or relative file-system path for the output file.
     * @return      Empty string on success; error message on failure.
     * @throws      std::invalid_argument if the model is not fitted.
     */
    std::string exportONNX(const std::string& path) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    friend class AutoML;
};

// ============================================================================
// AutoML  (training façade)
// ============================================================================

/**
 * Automated Machine Learning engine.
 *
 * Runs a time-bounded random hyperparameter search, evaluates candidates via
 * k-fold cross-validation, optionally builds an ensemble of the top-k models,
 * and returns a ready-to-use AutoMLModel.
 */
class AutoML {
public:
    AutoML();
    ~AutoML();

    AutoML(const AutoML&)            = delete;
    AutoML& operator=(const AutoML&) = delete;

    // ---- Classification ----

    /**
     * Train a classification model.
     *
     * @param data     Training DataPoints; each must contain the target field
     *                 as a string (class label) or int64/double (class index).
     * @param config   AutoML configuration.
     * @param progress Optional callback invoked after each trial:
     *                 (trial_index, total_trials, best_score_so_far).
     */
    AutoMLModel trainClassifier(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        std::function<void(int, int, double)> progress = nullptr);

    // ---- Regression ----

    /**
     * Train a regression model.
     *
     * @param data   Training DataPoints; target field must be numeric.
     * @param config AutoML configuration.
     */
    AutoMLModel trainRegressor(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        std::function<void(int, int, double)> progress = nullptr);

    // ---- Cross-validation helper ----

    /**
     * Evaluate a single algorithm with given hyperparameters via k-fold CV.
     * Returns cross-validated EvalMetrics.
     */
    EvalMetrics crossValidate(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        ModelAlgorithm algorithm,
        const std::map<std::string, double>& hyperparameters) const;

    // ---- Helper functions (Phase 2B) ----

    /**
     * @brief Validate training feature matrix structure and quality.
     * 
     * Checks:
     * - Non-empty data (n_samples > 0)
     * - Consistent dimensions (all rows have same n_features)
     * - No NaN or Inf values
     * - At least 2 samples for meaningful training
     * - For classification: at least 2 distinct classes
     * 
     * @param features Training feature matrix (n_samples × n_features)
     * @param target Target vector (n_samples,)
     * @param task Classification or regression task
     * @return Status::OK() if valid; Status::Error(msg) with specific validation error
     * 
     * @code
     *   std::vector<std::vector<double>> X = {{ 1.0, 2.0 }, { 3.0, 4.0 }};
     *   std::vector<double> y = { 0.0, 1.0 };
     *   auto status = automl.validateTrainingData(X, y, AutoMLTask::CLASSIFICATION);
     * @endcode
     */
    std::pair<bool, std::string> validateTrainingData(
        const std::vector<std::vector<double>>& features,
        const std::vector<double>& target,
        AutoMLTask task = AutoMLTask::CLASSIFICATION) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Free helpers
// ============================================================================

/** Convert AutoMLTask to a human-readable string. */
inline const char* automlTaskName(AutoMLTask t) noexcept {
    switch (t) {
        case AutoMLTask::CLASSIFICATION: return "CLASSIFICATION";
        case AutoMLTask::REGRESSION:     return "REGRESSION";
        default:                         return "UNKNOWN";
    }
}

/** Convert ModelAlgorithm to a human-readable string. */
inline const char* modelAlgorithmName(ModelAlgorithm a) noexcept {
    switch (a) {
        case ModelAlgorithm::LOGISTIC_REGRESSION: return "LOGISTIC_REGRESSION";
        case ModelAlgorithm::LINEAR_REGRESSION:   return "LINEAR_REGRESSION";
        case ModelAlgorithm::DECISION_TREE:       return "DECISION_TREE";
        case ModelAlgorithm::RANDOM_FOREST:       return "RANDOM_FOREST";
        case ModelAlgorithm::GRADIENT_BOOSTING:   return "GRADIENT_BOOSTING";
        case ModelAlgorithm::KNN:                 return "KNN";
        case ModelAlgorithm::ENSEMBLE:            return "ENSEMBLE";
        default:                                  return "UNKNOWN";
    }
}

/** Convert AutoMLMetric to a human-readable string. */
inline const char* automlMetricName(AutoMLMetric m) noexcept {
    switch (m) {
        case AutoMLMetric::ACCURACY:   return "ACCURACY";
        case AutoMLMetric::F1:         return "F1";
        case AutoMLMetric::PRECISION:  return "PRECISION";
        case AutoMLMetric::RECALL:     return "RECALL";
        case AutoMLMetric::AUC_ROC:    return "AUC_ROC";
        case AutoMLMetric::R2:         return "R2";
        case AutoMLMetric::RMSE:       return "RMSE";
        case AutoMLMetric::MAE:        return "MAE";
        case AutoMLMetric::MAPE:       return "MAPE";
        default:                       return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb

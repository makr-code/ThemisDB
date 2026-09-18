/**
 * @file automl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class AutoMLTask {
    CLASSIFICATION, ///< Predict a discrete class label
    REGRESSION      ///< Predict a continuous value
};

enum class ModelAlgorithm {
    LOGISTIC_REGRESSION, ///< L2-regularised logistic regression (classification)
    LINEAR_REGRESSION,   ///< OLS / ridge regression (regression)
    DECISION_TREE,       ///< CART decision tree (classification or regression)
    RANDOM_FOREST,       ///< Ensemble of CART trees via bagging
    GRADIENT_BOOSTING,   ///< Gradient-boosted decision trees
    KNN,                 ///< k-Nearest Neighbours
    ENSEMBLE             ///< AutoML-generated soft-voting ensemble
};

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
    std::vector<ModelAlgorithm> algorithms;
};

// ============================================================================
// Evaluation metrics
// ============================================================================

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

    /**
     * @brief Primary.
     * @param[in] m Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    double primary(AutoMLMetric m) const noexcept;
};

// ============================================================================
// Model explanation
// ============================================================================

struct ModelExplanation {
    std::string id;                                              ///< DataPoint id
    double      predicted_value = 0.0;                           ///< Raw model output
    std::string predicted_label;                                 ///< Class label (classification)
    double      confidence      = 0.0;                           ///< Confidence / probability
    std::vector<std::pair<std::string, double>> feature_contributions;
    std::string top_features;
    std::string description;
};

// ============================================================================
// Trained model metadata
// ============================================================================

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

class AutoMLModel {
public:
    // ---- Construction (only AutoML creates these) ----
    AutoMLModel();
    ~AutoMLModel();

    AutoMLModel(const AutoMLModel&)            = delete;
    AutoMLModel& operator=(const AutoMLModel&) = delete;
    AutoMLModel(AutoMLModel&&)                 noexcept;
    AutoMLModel& operator=(AutoMLModel&&)      noexcept;

    /**
     * @brief ---- Inference ----
     * @param[in] data Input parameter.
     * @return Return value.
     */

    std::vector<std::string> predict(const std::vector<DataPoint>& data) const;

    /**
     * @brief Predict One.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    std::string predictOne(const DataPoint& point) const;

    std::vector<std::map<std::string, double>>
    predictProba(const std::vector<DataPoint>& data) const;

    /**
     * @brief ---- Explanation ----
     * @param[in] data Input parameter.
     * @return Return value.
     */

    std::vector<ModelExplanation> explain(const std::vector<DataPoint>& data) const;

    /**
     * @brief Explain One.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    ModelExplanation explainOne(const DataPoint& point) const;

    /**
     * @brief ---- Metadata ----
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    AutoMLTask     task()      const noexcept;
    /**
     * @brief Algorithm.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    ModelAlgorithm algorithm() const noexcept;
    /**
     * @brief Name.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::string    name()      const noexcept;
    /**
     * @brief Metrics.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    EvalMetrics    metrics()   const noexcept;

    /**
     * @brief Candidate Models.
     * @return Return value.
     */
    std::vector<CandidateModelInfo> candidateModels() const;

    std::map<std::string, double> featureImportance() const;

    /**
     * @brief ---- Serialisation ----
     * @return Return value.
     */
    std::string   serialize()   const;
    /**
     * @brief Deserialize.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static AutoMLModel deserialize(const std::string& data);

    /**
     * @brief Export ONNX.
     * @param[in] path Input parameter.
     * @return Return value.
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

class AutoML {
public:
    AutoML();
    ~AutoML();

    AutoML(const AutoML&)            = delete;
    AutoML& operator=(const AutoML&) = delete;

    // ---- Classification ----

    AutoMLModel trainClassifier(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        std::function<void(int, int, double)> progress = nullptr);

    // ---- Regression ----

    AutoMLModel trainRegressor(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        std::function<void(int, int, double)> progress = nullptr);

    // ---- Cross-validation helper ----

    EvalMetrics crossValidate(
        const std::vector<DataPoint>& data,
        const AutoMLConfig& config,
        ModelAlgorithm algorithm,
        const std::map<std::string, double>& hyperparameters) const;

    // ---- Helper functions (Phase 2B) ----

    std::pair<bool, std::string> validateTrainingData(
        const std::vector<std::vector<double>>& features,
        const std::vector<double>& target,
        AutoMLTask task = AutoMLTask::CLASSIFICATION) const noexcept;

    ModelAlgorithm selectMetalearner(
        const std::vector<std::vector<double>>& features,
        const std::vector<double>& target,
        const std::vector<ModelAlgorithm>& candidates,
        AutoMLTask task = AutoMLTask::CLASSIFICATION) const;

    /**
     * @brief Select Ensemble Method.
     * @param[in] candidate_metrics Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    ModelAlgorithm selectEnsembleMethod(
        const std::vector<EvalMetrics>& candidate_metrics) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Free helpers
// ============================================================================

inline const char* automlTaskName(AutoMLTask t) noexcept {
    switch (t) {
        case AutoMLTask::CLASSIFICATION: return "CLASSIFICATION";
        case AutoMLTask::REGRESSION:     return "REGRESSION";
        default:                         return "UNKNOWN";
    }
}

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

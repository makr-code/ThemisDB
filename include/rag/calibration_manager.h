/**
 * @file calibration_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace themis::rag::judge {

/**
 * @brief Calibration method
 */
enum class CalibrationMethod {
    TEMPERATURE_SCALING,  ///< Simple temperature scaling
    PLATT_SCALING,        ///< Platt scaling (logistic regression)
    ISOTONIC_REGRESSION,  ///< Isotonic regression (non-parametric)
    NONE                  ///< No calibration
};

/**
 * @brief Ground truth annotation for calibration
 */
struct GroundTruthAnnotation {
    std::string test_id;
    std::string query;
    std::string answer;
    std::vector<RetrievedDocument> documents;
    
    // Human annotations
    double faithfulness_score;    ///< Expert annotation (0-1)
    double relevance_score;
    double completeness_score;
    double coherence_score;
    double overall_score;
    
    std::vector<std::string> annotators; ///< List of annotators
    double inter_annotator_agreement;    ///< Agreement score
};

/**
 * @brief Calibration metrics
 */
struct CalibrationMetrics {
    double expected_calibration_error = 0;  ///< ECE
    double brier_score;                 ///< Brier score
    double correlation;                 ///< Correlation with ground truth
    double mae;                         ///< Mean absolute error
    double rmse;                        ///< Root mean squared error
    
    std::unordered_map<std::string, double> per_dimension_ece;
};

/**
 * @brief Configuration for calibration
 */
struct CalibrationConfig {
    CalibrationMethod method = CalibrationMethod::TEMPERATURE_SCALING;
    int num_bins = 10;                  ///< Number of bins for ECE calculation
    double confidence_threshold = 0.5;   ///< Minimum confidence for predictions
    bool apply_to_all_dimensions = true; ///< Apply calibration to all dimensions
};

/**
 * @brief Calibration manager
 * 
 * Calibrates judge scores to align with human judgments using various
 * calibration techniques. Tracks calibration metrics and provides
 * calibration-adjusted scores.
 */
class CalibrationManager {
public:
    /**
     * @brief Construct calibration manager with default configuration.
     */
    CalibrationManager();
    /**
     * @brief Construct calibration manager.
     * @param config Calibration configuration.
     */
    explicit CalibrationManager(const CalibrationConfig& config);
    
    /**
     * @brief Load ground truth annotations from file
     * @param filepath Path to annotations file (JSON/YAML)
     * @return Number of annotations loaded
     */
    size_t loadGroundTruth(const std::string& filepath);
    
    /**
     * @brief Add ground truth annotation
     * @param annotation Ground truth annotation
     */
    void addGroundTruth(const GroundTruthAnnotation& annotation);
    
    /**
     * @brief Train calibration model
     * 
     * Trains calibration model on loaded ground truth data.
     * 
     * @param judge Judge instance to calibrate
     * @return Calibration metrics before and after
     */
    std::pair<CalibrationMetrics, CalibrationMetrics> train(RAGJudge& judge);
    
    /**
     * @brief Apply calibration to evaluation result
     * @param result Uncalibrated evaluation result
     * @return Calibrated evaluation result
     */
    EvaluationResult calibrate(const EvaluationResult& result);
    
    /**
     * @brief Calculate calibration metrics
     * @param predictions Judge predictions
     * @param ground_truth Ground truth annotations
     * @return Calibration metrics
     */
    CalibrationMetrics calculateMetrics(
        const std::vector<EvaluationResult>& predictions,
        const std::vector<GroundTruthAnnotation>& ground_truth
    );
    
    /**
     * @brief Calculate expected calibration error (ECE)
     * @param predictions Predicted scores
     * @param ground_truth True scores
     * @param confidences Confidence scores
     * @return ECE value
     */
    double calculateECE(
        const std::vector<double>& predictions,
        const std::vector<double>& ground_truth,
        const std::vector<double>& confidences
    );
    
    /**
     * @brief Calculate Brier score
     * @param predictions Predicted probabilities
     * @param ground_truth True binary outcomes
     * @return Brier score
     */
    double calculateBrierScore(
        const std::vector<double>& predictions,
        const std::vector<double>& ground_truth
    );
    
    /**
     * @brief Calculate inter-annotator agreement
     * 
     * Computes Cohen's Kappa, Fleiss' Kappa, or Krippendorff's Alpha
     * depending on number of annotators.
     * 
     * @param annotations Multi-annotator annotations
     * @return Agreement score (0-1)
     */
    double calculateInterAnnotatorAgreement(
        const std::vector<std::vector<double>>& annotations
    );
    
    /**
     * @brief Save calibration model to file
     * @param filepath Path to save model
     * @return true if successful
     */
    bool saveModel(const std::string& filepath);
    
    /**
     * @brief Load calibration model from file
     * @param filepath Path to model file
     * @return true if successful
     */
    bool loadModel(const std::string& filepath);
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const CalibrationConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    CalibrationConfig getConfig() const;

private:
    CalibrationConfig config_;
    std::vector<GroundTruthAnnotation> ground_truth_;
    
    // Calibration parameters (learned from training)
    double temperature_ = 1.0;  ///< Temperature scaling parameter
    std::unordered_map<std::string, double> dimension_temperatures_;
    
    // For Platt scaling
    struct PlattParameters {
        double A = 0.0;
        double B = 0.0;
    };
    std::unordered_map<std::string, PlattParameters> platt_params_;
    
    // Helper methods
    double applyTemperatureScaling(double score, double temperature);
    double applyPlattScaling(double score, const PlattParameters& params);
    std::vector<std::pair<double, double>> buildIsotonicModel(
        const std::vector<double>& predictions,
        const std::vector<double>& ground_truth
    );
};

} // namespace themis::rag::judge

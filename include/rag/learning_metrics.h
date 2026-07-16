/**
 * @file learning_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "knowledge_gap_detector.h"

namespace themis::rag::learning {

/**
 * @brief Type of user feedback
 */
enum class FeedbackType {
    POSITIVE,   ///< User indicated answer was helpful
    NEGATIVE,   ///< User indicated answer was not helpful
    CORRECTION, ///< User provided a correction
    NEUTRAL     ///< No explicit feedback
};

/**
 * @brief Retrieved document information for learning
 */
struct RetrievedDocument {
    std::string id;
    std::string content;
    double similarity_score;
    std::unordered_map<std::string, std::string> metadata;
};

/**
 * @brief Complete interaction record for learning
 */
struct Interaction {
    std::string interaction_id;
    std::chrono::system_clock::time_point timestamp;

    // Input
    std::string query;
    std::vector<RetrievedDocument> retrieved_docs;
    std::string prompt_template_used;

    // Output
    std::string generated_answer;
    std::vector<double> token_probabilities;

    // Metrics
    knowledge_gap::DetectionResult gap_detection_result;
    double perplexity       = 0.0;
    double confidence_score = 0.0;

    // User feedback (optional)
    std::optional<FeedbackType> user_feedback;
    std::optional<std::string> user_correction;

    // Metadata
    std::string model_version;
    std::string retrieval_config_version;
    std::string prompt_version;
    bool is_ab_test_traffic = false;
};

/**
 * @brief Snapshot of performance at a point in time
 */
struct PerformanceSnapshot {
    std::chrono::system_clock::time_point timestamp;
    double accuracy           = 0.0;
    double avg_confidence     = 0.0;
    double gap_detection_rate = 0.0;
    size_t total_queries      = 0;
    std::string component_version;
};

/**
 * @brief Information about an improvement event
 */
struct ImprovementEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string component;        ///< Which component improved (LoRA, Prompt, Retrieval)
    std::string improvement_type; ///< Type of improvement
    double metric_before = 0.0;
    double metric_after  = 0.0;
    std::string description;
};

/**
 * @brief A/B test information
 */
struct ABTestInfo {
    std::string test_id;
    std::string component;
    std::chrono::system_clock::time_point start_time;
    size_t control_samples        = 0;
    size_t treatment_samples      = 0;
    double control_success_rate   = 0.0;
    double treatment_success_rate = 0.0;
    bool is_active                = true;
};

/**
 * @brief Overall learning statistics
 */
struct LearningStats {
    size_t total_interactions_logged = 0;
    size_t lora_retraining_count     = 0;
    size_t prompt_optimizations      = 0;
    size_t retrieval_optimizations   = 0;

    double current_accuracy = 0.0;
    double accuracy_7d_avg  = 0.0;
    double accuracy_trend   = 0.0; ///< Positive = improving, Negative = degrading

    std::vector<ImprovementEvent> recent_improvements;
    std::vector<ABTestInfo> active_ab_tests;
};

/**
 * @brief Result of A/B test statistical analysis
 */
struct ABTestResult {
    std::string test_id;
    double control_success_rate   = 0.0;
    double treatment_success_rate = 0.0;
    double improvement            = 0.0; ///< Treatment - Control
    double p_value                = 1.0;
    bool is_significant           = false; ///< p < 0.05
    size_t sample_size_control    = 0;
    size_t sample_size_treatment  = 0;
};

/**
 * @brief Retrieval parameters for optimization
 */
struct RetrievalParams {
    size_t top_k                = 10;
    double similarity_threshold = 0.75;
    double coverage_threshold   = 0.8;
};

// Forward declaration of EvaluationResult used by LearningMetrics
struct MetricsSnapshot {
    double mean_accuracy     = 0.0;
    double mean_faithfulness = 0.0;
    double mean_relevance    = 0.0;
    double mean_completeness = 0.0;
    double mean_coherence    = 0.0;

    double std_accuracy     = 0.0;
    double std_faithfulness = 0.0;

    double min_accuracy = 0.0;
    double max_accuracy = 0.0;

    double trend_accuracy     = 0.0; ///< Linear regression slope over window
    double trend_faithfulness = 0.0;

    size_t num_evaluations = 0;
};

/**
 * @brief Recorded entry for a single RAG evaluation
 */
struct EvaluationEntry {
    double overall_score      = 0.0;
    double faithfulness_score = 0.0;
    double relevance_score    = 0.0;
    double completeness_score = 0.0;
    double coherence_score    = 0.0;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Tracks RAG evaluation metrics over a sliding window.
 *
 * Thread-safe metrics accumulator that records evaluation results and
 * computes aggregated statistics (mean, std-dev, trend) for monitoring
 * and continuous-learning workflows.
 */
class LearningMetrics {
public:
    /**
     * @brief Configuration for LearningMetrics
     */
    struct Config {
        size_t window_size = 100; ///< Maximum number of evaluations to retain
    };

    LearningMetrics();
    explicit LearningMetrics(const Config& config);
    ~LearningMetrics();

    /**
     * @brief Record a new evaluation entry
     * @param entry Evaluation scores to record
     */
    void recordEvaluation(const EvaluationEntry& entry);

    /**
     * @brief Compute aggregated metrics over the current window
     * @return Snapshot of statistics
     */
    MetricsSnapshot computeMetrics() const;

    /**
     * @brief Export recorded entries to a CSV file
     * @param filepath Destination file path
     */
    void exportMetrics(const std::string& filepath) const;

    /**
     * @brief Print a human-readable report to an output stream
     * @param os Output stream (default: std::cout)
     */
    void printReport(std::ostream& os) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    double computeMean(const std::deque<double>& data) const;
    double computeStdDev(const std::deque<double>& data, double mean) const;
    double computeTrend(const std::deque<double>& data) const;
};

} // namespace themis::rag::learning

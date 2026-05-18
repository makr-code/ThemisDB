/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fairness_detector.h                                ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-18 18:04:35                                ║
  Author:          Copilot AI                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔄 In Development (Wave A3: Fairness & Bias Detection)       ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file fairness_detector.h
 * @brief Bias and fairness detection for RAG documents
 *
 * Implements bias scoring for retrieved documents using word-embedding
 * projections (PCA-based gender/occupational bias), stereotype density
 * analysis, and optional intersectional bias scoring.
 *
 * Purpose: Quantify corpus biases in RAG to enable ethical audits,
 * filtering, and fairness-aware ranking adjustments.
 *
 * @reference Bolukbasi et al. (2016) "Man is to Computer Programmer
 *            as Woman is to Homemaker: Debiasing Word Embeddings"
 *            NeurIPS 2016, arXiv:1607.06520
 */

#pragma once

#include "rag/rag_judge.h"
#include "common/result.h"
#include "common/logger.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace themis::rag {

/**
 * @brief Configuration for FairnessDetector.
 *
 * Controls bias detection thresholds, which bias types to check, and
 * optional filtering behavior.
 */
struct FairnessDetectorConfig {
    /// Overall bias score threshold for flagging (0.0–1.0); default 0.6
    double bias_threshold = 0.6;

    /// Enable gender bias detection (PCA-based)
    bool detect_gender_bias = true;

    /// Enable occupational stereotype detection
    bool detect_occupational_bias = true;

    /// Enable ethnicity/cultural bias detection
    bool detect_ethnicity_bias = true;

    /// Enable intersectional (compound) bias detection
    bool detect_intersectional_bias = true;

    /// Path to word embeddings (GloVe/FastText) for PCA projection
    std::string embedding_model_path;

    /// Target embedding dimension (typically 300)
    size_t embedding_dimension = 300;

    /// Maximum document length for bias analysis (truncate if larger)
    size_t max_doc_length = 10000;

    /// Minimum confidence threshold for flagging (0.0–1.0)
    double min_confidence = 0.5;
};

/**
 * @brief Fairness and bias detector for RAG documents.
 *
 * Analyzes retrieved documents for various forms of bias (gender, occupational,
 * ethnic, intersectional) using word-embedding projection and stereotype density.
 * Results enable ethical RAG audits and fairness-aware ranking adjustments.
 *
 * Thread Safety: A single FairnessDetector instance is NOT thread-safe.
 * Create one instance per thread or protect concurrent calls with a mutex.
 *
 * Performance Target (Wave A3):
 *  - ≤ 5 ms per document analysis
 *  - Bias score correlation ≥ 0.70 with human raters (500-doc sample)
 *  - False-positive rate ≤ 10%
 */
class FairnessDetector {
public:
    /**
     * @brief Construct a fairness detector with configuration.
     *
     * @param config Configuration for bias detection thresholds and models.
     */
    explicit FairnessDetector(const FairnessDetectorConfig& config);

    /**
     * @brief Destructor.
     */
    ~FairnessDetector();

    /**
     * @brief Initialize the detector (load embeddings and models).
     *
     * Must be called before @ref detectBias().
     *
     * @return Status::OK on success; error if models cannot be loaded.
     */
    common::Status initialize();

    /**
     * @brief Check if detector is initialized and ready.
     *
     * @return true if embeddings and bias models are loaded.
     */
    bool isInitialized() const;

    /**
     * @brief Analyze a document for bias and return a bias score.
     *
     * Evaluates gender, occupational, ethnic, and intersectional biases
     * based on word embeddings and detected biased terms.
     *
     * @param document_text The document to analyze.
     * @return BiasScore with multiple bias dimensions and flagging decision.
     */
    common::Result<judge::BiasScore> detectBias(const std::string& document_text);

    /**
     * @brief Analyze multiple documents in batch.
     *
     * Default implementation calls @ref detectBias() sequentially.
     * Can be optimized with parallelization in subclasses.
     *
     * @param documents Vector of document texts.
     * @return Vector of BiasScores (same length as input).
     */
    common::Result<std::vector<judge::BiasScore>> detectBiasBatch(
        const std::vector<std::string>& documents);

    /**
     * @brief Filter documents below bias threshold.
     *
     * Returns documents with bias score < config.bias_threshold.
     *
     * @param documents Vector of document texts.
     * @return Filtered documents with acceptable bias levels.
     */
    common::Result<std::vector<std::pair<std::string, judge::BiasScore>>>
    filterByBiasThreshold(const std::vector<std::string>& documents);

    /**
     * @brief Get the configuration used by this detector.
     *
     * @return Configuration struct.
     */
    const FairnessDetectorConfig& getConfig() const;

    /**
     * @brief Set bias detection threshold dynamically.
     *
     * Allows runtime adjustment of the bias score threshold for filtering.
     *
     * @param threshold New threshold (0.0–1.0).
     */
    void setBiasThreshold(double threshold);

private:
    FairnessDetectorConfig config_;
    bool initialized_ = false;

    // Opaque implementation details (PIMPL)
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Private helpers
    common::Result<double> computeGenderBias(const std::string& text);
    common::Result<double> computeOccupationalBias(const std::string& text);
    common::Result<double> computeEthnicityBias(const std::string& text);
    common::Result<double> computeIntersectionalBias(
        const std::string& text,
        double gender_bias,
        double occupational_bias,
        double ethnicity_bias);
    common::Result<double> computeStereotypeDensity(const std::string& text);
    common::Result<std::vector<std::string>> extractBiasedTerms(
        const std::string& text);
};

}  // namespace themis::rag

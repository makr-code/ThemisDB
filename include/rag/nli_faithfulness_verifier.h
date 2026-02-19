/**
 * @file nli_faithfulness_verifier.h
 * @brief NLI (Natural Language Inference) model for faithfulness verification
 * 
 * Implements claim verification using NLI models like RoBERTa-large-MNLI
 * to check entailment between claims and documents.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::rag::judge {

/**
 * @brief NLI prediction result
 */
enum class NLIPrediction {
    ENTAILMENT,      ///< Premise entails hypothesis
    NEUTRAL,         ///< No clear relationship
    CONTRADICTION    ///< Premise contradicts hypothesis
};

/**
 * @brief NLI verification result with confidence
 */
struct NLIResult {
    NLIPrediction prediction;
    double entailment_score;      ///< Confidence in entailment (0-1)
    double neutral_score;         ///< Confidence in neutral (0-1)
    double contradiction_score;   ///< Confidence in contradiction (0-1)
    double confidence;            ///< Overall confidence (0-1)
};

/**
 * @brief NLI-based faithfulness verifier
 * 
 * Uses Natural Language Inference models to verify claims against documents.
 * Provides more accurate entailment checking than simple text matching.
 * 
 * Model Options:
 * - RoBERTa-large-MNLI (default, high accuracy)
 * - DeBERTa-v3-large-MNLI (better performance)
 * - Custom fine-tuned models
 * 
 * Performance Target: <50ms per claim verification
 */
class NLIFaithfulnessVerifier {
public:
    /**
     * @brief Configuration for NLI verifier
     */
    struct Config {
        std::string model_path;               ///< Path to NLI model
        std::string model_type = "roberta";   ///< Model type (roberta, deberta, bart)
        
        // Inference settings
        size_t max_sequence_length = 512;     ///< Max tokens per input
        size_t batch_size = 8;                ///< Batch size for inference
        
        // Thresholds
        double entailment_threshold = 0.7;    ///< Min score for entailment
        double contradiction_threshold = 0.7; ///< Min score for contradiction
        
        // Performance
        bool use_gpu = true;                  ///< Use GPU acceleration
        bool use_quantization = false;        ///< Use INT8 quantization
        size_t max_cache_size = 1000;         ///< Cache size for results
        
        // Fallback
        bool use_heuristic_fallback = true;   ///< Fall back to heuristics if model unavailable
    };

    /**
     * @brief Construct verifier with configuration
     */
    NLIFaithfulnessVerifier();
    explicit NLIFaithfulnessVerifier(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~NLIFaithfulnessVerifier();
    
    /**
     * @brief Verify if claim is entailed by premise
     * 
     * @param premise Document or context text
     * @param hypothesis Claim to verify
     * @return NLI result with prediction and confidence
     */
    NLIResult verify(const std::string& premise, const std::string& hypothesis);
    
    /**
     * @brief Batch verify multiple claim-document pairs
     * 
     * @param pairs Vector of (premise, hypothesis) pairs
     * @return Vector of NLI results
     */
    std::vector<NLIResult> verifyBatch(
        const std::vector<std::pair<std::string, std::string>>& pairs
    );
    
    /**
     * @brief Check if verifier is ready (model loaded)
     * @return true if model is loaded and ready
     */
    bool isReady() const;
    
    /**
     * @brief Get model information
     * @return Model type and path
     */
    std::string getModelInfo() const;
    
    /**
     * @brief Warm up model with sample inputs
     */
    void warmup();
    
    /**
     * @brief Clear result cache
     */
    void clearCache();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convert NLI prediction to support level
 * 
 * Helper function to map NLI predictions to faithfulness support levels.
 */
enum class SupportLevel {
    FULLY_SUPPORTED,
    PARTIALLY_SUPPORTED,
    UNSUPPORTED,
    CONTRADICTED
};

SupportLevel nliPredictionToSupportLevel(
    NLIPrediction prediction,
    double confidence
);

} // namespace themis::rag::judge

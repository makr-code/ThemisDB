/**
 * @file nli_faithfulness_verifier.h
 * @brief NLI-based Faithfulness Verification using ONNX models
 * 
 * Implements Natural Language Inference (NLI) for claim verification
 * using DeBERTa-v3-large-mnli or similar ONNX models. Provides fast,
 * accurate claim-document entailment checking.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief NLI prediction result
 */
enum class NLILabel {
    ENTAILMENT,      ///< Hypothesis entailed by premise (score: 1.0)
    NEUTRAL,         ///< Neither entailment nor contradiction (score: 0.5)
    CONTRADICTION    ///< Hypothesis contradicts premise (score: 0.0)
};

/**
 * @brief NLI verification result
 */
struct NLIResult {
    NLILabel label;                  ///< Predicted label
    double entailment_prob;          ///< P(entailment)
    double neutral_prob;             ///< P(neutral)
    double contradiction_prob;       ///< P(contradiction)
    double confidence;               ///< Max probability
    std::string explanation;         ///< Optional explanation
    bool success;                    ///< Whether verification succeeded
    std::chrono::milliseconds latency;  ///< Inference time
};

/**
 * @brief NLI Faithfulness Verifier
 * 
 * Uses ONNX Runtime to run NLI models for fast claim verification.
 * Target: <50ms per claim verification.
 * 
 * Supported models:
 * - microsoft/deberta-v3-large-mnli
 * - roberta-large-mnli
 * - bart-large-mnli
 */
class NLIFaithfulnessVerifier {
public:
    /**
     * @brief Configuration for NLI verifier
     */
    struct Config {
        std::string model_path;          ///< Path to ONNX model file
        std::string tokenizer_path;      ///< Path to tokenizer config
        int max_sequence_length = 512;   ///< Max tokens for premise+hypothesis
        double entailment_threshold = 0.7;  ///< Min prob for ENTAILMENT
        bool enable_caching = true;      ///< Cache claim-document pairs
        int num_threads = 4;             ///< ONNX Runtime threads
        bool use_gpu = false;            ///< Enable GPU acceleration if available
    };
    
    /**
     * @brief Construct verifier with configuration
     * @param config Verifier configuration
     */
    NLIFaithfulnessVerifier();
    explicit NLIFaithfulnessVerifier(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~NLIFaithfulnessVerifier();
    
    /**
     * @brief Verify if claim is entailed by document
     * @param claim Hypothesis to verify
     * @param document Premise document
     * @return NLI verification result
     */
    NLIResult verifyClaim(
        const std::string& claim,
        const std::string& document
    );
    
    /**
     * @brief Batch verify multiple claims against a document
     * @param claims List of claims to verify
     * @param document Premise document
     * @return Vector of NLI results (same order as claims)
     */
    std::vector<NLIResult> verifyClaimsBatch(
        const std::vector<std::string>& claims,
        const std::string& document
    );
    
    /**
     * @brief Verify claim against multiple documents, return best match
     * @param claim Hypothesis to verify
     * @param documents List of premise documents
     * @return NLI result with highest entailment probability
     */
    NLIResult verifyAgainstMultipleDocs(
        const std::string& claim,
        const std::vector<std::string>& documents
    );
    
    /**
     * @brief Check if model is loaded and ready
     * @return true if model is ready
     */
    bool isReady() const;
    
    /**
     * @brief Load ONNX model from path
     * @param model_path Path to ONNX model file
     * @param tokenizer_path Path to tokenizer config
     * @return true if loading succeeded
     */
    bool loadModel(const std::string& model_path, const std::string& tokenizer_path);
    
    /**
     * @brief Get current configuration
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Clear verification cache
     */
    void clearCache();
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        double hit_rate = 0.0;
    };
    CacheStats getCacheStats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal helpers
    std::vector<int> tokenize(const std::string& premise, const std::string& hypothesis);
    std::vector<float> runInference(const std::vector<int>& input_ids);
    NLIResult interpretLogits(const std::vector<float>& logits);
    std::string generateCacheKey(const std::string& claim, const std::string& document);
};

/**
 * @brief Utilities for NLI-based faithfulness scoring
 */
namespace nli_utils {

/**
 * @brief Convert NLI label to numeric score
 * @param label NLI prediction label
 * @return Score: ENTAILMENT=1.0, NEUTRAL=0.5, CONTRADICTION=0.0
 */
double labelToScore(NLILabel label);

/**
 * @brief Compute overall faithfulness from multiple NLI results
 * @param results Vector of NLI results for different claims
 * @return Aggregated faithfulness score (0-1)
 */
double aggregateFaithfulness(const std::vector<NLIResult>& results);

/**
 * @brief Check if claim is factual (vs opinion/reasoning)
 * @param claim Claim text
 * @return true if claim appears to be factual
 */
bool isFactualClaim(const std::string& claim);

} // namespace nli_utils

} // namespace themis::rag::judge

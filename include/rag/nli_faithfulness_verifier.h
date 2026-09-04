/**
 * @file nli_faithfulness_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief NLI label for entailment checking
 */
enum class NLILabel {
    ENTAILMENT,      ///< Hypothesis is entailed by premise
    NEUTRAL,         ///< Hypothesis is neither entailed nor contradicted
    CONTRADICTION    ///< Hypothesis contradicts premise
};

// Shared support level enum is defined in faithfulness_evaluator.h
enum class SupportLevel;

/**
 * @brief Result from NLI model
 */
struct NLIResult {
    NLILabel label;
    double entailment_score;      ///< P(entailment)
    double neutral_score;          ///< P(neutral)
    double contradiction_score;    ///< P(contradiction)
    double confidence;             ///< Confidence in prediction
};

/**
 * @brief Verification result for a single claim
 */
struct ClaimVerification {
    std::string claim;
    SupportLevel support_level;
    std::vector<std::string> supporting_doc_ids;
    std::unordered_map<std::string, NLIResult> nli_scores;  ///< Per-document NLI scores
    double confidence;
};

/**
 * @brief Complete faithfulness verification result
 */
struct FaithfulnessVerificationResult {
    double faithfulness_score = 0;     ///< Overall score (0-1)
    bool is_faithful;              ///< Meets threshold
    
    // Claim statistics
    size_t total_claims;
    size_t supported_claims;
    size_t partially_supported_claims;
    size_t unsupported_claims;
    size_t contradicted_claims;
    
    std::vector<ClaimVerification> claims;
    std::string explanation;
    std::chrono::milliseconds verification_time;
    
    FaithfulnessVerificationResult() 
        : faithfulness_score(0.0), is_faithful(false),
          total_claims(0), supported_claims(0), 
          partially_supported_claims(0), unsupported_claims(0),
          contradicted_claims(0), verification_time(0) {}
};

/**
 * @brief NLI-based faithfulness verifier
 * 
 * Uses Natural Language Inference models to verify if claims in generated
 * answers are supported by retrieved documents. This provides a more
 * principled approach than simple text matching.
 * 
 * Typical NLI models:
 * - RoBERTa-large-MNLI (best accuracy)
 * - DeBERTa-large-MNLI (state-of-the-art)
 * - BART-large-MNLI (good balance)
 * 
 * Performance targets:
 * - Fast mode: ~50ms per answer (uses heuristics)
 * - Balanced mode: ~200ms per answer (lightweight NLI)
 * - Thorough mode: ~500ms per answer (full transformer NLI)
 */
class NLIFaithfulnessVerifier {
public:
    /**
     * @brief Configuration for NLI verification
     */
    struct Config {
        // NLI model settings
        std::string model_path = "roberta-large-mnli";
        bool use_gpu = false;
        
        // Thresholds for classification
        double entailment_threshold = 0.7;    ///< Min score for entailment
        double neutral_threshold = 0.4;       ///< Min score for neutral
        double contradiction_threshold = 0.7; ///< Min score for contradiction
        
        // Claim extraction
        size_t max_claims = 20;               ///< Max claims to extract
        bool enable_claim_decomposition = true; ///< Break complex claims
        
        // Faithfulness scoring
        double min_faithfulness_score = 0.8;  ///< Threshold for "faithful"
        
        // Performance
        bool enable_batching = true;          ///< Batch NLI inference
        size_t batch_size = 8;
        
        // ONNX Runtime options (Phase 1 v1.5)
        std::string onnx_model_path = "roberta-large-mnli";           ///< ONNX model path
        std::string onnx_tokenizer_path = "tokenizer.json";           ///< Tokenizer config path
        bool use_onnx = true;                                         ///< Enable ONNX inference
        bool fallback_to_heuristic = true;                            ///< Fallback if ONNX fails
        int onnx_inference_timeout_ms = 500;                          ///< ONNX inference timeout
        bool log_inference_mode = false;                              ///< Log ONNX vs heuristic
    };
    
    /**
     * @brief Construct verifier with default config
     */
    NLIFaithfulnessVerifier();
    
    /**
     * @brief Construct verifier with custom config
     * @param config Verifier configuration
     */
    explicit NLIFaithfulnessVerifier(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~NLIFaithfulnessVerifier();
    
    /**
     * @brief Verify faithfulness of an answer using NLI
     * @param answer Generated answer to verify
     * @param documents Retrieved documents (id, content pairs)
     * @return Verification result with claim-level analysis
     */
    FaithfulnessVerificationResult verify(
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents
    );
    
    /**
     * @brief Check entailment between premise and hypothesis
     * @param premise The document/context (entailer)
     * @param hypothesis The claim to verify (entailed)
     * @return NLI result with probabilities
     */
    NLIResult checkEntailment(
        const std::string& premise,
        const std::string& hypothesis
    );
    
    /**
     * @brief Load NLI model from disk
     * @param model_path Path to NLI model files
     */
    void loadModel(const std::string& model_path);
    
    /**
     * @brief Check if NLI model is loaded and ready
     * @return true if model is loaded
     */
    bool isModelLoaded() const;
    
    /**
     * @brief Check if verifier is ready for inference.
     *
     * Returns `true` when at least one inference path is available:
     * - ONNX model is loaded (`isModelLoaded()` is `true`), **or**
     * - ONNX is disabled (`Config::use_onnx == false`), so heuristic runs
     *   unconditionally, **or**
     * - ONNX is enabled but the heuristic fallback is also enabled
     *   (`Config::fallback_to_heuristic == true`).
     *
     * Returns `false` only when ONNX is required (`use_onnx == true`) **and**
     * no fallback is permitted (`fallback_to_heuristic == false`) **and** no
     * model has been loaded yet.
     *
     * @return true if the verifier can perform inference
     */
    bool isReady() const;
    
    /**
     * @brief Get current configuration
     * @return Current config
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge

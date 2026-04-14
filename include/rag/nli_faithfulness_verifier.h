/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            nli_faithfulness_verifier.h                        ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:27:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file nli_faithfulness_verifier.h
 * @brief NLI-based faithfulness verification using Natural Language Inference
 * 
 * Implements faithfulness verification using NLI models (e.g., RoBERTa-large-MNLI)
 * to check if generated claims are entailed by retrieved documents.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
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
    double faithfulness_score;     ///< Overall score (0-1)
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

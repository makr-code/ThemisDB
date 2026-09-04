/**
 * @file faithfulness_evaluator.h
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
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Support level for a claim
 */
enum class SupportLevel {
    FULLY_SUPPORTED,      ///< Score: 1.0
    PARTIALLY_SUPPORTED,  ///< Score: 0.5
    UNSUPPORTED,          ///< Score: 0.0
    CONTRADICTED          ///< Score: -0.5
};

/**
 * @brief A claim extracted from an answer
 */
struct Claim {
    std::string text;
    std::string category;  ///< factual, opinion, reasoning
    SupportLevel support_level;
    std::vector<std::string> supporting_doc_ids;
    double confidence;
};

/**
 * @brief Citation information
 */
struct Citation {
    std::string claim_text;
    std::vector<std::string> cited_doc_ids;
    bool has_explicit_reference;
    double quality_score;  ///< 0-1
};

/**
 * @brief Faithfulness evaluation result
 */
struct FaithfulnessResult {
    double faithfulness_score = 0;  ///< Overall score 0-1
    std::vector<Claim> claims;
    std::vector<Citation> citations;
    size_t supported_claims_count;
    size_t total_claims_count;
    double citation_quality_score;
    std::string explanation;
};

/**
 * @brief Faithfulness evaluator
 * 
 * Evaluates answer faithfulness through:
 * 1. Claim extraction from answer
 * 2. Document entailment checking per claim
 * 3. Citation verification
 */
class FaithfulnessEvaluator {
public:
    /**
     * @brief Configuration for faithfulness evaluation
     */
    struct Config {
        size_t max_claims_to_extract = 10;
        double entailment_threshold = 0.7;
        bool enable_citation_check = true;
        bool enable_claim_deduplication = true;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    FaithfulnessEvaluator();
    explicit FaithfulnessEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~FaithfulnessEvaluator();
    
    /**
     * @brief Evaluate faithfulness of an answer
     * @param answer Generated answer
     * @param documents Retrieved documents
     * @param query Original query (optional, for context)
     * @return Faithfulness evaluation result
     */
    FaithfulnessResult evaluate(
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,  // id, content
        const std::string& query = ""
    );
    
    /**
     * @brief Extract atomic claims from answer
     * @param answer Generated answer
     * @return List of extracted claims
     */
    std::vector<Claim> extractClaims(const std::string& answer);
    
    /**
     * @brief Check if claim is entailed by documents
     * @param claim Claim to verify
     * @param documents Documents to check against
     * @return Support level for the claim
     */
    SupportLevel checkEntailment(
        const std::string& claim,
        const std::vector<std::pair<std::string, std::string>>& documents
    );
    
    /**
     * @brief Find and verify citations in answer
     * @param answer Generated answer
     * @param documents Available documents
     * @param claims Extracted claims
     * @return List of citations with quality scores
     */
    std::vector<Citation> verifyCitations(
        const std::string& answer,
        [[maybe_unused]] const std::vector<std::pair<std::string, std::string>>& documents,
        const std::vector<Claim>& claims
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge

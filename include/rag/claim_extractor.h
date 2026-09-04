/**
 * @file claim_extractor.h
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

namespace themis::rag {

/**
 * @brief Represents an extracted claim from generated text
 */
struct Claim {
    std::string text = {};
    size_t position;  // Position in original text
    double confidence;  // Confidence in extraction
    std::string category;  // e.g., "factual", "opinion", "prediction"
};

/**
 * @brief Result of claim verification against documents
 */
struct ClaimVerificationResult {
    Claim claim;
    enum class Verdict {
        SUPPORTED,      // Claim is supported by documents
        CONTRADICTED,   // Claim is contradicted by documents
        NOT_FOUND,      // No evidence found
        INSUFFICIENT    // Insufficient evidence
    };
    Verdict verdict;
    double confidence;
    std::string evidence;  // Quote from document supporting/contradicting
    std::string document_id;  // Which document provided evidence
};

/**
 * @brief Extracts atomic claims from generated text
 */
class ClaimExtractor {
public:
    /**
     * @brief Extract atomic claims from text using LLM
     * @param text The generated text to extract claims from
     * @return Vector of extracted claims
     */
    static std::vector<Claim> extract(const std::string& text);
    
    /**
     * @brief Verify a claim against source documents
     * @param claim The claim to verify
     * @param documents Source documents as strings
     * @return Verification result
     */
    static ClaimVerificationResult verify(
        const Claim& claim,
        const std::vector<std::string>& documents
    );
    
    /**
     * @brief Verify all claims in text against documents
     * @param text Generated text containing claims
     * @param documents Source documents
     * @return Vector of verification results
     */
    static std::vector<ClaimVerificationResult> verifyAll(
        const std::string& text,
        const std::vector<std::string>& documents
    );
    
    /**
     * @brief Calculate faithfulness score from verification results
     * @param results Verification results
     * @return Faithfulness score (0-1)
     */
    static double calculateFaithfulness(
        const std::vector<ClaimVerificationResult>& results
    );
};

/**
 * @brief Self-consistency evaluation for multiple generated samples
 */
class SelfConsistencyEvaluator {
public:
    /**
     * @brief Result of self-consistency evaluation
     */
    struct ConsistencyResult {
        double consistency_score = 0;  // 0-1, higher is more consistent
        std::vector<std::string> agreements;  // Points of agreement
        std::vector<std::string> disagreements;  // Points of disagreement
        std::string consensus_answer;  // Most consistent answer
        double confidence;  // Confidence in consensus
    };
    
    /**
     * @brief Evaluate consistency across multiple samples
     * @param samples Generated samples to compare
     * @return Consistency evaluation result
     */
    static ConsistencyResult evaluate(const std::vector<std::string>& samples);
    
    /**
     * @brief Extract consensus answer from samples
     * @param samples Generated samples
     * @return Consensus answer with highest agreement
     */
    static std::string extractConsensus(const std::vector<std::string>& samples);
    
    /**
     * @brief Calculate pairwise similarity matrix
     * @param samples Generated samples
     * @return Matrix of similarity scores
     */
    static std::vector<std::vector<double>> calculateSimilarityMatrix(
        const std::vector<std::string>& samples
    );
};

} // namespace themis::rag

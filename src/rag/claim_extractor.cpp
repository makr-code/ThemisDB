/**
 * @file claim_extractor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/claim_extractor.h"
#include "rag/llm_integration.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <regex>

namespace themis::rag {

// ============================================================================
// ClaimExtractor Implementation
// ============================================================================

std::vector<Claim> ClaimExtractor::extract(const std::string& text) {
    THEMIS_DEBUG("Extracting claims from text (length: {})", text.length());
    
    if (text.empty()) {
        return {};
    }
    
    // Use LLM to extract atomic claims
    auto tmpl = PromptLibrary::getClaimVerificationPrompt();
    
    // Create a claim extraction prompt
    PromptTemplate extraction_tmpl;
    extraction_tmpl.system_prompt = 
        "You are a claim extraction expert. Extract atomic, verifiable claims from text.";
    extraction_tmpl.user_template =
        "Text: {text}\n\n"
        "Extract all atomic claims from this text. Each claim should be:\n"
        "1. A single, verifiable statement\n"
        "2. Self-contained (understandable without context)\n"
        "3. Factual (not opinions unless explicitly stated as such)\n\n"
        "List each claim on a new line, numbered.";
    
    std::unordered_map<std::string, std::string> vars;
    vars["text"] = text;
    
    std::string prompt = extraction_tmpl.format(vars);
    std::string response = LLMIntegration::generate(prompt);
    
    // Validate LLM output before parsing
    if (response.empty()) {
        THEMIS_WARN("ClaimExtractor: Empty LLM response for claim extraction");
        return {};
    }
    
    if (response.length() > 1000000) {  // Sanity check for reasonable response size
        THEMIS_WARN("ClaimExtractor: LLM response exceeds maximum reasonable size ({})", response.length());
        return {};
    }
    
    // Parse claims from response
    std::vector<Claim> claims;
    std::istringstream iss(response);
    std::string line = {};
    size_t position = 0;
    
    while (std::getline(iss, line)) {
        // Skip empty lines
        if (line.empty()) {
          continue;
        }
        
        // Remove numbering (e.g., "1. ", "2. ", etc.)
        std::regex number_regex(R"(^\s*\d+\.\s*)");
        line = std::regex_replace(line, number_regex, "");
        
        if (!line.empty()) {
            Claim claim;
            claim.text = line;
            claim.position = position++;
            claim.confidence = 0.8;  // Default confidence
            claim.category = "factual";
            claims.push_back(claim);
        }
    }
    
    THEMIS_DEBUG("Extracted {} claims", claims.size());
    return claims;
}

ClaimVerificationResult ClaimExtractor::verify(
    const Claim& claim,
    const std::vector<std::string>& documents
) {
    THEMIS_DEBUG("Verifying claim: {}", claim.text);
    
    ClaimVerificationResult result;
    result.claim = claim;
    result.verdict = ClaimVerificationResult::Verdict::NOT_FOUND;
    result.confidence = 0.0;
    
    if (documents.empty()) {
        result.verdict = ClaimVerificationResult::Verdict::INSUFFICIENT;
        return result;
    }
    
    // Concatenate documents
    std::ostringstream docs_stream = {};
    for (size_t i = 0; i < documents.size(); ++i) {
        docs_stream << "Document " << (i + 1) << ":\n" << documents[i] << "\n\n";
    }
    
    // Use LLM to verify claim
    auto tmpl = PromptLibrary::getClaimVerificationPrompt();
    std::unordered_map<std::string, std::string> vars;
    vars["claim"] = claim.text;
    vars["documents"] = docs_stream.str();
    
    std::string prompt = tmpl.format(vars);
    std::string response = LLMIntegration::generate(prompt);
    
    // Validate LLM response before parsing
    if (response.empty()) {
        THEMIS_WARN("ClaimExtractor: Empty LLM response for claim verification");
        result.verdict = ClaimVerificationResult::Verdict::INSUFFICIENT;
        return result;
    }
    
    // Parse verification result
    auto parsed = LLMIntegration::parseEvaluationResponse(response);
    
    // Determine verdict from response
    std::string response_lower = response;
    std::transform(response_lower.begin(), response_lower.end(), 
                   response_lower.begin(), ::tolower);
    
    if (response_lower.find("supported") != std::string::npos) {
        result.verdict = ClaimVerificationResult::Verdict::SUPPORTED;
        result.confidence = parsed.confidence;
    } else if (response_lower.find("contradicted") != std::string::npos ||
               response_lower.find("contradicts") != std::string::npos) {
        result.verdict = ClaimVerificationResult::Verdict::CONTRADICTED;
        result.confidence = parsed.confidence;
    } else if (response_lower.find("insufficient") != std::string::npos) {
        result.verdict = ClaimVerificationResult::Verdict::INSUFFICIENT;
        result.confidence = parsed.confidence;
    } else {
        result.verdict = ClaimVerificationResult::Verdict::NOT_FOUND;
        result.confidence = parsed.confidence;
    }
    
    result.evidence = parsed.explanation;
    
    return result;
}

std::vector<ClaimVerificationResult> ClaimExtractor::verifyAll(
    const std::string& text,
    const std::vector<std::string>& documents
) {
    THEMIS_DEBUG("Verifying all claims in text against documents");
    
    // Extract claims
    auto claims = extract(text);
    
    // Verify each claim
    std::vector<ClaimVerificationResult> results = {};

    results.reserve(claims.size());
    
    for (const auto& claim : claims) {
        results.push_back(verify(claim, documents));
    }
    
    return results;
}

double ClaimExtractor::calculateFaithfulness(
    const std::vector<ClaimVerificationResult>& results
) {
    if (results.empty()) {
        return 1.0;  // No claims to verify
    }
    
    size_t supported = 0;
    size_t total = results.size();
    
    for (const auto& result : results) {
        if (result.verdict == ClaimVerificationResult::Verdict::SUPPORTED) {
            supported++;
        }
    }
    
    double faithfulness = static_cast<double>(supported) / total;
    
    THEMIS_DEBUG("Faithfulness score: {:.3f} ({}/{} claims supported)",
                 faithfulness, supported, total);
    
    return faithfulness;
}

// ============================================================================
// SelfConsistencyEvaluator Implementation
// ============================================================================

SelfConsistencyEvaluator::ConsistencyResult SelfConsistencyEvaluator::evaluate(
    const std::vector<std::string>& samples
) {
    THEMIS_DEBUG("Evaluating self-consistency across {} samples", samples.size());
    
    ConsistencyResult result;
    result.consistency_score = 0.0;
    result.confidence = 0.0;
    
    if (samples.empty()) {
        return result;
    }
    
    if (samples.size() == 1) {
        result.consistency_score = 1.0;
        result.confidence = 1.0;
        result.consensus_answer = samples[0];
        return result;
    }
    
    // Calculate pairwise similarity matrix
    auto similarity_matrix = calculateSimilarityMatrix(samples);
    
    // Calculate average similarity (consistency score)
    double total_similarity = 0.0;
    size_t num_pairs = 0;
    
    for (size_t i = 0; i < similarity_matrix.size(); ++i) {
        for (size_t j = i + 1; j < similarity_matrix[i].size(); ++j) {
            total_similarity += similarity_matrix[i][j];
            num_pairs++;
        }
    }
    
    result.consistency_score = num_pairs > 0 ? total_similarity / num_pairs : 0.0;
    result.confidence = result.consistency_score;
    
    // Extract consensus (most similar to all others)
    result.consensus_answer = extractConsensus(samples);
    
    // Use LLM to identify specific agreements/disagreements
    if (static_cast<int>(samples.size()) <= 5) {  // Only for small sets to avoid token limits
        PromptTemplate consistency_tmpl = PromptLibrary::getConsistencyCheckPrompt();
        
        std::ostringstream samples_str = {};
        for (size_t i = 0; i < samples.size(); ++i) {
            samples_str << "Sample " << (i + 1) << ": " << samples[i] << "\n\n";
        }
        
        std::unordered_map<std::string, std::string> vars;
        vars["query"] = "Multiple samples comparison";
        vars["response1"] = samples[0];
        vars["response2"] = samples.size() > 1 ? samples[1] : samples[0];
        
        std::string prompt = consistency_tmpl.format(vars);
        std::string llm_response = LLMIntegration::generate(prompt);
        
        // Validate LLM response before parsing
        if (llm_response.empty()) {
            THEMIS_WARN("ClaimExtractor: Empty LLM response for consistency check");
            result.consistency_score = 0.0;
        } else {
            // Parse agreements and disagreements from response
            // Simplified parsing - in production would use structured output
            if (llm_response.find("agreement") != std::string::npos ||
                llm_response.find("consistent") != std::string::npos) {
                result.agreements.push_back("General agreement found");
            }
            if (llm_response.find("disagreement") != std::string::npos ||
                llm_response.find("inconsistent") != std::string::npos) {
                result.disagreements.push_back("Some disagreements found");
            }
        }
    }
    
    THEMIS_DEBUG("Consistency score: {:.3f}", result.consistency_score);
    
    return result;
}

std::string SelfConsistencyEvaluator::extractConsensus(
    const std::vector<std::string>& samples
) {
    if (samples.empty()) {
        return "";
    }
    
    if (samples.size() == 1) {
        return samples[0];
    }
    
    // Calculate similarity matrix
    auto similarity_matrix = calculateSimilarityMatrix(samples);
    
    // Find sample with highest average similarity to others
    size_t best_idx = 0;
    double best_avg_similarity = 0.0;
    
    for (size_t i = 0; i < similarity_matrix.size(); ++i) {
        double avg_similarity = 0.0;
        for (size_t j = 0; j < similarity_matrix[i].size(); ++j) {
            if (i != j) {
                avg_similarity += similarity_matrix[i][j];
            }
        }
        avg_similarity /= (similarity_matrix[i].size() - 1);
        
        if (avg_similarity > best_avg_similarity) {
            best_avg_similarity = avg_similarity;
            best_idx = i;
        }
    }
    
    THEMIS_DEBUG("Consensus is sample {} with avg similarity {:.3f}",
                 best_idx, best_avg_similarity);
    
    return samples[best_idx];
}

std::vector<std::vector<double>> SelfConsistencyEvaluator::calculateSimilarityMatrix(
    const std::vector<std::string>& samples
) {
    size_t n = samples.size();
    std::vector<std::vector<double>> matrix(n, std::vector<double>(n, 0.0));
    
    for (size_t i = 0; i < n; ++i) {
        matrix[i][i] = 1.0;  // Self-similarity is 1.0
        
        for (size_t j = i + 1; j < n; ++j) {
            double similarity = LLMIntegration::calculateSemanticSimilarity(
                samples[i], samples[j]
            );
            matrix[i][j] = similarity;
            matrix[j][i] = similarity;  // Symmetric
        }
    }
    
    return matrix;
}

} // namespace themis::rag

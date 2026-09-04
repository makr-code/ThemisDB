/**
 * @file faithfulness_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/faithfulness_evaluator.h"
#include "rag/llm_judge_integration.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <regex>
#include <set>
#include <mutex>

namespace themis::rag::judge {

using json = nlohmann::json;

struct FaithfulnessEvaluator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access
    
    // NLI-based entailment check: uses NLIFaithfulnessVerifier when loaded,
    // falls back to term-overlap heuristic when no model is configured.
    SupportLevel checkNLIEntailment(const std::string& claim, const std::string& document) {
        // Use NLI verifier if available
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            if (nli_verifier) {
                auto nli_result = nli_verifier->checkEntailment(document, claim);
                
                if (nli_result.label == NLILabel::ENTAILMENT) {
                    return SupportLevel::FULLY_SUPPORTED;
                } else if (nli_result.label == NLILabel::NEUTRAL) {
                    return SupportLevel::PARTIALLY_SUPPORTED;
                } else if (nli_result.label == NLILabel::CONTRADICTION) {
                    return SupportLevel::CONTRADICTED;
                } else {
                    return SupportLevel::UNSUPPORTED;
                }
           }
        }
        
        // Fallback: term-overlap heuristic when no NLI verifier is configured
        
        std::string claim_lower = claim;
        std::string doc_lower = document;
        std::transform(claim_lower.begin(), claim_lower.end(), claim_lower.begin(), ::tolower);
        std::transform(doc_lower.begin(), doc_lower.end(), doc_lower.begin(), ::tolower);
        
        // Simple heuristic: check for key terms
        std::istringstream claim_stream(claim_lower);
        std::vector<std::string> claim_words;
        std::string word = {};
        while (claim_stream >> word) {
            if (word.length() > 3) {  // Skip short words
                claim_words.push_back(word);
            }
        }
        
        if (claim_words.empty()) {
            return SupportLevel::UNSUPPORTED;
        }
        
        // Count matching words
        size_t matches = 0;
        for (const auto& w : claim_words) {
            if (doc_lower.find(w) != std::string::npos) {
                matches++;
            }
        }
        
        double match_ratio = static_cast<double>(matches) / claim_words.size();
        
        if (match_ratio >= 0.8) {
            return SupportLevel::FULLY_SUPPORTED;
        } else if (match_ratio >= 0.4) {
            return SupportLevel::PARTIALLY_SUPPORTED;
        } else {
            return SupportLevel::UNSUPPORTED;
        }
    }
};

FaithfulnessEvaluator::FaithfulnessEvaluator()
    : FaithfulnessEvaluator(Config{}) {
}

FaithfulnessEvaluator::FaithfulnessEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration for claim extraction
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.3;
    llm_config.max_tokens = 512;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    // Initialize NLI verifier for faithfulness checking
    NLIFaithfulnessVerifier::Config nli_config;
    nli_config.entailment_threshold = 0.7;
    nli_config.max_claims = config.max_claims_to_extract;
    impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
    
    THEMIS_DEBUG("FaithfulnessEvaluator initialized with NLI support");
}

FaithfulnessEvaluator::~FaithfulnessEvaluator() = default;

std::vector<Claim> FaithfulnessEvaluator::extractClaims(const std::string& answer) {
    std::vector<Claim> claims;
    
    if (answer.empty()) {
        return claims;
    }
    
    // Try LLM-based extraction first; fall back to sentence-boundary heuristic
    if (impl_->llm_integration) {
        try {
            std::string prompt =
                "Extract factual claims from the following text as a JSON array.\n"
                "Return only standalone factual statements (not opinions or questions).\n"
                "Format: {\"claims\": [\"claim1\", \"claim2\", ...]}\n\n"
                "Text:\n" + answer + "\n\nJSON:\n";

            std::string response = impl_->llm_integration->evaluateDimension(
                prompt, EvaluationDimension::FAITHFULNESS
            );

            auto json_resp = nlohmann::json::parse(response);
            if (json_resp.contains("claims") && json_resp["claims"].is_array()) {
                for (const auto& item : json_resp["claims"]) {
                    if (item.is_string() && static_cast<int>(claims.size()) < impl_->config.max_claims_to_extract) {
                        std::string text = item.get<std::string>();
                        if (!text.empty()) {
                            Claim claim;
                            claim.text = std::move(text);
                            claim.category = "factual";
                            claim.support_level = SupportLevel::UNSUPPORTED;
                            claim.confidence = 0.9;
                            claims.push_back(std::move(claim));
                        }
                    }
                }
                THEMIS_DEBUG("LLM extracted {} claims",static_cast<int>(claims.size()));
                return claims;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("LLM claim extraction failed: {}, falling back to heuristic", e.what());
            claims.clear();
        }
    }

    // Fallback: sentence-boundary heuristic
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    
    for (auto it = sentences_begin; it != sentences_end && static_cast<int>(claims.size()) < impl_->config.max_claims_to_extract; ++it) {
        Claim claim;
        claim.text = it->str();
        claim.category = "factual";
        claim.support_level = SupportLevel::UNSUPPORTED;
        claim.confidence = 0.6;  // Lower confidence for heuristic extraction
        claims.push_back(claim);
    }
    
    THEMIS_DEBUG("Extracted {} claims from answer",static_cast<int>(claims.size()));
    return claims;
}

SupportLevel FaithfulnessEvaluator::checkEntailment(
    const std::string& claim,
    const std::vector<std::pair<std::string, std::string>>& documents
) {
    if (claim.empty() || documents.empty()) {
        return SupportLevel::UNSUPPORTED;
    }
    
    // Check claim against each document using NLI
    SupportLevel best_support = SupportLevel::UNSUPPORTED;
    
    for (const auto& [doc_id, doc_content] : documents) {
        SupportLevel support = impl_->checkNLIEntailment(claim, doc_content);
        
        // Keep the best (highest) support level found
        if (static_cast<int>(support) > static_cast<int>(best_support)) {
            best_support = support;
        }
        
        // If fully supported, no need to check further
        if (best_support == SupportLevel::FULLY_SUPPORTED) {
            break;
        }
    }
    
    return best_support;
}

std::vector<Citation> FaithfulnessEvaluator::verifyCitations(
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::vector<Claim>& claims
) {
    std::vector<Citation> citations;
    static_cast<void>(documents);
    
    if (!impl_->config.enable_citation_check) {
        return citations;
    }
    
    // Look for explicit citation patterns: [1], [doc1], (Source: X), etc.
    std::regex citation_pattern(R"(\[(\d+|doc\d+|[A-Za-z0-9_-]+)\]|\(Source:\s*([^)]+)\))");
    
    std::set<std::string> cited_doc_ids;
    auto citations_begin = std::sregex_iterator(answer.begin(), answer.end(), citation_pattern);
    auto citations_end = std::sregex_iterator();
    
    for (auto it = citations_begin; it != citations_end; ++it) {
        std::string matched = it->str();
        cited_doc_ids.insert(matched);
    }
    
    // Create citation entries
    for (const auto& claim : claims) {
        if (!claim.supporting_doc_ids.empty()) {
            Citation citation;
            citation.claim_text = claim.text;
            citation.cited_doc_ids = claim.supporting_doc_ids;
            citation.has_explicit_reference = !cited_doc_ids.empty();
            
            // Quality score based on support level and citation presence
            double support_score = 0.0;
            switch (claim.support_level) {
                case SupportLevel::FULLY_SUPPORTED:
                    support_score = 1.0;
                    break;
                case SupportLevel::PARTIALLY_SUPPORTED:
                    support_score = 0.5;
                    break;
                default:
                    support_score = 0.0;
            }
            
            double citation_score = citation.has_explicit_reference ? 1.0 : 0.5;
            citation.quality_score = (support_score + citation_score) / 2.0;
            
            citations.push_back(citation);
        }
    }
    
    THEMIS_DEBUG("Verified {} citations",static_cast<int>(citations.size()));
    return citations;
}

FaithfulnessResult FaithfulnessEvaluator::evaluate(
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& query
) {
    FaithfulnessResult result;
    static_cast<void>(query);
    
    // Step 1: Extract claims
    result.claims = extractClaims(answer);
    result.total_claims_count = result.claims.size();
    
    if (result.claims.empty()) {
        result.faithfulness_score = 1.0;  // No claims = vacuously true
        result.explanation = "No factual claims found in answer.";
        return result;
    }
    
    // Step 2: Check entailment for each claim
    result.supported_claims_count = 0;
    
    for (auto& claim : result.claims) {
        claim.support_level = checkEntailment(claim.text, documents);
        
        // Track which documents support this claim
        for (const auto& [doc_id, doc_content] : documents) {
            SupportLevel support = impl_->checkNLIEntailment(claim.text, doc_content);
            if (support != SupportLevel::UNSUPPORTED) {
                claim.supporting_doc_ids.push_back(doc_id);
            }
        }
        
        if (claim.support_level == SupportLevel::FULLY_SUPPORTED ||
            claim.support_level == SupportLevel::PARTIALLY_SUPPORTED) {
            result.supported_claims_count++;
        }
    }
    
    // Step 3: Verify citations
    result.citations = verifyCitations(answer, documents, result.claims);
    
    // Calculate citation quality score
    if (!result.citations.empty()) {
        double total_quality = 0.0;
        for (const auto& citation : result.citations) {
            total_quality += citation.quality_score;
        }
        result.citation_quality_score = total_quality / result.citations.size();
    } else {
        result.citation_quality_score = 0.0;
    }
    
    // Step 4: Calculate overall faithfulness score
    // Score based on: (supported claims / total claims) * 0.8 + citation quality * 0.2
    double claim_support_ratio = result.total_claims_count > 0 
        ? static_cast<double>(result.supported_claims_count) / result.total_claims_count
        : 1.0;
    
    result.faithfulness_score = claim_support_ratio * 0.8 + result.citation_quality_score * 0.2;
    
    // Generate explanation
    std::ostringstream explanation = {};
    explanation << "Faithfulness Score: " << result.faithfulness_score << "\n";
    explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << " supported\n";
    explanation << "Citation Quality: " << result.citation_quality_score << "\n";
    
    if (result.supported_claims_count < result.total_claims_count) {
        explanation << "Some claims lack sufficient document support.";
    } else {
        explanation << "All claims are supported by retrieved documents.";
    }
    
    result.explanation = explanation.str();
    
    THEMIS_INFO("Faithfulness evaluation complete: score={:.2f}, claims={}/{}", 
                result.faithfulness_score, result.supported_claims_count, result.total_claims_count);
    
    return result;
}

} // namespace themis::rag::judge


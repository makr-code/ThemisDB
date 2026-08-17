/**
 * @file pairwise_comparator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/pairwise_comparator.h"
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <mutex>
#include <random>
#include <cmath>

namespace themis::rag::judge {

using json = nlohmann::json;

struct PairwiseComparator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    std::mt19937 rng;
    mutable std::mutex scores_mutex;
    
    Impl() : rng(std::random_device{}()) {}
    
    // Generate comparison prompt
    std::string generateComparisonPrompt(
        const std::string& query,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& answer_first,
        const std::string& answer_second,
        const std::string& label_first,
        const std::string& label_second
    ) {
        std::ostringstream prompt;
        
        prompt << "Compare two answers to the following query and determine which is better.\n\n";
        prompt << "Query: " << query << "\n\n";
        
        // Add documents
        if (!documents.empty()) {
            prompt << "Retrieved Documents:\n";
            for (size_t i = 0; i < documents.size(); ++i) {
                prompt << "Document " << (i+1) << ": " << documents[i].second << "\n";
            }
            prompt << "\n";
        }
        
        prompt << "Answer " << label_first << ":\n" << answer_first << "\n\n";
        prompt << "Answer " << label_second << ":\n" << answer_second << "\n\n";
        
        prompt << R"(Evaluate both answers on these criteria:
1. Faithfulness - Supported by documents
2. Relevance - Addresses the query
3. Completeness - Covers all aspects
4. Coherence - Well-structured and clear

Respond in JSON format:
{
  "winner": "A" or "B" or "TIE",
  "confidence": 0.0-1.0,
  "reasoning": "Brief explanation of why one is better",
  "faithfulness_winner": "A" or "B" or "TIE",
  "relevance_winner": "A" or "B" or "TIE",
  "completeness_winner": "A" or "B" or "TIE",
  "coherence_winner": "A" or "B" or "TIE"
}

Winner:)";
        
        return prompt.str();
    }
    
    // Parse comparison response
    ComparisonWinner parseComparisonResult(const std::string& response) {
        try {
            json j = parser.parseJSONResponse(response);
            
            if (j.contains("winner")) {
                std::string winner_str = j["winner"].get<std::string>();
                if (winner_str == "A") return ComparisonWinner::ANSWER_A;
                if (winner_str == "B") return ComparisonWinner::ANSWER_B;
                return ComparisonWinner::TIE;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse comparison result: {}", e.what());
            
            // Fallback: look for winner in text
            std::string resp_lower = response;
            std::transform(resp_lower.begin(), resp_lower.end(), resp_lower.begin(), ::tolower);
            
            if (resp_lower.find("answer a") != std::string::npos && 
                resp_lower.find("better") != std::string::npos) {
                return ComparisonWinner::ANSWER_A;
            }
            if (resp_lower.find("answer b") != std::string::npos && 
                resp_lower.find("better") != std::string::npos) {
                return ComparisonWinner::ANSWER_B;
            }
        }
        
        return ComparisonWinner::TIE;
    }
};

PairwiseComparator::PairwiseComparator()
    : PairwiseComparator(Config{}) {
}

PairwiseComparator::PairwiseComparator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.3;
    llm_config.max_tokens = 512;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_DEBUG("PairwiseComparator initialized with bias strategy: {}", 
                 static_cast<int>(config.bias_strategy));
}

PairwiseComparator::~PairwiseComparator() = default;

ComparisonWinner PairwiseComparator::compareWithLLM(
    const std::string& query,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& answer_a,
    const std::string& answer_b,
    bool order_a_first
) {
    std::string prompt;
    
    if (order_a_first) {
        prompt = impl_->generateComparisonPrompt(
            query, documents, answer_a, answer_b, "A", "B"
        );
    } else {
        prompt = impl_->generateComparisonPrompt(
            query, documents, answer_b, answer_a, "A", "B"
        );
    }
    
    try {
        std::string response = impl_->llm_integration->evaluateDimension(
            prompt, EvaluationDimension::OVERALL
        );
        
        ComparisonWinner winner = impl_->parseComparisonResult(response);
        
        // If order was flipped, flip the result back
        if (!order_a_first) {
            if (winner == ComparisonWinner::ANSWER_A) {
                return ComparisonWinner::ANSWER_B;
            } else if (winner == ComparisonWinner::ANSWER_B) {
                return ComparisonWinner::ANSWER_A;
            }
        }
        
        return winner;
        
    } catch (const std::exception& e) {
        THEMIS_WARN("LLM comparison failed: {}", e.what());
        return ComparisonWinner::TIE;
    }
}

double PairwiseComparator::detectPositionBias(
    ComparisonWinner forward_result,
    ComparisonWinner reverse_result
) {
    // If results agree, no position bias
    if (forward_result == reverse_result) {
        return 0.0;
    }
    
    // If one is TIE, partial bias
    if (forward_result == ComparisonWinner::TIE || 
        reverse_result == ComparisonWinner::TIE) {
        return 0.3;
    }
    
    // If results are opposite (A wins forward, B wins reverse), strong bias
    if ((forward_result == ComparisonWinner::ANSWER_A && 
         reverse_result == ComparisonWinner::ANSWER_B) ||
        (forward_result == ComparisonWinner::ANSWER_B && 
         reverse_result == ComparisonWinner::ANSWER_A)) {
        return 1.0;
    }
    
    return 0.5;
}

PairwiseComparisonResult PairwiseComparator::compare(
    const std::string& query,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& answer_a,
    const std::string& answer_b
) {
    PairwiseComparisonResult result;
    result.num_evaluations = 0;
    result.flip_tested = false;
    result.position_bias_detected = false;
    result.position_bias_magnitude = 0.0;
    
    THEMIS_DEBUG("Starting pairwise comparison with strategy: {}", 
                 static_cast<int>(impl_->config.bias_strategy));
    
    switch (impl_->config.bias_strategy) {
        case BiasMitigationStrategy::NONE: {
            // Simple comparison, A presented first
            result.overall_winner = compareWithLLM(query, documents, answer_a, answer_b, true);
            result.num_evaluations = 1;
            result.overall_confidence = 0.7;
            break;
        }
        
        case BiasMitigationStrategy::RANDOMIZE_ORDER: {
            // Random order
            std::uniform_int_distribution<> dist(0, 1);
            bool a_first = dist(impl_->rng) == 0;
            
            result.overall_winner = compareWithLLM(query, documents, answer_a, answer_b, a_first);
            result.num_evaluations = 1;
            result.overall_confidence = 0.75;
            break;
        }
        
        case BiasMitigationStrategy::FLIP_AND_AVERAGE: {
            // Evaluate both orders
            ComparisonWinner forward = compareWithLLM(query, documents, answer_a, answer_b, true);
            ComparisonWinner reverse = compareWithLLM(query, documents, answer_a, answer_b, false);
            
            result.num_evaluations = 2;
            result.flip_tested = true;
            
            // Detect bias
            result.position_bias_magnitude = detectPositionBias(forward, reverse);
            result.position_bias_detected = result.position_bias_magnitude > 0.3;
            
            // Resolve winner
            if (forward == reverse) {
                // Agreement
                result.overall_winner = forward;
                result.overall_confidence = 0.9;
            } else if (forward == ComparisonWinner::TIE || reverse == ComparisonWinner::TIE) {
                // One is tie, use the other
                result.overall_winner = (forward == ComparisonWinner::TIE) ? reverse : forward;
                result.overall_confidence = 0.6;
            } else {
                // Disagreement - default to TIE
                result.overall_winner = ComparisonWinner::TIE;
                result.overall_confidence = 0.5;
            }
            
            break;
        }
        
        case BiasMitigationStrategy::MULTI_SAMPLE: {
            // Multiple evaluations with random orders
            std::vector<ComparisonWinner> results;
            
            int num_samples;
            {
                std::lock_guard<std::mutex> lock(impl_->scores_mutex);
                num_samples = impl_->config.num_samples;
            }
            
            results.reserve(num_samples);
            std::uniform_int_distribution<> dist(0, 1);
             
            for (int i = 0; i < num_samples; ++i) {
                bool a_first;
                {
                    std::lock_guard<std::mutex> lock(impl_->scores_mutex);
                    a_first = dist(impl_->rng) == 0;
                }
                results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));
            }
            
            result.num_evaluations = num_samples;
            
            // Count votes
            int a_votes = 0, b_votes = 0, tie_votes = 0;
            for (auto winner : results) {
                if (winner == ComparisonWinner::ANSWER_A) a_votes++;
                else if (winner == ComparisonWinner::ANSWER_B) b_votes++;
                else tie_votes++;
            }
            
            // Determine winner by majority
            if (a_votes > b_votes && a_votes > tie_votes) {
                result.overall_winner = ComparisonWinner::ANSWER_A;
                result.overall_confidence = static_cast<double>(a_votes) / num_samples;
            } else if (b_votes > a_votes && b_votes > tie_votes) {
                result.overall_winner = ComparisonWinner::ANSWER_B;
                result.overall_confidence = static_cast<double>(b_votes) / num_samples;
            } else {
                result.overall_winner = ComparisonWinner::TIE;
                result.overall_confidence = 0.5;
            }
            
            break;
        }
    }
    
    // Generate reasoning
    std::ostringstream reasoning;
    reasoning << "Winner: ";
    switch (result.overall_winner) {
        case ComparisonWinner::ANSWER_A: reasoning << "Answer A"; break;
        case ComparisonWinner::ANSWER_B: reasoning << "Answer B"; break;
        case ComparisonWinner::TIE: reasoning << "Tie"; break;
    }
    reasoning << " (confidence: " << result.overall_confidence << ")";
    
    if (result.position_bias_detected) {
        reasoning << "\nWarning: Position bias detected (magnitude: " 
                  << result.position_bias_magnitude << ")";
    }
    
    reasoning << "\nEvaluations performed: " << result.num_evaluations;
    
    result.overall_reasoning = reasoning.str();
    
    THEMIS_INFO("Pairwise comparison complete: winner={}, confidence={:.2f}, bias={:.2f}",
                static_cast<int>(result.overall_winner), result.overall_confidence, 
                result.position_bias_magnitude);
    
    return result;
}

} // namespace themis::rag::judge


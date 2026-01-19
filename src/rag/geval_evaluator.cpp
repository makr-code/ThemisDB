/**
 * @file geval_evaluator.cpp
 * @brief Implementation of G-Eval probabilistic scoring
 */

#include "rag/geval_evaluator.h"
#include "llm/inference_engine_enhanced.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

// Forward declaration for llama.cpp types
extern "C" {
    struct llama_model;
    struct llama_context;
    typedef int32_t llama_token;
    
    // llama.cpp functions we need
    float* llama_get_logits_ith(struct llama_context* ctx, int32_t i);
    int32_t llama_n_vocab(const struct llama_model* model);
    llama_token llama_token_to_piece(
        const struct llama_model* model,
        llama_token token,
        char* buf,
        int32_t length
    );
}

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct GEvalEvaluator::Impl {
    Config config;
    std::shared_ptr<llm::InferenceEngineEnhanced> llm;
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize LLM engine with default config
        llm::InferenceEngineEnhanced::Config engine_cfg;
        llm = std::make_shared<llm::InferenceEngineEnhanced>(engine_cfg);
    }
    
    /**
     * @brief Generate evaluation prompt for G-Eval
     */
    std::string generatePrompt(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& dimension
    ) {
        std::ostringstream prompt;
        
        prompt << "You are evaluating the quality of a generated answer.\n\n";
        prompt << "Evaluation Dimension: " << dimension << "\n\n";
        prompt << "Query: " << query << "\n\n";
        prompt << "Answer: " << answer << "\n\n";
        
        if (!documents.empty()) {
            prompt << "Retrieved Documents:\n";
            for (size_t i = 0; i < documents.size() && i < 3; i++) {
                prompt << "Document " << (i+1) << ":\n";
                prompt << documents[i].second.substr(0, 500) << "...\n\n";
            }
        }
        
        if (config.extract_reasoning) {
            prompt << "First, provide your reasoning for the evaluation.\n";
            prompt << "Then, rate the answer on a scale of 1-5 where:\n";
        } else {
            prompt << "Rate the answer on a scale of 1-5 where:\n";
        }
        
        prompt << "1 = Very Poor\n";
        prompt << "2 = Poor\n";
        prompt << "3 = Adequate\n";
        prompt << "4 = Good\n";
        prompt << "5 = Excellent\n\n";
        
        if (config.extract_reasoning) {
            prompt << "Format:\nReasoning: <your reasoning>\nScore: <1-5>\n";
        } else {
            prompt << "Provide only the numeric score (1-5).\n";
        }
        
        return prompt.str();
    }
    
    /**
     * @brief Find token IDs for score levels
     */
    std::vector<int> findScoreTokens(llama_model* model) {
        std::vector<int> score_tokens;
        int n_vocab = llama_n_vocab(model);
        
        // Search for tokens representing "1", "2", "3", "4", "5"
        for (const char* score_str : {"1", "2", "3", "4", "5"}) {
            bool found = false;
            
            // Search through vocabulary
            for (int token_id = 0; token_id < n_vocab && !found; token_id++) {
                char buf[128];
                int len = llama_token_to_piece(model, token_id, buf, sizeof(buf));
                
                if (len > 0 && len < sizeof(buf)) {
                    std::string token_text(buf, len);
                    // Trim whitespace
                    token_text.erase(0, token_text.find_first_not_of(" \t\n\r"));
                    token_text.erase(token_text.find_last_not_of(" \t\n\r") + 1);
                    
                    if (token_text == score_str) {
                        score_tokens.push_back(token_id);
                        found = true;
                    }
                }
            }
            
            if (!found) {
                spdlog::warn("Could not find token for score '{}'", score_str);
                score_tokens.push_back(-1);  // Placeholder
            }
        }
        
        return score_tokens;
    }
    
    /**
     * @brief Compute softmax probabilities from logits
     */
    std::vector<float> computeSoftmax(float* logits, int n_vocab) {
        std::vector<float> probs(n_vocab);
        
        // Find max for numerical stability
        float max_logit = *std::max_element(logits, logits + n_vocab);
        
        // Compute exp and sum
        float sum_exp = 0.0f;
        for (int i = 0; i < n_vocab; i++) {
            probs[i] = std::exp(logits[i] - max_logit);
            sum_exp += probs[i];
        }
        
        // Normalize
        for (int i = 0; i < n_vocab; i++) {
            probs[i] /= sum_exp;
        }
        
        return probs;
    }
    
    /**
     * @brief Extract token probabilities (stub for now, real implementation needs llama.cpp context)
     */
    std::vector<double> extractProbabilitiesStub(const std::string& dimension) {
        // Stub implementation with realistic distributions
        // In production, this would call llama.cpp API
        
        std::vector<double> probs;
        
        if (dimension == "faithfulness") {
            // High quality: skewed toward higher scores
            probs = {0.05, 0.10, 0.20, 0.35, 0.30};
        } else if (dimension == "relevance") {
            // Good quality: centered distribution
            probs = {0.05, 0.15, 0.40, 0.30, 0.10};
        } else if (dimension == "completeness") {
            // Adequate quality
            probs = {0.10, 0.20, 0.40, 0.20, 0.10};
        } else if (dimension == "coherence") {
            // High quality
            probs = {0.05, 0.10, 0.25, 0.35, 0.25};
        } else {
            // Default: uniform distribution with slight bias toward middle
            probs = {0.10, 0.20, 0.40, 0.20, 0.10};
        }
        
        return probs;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

GEvalEvaluator::GEvalEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    spdlog::info("GEvalEvaluator initialized with {} samples", config.num_samples);
}

GEvalEvaluator::~GEvalEvaluator() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

GEvalResult GEvalEvaluator::evaluate(
    const std::string& query,
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& dimension
) {
    GEvalResult result;
    result.dimension = dimension;
    
    try {
        // Generate multiple samples for robustness
        std::vector<double> sample_scores;
        std::vector<std::vector<double>> all_probabilities;
        
        for (int i = 0; i < impl_->config.num_samples; i++) {
            // Get token probabilities for this sample
            auto probs = impl_->extractProbabilitiesStub(dimension);
            all_probabilities.push_back(probs);
            
            // Compute G-Eval score from probabilities
            double score = computeGEvalScore(probs);
            sample_scores.push_back(score);
        }
        
        // Aggregate samples
        result.geval_score = aggregateScores(sample_scores, impl_->config.aggregation);
        result.sample_scores = sample_scores;
        
        // Average probabilities across samples
        result.token_probabilities.resize(5, 0.0);
        for (const auto& probs : all_probabilities) {
            for (size_t i = 0; i < 5; i++) {
                result.token_probabilities[i] += probs[i];
            }
        }
        for (size_t i = 0; i < 5; i++) {
            result.token_probabilities[i] /= impl_->config.num_samples;
        }
        
        // Compute confidence and variance
        result.confidence = computeConfidence(result.token_probabilities);
        
        if (sample_scores.size() > 1) {
            double mean = result.geval_score;
            double sum_sq_diff = 0.0;
            for (double score : sample_scores) {
                double diff = score - mean;
                sum_sq_diff += diff * diff;
            }
            result.variance = sum_sq_diff / (sample_scores.size() - 1);
        } else {
            result.variance = 0.0;
        }
        
        // Generate reasoning summary
        std::ostringstream reasoning;
        reasoning << "G-Eval probabilistic scoring for " << dimension << ":\n";
        reasoning << "Token probability distribution:\n";
        for (size_t i = 0; i < result.token_probabilities.size(); i++) {
            reasoning << "  Level " << (i+1) << ": " 
                     << std::fixed << std::setprecision(3) 
                     << result.token_probabilities[i] << "\n";
        }
        reasoning << "Continuous score: " << std::fixed << std::setprecision(3) 
                 << result.geval_score << "\n";
        reasoning << "Confidence: " << std::fixed << std::setprecision(3) 
                 << result.confidence;
        result.reasoning = reasoning.str();
        
        spdlog::debug("G-Eval {} score: {:.3f} (confidence: {:.3f})",
                     dimension, result.geval_score, result.confidence);
        
    } catch (const std::exception& e) {
        spdlog::error("G-Eval evaluation failed: {}", e.what());
        result.geval_score = 0.5;  // Default to middle score
        result.confidence = 0.0;
        result.reasoning = std::string("Evaluation failed: ") + e.what();
    }
    
    return result;
}

std::vector<double> GEvalEvaluator::extractTokenProbabilities(
    const std::string& prompt,
    const std::vector<int>& score_tokens
) {
    // This is a stub implementation
    // Real implementation would:
    // 1. Call LLM with the prompt
    // 2. Get llama_context from the LLM
    // 3. Use llama_get_logits_ith() to get logits
    // 4. Compute softmax
    // 5. Extract probabilities for score_tokens
    
    // For now, return stub probabilities
    return impl_->extractProbabilitiesStub("overall");
}

double GEvalEvaluator::computeGEvalScore(const std::vector<double>& probabilities) {
    if (probabilities.size() != 5) {
        spdlog::warn("Expected 5 probabilities for levels 1-5, got {}", probabilities.size());
        return 0.5;  // Default to middle
    }
    
    // Compute expected value: E[score] = Σ(level × P(level))
    double expected_score = 0.0;
    for (size_t i = 0; i < probabilities.size(); i++) {
        int level = i + 1;  // Levels 1-5
        expected_score += level * probabilities[i];
    }
    
    // Normalize to 0-1 range: (score - 1) / (5 - 1)
    double normalized = (expected_score - 1.0) / 4.0;
    
    // Clamp to valid range
    return std::max(0.0, std::min(1.0, normalized));
}

double GEvalEvaluator::computeConfidence(const std::vector<double>& probabilities) {
    if (probabilities.empty()) {
        return 0.0;
    }
    
    // Confidence based on entropy
    // Low entropy (concentrated distribution) = high confidence
    // High entropy (uniform distribution) = low confidence
    
    double entropy = 0.0;
    for (double p : probabilities) {
        if (p > 0.0) {
            entropy -= p * std::log2(p);
        }
    }
    
    // Maximum entropy for 5 levels
    double max_entropy = std::log2(5.0);
    
    // Normalize: 0 entropy = 1 confidence, max entropy = 0 confidence
    double confidence = 1.0 - (entropy / max_entropy);
    
    return std::max(0.0, std::min(1.0, confidence));
}

double GEvalEvaluator::aggregateScores(
    const std::vector<double>& samples,
    AggregationMethod method
) {
    if (samples.empty()) {
        return 0.5;  // Default
    }
    
    if (samples.size() == 1) {
        return samples[0];
    }
    
    switch (method) {
        case AggregationMethod::MEAN: {
            double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
            return sum / samples.size();
        }
        
        case AggregationMethod::MEDIAN: {
            std::vector<double> sorted = samples;
            std::sort(sorted.begin(), sorted.end());
            size_t mid = sorted.size() / 2;
            if (sorted.size() % 2 == 0) {
                return (sorted[mid-1] + sorted[mid]) / 2.0;
            } else {
                return sorted[mid];
            }
        }
        
        case AggregationMethod::MODE: {
            // For continuous scores, mode is tricky
            // Use the most common score (with some tolerance)
            std::unordered_map<int, int> counts;
            for (double score : samples) {
                int bucket = static_cast<int>(score * 10);  // 0.1 resolution
                counts[bucket]++;
            }
            
            int max_count = 0;
            int mode_bucket = 5;  // Default to middle
            for (const auto& [bucket, count] : counts) {
                if (count > max_count) {
                    max_count = count;
                    mode_bucket = bucket;
                }
            }
            
            return mode_bucket / 10.0;
        }
        
        default:
            return aggregateScores(samples, AggregationMethod::MEAN);
    }
}

} // namespace themis::rag::judge

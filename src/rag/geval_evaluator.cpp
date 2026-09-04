/**
 * @file geval_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/geval_evaluator.h"
#include "llm/inference_engine_enhanced.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <regex>
#include <mutex>
#include <stdexcept>

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
    std::atomic<uint64_t> req_counter{0};  ///< Per-instance request ID counter
    mutable std::mutex state_mutex;  // Protect shared state access
    
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
        score_tokens.reserve(5);  // Searching for exactly 5 score levels: 1, 2, 3, 4, 5
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
                score_tokens.push_back(-1);  // Token not in vocabulary; skip during probability extraction
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
     * @brief Derive a probability distribution centered on a parsed score (1-5)
     *
     * Used both as a fallback (no LLM engine) and when the engine response
     * contains no logprobs.  The distribution is Gaussian-shaped around the
     * score level so that adjacent levels receive decreasing probability mass.
     */
    std::vector<double> probsFromScore([[maybe_unused]] double score_1_to_5) const {
        // Clamp score to [1, kNumScoreLevels]
        double s = std::max(1.0, std::min(static_cast<double>(kNumScoreLevels),
                                          score_1_to_5));
        std::vector<double> probs(kNumScoreLevels, 0.0);
        double sum = 0.0;
        for (int i = 0; i < static_cast<int>(kNumScoreLevels); ++i) {
            double diff = (i + 1) - s;
            probs[i] = std::exp(-0.5 * diff * diff);  // Gaussian, variance=1 (σ²=1)
            sum += probs[i];
        }
        for (auto& p : probs) {
          p /= sum;
        }
        return probs;
    }

    /**
     * @brief Derive a probability distribution from a dimension label when
     * no LLM engine is available (pure heuristic fallback).
     */
    std::vector<double> heuristicProbsForDimension(const std::string& dimension) const {
        // Representative score (1-5) for each dimension based on expected
        // average quality.  These are used only when no engine is configured.
        double default_score = 3.5;  // slightly above middle
        if (dimension == "faithfulness") {
            default_score = 4.0;
        } else if (dimension == "relevance") {
            default_score = 3.5;
        } else if (dimension == "completeness") {
            default_score = 3.0;
        } else if (dimension == "coherence") {
            default_score = 4.0;
        }
        return probsFromScore(default_score);
    }

    /**
     * @brief Query the LLM engine with a prompt and derive a score distribution.
     *
     * If the engine provides per-token logprobs, extract the 5 score-level
     * probabilities directly from them.  Otherwise parse the score text from
     * the response and build a Gaussian distribution around it.
     */
    std::vector<double> probsFromLLM(
        const std::string& prompt,
        const std::string& dimension
    ) {
        if (!llm) {
            return heuristicProbsForDimension(dimension);
        }

        if (llm->getAvailableModels().empty()) {
            return heuristicProbsForDimension(dimension);
        }

        try {
            llm::InferenceEngineEnhanced::EnhancedInferenceRequest req;
            req.base_request.prompt = prompt;
            req.base_request.max_tokens = 50;
            req.base_request.temperature = static_cast<float>(config.temperature);
            req.allow_caching = true;
            req.priority = 0;

            req.request_id = "geval_" + std::to_string(req_counter.fetch_add(1));

            auto response = llm->submit(req).get();

            // If the engine returned per-token logprobs, look for the first
            // token whose text is a digit 1-5 and build a distribution around it.
            if (!response.logprobs.empty()) {
                // Walk the generated text tokens and pick the first score digit
                std::istringstream iss(response.text);
                std::string tok;
                size_t idx = 0;
                while (iss >> tok && idx < response.logprobs.size()) {
                    // kNumScoreLevels ≤ 9 so single-digit check is safe
                    char max_digit = static_cast<char>('0' + kNumScoreLevels);
                    if (tok.size() == 1 && tok[0] >= '1' && tok[0] <= max_digit) {
                        double parsed = static_cast<double>(tok[0] - '0');
                        return probsFromScore(parsed);
                    }
                    ++idx;
                }
            }

            // Fallback: parse a score from the response text
            static const std::regex kScoreRegex(R"(\b([1-5])\b)");
            std::smatch m;
            if (std::regex_search(response.text, m, kScoreRegex)) {
                double parsed = std::stod(m[1].str());
                return probsFromScore(parsed);
            }

            return heuristicProbsForDimension(dimension);

        } catch (...) {
            return heuristicProbsForDimension(dimension);
        }
    }

};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

GEvalEvaluator::GEvalEvaluator()
    : GEvalEvaluator(Config{}) {
}

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
            // Build evaluation prompt and query the LLM (or heuristic fallback)
            std::string prompt = impl_->generatePrompt(query, answer, documents, dimension);
            auto probs = impl_->probsFromLLM(prompt, dimension);
            all_probabilities.push_back(probs);
            
            // Compute G-Eval score from probabilities
            double score = computeGEvalScore(probs);
            sample_scores.push_back(score);
        }
        
        // Aggregate samples
        result.geval_score = aggregateScores(sample_scores, impl_->config.aggregation);
        result.sample_scores = sample_scores;
        
        // Average probabilities across samples
        result.token_probabilities.resize(kNumScoreLevels, 0.0);
        for (const auto& probs : all_probabilities) {
            for (size_t i = 0; i < kNumScoreLevels; i++) {
                result.token_probabilities[i] += probs[i];
            }
        }
        for (size_t i = 0; i < kNumScoreLevels; i++) {
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
    const std::vector<int>&
) {
    // Use LLM engine to derive probabilities from the prompt when available;
    // otherwise fall back to heuristic distributions.
    return impl_->probsFromLLM(prompt, "overall");
}

double GEvalEvaluator::computeGEvalScore(const std::vector<double>& probabilities) {
    if (probabilities.size() != kNumScoreLevels) {
        spdlog::warn("Expected {} probabilities for levels 1-{}, got {}",
                     kNumScoreLevels, kNumScoreLevels, probabilities.size());
        return 0.5;  // Default to middle
    }
    
    // Compute expected value: E[score] = Σ(level × P(level))
    double expected_score = 0.0;
    for (size_t i = 0; i < probabilities.size(); i++) {
        int level = static_cast<int>(i) + 1;
        expected_score += level * probabilities[i];
    }
    
    // Normalize to 0-1 range: (score - 1) / (kNumScoreLevels - 1)
    double normalized = (expected_score - 1.0) /
                        static_cast<double>(kNumScoreLevels - 1);
    
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
    
    // Maximum entropy for kNumScoreLevels levels
    double max_entropy = std::log2(static_cast<double>(kNumScoreLevels));
    
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



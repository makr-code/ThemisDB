/**
 * @file nli_faithfulness_verifier.cpp
 * @brief Implementation of NLI-based Faithfulness Verification
 */

#include "rag/nli_faithfulness_verifier.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <regex>

// Stub for ONNX Runtime - in production would use actual ONNX Runtime API
// #include <onnxruntime_cxx_api.h>

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct NLIFaithfulnessVerifier::Impl {
    Config config;
    bool model_loaded = false;
    
    // Cache for claim-document pairs
    std::unordered_map<std::string, NLIResult> cache;
    std::mutex cache_mutex;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    // Stub: In production, would have ONNX Runtime session
    // std::unique_ptr<Ort::Session> ort_session;
    // std::unique_ptr<Ort::Env> ort_env;
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize ONNX Runtime environment (stub)
        // In production:
        // ort_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "NLIVerifier");
    }
    
    /**
     * @brief Stub NLI inference - uses heuristics
     * In production, this would run actual ONNX model
     */
    NLIResult stubInference(const std::string& claim, const std::string& document) {
        constexpr size_t MIN_WORD_LENGTH = 3;
        
        auto start = std::chrono::steady_clock::now();
        
        NLIResult result;
        result.success = true;
        
        // Simple heuristic: measure text overlap
        std::string claim_lower = claim;
        std::string doc_lower = document;
        std::transform(claim_lower.begin(), claim_lower.end(), claim_lower.begin(), ::tolower);
        std::transform(doc_lower.begin(), doc_lower.end(), doc_lower.begin(), ::tolower);
        
        // Extract key terms from claim
        std::vector<std::string> claim_terms;
        std::istringstream claim_stream(claim_lower);
        std::string word;
        while (claim_stream >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), 
                      [](char c) { return !std::isalnum(c); }), word.end());
            if (word.length() > MIN_WORD_LENGTH) {  // Skip short words
                claim_terms.push_back(word);
            }
        }
        
        if (claim_terms.empty()) {
            result.entailment_prob = 0.5;
            result.neutral_prob = 0.5;
            result.contradiction_prob = 0.0;
            result.label = NLILabel::NEUTRAL;
            result.confidence = 0.5;
            result.explanation = "Empty or very short claim";
        } else {
            // Count matches
            size_t matches = 0;
            for (const auto& term : claim_terms) {
                if (doc_lower.find(term) != std::string::npos) {
                    matches++;
                }
            }
            
            double match_ratio = static_cast<double>(matches) / claim_terms.size();
            
            // Convert to probabilities
            if (match_ratio >= 0.8) {
                // Strong overlap -> likely entailment
                result.entailment_prob = 0.7 + (match_ratio - 0.8) * 1.5;
                result.neutral_prob = 1.0 - result.entailment_prob;
                result.contradiction_prob = 0.0;
                result.label = NLILabel::ENTAILMENT;
            } else if (match_ratio >= 0.4) {
                // Medium overlap -> neutral
                result.entailment_prob = match_ratio * 0.5;
                result.neutral_prob = 0.8 - match_ratio * 0.4;
                result.contradiction_prob = 1.0 - result.entailment_prob - result.neutral_prob;
                result.label = NLILabel::NEUTRAL;
            } else {
                // Low overlap -> possibly contradiction or just unrelated
                result.entailment_prob = match_ratio * 0.3;
                result.neutral_prob = 0.7;
                result.contradiction_prob = 0.3 - result.entailment_prob;
                result.label = NLILabel::NEUTRAL;
            }
            
            // Clamp probabilities
            result.entailment_prob = std::max(0.0, std::min(1.0, result.entailment_prob));
            result.neutral_prob = std::max(0.0, std::min(1.0, result.neutral_prob));
            result.contradiction_prob = std::max(0.0, std::min(1.0, result.contradiction_prob));
            
            // Normalize
            double total = result.entailment_prob + result.neutral_prob + result.contradiction_prob;
            if (total > 0) {
                result.entailment_prob /= total;
                result.neutral_prob /= total;
                result.contradiction_prob /= total;
            }
            
            result.confidence = std::max({result.entailment_prob, result.neutral_prob, 
                                         result.contradiction_prob});
            
            std::ostringstream explanation;
            explanation << "Match ratio: " << std::fixed << std::setprecision(2) 
                       << (match_ratio * 100) << "% (" 
                       << matches << "/" << claim_terms.size() << " terms)";
            result.explanation = explanation.str();
        }
        
        result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        
        return result;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

NLIFaithfulnessVerifier::NLIFaithfulnessVerifier()
    : NLIFaithfulnessVerifier(Config{}) {
}

NLIFaithfulnessVerifier::NLIFaithfulnessVerifier(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    
    // Try to load model if paths provided
    if (!config.model_path.empty()) {
        if (loadModel(config.model_path, config.tokenizer_path)) {
            THEMIS_INFO("NLI Faithfulness Verifier initialized with model: {}", config.model_path);
        } else {
            THEMIS_WARN("NLI model loading failed, using heuristic fallback");
        }
    } else {
        THEMIS_INFO("NLI Faithfulness Verifier initialized with heuristic fallback (no model path)");
    }
}

NLIFaithfulnessVerifier::~NLIFaithfulnessVerifier() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

NLIResult NLIFaithfulnessVerifier::verifyClaim(
    const std::string& claim,
    const std::string& document
) {
    // Check cache
    if (impl_->config.enable_caching) {
        std::string cache_key = generateCacheKey(claim, document);
        std::lock_guard<std::mutex> lock(impl_->cache_mutex);
        
        auto it = impl_->cache.find(cache_key);
        if (it != impl_->cache.end()) {
            impl_->cache_hits++;
            THEMIS_DEBUG("NLI cache hit");
            return it->second;
        }
        impl_->cache_misses++;
    }
    
    // Perform inference (stub for now)
    NLIResult result;
    
    if (impl_->model_loaded) {
        // In production: run ONNX model
        // auto input_ids = tokenize(document, claim);
        // auto logits = runInference(input_ids);
        // result = interpretLogits(logits);
        
        // Stub
        result = impl_->stubInference(claim, document);
    } else {
        // Fallback heuristic
        result = impl_->stubInference(claim, document);
    }
    
    // Cache result
    if (impl_->config.enable_caching && result.success) {
        std::string cache_key = generateCacheKey(claim, document);
        std::lock_guard<std::mutex> lock(impl_->cache_mutex);
        impl_->cache[cache_key] = result;
    }
    
    return result;
}

std::vector<NLIResult> NLIFaithfulnessVerifier::verifyClaimsBatch(
    const std::vector<std::string>& claims,
    const std::string& document
) {
    std::vector<NLIResult> results;
    results.reserve(claims.size());
    
    // In production with ONNX: batch process for efficiency
    // For now: process sequentially
    for (const auto& claim : claims) {
        results.push_back(verifyClaim(claim, document));
    }
    
    return results;
}

NLIResult NLIFaithfulnessVerifier::verifyAgainstMultipleDocs(
    const std::string& claim,
    const std::vector<std::string>& documents
) {
    if (documents.empty()) {
        NLIResult result;
        result.success = false;
        result.label = NLILabel::NEUTRAL;
        result.entailment_prob = 0.0;
        result.neutral_prob = 1.0;
        result.contradiction_prob = 0.0;
        result.confidence = 0.0;
        result.explanation = "No documents provided";
        return result;
    }
    
    // Verify against each document and return best match
    NLIResult best_result;
    best_result.entailment_prob = 0.0;
    
    for (const auto& doc : documents) {
        auto result = verifyClaim(claim, doc);
        if (result.success && result.entailment_prob > best_result.entailment_prob) {
            best_result = result;
        }
    }
    
    return best_result;
}

bool NLIFaithfulnessVerifier::isReady() const {
    return impl_->model_loaded || true;  // Heuristic fallback always ready
}

bool NLIFaithfulnessVerifier::loadModel(
    const std::string& model_path,
    const std::string& tokenizer_path
) {
    // Stub implementation
    // In production:
    // 1. Load ONNX model using ONNX Runtime
    // 2. Load tokenizer configuration
    // 3. Verify model inputs/outputs match expected format
    
    THEMIS_INFO("NLI model loading stubbed - using heuristic fallback");
    THEMIS_INFO("Model path: {}", model_path);
    THEMIS_INFO("Tokenizer path: {}", tokenizer_path);
    
    // For now, mark as not loaded to use heuristic
    impl_->model_loaded = false;
    
    return false;  // Return false to indicate stub
}

NLIFaithfulnessVerifier::Config NLIFaithfulnessVerifier::getConfig() const {
    return impl_->config;
}

void NLIFaithfulnessVerifier::setConfig(const Config& config) {
    impl_->config = config;
}

void NLIFaithfulnessVerifier::clearCache() {
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    impl_->cache.clear();
    impl_->cache_hits = 0;
    impl_->cache_misses = 0;
    THEMIS_INFO("NLI cache cleared");
}

NLIFaithfulnessVerifier::CacheStats NLIFaithfulnessVerifier::getCacheStats() const {
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    CacheStats stats;
    stats.hits = impl_->cache_hits;
    stats.misses = impl_->cache_misses;
    size_t total = stats.hits + stats.misses;
    stats.hit_rate = total > 0 ? static_cast<double>(stats.hits) / total : 0.0;
    return stats;
}

// ═══════════════════════════════════════════════════════════
// Private Helpers
// ═══════════════════════════════════════════════════════════

std::vector<int> NLIFaithfulnessVerifier::tokenize(
    const std::string& premise,
    const std::string& hypothesis
) {
    // Stub: In production, use proper tokenizer (e.g., HuggingFace tokenizers)
    std::vector<int> tokens;
    // Would tokenize: [CLS] premise [SEP] hypothesis [SEP]
    return tokens;
}

std::vector<float> NLIFaithfulnessVerifier::runInference(const std::vector<int>& input_ids) {
    // Stub: In production, run ONNX model
    std::vector<float> logits = {0.0f, 0.0f, 0.0f};  // [entailment, neutral, contradiction]
    return logits;
}

NLIResult NLIFaithfulnessVerifier::interpretLogits(const std::vector<float>& logits) {
    // Stub: Convert logits to probabilities via softmax
    NLIResult result;
    result.success = true;
    
    // Softmax
    float max_logit = *std::max_element(logits.begin(), logits.end());
    std::vector<float> exp_logits;
    float sum_exp = 0.0f;
    
    for (float logit : logits) {
        float exp_val = std::exp(logit - max_logit);
        exp_logits.push_back(exp_val);
        sum_exp += exp_val;
    }
    
    for (float& exp_val : exp_logits) {
        exp_val /= sum_exp;
    }
    
    result.entailment_prob = exp_logits[0];
    result.neutral_prob = exp_logits[1];
    result.contradiction_prob = exp_logits[2];
    
    // Determine label
    if (result.entailment_prob >= result.neutral_prob && 
        result.entailment_prob >= result.contradiction_prob) {
        result.label = NLILabel::ENTAILMENT;
        result.confidence = result.entailment_prob;
    } else if (result.neutral_prob >= result.contradiction_prob) {
        result.label = NLILabel::NEUTRAL;
        result.confidence = result.neutral_prob;
    } else {
        result.label = NLILabel::CONTRADICTION;
        result.confidence = result.contradiction_prob;
    }
    
    return result;
}

std::string NLIFaithfulnessVerifier::generateCacheKey(
    const std::string& claim,
    const std::string& document
) {
    // Simple concatenation - in production might use hash
    return claim + "|" + document;
}

// ═══════════════════════════════════════════════════════════
// Utility Functions
// ═══════════════════════════════════════════════════════════

namespace nli_utils {

double labelToScore(NLILabel label) {
    switch (label) {
        case NLILabel::ENTAILMENT:
            return 1.0;
        case NLILabel::NEUTRAL:
            return 0.5;
        case NLILabel::CONTRADICTION:
            return 0.0;
        default:
            return 0.5;
    }
}

double aggregateFaithfulness(const std::vector<NLIResult>& results) {
    if (results.empty()) {
        return 0.5;  // Neutral if no results
    }
    
    // Compute average entailment probability
    double sum_entailment = 0.0;
    size_t valid_results = 0;
    
    for (const auto& result : results) {
        if (result.success) {
            sum_entailment += result.entailment_prob;
            valid_results++;
        }
    }
    
    return valid_results > 0 ? sum_entailment / valid_results : 0.5;
}

bool isFactualClaim(const std::string& claim) {
    // Simple heuristics to identify factual claims
    std::string lower = claim;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Opinion indicators
    std::vector<std::string> opinion_markers = {
        "i think", "i believe", "in my opinion", "it seems", "probably", 
        "maybe", "might", "could", "should", "would"
    };
    
    for (const auto& marker : opinion_markers) {
        if (lower.find(marker) != std::string::npos) {
            return false;
        }
    }
    
    // Reasoning indicators
    if (lower.find("because") != std::string::npos ||
        lower.find("therefore") != std::string::npos ||
        lower.find("thus") != std::string::npos) {
        return false;
    }
    
    // Likely factual if contains numbers, dates, or specific entities
    std::regex number_pattern(R"(\d+)");
    if (std::regex_search(claim, number_pattern)) {
        return true;
    }
    
    // Default to factual
    return true;
}

} // namespace nli_utils

} // namespace themis::rag::judge

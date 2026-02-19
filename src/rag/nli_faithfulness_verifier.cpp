/**
 * @file nli_faithfulness_verifier.cpp
 * @brief Implementation of NLI-based faithfulness verification
 */

#include "rag/nli_faithfulness_verifier.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct NLIFaithfulnessVerifier::Impl {
    Config config;
    bool model_loaded = false;
    
    // Cache for verified pairs to avoid redundant computation
    std::unordered_map<std::string, NLIResult> cache;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    Impl(const Config& cfg) : config(cfg) {
        // Attempt to load model
        if (!config.model_path.empty()) {
            loadModel();
        } else {
            THEMIS_WARN("No NLI model path specified, using heuristic fallback");
        }
    }
    
    /**
     * @brief Load NLI model (stub for now)
     * 
     * Full implementation would:
     * 1. Load tokenizer and model from config.model_path
     * 2. Initialize GPU/CPU inference
     * 3. Apply quantization if enabled
     * 4. Verify model is working
     */
    void loadModel() {
        // Stub: Model loading not implemented yet
        // In production, this would use ONNX Runtime, TensorRT, or PyTorch
        THEMIS_INFO("NLI model loading not yet implemented");
        model_loaded = false;
    }
    
    /**
     * @brief Generate cache key for premise-hypothesis pair
     */
    std::string getCacheKey(const std::string& premise, const std::string& hypothesis) {
        // Simple concatenation with separator
        return premise + "|||" + hypothesis;
    }
    
    /**
     * @brief Check cache for existing result
     */
    std::pair<bool, NLIResult> checkCache(
        const std::string& premise,
        const std::string& hypothesis
    ) {
        if (cache.size() > config.max_cache_size) {
            // Simple eviction: clear half the cache
            auto it = cache.begin();
            std::advance(it, cache.size() / 2);
            cache.erase(cache.begin(), it);
        }
        
        std::string key = getCacheKey(premise, hypothesis);
        auto it = cache.find(key);
        
        if (it != cache.end()) {
            cache_hits++;
            return {true, it->second};
        }
        
        cache_misses++;
        return {false, NLIResult{}};
    }
    
    /**
     * @brief Store result in cache
     */
    void storeInCache(
        const std::string& premise,
        const std::string& hypothesis,
        const NLIResult& result
    ) {
        std::string key = getCacheKey(premise, hypothesis);
        cache[key] = result;
    }
    
    /**
     * @brief Heuristic-based NLI fallback
     * 
     * Uses simple text similarity and overlap heuristics when model is unavailable.
     * Less accurate than model-based NLI but provides reasonable baseline.
     */
    NLIResult heuristicNLI(const std::string& premise, const std::string& hypothesis) {
        NLIResult result;
        
        // Normalize to lowercase
        std::string premise_lower = premise;
        std::string hyp_lower = hypothesis;
        std::transform(premise_lower.begin(), premise_lower.end(), 
                      premise_lower.begin(), ::tolower);
        std::transform(hyp_lower.begin(), hyp_lower.end(), 
                      hyp_lower.begin(), ::tolower);
        
        // Extract words from hypothesis
        std::istringstream hyp_stream(hyp_lower);
        std::vector<std::string> hyp_words;
        std::string word;
        while (hyp_stream >> word) {
            if (word.length() > 3) {  // Skip short words
                hyp_words.push_back(word);
            }
        }
        
        if (hyp_words.empty()) {
            result.prediction = NLIPrediction::NEUTRAL;
            result.entailment_score = 0.33;
            result.neutral_score = 0.34;
            result.contradiction_score = 0.33;
            result.confidence = 0.5;
            return result;
        }
        
        // Count word matches
        size_t matches = 0;
        for (const auto& w : hyp_words) {
            if (premise_lower.find(w) != std::string::npos) {
                matches++;
            }
        }
        
        double match_ratio = static_cast<double>(matches) / hyp_words.size();
        
        // Check for negation indicators in premise vs hypothesis
        bool premise_has_not = premise_lower.find(" not ") != std::string::npos ||
                               premise_lower.find("n't ") != std::string::npos;
        bool hyp_has_not = hyp_lower.find(" not ") != std::string::npos ||
                          hyp_lower.find("n't ") != std::string::npos;
        bool negation_mismatch = premise_has_not != hyp_has_not;
        
        // Determine prediction based on match ratio and negation
        if (negation_mismatch && match_ratio > 0.5) {
            // High overlap but negation mismatch suggests contradiction
            result.prediction = NLIPrediction::CONTRADICTION;
            result.entailment_score = 0.2;
            result.neutral_score = 0.2;
            result.contradiction_score = 0.6;
            result.confidence = 0.6;
        } else if (match_ratio >= 0.8) {
            // High overlap suggests entailment
            result.prediction = NLIPrediction::ENTAILMENT;
            result.entailment_score = 0.7;
            result.neutral_score = 0.2;
            result.contradiction_score = 0.1;
            result.confidence = 0.7;
        } else if (match_ratio >= 0.4) {
            // Medium overlap suggests possible entailment or neutral
            result.prediction = NLIPrediction::ENTAILMENT;
            result.entailment_score = 0.5;
            result.neutral_score = 0.4;
            result.contradiction_score = 0.1;
            result.confidence = 0.5;
        } else {
            // Low overlap suggests neutral or contradiction
            result.prediction = NLIPrediction::NEUTRAL;
            result.entailment_score = 0.2;
            result.neutral_score = 0.6;
            result.contradiction_score = 0.2;
            result.confidence = 0.5;
        }
        
        return result;
    }
    
    /**
     * @brief Model-based NLI prediction (stub)
     * 
     * Full implementation would:
     * 1. Tokenize premise and hypothesis
     * 2. Run inference through NLI model
     * 3. Get softmax scores for [entailment, neutral, contradiction]
     * 4. Return result with confidence
     */
    NLIResult modelNLI(const std::string& premise, const std::string& hypothesis) {
        // Stub: Not implemented yet
        THEMIS_DEBUG("Model-based NLI not implemented, using heuristic");
        return heuristicNLI(premise, hypothesis);
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
    THEMIS_INFO("NLIFaithfulnessVerifier initialized (using {} mode)", 
                impl_->model_loaded ? "model" : "heuristic");
}

NLIFaithfulnessVerifier::~NLIFaithfulnessVerifier() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

NLIResult NLIFaithfulnessVerifier::verify(
    const std::string& premise,
    const std::string& hypothesis
) {
    // Check cache first
    auto [cached, result] = impl_->checkCache(premise, hypothesis);
    if (cached) {
        return result;
    }
    
    // Perform verification
    if (impl_->model_loaded) {
        result = impl_->modelNLI(premise, hypothesis);
    } else if (impl_->config.use_heuristic_fallback) {
        result = impl_->heuristicNLI(premise, hypothesis);
    } else {
        // No model and no fallback - return neutral with low confidence
        result.prediction = NLIPrediction::NEUTRAL;
        result.entailment_score = 0.33;
        result.neutral_score = 0.34;
        result.contradiction_score = 0.33;
        result.confidence = 0.0;
    }
    
    // Store in cache
    impl_->storeInCache(premise, hypothesis, result);
    
    return result;
}

std::vector<NLIResult> NLIFaithfulnessVerifier::verifyBatch(
    const std::vector<std::pair<std::string, std::string>>& pairs
) {
    std::vector<NLIResult> results;
    results.reserve(pairs.size());
    
    // For now, process sequentially
    // TODO: Implement true batch processing with model
    for (const auto& [premise, hypothesis] : pairs) {
        results.push_back(verify(premise, hypothesis));
    }
    
    THEMIS_DEBUG("Verified {} pairs in batch", pairs.size());
    return results;
}

bool NLIFaithfulnessVerifier::isReady() const {
    return impl_->model_loaded || impl_->config.use_heuristic_fallback;
}

std::string NLIFaithfulnessVerifier::getModelInfo() const {
    std::ostringstream info;
    info << "Model type: " << impl_->config.model_type << ", ";
    info << "Path: " << (impl_->config.model_path.empty() ? "none" : impl_->config.model_path) << ", ";
    info << "Status: " << (impl_->model_loaded ? "loaded" : "not loaded") << ", ";
    info << "Cache hits/misses: " << impl_->cache_hits << "/" << impl_->cache_misses;
    return info.str();
}

void NLIFaithfulnessVerifier::warmup() {
    // Warm up with sample inputs
    std::vector<std::pair<std::string, std::string>> warmup_pairs = {
        {"The cat sat on the mat.", "A cat was sitting."},
        {"Paris is the capital of France.", "Paris is a city."},
        {"It is raining outside.", "The weather is dry."}
    };
    
    verifyBatch(warmup_pairs);
    THEMIS_INFO("NLI verifier warmed up");
}

void NLIFaithfulnessVerifier::clearCache() {
    impl_->cache.clear();
    impl_->cache_hits = 0;
    impl_->cache_misses = 0;
    THEMIS_INFO("NLI cache cleared");
}

// ═══════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════

SupportLevel nliPredictionToSupportLevel(NLIPrediction prediction, double confidence) {
    // Note: This assumes faithfulness_evaluator.h enum is available
    // Map NLI prediction to support level
    switch (prediction) {
        case NLIPrediction::ENTAILMENT:
            return confidence >= 0.7 ? SupportLevel::FULLY_SUPPORTED 
                                     : SupportLevel::PARTIALLY_SUPPORTED;
        
        case NLIPrediction::NEUTRAL:
            return confidence >= 0.7 ? SupportLevel::UNSUPPORTED
                                     : SupportLevel::PARTIALLY_SUPPORTED;
        
        case NLIPrediction::CONTRADICTION:
            return SupportLevel::CONTRADICTED;
        
        default:
            return SupportLevel::UNSUPPORTED;
    }
}

} // namespace themis::rag::judge

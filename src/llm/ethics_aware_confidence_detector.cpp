/**
 * @file ethics_aware_confidence_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=9, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/ethics_aware_confidence_detector.h"
#include <algorithm>
#include <cmath>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Implementation details
// ═══════════════════════════════════════════════════════════

struct EthicsAwareConfidenceDetector::Impl {
    EthicsAwareConfidenceConfig config;
    mutable Statistics stats;
    mutable std::mutex mutex;
    
    // Cache
    std::unordered_map<std::string, ConfidenceResult> cache;
    
    // Patronizing language patterns (multilingual)
    std::vector<std::string> patronizing_patterns_en = {
        "you must", "you should", "you have to", "you need to",
        "it is your duty", "you are obliged", "you are required",
        "obviously you", "clearly you must", "any reasonable person",
        "everyone knows", "it's obvious that you", "you can't possibly",
        "you obviously need", "of course you should"
    };
    
    std::vector<std::string> patronizing_patterns_de = {
        "sie müssen", "du musst", "sie sollten", "du solltest",
        "sie haben zu", "du hast zu", "es ist ihre pflicht",
        "es ist deine pflicht", "offensichtlich müssen sie",
        "natürlich müssen sie", "jeder vernünftige mensch",
        "es ist klar, dass sie", "sie können unmöglich"
    };
    
    // Imperative patterns
    std::vector<std::string> imperative_patterns_en = {
        "do this", "don't do", "you must do", "make sure you",
        "ensure that you", "it is imperative", "you are required to",
        "the right thing is", "the correct action is"
    };
    
    std::vector<std::string> imperative_patterns_de = {
        "tun sie das", "machen sie", "stellen sie sicher",
        "das richtige ist", "die korrekte handlung"
    };
    
    // Hedge words indicating uncertainty acknowledgment
    std::vector<std::string> hedge_words_en = {
        "may", "might", "could", "possibly", "perhaps", "probably",
        "likely", "it seems", "appears to", "suggests", "indicates",
        "in my understanding", "based on available", "uncertain",
        "not entirely clear", "to the best of", "it's possible",
        "one interpretation", "could be interpreted"
    };
    
    std::vector<std::string> hedge_words_de = {
        "könnte", "möglicherweise", "vielleicht", "wahrscheinlich",
        "vermutlich", "es scheint", "deutet darauf hin",
        "meines wissens", "soweit ich weiß", "nicht ganz klar",
        "es ist möglich", "eine interpretation"
    };
    
    // Choice preservation phrases
    std::vector<std::string> choice_phrases_en = {
        "you can decide", "your choice", "up to you", "you may choose",
        "consider", "options include", "alternatives are", "you might",
        "one option is", "another approach", "different perspectives",
        "various viewpoints", "multiple ways"
    };
    
    std::vector<std::string> choice_phrases_de = {
        "sie können entscheiden", "ihre wahl", "ihre entscheidung",
        "sie könnten", "optionen sind", "alternativen sind",
        "verschiedene perspektiven", "verschiedene sichtweisen"
    };
};

// ═══════════════════════════════════════════════════════════
// Configuration validation
// ═══════════════════════════════════════════════════════════

bool EthicsAwareConfidenceConfig::validateWeights() const {
    const float epsilon = 0.001f;
    float sum = technical_weight + autonomy_weight + transparency_weight;
    return std::abs(sum - 1.0f) < epsilon;
}

// ═══════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════

EthicsAwareConfidenceDetector::EthicsAwareConfidenceDetector(
    const EthicsAwareConfidenceConfig& config
) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Validate configuration
    if (!config.validateWeights()) {
        // Use default weights if invalid
        impl_->config.technical_weight = 0.40f;
        impl_->config.autonomy_weight = 0.35f;
        impl_->config.transparency_weight = 0.25f;
    }
}

EthicsAwareConfidenceDetector::~EthicsAwareConfidenceDetector() = default;

// ═══════════════════════════════════════════════════════════
// Core functionality
// ═══════════════════════════════════════════════════════════

ConfidenceResult EthicsAwareConfidenceDetector::detectConfidence(
    const std::string& text,
    const std::vector<TokenConfidence>& token_confidences
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats.total_detections++;
    
    // Check cache
    if (impl_->config.cache_results) {
        std::string cache_key = generateCacheKey(text);
        ConfidenceResult cached;
        if (getCachedResult(cache_key, cached)) {
            impl_->stats.cache_hits++;
            return cached;
        }
        impl_->stats.cache_misses++;
    }
    
    ConfidenceResult result;
    
    // Evaluate individual dimensions
    if (impl_->config.enable_entropy_analysis && !token_confidences.empty()) {
        result.technical_confidence = evaluateTechnicalConfidence(text, token_confidences);
        result.avg_token_entropy = calculateTokenEntropy(token_confidences);
        result.perplexity = calculatePerplexity(token_confidences);
    } else {
        // Default to moderate confidence without token data
        result.technical_confidence = 0.7f;
    }
    
    if (impl_->config.enable_patronizing_detection) {
        result.autonomy_respect_score = evaluateAutonomyRespect(text);
        result.patronizing_phrases = detectPatronizingLanguage(text);
        result.imperatives = detectImperatives(text);
        result.has_patronizing_language = !result.patronizing_phrases.empty();
        result.preserves_human_choice = checkChoicePreservation(text);
    } else {
        result.autonomy_respect_score = 1.0f;
        result.preserves_human_choice = true;
    }
    
    if (impl_->config.enable_uncertainty_detection) {
        result.transparency_score = evaluateTransparency(text);
        result.hedge_words = detectUncertaintyAcknowledgment(text);
        result.acknowledges_uncertainty = !result.hedge_words.empty();
    } else {
        result.transparency_score = 0.8f;
    }
    
    // Combine scores
    result.combined_confidence = combineScores(
        result.technical_confidence,
        result.autonomy_respect_score,
        result.transparency_score
    );
    
    // Quality assessment
    result.meets_quality_threshold = 
        result.combined_confidence >= impl_->config.min_technical_confidence &&
        result.autonomy_respect_score >= impl_->config.min_autonomy_respect &&
        result.transparency_score >= impl_->config.min_transparency;
    
    // Generate reasoning
    result.reasoning = generateReasoning(result);
    
    // Update statistics
    if (result.has_patronizing_language) {
        impl_->stats.patronizing_detected++;
    }
    if (result.combined_confidence < impl_->config.min_technical_confidence) {
        impl_->stats.low_confidence_detected++;
    }
    
    impl_->stats.avg_technical_confidence = 
        (impl_->stats.avg_technical_confidence * (impl_->stats.total_detections - 1) + 
         result.technical_confidence) / impl_->stats.total_detections;
    impl_->stats.avg_autonomy_respect = 
        (impl_->stats.avg_autonomy_respect * (impl_->stats.total_detections - 1) + 
         result.autonomy_respect_score) / impl_->stats.total_detections;
    impl_->stats.avg_transparency = 
        (impl_->stats.avg_transparency * (impl_->stats.total_detections - 1) + 
         result.transparency_score) / impl_->stats.total_detections;
    impl_->stats.avg_combined_confidence = 
        (impl_->stats.avg_combined_confidence * (impl_->stats.total_detections - 1) + 
         result.combined_confidence) / impl_->stats.total_detections;
    
    // Cache result
    if (impl_->config.cache_results) {
        std::string cache_key = generateCacheKey(text);
        cacheResult(cache_key, result);
    }
    
    return result;
}

ConfidenceResult EthicsAwareConfidenceDetector::detectConfidenceWithContext(
    const std::string& text,
    const std::string& query,
    const std::vector<std::string>& /*context*/,
    const std::vector<TokenConfidence>& token_confidences
) {
    // Start with basic detection
    ConfidenceResult result = detectConfidence(text, token_confidences);
    
    // Adjust autonomy respect based on query context
    if (!query.empty()) {
        float context_adjusted_autonomy = evaluateAutonomyRespect(text, query);
        // Weight: 70% context-aware, 30% standalone
        result.autonomy_respect_score = 
            0.7f * context_adjusted_autonomy + 0.3f * result.autonomy_respect_score;
    }
    
    // Recombine scores
    result.combined_confidence = combineScores(
        result.technical_confidence,
        result.autonomy_respect_score,
        result.transparency_score
    );
    
    // Regenerate reasoning
    result.reasoning = generateReasoning(result);
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// Individual dimension evaluation
// ═══════════════════════════════════════════════════════════

float EthicsAwareConfidenceDetector::evaluateTechnicalConfidence(
    const std::string& /*text*/,
    const std::vector<TokenConfidence>& token_confidences
) {
    if (token_confidences.empty()) {
        return 0.7f; // Default moderate confidence
    }
    
    // Calculate average token probability
    float avg_prob = 0.0f;
    for (const auto& tc : token_confidences) {
        avg_prob += tc.probability;
    }
    avg_prob /= token_confidences.size();
    
    // Calculate entropy-based confidence
    float entropy = calculateTokenEntropy(token_confidences);
    float max_entropy = std::log2(10000.0f); // Assume vocab size ~10k
    float normalized_entropy = 1.0f - (entropy / max_entropy);
    
    // Combine probability and entropy (50-50 weight)
    return 0.5f * avg_prob + 0.5f * normalized_entropy;
}

float EthicsAwareConfidenceDetector::evaluateAutonomyRespect(
    const std::string& text,
    const std::string& /*query*/
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Detect patronizing language
    auto patronizing = detectPatronizingLanguage(text);
    auto imperatives = detectImperatives(text);
    
    // Count total violations
    const auto violations = static_cast<int>(patronizing.size() + imperatives.size());
    
    // Check if human choice is preserved
    bool preserves_choice = checkChoicePreservation(text);
    
    // Calculate score
    // Start at 1.0, deduct for violations
    float score = 1.0f;
    
    // Each patronizing phrase reduces score by 0.1
    score -= violations * 0.1f;
    
    // Not preserving choice is a major violation
    if (!preserves_choice && violations > 0) {
        score -= 0.2f;
    }
    
    // Clamp to [0, 1]
    return std::max(0.0f, std::min(1.0f, score));
}

float EthicsAwareConfidenceDetector::evaluateTransparency(const std::string& text) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Detect uncertainty acknowledgment
    auto hedge_words = detectUncertaintyAcknowledgment(text);
    
    // Check for explicit limitation statements
    std::string text_lower = toLowerCase(text);
    
    std::vector<std::string> limitation_phrases = {
        "i don't know", "i'm not sure", "i cannot", "i'm uncertain",
        "limited information", "may not be accurate", "to the best of",
        "consult", "expert", "professional", "specialist",
        "ich weiß nicht", "ich bin nicht sicher", "begrenzte information",
        "möglicherweise nicht genau", "konsultieren sie", "fachmann"
    };
    
    int limitation_count = countPatternMatches(text_lower, limitation_phrases);
    
    // Calculate score
    float score = 0.5f; // Base score
    
    // Add for hedge words (up to 0.3)
    score += std::min(0.3f, static_cast<float>(hedge_words.size()) * 0.05f);
    
    // Add for limitation acknowledgments (up to 0.2)
    score += std::min(0.2f, limitation_count * 0.1f);
    
    return std::min(1.0f, score);
}

// ═══════════════════════════════════════════════════════════
// Pattern detection
// ═══════════════════════════════════════════════════════════

std::vector<std::string> EthicsAwareConfidenceDetector::detectPatronizingLanguage(
    const std::string& text
) {
    std::vector<std::string> detected;
    std::string text_lower = toLowerCase(text);
    
    // Check English patterns
    for (const auto& pattern : impl_->patronizing_patterns_en) {
        if (text_lower.find(pattern) != std::string::npos) {
            detected.push_back(pattern);
        }
    }
    
    // Check German patterns
    for (const auto& pattern : impl_->patronizing_patterns_de) {
        if (text_lower.find(pattern) != std::string::npos) {
            detected.push_back(pattern);
        }
    }
    
    return detected;
}

std::vector<std::string> EthicsAwareConfidenceDetector::detectImperatives(
    const std::string& text
) {
    std::vector<std::string> detected;
    std::string text_lower = toLowerCase(text);
    
    // Check English patterns
    for (const auto& pattern : impl_->imperative_patterns_en) {
        if (text_lower.find(pattern) != std::string::npos) {
            detected.push_back(pattern);
        }
    }
    
    // Check German patterns
    for (const auto& pattern : impl_->imperative_patterns_de) {
        if (text_lower.find(pattern) != std::string::npos) {
            detected.push_back(pattern);
        }
    }
    
    return detected;
}

std::vector<std::string> EthicsAwareConfidenceDetector::detectUncertaintyAcknowledgment(
    const std::string& text
) {
    std::vector<std::string> detected;
    std::string text_lower = toLowerCase(text);
    
    // Check English hedge words
    for (const auto& word : impl_->hedge_words_en) {
        if (text_lower.find(word) != std::string::npos) {
            detected.push_back(word);
        }
    }
    
    // Check German hedge words
    for (const auto& word : impl_->hedge_words_de) {
        if (text_lower.find(word) != std::string::npos) {
            detected.push_back(word);
        }
    }
    
    // Remove duplicates
    std::unordered_set<std::string> unique(detected.begin(), detected.end());
    detected.assign(unique.begin(), unique.end());
    
    return detected;
}

bool EthicsAwareConfidenceDetector::checkChoicePreservation(const std::string& text) {
    std::string text_lower = toLowerCase(text);
    
    // Check for choice-preserving phrases
    for (const auto& phrase : impl_->choice_phrases_en) {
        if (text_lower.find(phrase) != std::string::npos) {
            return true;
        }
    }
    
    for (const auto& phrase : impl_->choice_phrases_de) {
        if (text_lower.find(phrase) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════

void EthicsAwareConfidenceDetector::setConfig(const EthicsAwareConfidenceConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
    
    if (!config.validateWeights()) {
        // Reset to default weights
        impl_->config.technical_weight = 0.40f;
        impl_->config.autonomy_weight = 0.35f;
        impl_->config.transparency_weight = 0.25f;
    }
}

EthicsAwareConfidenceConfig EthicsAwareConfidenceDetector::getConfig() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

void EthicsAwareConfidenceDetector::clearCache() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cache.clear();
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

EthicsAwareConfidenceDetector::Statistics EthicsAwareConfidenceDetector::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats;
}

void EthicsAwareConfidenceDetector::resetStatistics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats = Statistics();
}

// ═══════════════════════════════════════════════════════════
// Helper methods
// ═══════════════════════════════════════════════════════════

bool EthicsAwareConfidenceDetector::containsPattern(
    const std::string& text,
    const std::vector<std::string>& patterns
) {
    std::string text_lower = toLowerCase(text);
    for (const auto& pattern : patterns) {
        if (text_lower.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int EthicsAwareConfidenceDetector::countPatternMatches(
    const std::string& text,
    const std::vector<std::string>& patterns
) {
    int count = 0;
    std::string text_lower = toLowerCase(text);
    for (const auto& pattern : patterns) {
        if (text_lower.find(pattern) != std::string::npos) {
            count++;
        }
    }
    return count;
}

std::string EthicsAwareConfidenceDetector::toLowerCase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

float EthicsAwareConfidenceDetector::calculateTokenEntropy(
    const std::vector<TokenConfidence>& tokens
) {
    if (tokens.empty()) return 0.0f;
    
    float entropy = 0.0f;
    for (const auto& token : tokens) {
        if (token.probability > 0.0f) {
            entropy += token.probability * std::log2(token.probability);
        }
    }
    return -entropy / tokens.size();
}

float EthicsAwareConfidenceDetector::calculatePerplexity(
    const std::vector<TokenConfidence>& tokens
) {
    if (tokens.empty()) return 1.0f;
    
    float log_prob_sum = 0.0f;
    for (const auto& token : tokens) {
        if (token.probability > 0.0f) {
            log_prob_sum += std::log2(token.probability);
        }
    }
    
    return std::pow(2.0f, -log_prob_sum / tokens.size());
}

float EthicsAwareConfidenceDetector::combineScores(
    float technical,
    float autonomy,
    float transparency
) {
    return impl_->config.technical_weight * technical +
           impl_->config.autonomy_weight * autonomy +
           impl_->config.transparency_weight * transparency;
}

std::string EthicsAwareConfidenceDetector::generateReasoning(const ConfidenceResult& result) {
    std::ostringstream oss;
    
    oss << "Confidence Analysis: ";
    
    if (result.combined_confidence >= 0.8f) {
        oss << "High confidence. ";
    } else if (result.combined_confidence >= 0.6f) {
        oss << "Moderate confidence. ";
    } else {
        oss << "Low confidence. ";
    }
    
    if (result.has_patronizing_language) {
        oss << "Detected patronizing language (" << result.patronizing_phrases.size() 
            << " instances). ";
    }
    
    if (!result.preserves_human_choice) {
        oss << "Does not adequately preserve human choice. ";
    }
    
    if (result.acknowledges_uncertainty) {
        oss << "Appropriately acknowledges uncertainty. ";
    }
    
    if (!result.meets_quality_threshold) {
        oss << "Does not meet minimum quality thresholds. ";
    }
    
    return oss.str();
}

std::string EthicsAwareConfidenceDetector::generateCacheKey(const std::string& text) {
    // Simple hash-based cache key
    std::hash<std::string> hasher;
    return std::to_string(hasher(text));
}

bool EthicsAwareConfidenceDetector::getCachedResult(
    const std::string& key,
    ConfidenceResult& result
) {
    auto it = impl_->cache.find(key);
    if (it != impl_->cache.end()) {
        result = it->second;
        return true;
    }
    return false;
}

void EthicsAwareConfidenceDetector::cacheResult(
    const std::string& key,
    const ConfidenceResult& result
) {
    // Simple cache size management
    if (impl_->cache.size() >= impl_->config.max_cache_size) {
        // Remove oldest entry (simplified - could use LRU)
        impl_->cache.erase(impl_->cache.begin());
    }
    impl_->cache[key] = result;
}

// ═══════════════════════════════════════════════════════════
// Factory methods
// ═══════════════════════════════════════════════════════════

std::unique_ptr<EthicsAwareConfidenceDetector> ConfidenceDetectorFactory::createDefault() {
    return std::make_unique<EthicsAwareConfidenceDetector>();
}

std::unique_ptr<EthicsAwareConfidenceDetector> ConfidenceDetectorFactory::createStrict() {
    EthicsAwareConfidenceConfig config;
    config.min_autonomy_respect = 0.85f;
    config.min_transparency = 0.75f;
    config.min_technical_confidence = 0.70f;
    return std::make_unique<EthicsAwareConfidenceDetector>(config);
}

std::unique_ptr<EthicsAwareConfidenceDetector> ConfidenceDetectorFactory::createLenient() {
    EthicsAwareConfidenceConfig config;
    config.min_autonomy_respect = 0.60f;
    config.min_transparency = 0.50f;
    config.min_technical_confidence = 0.40f;
    return std::make_unique<EthicsAwareConfidenceDetector>(config);
}

std::unique_ptr<EthicsAwareConfidenceDetector> ConfidenceDetectorFactory::create(
    const EthicsAwareConfidenceConfig& config
) {
    return std::make_unique<EthicsAwareConfidenceDetector>(config);
}

} // namespace llm
} // namespace themis

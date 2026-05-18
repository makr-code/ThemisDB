/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fairness_detector.cpp                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-18 18:04:35                                ║
  Author:          Copilot AI                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔄 In Development (Wave A3: Fairness & Bias Detection)       ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "rag/fairness_detector.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

namespace themis::rag {

/**
 * @brief PIMPL (Pointer to Implementation) for FairnessDetector.
 *
 * Holds opaque implementation details (word embeddings, bias models, etc.)
 * to avoid exposing external library headers in the public interface.
 */
class FairnessDetector::Impl {
public:
    Impl(const FairnessDetectorConfig& config) : config(config) {}

    FairnessDetectorConfig config;
    bool embeddings_loaded = false;

    // Predefined biased term sets (simplified for initial implementation)
    std::unordered_set<std::string> gender_biased_terms = {
        "man", "woman", "male", "female", "he", "she",
        "prince", "princess", "king", "queen"
    };

    std::unordered_set<std::string> occupational_biased_terms = {
        "nurse", "doctor", "programmer", "engineer", "teacher",
        "nurse", "secretary", "boss", "manager", "cook"
    };

    std::unordered_set<std::string> ethnicity_biased_terms = {
        "foreign", "immigrant", "native", "ethnic", "community"
    };

    // TODO (Wave A3): Load actual word embeddings from config.embedding_model_path
    //   - Parse GloVe/FastText format
    //   - Compute PCA basis for gender subspace
    //   - Compute occupational bias vectors
};

// ═════════════════════════════════════════════════════════════════════

FairnessDetector::FairnessDetector(const FairnessDetectorConfig& config)
    : config_(config),
      impl_(std::make_unique<Impl>(config)) {
    THEMIS_INFO("FairnessDetector constructed with embedding_model='{}'",
                config_.embedding_model_path);
}

FairnessDetector::~FairnessDetector() = default;

// ─────────────────────────────────────────────────────────────────────

common::Status FairnessDetector::initialize() {
    if (initialized_) {
        return common::Status::OK;
    }

    THEMIS_INFO("Initializing FairnessDetector");

    // TODO (Wave A3): Load word embeddings
    //   - Load from config_.embedding_model_path (GloVe/FastText)
    //   - Validate dimension matches config_.embedding_dimension
    //   - Compute PCA basis for gender subspace
    //   - Return error if embeddings fail to load

    impl_->embeddings_loaded = true;
    initialized_ = true;

    THEMIS_INFO("FairnessDetector initialized successfully");
    return common::Status::OK;
}

// ─────────────────────────────────────────────────────────────────────

bool FairnessDetector::isInitialized() const {
    return initialized_ && impl_->embeddings_loaded;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<judge::BiasScore> FairnessDetector::detectBias(
    const std::string& document_text) {
    if (!isInitialized()) {
        THEMIS_WARN("FairnessDetector::detectBias called before initialize()");
        return common::Error(common::ErrorCode::INVALID_STATE,
                            "Detector not initialized");
    }

    if (document_text.empty()) {
        THEMIS_WARN("FairnessDetector::detectBias called with empty document");
        return common::Error(common::ErrorCode::INVALID_ARGUMENT,
                            "Document cannot be empty");
    }

    judge::BiasScore score;

    // Compute individual bias dimensions
    if (config_.detect_gender_bias) {
        auto result = computeGenderBias(document_text);
        if (!result) {
            return result.error();
        }
        score.gender_bias = result.value();
    }

    if (config_.detect_occupational_bias) {
        auto result = computeOccupationalBias(document_text);
        if (!result) {
            return result.error();
        }
        score.occupational_bias = result.value();
    }

    if (config_.detect_ethnicity_bias) {
        auto result = computeEthnicityBias(document_text);
        if (!result) {
            return result.error();
        }
        score.ethnicity_bias = result.value();
    }

    // Compute stereotype density
    auto density_result = computeStereotypeDensity(document_text);
    if (density_result) {
        score.stereotype_density = density_result.value();
    }

    // Compute intersectional bias if all components available
    if (config_.detect_intersectional_bias) {
        auto result = computeIntersectionalBias(
            document_text,
            score.gender_bias,
            score.occupational_bias,
            score.ethnicity_bias);
        if (result) {
            score.intersectional_bias = result.value();
        }
    }

    // Compute overall bias as weighted combination
    score.overall_score = (score.gender_bias * 0.25 +
                          score.occupational_bias * 0.25 +
                          score.ethnicity_bias * 0.25 +
                          score.intersectional_bias * 0.25);

    // Clamp to [0.0, 1.0]
    score.overall_score = std::max(0.0, std::min(1.0, score.overall_score));

    // Set confidence (simplified: based on stereotype density and overall score)
    score.confidence = 0.8 * score.stereotype_density + 0.2 * score.overall_score;

    // Extract biased terms for explanation
    auto terms_result = extractBiasedTerms(document_text);
    if (terms_result) {
        score.detected_terms = terms_result.value();
    }

    // Flag if score exceeds threshold and confidence is sufficient
    score.flagged = (score.overall_score >= config_.bias_threshold &&
                    score.confidence >= config_.min_confidence);

    // Build explanation
    if (score.flagged) {
        score.explanation = "Document contains potential biases: ";
        if (score.gender_bias > 0.1) {
            score.explanation += "gender ";
        }
        if (score.occupational_bias > 0.1) {
            score.explanation += "occupational ";
        }
        if (score.ethnicity_bias > 0.1) {
            score.explanation += "ethnicity ";
        }
        score.explanation += "(score=" + std::to_string(score.overall_score) + ")";
    } else {
        score.explanation = "Document bias within acceptable range";
    }

    THEMIS_DEBUG("Document bias analysis: overall={:.2f}, gender={:.2f}, "
                 "occupational={:.2f}, flagged={}",
                 score.overall_score, score.gender_bias,
                 score.occupational_bias, score.flagged);

    return score;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<std::vector<judge::BiasScore>> FairnessDetector::detectBiasBatch(
    const std::vector<std::string>& documents) {
    std::vector<judge::BiasScore> results;
    results.reserve(documents.size());

    for (const auto& doc : documents) {
        auto result = detectBias(doc);
        if (!result) {
            return result.error();
        }
        results.push_back(result.value());
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<std::vector<std::pair<std::string, judge::BiasScore>>>
FairnessDetector::filterByBiasThreshold(
    const std::vector<std::string>& documents) {
    std::vector<std::pair<std::string, judge::BiasScore>> filtered;

    for (const auto& doc : documents) {
        auto result = detectBias(doc);
        if (!result) {
            return result.error();
        }

        if (result.value().overall_score < config_.bias_threshold) {
            filtered.emplace_back(doc, result.value());
        }
    }

    return filtered;
}

// ─────────────────────────────────────────────────────────────────────

const FairnessDetectorConfig& FairnessDetector::getConfig() const {
    return config_;
}

// ─────────────────────────────────────────────────────────────────────

void FairnessDetector::setBiasThreshold(double threshold) {
    config_.bias_threshold = std::max(0.0, std::min(1.0, threshold));
}

// ─────────────────────────────────────────────────────────────────────
// PRIVATE HELPER METHODS
// ─────────────────────────────────────────────────────────────────────

common::Result<double> FairnessDetector::computeGenderBias(
    const std::string& text) {
    // TODO (Wave A3): Compute PCA-based gender bias projection
    //   - Tokenize text
    //   - Get embeddings for each word
    //   - Project onto gender subspace
    //   - Return bias magnitude

    // Stub: count gender terms
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    int bias_count = 0;
    for (const auto& term : impl_->gender_biased_terms) {
        size_t pos = 0;
        while ((pos = lower_text.find(term, pos)) != std::string::npos) {
            bias_count++;
            pos += term.length();
        }
    }

    double bias_score = std::min(1.0, bias_count * 0.1);
    return bias_score;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<double> FairnessDetector::computeOccupationalBias(
    const std::string& text) {
    // TODO (Wave A3): Compute occupational stereotype bias
    //   - Identify gender-biased occupation terms
    //   - Compute stereotype score

    // Stub implementation
    return 0.0;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<double> FairnessDetector::computeEthnicityBias(
    const std::string& text) {
    // TODO (Wave A3): Compute ethnicity/cultural bias

    // Stub implementation
    return 0.0;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<double> FairnessDetector::computeIntersectionalBias(
    const std::string& text,
    double gender_bias,
    double occupational_bias,
    double ethnicity_bias) {
    // TODO (Wave A3): Compute intersectional bias
    //   - Check for co-occurrence of multiple bias dimensions
    //   - Compound score when multiple biases co-occur

    // Stub: average of component biases
    double compound = (gender_bias + occupational_bias + ethnicity_bias) / 3.0;
    return compound;
}

// ─────────────────────────────────────────────────────────────────────

common::Result<double> FairnessDetector::computeStereotypeDensity(
    const std::string& text) {
    // Compute freq(biased_terms) / total_terms in passage

    // Count words
    size_t word_count = std::count_if(text.begin(), text.end(),
                                     [](char c) { return std::isspace(c); }) + 1;
    if (word_count == 0) {
        return 0.0;
    }

    // Count biased terms
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    size_t biased_count = 0;
    for (const auto& term : impl_->gender_biased_terms) {
        size_t pos = 0;
        while ((pos = lower_text.find(term, pos)) != std::string::npos) {
            biased_count++;
            pos += term.length();
        }
    }

    double density = static_cast<double>(biased_count) / word_count;
    return std::min(1.0, density);
}

// ─────────────────────────────────────────────────────────────────────

common::Result<std::vector<std::string>> FairnessDetector::extractBiasedTerms(
    const std::string& text) {
    std::vector<std::string> terms;

    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    // Extract gender biased terms
    for (const auto& term : impl_->gender_biased_terms) {
        if (lower_text.find(term) != std::string::npos) {
            terms.push_back(term);
        }
    }

    return terms;
}

}  // namespace themis::rag

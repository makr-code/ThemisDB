/**
 * @file fairness_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/fairness_detector.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <numeric>
#include <mutex>

namespace themis::rag {

/**
 * @brief PIMPL (Pointer to Implementation) for FairnessDetector.
 *
 * Holds opaque implementation details (word embeddings, bias models, etc.)
 * to avoid exposing external library headers in the public interface.
 * 
 * Phase 2 Implementation:
 *  - Real word embedding loading (GloVe/FastText format)
 *  - PCA bias projection computation
 *  - Stereotype term dictionaries
 */
class FairnessDetector::Impl {
public:
    Impl(const FairnessDetectorConfig& config) : config(config) {}

    FairnessDetectorConfig config;
    bool embeddings_loaded = false;
    mutable std::mutex metrics_mutex;  // Protect embeddings and bias vectors
    
    // Word embeddings: word -> embedding vector
    std::unordered_map<std::string, std::vector<float>> embeddings;
    
    // PCA bias vectors (gender, occupational, ethnicity)
    std::vector<float> gender_bias_vector;
    std::vector<float> occupational_bias_vector;
    std::vector<float> ethnicity_bias_vector;

    // Predefined biased term sets (comprehensive for initial implementation)
    std::unordered_set<std::string> gender_biased_terms = {
        "man", "woman", "male", "female", "he", "she", "his", "her",
        "prince", "princess", "king", "queen", "actor", "actress",
        "waiter", "waitress", "steward", "stewardess", "boy", "girl"
    };

    std::unordered_set<std::string> occupational_biased_terms = {
        "nurse", "doctor", "programmer", "engineer", "teacher",
        "secretary", "boss", "manager", "cook", "chef", "pilot",
        "scientist", "mathematician", "lawyer", "judge", "police",
        "soldier", "firefighter", "construction", "mechanic"
    };

    std::unordered_set<std::string> ethnicity_biased_terms = {
        "foreign", "immigrant", "native", "ethnic", "community",
        "minority", "majority", "indigenous", "aboriginal", "tribal",
        "oriental", "occidental", "western", "eastern", "asian"
    };

    /**
     * @brief Load word embeddings from GloVe/FastText format
     * 
     * Expected format:
     *   word dim1 dim2 dim3 ...
     *   word2 dim1 dim2 dim3 ...
     */
    bool loadEmbeddings(const std::string& embedding_path) {
        std::ifstream file(embedding_path);
        if (!file.is_open()) {
            THEMIS_WARN("Could not open embedding file: {}", embedding_path);
            return false;
        }
        
        std::string line;
        size_t line_count = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) {
              continue;
            }
            
            std::istringstream iss(line);
            std::string word;
            iss >> word;
            
            std::vector<float> embedding;
            float value;
            while (iss >> value) {
                embedding.push_back(value);
            }
            
            if (!embedding.empty()) {
                embeddings[word] = embedding;
                line_count++;
            }
            
            // Limit to first 100k embeddings for memory efficiency
            if (line_count >= 100000) {
              break;
            }
        }
        
        file.close();
        THEMIS_INFO("Loaded {} word embeddings from {}", line_count, embedding_path);
        return line_count > 0;
    }
    
    /**
     * @brief Compute PCA bias vector from gender word pairs
     * 
     * Based on Bolukbasi et al. method:
     *   1. Select gender word pairs (man-woman, prince-princess, etc.)
     *   2. Compute difference vectors for each pair
     *   3. Average the difference vectors to get gender bias direction
     */
    void computeGenderBiasVector() {
        // Gender word pairs for PCA computation
        std::vector<std::pair<std::string, std::string>> gender_pairs = {
            {"man", "woman"},
            {"prince", "princess"},
            {"king", "queen"},
            {"actor", "actress"},
            {"waiter", "waitress"},
            {"he", "she"},
            {"his", "her"},
            {"boy", "girl"}
        };
        
        std::vector<std::vector<float>> difference_vectors;
        
        for (const auto& [male_word, female_word] : gender_pairs) {
            auto male_it = embeddings.find(male_word);
            auto female_it = embeddings.find(female_word);
            
            if (male_it != embeddings.end() && female_it != embeddings.end()) {
                const auto& male_emb = male_it->second;
                const auto& female_emb = female_it->second;
                
                if (male_emb.size() == female_emb.size()) {
                    std::vector<float> diff(male_emb.size());
                    for (size_t i = 0; i < male_emb.size(); ++i) {
                        diff[i] = male_emb[i] - female_emb[i];
                    }
                    difference_vectors.push_back(diff);
                }
            }
        }
        
        // Average the difference vectors
        if (!difference_vectors.empty()) {
            gender_bias_vector.resize(difference_vectors[0].size(), 0.0f);
            for (const auto& diff : difference_vectors) {
                for (size_t i = 0; i < diff.size(); ++i) {
                    gender_bias_vector[i] += diff[i];
                }
            }
            for (float& val : gender_bias_vector) {
                val /= static_cast<float>(difference_vectors.size());
            }
            
            THEMIS_INFO("Computed gender bias vector from {} word pairs", 
                        difference_vectors.size());
        }
    }
    
    /**
     * @brief Compute occupational bias vector
     */
    void computeOccupationalBiasVector() {
        // Occupational word pairs
        std::vector<std::pair<std::string, std::string>> occupational_pairs = {
            {"nurse", "doctor"},
            {"secretary", "boss"},
            {"teacher", "professor"},
            {"cook", "chef"},
            {"pilot", "captain"}
        };
        
        std::vector<std::vector<float>> difference_vectors;
        
        for (const auto& [low_status, high_status] : occupational_pairs) {
            auto low_it = embeddings.find(low_status);
            auto high_it = embeddings.find(high_status);
            
            if (low_it != embeddings.end() && high_it != embeddings.end()) {
                const auto& low_emb = low_it->second;
                const auto& high_emb = high_it->second;
                
                if (low_emb.size() == high_emb.size()) {
                    std::vector<float> diff(low_emb.size());
                    for (size_t i = 0; i < low_emb.size(); ++i) {
                        diff[i] = low_emb[i] - high_emb[i];
                    }
                    difference_vectors.push_back(diff);
                }
            }
        }
        
        if (!difference_vectors.empty()) {
            occupational_bias_vector.resize(difference_vectors[0].size(), 0.0f);
            for (const auto& diff : difference_vectors) {
                for (size_t i = 0; i < diff.size(); ++i) {
                    occupational_bias_vector[i] += diff[i];
                }
            }
            for (float& val : occupational_bias_vector) {
                val /= static_cast<float>(difference_vectors.size());
            }
            
            THEMIS_INFO("Computed occupational bias vector from {} word pairs", 
                        difference_vectors.size());
        }
    }

    /**
     * @brief Compute ethnicity bias vector from contrastive word pairs.
     */
    void computeEthnicityBiasVector() {
        std::vector<std::pair<std::string, std::string>> ethnicity_pairs = {
            {"majority", "minority"},
            {"native", "immigrant"},
            {"western", "eastern"},
            {"indigenous", "foreign"},
            {"urban", "tribal"}
        };

        std::vector<std::vector<float>> difference_vectors;
        for (const auto& [left_word, right_word] : ethnicity_pairs) {
            auto left_it = embeddings.find(left_word);
            auto right_it = embeddings.find(right_word);
            if (left_it == embeddings.end() || right_it == embeddings.end()) {
                continue;
            }
            const auto& lhs = left_it->second;
            const auto& rhs = right_it->second;
            if (lhs.size() != rhs.size()) {
                continue;
            }
            std::vector<float> diff(lhs.size(), 0.0f);
            for (size_t i = 0; i < lhs.size(); ++i) {
                diff[i] = lhs[i] - rhs[i];
            }
            difference_vectors.push_back(std::move(diff));
        }

        if (!difference_vectors.empty()) {
            ethnicity_bias_vector.assign(difference_vectors[0].size(), 0.0f);
            for (const auto& diff : difference_vectors) {
                for (size_t i = 0; i < diff.size(); ++i) {
                    ethnicity_bias_vector[i] += diff[i];
                }
            }
            for (float& val : ethnicity_bias_vector) {
                val /= static_cast<float>(difference_vectors.size());
            }
            THEMIS_INFO("Computed ethnicity bias vector from {} word pairs",
                        difference_vectors.size());
        }
    }
    
    /**
     * @brief Compute bias score for a word using PCA projection
     */
    double computeWordBiasScore(const std::string& word, 
                                const std::vector<float>& bias_vector) {
        auto it = embeddings.find(word);
        if (it == embeddings.end() || bias_vector.empty()) {
            return 0.0;
        }
        
        const auto& word_emb = it->second;
        if (word_emb.size() != bias_vector.size()) {
            return 0.0;
        }
        
        // Compute dot product (projection onto bias vector)
        double projection = 0.0;
        for (size_t i = 0; i < word_emb.size(); ++i) {
            projection += static_cast<double>(word_emb[i]) *
                          static_cast<double>(bias_vector[i]);
        }
        
        // Normalize to [0, 1] range
        return std::abs(projection) / (1.0 + std::abs(projection));
    }
    
    /**
     * @brief Convert text to lowercase
     */
    static std::string toLower(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
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

void FairnessDetector::initialize() {
    if (initialized_) {
        return;
    }

    THEMIS_INFO("Initializing FairnessDetector");

    // Phase 2: Load word embeddings and compute PCA bias vectors
    try {
        // Load embeddings if path provided
        if (!config_.embedding_model_path.empty()) {
            bool loaded = impl_->loadEmbeddings(config_.embedding_model_path);
            if (!loaded) {
                THEMIS_WARN("Failed to load embeddings from {}, using term-counting fallback",
                           config_.embedding_model_path);
            } else {
                impl_->embeddings_loaded = true;
                
                // Compute PCA bias vectors
                if (config_.detect_gender_bias) {
                    impl_->computeGenderBiasVector();
                }
                if (config_.detect_occupational_bias) {
                    impl_->computeOccupationalBiasVector();
                }
                if (config_.detect_ethnicity_bias) {
                    impl_->computeEthnicityBiasVector();
                }
            }
        }
        
        THEMIS_INFO("FairnessDetector initialized successfully");
        initialized_ = true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("FairnessDetector initialization failed: {}", e.what());
        throw;
    }
}

// ─────────────────────────────────────────────────────────────────────

bool FairnessDetector::isInitialized() const {
    return initialized_;
}

// ─────────────────────────────────────────────────────────────────────

judge::BiasScore FairnessDetector::detectBias(const std::string& document) {
    if (!isInitialized()) {
        THEMIS_WARN("FairnessDetector::detectBias called before initialize()");
        throw std::runtime_error("FairnessDetector not initialized");
    }

    if (document.empty()) {
        THEMIS_WARN("FairnessDetector::detectBias called with empty document");
        throw std::invalid_argument("Document cannot be empty");
    }

    judge::BiasScore score;
    
    try {
        // Tokenize document into words
        std::vector<std::string> words;
        std::string word;
        for (char c : document) {
            if (std::isalnum(c)) {
                word += c;
            } else {
                if (!word.empty()) {
                    words.push_back(Impl::toLower(word));
                    word.clear();
                }
            }
        }
        if (!word.empty()) {
            words.push_back(Impl::toLower(word));
        }
        
        if (words.empty()) {
            return score;
        }
        
        // Count biased terms and compute bias scores
        size_t gender_biased_count = 0;
        size_t occupational_biased_count = 0;
        size_t ethnicity_biased_count = 0;
        double gender_bias_sum = 0.0;
        double occupational_bias_sum = 0.0;
        double ethnicity_bias_sum = 0.0;
        
        for (const auto& w : words) {
            // Gender bias detection
            if (config_.detect_gender_bias) {
                if (impl_->gender_biased_terms.count(w) > 0) {
                    gender_biased_count++;
                    if (impl_->embeddings_loaded && !impl_->gender_bias_vector.empty()) {
                        gender_bias_sum += impl_->computeWordBiasScore(w, impl_->gender_bias_vector);
                    } else {
                        gender_bias_sum += 0.5;  // Default score for term-counting
                    }
                }
            }
            
            // Occupational bias detection
            if (config_.detect_occupational_bias) {
                if (impl_->occupational_biased_terms.count(w) > 0) {
                    occupational_biased_count++;
                    if (impl_->embeddings_loaded && !impl_->occupational_bias_vector.empty()) {
                        occupational_bias_sum += impl_->computeWordBiasScore(w, impl_->occupational_bias_vector);
                    } else {
                        occupational_bias_sum += 0.5;
                    }
                }
            }
            
            // Ethnicity bias detection
            if (config_.detect_ethnicity_bias) {
                if (impl_->ethnicity_biased_terms.count(w) > 0) {
                    ethnicity_biased_count++;
                    if (impl_->embeddings_loaded && !impl_->ethnicity_bias_vector.empty()) {
                        ethnicity_bias_sum += impl_->computeWordBiasScore(w, impl_->ethnicity_bias_vector);
                    } else {
                        ethnicity_bias_sum += 0.5;
                    }
                }
            }
        }
        
        // Compute average bias scores
        if (gender_biased_count > 0) {
            score.gender_bias = gender_bias_sum / gender_biased_count;
        }
        if (occupational_biased_count > 0) {
            score.occupational_bias = occupational_bias_sum / occupational_biased_count;
        }
        if (ethnicity_biased_count > 0) {
            score.ethnicity_bias = ethnicity_bias_sum / ethnicity_biased_count;
        }
        
        // Compute stereotype density
        size_t total_biased = gender_biased_count + occupational_biased_count + ethnicity_biased_count;
        score.stereotype_density = static_cast<double>(total_biased) / words.size();
        
        // Compute overall score (weighted average)
        score.overall_score = (score.gender_bias * 0.4 + 
                               score.occupational_bias * 0.35 + 
                               score.ethnicity_bias * 0.25);
        
        // Intersectional bias (compound gender × ethnicity, plus occupational contribution)
        if (config_.detect_intersectional_bias) {
            const double gx = score.gender_bias * score.ethnicity_bias;
            const double go = score.gender_bias * score.occupational_bias;
            score.overall_score = std::clamp(score.overall_score + 0.15 * (gx + go), 0.0, 1.0);
        }

        // Confidence increases with observed evidence density.
        const double evidence = static_cast<double>(total_biased);
        score.confidence = std::clamp(
            0.35 + 0.65 * (evidence / (evidence + 6.0)),
            config_.min_confidence,
            1.0);
        score.flagged = score.overall_score >= config_.bias_threshold &&
                        score.confidence >= config_.min_confidence;
        
        THEMIS_DEBUG("Bias detection: overall={:.3f}, gender={:.3f}, occupational={:.3f}, ethnicity={:.3f}",
                    score.overall_score, score.gender_bias, score.occupational_bias, score.ethnicity_bias);
        
        return score;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Bias detection failed: {}", e.what());
        throw std::runtime_error(std::string("Bias detection failed: ") + e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────

std::vector<judge::BiasScore> FairnessDetector::detectBiasBatch(
    const std::vector<std::string>& documents) {
    if (!isInitialized()) {
        THEMIS_WARN("FairnessDetector::detectBiasBatch called before initialize()");
        throw std::runtime_error("FairnessDetector not initialized");
    }

    THEMIS_DEBUG("Batch bias detection for {} documents", documents.size());
    
    std::vector<judge::BiasScore> results;
    results.reserve(documents.size());
    
    for (const auto& doc : documents) {
        results.push_back(detectBias(doc));
    }
    
    return results;
}

// ─────────────────────────────────────────────────────────────────────

std::vector<std::pair<std::string, judge::BiasScore>>
FairnessDetector::filterByBiasThreshold(const std::vector<std::string>& documents) {
    if (!isInitialized()) {
        THEMIS_WARN("FairnessDetector::filterByBiasThreshold called before initialize()");
        throw std::runtime_error("FairnessDetector not initialized");
    }

    std::vector<std::pair<std::string, judge::BiasScore>> filtered;
    
    for (const auto& doc : documents) {
        auto bias_score = detectBias(doc);
        if (bias_score.overall_score < config_.bias_threshold) {
            filtered.emplace_back(doc, bias_score);
        }
    }
    
    THEMIS_INFO("Filtered {} documents by bias threshold {}: {} passed",
               documents.size(), config_.bias_threshold, filtered.size());
    
    return filtered;
}

// ─────────────────────────────────────────────────────────────────────

const FairnessDetectorConfig& FairnessDetector::getConfig() const {
    return config_;
}

void FairnessDetector::setBiasThreshold([[maybe_unused]] double threshold) {
    config_.bias_threshold = std::clamp(threshold, 0.0, 1.0);
}

}  // namespace themis::rag

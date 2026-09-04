/**
 * @file nlp_metadata_extractor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "storage/nlp_metadata_extractor.h"
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "utils/logger.h"

namespace themis {
namespace storage {

using json = nlohmann::json;
using namespace analytics;

// Map analyzer Language enum to short code
static std::string languageToCode(NlpTextAnalyzer::Language lang) {
    switch (lang) {
    case NlpTextAnalyzer::Language::ENGLISH: return "en";
    case NlpTextAnalyzer::Language::GERMAN: return "de";
    case NlpTextAnalyzer::Language::FRENCH: return "fr";
    case NlpTextAnalyzer::Language::SPANISH: return "es";
    case NlpTextAnalyzer::Language::ITALIAN: return "it";
    case NlpTextAnalyzer::Language::DUTCH: return "nl";
    default: return "";
    }
}

// Constructor
NlpMetadataExtractor::NlpMetadataExtractor(const Config& config)
    : config_(config) {
    // Configure NLP analyzer
    NlpTextAnalyzer::Config nlp_config;
    nlp_config.max_keywords = config.max_keywords;
    nlp_config.enable_stopwords = config.enable_stopwords;
    nlp_config.stopwords_directory = config.stopwords_path.empty()
        ? nlp_config.stopwords_directory
        : config.stopwords_path;
    nlp_ = NlpTextAnalyzer(nlp_config);
}

// Extract metadata from text
NlpMetadataExtractor::ExtractedMetadata 
NlpMetadataExtractor::extractMetadata(const std::string& text) const {
    ExtractedMetadata meta = {};
    
    if (text.empty()) {
        return meta;
    }
    
    // 1. Extract keywords
    if (config_.max_keywords > 0) {
        auto keywords = nlp_.extractKeywords(text, config_.max_keywords);

        for (const auto& kw : keywords) {
            meta.keyword_scores[kw.text] = kw.score;
        }

        size_t n = std::min(static_cast<size_t>(config_.max_keywords), keywords.size());
        meta.keywords.reserve(n);  // upper bound; filtered items may be fewer
        for (size_t i = 0; i < n; ++i) {
            if (keywords[i].text.length() >= config_.min_keyword_length) {
                meta.keywords.push_back(keywords[i].text);
            }
        }
    }
    
    // 2. Extract named entities
    if (config_.extract_entities) {
        auto entities = nlp_.extractEntities(text);
        // missing_vector_reserve scanner alert: entities are dispatched by type
        // into four separate vectors; the per-type count is unknown before the
        // single pass, so a meaningful reserve() is not possible without a
        // second O(n) count pass. This single-pass dispatch is intentional.
        for (const auto& entity : entities) {
            if (entity.type == "EMAIL") {
                meta.emails.push_back(entity.text);
            } else if (entity.type == "URL") {
                meta.urls.push_back(entity.text);
            } else if (entity.type == "DATE") {
                meta.dates.push_back(entity.text);
            } else if (entity.type == "MEASUREMENT") {
                meta.measurements.push_back(entity.text);
            }
        }
    }
    
    // 3. Detect language
    if (config_.detect_language) {
        meta.detected_language = languageToCode(nlp_.detectLanguage(text));
        // Confidence based on text length (heuristic)
        if (text.length() > 1000) {
            meta.language_confidence = 0.9;
        } else if (text.length() > 100) {
            meta.language_confidence = 0.7;
        } else {
            meta.language_confidence = 0.5;
        }
    }
    
    // 4. Compute sentiment
    if (config_.compute_sentiment) {
        auto sentiment = nlp_.analyzeSentiment(text);
        meta.sentiment_score = sentiment.score;
    }
    
    // 5. Compute text complexity
    if (config_.compute_complexity) {
        auto complexity = nlp_.analyzeComplexity(text);
        // Use lexical diversity as a simple complexity proxy
        meta.text_complexity = complexity.lexical_diversity;
    }
    
    // 6. Compute text statistics
    computeTextStats(text, meta);
    
    return meta;
}

// Extract metadata from entity
NlpMetadataExtractor::ExtractedMetadata 
NlpMetadataExtractor::extractMetadataFromEntity(
    const BaseEntity& entity,
    const std::vector<std::string>& text_fields) const {
    
    // Concatenate text from all specified fields
    std::string combined_text = concatenateFields(entity, text_fields);
    
    // Extract metadata from combined text
    return extractMetadata(combined_text);
}

// Enrich entity with NLP metadata
bool NlpMetadataExtractor::enrichEntity(
    BaseEntity& entity,
    const std::vector<std::string>& text_fields) const {
    
    // Extract metadata
    auto meta = extractMetadataFromEntity(entity, text_fields);
    
    // Add metadata fields to entity
    try {
        // Keywords as JSON array
        if (!meta.keywords.empty()) {
            json keywords_json = meta.keywords;
            entity.setField("nlp_keywords", keywords_json.dump());
        }
        
        // Language
        if (!meta.detected_language.empty()) {
            entity.setField("nlp_language", meta.detected_language);
        }
        
        // Sentiment
        entity.setField("nlp_sentiment", std::to_string(meta.sentiment_score));
        
        // Complexity
        entity.setField("nlp_complexity", std::to_string(meta.text_complexity));
        
        // Entities as JSON
        if (!meta.emails.empty() || !meta.urls.empty() || 
            !meta.dates.empty() || !meta.measurements.empty()) {
            json entities_json = {};
            if (!meta.emails.empty()) {
              entities_json["emails"] = meta.emails;
            }
            if (!meta.urls.empty()) {
              entities_json["urls"] = meta.urls;
            }
            if (!meta.dates.empty()) {
              entities_json["dates"] = meta.dates;
            }
            if (!meta.measurements.empty()) {
              entities_json["measurements"] = meta.measurements;
            }
            entity.setField("nlp_entities", entities_json.dump());
        }
        
        // Text statistics
        entity.setField("nlp_word_count", std::to_string(meta.total_words));
        entity.setField("nlp_unique_words", std::to_string(meta.unique_words));
        entity.setField("nlp_lexical_diversity", std::to_string(meta.lexical_diversity));
        
        return true;
    } catch (...) {
        THEMIS_WARN("nlp_metadata_extractor: unhandled exception caught");
        return false;
    }
}

// Extract keywords only (fast path)
std::vector<std::string> NlpMetadataExtractor::extractKeywords(
    const std::string& text,
    size_t max_keywords) const {

    auto keywords_scored = nlp_.extractKeywords(text, max_keywords);

    // Already sorted by score in analyzer; enforce length filter
    std::vector<std::string> keywords = {};

    for (const auto& kw : keywords_scored) {
        if (kw.text.length() >= config_.min_keyword_length) {
            keywords.push_back(kw.text);
        }
        if (static_cast<int>(keywords.size()) >= max_keywords) {
          break;
        }
    }
    
    return keywords;
}

// Detect language only
std::string NlpMetadataExtractor::detectLanguage(const std::string& text) const {
    return languageToCode(nlp_.detectLanguage(text));
}

// Extract named entities only
std::map<std::string, std::vector<std::string>> 
NlpMetadataExtractor::extractEntities(const std::string& text) const {
    std::map<std::string, std::vector<std::string>> result;
    
    auto entities = nlp_.extractEntities(text);
    for (const auto& entity : entities) {
        result[entity.type].push_back(entity.text);
    }
    
    return result;
}

// Compute text statistics
void NlpMetadataExtractor::computeTextStats(
    const std::string& text,
    ExtractedMetadata& meta) const {
    
    if (text.empty()) {
        return;
    }
    
    // Tokenize text
    auto tokens = nlp_.tokenize(text);
    meta.total_words = tokens.size();

    // Count unique words (use token.text from analytics::Token)
    std::set<std::string> unique_tokens;
    size_t total_chars = 0;
    for (const auto& token : tokens) {
        unique_tokens.insert(token.text);
        total_chars += token.text.length();
    }
    meta.unique_words = unique_tokens.size();
    
    // Lexical diversity
    if (meta.total_words > 0) {
        meta.lexical_diversity = static_cast<double>(meta.unique_words) / 
                                 static_cast<double>(meta.total_words);
    }
    
    // Average word length
    if (meta.total_words > 0) {
        meta.avg_word_length = static_cast<double>(total_chars) / 
                               static_cast<double>(meta.total_words);
    }
    
    // Count sentences (simple heuristic: count sentence terminators)
    meta.total_sentences = 1; // At least one sentence
    for (char c : text) {
        if (c == '.' || c == '!' || c == '?') {
            meta.total_sentences++;
        }
    }
    
    // Average sentence length
    if (meta.total_sentences > 0) {
        meta.avg_sentence_length = static_cast<double>(meta.total_words) / 
                                    static_cast<double>(meta.total_sentences);
    }
}

// Concatenate text from multiple fields
std::string NlpMetadataExtractor::concatenateFields(
    const BaseEntity& entity,
    const std::vector<std::string>& fields) const {
    
    std::ostringstream oss = {};
    bool first = true;
    
    for (const auto& field : fields) {
        auto value = entity.getFieldString(field);
        if (!value.empty()) {
            if (!first) {
              oss << " ";
            }
            oss << value;
            first = false;
        }
    }
    
    return oss.str();
}

// Convert metadata to JSON
std::string NlpMetadataExtractor::ExtractedMetadata::toJson() const {
    json j;
    
    // Keywords
    j["keywords"] = keywords;
    j["keyword_scores"] = keyword_scores;
    
    // Named entities
    j["emails"] = emails;
    j["urls"] = urls;
    j["dates"] = dates;
    j["measurements"] = measurements;
    
    // Language & sentiment
    j["detected_language"] = detected_language;
    j["language_confidence"] = language_confidence;
    j["sentiment_score"] = sentiment_score;
    
    // Text complexity
    j["text_complexity"] = text_complexity;
    j["total_words"] = total_words;
    j["unique_words"] = unique_words;
    j["lexical_diversity"] = lexical_diversity;
    
    // Readability
    j["total_sentences"] = total_sentences;
    j["avg_sentence_length"] = avg_sentence_length;
    j["avg_word_length"] = avg_word_length;
    
    return j.dump();
}

// Parse metadata from JSON
NlpMetadataExtractor::ExtractedMetadata 
NlpMetadataExtractor::ExtractedMetadata::fromJson(const std::string& json_str) {
    ExtractedMetadata meta;
    
    try {
        auto j = json::parse(json_str);
        
        // Keywords
        if (j.contains("keywords")) {
          meta.keywords = j["keywords"].get<std::vector<std::string>>();
        }
        if (j.contains("keyword_scores")) {
          meta.keyword_scores = j["keyword_scores"].get<std::map<std::string, double>>();
        }
        
        // Named entities
        if (j.contains("emails")) {
          meta.emails = j["emails"].get<std::vector<std::string>>();
        }
        if (j.contains("urls")) {
          meta.urls = j["urls"].get<std::vector<std::string>>();
        }
        if (j.contains("dates")) {
          meta.dates = j["dates"].get<std::vector<std::string>>();
        }
        if (j.contains("measurements")) {
          meta.measurements = j["measurements"].get<std::vector<std::string>>();
        }
        
        // Language & sentiment
        if (j.contains("detected_language")) {
          meta.detected_language = j["detected_language"];
        }
        if (j.contains("language_confidence")) {
          meta.language_confidence = j["language_confidence"];
        }
        if (j.contains("sentiment_score")) {
          meta.sentiment_score = j["sentiment_score"];
        }
        
        // Text complexity
        if (j.contains("text_complexity")) {
          meta.text_complexity = j["text_complexity"];
        }
        if (j.contains("total_words")) {
          meta.total_words = j["total_words"];
        }
        if (j.contains("unique_words")) {
          meta.unique_words = j["unique_words"];
        }
        if (j.contains("lexical_diversity")) {
          meta.lexical_diversity = j["lexical_diversity"];
        }
        
        // Readability
        if (j.contains("total_sentences")) {
          meta.total_sentences = j["total_sentences"];
        }
        if (j.contains("avg_sentence_length")) {
          meta.avg_sentence_length = j["avg_sentence_length"];
        }
        if (j.contains("avg_word_length")) {
          meta.avg_word_length = j["avg_word_length"];
        }
    } catch (...) {
        THEMIS_WARN("nlp_metadata_extractor: unhandled exception caught");
        // Return empty metadata on parse error
    }
    
    return meta;
}

} // namespace storage
} // namespace themis


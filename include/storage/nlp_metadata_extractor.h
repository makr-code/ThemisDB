/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            nlp_metadata_extractor.h                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:27:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     208                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file nlp_metadata_extractor.h
 * @brief NLP-based metadata extraction for document ingestion
 * 
 * Part of PR #317 Phase 2: Ingestion Integration
 * Provides NLP-powered keyword extraction, metadata generation,
 * and content analysis during document ingestion.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include "analytics/nlp_text_analyzer.h"
#include "storage/base_entity.h"

namespace themis {
namespace storage {

/**
 * @brief NLP-powered metadata extractor for document ingestion
 * 
 * Enriches documents with:
 * - Keywords (TF-IDF based)
 * - Named entities (emails, URLs, dates, measurements)
 * - Language detection
 * - Content complexity metrics
 * - Sentiment analysis
 * - Text statistics
 */
class NlpMetadataExtractor {
public:
    /**
     * @brief Configuration for metadata extraction
     */
    struct Config {
        size_t max_keywords = 10;           ///< Maximum keywords to extract
        size_t min_keyword_length = 3;      ///< Minimum keyword length
        bool extract_entities = true;       ///< Extract named entities
        bool detect_language = true;        ///< Detect document language
        bool compute_sentiment = true;      ///< Compute sentiment score
        bool compute_complexity = true;     ///< Compute text complexity
        bool enable_stopwords = true;       ///< Use stopword filtering
        std::string stopwords_path;         ///< Path to stopwords directory
    };
    
    /**
     * @brief Extracted metadata from document
     */
    struct ExtractedMetadata {
        // Keywords
        std::vector<std::string> keywords;           ///< Top keywords (TF-IDF)
        std::map<std::string, double> keyword_scores; ///< Keyword -> TF-IDF score
        
        // Named Entities
        std::vector<std::string> emails;             ///< Extracted email addresses
        std::vector<std::string> urls;               ///< Extracted URLs
        std::vector<std::string> dates;              ///< Extracted dates
        std::vector<std::string> measurements;       ///< Extracted measurements
        
        // Language & Sentiment
        std::string detected_language;               ///< Detected language code (en, de, fr)
        double language_confidence = 0.0;            ///< Language detection confidence
        double sentiment_score = 0.0;                ///< Sentiment (-1.0 to +1.0)
        
        // Text Complexity
        double text_complexity = 0.0;                ///< Overall complexity (0.0-1.0)
        size_t total_words = 0;                      ///< Total word count
        size_t unique_words = 0;                     ///< Unique word count
        double lexical_diversity = 0.0;              ///< unique/total ratio
        
        // Readability
        size_t total_sentences = 0;                  ///< Sentence count
        double avg_sentence_length = 0.0;            ///< Avg words per sentence
        double avg_word_length = 0.0;                ///< Avg characters per word
        
        /**
         * @brief Convert to JSON for storage
         */
        std::string toJson() const;
        
        /**
         * @brief Parse from JSON
         */
        static ExtractedMetadata fromJson(const std::string& json_str);
    };
    
    /**
     * @brief Constructor with configuration
     */
    explicit NlpMetadataExtractor(const Config& config);
    
    /**
     * @brief Extract metadata from text content
     * 
     * @param text Input text to analyze
     * @return Extracted metadata
     */
    ExtractedMetadata extractMetadata(const std::string& text) const;
    
    /**
     * @brief Extract metadata from BaseEntity
     * 
     * Analyzes text fields in the entity and returns metadata.
     * Useful during document ingestion.
     * 
     * @param entity Entity to analyze
     * @param text_fields Fields to analyze (e.g., {"content", "description"})
     * @return Extracted metadata
     */
    ExtractedMetadata extractMetadataFromEntity(
        const BaseEntity& entity,
        const std::vector<std::string>& text_fields = {"content", "text", "body"}) const;
    
    /**
     * @brief Enrich entity with NLP metadata
     * 
     * Adds metadata fields to entity:
     * - nlp_keywords: array of keywords
     * - nlp_language: detected language
     * - nlp_sentiment: sentiment score
     * - nlp_complexity: text complexity
     * - nlp_entities: extracted named entities
     * 
     * @param entity Entity to enrich (modified in-place)
     * @param text_fields Fields to analyze
     * @return true if successful
     */
    bool enrichEntity(
        BaseEntity& entity,
        const std::vector<std::string>& text_fields = {"content", "text", "body"}) const;
    
    /**
     * @brief Extract keywords only (fast path)
     * 
     * @param text Input text
     * @param max_keywords Maximum keywords to return
     * @return List of keywords
     */
    std::vector<std::string> extractKeywords(
        const std::string& text,
        size_t max_keywords = 10) const;
    
    /**
     * @brief Detect language only (fast path)
     * 
     * @param text Input text
     * @return Language code (en, de, fr, etc.)
     */
    std::string detectLanguage(const std::string& text) const;
    
    /**
     * @brief Extract named entities only (fast path)
     * 
     * @param text Input text
     * @return Map of entity type -> list of entities
     */
    std::map<std::string, std::vector<std::string>> extractEntities(
        const std::string& text) const;
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;
    mutable analytics::NlpTextAnalyzer nlp_;
    
    /**
     * @brief Compute text statistics
     */
    void computeTextStats(const std::string& text, ExtractedMetadata& meta) const;
    
    /**
     * @brief Concatenate text from multiple fields
     */
    std::string concatenateFields(
        const BaseEntity& entity,
        const std::vector<std::string>& fields) const;
};

} // namespace storage
} // namespace themis

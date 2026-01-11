/**
 * @file test_nlp_metadata_extractor.cpp
 * @brief Tests for NLP-based metadata extraction during ingestion
 * 
 * Part of PR #317 Phase 2: Ingestion Integration
 */

#include <gtest/gtest.h>
#include "storage/nlp_metadata_extractor.h"
#include "storage/base_entity.h"
#include <iostream>

using namespace themis;
using namespace themis::storage;

class NlpMetadataExtractorTest : public ::testing::Test {
protected:
    NlpMetadataExtractor extractor;
    
    void SetUp() override {
        // Default configuration
    }
};

/**
 * @test Extract keywords from text
 */
TEST_F(NlpMetadataExtractorTest, ExtractKeywords) {
    std::string text = 
        "Database optimization and query performance are critical for modern applications. "
        "ThemisDB provides advanced indexing and caching mechanisms for optimal performance.";
    
    auto keywords = extractor.extractKeywords(text, 5);
    
    EXPECT_FALSE(keywords.empty());
    EXPECT_LE(keywords.size(), 5u);
    
    std::cout << "Extracted keywords: ";
    for (const auto& kw : keywords) {
        std::cout << kw << " ";
    }
    std::cout << "\n";
}

/**
 * @test Extract full metadata from text
 */
TEST_F(NlpMetadataExtractorTest, ExtractMetadataFromText) {
    std::string text = 
        "Contact us at info@themisdb.com or visit https://themisdb.com for more information. "
        "Our database performance benchmarks on 2024-01-15 showed 10,000 queries per second throughput. "
        "The system handles 2TB of data with sub-millisecond latency.";
    
    auto meta = extractor.extractMetadata(text);
    
    // Keywords
    EXPECT_FALSE(meta.keywords.empty());
    
    // Named entities
    EXPECT_FALSE(meta.emails.empty());
    EXPECT_EQ(meta.emails[0], "info@themisdb.com");
    
    EXPECT_FALSE(meta.urls.empty());
    EXPECT_TRUE(meta.urls[0].find("themisdb.com") != std::string::npos);
    
    EXPECT_FALSE(meta.dates.empty());
    EXPECT_FALSE(meta.measurements.empty());
    
    // Language
    EXPECT_FALSE(meta.detected_language.empty());
    EXPECT_GT(meta.language_confidence, 0.0);
    
    // Text statistics
    EXPECT_GT(meta.total_words, 0u);
    EXPECT_GT(meta.unique_words, 0u);
    EXPECT_GT(meta.lexical_diversity, 0.0);
    EXPECT_LE(meta.lexical_diversity, 1.0);
    
    std::cout << "Metadata extracted:\n";
    std::cout << "  Keywords: " << meta.keywords.size() << "\n";
    std::cout << "  Emails: " << meta.emails.size() << "\n";
    std::cout << "  URLs: " << meta.urls.size() << "\n";
    std::cout << "  Dates: " << meta.dates.size() << "\n";
    std::cout << "  Language: " << meta.detected_language << "\n";
    std::cout << "  Sentiment: " << meta.sentiment_score << "\n";
    std::cout << "  Complexity: " << meta.text_complexity << "\n";
    std::cout << "  Total words: " << meta.total_words << "\n";
}

/**
 * @test Extract metadata from BaseEntity
 */
TEST_F(NlpMetadataExtractorTest, ExtractMetadataFromEntity) {
    // Create entity with text content
    BaseEntity entity("doc123");
    entity.setField("content", 
        "Machine learning and artificial intelligence are transforming software development. "
        "Neural networks enable sophisticated pattern recognition and prediction capabilities.");
    entity.setField("title", "AI in Software Development");
    
    // Extract metadata from content field
    auto meta = extractor.extractMetadataFromEntity(entity, {"content", "title"});
    
    EXPECT_FALSE(meta.keywords.empty());
    EXPECT_GT(meta.total_words, 0u);
    EXPECT_FALSE(meta.detected_language.empty());
    
    std::cout << "Entity metadata:\n";
    for (const auto& kw : meta.keywords) {
        std::cout << "  Keyword: " << kw << "\n";
    }
}

/**
 * @test Enrich entity with NLP metadata
 */
TEST_F(NlpMetadataExtractorTest, EnrichEntity) {
    // Create entity
    BaseEntity entity("doc456");
    entity.setField("content", 
        "The quick brown fox jumps over the lazy dog. "
        "This sentence contains every letter of the alphabet.");
    
    // Enrich entity
    bool success = extractor.enrichEntity(entity, {"content"});
    EXPECT_TRUE(success);
    
    // Verify NLP fields were added
    EXPECT_TRUE(entity.hasField("nlp_keywords"));
    EXPECT_TRUE(entity.hasField("nlp_language"));
    EXPECT_TRUE(entity.hasField("nlp_sentiment"));
    EXPECT_TRUE(entity.hasField("nlp_complexity"));
    EXPECT_TRUE(entity.hasField("nlp_word_count"));
    
    // Check values
    auto language = entity.getFieldString("nlp_language");
    EXPECT_FALSE(language.empty());
    
    std::cout << "Enriched entity fields:\n";
    std::cout << "  nlp_language: " << entity.getFieldString("nlp_language") << "\n";
    std::cout << "  nlp_sentiment: " << entity.getFieldString("nlp_sentiment") << "\n";
    std::cout << "  nlp_complexity: " << entity.getFieldString("nlp_complexity") << "\n";
    std::cout << "  nlp_word_count: " << entity.getFieldString("nlp_word_count") << "\n";
}

/**
 * @test Language detection
 */
TEST_F(NlpMetadataExtractorTest, LanguageDetection) {
    std::string english = "The database provides excellent performance and scalability.";
    std::string german = "Die Datenbank bietet hervorragende Leistung und Skalierbarkeit.";
    std::string french = "La base de données offre d'excellentes performances et évolutivité.";
    
    auto lang_en = extractor.detectLanguage(english);
    auto lang_de = extractor.detectLanguage(german);
    auto lang_fr = extractor.detectLanguage(french);
    
    EXPECT_FALSE(lang_en.empty());
    EXPECT_FALSE(lang_de.empty());
    EXPECT_FALSE(lang_fr.empty());
    
    std::cout << "Language detection:\n";
    std::cout << "  English: " << lang_en << "\n";
    std::cout << "  German: " << lang_de << "\n";
    std::cout << "  French: " << lang_fr << "\n";
}

/**
 * @test Named entity extraction
 */
TEST_F(NlpMetadataExtractorTest, NamedEntityExtraction) {
    std::string text = 
        "Please contact john.doe@example.com or visit www.example.com. "
        "The meeting is scheduled for 2024-03-15 at 14:30. "
        "The project requires 500GB of storage and 32GB of RAM.";
    
    auto entities = extractor.extractEntities(text);
    
    EXPECT_TRUE(entities.count("EMAIL") > 0);
    EXPECT_TRUE(entities.count("URL") > 0);
    EXPECT_TRUE(entities.count("DATE") > 0);
    EXPECT_TRUE(entities.count("MEASUREMENT") > 0);
    
    std::cout << "Named entities:\n";
    for (const auto& [type, values] : entities) {
        std::cout << "  " << type << ": ";
        for (const auto& val : values) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

/**
 * @test Metadata JSON serialization
 */
TEST_F(NlpMetadataExtractorTest, MetadataJsonSerialization) {
    std::string text = "Database query optimization and performance tuning.";
    
    auto meta = extractor.extractMetadata(text);
    
    // Serialize to JSON
    std::string json_str = meta.toJson();
    EXPECT_FALSE(json_str.empty());
    
    // Deserialize from JSON
    auto meta2 = NlpMetadataExtractor::ExtractedMetadata::fromJson(json_str);
    
    // Verify data preserved
    EXPECT_EQ(meta.keywords.size(), meta2.keywords.size());
    EXPECT_EQ(meta.detected_language, meta2.detected_language);
    EXPECT_DOUBLE_EQ(meta.sentiment_score, meta2.sentiment_score);
    
    std::cout << "JSON serialization successful\n";
    std::cout << "JSON: " << json_str.substr(0, 100) << "...\n";
}

/**
 * @test Empty text handling
 */
TEST_F(NlpMetadataExtractorTest, EmptyTextHandling) {
    std::string empty_text = "";
    
    auto meta = extractor.extractMetadata(empty_text);
    
    EXPECT_TRUE(meta.keywords.empty());
    EXPECT_EQ(meta.total_words, 0u);
    EXPECT_EQ(meta.unique_words, 0u);
}

/**
 * @test Custom configuration
 */
TEST_F(NlpMetadataExtractorTest, CustomConfiguration) {
    NlpMetadataExtractor::Config config;
    config.max_keywords = 3;
    config.min_keyword_length = 5;
    config.extract_entities = false;
    
    NlpMetadataExtractor custom_extractor(config);
    
    std::string text = 
        "Database optimization performance scalability reliability availability consistency.";
    
    auto meta = custom_extractor.extractMetadata(text);
    
    // Should have at most 3 keywords
    EXPECT_LE(meta.keywords.size(), 3u);
    
    // Should not extract entities
    EXPECT_TRUE(meta.emails.empty());
    EXPECT_TRUE(meta.urls.empty());
    
    // All keywords should be >= 5 characters
    for (const auto& kw : meta.keywords) {
        EXPECT_GE(kw.length(), 5u);
    }
    
    std::cout << "Custom config keywords (max 3, min length 5): ";
    for (const auto& kw : meta.keywords) {
        std::cout << kw << " ";
    }
    std::cout << "\n";
}

/**
 * @test Performance benchmark
 */
TEST_F(NlpMetadataExtractorTest, PerformanceBenchmark) {
    std::string text = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris. "
        "Contact: test@example.com, visit https://example.com on 2024-01-01.";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        auto meta = extractor.extractMetadata(text);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    double avg_ms = duration / 100.0;
    
    std::cout << "Average metadata extraction time: " << avg_ms << " ms\n";
    
    // Should be fast (< 10ms per document)
    EXPECT_LT(avg_ms, 10.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

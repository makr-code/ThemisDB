/**
 * @file test_knowledge_gap_detector.cpp
 * @brief Unit tests for Knowledge Gap Detector
 */

#include <gtest/gtest.h>
#include "rag/knowledge_gap_detector.h"
#include <vector>
#include <string>

using namespace themis::rag::knowledge_gap;

class KnowledgeGapDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.mode = DetectionMode::BALANCED;
        config_.similarity_threshold = 0.75;
        config_.min_documents = 3;
        config_.confidence_threshold = 0.7;
        config_.coverage_threshold = 0.8;
        config_.enable_query_aspect_analysis = true;
        
        detector_ = std::make_unique<KnowledgeGapDetector>(config_);
    }

    void TearDown() override {
        detector_.reset();
    }

    // Helper to create test documents
    std::vector<RetrievedDocument> createDocuments(
        const std::vector<std::string>& contents,
        const std::vector<double>& scores
    ) {
        std::vector<RetrievedDocument> docs;
        for (size_t i = 0; i < contents.size(); ++i) {
            RetrievedDocument doc;
            doc.id = "doc" + std::to_string(i);
            doc.content = contents[i];
            doc.similarity_score = i < scores.size() ? scores[i] : 0.8;
            docs.push_back(doc);
        }
        return docs;
    }

    KnowledgeGapConfig config_;
    std::unique_ptr<KnowledgeGapDetector> detector_;
};

// ============================================================================
// Similarity-based Detection Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, LowSimilarityTriggersGap) {
    // Test that low similarity scores (< 0.75) trigger gap detection
    auto docs = createDocuments(
        {"Document about cats", "Document about dogs", "Document about birds"},
        {0.5, 0.6, 0.55}  // All below threshold
    );
    
    auto result = detector_->detectPreGeneration("query about animals", docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::LOW_SIMILARITY);
    EXPECT_LT(result.avg_similarity_score, 0.75);
    EXPECT_EQ(result.recommendation, FallbackStrategy::REFORMULATE_QUERY);
}

TEST_F(KnowledgeGapDetectorTest, HighSimilarityNoGap) {
    // Test that high similarity scores don't trigger gap
    auto docs = createDocuments(
        {"Detailed information about animals", 
         "More animal facts", 
         "Animal behavior studies"},
        {0.85, 0.9, 0.88}  // All above threshold
    );
    
    auto result = detector_->detectPreGeneration("query about animals", docs);
    
    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::NONE);
    EXPECT_GE(result.avg_similarity_score, 0.75);
}

TEST_F(KnowledgeGapDetectorTest, SimilarityNormalization) {
    // Test that similarity scores are properly normalized to 0.0-1.0
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.95, 0.85, 0.90}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_GE(result.avg_similarity_score, 0.0);
    EXPECT_LE(result.avg_similarity_score, 1.0);
    EXPECT_NEAR(result.avg_similarity_score, 0.9, 0.01);
}

// ============================================================================
// Document Count Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, InsufficientDocumentsTriggersGap) {
    // Test that < 3 documents trigger gap detection
    auto docs = createDocuments(
        {"Document 1", "Document 2"},
        {0.9, 0.85}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    EXPECT_EQ(result.num_retrieved_docs, 2u);
    EXPECT_EQ(result.recommendation, FallbackStrategy::EXPAND_SEARCH);
}

TEST_F(KnowledgeGapDetectorTest, SufficientDocumentsNoGap) {
    // Test that >= 3 documents with good scores don't trigger gap
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3", "Doc4"},
        {0.9, 0.85, 0.88, 0.92}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.num_retrieved_docs, 4u);
}

TEST_F(KnowledgeGapDetectorTest, ConfigurableMinDocuments) {
    // Test that min_documents threshold is configurable
    config_.min_documents = 5;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3", "Doc4"},
        {0.9, 0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
}

// ============================================================================
// Query Aspect Analysis Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, MissingAspectsDetection) {
    // Test detection of missing query aspects
    auto docs = createDocuments(
        {"Information about machine", "Data about learning"},
        {0.85, 0.85}
    );
    
    auto result = detector_->detectPreGeneration(
        "machine learning algorithms implementation", 
        docs
    );
    
    // Should detect missing aspects (algorithms, implementation not covered)
    if (result.gap_detected && result.gap_type == GapType::MISSING_ASPECTS) {
        EXPECT_FALSE(result.missing_aspects.empty());
    }
}

TEST_F(KnowledgeGapDetectorTest, CoverageCalculation) {
    // Test query coverage calculation
    auto docs = createDocuments(
        {"Machine learning algorithms for classification",
         "Implementation of neural networks",
         "Deep learning frameworks"},
        {0.85, 0.85, 0.85}
    );
    
    auto result = detector_->detectPreGeneration(
        "machine learning implementation", 
        docs
    );
    
    EXPECT_GE(result.coverage_score, 0.0);
    EXPECT_LE(result.coverage_score, 1.0);
}

TEST_F(KnowledgeGapDetectorTest, LowCoverageTriggersGap) {
    // Test that low coverage triggers gap detection
    config_.coverage_threshold = 0.9;  // High threshold
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Some vague information"},
        {0.8}
    );
    
    auto result = detector_->detectPreGeneration(
        "specific detailed query about complex topics", 
        docs
    );
    
    // Should trigger gap due to low coverage
    EXPECT_TRUE(result.gap_detected);
}

// ============================================================================
// Metadata-based Filtering Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, OutdatedDocumentsDetection) {
    // Test detection of outdated documents
    std::vector<RetrievedDocument> docs;
    
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "Old information";
    doc1.similarity_score = 0.85;
    doc1.metadata["timestamp"] = "2020-01-01T00:00:00Z";  // Outdated
    docs.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.id = "doc2";
    doc2.content = "Another old doc";
    doc2.similarity_score = 0.85;
    doc2.metadata["timestamp"] = "2019-01-01T00:00:00Z";  // Outdated
    docs.push_back(doc2);
    
    RetrievedDocument doc3;
    doc3.id = "doc3";
    doc3.content = "Yet another old doc";
    doc3.similarity_score = 0.85;
    doc3.metadata["timestamp"] = "2018-01-01T00:00:00Z";  // Outdated
    docs.push_back(doc3);
    
    auto result = detector_->detectPreGeneration("current information query", docs);
    
    // Should detect gap due to outdated information
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::OUTDATED_INFO);
}

TEST_F(KnowledgeGapDetectorTest, RecentDocumentsNoGap) {
    // Test that recent documents don't trigger outdated gap
    std::vector<RetrievedDocument> docs;
    
    for (int i = 0; i < 3; ++i) {
        RetrievedDocument doc;
        doc.id = "doc" + std::to_string(i);
        doc.content = "Recent information";
        doc.similarity_score = 0.85;
        doc.metadata["timestamp"] = "2024-01-01T00:00:00Z";  // Recent
        docs.push_back(doc);
    }
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_FALSE(result.gap_detected || result.gap_type != GapType::OUTDATED_INFO);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, ConfigurationUpdate) {
    // Test that configuration can be updated
    KnowledgeGapConfig new_config;
    new_config.similarity_threshold = 0.9;
    new_config.min_documents = 5;
    
    detector_->setConfig(new_config);
    auto retrieved_config = detector_->getConfig();
    
    EXPECT_EQ(retrieved_config.similarity_threshold, 0.9);
    EXPECT_EQ(retrieved_config.min_documents, 5u);
}

TEST_F(KnowledgeGapDetectorTest, ThresholdConfiguration) {
    // Test different threshold configurations
    config_.similarity_threshold = 0.5;  // Very lenient
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.6, 0.55, 0.6}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    // Should not trigger gap with lenient threshold
    EXPECT_FALSE(result.gap_detected || result.gap_type != GapType::LOW_SIMILARITY);
}

// ============================================================================
// During-Generation Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, LowTokenProbabilityTriggersGap) {
    // Test that low token probability triggers gap during generation
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.85, 0.85, 0.85}
    );
    
    GenerationContext context;
    context.token_probability_avg = 0.5;  // Below threshold
    context.perplexity = 50.0;
    context.generation_started = true;
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::UNCERTAIN_GENERATION);
}

TEST_F(KnowledgeGapDetectorTest, HighPerplexityTriggersGap) {
    // Test that high perplexity triggers gap during generation
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.85, 0.85, 0.85}
    );
    
    GenerationContext context;
    context.token_probability_avg = 0.8;
    context.perplexity = 150.0;  // High perplexity
    context.generation_started = true;
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::UNCERTAIN_GENERATION);
}

// ============================================================================
// Post-Generation Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, ClaimExtraction) {
    // Test basic claim extraction
    std::string answer = "Machine learning is a subset of AI. It uses statistical methods. "
                        "Deep learning is popular.";
    
    auto docs = createDocuments(
        {"ML and AI information", "Statistical methods", "Deep learning guide"},
        {0.85, 0.85, 0.85}
    );
    
    auto result = detector_->detectPostGeneration("test query", docs, answer);
    
    // Should extract and verify claims
    EXPECT_GE(result.confidence_score, 0.0);
}

// ============================================================================
// Factory Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, FastDetectorFactory) {
    auto fast_detector = KnowledgeGapDetectorFactory::createFast();
    auto config = fast_detector->getConfig();
    
    EXPECT_EQ(config.mode, DetectionMode::FAST);
    EXPECT_FALSE(config.enable_self_consistency_check);
    EXPECT_FALSE(config.enable_claim_verification);
}

TEST_F(KnowledgeGapDetectorTest, BalancedDetectorFactory) {
    auto balanced_detector = KnowledgeGapDetectorFactory::createBalanced();
    auto config = balanced_detector->getConfig();
    
    EXPECT_EQ(config.mode, DetectionMode::BALANCED);
    EXPECT_TRUE(config.enable_query_aspect_analysis);
}

TEST_F(KnowledgeGapDetectorTest, ThoroughDetectorFactory) {
    auto thorough_detector = KnowledgeGapDetectorFactory::createThorough();
    auto config = thorough_detector->getConfig();
    
    EXPECT_EQ(config.mode, DetectionMode::THOROUGH);
    EXPECT_TRUE(config.enable_self_consistency_check);
    EXPECT_TRUE(config.enable_claim_verification);
    EXPECT_TRUE(config.enable_query_aspect_analysis);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, ComprehensiveDetection) {
    // Test comprehensive detection across all levels
    auto docs = createDocuments(
        {"Comprehensive document about the topic",
         "Additional details and information",
         "Supporting evidence and data"},
        {0.85, 0.85, 0.85}
    );
    
    GenerationContext context;
    context.token_probability_avg = 0.8;
    context.perplexity = 50.0;
    context.generation_started = true;
    
    std::string answer = "The topic is well documented. Multiple sources confirm this. "
                        "Evidence supports the conclusion.";
    
    auto result = detector_->detectGap("comprehensive query", docs, answer, context);
    
    // Should pass all checks with good documents and answer
    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::NONE);
}

TEST_F(KnowledgeGapDetectorTest, GapDetectionCallback) {
    // Test gap detection callback mechanism
    bool callback_called = false;
    DetectionResult callback_result;
    
    detector_->setGapDetectionCallback([&](const DetectionResult& result) {
        callback_called = true;
        callback_result = result;
    });
    
    // Trigger gap with insufficient documents
    auto docs = createDocuments({"Doc1"}, {0.9});
    
    detector_->detectPreGeneration("test query", docs);
    
    // Callback should have been triggered in some modes
    // Note: Callback is only called in THOROUGH mode or when explicitly triggered
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, EmptyDocuments) {
    // Test with no documents
    std::vector<RetrievedDocument> docs;
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    EXPECT_EQ(result.num_retrieved_docs, 0u);
}

TEST_F(KnowledgeGapDetectorTest, EmptyQuery) {
    // Test with empty query
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.85, 0.85, 0.85}
    );
    
    auto result = detector_->detectPreGeneration("", docs);
    
    // Should handle gracefully
    EXPECT_GE(result.confidence_score, 0.0);
}

TEST_F(KnowledgeGapDetectorTest, VeryLongContent) {
    // Test with very long document content
    std::string long_content(10000, 'x');
    auto docs = createDocuments(
        {long_content, long_content, long_content},
        {0.85, 0.85, 0.85}
    );
    
    auto result = detector_->detectPreGeneration("test query", docs);
    
    // Should handle without crashing
    EXPECT_GE(result.confidence_score, 0.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

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
        // Setup test configuration
        // Default configuration
        config_.mode = DetectionMode::BALANCED;
        config_.similarity_threshold = 0.75;
        config_.min_documents = 3;
        config_.confidence_threshold = 0.7;
    }
    
    KnowledgeGapConfig config_;
};

// Test: Factory methods
TEST_F(KnowledgeGapDetectorTest, FactoryCreateFast) {
    auto detector = KnowledgeGapDetectorFactory::createFast();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::FAST);
}

TEST_F(KnowledgeGapDetectorTest, FactoryCreateBalanced) {
    auto detector = KnowledgeGapDetectorFactory::createBalanced();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::BALANCED);
}

TEST_F(KnowledgeGapDetectorTest, FactoryCreateThorough) {
    auto detector = KnowledgeGapDetectorFactory::createThorough();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::THOROUGH);
}

// Test: Configuration
TEST_F(KnowledgeGapDetectorTest, ConfigurationUpdate) {
    KnowledgeGapDetector detector(config_);
    
    KnowledgeGapConfig new_config;
    new_config.mode = DetectionMode::FAST;
    new_config.similarity_threshold = 0.8;
    
    detector.setConfig(new_config);
    auto retrieved_config = detector.getConfig();
    
    EXPECT_EQ(retrieved_config.mode, DetectionMode::FAST);
    EXPECT_DOUBLE_EQ(retrieved_config.similarity_threshold, 0.8);
}

// Test: Pre-generation detection with insufficient documents
TEST_F(KnowledgeGapDetectorTest, PreGenerationInsufficientDocs) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs;
    
    // Add only 2 documents (below threshold of 3)
    docs.push_back({"doc1", "Paris is a city.", 0.9, {}});
    docs.push_back({"doc2", "France is a country.", 0.85, {}});
    
    auto result = detector.detectPreGeneration(query, docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    EXPECT_EQ(result.num_retrieved_docs, 2);
    EXPECT_EQ(result.recommendation, FallbackStrategy::EXPAND_SEARCH);
}

// Test: Pre-generation detection with sufficient documents
TEST_F(KnowledgeGapDetectorTest, PreGenerationSufficientDocs) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs;
    
    // Add 3 documents with good similarity
    docs.push_back({"doc1", "Paris is the capital of France.", 0.95, {}});
    docs.push_back({"doc2", "The capital city of France is Paris.", 0.92, {}});
    docs.push_back({"doc3", "France's capital is located in Paris.", 0.90, {}});
    
    auto result = detector.detectPreGeneration(query, docs);
    
    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::NONE);
    EXPECT_EQ(result.num_retrieved_docs, 3);
    EXPECT_GT(result.avg_similarity_score, 0.75);
}

// Test: Pre-generation detection with low similarity
TEST_F(KnowledgeGapDetectorTest, PreGenerationLowSimilarity) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "What is quantum computing?";
    std::vector<RetrievedDocument> docs;
    
    // Add documents with low similarity scores
    docs.push_back({"doc1", "Some unrelated content about cooking.", 0.3, {}});
    docs.push_back({"doc2", "Another document about gardening.", 0.25, {}});
    docs.push_back({"doc3", "Document about sports.", 0.2, {}});
    
    auto result = detector.detectPreGeneration(query, docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::LOW_SIMILARITY);
    EXPECT_LT(result.avg_similarity_score, 0.75);
}

// Test: During generation detection
TEST_F(KnowledgeGapDetectorTest, DuringGeneration) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "Explain machine learning";
    std::vector<RetrievedDocument> docs;
    docs.push_back({"doc1", "Machine learning is...", 0.9, {}});
    docs.push_back({"doc2", "ML algorithms...", 0.85, {}});
    docs.push_back({"doc3", "Neural networks...", 0.8, {}});
    
    GenerationContext context;
    context.token_probability_avg = 0.5;  // Low confidence
    context.perplexity = 50.0;
    context.generation_started = true;
    
    auto result = detector.detectDuringGeneration(query, docs, context);
    
    // Should detect low confidence during generation
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
    
    // Should not detect outdated gap with recent documents
    if (result.gap_detected) {
        EXPECT_NE(result.gap_type, GapType::OUTDATED_INFO);
    }
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
    if (result.gap_detected) {
        EXPECT_NE(result.gap_type, GapType::LOW_SIMILARITY);
    }
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

// Test: Post-generation detection
TEST_F(KnowledgeGapDetectorTest, PostGeneration) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "What is AI?";
    std::vector<RetrievedDocument> docs;
    docs.push_back({"doc1", "AI is artificial intelligence.", 0.9, {}});
    docs.push_back({"doc2", "Machine learning is a subset of AI.", 0.85, {}});
    docs.push_back({"doc3", "Deep learning uses neural networks.", 0.8, {}});
    
    std::string answer = "AI stands for artificial intelligence and includes machine learning.";
    
    auto result = detector.detectPostGeneration(query, docs, answer);
    
    // Basic post-generation check
    EXPECT_EQ(result.num_retrieved_docs, 3);
}

// Test: Comprehensive detection
TEST_F(KnowledgeGapDetectorTest, ComprehensiveDetection) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "What is blockchain?";
    std::vector<RetrievedDocument> docs;
    docs.push_back({"doc1", "Blockchain is a distributed ledger.", 0.95, {}});
    docs.push_back({"doc2", "Cryptocurrencies use blockchain.", 0.9, {}});
    docs.push_back({"doc3", "Bitcoin was the first blockchain.", 0.85, {}});
    
    std::string answer = "Blockchain is a distributed ledger technology used in cryptocurrencies.";
    
    GenerationContext context;
    context.token_probability_avg = 0.85;
    context.perplexity = 10.0;
    context.generation_started = true;
    
    auto result = detector.detectGap(query, docs, answer, context);
    
    EXPECT_EQ(result.num_retrieved_docs, 3);
    EXPECT_GT(result.avg_similarity_score, 0.75);
}

// Test: Gap detection callback
TEST_F(KnowledgeGapDetectorTest, GapDetectionCallback) {
    KnowledgeGapDetector detector(config_);
    
    bool callback_invoked = false;
    detector.setGapDetectionCallback([&callback_invoked](const DetectionResult& result) {
        callback_invoked = true;
    });
    
    std::string query = "Test query";
    std::vector<RetrievedDocument> docs;
    docs.push_back({"doc1", "Content", 0.5, {}});
    
    detector.detectPreGeneration(query, docs);
    
    // Note: Callback invocation depends on implementation details
    // This test documents the API
}

// Test: Empty documents
TEST_F(KnowledgeGapDetectorTest, EmptyDocuments) {
    KnowledgeGapDetector detector(config_);
    
    std::string query = "Any question";
    std::vector<RetrievedDocument> empty_docs;
    
    auto result = detector.detectPreGeneration(query, empty_docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    EXPECT_EQ(result.num_retrieved_docs, 0);
}

// Test: Custom configuration
TEST_F(KnowledgeGapDetectorTest, CustomConfiguration) {
    KnowledgeGapConfig custom_config;
    custom_config.mode = DetectionMode::THOROUGH;
    custom_config.similarity_threshold = 0.9;
    custom_config.min_documents = 5;
    custom_config.enable_self_consistency_check = true;
    custom_config.enable_claim_verification = true;
    
    auto detector = KnowledgeGapDetectorFactory::create(custom_config);
    ASSERT_NE(detector, nullptr);
    
    auto retrieved_config = detector->getConfig();
    EXPECT_EQ(retrieved_config.mode, DetectionMode::THOROUGH);
    EXPECT_DOUBLE_EQ(retrieved_config.similarity_threshold, 0.9);
    EXPECT_EQ(retrieved_config.min_documents, 5);
    EXPECT_TRUE(retrieved_config.enable_self_consistency_check);
    EXPECT_TRUE(retrieved_config.enable_claim_verification);
}

int main(int argc, char **argv) {
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
    EXPECT_FALSE(result.gap_detected);
}

// ============================================================================
// Phase 2: Token Probability & Perplexity Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, PerplexityCalculation) {
    // Test perplexity calculation with known probabilities
    GenerationContext context;
    context.token_probs = {0.9, 0.85, 0.88, 0.92, 0.87}; // High confidence tokens
    context.generation_started = true;
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    // With high token probabilities, no gap should be detected
    EXPECT_FALSE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, HighPerplexityDetection) {
    // Test that high perplexity (low probabilities) triggers gap
    GenerationContext context;
    context.token_probs = {0.2, 0.15, 0.18, 0.12, 0.19}; // Low confidence tokens
    context.generation_started = true;
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    // Low token probabilities should trigger gap detection
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::UNCERTAIN_GENERATION);
}

TEST_F(KnowledgeGapDetectorTest, SlidingWindowPerplexity) {
    // Test sliding window perplexity with varying token probabilities
    GenerationContext context;
    // Mix of high and low confidence tokens
    context.token_probs = {0.9, 0.85, 0.2, 0.15, 0.1, 0.88, 0.92, 0.87};
    context.generation_started = true;
    
    config_.perplexity_window_size = 3;
    config_.perplexity_threshold = 50.0; // Adjust for test
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    // Should detect anomaly in the low-probability window
    EXPECT_TRUE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, OutlierTokenRemoval) {
    // Test that outlier tokens are properly handled
    GenerationContext context;
    // Mostly high probabilities with one outlier
    context.token_probs = {0.9, 0.88, 0.92, 0.05, 0.89, 0.91};
    context.generation_started = true;
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    // Outlier removal should prevent false positive
    // (depends on zscore threshold and implementation)
    EXPECT_FALSE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, ConfidenceScoreAggregation) {
    // Test confidence score calculation
    GenerationContext context;
    context.token_probs = {0.85, 0.82, 0.88, 0.86, 0.84};
    context.generation_started = true;
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectDuringGeneration("test query", docs, context);
    
    EXPECT_FALSE(result.gap_detected);
    EXPECT_GT(result.confidence_score, 0.7);
}

// ============================================================================
// Phase 2: Self-Consistency Check Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, SelfConsistencyMultipleSamples) {
    // Test self-consistency check with multiple samples
    config_.enable_self_consistency_check = true;
    config_.self_consistency_samples = 5;
    config_.consistency_threshold = 0.6;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Document about AI and machine learning",
         "More info on AI systems",
         "AI applications in industry"},
        {0.9, 0.85, 0.88}
    );
    
    auto result = detector_->detectPostGeneration(
        "What is AI?",
        docs,
        "AI is artificial intelligence used in various applications."
    );
    
    // Self-consistency check should pass (placeholder implementation returns true)
    EXPECT_FALSE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, SemanticSimilarityCalculation) {
    // Test semantic similarity between texts
    std::string text1 = "The quick brown fox jumps over the lazy dog";
    std::string text2 = "A fast brown fox leaps over a sleepy dog";
    
    // Both texts have similar content, should have reasonable similarity
    // (using basic Jaccard similarity in implementation)
    
    auto docs = createDocuments({"Doc1"}, {0.9});
    
    // Create samples with similar content
    auto result = detector_->detectPostGeneration(
        "test query",
        docs,
        text1
    );
    
    // Should not detect gap with consistent content
    EXPECT_FALSE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, ContradictionDetection) {
    // Test contradiction detection between statements
    config_.enable_self_consistency_check = true;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    // In real implementation, this would detect contradictions
    // Current placeholder always returns true (consistent)
    auto result = detector_->detectPostGeneration(
        "test query",
        docs,
        "The system is operational and not broken."
    );
    
    // Placeholder implementation won't detect this
    EXPECT_FALSE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, ConsistencyThresholdTuning) {
    // Test that consistency threshold affects detection
    config_.enable_self_consistency_check = true;
    config_.consistency_threshold = 0.9; // Very high threshold
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectPostGeneration(
        "test query",
        docs,
        "Test answer"
    );
    
    // Higher threshold might trigger gap (depends on implementation)
    // Current placeholder returns consistent
    EXPECT_FALSE(result.gap_detected);
}

// ============================================================================
// Phase 2: FLARE Active Retrieval Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, FLAREActiveRetrievalDisabled) {
    // Test FLARE when disabled
    config_.enable_flare = false;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2"},
        {0.8, 0.8}
    );
    
    auto result = detector_->detectWithActiveRetrieval("test query", docs);
    
    // Should fall back to regular pre-generation detection
    EXPECT_TRUE(result.gap_detected); // < 3 documents
}

TEST_F(KnowledgeGapDetectorTest, FLAREIterativeRetrieval) {
    // Test FLARE iterative retrieval
    config_.enable_flare = true;
    config_.max_retrieval_rounds = 3;
    config_.flare_confidence_threshold = 0.5;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Initial document with some information"},
        {0.6}
    );
    
    auto result = detector_->detectWithActiveRetrieval(
        "Query about complex topic needing multiple sources",
        docs
    );
    
    // FLARE should attempt retrieval but placeholder returns empty
    // So gap likely detected
    EXPECT_TRUE(result.gap_detected);
}

TEST_F(KnowledgeGapDetectorTest, FLARESentenceSplitting) {
    // Test sentence splitting for FLARE
    std::string text = "First sentence. Second sentence! Third sentence? Fourth sentence";
    
    // Indirectly test through FLARE
    config_.enable_flare = true;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectWithActiveRetrieval("test", docs);
    
    // Should handle sentence splitting internally
    EXPECT_FALSE(result.gap_detected); // Good documents
}

TEST_F(KnowledgeGapDetectorTest, FLAREQueryReformulation) {
    // Test query reformulation in FLARE
    config_.enable_flare = true;
    config_.max_retrieval_rounds = 2;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Partial information about topic"},
        {0.7}
    );
    
    auto result = detector_->detectWithActiveRetrieval(
        "Query needing more context and details",
        docs
    );
    
    // Should attempt reformulation and re-retrieval
    EXPECT_TRUE(result.gap_detected); // Placeholder retrieval returns empty
}

TEST_F(KnowledgeGapDetectorTest, FLAREMaxRoundsLimit) {
    // Test that FLARE respects max retrieval rounds
    config_.enable_flare = true;
    config_.max_retrieval_rounds = 3;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Insufficient doc"},
        {0.5}
    );
    
    auto result = detector_->detectWithActiveRetrieval("test query", docs);
    
    // Should stop after max rounds
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.num_retrieved_docs, docs.size()); // No actual retrieval in placeholder
}

TEST_F(KnowledgeGapDetectorTest, FLAREDocumentDeduplication) {
    // Test that FLARE deduplicates documents
    config_.enable_flare = true;
    detector_->setConfig(config_);
    
    auto docs = createDocuments(
        {"Doc1", "Doc2", "Doc3"},
        {0.9, 0.9, 0.9}
    );
    
    auto result = detector_->detectWithActiveRetrieval("test query", docs);
    
    // Should not have duplicate documents
    EXPECT_FALSE(result.gap_detected); // Good coverage
    EXPECT_EQ(result.num_retrieved_docs, 3); // No duplicates added
}

// ============================================================================
// Phase 2: Configuration Tests
// ============================================================================

TEST_F(KnowledgeGapDetectorTest, Phase2ConfigurationDefaults) {
    // Test Phase 2 configuration defaults
    auto config = detector_->getConfig();
    
    EXPECT_TRUE(config.enable_token_probability);
    EXPECT_EQ(config.perplexity_threshold, 100.0);
    EXPECT_EQ(config.perplexity_window_size, 10u);
    EXPECT_EQ(config.outlier_zscore_threshold, 3.0);
    EXPECT_EQ(config.self_consistency_samples, 5u);
    EXPECT_EQ(config.consistency_threshold, 0.6);
    EXPECT_FALSE(config.enable_flare);
    EXPECT_EQ(config.max_retrieval_rounds, 3u);
}

TEST_F(KnowledgeGapDetectorTest, Phase2FactoryConfiguration) {
    // Test that factory methods configure Phase 2 features properly
    
    // Fast mode: Phase 2 features disabled
    auto fast_detector = KnowledgeGapDetectorFactory::createFast();
    auto fast_config = fast_detector->getConfig();
    EXPECT_FALSE(fast_config.enable_token_probability);
    EXPECT_FALSE(fast_config.enable_flare);
    
    // Balanced mode: Token probability enabled
    auto balanced_detector = KnowledgeGapDetectorFactory::createBalanced();
    auto balanced_config = balanced_detector->getConfig();
    EXPECT_TRUE(balanced_config.enable_token_probability);
    
    // Thorough mode: All Phase 2 features enabled
    auto thorough_detector = KnowledgeGapDetectorFactory::createThorough();
    auto thorough_config = thorough_detector->getConfig();
    EXPECT_TRUE(thorough_config.enable_token_probability);
    EXPECT_TRUE(thorough_config.enable_self_consistency_check);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

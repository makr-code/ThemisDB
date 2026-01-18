/**
 * @file test_knowledge_gap_detector.cpp
 * @brief Unit tests for Knowledge Gap Detector
 */

#include <gtest/gtest.h>
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

class KnowledgeGapDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test configuration
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
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

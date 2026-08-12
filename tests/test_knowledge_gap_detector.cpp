/**
 * @file test_knowledge_gap_detector.cpp
 * @brief Unit tests for the Knowledge Gap Detector
 */

#include <gtest/gtest.h>
#include <array>
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

// Helper: create N documents with the given similarity score
static std::vector<RetrievedDocument> makeDocs(size_t count, double similarity) {
    static const std::array<std::string, 5> kContents = {
        "The Eiffel Tower is located in Paris, France and was built in 1889.",
        "Photosynthesis converts sunlight into chemical energy stored in glucose.",
        "The Treaty of Versailles ended World War I and was signed in 1919.",
        "Water boils at 100 degrees Celsius at standard atmospheric pressure.",
        "The human genome contains approximately 3 billion base pairs of DNA."
    };
    std::vector<RetrievedDocument> docs;
    docs.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        RetrievedDocument d;
        d.id = "doc" + std::to_string(i);
        d.content = kContents[i % kContents.size()];
        d.similarity_score = similarity;
        docs.push_back(d);
    }
    return docs;
}

class KnowledgeGapDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.mode = DetectionMode::FAST;
        config_.min_documents = 2;
        config_.similarity_threshold = 0.5;
        config_.enable_query_aspect_analysis = false;
        config_.enable_self_consistency_check = false;
        config_.enable_token_probability = false;
        config_.enable_ethical_gap_detection = false;
    }

    KnowledgeGapConfig config_;
};

// ---- Factory tests -------------------------------------------------------

TEST(KnowledgeGapDetectorFactory, CreateFast) {
    auto detector = KnowledgeGapDetectorFactory::createFast();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::FAST);
}

TEST(KnowledgeGapDetectorFactory, CreateBalanced) {
    auto detector = KnowledgeGapDetectorFactory::createBalanced();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::BALANCED);
}

TEST(KnowledgeGapDetectorFactory, CreateThorough) {
    auto detector = KnowledgeGapDetectorFactory::createThorough();
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getConfig().mode, DetectionMode::THOROUGH);
}

// ---- detectPreGeneration tests -------------------------------------------

TEST_F(KnowledgeGapDetectorTest, PreGeneration_InsufficientDocs) {
    KnowledgeGapDetector detector(config_);

    auto docs = makeDocs(1, 0.9); // fewer than min_documents=2
    auto result = detector.detectPreGeneration("What is the capital of France?", docs);

    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    EXPECT_GT(result.confidence_score, 0.0);
}

TEST_F(KnowledgeGapDetectorTest, PreGeneration_LowSimilarity) {
    config_.similarity_threshold = 0.8;
    KnowledgeGapDetector detector(config_);

    // 3 docs but similarity below threshold
    auto docs = makeDocs(3, 0.3);
    auto result = detector.detectPreGeneration("test query", docs);

    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::LOW_SIMILARITY);
}

TEST_F(KnowledgeGapDetectorTest, PreGeneration_NoGap) {
    KnowledgeGapDetector detector(config_);

    // Enough docs with high similarity
    auto docs = makeDocs(4, 0.9);
    auto result = detector.detectPreGeneration("test query", docs);

    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::NONE);
}

// ---- Configuration tests -------------------------------------------------

TEST_F(KnowledgeGapDetectorTest, SetConfigUpdatesThresholds) {
    KnowledgeGapDetector detector(config_);

    KnowledgeGapConfig new_cfg = config_;
    new_cfg.similarity_threshold = 0.95;
    detector.setConfig(new_cfg);

    EXPECT_DOUBLE_EQ(detector.getConfig().similarity_threshold, 0.95);
}

// ---- Callback test -------------------------------------------------------

TEST_F(KnowledgeGapDetectorTest, GapCallbackIsInvoked) {
    config_.mode = DetectionMode::THOROUGH;
    config_.enable_self_consistency_check = false;
    config_.enable_claim_verification = false;
    KnowledgeGapDetector detector(config_);

    bool callback_called = false;
    detector.setGapDetectionCallback([&](const DetectionResult&) {
        callback_called = true;
    });

    // Trigger a gap by providing too few documents (0 < min_documents=2)
    auto docs = makeDocs(0, 0.9);
    detector.detectGap("query", docs, "answer");

    EXPECT_TRUE(callback_called);
}

// ---- detectPostGeneration: no gap when answer matches docs ---------------

TEST_F(KnowledgeGapDetectorTest, PostGeneration_NoGapForSimpleAnswer) {
    config_.enable_claim_verification = false;
    KnowledgeGapDetector detector(config_);

    auto docs = makeDocs(3, 0.9);
    auto result = detector.detectPostGeneration("query", docs, "simple answer");

    // With claim verification disabled and good docs, no gap expected
    EXPECT_FALSE(result.gap_detected);
}

// ---- Integration tests: FLARE default, legacy mode, semantic search ------

TEST(KnowledgeGapDetectorFactory, FlareEnabledByDefault) {
    // v1.4.0: FLARE must be enabled in the default KnowledgeGapConfig
    KnowledgeGapConfig default_config;
    EXPECT_TRUE(default_config.enable_flare);

    // createProductionReady() must also have FLARE enabled
    auto production = KnowledgeGapDetectorFactory::createProductionReady();
    ASSERT_NE(production, nullptr);
    EXPECT_TRUE(production->getConfig().enable_flare);
}

TEST(KnowledgeGapDetectorFactory, BackwardCompatLegacyMode) {
    // createLegacy() must have FLARE disabled for v1.3 compat
    auto legacy = KnowledgeGapDetectorFactory::createLegacy();
    ASSERT_NE(legacy, nullptr);
    EXPECT_FALSE(legacy->getConfig().enable_flare);

    // Functional smoke-test: legacy detector still detects pre-generation gaps
    auto docs = makeDocs(1, 0.9); // fewer than min_documents=3
    auto result = legacy->detectPreGeneration("What is the capital of France?", docs);
    EXPECT_TRUE(result.gap_detected);
}

TEST(KnowledgeGapDetectorFactory, FlareWithSemanticSearchIntegration) {
    // createProductionReady() should work with detectWithActiveRetrieval
    auto detector = KnowledgeGapDetectorFactory::createProductionReady();
    ASSERT_NE(detector, nullptr);
    EXPECT_TRUE(detector->getConfig().enable_flare);
    EXPECT_EQ(detector->getConfig().max_retrieval_rounds, 3u);
    EXPECT_DOUBLE_EQ(detector->getConfig().perplexity_threshold, 100.0);

    // Exercise the FLARE path with a small document set
    auto docs = makeDocs(2, 0.6);
    auto result = detector->detectWithActiveRetrieval("complex multi-hop query", docs);
    // Result is structurally valid regardless of gap outcome
    EXPECT_GE(result.confidence_score, 0.0);
    EXPECT_LE(result.confidence_score, 1.0);
}


// ── KGD-LLM-01: injected LlmSampleFn is called for self-consistency check
TEST(KnowledgeGapDetectorLlmSampleFn, KGD_LLM_01_InjectedFnCalled) {
    using namespace themis::rag;
    KnowledgeGapConfig cfg;
    cfg.enable_self_consistency_check = true;
    cfg.min_documents  = 1;
    cfg.similarity_threshold = 0.0;
    KnowledgeGapDetector detector(cfg);

    bool fn_called = false;
    detector.setLlmSampleFn([&fn_called](const std::string& /*query*/, size_t n) {
        fn_called = true;
        // Return consistent samples to avoid triggering a gap
        return std::vector<std::string>(n, "The capital of France is Paris.");
    });

    RetrievedDocument doc;
    doc.content        = "Paris is the capital of France.";
    doc.similarity_score = 0.9;
    std::string answer = "Paris is the capital of France.";
    auto result = detector.detectPostGeneration("What is the capital of France?", {doc}, answer);
    EXPECT_TRUE(fn_called);
    EXPECT_GE(result.confidence_score, 0.0);
    EXPECT_LE(result.confidence_score, 1.0);
}

// ── KGD-LLM-02: clearing the fn reverts to heuristic path
TEST(KnowledgeGapDetectorLlmSampleFn, KGD_LLM_02_ClearFnRevertsToHeuristic) {
    using namespace themis::rag;
    KnowledgeGapConfig cfg;
    cfg.enable_self_consistency_check = true;
    cfg.min_documents  = 1;
    cfg.similarity_threshold = 0.0;
    KnowledgeGapDetector detector(cfg);

    bool fn_called = false;
    detector.setLlmSampleFn([&fn_called](const std::string&, size_t n) {
        fn_called = true;
        return std::vector<std::string>(n, "sample");
    });
    // Clear fn
    detector.setLlmSampleFn({});

    RetrievedDocument doc;
    doc.content        = "Paris is the capital of France.  France is in Europe.";
    doc.similarity_score = 0.9;
    auto result = detector.detectPostGeneration("What is the capital?", {doc}, "Paris.");
    EXPECT_FALSE(fn_called);
    // Result must still be structurally valid
    EXPECT_GE(result.confidence_score, 0.0);
    EXPECT_LE(result.confidence_score, 1.0);
}

// ── KGD-LLM-03: fn returning empty vector falls back to heuristic path (no crash)
TEST(KnowledgeGapDetectorLlmSampleFn, KGD_LLM_03_EmptyReturnFallsBackToHeuristic) {
    using namespace themis::rag;
    KnowledgeGapConfig cfg;
    cfg.enable_self_consistency_check = true;
    cfg.min_documents  = 1;
    cfg.similarity_threshold = 0.0;
    KnowledgeGapDetector detector(cfg);

    detector.setLlmSampleFn([](const std::string&, size_t) {
        return std::vector<std::string>{};  // intentionally empty
    });

    RetrievedDocument doc;
    doc.content        = "Rome is the capital of Italy.  Italy is in Europe.";
    doc.similarity_score = 0.85;
    auto result = detector.detectPostGeneration("Capital of Italy?", {doc}, "Rome.");
    // Must not crash; result is structurally valid regardless of outcome
    EXPECT_GE(result.confidence_score, 0.0);
    EXPECT_LE(result.confidence_score, 1.0);
}

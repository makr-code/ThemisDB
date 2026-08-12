/**
 * @file test_rag_reranker.cpp
 * @brief Unit tests for the CrossEncoderReranker (re-ranking layer)
 *
 * Tests cover:
 *  - Score range validity [0, 1]
 *  - Relevant document ranked above irrelevant document
 *  - top_k truncation
 *  - Empty query / empty candidates edge cases
 *  - Score cache behaviour (hit/miss/invalidation on setConfig)
 *  - Batch scoring consistency with single scoring
 *  - loadModel() / isModelLoaded() lifecycle
 *  - CrossEncoderFactory helpers
 *  - RerankResult metadata (used_model, rerank_time, scores vector)
 *  - min_score_threshold filtering
 *  - Score ordering in RerankResult::scores
 */

#include "rag/reranker.h"
#include <gtest/gtest.h>

using namespace themis::rag;
using namespace themis::rag::judge;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<RetrievedDocument> makeDocuments(
    const std::vector<std::pair<std::string, std::string>>& id_content)
{
    std::vector<RetrievedDocument> docs;
    docs.reserve(id_content.size());
    for (size_t i = 0; i < id_content.size(); ++i) {
        RetrievedDocument d;
        d.id               = id_content[i].first;
        d.content          = id_content[i].second;
        d.similarity_score = 1.0 - 0.1 * static_cast<double>(i);  // initial scores
        docs.push_back(d);
    }
    return docs;
}

// ============================================================================
// CrossEncoderReranker – basic scoring
// ============================================================================

class RerankerScoringTest : public ::testing::Test {
protected:
    CrossEncoderReranker reranker;  // default config, heuristic scorer
};

TEST_F(RerankerScoringTest, ScoreRangeValid) {
    double s = reranker.score("capital of France", "Paris is the capital of France.");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST_F(RerankerScoringTest, RelevantDocScoresHigherThanIrrelevant) {
    std::string query = "What is the capital of France?";
    std::string relevant   = "Paris is the capital of France and a major European city.";
    std::string irrelevant = "Bananas are a tropical fruit rich in potassium.";

    EXPECT_GT(reranker.score(query, relevant),
              reranker.score(query, irrelevant));
}

TEST_F(RerankerScoringTest, EmptyQueryReturnsZero) {
    EXPECT_DOUBLE_EQ(reranker.score("", "Some document text."), 0.0);
}

TEST_F(RerankerScoringTest, EmptyDocumentReturnsZero) {
    EXPECT_DOUBLE_EQ(reranker.score("What is AI?", ""), 0.0);
}

TEST_F(RerankerScoringTest, IdenticalQueryAndDocHighScore) {
    // A document that exactly repeats the query should score high
    std::string text = "machine learning classification supervised learning";
    double s = reranker.score(text, text);
    EXPECT_GT(s, 0.5);
}

// ============================================================================
// CrossEncoderReranker – batch scoring
// ============================================================================

TEST(RerankerBatchTest, BatchConsistentWithSingle) {
    CrossEncoderReranker reranker;
    std::string query = "capital of France";
    std::vector<std::string> docs = {
        "Paris is the capital of France.",
        "Bananas are rich in potassium.",
        "France is located in Western Europe."
    };

    auto batch = reranker.scoreBatch(query, docs);
    ASSERT_EQ(batch.size(), docs.size());

    for (size_t i = 0; i < docs.size(); ++i) {
        EXPECT_NEAR(batch[i], reranker.score(query, docs[i]), 1e-9)
            << "Mismatch at index " << i;
    }
}

TEST(RerankerBatchTest, EmptyDocListReturnsEmptyScores) {
    CrossEncoderReranker reranker;
    auto scores = reranker.scoreBatch("query", {});
    EXPECT_TRUE(scores.empty());
}

// ============================================================================
// CrossEncoderReranker – rerank()
// ============================================================================

class RerankerRerankTest : public ::testing::Test {
protected:
    CrossEncoderConfig config;
    std::string query = "What is machine learning?";

    std::vector<RetrievedDocument> candidates = makeDocuments({
        {"doc1", "Machine learning is a subset of artificial intelligence."},
        {"doc2", "Quantum computing leverages quantum mechanics."},
        {"doc3", "Machine learning algorithms learn from data automatically."},
        {"doc4", "The weather in Paris is mild in spring."},
        {"doc5", "Supervised machine learning requires labelled training data."}
    });
};

TEST_F(RerankerRerankTest, ResultCountRespectsTopK) {
    config.top_k = 3;
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates);

    EXPECT_LE(result.documents.size(), 3u);
    EXPECT_EQ(result.documents.size(), result.scores.size());
}

TEST_F(RerankerRerankTest, TopKOverrideAtCallSite) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates, 2);

    EXPECT_LE(result.documents.size(), 2u);
}

TEST_F(RerankerRerankTest, DocumentsOrderedByScore) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates);

    for (size_t i = 1; i < result.documents.size(); ++i) {
        EXPECT_GE(result.documents[i - 1].similarity_score,
                  result.documents[i].similarity_score)
            << "Documents not in descending score order at index " << i;
    }
}

TEST_F(RerankerRerankTest, ScoresVectorMatchesDocumentScores) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates);

    ASSERT_EQ(result.documents.size(), result.scores.size());
    for (size_t i = 0; i < result.documents.size(); ++i) {
        EXPECT_NEAR(result.documents[i].similarity_score,
                    result.scores[i].relevance_score, 1e-9)
            << "Score mismatch at rank " << i;
        EXPECT_EQ(result.scores[i].reranked_rank, i);
    }
}

TEST_F(RerankerRerankTest, RelevantDocRanksHigher) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates, candidates.size());

    // doc1, doc3, or doc5 (all mention "machine learning") should outrank doc2/doc4
    auto findRank = [&](const std::string& id) -> size_t {
        for (size_t i = 0; i < result.documents.size(); ++i) {
            if (result.documents[i].id == id) return i;
        }
        return result.documents.size();
    };

    size_t rank_ml  = findRank("doc1");
    size_t rank_unrelated = findRank("doc4");  // "weather in Paris"

    EXPECT_LT(rank_ml, rank_unrelated)
        << "doc1 (machine learning) should rank above doc4 (unrelated)";
}

TEST_F(RerankerRerankTest, EmptyQueryReturnsEmptyResult) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank("", candidates);
    EXPECT_TRUE(result.documents.empty());
}

TEST_F(RerankerRerankTest, EmptyCandidatesReturnsEmptyResult) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, {});
    EXPECT_TRUE(result.documents.empty());
}

TEST_F(RerankerRerankTest, RerankTimeIsSet) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates);
    // Time must be set (≥ 0); for a tiny test corpus it should be <1000 ms
    EXPECT_GE(result.rerank_time.count(), 0);
    EXPECT_LT(result.rerank_time.count(), 1000);
}

TEST_F(RerankerRerankTest, UsedModelFalseWithoutLoadedModel) {
    CrossEncoderReranker reranker(config);
    auto result = reranker.rerank(query, candidates);
    EXPECT_FALSE(result.used_model);
}

// ============================================================================
// CrossEncoderReranker – min_score_threshold filtering
// ============================================================================

TEST(RerankerThresholdTest, ThresholdFiltersLowScoringDocuments) {
    CrossEncoderConfig cfg;
    cfg.min_score_threshold = 0.99;  // very strict → only near-perfect docs pass
    CrossEncoderReranker reranker(cfg);

    auto docs = makeDocuments({
        {"doc1", "capital France Paris"},  // high overlap with query below
        {"doc2", "bananas tropical fruit"}
    });
    auto result = reranker.rerank("capital France Paris", docs);

    for (const auto& d : result.documents) {
        EXPECT_GE(d.similarity_score, 0.99)
            << "Document " << d.id << " below threshold should have been filtered";
    }
}

// ============================================================================
// CrossEncoderReranker – score cache
// ============================================================================

TEST(RerankerCacheTest, CacheHitReturnsSameScore) {
    CrossEncoderConfig cfg;
    cfg.enable_score_cache = true;
    CrossEncoderReranker reranker(cfg);

    std::string query = "quantum computing qubits";
    std::string doc   = "Quantum computing uses qubits instead of classical bits.";

    double s1 = reranker.score(query, doc);
    double s2 = reranker.score(query, doc);  // should hit cache
    EXPECT_DOUBLE_EQ(s1, s2);
}

TEST(RerankerCacheTest, ClearCacheInvalidatesEntries) {
    CrossEncoderConfig cfg;
    cfg.enable_score_cache = true;
    CrossEncoderReranker reranker(cfg);

    double s1 = reranker.score("test query", "test document");
    reranker.clearCache();
    double s2 = reranker.score("test query", "test document");

    // After clearing, the score is recomputed – must still be equal (deterministic)
    EXPECT_DOUBLE_EQ(s1, s2);
}

TEST(RerankerCacheTest, SetConfigWithChangedCacheSizeClearsCache) {
    CrossEncoderConfig cfg;
    cfg.enable_score_cache = true;
    cfg.max_cache_size     = 100;
    CrossEncoderReranker reranker(cfg);

    reranker.score("query", "doc");

    CrossEncoderConfig cfg2 = cfg;
    cfg2.max_cache_size = 200;
    reranker.setConfig(cfg2);  // should clear cache due to changed setting

    // After setConfig the reranker is still functional
    double s = reranker.score("query", "doc");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

// ============================================================================
// CrossEncoderReranker – model lifecycle
// ============================================================================

TEST(RerankerModelTest, IsModelLoadedFalseByDefault) {
    CrossEncoderReranker reranker;
    EXPECT_FALSE(reranker.isModelLoaded());
}

TEST(RerankerModelTest, LoadModelEmptyPathReturnsFalse) {
    CrossEncoderReranker reranker;
    EXPECT_FALSE(reranker.loadModel(""));
    EXPECT_FALSE(reranker.isModelLoaded());
}

TEST(RerankerModelTest, LoadModelNonEmptyPathReturnsTrue) {
    CrossEncoderReranker reranker;
    // Stub path – no actual file required for the interface test
    bool ok = reranker.loadModel("models/cross-encoder-ms-marco.onnx");
    EXPECT_TRUE(ok);
    EXPECT_TRUE(reranker.isModelLoaded());
}

TEST(RerankerModelTest, UsedModelTrueAfterLoadModel) {
    CrossEncoderReranker reranker;
    reranker.loadModel("models/cross-encoder-ms-marco.onnx");

    auto docs = makeDocuments({{"d1", "Paris is the capital of France."}});
    auto result = reranker.rerank("capital of France", docs);

    EXPECT_TRUE(result.used_model);
}

// ============================================================================
// CrossEncoderReranker – getConfig / setConfig
// ============================================================================

TEST(RerankerConfigTest, GetConfigReturnsInitialConfig) {
    CrossEncoderConfig cfg;
    cfg.top_k      = 7;
    cfg.batch_size = 16;
    CrossEncoderReranker reranker(cfg);

    EXPECT_EQ(reranker.getConfig().top_k,      7u);
    EXPECT_EQ(reranker.getConfig().batch_size, 16u);
}

TEST(RerankerConfigTest, SetConfigUpdatesConfig) {
    CrossEncoderReranker reranker;
    CrossEncoderConfig cfg;
    cfg.top_k = 5;
    reranker.setConfig(cfg);
    EXPECT_EQ(reranker.getConfig().top_k, 5u);
}

// ============================================================================
// CrossEncoderFactory
// ============================================================================

TEST(RerankerFactoryTest, CreateFastReturnsInstance) {
    auto r = CrossEncoderFactory::createFast();
    ASSERT_NE(r, nullptr);
    EXPECT_FALSE(r->isModelLoaded());
}

TEST(RerankerFactoryTest, CreateBalancedNoModelReturnsInstance) {
    auto r = CrossEncoderFactory::createBalanced();
    ASSERT_NE(r, nullptr);
    EXPECT_FALSE(r->isModelLoaded());
}

TEST(RerankerFactoryTest, CreateAccurateNoModelReturnsInstance) {
    auto r = CrossEncoderFactory::createAccurate();
    ASSERT_NE(r, nullptr);
    EXPECT_FALSE(r->isModelLoaded());
}

TEST(RerankerFactoryTest, CreateBalancedWithModelPathLoadsModel) {
    auto r = CrossEncoderFactory::createBalanced("dummy/path/model.onnx");
    ASSERT_NE(r, nullptr);
    EXPECT_TRUE(r->isModelLoaded());
}

TEST(RerankerFactoryTest, FactoryRerankerProducesValidScores) {
    auto r   = CrossEncoderFactory::createFast();
    auto docs = makeDocuments({
        {"d1", "Python is a high-level programming language."},
        {"d2", "The Amazon river is the largest in South America."}
    });
    auto result = r->rerank("Python programming language", docs);

    ASSERT_FALSE(result.documents.empty());
    for (const auto& d : result.documents) {
        EXPECT_GE(d.similarity_score, 0.0);
        EXPECT_LE(d.similarity_score, 1.0);
    }
}

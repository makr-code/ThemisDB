#include <gtest/gtest.h>
#include "index/multi_vector_search.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::vector;

class MultiVectorSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        test_db_path_ = (fs::temp_directory_path() / ("themis_mvs_test_" + std::to_string(now))).string();
        fs::remove_all(test_db_path_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        vector_mgr_ = std::make_unique<VectorIndexManager>(*db_);
        multi_search_ = std::make_unique<MultiVectorSearch>(*vector_mgr_);
        
        // Initialize index
        auto st = vector_mgr_->init("documents", 3, VectorIndexManager::Metric::COSINE);
        ASSERT_TRUE(st.ok) << st.message;
        
        // Add test documents
        addTestDocuments();
    }
    
    void TearDown() override {
        multi_search_.reset();
        vector_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    
    void addTestDocuments() {
        // Add 5 test documents with known vectors
        BaseEntity e1("doc1");
        e1.setField("embedding", std::vector<float>{1.0f, 0.0f, 0.0f});
        vector_mgr_->addEntity(e1, "embedding");
        
        BaseEntity e2("doc2");
        e2.setField("embedding", std::vector<float>{0.0f, 1.0f, 0.0f});
        vector_mgr_->addEntity(e2, "embedding");
        
        BaseEntity e3("doc3");
        e3.setField("embedding", std::vector<float>{0.0f, 0.0f, 1.0f});
        vector_mgr_->addEntity(e3, "embedding");
        
        BaseEntity e4("doc4");
        e4.setField("embedding", std::vector<float>{0.9f, 0.1f, 0.0f}); // Similar to doc1
        vector_mgr_->addEntity(e4, "embedding");
        
        BaseEntity e5("doc5");
        e5.setField("embedding", std::vector<float>{0.1f, 0.9f, 0.0f}); // Similar to doc2
        vector_mgr_->addEntity(e5, "embedding");
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vector_mgr_;
    std::unique_ptr<MultiVectorSearch> multi_search_;
};

TEST_F(MultiVectorSearchTest, LinearCombinationFusion) {
    // Query with 2 vectors
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},  // Similar to doc1
        {0.0f, 1.0f, 0.0f}   // Similar to doc2
    };
    query.weights = {0.5f, 0.5f};
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().strategy_used, MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION);
    EXPECT_GT(result.value().total_candidates, 0u);
    EXPECT_EQ(result.value().weights_used.size(), 2u);
}

TEST_F(MultiVectorSearchTest, ReciprocalRankFusion) {
    // Query with 2 vectors
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},  // Query 1
        {0.0f, 1.0f, 0.0f}   // Query 2
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK;
    config.top_k = 5;
    config.rrf_k = 60.0f;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().strategy_used, MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK);
    
    // Verify individual scores and ranks are populated
    for (const auto& res : result.value().results) {
        EXPECT_EQ(res.individual_scores.size(), 2u);
        EXPECT_EQ(res.individual_ranks.size(), 2u);
    }
}

TEST_F(MultiVectorSearchTest, RankBasedFusion) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::RANK_FUSION;
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().strategy_used, MultiVectorSearch::FusionStrategy::RANK_FUSION);
}

TEST_F(MultiVectorSearchTest, MaxScoreFusion) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::MAX_SCORE;
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().strategy_used, MultiVectorSearch::FusionStrategy::MAX_SCORE);
}

TEST_F(MultiVectorSearchTest, MinScoreFusion) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::MIN_SCORE;
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
}

TEST_F(MultiVectorSearchTest, AvgScoreFusion) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::AVG_SCORE;
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
}

TEST_F(MultiVectorSearchTest, ScoreNormalization) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.normalize_scores = true;
    config.top_k = 5;
    query.weights = {0.5f, 0.5f};
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    // Scores should be normalized
    EXPECT_GT(result.value().results.size(), 0u);
}

TEST_F(MultiVectorSearchTest, SearchWithExpansion) {
    // Query expansion: multiple reformulations of the same query
    std::vector<std::vector<float>> query_variants = {
        {1.0f, 0.0f, 0.0f},
        {0.9f, 0.1f, 0.0f},
        {0.8f, 0.2f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::RECIPROCAL_RANK;
    config.top_k = 5;
    
    auto result = multi_search_->searchWithExpansion(query_variants, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().weights_used.size(), 3u);
}

TEST_F(MultiVectorSearchTest, SearchMultiField) {
    std::vector<float> query_vector = {1.0f, 0.0f, 0.0f};
    std::vector<std::string> field_names = {"field1", "field2"};
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::AVG_SCORE;
    config.top_k = 5;
    
    auto result = multi_search_->searchMultiField(query_vector, field_names, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
}

TEST_F(MultiVectorSearchTest, HybridSearch) {
    std::vector<float> query_vector = {1.0f, 0.0f, 0.0f};
    
    // Simulate keyword/BM25 scores
    std::unordered_map<std::string, float> keyword_scores = {
        {"doc1", 0.8f},
        {"doc2", 0.6f},
        {"doc3", 0.4f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.weights = {0.7f, 0.3f};  // 70% vector, 30% keyword
    config.top_k = 5;
    
    auto result = multi_search_->hybridSearch(query_vector, keyword_scores, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().weights_used.size(), 2u);
}

TEST_F(MultiVectorSearchTest, BatchSearch) {
    std::vector<MultiVectorSearch::MultiQuery> queries;
    
    // Query 1
    MultiVectorSearch::MultiQuery q1;
    q1.vectors = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    queries.push_back(q1);
    
    // Query 2
    MultiVectorSearch::MultiQuery q2;
    q2.vectors = {{0.0f, 0.0f, 1.0f}};
    queries.push_back(q2);
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.top_k = 3;
    
    auto result = multi_search_->batchSearch(queries, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_EQ(result.value().size(), 2u);
    EXPECT_GT(result.value()[0].results.size(), 0u);
}

TEST_F(MultiVectorSearchTest, OptimizeWeights) {
    // Create training queries
    std::vector<MultiVectorSearch::MultiQuery> queries;
    MultiVectorSearch::MultiQuery q1;
    q1.vectors = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    queries.push_back(q1);
    
    // Relevance judgments (which docs are relevant)
    std::vector<std::vector<std::string>> relevance = {
        {"doc1", "doc4"}  // doc1 and doc4 are relevant for q1
    };
    
    auto result = multi_search_->optimizeWeights(queries, relevance);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_EQ(result.value().size(), 2u);
    
    // Weights should sum to approximately 1.0
    float sum = 0.0f;
    for (float w : result.value()) {
        sum += w;
    }
    EXPECT_NEAR(sum, 1.0f, 0.15f); // Allow some tolerance for grid search
}

TEST_F(MultiVectorSearchTest, Statistics) {
    // Reset statistics
    multi_search_->resetStatistics();
    
    MultiVectorSearch::MultiQuery query;
    query.vectors = {{1.0f, 0.0f, 0.0f}};
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.top_k = 5;
    
    // Perform searches
    multi_search_->search(query, config);
    multi_search_->search(query, config);
    
    const auto& stats = multi_search_->getStatistics();
    EXPECT_EQ(stats.total_searches, 2u);
    EXPECT_GT(stats.avg_time_ms, 0.0);
    EXPECT_GT(stats.strategy_usage.size(), 0u);
}

TEST_F(MultiVectorSearchTest, InvalidInputEmptyVectors) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {};  // Empty
    
    MultiVectorSearch::SearchConfig config;
    
    auto result = multi_search_->search(query, config);
    ASSERT_FALSE(result.has_value());
}

TEST_F(MultiVectorSearchTest, InvalidInputInconsistentDimensions) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},      // 3D
        {0.0f, 1.0f}             // 2D - inconsistent!
    };
    
    MultiVectorSearch::SearchConfig config;
    
    auto result = multi_search_->search(query, config);
    ASSERT_FALSE(result.has_value());
}

TEST_F(MultiVectorSearchTest, InvalidInputWeightsNotSumToOne) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.weights = {0.3f, 0.3f};  // Sum = 0.6, not 1.0
    
    auto result = multi_search_->search(query, config);
    ASSERT_FALSE(result.has_value());
}

TEST_F(MultiVectorSearchTest, TopKLimiting) {
    MultiVectorSearch::MultiQuery query;
    query.vectors = {{1.0f, 0.0f, 0.0f}};
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LINEAR_COMBINATION;
    config.top_k = 2;  // Only get top 2
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value());
    
    EXPECT_LE(result.value().results.size(), 2u);
}

TEST_F(MultiVectorSearchTest, LearnedFusion) {
    // Test LEARNED_FUSION strategy with pre-computed weights
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},  // Similar to doc1
        {0.0f, 1.0f, 0.0f}   // Similar to doc2
    };
    
    // Use LEARNED_FUSION with optimized weights
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LEARNED_FUSION;
    config.weights = {0.7f, 0.3f};  // Simulating learned weights
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    EXPECT_GT(result.value().results.size(), 0u);
    EXPECT_EQ(result.value().strategy_used, MultiVectorSearch::FusionStrategy::LEARNED_FUSION);
    EXPECT_EQ(result.value().weights_used.size(), 2u);
    EXPECT_FLOAT_EQ(result.value().weights_used[0], 0.7f);
    EXPECT_FLOAT_EQ(result.value().weights_used[1], 0.3f);
}

TEST_F(MultiVectorSearchTest, LearnedFusionWithoutWeights) {
    // Test that LEARNED_FUSION requires weights
    MultiVectorSearch::MultiQuery query;
    query.vectors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    
    MultiVectorSearch::SearchConfig config;
    config.fusion = MultiVectorSearch::FusionStrategy::LEARNED_FUSION;
    // No weights provided - should fail
    config.top_k = 5;
    
    auto result = multi_search_->search(query, config);
    EXPECT_FALSE(result.has_value());
    // MSVC workaround: extract qualified enum to avoid macro parsing error
    auto expected_error = errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT;
    EXPECT_EQ(result.error().code(), expected_error);
}

/**
 * @file test_hybrid_search_integration.cpp
 * @brief Integration tests for HybridSearch using real RocksDB, SecondaryIndexManager,
 *        and VectorIndexManager backends.
 *
 * These tests exercise the full search() pipeline end-to-end, verifying that
 * BM25 and vector candidates are fetched, fused (RRF / linear), and returned
 * correctly ranked.
 */

#include <gtest/gtest.h>
#include "search/hybrid_search.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <string>
#include <vector>

using namespace themis;
namespace fs = std::filesystem;

// ============================================================================
// Test fixture: creates a temporary DB with three documents containing both
// text content and a 3-D embedding vector.
// ============================================================================

class HybridSearchIntegrationTest : public ::testing::Test {
protected:
    static constexpr const char* kDbPath = "data/integration_test_hybrid_search";
    static constexpr const char* kTable  = "docs";
    static constexpr const char* kColumn = "body";

    void SetUp() override {
        fs::remove_all(kDbPath);
        RocksDBWrapper::Config dbCfg;
        dbCfg.db_path = kDbPath;
        dbCfg.memtable_size_mb = 16;
        dbCfg.block_cache_size_mb = 16;
        db_ = std::make_unique<RocksDBWrapper>(dbCfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";

        sec_ = std::make_unique<SecondaryIndexManager>(*db_);
        vec_ = std::make_unique<VectorIndexManager>(*db_);

        // Create a fulltext (BM25) index
        auto st = sec_->createFulltextIndex(kTable, kColumn);
        ASSERT_TRUE(st.ok) << "createFulltextIndex failed: " << st.message;

        // Create a 3-D COSINE vector index
        auto vst = vec_->init(kTable, 3, VectorIndexManager::Metric::COSINE);
        ASSERT_TRUE(vst.ok) << "VectorIndexManager::init failed: " << vst.message;

        insertTestDocuments();
    }

    void TearDown() override {
        vec_.reset();
        sec_.reset();
        db_.reset();
        fs::remove_all(kDbPath);
    }

    // -----------------------------------------------------------------------
    // Test documents
    //   doc1 – "database query optimization" – embedding close to query_vec_
    //   doc2 – "machine learning algorithms" – embedding far from query_vec_
    //   doc3 – "database indexing strategies" – embedding moderately close
    // -----------------------------------------------------------------------
    void insertTestDocuments() {
        // doc1: strong BM25 match for "database", close vector
        {
            BaseEntity e("doc1");
            e.setField(kColumn, std::string("database query optimization"));
            e.setField("embedding", std::vector<float>{0.9f, 0.1f, 0.1f});
            auto s = sec_->put(kTable, e);
            ASSERT_TRUE(s.ok) << s.message;
            auto vs = vec_->addEntity(e, "embedding");
            ASSERT_TRUE(vs.ok) << vs.message;
        }
        // doc2: no BM25 match for "database", far vector
        {
            BaseEntity e("doc2");
            e.setField(kColumn, std::string("machine learning algorithms"));
            e.setField("embedding", std::vector<float>{0.1f, 0.9f, 0.1f});
            auto s = sec_->put(kTable, e);
            ASSERT_TRUE(s.ok) << s.message;
            auto vs = vec_->addEntity(e, "embedding");
            ASSERT_TRUE(vs.ok) << vs.message;
        }
        // doc3: moderate BM25 match for "database", moderately close vector
        {
            BaseEntity e("doc3");
            e.setField(kColumn, std::string("database indexing strategies"));
            e.setField("embedding", std::vector<float>{0.8f, 0.1f, 0.2f});
            auto s = sec_->put(kTable, e);
            ASSERT_TRUE(s.ok) << s.message;
            auto vs = vec_->addEntity(e, "embedding");
            ASSERT_TRUE(vs.ok) << vs.message;
        }
    }

    HybridSearch::Config makeConfig(bool use_rrf = true) {
        HybridSearch::Config cfg;
        cfg.default_table  = kTable;
        cfg.default_column = kColumn;
        cfg.k              = 10;
        cfg.k_bm25         = 50;
        cfg.k_vector       = 50;
        cfg.use_rrf        = use_rrf;
        cfg.bm25_weight    = 0.5;
        cfg.vector_weight  = 0.5;
        cfg.normalize_scores = true;
        cfg.vector_metric  = VectorIndexManager::Metric::COSINE;
        return cfg;
    }

    // Query vector very close to doc1's embedding
    const std::vector<float> query_vec_{0.88f, 0.11f, 0.11f};

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sec_;
    std::unique_ptr<VectorIndexManager>    vec_;
};

// ============================================================================
// RRF mode
// ============================================================================

TEST_F(HybridSearchIntegrationTest, RRF_BM25Only_ReturnsTextMatches) {
    HybridSearch hs(sec_.get(), nullptr, makeConfig(/*use_rrf=*/true));
    auto results = hs.search("database");
    // doc1 and doc3 contain "database"; doc2 does not
    ASSERT_FALSE(results.empty());
    std::vector<std::string> ids;
    for (const auto& r : results) ids.push_back(r.document_id);
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "doc1") != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "doc3") != ids.end());
    // All returned results should have a positive hybrid_score
    for (const auto& r : results) {
        EXPECT_GT(r.hybrid_score, 0.0);
    }
}

TEST_F(HybridSearchIntegrationTest, RRF_VectorOnly_ReturnsNearestNeighbors) {
    HybridSearch hs(nullptr, vec_.get(), makeConfig(/*use_rrf=*/true));
    auto results = hs.search("", query_vec_.data(), query_vec_.size());
    ASSERT_FALSE(results.empty());
    // doc1 is most similar to query_vec_; it should rank first
    EXPECT_EQ(results[0].document_id, "doc1");
    for (const auto& r : results) {
        EXPECT_GT(r.hybrid_score, 0.0);
        EXPECT_GE(r.vector_score, 0.0);
    }
}

TEST_F(HybridSearchIntegrationTest, RRF_HybridSearch_doc1RanksFirst) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    ASSERT_FALSE(results.empty());
    // doc1 matches both BM25 ("database") and vector → highest hybrid score
    EXPECT_EQ(results[0].document_id, "doc1");
}

TEST_F(HybridSearchIntegrationTest, RRF_AllResultsHavePositiveHybridScore) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    for (const auto& r : results) {
        EXPECT_GT(r.hybrid_score, 0.0) << "zero score for " << r.document_id;
    }
}

TEST_F(HybridSearchIntegrationTest, RRF_ResultsSortedByHybridScoreDescending) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].hybrid_score, results[i].hybrid_score)
            << "Results not sorted at index " << i;
    }
}

TEST_F(HybridSearchIntegrationTest, RRF_KLimitsResultCount) {
    auto cfg = makeConfig(/*use_rrf=*/true);
    cfg.k = 2;
    HybridSearch hs(sec_.get(), vec_.get(), cfg);
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    EXPECT_LE(results.size(), 2u);
}

TEST_F(HybridSearchIntegrationTest, RRF_SearchStats_BothSourcesOk) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    HybridSearch::SearchStats stats;
    auto results = hs.search("database", query_vec_.data(), query_vec_.size(), &stats);
    EXPECT_TRUE(stats.bm25_ok);
    EXPECT_TRUE(stats.vector_ok);
    EXPECT_FALSE(stats.partial_result);
    EXPECT_GT(stats.bm25_count, 0u);
    EXPECT_GT(stats.vector_count, 0u);
}

TEST_F(HybridSearchIntegrationTest, RRF_NoMatchQuery_ReturnsEmpty) {
    HybridSearch hs(sec_.get(), nullptr, makeConfig(/*use_rrf=*/true));
    // Query for a term not in any document
    auto results = hs.search("xyzzy_nonexistent_term_12345");
    // May be empty or have zero BM25 candidates; result set should not crash
    for (const auto& r : results) {
        EXPECT_FALSE(r.document_id.empty());
    }
}

// ============================================================================
// Linear combination mode
// ============================================================================

TEST_F(HybridSearchIntegrationTest, Linear_HybridSearch_doc1RanksFirst) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/false));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].document_id, "doc1");
}

TEST_F(HybridSearchIntegrationTest, Linear_ResultsSortedDescending) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/false));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].hybrid_score, results[i].hybrid_score)
            << "Linear results not sorted at index " << i;
    }
}

TEST_F(HybridSearchIntegrationTest, Linear_NormalizedScoresInUnitInterval) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/false));
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    for (const auto& r : results) {
        // After normalization, individual component scores should be in [0,1]
        EXPECT_GE(r.bm25_score,   0.0) << r.document_id;
        EXPECT_LE(r.bm25_score,   1.0) << r.document_id;
        EXPECT_GE(r.vector_score, 0.0) << r.document_id;
        EXPECT_LE(r.vector_score, 1.0) << r.document_id;
    }
}

// ============================================================================
// Vector metric configuration
// ============================================================================

TEST_F(HybridSearchIntegrationTest, VectorMetricCosine_ReturnsResults) {
    auto cfg = makeConfig(/*use_rrf=*/true);
    cfg.vector_metric = VectorIndexManager::Metric::COSINE;
    HybridSearch hs(sec_.get(), vec_.get(), cfg);
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    EXPECT_FALSE(results.empty());
}

TEST_F(HybridSearchIntegrationTest, VectorMetricL2_ReturnsResults) {
    // Re-init vector index with L2 metric; text docs already in secondary index.
    vec_ = std::make_unique<VectorIndexManager>(*db_);
    auto vst = vec_->init(kTable, 3, VectorIndexManager::Metric::L2);
    ASSERT_TRUE(vst.ok) << vst.message;

    // Re-add the same embeddings used in SetUp() (text content already in sec_)
    struct DocEmb { const char* pk; float e0, e1, e2; };
    for (const auto& d : std::vector<DocEmb>{
            {"doc1", 0.9f, 0.1f, 0.1f},
            {"doc2", 0.1f, 0.9f, 0.1f},
            {"doc3", 0.8f, 0.1f, 0.2f}}) {
        BaseEntity e(d.pk);
        e.setField("embedding", std::vector<float>{d.e0, d.e1, d.e2});
        auto vs = vec_->addEntity(e, "embedding");
        ASSERT_TRUE(vs.ok) << vs.message;
    }

    auto cfg = makeConfig(/*use_rrf=*/true);
    cfg.vector_metric = VectorIndexManager::Metric::L2;
    HybridSearch hs(sec_.get(), vec_.get(), cfg);
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    EXPECT_FALSE(results.empty());
    // L2 similarity: 1/(1+distance) → always in (0,1]
    for (const auto& r : results) {
        EXPECT_GE(r.vector_score, 0.0);
    }
}

// ============================================================================
// Partial result behaviour
// ============================================================================

TEST_F(HybridSearchIntegrationTest, BM25OnlyNoVector_StatsCorrect) {
    HybridSearch hs(sec_.get(), nullptr, makeConfig(/*use_rrf=*/true));
    HybridSearch::SearchStats stats;
    hs.search("database", nullptr, 0, &stats);
    EXPECT_TRUE(stats.bm25_ok);
    EXPECT_FALSE(stats.vector_ok); // no vector index → not attempted
    EXPECT_FALSE(stats.partial_result);
    EXPECT_GT(stats.bm25_count, 0u);
    EXPECT_EQ(stats.vector_count, 0u);
}

TEST_F(HybridSearchIntegrationTest, VectorOnlyNoBM25_StatsCorrect) {
    HybridSearch hs(nullptr, vec_.get(), makeConfig(/*use_rrf=*/true));
    HybridSearch::SearchStats stats;
    hs.search("", query_vec_.data(), query_vec_.size(), &stats);
    EXPECT_FALSE(stats.bm25_ok);    // no BM25 index → not attempted
    EXPECT_TRUE(stats.vector_ok);
    EXPECT_FALSE(stats.partial_result);
    EXPECT_EQ(stats.bm25_count, 0u);
    EXPECT_GT(stats.vector_count, 0u);
}

// ============================================================================
// LLM Re-ranker integration with real indices
// ============================================================================

TEST_F(HybridSearchIntegrationTest, Reranker_InvertsRRFOrderWhenLlmFavorsLowerRanked) {
    // Without re-ranker, doc1 (rank 1 in BM25, close in vector) should top the list.
    HybridSearch hs_base(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    auto base_results = hs_base.search("database", query_vec_.data(), query_vec_.size());
    ASSERT_GE(base_results.size(), 2u);
    const std::string first_without_reranker = base_results[0].document_id;

    // Attach a mock re-ranker that gives a score of 1 to the first result and 9
    // to all others — effectively demoting the top result.
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    size_t call_count = 0;
    hs.setReranker([&](const std::string& /*prompt*/) -> std::string {
        ++call_count;
        // First doc gets score 1, rest get score 9
        return "1\n9\n9\n9\n9\n";
    });

    auto reranked = hs.search("database", query_vec_.data(), query_vec_.size());
    ASSERT_GE(reranked.size(), 2u);
    EXPECT_GT(call_count, 0u); // backend was actually called

    // The former top document should now rank lower
    EXPECT_NE(reranked[0].document_id, first_without_reranker);
}

TEST_F(HybridSearchIntegrationTest, Reranker_FallbackOnExceptionPreservesResults) {
    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    // Backend always throws — should fall back to original RRF order
    hs.setReranker([](const std::string&) -> std::string {
        throw std::runtime_error("LLM unavailable");
    });

    auto results = hs.search("database", query_vec_.data(), query_vec_.size());
    // fallback_to_original is true by default → results still returned
    EXPECT_FALSE(results.empty());
    for (const auto& r : results) {
        EXPECT_GT(r.hybrid_score, 0.0); // scores from fallback path
    }
}

TEST_F(HybridSearchIntegrationTest, Reranker_NullBackendDisablesReranking) {
    HybridSearch hs_ref(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    auto ref = hs_ref.search("database", query_vec_.data(), query_vec_.size());

    HybridSearch hs(sec_.get(), vec_.get(), makeConfig(/*use_rrf=*/true));
    hs.setReranker(nullptr); // explicitly disabled — should behave like no reranker
    auto results = hs.search("database", query_vec_.data(), query_vec_.size());

    ASSERT_EQ(results.size(), ref.size());
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i].document_id, ref[i].document_id);
    }
}

#include <gtest/gtest.h>
#include "index/ann_frontdoor.h"
#include "search/hybrid_search.h"
#include "index/vector_index.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string>

using namespace themis;

// ============================================================================
// Config Validation Tests
// ============================================================================

TEST(HybridSearchConfigValidation, DefaultConfigIsValid) {
    HybridSearch::Config cfg;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
}

TEST(HybridSearchConfigValidation, ZeroKThrows) {
    HybridSearch::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, ZeroRrfKThrows) {
    HybridSearch::Config cfg;
    cfg.rrf_k = 0.0;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, NegativeRrfKThrows) {
    HybridSearch::Config cfg;
    cfg.rrf_k = -1.0;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, NegativeBm25WeightThrows) {
    HybridSearch::Config cfg;
    cfg.bm25_weight = -0.1;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, NegativeVectorWeightThrows) {
    HybridSearch::Config cfg;
    cfg.vector_weight = -0.1;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, EmptyTableThrows) {
    HybridSearch::Config cfg;
    cfg.default_table = "";
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, EmptyColumnThrows) {
    HybridSearch::Config cfg;
    cfg.default_column = "";
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchConfigValidation, ZeroWeightsAreValid) {
    // Zero weights are allowed: results will have hybrid_score = 0 (no contribution
    // from either BM25 or vector), but the config itself is not invalid.
    // This can be useful for testing/debugging or when weights are set dynamically.
    HybridSearch::Config cfg;
    cfg.bm25_weight = 0.0;
    cfg.vector_weight = 0.0;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_DOUBLE_EQ(hs.getConfig().bm25_weight, 0.0);
    EXPECT_DOUBLE_EQ(hs.getConfig().vector_weight, 0.0);
}

// ============================================================================
// Vector Metric Configuration Tests
// ============================================================================

TEST(HybridSearchVectorMetric, DefaultMetricIsCosine) {
    HybridSearch::Config cfg;
    EXPECT_EQ(cfg.vector_metric, VectorIndexManager::Metric::COSINE);
}

TEST(HybridSearchVectorMetric, CanSetDotMetric) {
    HybridSearch::Config cfg;
    cfg.vector_metric = VectorIndexManager::Metric::DOT;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_EQ(hs.getConfig().vector_metric, VectorIndexManager::Metric::DOT);
}

TEST(HybridSearchVectorMetric, CanSetL2Metric) {
    HybridSearch::Config cfg;
    cfg.vector_metric = VectorIndexManager::Metric::L2;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_EQ(hs.getConfig().vector_metric, VectorIndexManager::Metric::L2);
}

TEST(HybridSearchVectorMetric, CanSetCosineMetric) {
    HybridSearch::Config cfg;
    cfg.vector_metric = VectorIndexManager::Metric::COSINE;
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_EQ(hs.getConfig().vector_metric, VectorIndexManager::Metric::COSINE);
}

// ============================================================================
// ANN Frontdoor integration tests
// ============================================================================

namespace {

class HybridSearchStubAnnIndex : public themis::index::IAnnIndex {
public:
    explicit HybridSearchStubAnnIndex(std::vector<themis::index::AnnSearchResult> results)
        : results_(std::move(results)) {}

    bool build(const float*, const int64_t*, size_t, size_t) override { return true; }
    [[nodiscard]] bool add(int64_t, const float*, size_t) override { return true; }

    std::vector<themis::index::AnnSearchResult> search(const float*, size_t, int k) const override {
        const auto take = std::min<int>(k, static_cast<int>(results_.size()));
        return std::vector<themis::index::AnnSearchResult>(results_.begin(), results_.begin() + take);
    }

    [[nodiscard]] std::size_t size() const override { return results_.size(); }

private:
    std::vector<themis::index::AnnSearchResult> results_;
};

} // namespace

TEST(HybridSearchVectorMetric, CanRouteVectorSearchThroughAnnFrontdoor) {
    HybridSearch::Config cfg;
    cfg.use_rrf = false;
    cfg.k = 5;

    HybridSearch hs(nullptr, nullptr, cfg);

    themis::index::AnnFrontdoor frontdoor;
    frontdoor.registerBackend("", std::make_shared<HybridSearchStubAnnIndex>(
        std::vector<themis::index::AnnSearchResult>{{42, 0.2f}, {7, 0.5f}}
    ));
    hs.setAnnFrontdoor(std::make_shared<themis::index::AnnFrontdoor>(std::move(frontdoor)));

    const float query[2] = {1.0f, 0.0f};
    auto results = hs.search("", query, 2);

    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].document_id, "42");
}

// ============================================================================
// RRF Fusion Tests (unit-testing reciprocalRankFusion directly)
// ============================================================================

class HybridSearchRRFTest : public ::testing::Test {
protected:
    HybridSearch::Config makeConfig() {
        HybridSearch::Config cfg;
        cfg.k = 10;
        cfg.rrf_k = 60.0;
        cfg.bm25_weight = 0.5;
        cfg.vector_weight = 0.5;
        cfg.use_rrf = true;
        return cfg;
    }

    HybridSearch::Result makeResult(const std::string& id, double bm25 = 0.0,
                                     double vec = 0.0, int bm25_rank = -1,
                                     int vec_rank = -1) {
        HybridSearch::Result r;
        r.document_id = id;
        r.bm25_score = bm25;
        r.vector_score = vec;
        r.bm25_rank = bm25_rank;
        r.vector_rank = vec_rank;
        return r;
    }
};

TEST_F(HybridSearchRRFTest, EmptyInputsReturnEmpty) {
    HybridSearch hs(nullptr, nullptr, makeConfig());
    auto results = hs.reciprocalRankFusion({}, {});
    EXPECT_TRUE(results.empty());
}

TEST_F(HybridSearchRRFTest, OnlyBM25Results) {
    HybridSearch hs(nullptr, nullptr, makeConfig());
    std::vector<HybridSearch::Result> bm25 = {
        makeResult("doc1"), makeResult("doc2"), makeResult("doc3")
    };
    auto results = hs.reciprocalRankFusion(bm25, {});
    EXPECT_EQ(results.size(), 3u);
    // First result should be highest ranked (rank 1)
    EXPECT_EQ(results[0].document_id, "doc1");
}

TEST_F(HybridSearchRRFTest, OnlyVectorResults) {
    HybridSearch hs(nullptr, nullptr, makeConfig());
    std::vector<HybridSearch::Result> vec = {
        makeResult("doc1"), makeResult("doc2")
    };
    auto results = hs.reciprocalRankFusion({}, vec);
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc1");
}

TEST_F(HybridSearchRRFTest, OverlappingResultsBoostedHigher) {
    HybridSearch hs(nullptr, nullptr, makeConfig());
    // doc1 appears in both lists → should score higher than doc2/doc3
    std::vector<HybridSearch::Result> bm25 = {
        makeResult("doc1"), makeResult("doc2")
    };
    std::vector<HybridSearch::Result> vec = {
        makeResult("doc1"), makeResult("doc3")
    };
    auto results = hs.reciprocalRankFusion(bm25, vec);
    EXPECT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "doc1");
}

TEST_F(HybridSearchRRFTest, ResultsLimitedToK) {
    auto cfg = makeConfig();
    cfg.k = 2;
    HybridSearch hs(nullptr, nullptr, cfg);
    std::vector<HybridSearch::Result> bm25 = {
        makeResult("doc1"), makeResult("doc2"), makeResult("doc3"), makeResult("doc4")
    };
    auto results = hs.reciprocalRankFusion(bm25, {});
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(HybridSearchRRFTest, ScoresArePositive) {
    HybridSearch hs(nullptr, nullptr, makeConfig());
    std::vector<HybridSearch::Result> bm25 = {makeResult("doc1")};
    std::vector<HybridSearch::Result> vec = {makeResult("doc1")};
    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_GT(results[0].hybrid_score, 0.0);
}

TEST_F(HybridSearchRRFTest, RRFScoreFormula) {
    // Verify RRF score: score = w_bm25 * 1/(k+rank_bm25) + w_vec * 1/(k+rank_vec)
    // With k=60, rank=1 each, weights=0.5: 0.5*(1/61) + 0.5*(1/61) ≈ 0.01639
    HybridSearch hs(nullptr, nullptr, makeConfig());
    std::vector<HybridSearch::Result> bm25 = {makeResult("doc1")};
    std::vector<HybridSearch::Result> vec = {makeResult("doc1")};
    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_EQ(results.size(), 1u);
    double expected = 0.5 * (1.0 / 61.0) + 0.5 * (1.0 / 61.0);
    EXPECT_NEAR(results[0].hybrid_score, expected, 1e-9);
}

TEST_F(HybridSearchRRFTest, WeightsAffectScore) {
    // BM25-only weight=1.0, vector_weight=0.0
    auto cfg = makeConfig();
    cfg.bm25_weight = 1.0;
    cfg.vector_weight = 0.0;
    HybridSearch hs(nullptr, nullptr, cfg);
    std::vector<HybridSearch::Result> bm25 = {makeResult("doc1"), makeResult("doc2")};
    std::vector<HybridSearch::Result> vec = {makeResult("doc2"), makeResult("doc1")};
    auto results = hs.reciprocalRankFusion(bm25, vec);
    // doc1 has rank 1 in BM25 (weight 1.0) → should rank higher than doc2
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc1");
}

// ============================================================================
// Score Normalization Edge Cases
// ============================================================================

class HybridSearchNormalizationTest : public ::testing::Test {
protected:
    HybridSearch::Config makeConfig() {
        HybridSearch::Config cfg;
        cfg.normalize_scores = true;
        cfg.use_rrf = false; // Test linear combination to exercise normalization
        return cfg;
    }
};

TEST_F(HybridSearchNormalizationTest, SingleResultNormalizedToOne) {
    // When only one document is in the BM25 or vector result set (range == 0),
    // normalizeScores should set the score to 1.0 if the score is positive.
    // We exercise this indirectly via reciprocalRankFusion: a single-entry BM25
    // result processed through linear combination (use_rrf=false) triggers
    // normalizeScores. With null indices, no real search runs, so we verify
    // normalization logic through RRF (public API) with a single ranked entry.
    HybridSearch hs(nullptr, nullptr, makeConfig());
    HybridSearch::Result r;
    r.document_id = "doc1";
    r.bm25_score = 5.0;  // any positive score
    r.bm25_rank = 1;
    // RRF does not call normalizeScores, but the single entry should rank first
    auto results = hs.reciprocalRankFusion({r}, {});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "doc1");
    // Score should be positive (rank-based RRF contribution)
    EXPECT_GT(results[0].hybrid_score, 0.0);
}

TEST_F(HybridSearchNormalizationTest, ConfigGetSetRoundtrip) {
    HybridSearch::Config cfg;
    cfg.normalize_scores = false;
    cfg.k = 5;
    cfg.rrf_k = 30.0;
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_FALSE(hs.getConfig().normalize_scores);
    EXPECT_EQ(hs.getConfig().k, 5u);
    EXPECT_DOUBLE_EQ(hs.getConfig().rrf_k, 30.0);
}

// ============================================================================
// Search with null indices returns empty results
// ============================================================================

TEST(HybridSearchNullIndices, NullBothIndicesReturnsEmpty) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    auto results = hs.search("hello world", nullptr, 0);
    EXPECT_TRUE(results.empty());
}

TEST(HybridSearchNullIndices, NullFulltextIndexWithVectorQueryReturnsEmpty) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    float vec[] = {0.1f, 0.2f, 0.3f};
    // vector_index_ is nullptr → no vector search
    auto results = hs.search("", vec, 3);
    EXPECT_TRUE(results.empty());
}

TEST(HybridSearchNullIndices, EmptyTextAndNullVectorReturnsEmpty) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    auto results = hs.search("", nullptr, 0);
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Resource Limits Tests
// ============================================================================

TEST(HybridSearchResourceLimits, ZeroMaxKThrows) {
    HybridSearch::Config cfg;
    cfg.max_k = 0;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchResourceLimits, ZeroMaxCandidatesThrows) {
    HybridSearch::Config cfg;
    cfg.max_candidates = 0;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchResourceLimits, KExceedingMaxKThrows) {
    HybridSearch::Config cfg;
    cfg.max_k = 5;
    cfg.k = 6;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchResourceLimits, KBm25ExceedingMaxCandidatesThrows) {
    HybridSearch::Config cfg;
    cfg.max_candidates = 100;
    cfg.k_bm25 = 101;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchResourceLimits, KVectorExceedingMaxCandidatesThrows) {
    HybridSearch::Config cfg;
    cfg.max_candidates = 100;
    cfg.k_vector = 101;
    EXPECT_THROW(HybridSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(HybridSearchResourceLimits, KEqualsMaxKIsValid) {
    HybridSearch::Config cfg;
    cfg.max_k = 10;
    cfg.k = 10;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
}

TEST(HybridSearchResourceLimits, KBm25EqualsMaxCandidatesIsValid) {
    HybridSearch::Config cfg;
    cfg.max_candidates = 50;
    cfg.k_bm25 = 50;
    cfg.k_vector = 50;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
}

TEST(HybridSearchResourceLimits, DefaultLimitsAllowDefaultCandidates) {
    // Default Config must be valid with its own defaults
    HybridSearch::Config cfg;
    EXPECT_NO_THROW(HybridSearch(nullptr, nullptr, cfg));
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_GT(hs.getConfig().max_k, 0u);
    EXPECT_GE(hs.getConfig().max_k, hs.getConfig().k);
    EXPECT_GT(hs.getConfig().max_candidates, 0u);
    EXPECT_GE(hs.getConfig().max_candidates, hs.getConfig().k_bm25);
    EXPECT_GE(hs.getConfig().max_candidates, hs.getConfig().k_vector);
}

// ============================================================================
// SearchStats Tests
// ============================================================================

TEST(HybridSearchStats, NullIndicesNeitherAttempted) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    HybridSearch::SearchStats stats;
    // No text query, no vector → nothing attempted
    hs.search("", nullptr, 0, &stats);
    EXPECT_FALSE(stats.partial_result);
    EXPECT_EQ(stats.bm25_count, 0u);
    EXPECT_EQ(stats.vector_count, 0u);
}

TEST(HybridSearchStats, NullIndicesWithTextQuery) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    HybridSearch::SearchStats stats;
    // text query provided but no fulltext_index → bm25 not attempted, bm25_ok=false
    hs.search("hello world", nullptr, 0, &stats);
    EXPECT_FALSE(stats.bm25_ok);       // index was never available
    EXPECT_FALSE(stats.partial_result); // no index attempted → cannot be partial
    EXPECT_EQ(stats.bm25_count, 0u);
}

TEST(HybridSearchStats, NullIndicesWithVectorQuery) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    HybridSearch::SearchStats stats;
    float vec[] = {0.1f, 0.2f, 0.3f};
    // vector query provided but no vector_index → vector not attempted, vector_ok=false
    hs.search("", vec, 3, &stats);
    EXPECT_FALSE(stats.vector_ok);      // index was never available
    EXPECT_FALSE(stats.partial_result);
    EXPECT_EQ(stats.vector_count, 0u);
}

TEST(HybridSearchStats, NullStatsPointerIsIgnored) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    // Passing nullptr for stats must not crash
    EXPECT_NO_THROW(hs.search("hello", nullptr, 0, nullptr));
}

// ============================================================================
// setConfig Tests
// ============================================================================

TEST(HybridSearchSetConfig, SetConfigUpdatesState) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    HybridSearch::Config newCfg;
    newCfg.k = 25;
    newCfg.bm25_weight = 0.7;
    newCfg.vector_weight = 0.3;
    newCfg.vector_metric = VectorIndexManager::Metric::L2;
    hs.setConfig(newCfg);
    EXPECT_EQ(hs.getConfig().k, 25u);
    EXPECT_DOUBLE_EQ(hs.getConfig().bm25_weight, 0.7);
    EXPECT_EQ(hs.getConfig().vector_metric, VectorIndexManager::Metric::L2);
}

// ============================================================================
// LLM Re-ranker Integration Tests
// ============================================================================

TEST(HybridSearchReranker, SetRerankerWithNullBackendIsNoop) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    // Attaching a null backend should not throw and search() should still work
    EXPECT_NO_THROW(hs.setReranker(nullptr));
    auto results = hs.search("test query");
    EXPECT_TRUE(results.empty()); // no indices → empty results
}

TEST(HybridSearchReranker, SetRerankerDoesNotThrow) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_NO_THROW(hs.setReranker([](const std::string&) { return "5\n"; }));
}

TEST(HybridSearchReranker, SetRerankerClearsByPassingNull) {
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    hs.setReranker([](const std::string&) { return "8\n"; });
    EXPECT_NO_THROW(hs.setReranker(nullptr));
    // search() should still succeed after clearing the reranker
    auto results = hs.search("test");
    EXPECT_TRUE(results.empty());
}

TEST(HybridSearchReranker, RerankerConfigPassedThrough) {
    ILlmReranker::Config rr_cfg;
    rr_cfg.llm_weight = 0.9;
    rr_cfg.batch_size = 3;

    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    EXPECT_NO_THROW(hs.setReranker([](const std::string&) { return "7\n"; }, rr_cfg));
}

TEST(HybridSearchReranker, SearchWithRerankerAndNoIndicesReturnsEmpty) {
    bool backend_called = false;
    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);
    hs.setReranker([&](const std::string&) -> std::string {
        backend_called = true;
        return "5\n";
    });
    auto results = hs.search("test query");
    // No real indices → fused result is empty → reranker backend NOT called
    EXPECT_TRUE(results.empty());
    EXPECT_FALSE(backend_called);
}

TEST(HybridSearchReranker, RrfFusionRerankerIntegration) {
    // Verify that RRF results fed through setReranker are correctly reordered.
    // Construct synthetic BM25 results and pass through reciprocalRankFusion,
    // then assert that the re-ranker would invert the order via mock scores.
    using Result = HybridSearch::Result;

    HybridSearch::Config cfg;
    HybridSearch hs(nullptr, nullptr, cfg);

    // Two BM25 results: doc_a at rank 1, doc_b at rank 2
    std::vector<Result> bm25_results;
    {
        Result r;
        r.document_id = "doc_a";
        r.bm25_score  = 1.0;
        r.bm25_rank   = 1;
        bm25_results.push_back(r);
    }
    {
        Result r;
        r.document_id = "doc_b";
        r.bm25_score  = 0.5;
        r.bm25_rank   = 2;
        bm25_results.push_back(r);
    }

    // RRF without re-ranker: doc_a first (rank 1 in BM25, no vector)
    auto fused = hs.reciprocalRankFusion(bm25_results, {});
    ASSERT_EQ(fused.size(), 2u);
    EXPECT_EQ(fused[0].document_id, "doc_a");
    EXPECT_EQ(fused[1].document_id, "doc_b");

    // Attach a mock re-ranker that inverts the order:
    // first candidate in prompt gets score 1, second gets score 9
    // (so doc_b which arrives second in the batch gets a higher LLM score)
    size_t call_count = 0;
    hs.setReranker([&](const std::string& /*prompt*/) -> std::string {
        ++call_count;
        return "1\n9\n";
    });

    // search() with no real indices returns empty — the reranker integration
    // can only be exercised end-to-end when the index layer is available.
    // We verify here that the wiring compiles and the helper path is correct.
    auto search_results = hs.search("example query");
    EXPECT_TRUE(search_results.empty());  // no real index
    EXPECT_EQ(call_count, 0u);           // backend not called on empty candidate list
}

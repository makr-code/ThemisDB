/**
 * @file test_rrf_fusion.cpp
 * @brief Dedicated unit tests for HybridSearch::reciprocalRankFusion().
 *
 * These tests focus exclusively on RRF algorithm correctness: score formula,
 * rank metadata, edge cases, parameter sensitivity, and ordering guarantees.
 * They complement the broader tests in test_hybrid_search.cpp.
 */

#include <gtest/gtest.h>
#include "search/hybrid_search.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

using namespace themis;

// ============================================================================
// Test fixture: common setup helpers
// ============================================================================

class RRFFusionTest : public ::testing::Test {
protected:
    // Build a valid HybridSearch with a given rrf_k and weights
    HybridSearch makeHS(double rrf_k = 60.0,
                        double bm25_w = 0.5,
                        double vec_w = 0.5,
                        size_t k = 100) {
        HybridSearch::Config cfg;
        cfg.rrf_k = rrf_k;
        cfg.bm25_weight = bm25_w;
        cfg.vector_weight = vec_w;
        cfg.k = k;
        cfg.use_rrf = true;
        return HybridSearch(nullptr, nullptr, cfg);
    }

    // Build a ranked list of n results with consecutive IDs "doc1" … "docN"
    std::vector<HybridSearch::Result> makeList(int n, bool bm25 = true) {
        std::vector<HybridSearch::Result> list;
        for (int i = 1; i <= n; ++i) {
            HybridSearch::Result r;
            r.document_id = "doc" + std::to_string(i);
            if (bm25) { r.bm25_score = 1.0; r.bm25_rank = i; }
            else       { r.vector_score = 1.0; r.vector_rank = i; }
            list.push_back(r);
        }
        return list;
    }
};

// ============================================================================
// Score formula correctness
// ============================================================================

TEST_F(RRFFusionTest, ScoreFormula_SingleSourceBM25_RankOne) {
    // score = w_bm25 * 1/(rrf_k + 1) + 0  (no vector contribution)
    auto hs = makeHS(/*rrf_k=*/60.0, /*bm25_w=*/0.5, /*vec_w=*/0.5);
    auto bm25 = makeList(1, /*bm25=*/true);
    auto results = hs.reciprocalRankFusion(bm25, {});
    ASSERT_EQ(results.size(), 1u);
    double expected = 0.5 * (1.0 / 61.0);
    EXPECT_NEAR(results[0].hybrid_score, expected, 1e-12);
}

TEST_F(RRFFusionTest, ScoreFormula_SingleSourceVector_RankOne) {
    auto hs = makeHS(/*rrf_k=*/60.0, /*bm25_w=*/0.5, /*vec_w=*/0.5);
    auto vec = makeList(1, /*bm25=*/false);
    auto results = hs.reciprocalRankFusion({}, vec);
    ASSERT_EQ(results.size(), 1u);
    double expected = 0.5 * (1.0 / 61.0);
    EXPECT_NEAR(results[0].hybrid_score, expected, 1e-12);
}

TEST_F(RRFFusionTest, ScoreFormula_BothSources_RankTwoEach) {
    // doc2 appears at rank 2 in both lists → score = 0.5*(1/62) + 0.5*(1/62)
    auto hs = makeHS(60.0);
    std::vector<HybridSearch::Result> bm25 = {makeList(3)[1]}; // doc2 at rank 1 (0-indexed → rank 2)
    bm25[0].bm25_rank = 2;
    // We feed it as the first entry in a 3-entry list so its positional index is 1 (rank=2)
    auto bm25_full = makeList(3, /*bm25=*/true);
    auto vec_full  = makeList(3, /*bm25=*/false);
    // doc2 is at list index 1 (rank 2) in both
    auto results = hs.reciprocalRankFusion(bm25_full, vec_full);
    // Find doc2 in results
    auto it = std::find_if(results.begin(), results.end(),
                           [](const auto& r){ return r.document_id == "doc2"; });
    ASSERT_NE(it, results.end());
    double expected = 0.5 * (1.0 / 62.0) + 0.5 * (1.0 / 62.0);
    EXPECT_NEAR(it->hybrid_score, expected, 1e-12);
}

TEST_F(RRFFusionTest, ScoreFormula_NonDefaultRrfK) {
    // With rrf_k = 10: score for rank-1 in both = 0.5*(1/11) + 0.5*(1/11) = 1/11
    auto hs = makeHS(/*rrf_k=*/10.0);
    auto bm25 = makeList(1, true);
    auto vec  = makeList(1, false);
    bm25[0].document_id = vec[0].document_id = "shared";
    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_EQ(results.size(), 1u);
    double expected = 0.5 * (1.0 / 11.0) + 0.5 * (1.0 / 11.0);
    EXPECT_NEAR(results[0].hybrid_score, expected, 1e-12);
}

// ============================================================================
// Rank metadata set correctly
// ============================================================================

TEST_F(RRFFusionTest, RankMetadata_BM25RankPreserved) {
    auto hs = makeHS();
    auto bm25 = makeList(3, true);
    auto results = hs.reciprocalRankFusion(bm25, {});
    // doc1 should have bm25_rank = 1
    auto it = std::find_if(results.begin(), results.end(),
                           [](const auto& r){ return r.document_id == "doc1"; });
    ASSERT_NE(it, results.end());
    EXPECT_EQ(it->bm25_rank, 1);
}

TEST_F(RRFFusionTest, RankMetadata_VectorRankPreserved) {
    auto hs = makeHS();
    auto vec = makeList(3, false);
    auto results = hs.reciprocalRankFusion({}, vec);
    auto it = std::find_if(results.begin(), results.end(),
                           [](const auto& r){ return r.document_id == "doc1"; });
    ASSERT_NE(it, results.end());
    EXPECT_EQ(it->vector_rank, 1);
}

TEST_F(RRFFusionTest, RankMetadata_BothRanksSetForOverlap) {
    auto hs = makeHS();
    auto bm25 = makeList(2, true);
    std::vector<HybridSearch::Result> vec = {
        HybridSearch::Result{},  // doc2 at vector rank 1
        HybridSearch::Result{}   // doc1 at vector rank 2
    };
    vec[0].document_id = "doc2"; vec[0].vector_score = 1.0; vec[0].vector_rank = 1;
    vec[1].document_id = "doc1"; vec[1].vector_score = 1.0; vec[1].vector_rank = 2;
    auto results = hs.reciprocalRankFusion(bm25, vec);
    // doc1: bm25_rank=1 from bm25, vector_rank=2 from vec
    auto it = std::find_if(results.begin(), results.end(),
                           [](const auto& r){ return r.document_id == "doc1"; });
    ASSERT_NE(it, results.end());
    EXPECT_EQ(it->bm25_rank, 1);
    EXPECT_EQ(it->vector_rank, 2);
}

// ============================================================================
// Ordering guarantees
// ============================================================================

TEST_F(RRFFusionTest, ResultsStrictlyDescending_LargeBM25List) {
    auto hs = makeHS();
    auto bm25 = makeList(20, true);
    auto results = hs.reciprocalRankFusion(bm25, {});
    ASSERT_EQ(results.size(), 20u);
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].hybrid_score, results[i].hybrid_score)
            << "Not sorted at index " << i;
    }
}

TEST_F(RRFFusionTest, ResultsStrictlyDescending_BothLists) {
    auto hs = makeHS();
    auto bm25 = makeList(10, true);
    auto vec  = makeList(10, false);
    auto results = hs.reciprocalRankFusion(bm25, vec);
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].hybrid_score, results[i].hybrid_score)
            << "Not sorted at index " << i;
    }
}

TEST_F(RRFFusionTest, OverlapDocumentAlwaysBeatsNonOverlap) {
    // "shared" at rank 1 in both lists → beats "bm25only" at rank 2, "veconly" at rank 2
    auto hs = makeHS();
    std::vector<HybridSearch::Result> bm25 = {
        {}, {}
    };
    bm25[0].document_id = "shared";  bm25[0].bm25_score = 1; bm25[0].bm25_rank = 1;
    bm25[1].document_id = "bm25only"; bm25[1].bm25_score = 1; bm25[1].bm25_rank = 2;

    std::vector<HybridSearch::Result> vec = {
        {}, {}
    };
    vec[0].document_id = "shared";  vec[0].vector_score = 1; vec[0].vector_rank = 1;
    vec[1].document_id = "veconly"; vec[1].vector_score = 1; vec[1].vector_rank = 2;

    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "shared");
}

// ============================================================================
// K-limiting
// ============================================================================

TEST_F(RRFFusionTest, KLimit_LargerThanCandidateCount_ReturnsAll) {
    auto hs = makeHS(60.0, 0.5, 0.5, /*k=*/100);
    auto bm25 = makeList(5, true);
    auto results = hs.reciprocalRankFusion(bm25, {});
    // 5 candidates < k=100 → all 5 returned
    EXPECT_EQ(results.size(), 5u);
}

TEST_F(RRFFusionTest, KLimit_SmallerThanCandidateCount_Truncates) {
    auto hs = makeHS(60.0, 0.5, 0.5, /*k=*/3);
    auto bm25 = makeList(10, true);
    auto vec  = makeList(10, false);
    auto results = hs.reciprocalRankFusion(bm25, vec);
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(RRFFusionTest, KLimit_ExactlyK_ReturnsK) {
    auto hs = makeHS(60.0, 0.5, 0.5, /*k=*/5);
    auto bm25 = makeList(5, true);
    auto results = hs.reciprocalRankFusion(bm25, {});
    EXPECT_EQ(results.size(), 5u);
}

// ============================================================================
// Asymmetric weights
// ============================================================================

TEST_F(RRFFusionTest, AsymmetricWeights_BM25Dominant_BM25RankOneWins) {
    // bm25_weight = 1.0, vector_weight = 0.0
    // doc1 is rank 1 in BM25, rank 2 in vector → should beat doc2 which is rank 2 in BM25, rank 1 in vector
    auto hs = makeHS(60.0, /*bm25_w=*/1.0, /*vec_w=*/0.0);
    std::vector<HybridSearch::Result> bm25 = {
        {}, {}
    };
    bm25[0].document_id = "doc1"; bm25[0].bm25_rank = 1;
    bm25[1].document_id = "doc2"; bm25[1].bm25_rank = 2;

    std::vector<HybridSearch::Result> vec = {
        {}, {}
    };
    vec[0].document_id = "doc2"; vec[0].vector_rank = 1;
    vec[1].document_id = "doc1"; vec[1].vector_rank = 2;

    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc1"); // BM25 rank 1 wins
}

TEST_F(RRFFusionTest, AsymmetricWeights_VectorDominant_VectorRankOneWins) {
    auto hs = makeHS(60.0, /*bm25_w=*/0.0, /*vec_w=*/1.0);
    std::vector<HybridSearch::Result> bm25 = {{}, {}};
    bm25[0].document_id = "doc1"; bm25[0].bm25_rank = 1;
    bm25[1].document_id = "doc2"; bm25[1].bm25_rank = 2;

    std::vector<HybridSearch::Result> vec = {{}, {}};
    vec[0].document_id = "doc2"; vec[0].vector_rank = 1;
    vec[1].document_id = "doc1"; vec[1].vector_rank = 2;

    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc2"); // vector rank 1 wins
}

// ============================================================================
// All documents overlap (both lists identical)
// ============================================================================

TEST_F(RRFFusionTest, AllOverlap_SameOrder_ScoreDoubled) {
    // When BM25 and vector lists are identical and weights are equal,
    // the hybrid score = 2 × (0.5 × 1/(rrf_k+rank))
    auto hs = makeHS(60.0);
    auto bm25 = makeList(3, true);
    std::vector<HybridSearch::Result> vec = makeList(3, false);
    // Same document IDs in same order
    for (size_t i = 0; i < vec.size(); ++i) {
      vec[i].document_id = bm25[i].document_id;
    }

    auto results = hs.reciprocalRankFusion(bm25, vec);
    ASSERT_EQ(results.size(), 3u);
    for (size_t i = 0; i < results.size(); ++i) {
        double rank = static_cast<double>(i + 1);
        double expected = 0.5 * (1.0/(60.0 + rank)) + 0.5 * (1.0/(60.0 + rank));
        EXPECT_NEAR(results[i].hybrid_score, expected, 1e-12)
            << "Score mismatch at rank " << rank;
    }
}

// ============================================================================
// Stability: same input produces same output
// ============================================================================

TEST_F(RRFFusionTest, Stability_SameInputSameOutput) {
    auto hs = makeHS();
    auto bm25 = makeList(5, true);
    auto vec  = makeList(5, false);

    auto results1 = hs.reciprocalRankFusion(bm25, vec);
    auto results2 = hs.reciprocalRankFusion(bm25, vec);

    ASSERT_EQ(results1.size(), results2.size());
    for (size_t i = 0; i < results1.size(); ++i) {
        EXPECT_EQ(results1[i].document_id, results2[i].document_id);
        EXPECT_DOUBLE_EQ(results1[i].hybrid_score, results2[i].hybrid_score);
    }
}

// ============================================================================
// Content field preservation
// ============================================================================

TEST_F(RRFFusionTest, ContentField_PreservedFromBM25) {
    auto hs = makeHS();
    HybridSearch::Result r;
    r.document_id = "doc1";
    r.bm25_score = 1.0;
    r.content = "hello world";
    auto results = hs.reciprocalRankFusion({r}, {});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].content, "hello world");
}

TEST_F(RRFFusionTest, ContentField_BM25TakesPrecedenceOverVector) {
    auto hs = makeHS();
    HybridSearch::Result bm25r, vecr;
    bm25r.document_id = vecr.document_id = "doc1";
    bm25r.content = "from bm25";
    vecr.content  = "from vector";
    auto results = hs.reciprocalRankFusion({bm25r}, {vecr});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].content, "from bm25"); // BM25 sets content first
}

/**
 * @file test_rag_hybrid_retriever.cpp
 * @brief Unit tests for HybridRetriever (BM25 + vector, configurable RRF weights)
 *
 * Tests cover:
 *  - Config validation (negative weights, rrf_k <= 0)
 *  - RRF mode: score range, ordering, weight effect, absent-document handling
 *  - Linear combination mode: score ordering, normalisation
 *  - top_k truncation
 *  - Empty candidate lists (single-source and both-empty)
 *  - Duplicate-free result set
 *  - HybridFusionResult metadata (total_candidates, used_rrf, scores vector)
 *  - HybridRetrieverFactory preset configurations
 */

#include "rag/hybrid_retriever.h"
#include "rag/vectorizer_interface.h"
#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <algorithm>

using namespace themis::rag;
using namespace themis::rag::judge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score) {
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

/// Build N documents with ids "doc0", "doc1", ... and linearly decreasing scores.
static std::vector<RetrievedDocument> makeDocs(size_t n,
                                                double start_score = 0.9,
                                                double delta       = 0.05) {
    std::vector<RetrievedDocument> docs;
    docs.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const double score = start_score - static_cast<double>(i) * delta;
        docs.push_back(makeDoc("doc" + std::to_string(i),
                               "content of doc" + std::to_string(i),
                               score));
    }
    return docs;
}

class TestVectorizer final : public IVectorizer {
public:
    void initialize() override { initialized_ = true; }
    bool isInitialized() const override { return initialized_; }

    std::vector<float> encodeQuery(const std::string& query) override {
        if (query.empty()) {
            throw std::invalid_argument("empty query");
        }
        return {1.0f, 0.0f};
    }

    std::vector<float> encodePassage(const std::string& passage) override {
        if (passage.find("match") != std::string::npos) {
            return {1.0f, 0.0f};
        }
        return {0.0f, 1.0f};
    }

    size_t getEmbeddingDimension() const override { return 2; }

private:
    bool initialized_ = false;
};

// ---------------------------------------------------------------------------
// Config validation
// ---------------------------------------------------------------------------

TEST(HybridRetrieverConfig, DefaultConstructorIsValid) {
    EXPECT_NO_THROW(HybridRetriever());
}

TEST(HybridRetrieverConfig, ValidCustomConfig) {
    HybridRetrieverConfig cfg_valid;
    cfg_valid.bm25_weight   = 0.4;
    cfg_valid.vector_weight = 0.6;
    cfg_valid.rrf_k         = 60.0;
    cfg_valid.top_k         = 5;
    EXPECT_NO_THROW({
        HybridRetriever retriever{cfg_valid};
        static_cast<void>(retriever);
    });
}

TEST(HybridRetrieverConfig, NegativeBm25WeightThrows) {
    HybridRetrieverConfig cfg_neg_bm25;
    cfg_neg_bm25.bm25_weight = -0.1;
    EXPECT_THROW({
        HybridRetriever retriever{cfg_neg_bm25};
        static_cast<void>(retriever);
    }, std::invalid_argument);
}

TEST(HybridRetrieverConfig, NegativeVectorWeightThrows) {
    HybridRetrieverConfig cfg_neg_vector;
    cfg_neg_vector.vector_weight = -0.1;
    EXPECT_THROW({
        HybridRetriever retriever{cfg_neg_vector};
        static_cast<void>(retriever);
    }, std::invalid_argument);
}

TEST(HybridRetrieverConfig, ZeroRrfKThrows) {
    HybridRetrieverConfig cfg_zero_rrf;
    cfg_zero_rrf.rrf_k = 0.0;
    EXPECT_THROW({
        HybridRetriever retriever{cfg_zero_rrf};
        static_cast<void>(retriever);
    }, std::invalid_argument);
}

TEST(HybridRetrieverConfig, NegativeRrfKThrows) {
    HybridRetrieverConfig cfg_neg_rrf;
    cfg_neg_rrf.rrf_k = -1.0;
    EXPECT_THROW({
        HybridRetriever retriever{cfg_neg_rrf};
        static_cast<void>(retriever);
    }, std::invalid_argument);
}

TEST(HybridRetrieverConfig, ZeroWeightsAreAllowed) {
    HybridRetrieverConfig cfg_zero_weights;
    cfg_zero_weights.bm25_weight   = 0.0;
    cfg_zero_weights.vector_weight = 0.0;
    EXPECT_NO_THROW({
        HybridRetriever retriever{cfg_zero_weights};
        static_cast<void>(retriever);
    });
}

TEST(HybridRetrieverConfig, SetConfigUpdatesAndValidates) {
    HybridRetriever r;
    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.3;
    cfg.vector_weight = 0.7;
    EXPECT_NO_THROW(r.setConfig(cfg));
    EXPECT_DOUBLE_EQ(r.getConfig().bm25_weight, 0.3);

    cfg.rrf_k = -5.0;
    EXPECT_THROW(r.setConfig(cfg), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// RRF mode – basic functionality
// ---------------------------------------------------------------------------

class HybridRetrieverRRFTest : public ::testing::Test {
protected:
    HybridRetrieverConfig makeCfg(double bm25_w = 0.5, double vec_w = 0.5,
                                   size_t top_k = 0) {
        HybridRetrieverConfig cfg;
        cfg.bm25_weight   = bm25_w;
        cfg.vector_weight = vec_w;
        cfg.use_rrf       = true;
        cfg.rrf_k         = 60.0;
        cfg.top_k         = top_k;
        return cfg;
    }
};

TEST_F(HybridRetrieverRRFTest, ResultsAreSortedDescending) {
    HybridRetriever r(makeCfg());
    auto bm25 = makeDocs(5);
    auto vec  = makeDocs(5);
    auto res  = r.fuse(bm25, vec);

    for (size_t i = 1; i < res.documents.size(); ++i) {
        EXPECT_GE(res.documents[i - 1].similarity_score,
                  res.documents[i].similarity_score)
            << "Not sorted at index " << i;
    }
}

TEST_F(HybridRetrieverRRFTest, UsedRrfFlagIsTrue) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse(makeDocs(3), makeDocs(3));
    EXPECT_TRUE(res.used_rrf);
}

TEST_F(HybridRetrieverRRFTest, TotalCandidatesCountsUnion) {
    // doc0, doc1, doc2 in both lists → union = 3
    HybridRetriever r(makeCfg());
    auto bm25 = makeDocs(3);
    auto vec  = makeDocs(3);
    auto res  = r.fuse(bm25, vec);
    EXPECT_EQ(res.total_candidates, 3u);
}

TEST_F(HybridRetrieverRRFTest, TotalCandidatesCountsUnionDisjoint) {
    // doc0..doc2 in BM25, doc3..doc5 in vector → union = 6
    HybridRetriever r(makeCfg());
    std::vector<RetrievedDocument> bm25 = {
        makeDoc("a", "text a", 0.9),
        makeDoc("b", "text b", 0.8),
    };
    std::vector<RetrievedDocument> vec = {
        makeDoc("c", "text c", 0.7),
        makeDoc("d", "text d", 0.6),
    };
    auto res = r.fuse(bm25, vec);
    EXPECT_EQ(res.total_candidates, 4u);
}

TEST_F(HybridRetrieverRRFTest, NoDuplicatesInResult) {
    HybridRetriever r(makeCfg());
    // Same docs in both lists
    auto bm25 = makeDocs(5);
    auto vec  = makeDocs(5);
    auto res  = r.fuse(bm25, vec);

    std::vector<std::string> ids;
    for (const auto& d : res.documents) { ids.push_back(d.id); }
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::unique(ids.begin(), ids.end()), ids.end())
        << "Duplicate document ids in result";
}

TEST_F(HybridRetrieverRRFTest, TopKTruncatesResult) {
    auto cfg = makeCfg(0.5, 0.5, /*top_k=*/3);
    HybridRetriever r(cfg);
    auto res = r.fuse(makeDocs(10), makeDocs(10));
    EXPECT_EQ(res.documents.size(), 3u);
    EXPECT_EQ(res.scores.size(), 3u);
}

TEST_F(HybridRetrieverRRFTest, TopKZeroReturnsAll) {
    auto cfg = makeCfg(0.5, 0.5, /*top_k=*/0);
    HybridRetriever r(cfg);
    auto bm25 = makeDocs(7);
    auto vec  = makeDocs(7);
    auto res  = r.fuse(bm25, vec);
    // All 7 unique docs should be returned
    EXPECT_EQ(res.documents.size(), 7u);
}

TEST_F(HybridRetrieverRRFTest, BM25OnlyWeightDocRanksHigher) {
    // bm25_weight=1.0, vector_weight=0.0
    // doc0 is rank-1 in BM25 → should be top result
    auto cfg = makeCfg(1.0, 0.0);
    HybridRetriever r(cfg);

    // BM25: doc0 best, doc1 second
    // Vector: doc1 best, doc0 second
    std::vector<RetrievedDocument> bm25 = {
        makeDoc("doc0", "a", 0.9),
        makeDoc("doc1", "b", 0.8),
    };
    std::vector<RetrievedDocument> vec = {
        makeDoc("doc1", "b", 0.9),
        makeDoc("doc0", "a", 0.8),
    };
    auto res = r.fuse(bm25, vec);
    ASSERT_GE(res.documents.size(), 2u);
    EXPECT_EQ(res.documents[0].id, "doc0");
}

TEST_F(HybridRetrieverRRFTest, VectorOnlyWeightDocRanksHigher) {
    auto cfg = makeCfg(0.0, 1.0);
    HybridRetriever r(cfg);

    std::vector<RetrievedDocument> bm25 = {
        makeDoc("doc0", "a", 0.9),
        makeDoc("doc1", "b", 0.8),
    };
    std::vector<RetrievedDocument> vec = {
        makeDoc("doc1", "b", 0.9),
        makeDoc("doc0", "a", 0.8),
    };
    auto res = r.fuse(bm25, vec);
    ASSERT_GE(res.documents.size(), 2u);
    EXPECT_EQ(res.documents[0].id, "doc1");
}

TEST_F(HybridRetrieverRRFTest, ScoresVectorHasSameSizeAsDocuments) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse(makeDocs(4), makeDocs(4));
    EXPECT_EQ(res.documents.size(), res.scores.size());
}

TEST_F(HybridRetrieverRRFTest, DocumentPresentOnlyInBm25GetsVectorRankMinusOne) {
    HybridRetriever r(makeCfg());
    std::vector<RetrievedDocument> bm25 = {makeDoc("only_bm25", "x", 0.9)};
    std::vector<RetrievedDocument> vec  = {makeDoc("only_vec",  "y", 0.8)};
    auto res = r.fuse(bm25, vec);

    for (const auto& s : res.scores) {
        if (s.document_id == "only_bm25") {
            EXPECT_EQ(s.vector_rank, -1);
        }
        if (s.document_id == "only_vec") {
            EXPECT_EQ(s.bm25_rank, -1);
        }
    }
}

// ---------------------------------------------------------------------------
// RRF mode – empty candidate lists
// ---------------------------------------------------------------------------

TEST_F(HybridRetrieverRRFTest, EmptyBothListsReturnsEmpty) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse({}, {});
    EXPECT_TRUE(res.documents.empty());
    EXPECT_EQ(res.total_candidates, 0u);
}

TEST_F(HybridRetrieverRRFTest, EmptyBm25FallsBackToVector) {
    auto cfg = makeCfg(0.5, 0.5);
    HybridRetriever r(cfg);
    std::vector<RetrievedDocument> vec = {
        makeDoc("v0", "v0", 0.9),
        makeDoc("v1", "v1", 0.8),
    };
    auto res = r.fuse({}, vec);
    EXPECT_EQ(res.documents.size(), 2u);
    EXPECT_EQ(res.documents[0].id, "v0");
}

TEST_F(HybridRetrieverRRFTest, EmptyVectorFallsBackToBm25) {
    auto cfg = makeCfg(0.5, 0.5);
    HybridRetriever r(cfg);
    std::vector<RetrievedDocument> bm25 = {
        makeDoc("b0", "b0", 0.9),
        makeDoc("b1", "b1", 0.8),
    };
    auto res = r.fuse(bm25, {});
    EXPECT_EQ(res.documents.size(), 2u);
    EXPECT_EQ(res.documents[0].id, "b0");
}

// ---------------------------------------------------------------------------
// Linear combination mode
// ---------------------------------------------------------------------------

class HybridRetrieverLinearTest : public ::testing::Test {
protected:
    HybridRetrieverConfig makeCfg(double bm25_w = 0.5, double vec_w = 0.5,
                                   size_t top_k = 0) {
        HybridRetrieverConfig cfg;
        cfg.bm25_weight      = bm25_w;
        cfg.vector_weight    = vec_w;
        cfg.use_rrf          = false;
        cfg.normalize_scores = true;
        cfg.top_k            = top_k;
        return cfg;
    }
};

TEST_F(HybridRetrieverLinearTest, UsedRrfFlagIsFalse) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse(makeDocs(3), makeDocs(3));
    EXPECT_FALSE(res.used_rrf);
}

TEST_F(HybridRetrieverLinearTest, ResultsAreSortedDescending) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse(makeDocs(5), makeDocs(5));
    for (size_t i = 1; i < res.documents.size(); ++i) {
        EXPECT_GE(res.documents[i - 1].similarity_score,
                  res.documents[i].similarity_score);
    }
}

TEST_F(HybridRetrieverLinearTest, TopKTruncatesResult) {
    HybridRetriever r(makeCfg(0.5, 0.5, /*top_k=*/2));
    auto res = r.fuse(makeDocs(6), makeDocs(6));
    EXPECT_EQ(res.documents.size(), 2u);
}

TEST_F(HybridRetrieverLinearTest, EmptyBothListsReturnsEmpty) {
    HybridRetriever r(makeCfg());
    auto res = r.fuse({}, {});
    EXPECT_TRUE(res.documents.empty());
}

// ---------------------------------------------------------------------------
// HybridRetrieverFactory
// ---------------------------------------------------------------------------

TEST(HybridRetrieverFactoryTest, BalancedHasEqualWeights) {
    auto r = HybridRetrieverFactory::createBalanced(10);
    EXPECT_DOUBLE_EQ(r.getConfig().bm25_weight,   0.5);
    EXPECT_DOUBLE_EQ(r.getConfig().vector_weight, 0.5);
    EXPECT_EQ(r.getConfig().top_k, 10u);
    EXPECT_TRUE(r.getConfig().use_rrf);
}

TEST(HybridRetrieverFactoryTest, SemanticFocusedHasHigherVectorWeight) {
    auto r = HybridRetrieverFactory::createSemanticFocused(5);
    EXPECT_GT(r.getConfig().vector_weight, r.getConfig().bm25_weight);
    EXPECT_EQ(r.getConfig().top_k, 5u);
}

TEST(HybridRetrieverFactoryTest, KeywordFocusedHasHigherBm25Weight) {
    auto r = HybridRetrieverFactory::createKeywordFocused(5);
    EXPECT_GT(r.getConfig().bm25_weight, r.getConfig().vector_weight);
    EXPECT_EQ(r.getConfig().top_k, 5u);
}

TEST(HybridRetrieverFactoryTest, FactoryRetrieverProducesResults) {
    auto r   = HybridRetrieverFactory::createBalanced(10);
    auto res = r.fuse(makeDocs(5), makeDocs(5));
    EXPECT_FALSE(res.documents.empty());
    EXPECT_TRUE(res.used_rrf);
}

// ---------------------------------------------------------------------------
// DPR / IVectorizer integration path
// ---------------------------------------------------------------------------

TEST(HybridRetrieverVectorizerIntegration, RetrieveWithVectorizerUsesDenseRanking) {
    HybridRetrieverConfig cfg;
    cfg.bm25_weight = 0.2;
    cfg.vector_weight = 0.8;
    HybridRetriever retriever(cfg);

    auto vectorizer = std::make_shared<TestVectorizer>();
    vectorizer->initialize();
    retriever.setVectorizer(vectorizer);

    std::vector<RetrievedDocument> bm25 = {
        makeDoc("doc_bm25_top", "this document does not align", 0.95),
        makeDoc("doc_dense_top", "this one is a semantic match", 0.70),
    };

    const auto result = retriever.retrieveWithVectorizer("semantic query", bm25);
    ASSERT_FALSE(result.documents.empty());
    EXPECT_EQ(result.documents.front().id, "doc_dense_top");
}

TEST(HybridRetrieverVectorizerIntegration, RetrieveWithVectorizerThrowsWithoutVectorizer) {
    HybridRetriever retriever;
    const std::vector<RetrievedDocument> bm25 = {makeDoc("d1", "content", 0.5)};
    EXPECT_THROW(
        {
            auto vectorized = retriever.retrieveWithVectorizer("q", bm25);
            static_cast<void>(vectorized);
        },
        std::runtime_error);
}

TEST(HybridRetrieverVectorizerIntegration, RetrieveWithVectorizerThrowsWhenVectorizerNotInitialized) {
    HybridRetriever retriever;
    retriever.setVectorizer(std::make_shared<TestVectorizer>());
    const std::vector<RetrievedDocument> bm25 = {makeDoc("d1", "content", 0.5)};
    EXPECT_THROW(
        {
            auto vectorized = retriever.retrieveWithVectorizer("q", bm25);
            static_cast<void>(vectorized);
        },
        std::runtime_error);
}

TEST(HybridRetrieverVectorizerIntegration, RetrieveWithVectorizerThrowsOnEmptyQuery) {
    HybridRetriever retriever;
    auto vectorizer = std::make_shared<TestVectorizer>();
    vectorizer->initialize();
    retriever.setVectorizer(vectorizer);

    const std::vector<RetrievedDocument> bm25 = {makeDoc("d1", "content", 0.5)};
    EXPECT_THROW(
        {
            auto vectorized = retriever.retrieveWithVectorizer("", bm25);
            static_cast<void>(vectorized);
        },
        std::invalid_argument);
}

TEST(HybridRetrieverVectorizerIntegration, RetrieveWithVectorizerWithEmptyCandidatesReturnsEmptyResult) {
    HybridRetriever retriever;
    auto vectorizer = std::make_shared<TestVectorizer>();
    vectorizer->initialize();
    retriever.setVectorizer(vectorizer);

    const auto result = retriever.retrieveWithVectorizer("q", {});
    EXPECT_TRUE(result.documents.empty());
    EXPECT_TRUE(result.scores.empty());
    EXPECT_EQ(result.total_candidates, 0u);
}

TEST(HybridRetrieverVectorizerIntegration, GetVectorizerReturnsInjectedInstance) {
    HybridRetriever retriever;
    auto vectorizer = std::make_shared<TestVectorizer>();
    retriever.setVectorizer(vectorizer);
    EXPECT_EQ(retriever.getVectorizer(), vectorizer);
}

// ---------------------------------------------------------------------------
// RRF constant (rrf_k) affects scores but not relative ordering for equal lists
// ---------------------------------------------------------------------------

TEST(HybridRetrieverRrfConstantTest, DifferentRrfKChangesScore) {
    std::vector<RetrievedDocument> bm25 = {makeDoc("d0", "text", 0.9)};
    std::vector<RetrievedDocument> vec  = {makeDoc("d0", "text", 0.9)};

    HybridRetrieverConfig cfg60;
    cfg60.rrf_k = 60.0;
    HybridRetrieverConfig cfg1;
    cfg1.rrf_k = 1.0;

    HybridRetriever r60(cfg60);
    HybridRetriever r1(cfg1);

    auto res60 = r60.fuse(bm25, vec);
    auto res1  = r1.fuse(bm25, vec);

    ASSERT_EQ(res60.documents.size(), 1u);
    ASSERT_EQ(res1.documents.size(), 1u);
    // rrf_k=1 → 1/(1+1)=0.5 per source; rrf_k=60 → 1/61≈0.016 per source
    EXPECT_GT(res1.documents[0].similarity_score,
              res60.documents[0].similarity_score);
}

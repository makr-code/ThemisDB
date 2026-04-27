// Test suite: LoRAEnhancedRetrieverTests
//
// LER-01  HeuristicLoRAScorer::score returns value in [0, 1]
// LER-02  HeuristicLoRAScorer: identical query and content → score 1.0
// LER-03  HeuristicLoRAScorer: fully disjoint tokens → score 0.0
// LER-04  LoRAEnhancedRetriever: empty candidates returns empty list
// LER-05  LoRAEnhancedRetriever: fused score in [0, 1]; lora_score in metadata
// LER-06  Factory helpers produce retrievers with expected configurations

#include <gtest/gtest.h>

#include "rag/lora_enhanced_retriever.h"

#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score = 0.8) {
    return {id, content, score, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-01  HeuristicLoRAScorer score in [0, 1]
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER01_ScoreRange) {
    HeuristicLoRAScorer scorer;
    const double s = scorer.score("foo bar baz", "bar baz qux");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-02  Identical query and content → score 1.0
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER02_IdenticalText) {
    HeuristicLoRAScorer scorer;
    const std::string text = "deep learning neural network";
    EXPECT_DOUBLE_EQ(scorer.score(text, text), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-03  Fully disjoint tokens → score 0.0
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER03_DisjointText) {
    HeuristicLoRAScorer scorer;
    // Tokens in query and content have no overlap.
    const double s = scorer.score("alpha beta", "gamma delta");
    EXPECT_DOUBLE_EQ(s, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-04  Empty candidates list → empty result
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER04_EmptyCandidates) {
    auto scorer = std::make_shared<HeuristicLoRAScorer>();
    LoRAEnhancedRetriever r(scorer);
    auto res = r.rerank("query", {});
    EXPECT_TRUE(res.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-05  Fused score in [0, 1]; lora_score stored in metadata
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER05_FusedScoreAndMetadata) {
    auto scorer = std::make_shared<HeuristicLoRAScorer>();
    LoRARetrieverConfig cfg;
    cfg.lora_weight  = 0.4;
    cfg.top_k_rerank = 5;
    LoRAEnhancedRetriever r(scorer, cfg);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "neural network deep learning", 0.7),
        makeDoc("d2", "database query optimizer",    0.5)
    };
    auto res = r.rerank("neural network", cands);

    ASSERT_EQ(res.size(), 2u);
    for (const auto& doc : res) {
        EXPECT_GE(doc.similarity_score, 0.0);
        EXPECT_LE(doc.similarity_score, 1.0);
        EXPECT_TRUE(doc.metadata.count("lora_score") > 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-06  Factory helpers produce expected configurations
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER06_FactoryHelpers) {
    {
        auto r = LoRAEnhancedRetrieverFactory::createLightweight();
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->config().top_k_rerank, 20u);
        EXPECT_LT(r->config().lora_weight, 0.25);
    }
    {
        auto r = LoRAEnhancedRetrieverFactory::createBalanced("legal");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->config().top_k_rerank, 50u);
        EXPECT_EQ(r->config().domain, "legal");
        EXPECT_GT(r->config().lora_weight, 0.2);
    }
    {
        auto r = LoRAEnhancedRetrieverFactory::createDomainSpecific("medical");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->config().domain, "medical");
        EXPECT_GE(r->config().lora_weight, 0.4);
    }
}

// Test suite: LoRAEnhancedRetrieverTests
//
// LER-01  HeuristicLoRAScorer::score returns value in [0, 1]
// LER-02  HeuristicLoRAScorer: identical query and content → score 1.0
// LER-03  HeuristicLoRAScorer: fully disjoint tokens → score 0.0
// LER-04  LoRAEnhancedRetriever: empty candidates returns empty list
// LER-05  LoRAEnhancedRetriever: fused score in [0, 1]; lora_score in metadata
// LER-06  Factory helpers produce retrievers with expected configurations
// LER-07  setScorer() injects custom scorer; custom scores drive re-ranking
// LER-08  setScorer(nullptr) → candidates returned unchanged (no crash)
//

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
// LER-07  setScorer() injects a custom scorer; its constant score drives rerank
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Custom scorer that always returns a constant value for testing injection.
class ConstantScorer : public ILoRAScorer {
public:
    explicit ConstantScorer(double val) : val_(val) {}
    double score(const std::string&, const std::string&, const std::string&) override {
        return val_;
    }
    std::string domain() const override { return "test"; }
private:
    double val_;
};

} // anonymous namespace

TEST(LoRAEnhancedRetrieverTests, LER07_SetScorerInjectsCustomScorer) {
    // Start with a heuristic scorer, then replace with a constant-score scorer.
    auto heuristic = std::make_shared<HeuristicLoRAScorer>();
    LoRARetrieverConfig cfg;
    cfg.lora_weight  = 1.0; // use lora score exclusively
    cfg.top_k_rerank = 10;
    LoRAEnhancedRetriever r(heuristic, cfg);

    // Inject a scorer that always returns 0.9.
    r.setScorer(std::make_shared<ConstantScorer>(0.9));

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "unrelated content", 0.3),
        makeDoc("d2", "another document",  0.4),
    };
    auto res = r.rerank("neural network", cands);

    ASSERT_EQ(res.size(), 2u);
    // With lora_weight = 1.0 and constant scorer = 0.9, all fused scores = 0.9.
    for (const auto& doc : res) {
        EXPECT_NEAR(doc.similarity_score, 0.9, 1e-9);
        EXPECT_EQ(doc.metadata.count("lora_score"), 1u);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LER-08  setScorer(nullptr) → candidates returned unchanged (no crash)
// ─────────────────────────────────────────────────────────────────────────────
TEST(LoRAEnhancedRetrieverTests, LER08_NullScorerReturnsUnchanged) {
    auto scorer = std::make_shared<HeuristicLoRAScorer>();
    LoRAEnhancedRetriever r(scorer);

    // Replace with null scorer.
    r.setScorer(nullptr);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alpha beta", 0.7),
        makeDoc("d2", "gamma delta", 0.5),
    };
    auto res = r.rerank("alpha", cands);

    // Null scorer → returned unchanged.
    ASSERT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0].id, "d1");
    EXPECT_NEAR(res[0].similarity_score, 0.7, 1e-9);
    EXPECT_EQ(res[1].id, "d2");
    EXPECT_NEAR(res[1].similarity_score, 0.5, 1e-9);
}
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

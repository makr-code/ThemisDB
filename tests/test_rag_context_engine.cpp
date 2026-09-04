#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/rag_context_engine.h"
#include "ethics_ai/ethics_ai_types.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

using namespace themis::plugins::ethics;
using SimilarityResults = std::vector<std::pair<std::string, double>>;

// ============================================================================
// Helpers
// ============================================================================

static EthicalArgument makeArg(const std::string& id,
                                const std::string& school,
                                ArgumentType type,
                                ArgumentStrength strength,
                                const std::string& content) {
    EthicalArgument a;
    a.id                = id;
    a.philosophy_school = school;
    a.argument_type     = type;
    a.strength          = strength;
    a.content           = content;
    return a;
}

// ============================================================================
// Test fixture
// ============================================================================
//
//  Seeds 20 arguments in SetUp across 3 philosophy schools and 2 argument types:
//
//    kant           :  7 PRO (STRONG)   + 3 CONTRA (MODERATE) = 10 arguments
//    utilitarianism :  5 PRO (DECISIVE) + 2 CONTRA (WEAK)     =  7 arguments
//    virtue_ethics  :  2 PRO (STRONG)   + 1 CONTRA (MODERATE) =  3 arguments
//    Total: 20 arguments
//
//  Explicit chain for BFS tests (direction="supports"):
//    kant-pro-1 → kant-pro-2 → kant-pro-3
// ============================================================================

class RAGContextEngineFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_shared<ArgumentStore>();
        ASSERT_TRUE(store_->initialize(nullptr, nullptr).isOK());
        engine_ = std::make_unique<RAGContextEngine>(store_);

        // ── kant PRO (7, STRONG) ──────────────────────────────────────────
        // Content shares the core Kantian vocabulary so that text-similarity
        // queries on "justice fairness duty" reliably return these arguments.
        // Ordinal suffixes differentiate entries while keeping them uniquely
        // identifiable without introducing semantic noise into the test.
        const std::vector<std::string> kant_content = {
            "justice fairness duty universalizability categorical imperative one",
            "justice fairness duty universalizability categorical imperative two",
            "justice fairness duty universalizability categorical imperative three",
            "justice fairness duty universalizability categorical imperative four",
            "justice fairness duty universalizability categorical imperative five",
            "justice fairness duty universalizability categorical imperative six",
            "justice fairness duty universalizability categorical imperative seven",
        };
        for (int i = 1; i <= 7; ++i) {
            store_->storeArgument(makeArg(
                "kant-pro-" + std::to_string(i), "kant",
                ArgumentType::PRO, ArgumentStrength::STRONG,
                kant_content[static_cast<size_t>(i - 1)]));
        }

        // ── kant CONTRA (3, MODERATE) ─────────────────────────────────────
        for (int i = 1; i <= 3; ++i) {
            store_->storeArgument(makeArg(
                "kant-contra-" + std::to_string(i), "kant",
                ArgumentType::CONTRA, ArgumentStrength::MODERATE,
                "Kantian counter-argument " + std::to_string(i)));
        }

        // ── utilitarianism PRO (5, DECISIVE) ──────────────────────────────
        for (int i = 1; i <= 5; ++i) {
            store_->storeArgument(makeArg(
                "util-pro-" + std::to_string(i), "utilitarianism",
                ArgumentType::PRO, ArgumentStrength::DECISIVE,
                "utility happiness welfare consequences benefit " + std::to_string(i)));
        }

        // ── utilitarianism CONTRA (2, WEAK) ───────────────────────────────
        for (int i = 1; i <= 2; ++i) {
            store_->storeArgument(makeArg(
                "util-contra-" + std::to_string(i), "utilitarianism",
                ArgumentType::CONTRA, ArgumentStrength::WEAK,
                "Utilitarian counter-argument " + std::to_string(i)));
        }

        // ── virtue_ethics PRO (2, STRONG) ─────────────────────────────────
        for (int i = 1; i <= 2; ++i) {
            store_->storeArgument(makeArg(
                "virtue-pro-" + std::to_string(i), "virtue_ethics",
                ArgumentType::PRO, ArgumentStrength::STRONG,
                "virtue character excellence flourishing " + std::to_string(i)));
        }

        // ── virtue_ethics CONTRA (1, MODERATE) ────────────────────────────
        store_->storeArgument(makeArg(
            "virtue-contra-1", "virtue_ethics",
            ArgumentType::CONTRA, ArgumentStrength::MODERATE,
            "Virtue ethics counter-argument"));

        // ── Build explicit chain: kant-pro-1 → kant-pro-2 → kant-pro-3 ───
        {
            auto r = store_->getArgument("kant-pro-1");
            if (auto* a = std::get_if<EthicalArgument>(&r)) {
                a->supports.push_back("kant-pro-2");
                store_->storeArgument(*a);
            }
        }
        {
            auto r = store_->getArgument("kant-pro-2");
            if (auto* a = std::get_if<EthicalArgument>(&r)) {
                a->supports.push_back("kant-pro-3");
                store_->storeArgument(*a);
            }
        }
    }

    std::shared_ptr<ArgumentStore>    store_;
    std::unique_ptr<RAGContextEngine> engine_;
};

// ============================================================================
// buildContext
// ============================================================================

TEST_F(RAGContextEngineFocusedTests, BuildContextReturnsRAGContext) {
    auto result = engine_->buildContext(
        "justice fairness duty categorical imperative",
        {"kant", "utilitarianism"},
        "resource_allocation");

    ASSERT_TRUE(std::holds_alternative<RAGContext>(result));
}

TEST_F(RAGContextEngineFocusedTests, BuildContextPopulatesPhilosophyArguments) {
    auto result = engine_->buildContext(
        "justice duty universalizability",
        {"kant", "utilitarianism"},
        "ethics");

    ASSERT_TRUE(std::holds_alternative<RAGContext>(result));
    const auto& ctx = std::get<RAGContext>(result);

    EXPECT_EQ(2u, ctx.philosophy_arguments.size());
    EXPECT_GT(ctx.philosophy_arguments.count("kant"), 0u);
    EXPECT_GT(ctx.philosophy_arguments.count("utilitarianism"), 0u);
    EXPECT_GE(ctx.philosophy_arguments.at("kant").size(), 1u);
    EXPECT_GE(ctx.philosophy_arguments.at("utilitarianism").size(), 1u);
}

TEST_F(RAGContextEngineFocusedTests, BuildContextRelevanceScoresPresent) {
    auto result = engine_->buildContext(
        "justice fairness duty", {"kant"}, "deontology");

    ASSERT_TRUE(std::holds_alternative<RAGContext>(result));
    const auto& ctx = std::get<RAGContext>(result);

    for (const auto& [school, ids] : ctx.philosophy_arguments) {
        for (const auto& id : ids) {
            EXPECT_GT(ctx.relevance_scores.count(id), 0u)
                << "Missing relevance score for argument " << id;
        }
    }
}

// ============================================================================
// findSimilarDilemmas (Pattern 1)
// ============================================================================

TEST_F(RAGContextEngineFocusedTests, FindSimilarDilemmasReturnsSimilarIds) {
    auto result = engine_->findSimilarDilemmas(
        "justice fairness duty universalizability", 0.1, 20);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_GE(std::get<std::vector<std::string>>(result).size(), 1u);
}

TEST_F(RAGContextEngineFocusedTests, FindSimilarDilemmasRespectsLimit) {
    auto result = engine_->findSimilarDilemmas(
        "justice fairness duty universalizability", 0.0, 3);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_LE(std::get<std::vector<std::string>>(result).size(), 3u);
}

TEST_F(RAGContextEngineFocusedTests, FindSimilarDilemmasHighThresholdReducesResults) {
    auto high = engine_->findSimilarDilemmas(
        "justice fairness duty universalizability", 0.9, 20);
    auto low  = engine_->findSimilarDilemmas(
        "justice fairness duty universalizability", 0.0, 20);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(high));
    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(low));
    EXPECT_LE(std::get<std::vector<std::string>>(high).size(),
              std::get<std::vector<std::string>>(low).size());
}

TEST_F(RAGContextEngineFocusedTests, FindSimilarDilemmasNoMatchReturnsEmpty) {
    // Query has zero overlap with any seeded content
    auto result = engine_->findSimilarDilemmas("xyzzy_qqq_zzzz", 0.99, 20);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_TRUE(std::get<std::vector<std::string>>(result).empty());
}

// ============================================================================
// getBestPractices (Pattern 3)
// ============================================================================

TEST_F(RAGContextEngineFocusedTests, GetBestPracticesReturnsDECISIVEandSTRONG) {
    // util-pro-* are DECISIVE (1.0) and kant-pro-* are STRONG (0.85)
    // min_satisfaction = 0.8 → both tiers qualify
    auto result = engine_->getBestPractices("ethics", 0.8, 50);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_GE(std::get<std::vector<std::string>>(result).size(), 1u);
}

TEST_F(RAGContextEngineFocusedTests, GetBestPracticesMinSatisfaction1ReturnsDECISIVE) {
    // Only DECISIVE arguments (score = 1.0) should survive threshold 1.0
    auto result = engine_->getBestPractices("ethics", 1.0, 50);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    // util-pro-* are DECISIVE → at least 1 result
    EXPECT_GE(std::get<std::vector<std::string>>(result).size(), 1u);
}

TEST_F(RAGContextEngineFocusedTests, GetBestPracticesRespectsLimit) {
    auto result = engine_->getBestPractices("ethics", 0.0, 2);

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_LE(std::get<std::vector<std::string>>(result).size(), 2u);
}

// ============================================================================
// vectorSemanticSearch (Pattern 4)
// ============================================================================

TEST_F(RAGContextEngineFocusedTests, VectorSemanticSearchReturnsRankedResults) {
    // Build a 768-dim query vector using the same character-frequency hashing
    // approach as RAGContextEngine::generateEmbedding() so that query and
    // argument embeddings share the same dimensionality and can be compared.
    // Note: this is a bag-of-characters placeholder, not a semantic embedding;
    // it exercises the search path end-to-end without requiring a real model.
    std::vector<float> query(768, 0.0f);
    const std::string query_text = "justice fairness duty universalizability";
    for (unsigned char c : query_text) {
        query[c % 768] += 1.0f;
    }
    float norm = 0.0f;
    for (float v : query) {
      norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& v : query) {
          v /= norm;
        }
    }

    auto result = engine_->vectorSemanticSearch(query, "kant", 5);

    ASSERT_TRUE(std::holds_alternative<SimilarityResults>(result));
    const auto& hits = std::get<SimilarityResults>(result);
    EXPECT_GE(hits.size(), 1u);
    EXPECT_LE(hits.size(), 5u);

    for (const auto& [id, score] : hits) {
        EXPECT_GE(score, -1.01);
        EXPECT_LE(score,  1.01);
    }
}

TEST_F(RAGContextEngineFocusedTests, VectorSemanticSearchEmptyQueryReturnsEmpty) {
    auto result = engine_->vectorSemanticSearch({}, "kant", 5);

    ASSERT_TRUE(std::holds_alternative<SimilarityResults>(result));
    EXPECT_TRUE(std::get<SimilarityResults>(result).empty());
}

TEST_F(RAGContextEngineFocusedTests, VectorSemanticSearchNoSchoolFilterSearchesAll) {
    std::vector<float> query(768, 0.01f);

    auto filtered = engine_->vectorSemanticSearch(query, "kant", 100);
    auto all      = engine_->vectorSemanticSearch(query, "",     100);

    ASSERT_TRUE(std::holds_alternative<SimilarityResults>(filtered));
    ASSERT_TRUE(std::holds_alternative<SimilarityResults>(all));

    // Unfiltered covers all 3 schools → at least as many results
    EXPECT_GE(std::get<SimilarityResults>(all).size(),
              std::get<SimilarityResults>(filtered).size());
}

// ============================================================================
// traverseArgumentChain (Pattern 5)
// ============================================================================

TEST_F(RAGContextEngineFocusedTests, TraverseArgumentChainBFSOrderIsCorrect) {
    // Chain kant-pro-1 → kant-pro-2 → kant-pro-3 was built in SetUp
    auto result = engine_->traverseArgumentChain("kant-pro-1", 3, "supports");

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& chain = std::get<std::vector<std::string>>(result);

    ASSERT_GE(chain.size(), 1u);
    EXPECT_EQ("kant-pro-1", chain[0]);

    auto pos2 = std::find(chain.begin(), chain.end(), "kant-pro-2");
    auto pos3 = std::find(chain.begin(), chain.end(), "kant-pro-3");
    if (pos2 != chain.end() && pos3 != chain.end()) {
        EXPECT_LT(pos2, pos3) << "BFS must visit depth-1 before depth-2";
    }
}

TEST_F(RAGContextEngineFocusedTests, TraverseArgumentChainRespectsMaxDepth) {
    // max_depth = 1 → seed + direct children only; kant-pro-3 is at depth-2 → absent
    auto result = engine_->traverseArgumentChain("kant-pro-1", 1, "supports");

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& chain = std::get<std::vector<std::string>>(result);

    bool has_deep = std::find(chain.begin(), chain.end(), "kant-pro-3") != chain.end();
    EXPECT_FALSE(has_deep) << "Depth-2 node must not appear at max_depth=1";
}

TEST_F(RAGContextEngineFocusedTests, TraverseArgumentChainUnknownIdReturnsSeedOnly) {
    auto result = engine_->traverseArgumentChain("nonexistent-id", 5, "supports");

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& chain = std::get<std::vector<std::string>>(result);
    EXPECT_EQ(1u, chain.size());
    EXPECT_EQ("nonexistent-id", chain[0]);
}

TEST_F(RAGContextEngineFocusedTests, TraverseArgumentChainCycleSafe) {
    // Create a cycle: kant-pro-4 → kant-pro-5 → back to kant-pro-4
    {
        auto r = store_->getArgument("kant-pro-4");
        if (auto* a = std::get_if<EthicalArgument>(&r)) {
            a->supports.push_back("kant-pro-5");
            store_->storeArgument(*a);
        }
    }
    {
        auto r = store_->getArgument("kant-pro-5");
        if (auto* a = std::get_if<EthicalArgument>(&r)) {
            a->supports.push_back("kant-pro-4");
            store_->storeArgument(*a);
        }
    }

    auto result = engine_->traverseArgumentChain("kant-pro-4", 10, "supports");

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& chain = std::get<std::vector<std::string>>(result);

    std::unordered_set<std::string> seen = {};

    for (const auto& id : chain) {
        EXPECT_TRUE(seen.insert(id).second)
            << "Cycle-safe BFS produced duplicate node: " << id;
    }
}

TEST_F(RAGContextEngineFocusedTests, TraverseArgumentChainEmptyStartIdReturnsEmpty) {
    auto result = engine_->traverseArgumentChain("", 5, "supports");

    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_TRUE(std::get<std::vector<std::string>>(result).empty());
}

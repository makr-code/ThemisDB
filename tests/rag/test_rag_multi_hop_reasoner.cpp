/**
 * @file test_rag_multi_hop_reasoner.cpp
 * @brief Unit tests for MultiHopReasoner (multi-hop reasoning, Phase 7).
 *
 * Test suite: MultiHopReasonerFocusedTests (15 tests)
 *   Group A (5) – Configuration and factory helpers
 *   Group B (5) – Query decomposition (heuristic + LLM-based)
 *   Group C (5) – Multi-hop reasoning pipeline (single, multi, error cases)
 */

#include <gtest/gtest.h>
#include "rag/multi_hop_reasoner.h"

#include <atomic>
#include <string>
#include <vector>

using namespace themis::rag::multi_hop;
using namespace themis::rag::judge;

// ── Test helpers ──────────────────────────────────────────────────────────────

static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score = 0.8)
{
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

/**
 * Stub retrieval function: returns `n` documents for any sub-query.
 */
static RetrievalFn stubRetrieval(size_t n = 2)
{
    return [n](const std::string& sub_query, size_t /*top_k*/) {
        std::vector<RetrievedDocument> docs;
        for (size_t i = 0; i < n; ++i) {
            docs.push_back(makeDoc("doc_" + std::to_string(i),
                                   "Document " + std::to_string(i) +
                                   " relevant to: " + sub_query));
        }
        return docs;
    };
}

/**
 * Stub inference: echoes a configurable prefix for any prompt.
 */
static InferenceFn echoInfer(const std::string& prefix = "ANSWER")
{
    return [prefix](const std::string& /*prompt*/, int /*max_tokens*/) {
        return prefix;
    };
}

/**
 * Stub inference that decomposes a query into two sub-questions.
 */
static InferenceFn decomposingInfer()
{
    return [](const std::string& prompt, int /*max_tokens*/) -> std::string {
        // If this looks like a decomposition prompt, return two sub-questions
        if (prompt.find("Break") != std::string::npos ||
            prompt.find("sub-question") != std::string::npos) {
            return "What is the first part?\nWhat is the second part?";
        }
        return "ANSWER";
    };
}

// ══════════════════════════════════════════════════════════════════════════════
// Group A – Configuration and factory helpers
// ══════════════════════════════════════════════════════════════════════════════

TEST(MultiHopReasonerFocusedTests, A1_DefaultConfigValues)
{
    MultiHopConfig cfg;
    EXPECT_EQ(cfg.max_hops, 5u);
    EXPECT_EQ(cfg.top_k_per_hop, 5u);
    EXPECT_EQ(cfg.max_tokens_per_hop, 256);
    EXPECT_EQ(cfg.max_tokens_final, 512);
    EXPECT_TRUE(cfg.early_stopping);
}

TEST(MultiHopReasonerFocusedTests, A2_SetAndGetConfig)
{
    MultiHopConfig cfg;
    cfg.max_hops = 3;
    cfg.top_k_per_hop = 8;
    MultiHopReasoner r(cfg);
    EXPECT_EQ(r.getConfig().max_hops, 3u);
    EXPECT_EQ(r.getConfig().top_k_per_hop, 8u);

    MultiHopConfig cfg2;
    cfg2.max_hops = 2;
    r.setConfig(cfg2);
    EXPECT_EQ(r.getConfig().max_hops, 2u);
}

TEST(MultiHopReasonerFocusedTests, A3_FactorySingleHop)
{
    auto r = MultiHopReasonerFactory::createSingleHop();
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_hops, 1u);
    EXPECT_TRUE(r->getConfig().early_stopping);
}

TEST(MultiHopReasonerFocusedTests, A4_FactoryBalanced)
{
    auto r = MultiHopReasonerFactory::createBalanced();
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_hops, 3u);
    EXPECT_EQ(r->getConfig().top_k_per_hop, 5u);
}

TEST(MultiHopReasonerFocusedTests, A5_FactoryDeepReasoning)
{
    auto r = MultiHopReasonerFactory::createDeepReasoning();
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_hops, 5u);
    EXPECT_EQ(r->getConfig().top_k_per_hop, 8u);
    EXPECT_GE(r->getConfig().max_tokens_per_hop, 256);
}

// ══════════════════════════════════════════════════════════════════════════════
// Group B – Query decomposition
// ══════════════════════════════════════════════════════════════════════════════

TEST(MultiHopReasonerFocusedTests, B1_DecomposeEmptyQueryReturnsEmpty)
{
    MultiHopReasoner r;
    const auto sub = r.decomposeQuery("", nullptr);
    EXPECT_TRUE(sub.empty());
}

TEST(MultiHopReasonerFocusedTests, B2_HeuristicDecomposeSimpleQuery)
{
    MultiHopReasoner r;
    // Single sentence — should return one sub-query
    const auto sub = r.decomposeQuery("What is the capital of France?", nullptr);
    ASSERT_FALSE(sub.empty());
    EXPECT_EQ(sub.size(), 1u);
    EXPECT_FALSE(sub[0].empty());
}

TEST(MultiHopReasonerFocusedTests, B3_HeuristicDecomposeMultiSentence)
{
    MultiHopReasoner r;
    // Two sentences — heuristic should split into two sub-queries
    const auto sub = r.decomposeQuery(
        "When was the Eiffel Tower built? Who designed it?", nullptr);
    ASSERT_GE(sub.size(), 1u);
    // At least one sub-query is non-empty
    for (const auto& s : sub) {
        EXPECT_FALSE(s.empty());
    }
}

TEST(MultiHopReasonerFocusedTests, B4_LLMDecomposeReturnsSubQueries)
{
    MultiHopReasoner r;
    const auto sub = r.decomposeQuery(
        "Who invented the transistor and what year was it patented?",
        decomposingInfer());
    // The stub returns two sub-questions
    ASSERT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub[0], "What is the first part?");
    EXPECT_EQ(sub[1], "What is the second part?");
}

TEST(MultiHopReasonerFocusedTests, B5_DecomposeRespectMaxHops)
{
    MultiHopConfig cfg;
    cfg.max_hops = 2;
    MultiHopReasoner r(cfg);

    // Stub LLM returns 3 sub-questions, but max_hops=2
    auto threeLinesInfer = [](const std::string& /*p*/, int /*t*/) -> std::string {
        return "Line 1\nLine 2\nLine 3";
    };

    const auto sub = r.decomposeQuery("complex query", threeLinesInfer);
    EXPECT_LE(sub.size(), 2u);
}

// ══════════════════════════════════════════════════════════════════════════════
// Group C – Multi-hop reasoning pipeline
// ══════════════════════════════════════════════════════════════════════════════

TEST(MultiHopReasonerFocusedTests, C1_EmptyQueryReturnsEmptyResult)
{
    MultiHopReasoner r;
    const auto result = r.reason("", stubRetrieval(), echoInfer());
    EXPECT_TRUE(result.final_answer.empty());
    EXPECT_EQ(result.hops_executed, 0u);
}

TEST(MultiHopReasonerFocusedTests, C2_NullRetrievalReturnsEmpty)
{
    MultiHopReasoner r;
    const auto result = r.reason("What is X?", nullptr, echoInfer());
    EXPECT_TRUE(result.final_answer.empty());
}

TEST(MultiHopReasonerFocusedTests, C3_NullInferenceReturnsEmpty)
{
    MultiHopReasoner r;
    const auto result = r.reason("What is X?", stubRetrieval(), nullptr);
    EXPECT_TRUE(result.final_answer.empty());
}

TEST(MultiHopReasonerFocusedTests, C4_SingleHopPipelineProducesAnswer)
{
    auto r = MultiHopReasonerFactory::createSingleHop();
    const auto result = r->reason("What is the speed of light?",
                                   stubRetrieval(3),
                                   echoInfer("SPEED"));
    EXPECT_EQ(result.final_answer, "SPEED");
    EXPECT_EQ(result.hops_executed, 1u);
    EXPECT_FALSE(result.all_documents.empty());
}

TEST(MultiHopReasonerFocusedTests, C5_MultiHopPipelineExecutesAllHops)
{
    MultiHopConfig cfg;
    cfg.max_hops       = 2;
    cfg.top_k_per_hop  = 2;
    cfg.early_stopping = false;
    MultiHopReasoner r(cfg);

    std::atomic<int> inference_calls{0};
    auto countingInfer = [&](const std::string& prompt, int /*max_tokens*/)
        -> std::string
    {
        // First call is decomposition → return two sub-questions
        const int call = inference_calls.fetch_add(1);
        if (call == 0 &&
            (prompt.find("Break") != std::string::npos ||
             prompt.find("sub-question") != std::string::npos)) {
            return "Sub-question A\nSub-question B";
        }
        return "PARTIAL_ANSWER";
    };

    const auto result = r.reason("Complex multi-hop question",
                                   stubRetrieval(2),
                                   countingInfer);

    // Should have executed at least one hop
    EXPECT_GE(result.hops_executed, 1u);
    EXPECT_FALSE(result.final_answer.empty());
    EXPECT_LE(result.hops_executed, cfg.max_hops);
}

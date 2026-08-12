/**
 * @file test_llm_reranker.cpp
 * @brief Unit tests for LlmReranker — configurable re-ranking with LLM feedback loop.
 */

#include <gtest/gtest.h>
#include "search/llm_reranker.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static LlmRerankCandidate makeCandidate(const std::string& id,
                                         const std::string& content,
                                         double initial_score = 0.5) {
    LlmRerankCandidate c;
    c.document_id   = id;
    c.content       = content;
    c.initial_score = initial_score;
    return c;
}

static LlmReranker::LlmBackend makeMockBackend(const std::string& response) {
    return [response](const std::string& /*prompt*/) { return response; };
}

// ============================================================================
// Config validation
// ============================================================================

TEST(LlmRerankerConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(LlmReranker{});
}

TEST(LlmRerankerConfig, ZeroBatchSizeThrows) {
    LlmReranker::Config cfg;
    cfg.batch_size = 0;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, LlmWeightBelowZeroThrows) {
    LlmReranker::Config cfg;
    cfg.llm_weight = -0.1;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, LlmWeightAboveOneThrows) {
    LlmReranker::Config cfg;
    cfg.llm_weight = 1.1;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, ZeroSnippetLengthThrows) {
    LlmReranker::Config cfg;
    cfg.max_snippet_length = 0;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, NegativeThresholdThrows) {
    LlmReranker::Config cfg;
    cfg.min_score_threshold = -0.1;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, ThresholdAboveOneThrows) {
    LlmReranker::Config cfg;
    cfg.min_score_threshold = 1.1;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, NegativeTemperatureThrows) {
    LlmReranker::Config cfg;
    cfg.temperature = -0.1f;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, TemperatureAboveTwoThrows) {
    LlmReranker::Config cfg;
    cfg.temperature = 2.1f;
    EXPECT_THROW(LlmReranker{cfg}, std::invalid_argument);
}

TEST(LlmRerankerConfig, ConfigRoundtrip) {
    LlmReranker::Config cfg;
    cfg.batch_size   = 3;
    cfg.llm_weight   = 0.8;
    cfg.temperature  = 0.7f;
    LlmReranker rr{cfg};
    EXPECT_EQ(rr.getConfig().batch_size, 3u);
    EXPECT_DOUBLE_EQ(rr.getConfig().llm_weight, 0.8);
    EXPECT_FLOAT_EQ(rr.getConfig().temperature, 0.7f);
}

// ============================================================================
// rerank() — empty input
// ============================================================================

TEST(LlmRerankerRerank, EmptyCandidatesReturnsEmpty) {
    LlmReranker rr;
    auto results = rr.rerank("query", {});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// rerank() — no backend
// ============================================================================

TEST(LlmRerankerNoBackend, FallbackSortsByInitialScore) {
    LlmReranker rr; // no backend
    // Input intentionally in WRONG order (low score first) to verify sorting
    auto candidates = {
        makeCandidate("doc_low",  "content B", 0.3),
        makeCandidate("doc_high", "content A", 0.9)
    };
    auto results = rr.rerank("query", candidates);
    ASSERT_EQ(results.size(), 2u);
    // Fallback must sort by initial_score descending — doc_high should come first
    EXPECT_EQ(results[0].document_id, "doc_high");
    EXPECT_DOUBLE_EQ(results[0].final_score, 0.9);
    EXPECT_EQ(results[1].document_id, "doc_low");
    EXPECT_DOUBLE_EQ(results[1].final_score, 0.3);
    EXPECT_FALSE(results[0].llm_scored);
}

TEST(LlmRerankerNoBackend, FallbackDisabledReturnsEmpty) {
    LlmReranker::Config cfg;
    cfg.fallback_to_original = false;
    LlmReranker rr{cfg};
    auto results = rr.rerank("query", {makeCandidate("d1", "text", 0.8)});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// rerank() — with mock backend
// ============================================================================

TEST(LlmRerankerWithBackend, LlmScoresApplied) {
    // LLM scores: doc1 = 3, doc2 = 9  → doc2 should rank first
    LlmReranker rr{{}, makeMockBackend("3\n9\n")};
    auto candidates = {
        makeCandidate("doc1", "irrelevant text", 0.5),
        makeCandidate("doc2", "highly relevant", 0.5)
    };
    auto results = rr.rerank("fast db insert", candidates);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc2");
    EXPECT_GT(results[0].final_score, results[1].final_score);
    EXPECT_TRUE(results[0].llm_scored);
}

TEST(LlmRerankerWithBackend, ScoresNormalisedToUnitRange) {
    // LLM returns score 10 (max) and 0 (min)
    LlmReranker rr{{}, makeMockBackend("10\n0\n")};
    auto candidates = {
        makeCandidate("d1", "text", 0.0),
        makeCandidate("d2", "text", 0.0)
    };
    auto results = rr.rerank("q", candidates);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_DOUBLE_EQ(results[0].llm_score, 1.0);
    EXPECT_DOUBLE_EQ(results[1].llm_score, 0.0);
}

TEST(LlmRerankerWithBackend, ScoresClamped) {
    // LLM returns out-of-range values
    LlmReranker rr{{}, makeMockBackend("15\n-3\n")};
    auto candidates = {
        makeCandidate("d1", "text", 0.0),
        makeCandidate("d2", "text", 0.0)
    };
    auto results = rr.rerank("q", candidates);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_DOUBLE_EQ(results[0].llm_score, 1.0); // clamped from 15 → 10 → 1.0
    EXPECT_DOUBLE_EQ(results[1].llm_score, 0.0); // clamped from -3 → 0 → 0.0
}

TEST(LlmRerankerWithBackend, BlendingWeight) {
    // llm_weight=0.5, llm_score=1.0 (score 10), initial_score=0.0
    // final = 0.5*1.0 + 0.5*0.0 = 0.5
    LlmReranker::Config cfg;
    cfg.llm_weight = 0.5;
    LlmReranker rr{cfg, makeMockBackend("10\n")};
    auto results = rr.rerank("q", {makeCandidate("d1", "text", 0.0)});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_NEAR(results[0].final_score, 0.5, 1e-9);
}

TEST(LlmRerankerWithBackend, MissingScoresPaddedWithZero) {
    // LLM only returns one score for two candidates
    LlmReranker rr{{}, makeMockBackend("8\n")};
    auto candidates = {
        makeCandidate("d1", "text", 0.5),
        makeCandidate("d2", "text", 0.5)
    };
    auto results = rr.rerank("q", candidates);
    ASSERT_EQ(results.size(), 2u);
    // d1 has llm_score 0.8, d2 padded with 0.0
    bool d1_first = (results[0].document_id == "d1");
    EXPECT_TRUE(d1_first);
}

TEST(LlmRerankerWithBackend, MinScoreThresholdFilters) {
    LlmReranker::Config cfg;
    cfg.min_score_threshold = 0.5; // only keep final_score >= 0.5
    cfg.llm_weight = 1.0;          // final == llm_score
    LlmReranker rr{cfg, makeMockBackend("9\n2\n")};
    auto candidates = {
        makeCandidate("d1", "text", 0.0),
        makeCandidate("d2", "text", 0.0)
    };
    auto results = rr.rerank("q", candidates);
    // d1: llm_score=0.9, d2: llm_score=0.2 — only d1 passes the threshold
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "d1");
}

TEST(LlmRerankerWithBackend, BatchingHandlesMultipleBatches) {
    LlmReranker::Config cfg;
    cfg.batch_size = 2; // process 2 at a time
    // LLM will return 2 scores per call
    LlmReranker rr{cfg, makeMockBackend("5\n5\n")};
    std::vector<LlmRerankCandidate> candidates = {
        makeCandidate("d1", "text", 0.0),
        makeCandidate("d2", "text", 0.0),
        makeCandidate("d3", "text", 0.0) // third candidate, second batch
    };
    auto results = rr.rerank("q", candidates);
    EXPECT_EQ(results.size(), 3u);
}

TEST(LlmRerankerWithBackend, BackendExceptionTriggeresFallback) {
    LlmReranker::LlmBackend throwing_backend = [](const std::string&) -> std::string {
        throw std::runtime_error("LLM unavailable");
    };
    LlmReranker rr{{}, throwing_backend};
    auto candidates = {makeCandidate("d1", "text", 0.7)};
    auto results = rr.rerank("q", candidates);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "d1");
    EXPECT_FALSE(results[0].llm_scored);
}

TEST(LlmRerankerWithBackend, BackendExceptionNoFallback) {
    LlmReranker::Config cfg;
    cfg.fallback_to_original = false;
    LlmReranker::LlmBackend throwing_backend = [](const std::string&) -> std::string {
        throw std::runtime_error("LLM error");
    };
    LlmReranker rr{cfg, throwing_backend};
    auto results = rr.rerank("q", {makeCandidate("d1", "text", 0.7)});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// setBackend() — dynamic backend replacement
// ============================================================================

TEST(LlmRerankerSetBackend, ReplacesNullBackend) {
    LlmReranker rr; // no backend
    rr.setBackend(makeMockBackend("8\n"));
    auto results = rr.rerank("q", {makeCandidate("d1", "text", 0.0)});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].llm_scored);
    EXPECT_DOUBLE_EQ(results[0].llm_score, 0.8);
}

TEST(LlmRerankerSetBackend, ClearingBackendFallsBack) {
    LlmReranker rr{{}, makeMockBackend("10\n")};
    rr.setBackend(nullptr);
    auto results = rr.rerank("q", {makeCandidate("d1", "text", 0.6)});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].llm_scored);
    EXPECT_DOUBLE_EQ(results[0].final_score, 0.6);
}

// ============================================================================
// Prompt content
// ============================================================================

TEST(LlmRerankerPrompt, PromptContainsQuery) {
    std::string captured;
    LlmReranker rr{{}, [&](const std::string& p) { captured = p; return "5\n"; }};
    rr.rerank("database indexing performance", {makeCandidate("d1", "text")});
    EXPECT_NE(captured.find("database indexing performance"), std::string::npos);
}

TEST(LlmRerankerPrompt, PromptContainsDocumentContent) {
    std::string captured;
    LlmReranker rr{{}, [&](const std::string& p) { captured = p; return "5\n"; }};
    rr.rerank("query", {makeCandidate("d1", "unique content phrase")});
    EXPECT_NE(captured.find("unique content phrase"), std::string::npos);
}

TEST(LlmRerankerPrompt, SnippetTruncatedToMaxLength) {
    std::string captured;
    LlmReranker::Config cfg;
    cfg.max_snippet_length = 10;
    LlmReranker rr{cfg, [&](const std::string& p) { captured = p; return "5\n"; }};
    rr.rerank("q", {makeCandidate("d1", "this is a very long content snippet that should be truncated")});
    // The snippet in the prompt should not contain the full long string
    EXPECT_EQ(captured.find("truncated"), std::string::npos);
}

TEST(LlmRerankerPrompt, PromptContainsScoreInstructions) {
    std::string captured;
    LlmReranker rr{{}, [&](const std::string& p) { captured = p; return "5\n"; }};
    rr.rerank("q", {makeCandidate("d1", "text")});
    EXPECT_NE(captured.find("0"), std::string::npos);
    EXPECT_NE(captured.find("10"), std::string::npos);
}

TEST(LlmRerankerPrompt, TemperatureHintIncludedWhenNonZero) {
    std::string captured;
    LlmReranker::Config cfg;
    cfg.temperature = 0.5f;
    LlmReranker rr{cfg, [&](const std::string& p) { captured = p; return "5\n"; }};
    rr.rerank("q", {makeCandidate("d1", "text")});
    EXPECT_NE(captured.find("temperature"), std::string::npos);
}

TEST(LlmRerankerPrompt, TemperatureHintOmittedWhenZero) {
    std::string captured;
    LlmReranker rr{{}, [&](const std::string& p) { captured = p; return "5\n"; }};
    // Default temperature is 0.0 — hint should not appear
    rr.rerank("q", {makeCandidate("d1", "text")});
    EXPECT_EQ(captured.find("temperature"), std::string::npos);
}

// ============================================================================
// toClickEvents() — LLM feedback bridge
// ============================================================================

TEST(LlmRerankerFeedback, RelevantResultBecomesClickEvent) {
    std::vector<LlmRerankResult> results;
    LlmRerankResult r;
    r.document_id = "doc1";
    r.llm_score   = 0.8;
    r.final_score = 0.8;
    r.llm_scored  = true;
    results.push_back(r);

    auto events = LlmReranker::toClickEvents("ml query", results, 0.5);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].query, "ml query");
    EXPECT_EQ(events[0].document_id, "doc1");
    EXPECT_EQ(events[0].result_position, 0u);
}

TEST(LlmRerankerFeedback, IrrelevantResultNotInClickEvents) {
    std::vector<LlmRerankResult> results;
    LlmRerankResult r;
    r.document_id = "doc1";
    r.llm_score   = 0.3; // below default threshold 0.5
    r.llm_scored  = true;
    results.push_back(r);

    auto events = LlmReranker::toClickEvents("q", results, 0.5);
    EXPECT_TRUE(events.empty());
}

TEST(LlmRerankerFeedback, EmptyResultsYieldsNoClickEvents) {
    auto events = LlmReranker::toClickEvents("q", {}, 0.5);
    EXPECT_TRUE(events.empty());
}

TEST(LlmRerankerFeedback, RankPositionPreservedInClickEvents) {
    std::vector<LlmRerankResult> results;
    for (size_t i = 0; i < 3; ++i) {
        LlmRerankResult r;
        r.document_id = "doc" + std::to_string(i);
        r.llm_score   = 0.9; // all above threshold
        r.llm_scored  = true;
        results.push_back(r);
    }
    auto events = LlmReranker::toClickEvents("q", results, 0.5);
    ASSERT_EQ(events.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(events[i].result_position, i);
    }
}

TEST(LlmRerankerFeedback, IntegrationWithLearningToRank) {
    // End-to-end: LLM re-ranks, feedback fed to LTR
    LlmReranker rr{{}, makeMockBackend("9\n2\n")};
    auto candidates = {
        makeCandidate("doc_good", "highly relevant", 0.5),
        makeCandidate("doc_bad",  "not relevant",    0.5)
    };
    auto reranked = rr.rerank("test query", candidates);
    ASSERT_EQ(reranked.size(), 2u);

    auto events = LlmReranker::toClickEvents("test query", reranked, 0.5);
    // doc_good has llm_score 0.9, doc_bad has 0.2 → only doc_good yields a click
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].document_id, "doc_good");

    // Feed into LTR (just verify it doesn't crash)
    LearningToRank ltr;
    for (const auto& ev : events) {
        ltr.recordClick(ev);
    }
    EXPECT_EQ(ltr.train(), 1u);
}

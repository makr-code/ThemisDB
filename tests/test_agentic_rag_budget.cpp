/**
 * @file test_agentic_rag_budget.cpp
 * @brief Unit tests for AgenticRAG session token-budget cap (Gap 4).
 *
 * Test group: ARG_BUD (AgenticRAG Budget)
 *
 *   ARG_BUD_01  No budget set (max_session_tokens=0) → existing behaviour preserved,
 *               stop_reason != BUDGET_EXCEEDED, tokens_consumed == 0
 *   ARG_BUD_02  Budget set to 1 token → BUDGET_EXCEEDED after first iteration estimate
 *   ARG_BUD_03  Budget large enough for all iterations → loop runs to quality/gap result
 *   ARG_BUD_04  StopReason::BUDGET_EXCEEDED enum value exists and is distinct
 *   ARG_BUD_05  result.tokens_consumed is populated when budget is set
 *   ARG_BUD_06  SIZE_MAX budget is sanitized to keep internal sentinel arithmetic safe
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 4 (Severity: Medium/S1)
 * Tracked: src/rag/FUTURE_ENHANCEMENTS.md §"Session Token-Budget Cap for AgenticRAG"
 */

#include <gtest/gtest.h>

#include "rag/agentic_rag.h"

#include <limits>
#include <string>
#include <vector>

using namespace themis::rag::agentic;

namespace {

AgenticRAGConfig makeFastBudgetConfig()
{
    AgenticRAGConfig cfg;

    // Keep budget tests independent of heavyweight LLM-driven judge paths.
    cfg.judge_config.mode = themis::rag::judge::EvaluationMode::FAST;
    cfg.judge_config.enable_claim_verification = false;
    cfg.judge_config.enable_citation_check = false;
    cfg.judge_config.enable_ethical_evaluation = false;
    cfg.judge_config.enable_prompt_injection_screening = false;
    cfg.judge_config.cache_evaluations = false;

    cfg.gap_config.mode = themis::rag::knowledge_gap::DetectionMode::FAST;
    cfg.gap_config.enable_self_consistency_check = false;
    cfg.gap_config.enable_flare = false;
    cfg.gap_config.enable_claim_verification = false;
    cfg.gap_config.enable_query_aspect_analysis = false;
    cfg.gap_config.enable_token_probability = false;
    cfg.gap_config.enable_ethical_gap_detection = false;

    return cfg;
}

AgenticRAG& sharedBudgetAgent()
{
    static AgenticRAG agent(makeFastBudgetConfig());
    return agent;
}

std::vector<themis::rag::judge::RetrievedDocument> makeDocs(size_t count,
                                                            size_t content_len = 100);

class ARG_BUD : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        auto& agent = sharedBudgetAgent();
        auto cfg = makeFastBudgetConfig();
        cfg.max_session_tokens = 1'000'000;
        cfg.max_iterations = 1;
        agent.setConfig(cfg);

        // Prime one shared instance so expensive first-use initialization stays
        // outside the individual test assertion paths.
        [[maybe_unused]] const auto warmup = agent.run("warmup", makeDocs(1, 16));
    }
};

// Helper: build a list of N fake documents, each with content of a given length.
std::vector<themis::rag::judge::RetrievedDocument> makeDocs(size_t count, size_t content_len)
{
    std::vector<themis::rag::judge::RetrievedDocument> docs;
    docs.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        themis::rag::judge::RetrievedDocument d;
        d.id               = "doc-" + std::to_string(i);
        d.content          = std::string(content_len, 'x');
        d.similarity_score = 0.5f;
        docs.push_back(std::move(d));
    }
    return docs;
}

} // namespace

// ---------------------------------------------------------------------------
// ARG_BUD_01 – no budget → behaviour unchanged
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_01_NoBudgetPreservesExistingBehaviour) {
    AgenticRAGConfig cfg = makeFastBudgetConfig();
    cfg.max_session_tokens = 0;   // disabled
    cfg.max_iterations     = 1;
    auto& agent = sharedBudgetAgent();
    agent.setConfig(cfg);

    const auto docs   = makeDocs(1, 10);
    const auto result = agent.run("query", docs);

    EXPECT_NE(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "When max_session_tokens==0 the loop must never stop with BUDGET_EXCEEDED";
    EXPECT_EQ(result.tokens_consumed, 0u)
        << "tokens_consumed must stay 0 when budget enforcement is disabled";
}

// ---------------------------------------------------------------------------
// ARG_BUD_02 – budget of 1 → BUDGET_EXCEEDED on first iteration
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_02_TinyBudgetTriggersExceeded) {
    AgenticRAGConfig cfg = makeFastBudgetConfig();
    cfg.max_session_tokens = 1;   // every doc+query estimate exceeds this
    cfg.max_iterations     = 10;  // many iterations available, but budget stops it
    auto& agent = sharedBudgetAgent();
    agent.setConfig(cfg);

    // A document with 100 chars → token estimate ≈ 26; already > 1.
    const auto docs   = makeDocs(1, 100);
    const auto result = agent.run("some query", docs);

    EXPECT_EQ(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "With max_session_tokens=1 and docs with 100 chars, BUDGET_EXCEEDED expected";
}

// ---------------------------------------------------------------------------
// ARG_BUD_03 – large budget → loop runs until natural stop
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_03_LargeBudgetDoesNotInterfere) {
    AgenticRAGConfig cfg = makeFastBudgetConfig();
    cfg.max_session_tokens = 1'000'000;  // effectively unlimited
    cfg.max_iterations     = 2;
    auto& agent = sharedBudgetAgent();
    agent.setConfig(cfg);

    const auto docs   = makeDocs(2, 50);
    const auto result = agent.run("query", docs);

    // Should stop for some natural reason, not because of budget.
    EXPECT_NE(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "Large budget must not cause premature BUDGET_EXCEEDED termination";
}

// ---------------------------------------------------------------------------
// ARG_BUD_04 – StopReason::BUDGET_EXCEEDED is a distinct enum value
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_04_BudgetExceededEnumDistinct) {
    // Compile-time check: the enum value must exist and be different from
    // all other stop reasons.
    const auto v = static_cast<int>(StopReason::BUDGET_EXCEEDED);
    EXPECT_NE(v, static_cast<int>(StopReason::QUALITY_SATISFIED));
    EXPECT_NE(v, static_cast<int>(StopReason::MAX_ITERATIONS));
    EXPECT_NE(v, static_cast<int>(StopReason::NO_GAP_DETECTED));
    EXPECT_NE(v, static_cast<int>(StopReason::NO_NEW_DOCUMENTS));
    EXPECT_NE(v, static_cast<int>(StopReason::CANCELLED));
}

// ---------------------------------------------------------------------------
// ARG_BUD_05 – tokens_consumed > 0 when budget enforcement is active
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_05_TokensConsumedPopulatedWithBudget) {
    AgenticRAGConfig cfg = makeFastBudgetConfig();
    cfg.max_session_tokens = 1'000'000;  // large — won't exceed
    cfg.max_iterations     = 1;
    auto& agent = sharedBudgetAgent();
    agent.setConfig(cfg);

    // 3 docs with 40 chars each → token estimate per doc: 40/4+1 = 11.
    // Query "some query" → 10/4+1 = 3.
    // Total estimate >= 36.
    const auto docs   = makeDocs(3, 40);
    const auto result = agent.run("some query", docs);

    EXPECT_GT(result.tokens_consumed, 0u)
        << "tokens_consumed must be > 0 when max_session_tokens > 0 and docs are present";
}

// ---------------------------------------------------------------------------
// ARG_BUD_06 – SIZE_MAX budget is sanitized to avoid internal sentinel overflow
// ---------------------------------------------------------------------------
TEST_F(ARG_BUD, ARG_BUD_06_MaxSizeBudgetIsSanitized) {
    AgenticRAGConfig cfg = makeFastBudgetConfig();
    cfg.max_session_tokens = std::numeric_limits<size_t>::max();
    cfg.max_iterations = 1;
    auto& agent = sharedBudgetAgent();
    agent.setConfig(cfg);

    const auto max_size = std::numeric_limits<size_t>::max();
    EXPECT_EQ(agent.getConfig().max_session_tokens, max_size - 1u)
        << "SIZE_MAX must be clamped to keep internal budget+1 sentinel representable";

    const auto docs = makeDocs(1, 32);
    const auto result = agent.run("q", docs);
    EXPECT_NE(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "Sanitized near-unlimited budget must not fail immediately for small inputs";
    EXPECT_GT(result.tokens_consumed, 0u);

    AgenticRAGConfig cfg2 = makeFastBudgetConfig();
    cfg2.max_session_tokens = max_size;
    cfg2.max_iterations = 1;
    agent.setConfig(cfg2);
    EXPECT_EQ(agent.getConfig().max_session_tokens, max_size - 1u)
        << "setConfig must apply the same budget sanitization";
}

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
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 4 (Severity: Medium/S1)
 * Tracked: src/rag/FUTURE_ENHANCEMENTS.md §"Session Token-Budget Cap for AgenticRAG"
 */

#include <gtest/gtest.h>

#include "rag/agentic_rag.h"

#include <string>
#include <vector>

using namespace themis::rag::agentic;

namespace {

// Helper: build a list of N fake documents, each with content of a given length.
std::vector<themis::rag::judge::RetrievedDocument> makeDocs(size_t count, size_t content_len = 100)
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
TEST(ARG_BUD, ARG_BUD_01_NoBudgetPreservesExistingBehaviour) {
    AgenticRAGConfig cfg;
    cfg.max_session_tokens = 0;   // disabled
    cfg.max_iterations     = 1;
    AgenticRAG agent(cfg);

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
TEST(ARG_BUD, ARG_BUD_02_TinyBudgetTriggersExceeded) {
    AgenticRAGConfig cfg;
    cfg.max_session_tokens = 1;   // every doc+query estimate exceeds this
    cfg.max_iterations     = 10;  // many iterations available, but budget stops it
    AgenticRAG agent(cfg);

    // A document with 100 chars → token estimate ≈ 26; already > 1.
    const auto docs   = makeDocs(1, 100);
    const auto result = agent.run("some query", docs);

    EXPECT_EQ(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "With max_session_tokens=1 and docs with 100 chars, BUDGET_EXCEEDED expected";
}

// ---------------------------------------------------------------------------
// ARG_BUD_03 – large budget → loop runs until natural stop
// ---------------------------------------------------------------------------
TEST(ARG_BUD, ARG_BUD_03_LargeBudgetDoesNotInterfere) {
    AgenticRAGConfig cfg;
    cfg.max_session_tokens = 1'000'000;  // effectively unlimited
    cfg.max_iterations     = 2;
    AgenticRAG agent(cfg);

    const auto docs   = makeDocs(2, 50);
    const auto result = agent.run("query", docs);

    // Should stop for some natural reason, not because of budget.
    EXPECT_NE(result.stop_reason, StopReason::BUDGET_EXCEEDED)
        << "Large budget must not cause premature BUDGET_EXCEEDED termination";
}

// ---------------------------------------------------------------------------
// ARG_BUD_04 – StopReason::BUDGET_EXCEEDED is a distinct enum value
// ---------------------------------------------------------------------------
TEST(ARG_BUD, ARG_BUD_04_BudgetExceededEnumDistinct) {
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
TEST(ARG_BUD, ARG_BUD_05_TokensConsumedPopulatedWithBudget) {
    AgenticRAGConfig cfg;
    cfg.max_session_tokens = 1'000'000;  // large — won't exceed
    cfg.max_iterations     = 1;
    AgenticRAG agent(cfg);

    // 3 docs with 40 chars each → token estimate per doc: 40/4+1 = 11.
    // Query "some query" → 10/4+1 = 3.
    // Total estimate >= 36.
    const auto docs   = makeDocs(3, 40);
    const auto result = agent.run("some query", docs);

    EXPECT_GT(result.tokens_consumed, 0u)
        << "tokens_consumed must be > 0 when max_session_tokens > 0 and docs are present";
}

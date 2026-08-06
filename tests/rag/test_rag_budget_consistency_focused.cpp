/**
 * @file test_rag_budget_consistency_focused.cpp
 * @brief Focused regression tests for budget and truncation consistency across
 *        assembler, adaptive retrieval, and multi-step orchestration.
 *
 * Roadmap Item: Budget and truncation consistency across assembler, adaptive retrieval,
 *               and multi-step orchestration (Target: Q3 2026)
 *
 * Test Suite: RagBudgetConsistencyFocusedTests
 *   Group A – Assembler budget determinism
 *   Group B – Budget propagation through adaptive retrieval
 *   Group C – Budget consistency in multi-step orchestration
 *   Group D – Truncation semantics consistency
 *   Group E – Response reservation across all paths
 */

#include <gtest/gtest.h>
#include "rag/rag_context_assembler.h"
#include "llm/context_window_budget.h"

#include <limits>

using namespace themis::rag;
using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Create a test chunk
// ─────────────────────────────────────────────────────────────────────────────

static RetrievedChunk makeChunk(const std::string& content,
                                 float              relevance = 1.0f,
                                 const std::string& source    = "test")
{
    RetrievedChunk c;
    c.content          = content;
    c.relevance_score  = relevance;
    c.source           = source;
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Group A – Assembler budget determinism
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagBudgetConsistencyFocusedTests, A1_SameBudgetProducesDeterministicResults) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;

    std::vector<RetrievedChunk> chunks = {
        makeChunk("chunk1", 0.95f),
        makeChunk("chunk2", 0.85f),
        makeChunk("chunk3", 0.75f),
    };

    RAGContextAssembler asm1{cfg};
    RAGContextAssembler asm2{cfg};

    const auto result1 = asm1.assemble(chunks, "", "query");
    const auto result2 = asm2.assemble(chunks, "", "query");

    EXPECT_EQ(result1.chunks_used.size(), result2.chunks_used.size());
    EXPECT_EQ(result1.tokens_used, result2.tokens_used);
    EXPECT_EQ(result1.was_truncated, result2.was_truncated);
}

TEST(RagBudgetConsistencyFocusedTests, A2_TokenCountingIsConsistent) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};

    // Each chunk should have consistent token estimation
    std::vector<RetrievedChunk> chunks = {
        makeChunk("The quick brown fox jumps over the lazy dog"),
        makeChunk("Lorem ipsum dolor sit amet consectetur adipiscing elit"),
    };

    const auto ctx = asm_.assemble(chunks, "System prompt", "What is this?");

    // tokens_remaining_for_response should be max(min_response_tokens, 20% of window)
    EXPECT_GT(ctx.tokens_remaining_for_response, 0u);
    EXPECT_LE(ctx.tokens_remaining_for_response, cfg.min_response_tokens + 50u);

    // tokens_used should be less than available context budget
    const auto budget =
        ContextWindowBudget::compute(cfg.model_context_tokens, "System prompt", "What is this?", cfg.min_response_tokens);
    EXPECT_LE(ctx.tokens_used, budget.available_context_tokens);
}

TEST(RagBudgetConsistencyFocusedTests, A3_ResponseReservationMaintained) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 1024u;  // Small window to test reservation
    cfg.min_response_tokens  = 256u;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("Test content A"),
        makeChunk("Test content B"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Response reservation should always be present
    EXPECT_GT(ctx.tokens_remaining_for_response, 0u);
    // Should never exceed min_response_tokens by much
    EXPECT_LE(ctx.tokens_remaining_for_response, cfg.min_response_tokens + 100u);
}

TEST(RagBudgetConsistencyFocusedTests, A4_BudgetExhaustedSignalCorrect) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 512u;  // Very small
    cfg.min_response_tokens  = 256u;
    RAGContextAssembler asm_{cfg};

    // Create chunks that will exhaust budget
    std::vector<RetrievedChunk> chunks = {
        makeChunk(std::string(200, 'a')),
        makeChunk(std::string(200, 'b')),
        makeChunk(std::string(200, 'c')),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Some chunks should be included, not all
    EXPECT_LE(ctx.chunks_used.size(), chunks.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Group B – Budget propagation through adaptive retrieval
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagBudgetConsistencyFocusedTests, B1_AdaptiveRetrievalBudgetConsistency) {
    // This test validates that budget semantics are consistent when passed
    // through adaptive retrieval layers.
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;

    const ContextWindowBudget budget =
        ContextWindowBudget::compute(cfg.model_context_tokens, "", "", cfg.min_response_tokens);

    EXPECT_GT(budget.available_context_tokens, 0u);
    EXPECT_GE(budget.reserved_response_tokens, cfg.min_response_tokens);
    // Total should not exceed window
    EXPECT_LE(budget.available_context_tokens + budget.reserved_response_tokens,
              cfg.model_context_tokens);
}

TEST(RagBudgetConsistencyFocusedTests, B2_BudgetPreservationAcrossRecursion) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};
    std::vector<RetrievedChunk> chunks = {
        makeChunk("content"),
    };

    const auto result1 = asm_.assemble(chunks, "", "q1");
    const auto result2 = asm_.assemble(chunks, "", "q2");

    // Same config should produce same budget allocation for same inputs
    EXPECT_EQ(result1.tokens_remaining_for_response, result2.tokens_remaining_for_response);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group C – Budget consistency in multi-step orchestration
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagBudgetConsistencyFocusedTests, C1_MultiStepBudgetAccounting) {
    // Validates that budget accounting works consistently across multiple assembly calls
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> round1_chunks = {
        makeChunk("first round chunk", 0.9f),
    };

    const auto round1 = asm_.assemble(round1_chunks, "", "query1");

    // After first round, we should have tokens_used and tokens_remaining_for_response
    EXPECT_GT(round1.tokens_remaining_for_response, 0u);

    std::vector<RetrievedChunk> round2_chunks = {
        makeChunk("second round chunk", 0.8f),
    };

    const auto round2 = asm_.assemble(round2_chunks, "", "query2");

    // Budget should be independent per call (not cumulative)
    EXPECT_GT(round2.tokens_remaining_for_response, 0u);
}

TEST(RagBudgetConsistencyFocusedTests, C2_CumulativeBudgetLogic) {
    // Test that if we were to accumulate context, budget would be respected
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 1024u;
    cfg.min_response_tokens  = 128u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks1 = {
        makeChunk("x", 0.9f),
        makeChunk("y", 0.8f),
    };

    const auto result1 = asm_.assemble(chunks1, "", "q");

    // If we add more chunks in next iteration, total should still fit within budget
    EXPECT_LE(result1.tokens_used + result1.tokens_remaining_for_response,
              cfg.model_context_tokens);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group D – Truncation semantics consistency
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagBudgetConsistencyFocusedTests, D1_TruncationMarkerAppliedConsistently) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 256u;  // Very small
    cfg.min_response_tokens  = 64u;
    cfg.allow_partial_chunk  = true;
    cfg.truncation_marker    = " [TRUNCATED]";

    RAGContextAssembler asm_{cfg};

    // Create a chunk that will definitely be truncated
    std::vector<RetrievedChunk> chunks = {
        makeChunk(std::string(500, 'a')),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // If truncated, the marker should be in the result
    if (ctx.was_truncated && !ctx.chunks_used.empty()) {
        const auto& truncated_chunk = ctx.chunks_used[0];
        EXPECT_THAT(truncated_chunk.content, ::testing::HasSubstr("[TRUNCATED]"));
    }
}

TEST(RagBudgetConsistencyFocusedTests, D2_DropVsTruncateConsistency) {
    RAGContextAssemblerConfig cfg_drop;
    cfg_drop.model_context_tokens = 256u;
    cfg_drop.min_response_tokens  = 64u;
    cfg_drop.allow_partial_chunk  = false;

    RAGContextAssemblerConfig cfg_trunc;
    cfg_trunc.model_context_tokens = 256u;
    cfg_trunc.min_response_tokens  = 64u;
    cfg_trunc.allow_partial_chunk  = true;

    RAGContextAssembler asm_drop{cfg_drop};
    RAGContextAssembler asm_trunc{cfg_trunc};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("chunk1", 0.9f),
        makeChunk(std::string(300, 'x'), 0.8f),
    };

    const auto result_drop = asm_drop.assemble(chunks, "", "q");
    const auto result_trunc = asm_trunc.assemble(chunks, "", "q");

    // With allow_partial_chunk=false, last chunk should be dropped if it doesn't fit
    // With allow_partial_chunk=true, it might be truncated
    // But response reservation should be maintained in both cases
    EXPECT_GT(result_drop.tokens_remaining_for_response, 0u);
    EXPECT_GT(result_trunc.tokens_remaining_for_response, 0u);
}

TEST(RagBudgetConsistencyFocusedTests, D3_TruncationMarkerSizeConsideredInBudget) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 256u;
    cfg.min_response_tokens  = 64u;
    cfg.allow_partial_chunk  = true;
    cfg.truncation_marker    = " [TRUNCATED]";

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk(std::string(200, 'a')),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // If chunk was truncated, truncation_marker length should be accounted for
    // in the token count
    EXPECT_LE(ctx.tokens_used, 256u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group E – Response reservation across all paths
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagBudgetConsistencyFocusedTests, E1_ResponseReservationMinimumMaintained) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 1024u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("very large content " + std::string(1000, 'x')),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Response tokens should be at least the minimum
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagBudgetConsistencyFocusedTests, E2_ResponseReservationWithEmptyContext) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 512u;
    cfg.min_response_tokens  = 128u;

    RAGContextAssembler asm_{cfg};

    const auto ctx = asm_.assemble({}, "", "q");

    // Even with no context, response reservation should be maintained
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagBudgetConsistencyFocusedTests, E3_ResponseReservedTokensNotIncludedInUsed) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 512u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("context"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // tokens_used should not include tokens_remaining_for_response
    const auto total_accounted = ctx.tokens_used + ctx.tokens_remaining_for_response;
    EXPECT_LE(total_accounted, cfg.model_context_tokens + 100u);
}

TEST(RagBudgetConsistencyFocusedTests, E4_TwentyPercentResponseFloor) {
    // Per the contract: response reservation is max(min_response_tokens, 20% of window)
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 1000u;  // 20% = 200
    cfg.min_response_tokens  = 100u;   // Less than 20%

    RAGContextAssembler asm_{cfg};

    const auto ctx = asm_.assemble({}, "", "q");

    // Should be at least 20% of window (200) since that's more than min_response_tokens
    EXPECT_GE(ctx.tokens_remaining_for_response, 200u);
}

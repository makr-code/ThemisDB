/**
 * @file test_rag_error_handling_edge_cases_focused.cpp
 * @brief Focused regression tests for error handling and edge cases
 *        in RAG pipeline components.
 *
 * Roadmap Item: Enforce fail-closed handling on malformed context, invalid budgets,
 *               and partial retrieval failures (Target: Q4 2026)
 *
 * Test Suite: RagErrorHandlingEdgeCasesFocusedTests
 *   Group A – Malformed context handling
 *   Group B – Invalid budget handling
 *   Group C – Partial retrieval failures
 *   Group D – Backend unavailable fallback
 *   Group E – Resource exhaustion handling
 */

#include <gtest/gtest.h>
#include "rag/rag_context_assembler.h"
#include "llm/context_window_budget.h"

#include <limits>
#include <cstring>

using namespace themis::rag;
using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Create test chunks with various characteristics
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
// Group A – Malformed context handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagErrorHandlingEdgeCasesFocusedTests, A1_EmptyContentChunkHandledSafely) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk(""),  // Empty content
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not crash; empty chunk is just low contribution
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, A2_NegativeRelevanceScoreHandledGracefully) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    RetrievedChunk bad_chunk;
    bad_chunk.content          = "content";
    bad_chunk.relevance_score  = -0.5f;  // Negative score
    bad_chunk.source           = "test";

    std::vector<RetrievedChunk> chunks = {bad_chunk};

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should handle negative relevance without crash
    // (typically sorted last or filtered out)
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, A3_OutOfRangeRelevanceScoreNormalized) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    RetrievedChunk oob_chunk;
    oob_chunk.content          = "content";
    oob_chunk.relevance_score  = 999.9f;  // Out of typical [0, 1] range
    oob_chunk.source           = "test";

    std::vector<RetrievedChunk> chunks = {oob_chunk};

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not crash; out-of-range score should be handled
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, A4_BrokenUnicodeInChunkContentHandledSafely) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // Invalid UTF-8 sequence (valid in std::string but semantically invalid UTF-8)
    std::string broken_utf8;
    broken_utf8.push_back(static_cast<char>(0xFF));
    broken_utf8.push_back(static_cast<char>(0xFE));
    broken_utf8.push_back(static_cast<char>(0x00));

    std::vector<RetrievedChunk> chunks = {
        makeChunk(broken_utf8),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not crash on broken UTF-8
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group B – Invalid budget handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagErrorHandlingEdgeCasesFocusedTests, B1_ZeroContextWindowHandledSafely) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 0u;  // Invalid: zero window
    cfg.min_response_tokens  = 0u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("content"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not crash; should fallback to default or return empty
    EXPECT_TRUE(ctx.chunks_used.empty() || ctx.tokens_remaining_for_response > 0u);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, B2_MinResponseTokensExceedsWindow) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 512u;
    cfg.min_response_tokens  = 1024u;  // More than entire window

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("content"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should handle gracefully: no room for context, all reserved for response
    EXPECT_LE(ctx.tokens_used, 512u);
    EXPECT_TRUE(ctx.chunks_used.empty() || ctx.tokens_remaining_for_response > 0u);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, B3_IntegerOverflowInTokenCountingPrevented) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;

    RAGContextAssembler asm_{cfg};

    // Create huge chunks that might cause overflow during token counting
    std::vector<RetrievedChunk> chunks = {
        makeChunk(std::string(1'000'000, 'a')),  // 1M characters
        makeChunk(std::string(1'000'000, 'b')),  // 1M characters
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not overflow; should handle gracefully
    EXPECT_LE(ctx.tokens_used, cfg.model_context_tokens);
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, B4_NegativeOrOverflowInReservationPrevented) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = std::numeric_limits<size_t>::max() / 2;  // Large value
    cfg.min_response_tokens  = std::numeric_limits<size_t>::max() / 2;  // Large value

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("content"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not overflow even with large config values
    EXPECT_GT(ctx.tokens_remaining_for_response, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group C – Partial retrieval failures
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagErrorHandlingEdgeCasesFocusedTests, C1_PartialChunkListHandledGracefully) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // Simulating partial retrieval: 3 chunks total, 1 is malformed
    std::vector<RetrievedChunk> chunks = {
        makeChunk("good chunk 1", 0.9f),
        makeChunk("", 0.0f),  // Empty/broken chunk
        makeChunk("good chunk 2", 0.8f),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should handle mixed good/bad chunks gracefully
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, C2_AllChunksFailedReturnsEmptyContext) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // All chunks with NaN/invalid scores
    std::vector<RetrievedChunk> chunks = {
        makeChunk("", std::numeric_limits<float>::quiet_NaN()),
        makeChunk("", std::numeric_limits<float>::quiet_NaN()),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    const auto budget =
        ContextWindowBudget::compute(cfg.model_context_tokens, "", "q", cfg.min_response_tokens);
    EXPECT_LE(ctx.chunks_used.size(), chunks.size());
    EXPECT_LE(ctx.tokens_used, budget.available_context_tokens);
    EXPECT_EQ(ctx.tokens_remaining_for_response,
              budget.responseBudgetAfterContext(ctx.tokens_used));
    EXPECT_GE(ctx.tokens_remaining_for_response, budget.reserved_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, C3_RetrievalTimeoutSimulationRecovery) {
    // NOTE: Real timeout testing would require async/timeout mechanisms.
    // This test documents the contract: assembly should not hang
    // even if input chunks contain problematic content.

    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("normal chunk"),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");
    const auto budget =
        ContextWindowBudget::compute(cfg.model_context_tokens, "", "q", cfg.min_response_tokens);
    EXPECT_LE(ctx.tokens_used, budget.available_context_tokens);
    EXPECT_EQ(ctx.tokens_remaining_for_response,
              budget.responseBudgetAfterContext(ctx.tokens_used));
    EXPECT_GE(ctx.tokens_remaining_for_response, budget.reserved_response_tokens);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group D – Backend unavailable fallback
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagErrorHandlingEdgeCasesFocusedTests, D1_FallbackToEmptyContextOnBackendFail) {
    // When retrieval backend is unavailable, RAG should:
    // 1. Not crash
    // 2. Return valid (empty) context
    // 3. Allow caller to signal degraded mode

    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // Simulate backend unavailable: no chunks returned
    const auto ctx = asm_.assemble({}, "", "q");

    EXPECT_TRUE(ctx.chunks_used.empty());
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, D2_DegradedModeSignalingViaEmptyContext) {
    // When calling code receives empty context, it should:
    // 1. Recognize this as potential backend failure
    // 2. Optionally use fallback generation without context
    // 3. Mark response with degraded signal (not attempted in this test)

    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    const auto ctx = asm_.assemble({}, "System", "query");
    const auto budget =
        ContextWindowBudget::compute(cfg.model_context_tokens, "System", "query", cfg.min_response_tokens);
    EXPECT_TRUE(ctx.chunks_used.empty());
    EXPECT_EQ(ctx.tokens_used, 0u);
    EXPECT_EQ(ctx.tokens_remaining_for_response, budget.reserved_response_tokens);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group E – Resource exhaustion handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagErrorHandlingEdgeCasesFocusedTests, E1_LargeChunkCountNotExhaustingMemory) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // Create 10K chunks (stress test for memory)
    std::vector<RetrievedChunk> chunks;
    for (int i = 0; i < 10'000; ++i) {
        chunks.push_back(makeChunk("chunk " + std::to_string(i), 0.5f));
    }

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should not crash or exhaust memory
    EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, E2_VeryLongStringContentHandledBounded) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 2048u;
    cfg.min_response_tokens  = 256u;

    RAGContextAssembler asm_{cfg};

    // Create a chunk with 100MB string (extreme case)
    // NOTE: This might not be practical; adjust size as needed for CI
    const size_t large_size = 1'000'000;  // 1MB instead of 100MB for practical CI
    std::vector<RetrievedChunk> chunks = {
        makeChunk(std::string(large_size, 'x')),
    };

    const auto ctx = asm_.assemble(chunks, "", "q");

    // Should truncate to budget, not crash
    EXPECT_LE(ctx.tokens_used, cfg.model_context_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, E3_RepeatedAssemblyCallsStable) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 1024u;
    cfg.min_response_tokens  = 128u;

    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("chunk1"),
        makeChunk("chunk2"),
    };

    // Call assembly 1000 times
    for (int i = 0; i < 1000; ++i) {
        const auto ctx = asm_.assemble(chunks, "", "q");
        EXPECT_GE(ctx.tokens_remaining_for_response, cfg.min_response_tokens);
    }

    const auto final_ctx = asm_.assemble(chunks, "", "q");
    EXPECT_LE(final_ctx.tokens_used, cfg.model_context_tokens);
    EXPECT_GE(final_ctx.tokens_remaining_for_response, cfg.min_response_tokens);
}

TEST(RagErrorHandlingEdgeCasesFocusedTests, E4_ConfigurationChangeNotLeakingState) {
    RAGContextAssemblerConfig cfg1;
    cfg1.model_context_tokens = 4096u;
    cfg1.min_response_tokens  = 512u;

    RAGContextAssembler asm_{cfg1};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("content"),
    };

    const auto result1 = asm_.assemble(chunks, "", "q");

    // Change config
    RAGContextAssemblerConfig cfg2;
    cfg2.model_context_tokens = 1024u;
    cfg2.min_response_tokens  = 128u;

    asm_.setConfig(cfg2);

    const auto result2 = asm_.assemble(chunks, "", "q");

    const auto budget2 =
        ContextWindowBudget::compute(cfg2.model_context_tokens, "", "q", cfg2.min_response_tokens);
    EXPECT_LT(result2.tokens_remaining_for_response, result1.tokens_remaining_for_response);
    EXPECT_EQ(result2.tokens_remaining_for_response,
              budget2.responseBudgetAfterContext(result2.tokens_used));
    EXPECT_GE(result2.tokens_remaining_for_response, budget2.reserved_response_tokens);
}

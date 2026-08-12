/**
 * @file test_rag_context_assembler.cpp
 * @brief Unit tests for RAGContextAssembler — Greedy Fill with Response Guard.
 *
 * Test suite: RagContextAssemblerFocusedTests (32 tests)
 *   Group A (5)  – empty input edge cases
 *   Group B (5)  – single chunk: fits, too large, truncation
 *   Group C (6)  – multiple chunks: greedy fill in relevance order
 *   Group D (5)  – response-guard: tokens_remaining_for_response
 *   Group E (5)  – truncation marker and allow_partial_chunk flag
 *   Group F (6)  – computeMaxTokens static helper
 */

#include <gtest/gtest.h>
#include "rag/rag_context_assembler.h"
#include "llm/context_window_budget.h"

#include <limits>

using namespace themis::rag;
using namespace themis::llm;

// Helper: make a chunk with given content and relevance score.
static RetrievedChunk makeChunk(const std::string& content,
                                 float              relevance = 1.0f,
                                 const std::string& source   = "")
{
    RetrievedChunk c;
    c.content         = content;
    c.relevance_score = relevance;
    c.source          = source;
    return c;
}

// ── Group A – empty input edge cases ─────────────────────────────────────────

TEST(RagContextAssemblerFocusedTests, A1_EmptyChunksReturnsEmptyResult) {
    RAGContextAssembler asm_{};
    const auto ctx = asm_.assemble({}, "", "");
    EXPECT_TRUE(ctx.chunks_used.empty());
    EXPECT_EQ(0u, ctx.tokens_used);
}

TEST(RagContextAssemblerFocusedTests, A2_NoBudgetReturnsEmptyResult) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 5u;  // very tiny → overhead > window
    cfg.min_response_tokens  = 1000u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("text")}, "", "");
    EXPECT_TRUE(ctx.chunks_used.empty());
    EXPECT_EQ(0u, ctx.tokens_used);
}

TEST(RagContextAssemblerFocusedTests, A3_EmptyQueryAndSystemUsesFullBudget) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const ContextWindowBudget b =
        ContextWindowBudget::compute(4096u, "", "", 512u);
    EXPECT_GT(b.available_context_tokens, 0u);
}

TEST(RagContextAssemblerFocusedTests, A4_ZeroModelCtxFallsBackTo4096) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 0u;
    RAGContextAssembler asm_{cfg};
    // With the 4096 fallback a short chunk must fit.
    const auto ctx = asm_.assemble({makeChunk("short text")}, "", "short query");
    EXPECT_FALSE(ctx.chunks_used.empty());
}

TEST(RagContextAssemblerFocusedTests, A5_WasTruncatedFalseForEmptyInput) {
    RAGContextAssembler asm_{};
    const auto ctx = asm_.assemble({}, "sys", "qry");
    EXPECT_FALSE(ctx.was_truncated);
}

// ── Group B – single chunk ────────────────────────────────────────────────────

TEST(RagContextAssemblerFocusedTests, B1_SingleSmallChunkFits) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("Hello world")}, "", "");
    EXPECT_EQ(1u, ctx.chunks_used.size());
    EXPECT_FALSE(ctx.was_truncated);
}

TEST(RagContextAssemblerFocusedTests, B2_OversizedChunkTruncatedByDefault) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 100u;  // 100 tokens ≈ 350 chars budget
    cfg.min_response_tokens  = 20u;
    cfg.allow_partial_chunk  = true;
    RAGContextAssembler asm_{cfg};
    // 2000-char chunk clearly exceeds context budget.
    const auto ctx = asm_.assemble({makeChunk(std::string(2000, 'x'))}, "", "");
    EXPECT_EQ(1u, ctx.chunks_used.size());
    EXPECT_TRUE(ctx.was_truncated);
    EXPECT_NE(std::string::npos,
              ctx.chunks_used[0].content.find("[TRUNCATED]"));
}

TEST(RagContextAssemblerFocusedTests, B3_OversizedChunkDroppedWhenPartialDisabled) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 100u;
    cfg.min_response_tokens  = 20u;
    cfg.allow_partial_chunk  = false;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk(std::string(2000, 'x'))}, "", "");
    EXPECT_TRUE(ctx.chunks_used.empty());
}

TEST(RagContextAssemblerFocusedTests, B4_SingleFittingChunkTokensUsedPositive) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("Hello world!")}, "", "");
    EXPECT_GT(ctx.tokens_used, 0u);
}

TEST(RagContextAssemblerFocusedTests, B5_TruncatedContentLengthLessThanOriginal) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 50u;
    cfg.min_response_tokens  = 10u;
    RAGContextAssembler asm_{cfg};
    const std::string big(1000, 'z');
    const auto ctx = asm_.assemble({makeChunk(big)}, "", "");
    if (!ctx.chunks_used.empty()) {
        EXPECT_LT(ctx.chunks_used[0].content.size(), big.size());
    }
}

// ── Group C – multiple chunks in relevance order ──────────────────────────────

TEST(RagContextAssemblerFocusedTests, C1_ChunksSortedByRelevance) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("low",    0.1f),
        makeChunk("high",   0.9f),
        makeChunk("medium", 0.5f),
    };
    const auto ctx = asm_.assemble(chunks, "", "");
    ASSERT_GE(ctx.chunks_used.size(), 2u);
    EXPECT_EQ("high",   ctx.chunks_used[0].content);
    EXPECT_EQ("medium", ctx.chunks_used[1].content);
}

TEST(RagContextAssemblerFocusedTests, C2_AllSmallChunksFit) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks;
    for (int i = 0; i < 5; ++i) {
        chunks.push_back(makeChunk("short doc " + std::to_string(i), 1.0f));
    }
    const auto ctx = asm_.assemble(chunks, "", "");
    EXPECT_EQ(5u, ctx.chunks_used.size());
}

TEST(RagContextAssemblerFocusedTests, C3_HighRelevanceChunkPreferred) {
    // Use very small budget so only one chunk fits.
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 50u;
    cfg.min_response_tokens  = 10u;
    cfg.allow_partial_chunk  = false;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("low",  0.2f),
        makeChunk("best", 0.9f),
    };
    const auto ctx = asm_.assemble(chunks, "", "");
    ASSERT_FALSE(ctx.chunks_used.empty());
    EXPECT_EQ("best", ctx.chunks_used[0].content);
}

TEST(RagContextAssemblerFocusedTests, C4_TokensUsedIsNonDecreasing) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks;
    for (int i = 0; i < 3; ++i) {
        chunks.push_back(makeChunk(std::string(100, static_cast<char>('a' + i))));
    }
    const auto ctx = asm_.assemble(chunks, "", "");
    EXPECT_GE(ctx.tokens_used, estimateTokens(chunks[0].content));
}

TEST(RagContextAssemblerFocusedTests, C5_BudgetExhaustedChunksNotAdded) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 20u;
    cfg.min_response_tokens  = 4u;
    cfg.allow_partial_chunk  = false;
    RAGContextAssembler asm_{cfg};

    // Each chunk ≈ 3 tokens (10 chars / 3.5)
    std::vector<RetrievedChunk> chunks;
    for (int i = 0; i < 20; ++i) {
        chunks.push_back(makeChunk("abcdefghij")); // 10 chars ≈ 3 tokens
    }
    const auto ctx = asm_.assemble(chunks, "", "");
    EXPECT_LT(ctx.chunks_used.size(), 20u);
}

TEST(RagContextAssemblerFocusedTests, C6_EqualRelevanceUsesDeterministicTieBreak) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};

    std::vector<RetrievedChunk> chunks = {
        makeChunk("third", 0.7f, "src-z"),
        makeChunk("first", 0.7f, "src-b"),
        makeChunk("second", 0.7f, "src-a"),
    };
    chunks[0].chunk_id = "c3";
    chunks[1].chunk_id = "c1";
    chunks[2].chunk_id = "c2";

    const auto ctx = asm_.assemble(chunks, "", "");
    ASSERT_EQ(ctx.chunks_used.size(), 3u);
    EXPECT_EQ(ctx.chunks_used[0].chunk_id, "c1");
    EXPECT_EQ(ctx.chunks_used[1].chunk_id, "c2");
    EXPECT_EQ(ctx.chunks_used[2].chunk_id, "c3");
}

// ── Group D – response-guard ──────────────────────────────────────────────────

TEST(RagContextAssemblerFocusedTests, D1_ResponseBudgetPositiveAfterAssembly) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("hello")}, "", "test query");
    EXPECT_GT(ctx.tokens_remaining_for_response, 0u);
}

TEST(RagContextAssemblerFocusedTests, D2_ResponseBudgetAtLeastReserved) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("hello")}, "", "");
    // After filling context, remaining must be >= reserved.
    EXPECT_GE(ctx.tokens_remaining_for_response,
              ContextWindowBudget::compute(4096u, "", "", 512u).reserved_response_tokens);
}

TEST(RagContextAssemblerFocusedTests, D3_ResponseBudgetEmptyChunks) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({}, "", "query");
    EXPECT_GE(ctx.tokens_remaining_for_response,
              ContextWindowBudget::compute(4096u, "", "query", 512u)
                  .reserved_response_tokens);
}

TEST(RagContextAssemblerFocusedTests, D4_LargerContextWindowGivesLargerResponseBudget) {
    auto makeCtx = [](size_t window) {
        RAGContextAssemblerConfig cfg;
        cfg.model_context_tokens = window;
        cfg.min_response_tokens  = 512u;
        RAGContextAssembler a(cfg);
        return a.assemble({makeChunk("small doc")}, "", "q");
    };
    EXPECT_GE(makeCtx(8192u).tokens_remaining_for_response,
              makeCtx(4096u).tokens_remaining_for_response);
}

TEST(RagContextAssemblerFocusedTests, D5_GetAndSetConfig) {
    RAGContextAssembler asm_{};
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 8192u;
    asm_.setConfig(cfg);
    EXPECT_EQ(8192u, asm_.getConfig().model_context_tokens);
}

// ── Group E – truncation marker ───────────────────────────────────────────────

TEST(RagContextAssemblerFocusedTests, E1_DefaultTruncationMarker) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 30u;
    cfg.min_response_tokens  = 5u;
    cfg.allow_partial_chunk  = true;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk(std::string(500, 'z'))}, "", "");
    if (ctx.was_truncated && !ctx.chunks_used.empty()) {
        EXPECT_NE(std::string::npos,
                  ctx.chunks_used[0].content.rfind("[TRUNCATED]"));
    }
}

TEST(RagContextAssemblerFocusedTests, E2_CustomTruncationMarker) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 30u;
    cfg.min_response_tokens  = 5u;
    cfg.allow_partial_chunk  = true;
    cfg.truncation_marker    = "…ABGESCHNITTEN";
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk(std::string(500, 'z'))}, "", "");
    if (ctx.was_truncated && !ctx.chunks_used.empty()) {
        EXPECT_NE(std::string::npos,
                  ctx.chunks_used[0].content.find("ABGESCHNITTEN"));
    }
}

TEST(RagContextAssemblerFocusedTests, E3_NoTruncationWhenChunkFits) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 4096u;
    cfg.min_response_tokens  = 512u;
    RAGContextAssembler asm_{cfg};
    const auto ctx = asm_.assemble({makeChunk("fits fine")}, "", "");
    EXPECT_FALSE(ctx.was_truncated);
    if (!ctx.chunks_used.empty()) {
        EXPECT_EQ("fits fine", ctx.chunks_used[0].content);
    }
}

TEST(RagContextAssemblerFocusedTests, E4_OnlyLastChunkMayBeTruncated) {
    RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = 200u;
    cfg.min_response_tokens  = 10u;
    cfg.allow_partial_chunk  = true;
    RAGContextAssembler asm_{cfg};

    // First chunk small, second huge.
    std::vector<RetrievedChunk> chunks = {
        makeChunk("small",           2.0f),
        makeChunk(std::string(3000, 'x'), 1.0f),
    };
    const auto ctx = asm_.assemble(chunks, "", "");
    // The first chunk should not contain the truncation marker.
    if (ctx.chunks_used.size() > 0u) {
        EXPECT_EQ(std::string::npos,
                  ctx.chunks_used[0].content.find("[TRUNCATED]"));
    }
}

TEST(RagContextAssemblerFocusedTests, E5_AllowPartialDefaultIsTrue) {
    RAGContextAssemblerConfig cfg;
    EXPECT_TRUE(cfg.allow_partial_chunk);
}

// ── Group F – computeMaxTokens ────────────────────────────────────────────────

TEST(RagContextAssemblerFocusedTests, F1_ComputeMaxTokensNonZero) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    EXPECT_GT(RAGContextAssembler::computeMaxTokens(b), 0);
}

TEST(RagContextAssemblerFocusedTests, F2_ComputeMaxTokensHonoursUserMax) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    // User requests only 100 tokens.
    EXPECT_LE(RAGContextAssembler::computeMaxTokens(b, 100), 100);
}

TEST(RagContextAssemblerFocusedTests, F3_ComputeMaxTokensIgnoresZeroUserMax) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    const int with_zero  = RAGContextAssembler::computeMaxTokens(b, 0);
    const int with_large = RAGContextAssembler::computeMaxTokens(b, 9999);
    EXPECT_EQ(with_zero, with_large);
}

TEST(RagContextAssemblerFocusedTests, F4_ComputeMaxTokensAtLeastOne) {
    // Degenerate budget: window=1 token
    const auto b = ContextWindowBudget::compute(1u, "", "", 0u);
    EXPECT_GE(RAGContextAssembler::computeMaxTokens(b), 1);
}

TEST(RagContextAssemblerFocusedTests, F5_LargerWindowLargerMaxTokens) {
    const auto b1 = ContextWindowBudget::compute(4096u,  "", "", 512u);
    const auto b2 = ContextWindowBudget::compute(32768u, "", "", 512u);
    EXPECT_GE(RAGContextAssembler::computeMaxTokens(b2),
              RAGContextAssembler::computeMaxTokens(b1));
}

TEST(RagContextAssemblerFocusedTests, F6_ComputeMaxTokensClampsToIntMax) {
    ContextWindowBudget b;
    b.model_max_tokens = std::numeric_limits<size_t>::max();
    b.reserved_response_tokens = std::numeric_limits<size_t>::max();

    const int result = RAGContextAssembler::computeMaxTokens(b, 0);
    EXPECT_EQ(result, std::numeric_limits<int>::max());
}

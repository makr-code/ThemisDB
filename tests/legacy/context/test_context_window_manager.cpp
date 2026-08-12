/**
 * @file test_context_window_manager.cpp
 * @brief Unit tests for ContextWindowBudgetManager (Issue: prompt_engineering
 *        Phase 2 — token counting and context-window budget enforcement).
 *
 * Acceptance criteria covered:
 *  AC-1  CharDivisionCounter — correct estimates, non-zero minimum, custom divisor.
 *  AC-2  ITokenCounter injection — pluggable counter used for all counts.
 *  AC-3  fitChunksInBudget(chunks, available_tokens) — greedy selection, budget
 *        boundary, first-chunk always included even when > budget.
 *  AC-4  fitChunksInBudget(chunks, system_prompt, query) — overhead deduction.
 *  AC-5  computeBudget() — correct per-section breakdown, utilisation ratio.
 *  AC-6  computeAndCheck() — returns allocation when fits, throws
 *        PromptBudgetExceededError with accurate metadata when over limit.
 *  AC-7  PromptBudgetExceededError — totalTokens / maxTokens / modelName accessors.
 *  AC-8  setUtilizationCallback() — invoked on both computeBudget and computeAndCheck.
 *  AC-9  ModelTokenBudget::promptBudget() — max_tokens minus reserved.
 *  AC-10 ModelTokenBudget::promptBudget() — saturates at 0 when reserved >= max.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/context_window_manager.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Helper: exact token counter (1 token per character, for deterministic tests)
// ============================================================================

class CharPerTokenCounter final : public ITokenCounter {
public:
    size_t count(const std::string& text) const override {
        return text.size();
    }
};

// Helper to build a RetrievedChunk with a given content length.
static RetrievedChunk makeChunk(const std::string& content,
                                double relevance = 1.0,
                                const std::string& source = "test") {
    RetrievedChunk c;
    c.content         = content;
    c.relevance_score = relevance;
    c.source          = source;
    return c;
}

// ============================================================================
// AC-1: CharDivisionCounter
// ============================================================================

TEST(CharDivisionCounterTest, EmptyStringReturnsZero) {
    CharDivisionCounter counter;
    EXPECT_EQ(counter.count(""), 0u);
}

TEST(CharDivisionCounterTest, FourCharsIsOneToken) {
    CharDivisionCounter counter;
    EXPECT_EQ(counter.count("abcd"), 1u);
}

TEST(CharDivisionCounterTest, EightCharsIsTwoTokens) {
    CharDivisionCounter counter;
    EXPECT_EQ(counter.count("abcdefgh"), 2u);
}

TEST(CharDivisionCounterTest, OddLengthRoundsUp) {
    CharDivisionCounter counter;
    // 5 chars / 4 = 1.25 → ceiling = 2
    EXPECT_EQ(counter.count("hello"), 2u);
}

TEST(CharDivisionCounterTest, SingleCharIsOneToken) {
    CharDivisionCounter counter;
    EXPECT_EQ(counter.count("x"), 1u);
}

TEST(CharDivisionCounterTest, CustomDivisorOfThree) {
    CharDivisionCounter counter(3);
    // 9 chars / 3 = 3 tokens
    EXPECT_EQ(counter.count("abcdefghi"), 3u);
}

TEST(CharDivisionCounterTest, DivisorZeroClampedToOne) {
    CharDivisionCounter counter(0);
    // divisor clamped to 1 → each char is one token
    EXPECT_EQ(counter.count("abc"), 3u);
}

// ============================================================================
// AC-9 & AC-10: ModelTokenBudget::promptBudget()
// ============================================================================

TEST(ModelTokenBudgetTest, PromptBudgetSubtractsReserved) {
    ModelTokenBudget b;
    b.max_tokens                = 4096;
    b.reserved_completion_tokens = 512;
    EXPECT_EQ(b.promptBudget(), 3584u);
}

TEST(ModelTokenBudgetTest, PromptBudgetSaturatesAtZero) {
    ModelTokenBudget b;
    b.max_tokens                = 256;
    b.reserved_completion_tokens = 512;
    EXPECT_EQ(b.promptBudget(), 0u);
}

TEST(ModelTokenBudgetTest, PromptBudgetExactlyZeroWhenEqual) {
    ModelTokenBudget b;
    b.max_tokens                = 512;
    b.reserved_completion_tokens = 512;
    EXPECT_EQ(b.promptBudget(), 0u);
}

// ============================================================================
// AC-2: pluggable ITokenCounter
// ============================================================================

TEST(ContextWindowBudgetManagerTest, DefaultCounterIsCharDivision) {
    ContextWindowBudgetManager mgr;
    // "abcdefgh" = 8 chars / 4 = 2 tokens
    EXPECT_EQ(mgr.countTokens("abcdefgh"), 2u);
}

TEST(ContextWindowBudgetManagerTest, CustomCounterUsedForCounting) {
    auto exact = std::make_shared<CharPerTokenCounter>();
    ContextWindowBudgetManager mgr({}, exact);
    // With char-per-token, "hello" = 5 tokens
    EXPECT_EQ(mgr.countTokens("hello"), 5u);
}

TEST(ContextWindowBudgetManagerTest, SetTokenCounterReplaces) {
    ContextWindowBudgetManager mgr;
    mgr.setTokenCounter(std::make_shared<CharPerTokenCounter>());
    EXPECT_EQ(mgr.countTokens("abcd"), 4u);  // not 1
}

// ============================================================================
// AC-3: fitChunksInBudget(chunks, available_tokens)
// ============================================================================

TEST(ContextWindowBudgetManagerTest, NoChunksReturnsEmpty) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    auto result = mgr.fitChunksInBudget({}, 100);
    EXPECT_TRUE(result.empty());
}

TEST(ContextWindowBudgetManagerTest, AllChunksFitInBudget) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = {
        makeChunk("abc"),   // 3 tokens
        makeChunk("defg"),  // 4 tokens
    };
    auto result = mgr.fitChunksInBudget(chunks, 100);
    EXPECT_EQ(result.size(), 2u);
}

TEST(ContextWindowBudgetManagerTest, SecondChunkExceedsBudgetIsDropped) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = {
        makeChunk("abc"),   // 3 tokens
        makeChunk("defgh"), // 5 tokens — 3+5=8 > 6
    };
    auto result = mgr.fitChunksInBudget(chunks, 6);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].content, "abc");
}

TEST(ContextWindowBudgetManagerTest, FirstChunkAlwaysIncludedEvenIfOverBudget) {
    // When selected is empty the break is not triggered — first chunk added regardless.
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = {
        makeChunk("verylongchunktext"),  // 17 tokens > budget of 5
        makeChunk("short"),              // 5 tokens
    };
    auto result = mgr.fitChunksInBudget(chunks, 5);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].content, "verylongchunktext");
}

TEST(ContextWindowBudgetManagerTest, ZeroBudgetYieldsFirstChunkOnly) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = {
        makeChunk("a"),
        makeChunk("b"),
    };
    auto result = mgr.fitChunksInBudget(chunks, 0);
    // 0 budget: first chunk always taken (selected is empty on entry), second dropped
    EXPECT_EQ(result.size(), 1u);
}

TEST(ContextWindowBudgetManagerTest, ExactBudgetFitIncludesChunk) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = {
        makeChunk("abcd"),   // 4 tokens
    };
    auto result = mgr.fitChunksInBudget(chunks, 4);
    EXPECT_EQ(result.size(), 1u);
}

// ============================================================================
// AC-4: fitChunksInBudget(chunks, system_prompt, query)
// ============================================================================

TEST(ContextWindowBudgetManagerTest, OverheadDeductedFromBudget) {
    ModelTokenBudget b;
    b.max_tokens                 = 20;   // prompt budget = 20 - 0 = 20
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    // system = 5 tokens, query = 5 tokens → context budget = 10
    std::vector<RetrievedChunk> chunks = {
        makeChunk("0123456789"),   // 10 tokens — exactly fits
        makeChunk("extra"),        // 5 tokens — would exceed 10
    };
    auto result = mgr.fitChunksInBudget(chunks, "12345", "12345");
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].content, "0123456789");
}

TEST(ContextWindowBudgetManagerTest, ZeroContextBudgetReturnsFirstChunkOnly) {
    ModelTokenBudget b;
    b.max_tokens                 = 5;
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    // system=3 + query=3 = 6 > max_tokens=5 → contextBudget clamped to 0
    std::vector<RetrievedChunk> chunks = {
        makeChunk("a"),
        makeChunk("b"),
    };
    auto result = mgr.fitChunksInBudget(chunks, "sys", "qry");
    EXPECT_EQ(result.size(), 1u);
}

// ============================================================================
// AC-5: computeBudget()
// ============================================================================

TEST(ContextWindowBudgetManagerTest, ComputeBudgetBreakdownCorrect) {
    ModelTokenBudget b;
    b.model_name                 = "test-model";
    b.max_tokens                 = 100;
    b.reserved_completion_tokens = 10;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    std::vector<RetrievedChunk> chunks = {
        makeChunk("chunk"),  // 5 tokens
    };
    auto alloc = mgr.computeBudget("system", "query", chunks);
    // system=6, query=5, context=5 → total=16, budget=90
    EXPECT_EQ(alloc.system_tokens,  6u);
    EXPECT_EQ(alloc.query_tokens,   5u);
    EXPECT_EQ(alloc.context_tokens, 5u);
    EXPECT_EQ(alloc.total_tokens,  16u);
    EXPECT_EQ(alloc.budget_tokens, 90u);
    EXPECT_TRUE(alloc.fits());
    EXPECT_NEAR(alloc.utilization, 16.0 / 90.0, 1e-9);
}

TEST(ContextWindowBudgetManagerTest, ComputeBudgetOverLimitFitsReturnsFalse) {
    ModelTokenBudget b;
    b.max_tokens                 = 5;
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    std::vector<RetrievedChunk> chunks = { makeChunk("long") };  // 4 tokens
    // system="sys"=3, query="q"=1, context=4 → total=8 > budget=5
    auto alloc = mgr.computeBudget("sys", "q", chunks);
    EXPECT_FALSE(alloc.fits());
    EXPECT_GT(alloc.utilization, 1.0);
}

TEST(ContextWindowBudgetManagerTest, ComputeBudgetEmptyChunksWorks) {
    ContextWindowBudgetManager mgr;
    auto alloc = mgr.computeBudget("", "", {});
    EXPECT_EQ(alloc.context_tokens, 0u);
    EXPECT_EQ(alloc.total_tokens, 0u);
    EXPECT_TRUE(alloc.fits());
}

// ============================================================================
// AC-6 & AC-7: computeAndCheck() + PromptBudgetExceededError
// ============================================================================

TEST(ContextWindowBudgetManagerTest, ComputeAndCheckReturnsFitsAllocation) {
    ModelTokenBudget b;
    b.max_tokens                 = 1000;
    b.reserved_completion_tokens = 100;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    auto alloc = mgr.computeAndCheck("system", "query", {});
    EXPECT_TRUE(alloc.fits());
}

TEST(ContextWindowBudgetManagerTest, ComputeAndCheckThrowsWhenOverLimit) {
    ModelTokenBudget b;
    b.model_name                 = "tiny-model";
    b.max_tokens                 = 4;
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    // "hello"=5 > budget=4 → throw
    EXPECT_THROW(mgr.computeAndCheck("hello", "", {}),
                 PromptBudgetExceededError);
}

TEST(ContextWindowBudgetManagerTest, BudgetExceededErrorAccessors) {
    ModelTokenBudget b;
    b.model_name                 = "nano";
    b.max_tokens                 = 3;
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    try {
        mgr.computeAndCheck("abcd", "", {});  // 4 > 3
        FAIL() << "Expected PromptBudgetExceededError";
    } catch (const PromptBudgetExceededError& e) {
        EXPECT_EQ(e.totalTokens(), 4u);
        EXPECT_EQ(e.maxTokens(),   3u);
        EXPECT_EQ(e.modelName(), "nano");
        // what() must contain relevant info
        std::string msg = e.what();
        EXPECT_NE(msg.find("nano"), std::string::npos);
    }
}

TEST(ContextWindowBudgetManagerTest, BudgetExceededErrorIsRuntimeError) {
    PromptBudgetExceededError err(10, 8, "model-x");
    EXPECT_THROW(throw err, std::runtime_error);
}

// ============================================================================
// AC-8: setUtilizationCallback()
// ============================================================================

TEST(ContextWindowBudgetManagerTest, UtilizationCallbackInvokedOnComputeBudget) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());

    double captured = -1.0;
    mgr.setUtilizationCallback([&captured](double u) { captured = u; });

    mgr.computeBudget("sys", "qry", {});
    EXPECT_GE(captured, 0.0);
}

TEST(ContextWindowBudgetManagerTest, UtilizationCallbackInvokedOnComputeAndCheck) {
    ModelTokenBudget b;
    b.max_tokens                 = 1000;
    b.reserved_completion_tokens = 0;
    ContextWindowBudgetManager mgr(b, std::make_shared<CharPerTokenCounter>());

    double captured = -1.0;
    mgr.setUtilizationCallback([&captured](double u) { captured = u; });

    mgr.computeAndCheck("hi", "there", {});
    EXPECT_GE(captured, 0.0);
}

TEST(ContextWindowBudgetManagerTest, UtilizationCallbackNotInvokedWhenUnset) {
    ContextWindowBudgetManager mgr;
    // Should not crash when no callback is registered
    EXPECT_NO_THROW(mgr.computeBudget("x", "y", {}));
}

// ============================================================================
// getModel / setModel round-trip
// ============================================================================

TEST(ContextWindowBudgetManagerTest, SetModelUpdatesConfiguration) {
    ContextWindowBudgetManager mgr;

    ModelTokenBudget b;
    b.model_name                 = "gpt-4";
    b.max_tokens                 = 8192;
    b.reserved_completion_tokens = 1024;
    mgr.setModel(b);

    EXPECT_EQ(mgr.getModel().model_name, "gpt-4");
    EXPECT_EQ(mgr.getModel().max_tokens, 8192u);
    EXPECT_EQ(mgr.getModel().promptBudget(), 7168u);
}

// ============================================================================
// Edge case: zero-length chunks in batch
// ============================================================================

TEST(ContextWindowBudgetManagerTest, EmptyContentChunkCountedAsZeroTokens) {
    ContextWindowBudgetManager mgr({}, std::make_shared<CharPerTokenCounter>());
    std::vector<RetrievedChunk> chunks = { makeChunk("") };
    auto alloc = mgr.computeBudget("", "", chunks);
    EXPECT_EQ(alloc.context_tokens, 0u);
}

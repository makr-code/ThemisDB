/**
 * @file test_context_window_budget.cpp
 * @brief Unit tests for ContextWindowBudget and token estimation utilities.
 *
 * Test suite: ContextWindowBudgetFocusedTests (30 tests)
 *   Group A (5)  – estimateTokens: empty, short, typical, long, char-count overload
 *   Group B (5)  – tokensToChars: round-trip, zero, typical values
 *   Group C (5)  – ContextWindowBudget::compute: defaults, model_ctx=0, overhead > model
 *   Group D (5)  – reserved_response_tokens enforcement (20% floor, custom min)
 *   Group E (5)  – available_context_tokens arithmetic
 *   Group F (5)  – availableContextChars, hasContextBudget, responseBudgetAfterContext
 */

#include <gtest/gtest.h>
#include "llm/context_window_budget.h"

using namespace themis::llm;

// ── Group A – estimateTokens (string overload) ────────────────────────────────

TEST(ContextWindowBudgetFocusedTests, A1_EmptyStringReturnsZero) {
    EXPECT_EQ(0u, estimateTokens(""));
}

TEST(ContextWindowBudgetFocusedTests, A2_ShortStringAtLeastOne) {
    // "Hi" = 2 chars → ceil(2/3.5) = 1
    EXPECT_EQ(1u, estimateTokens("Hi"));
}

TEST(ContextWindowBudgetFocusedTests, A3_TypicalSentence) {
    // 35 chars → ceil(35/3.5) = 10
    const std::string s(35, 'x');
    EXPECT_EQ(10u, estimateTokens(s));
}

TEST(ContextWindowBudgetFocusedTests, A4_LongText) {
    // 700 chars → ceil(700/3.5) = 200
    const std::string s(700, 'a');
    EXPECT_EQ(200u, estimateTokens(s));
}

TEST(ContextWindowBudgetFocusedTests, A5_CharCountOverload) {
    // char_count overload and string overload agree
    const std::string s(42, 'x');
    EXPECT_EQ(estimateTokens(s), estimateTokens(42u));
}

// ── Group B – tokensToChars ───────────────────────────────────────────────────

TEST(ContextWindowBudgetFocusedTests, B1_ZeroTokensIsZeroChars) {
    EXPECT_EQ(0u, tokensToChars(0u));
}

TEST(ContextWindowBudgetFocusedTests, B2_TenTokensIsThirtyFiveChars) {
    EXPECT_EQ(35u, tokensToChars(10u));
}

TEST(ContextWindowBudgetFocusedTests, B3_RoundTripConservative) {
    // estimateTokens(tokensToChars(n)) >= n  (heuristic is conservative)
    const size_t n = 100u;
    EXPECT_GE(estimateTokens(tokensToChars(n)), n);
}

TEST(ContextWindowBudgetFocusedTests, B4_OneThousandTokensToChars) {
    EXPECT_EQ(3500u, tokensToChars(1000u));
}

TEST(ContextWindowBudgetFocusedTests, B5_LargeTokenCount) {
    EXPECT_EQ(static_cast<size_t>(4096.0 * kCharsPerTokenHeuristic),
              tokensToChars(4096u));
}

// ── Group C – ContextWindowBudget::compute defaults & edge cases ──────────────

TEST(ContextWindowBudgetFocusedTests, C1_DefaultContextWith4096Window) {
    const auto b = ContextWindowBudget::compute(4096u, "", "");
    EXPECT_EQ(4096u, b.model_max_tokens);
}

TEST(ContextWindowBudgetFocusedTests, C2_ZeroModelCtxFallsBackToDefault) {
    const auto b = ContextWindowBudget::compute(0u, "", "");
    EXPECT_EQ(kDefaultContextWindowTokens, b.model_max_tokens);
}

TEST(ContextWindowBudgetFocusedTests, C3_EmptyPromptAndQueryZeroOverhead) {
    const auto b = ContextWindowBudget::compute(4096u, "", "");
    EXPECT_EQ(0u, b.system_prompt_tokens);
    EXPECT_EQ(0u, b.query_tokens);
}

TEST(ContextWindowBudgetFocusedTests, C4_OverheadExceedsWindowClampsToZero) {
    // Tiny 10-token window with 512 min_response → overhead > window
    const auto b = ContextWindowBudget::compute(10u, "", "", 512u);
    EXPECT_EQ(0u, b.available_context_tokens);
}

TEST(ContextWindowBudgetFocusedTests, C5_LargeWindowHasPositiveBudget) {
    const auto b = ContextWindowBudget::compute(32768u, "System.", "Query.", 512u);
    EXPECT_GT(b.available_context_tokens, 0u);
}

// ── Group D – reserved_response_tokens enforcement ───────────────────────────

TEST(ContextWindowBudgetFocusedTests, D1_Default512MinRespTokensForLargeWindow) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    EXPECT_GE(b.reserved_response_tokens, 512u);
}

TEST(ContextWindowBudgetFocusedTests, D2_TwentyPercentFloorApplied) {
    // 4096 * 0.20 = 819.2 → floor = 820 (ceil). min_response=100 < floor.
    const auto b = ContextWindowBudget::compute(4096u, "", "", 100u);
    const size_t expected_floor = static_cast<size_t>(
        std::ceil(4096.0 * 0.20));
    EXPECT_GE(b.reserved_response_tokens, expected_floor);
}

TEST(ContextWindowBudgetFocusedTests, D3_LargeMinResponseHonoured) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 2000u);
    EXPECT_GE(b.reserved_response_tokens, 2000u);
}

TEST(ContextWindowBudgetFocusedTests, D4_SmallWindowFloorIsEnsured) {
    // Window = 100, 20% = 20 tokens min response
    const auto b = ContextWindowBudget::compute(100u, "", "", 0u);
    EXPECT_GE(b.reserved_response_tokens, static_cast<size_t>(std::ceil(100.0 * 0.20)));
}

TEST(ContextWindowBudgetFocusedTests, D5_ReservedNeverExceedsModelMax) {
    // For a tiny window the reserved_response_tokens == model_max_tokens
    // (available_context_tokens will be 0, not negative).
    const auto b = ContextWindowBudget::compute(512u, "", "", 512u);
    EXPECT_LE(b.reserved_response_tokens, b.model_max_tokens);
}

// ── Group E – available_context_tokens arithmetic ────────────────────────────

TEST(ContextWindowBudgetFocusedTests, E1_AvailableIsModelMinusOverhead) {
    const std::string sys = std::string(35, 'a'); // ~10 tokens
    const std::string qry = std::string(35, 'b'); // ~10 tokens
    const auto b = ContextWindowBudget::compute(4096u, sys, qry, 512u);
    const size_t overhead =
        b.system_prompt_tokens + b.query_tokens + b.reserved_response_tokens;
    EXPECT_EQ(b.available_context_tokens, b.model_max_tokens - overhead);
}

TEST(ContextWindowBudgetFocusedTests, E2_NeverNegative) {
    const auto b = ContextWindowBudget::compute(5u, "too long for this window",
                                                "also long", 512u);
    EXPECT_EQ(0u, b.available_context_tokens);
}

TEST(ContextWindowBudgetFocusedTests, E3_ExactFitLeavesZeroBudget) {
    // Reserve exactly all tokens for response+overhead: no context tokens left.
    const size_t w = 1024u;
    // system + query = 0; min_response > w → available_context_tokens = 0
    const auto b = ContextWindowBudget::compute(w, "", "", w + 1u);
    EXPECT_EQ(0u, b.available_context_tokens);
}

TEST(ContextWindowBudgetFocusedTests, E4_IncreasingWindowIncreasesAvailable) {
    const auto b1 = ContextWindowBudget::compute(4096u,  "", "hello", 512u);
    const auto b2 = ContextWindowBudget::compute(8192u, "", "hello", 512u);
    EXPECT_GT(b2.available_context_tokens, b1.available_context_tokens);
}

TEST(ContextWindowBudgetFocusedTests, E5_LongerSystemPromptReducesAvailable) {
    const auto b1 = ContextWindowBudget::compute(4096u, "short", "q", 512u);
    const auto b2 = ContextWindowBudget::compute(4096u, std::string(700,'x'), "q", 512u);
    EXPECT_LT(b2.available_context_tokens, b1.available_context_tokens);
}

// ── Group F – helper methods ──────────────────────────────────────────────────

TEST(ContextWindowBudgetFocusedTests, F1_HasContextBudgetTrueWhenPositive) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    EXPECT_TRUE(b.hasContextBudget());
}

TEST(ContextWindowBudgetFocusedTests, F2_HasContextBudgetFalseWhenZero) {
    const auto b = ContextWindowBudget::compute(10u, "", "", 1000u);
    EXPECT_FALSE(b.hasContextBudget());
}

TEST(ContextWindowBudgetFocusedTests, F3_AvailableContextCharsIsTokensTimesFactor) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    EXPECT_EQ(tokensToChars(b.available_context_tokens),
              b.availableContextChars());
}

TEST(ContextWindowBudgetFocusedTests, F4_ResponseBudgetAfterContextReturnsAtLeastReserved) {
    const auto b = ContextWindowBudget::compute(4096u, "", "query", 512u);
    // If we used ALL context tokens, response budget is still reserved.
    EXPECT_GE(b.responseBudgetAfterContext(b.available_context_tokens),
              b.reserved_response_tokens);
}

TEST(ContextWindowBudgetFocusedTests, F5_ResponseBudgetClampsWhenOverUsed) {
    const auto b = ContextWindowBudget::compute(4096u, "", "", 512u);
    // Passing more context_tokens_used than model_max_tokens should return reserved.
    EXPECT_EQ(b.reserved_response_tokens,
              b.responseBudgetAfterContext(b.model_max_tokens + 9999u));
}

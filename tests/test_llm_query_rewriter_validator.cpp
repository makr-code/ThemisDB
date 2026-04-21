/**
 * @file test_llm_query_rewriter_validator.cpp
 * @brief Unit tests for LlmQueryRewriter semantic output validator (Gap 2 / search).
 *
 * Tests
 * -----
 * LQR_VAL_01  Semantically valid rewrites (high overlap) are accepted; quality=OK
 * LQR_VAL_02  Semantically nonsensical rewrite (zero overlap) is discarded;
 *             quality=FALLBACK; original query returned in rewrites
 * LQR_VAL_03  Overlap threshold is configurable (0.0 disables the filter)
 * LQR_VAL_04  Mixed rewrites: valid ones kept, invalid ones discarded
 * LQR_VAL_05  RewriteQuality::OK / FALLBACK enum values exist and are distinct
 * LQR_VAL_06  No backend: quality stays OK (existing fallback path unchanged)
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 2 (Severity: Medium/S2)
 * Tracked: src/search/FUTURE_ENHANCEMENTS.md §Gap 2
 */

#include <gtest/gtest.h>
#include "search/llm_query_rewriter.h"
#include <string>
#include <vector>

using namespace themis;

namespace {

// Build a backend that returns the given lines as numbered rewrites.
LlmQueryRewriter::LlmBackend makeBackend(const std::vector<std::string>& rewrites) {
    return [rewrites](const std::string& /*prompt*/) -> std::string {
        std::string out;
        for (size_t i = 0; i < rewrites.size(); ++i) {
            out += std::to_string(i + 1) + ". " + rewrites[i] + "\n";
        }
        return out;
    };
}

} // namespace

// ---------------------------------------------------------------------------
// LQR_VAL_05 — enum values exist and are distinct
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_05_EnumValuesDistinct) {
    EXPECT_NE(static_cast<int>(RewriteQuality::OK),
              static_cast<int>(RewriteQuality::FALLBACK));
}

// ---------------------------------------------------------------------------
// LQR_VAL_06 — no backend: quality stays OK (error/no-backend paths unchanged)
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_06_NoBackendQualityOK) {
    LlmQueryRewriter::Config cfg;
    cfg.min_token_overlap_ratio = 0.2f;
    LlmQueryRewriter rewriter(cfg, nullptr);

    const auto result = rewriter.rewrite("fast database insert");

    // Without a backend, llm_used == false and the original should be returned.
    EXPECT_FALSE(result.llm_used);
    EXPECT_EQ(result.quality, RewriteQuality::OK)
        << "quality must stay OK when no backend is involved (no validator run)";
}

// ---------------------------------------------------------------------------
// LQR_VAL_01 — valid rewrites (high overlap) → quality=OK
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_01_ValidRewriteAccepted) {
    // These rewrites share "database" + "insert" with the query.
    const std::vector<std::string> good_rewrites = {
        "high-throughput database insertion",
        "quick record insert in database",
    };

    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 2;
    cfg.min_token_overlap_ratio = 0.1f; // low enough for these to pass
    LlmQueryRewriter rewriter(cfg, makeBackend(good_rewrites));

    const auto result = rewriter.rewrite("fast database insert");

    EXPECT_TRUE(result.llm_used);
    EXPECT_EQ(result.quality, RewriteQuality::OK)
        << "quality must be OK when LLM rewrites pass the overlap threshold";
    EXPECT_FALSE(result.rewrites.empty())
        << "Rewrites must not be empty when they pass validation";
}

// ---------------------------------------------------------------------------
// LQR_VAL_02 — nonsensical rewrite (zero overlap) → quality=FALLBACK
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_02_NonsensicalRewriteFallback) {
    // Completely unrelated rewrite — shares no tokens with the query.
    const std::vector<std::string> bad_rewrites = {
        "xyz pqr lmno tuv",   // no shared tokens
    };

    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 1;
    cfg.min_token_overlap_ratio = 0.2f;
    cfg.fallback_to_original = true;
    LlmQueryRewriter rewriter(cfg, makeBackend(bad_rewrites));

    const auto result = rewriter.rewrite("fast database insert");

    EXPECT_TRUE(result.llm_used);
    EXPECT_EQ(result.quality, RewriteQuality::FALLBACK)
        << "quality must be FALLBACK when all LLM rewrites fail the overlap check";
    // Original query should be in rewrites due to fallback_to_original=true.
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "fast database insert")
        << "Fallback must return the original query";
}

// ---------------------------------------------------------------------------
// LQR_VAL_03 — overlap threshold 0.0 disables the filter
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_03_ZeroThresholdDisablesFilter) {
    // Completely unrelated rewrite — would be filtered at threshold=0.2.
    const std::vector<std::string> unrelated = {
        "xyz pqr lmno tuv",
    };

    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 1;
    cfg.min_token_overlap_ratio = 0.0f; // disabled
    LlmQueryRewriter rewriter(cfg, makeBackend(unrelated));

    const auto result = rewriter.rewrite("fast database insert");

    EXPECT_TRUE(result.llm_used);
    EXPECT_EQ(result.quality, RewriteQuality::OK)
        << "When threshold==0.0, filter is disabled and quality must be OK";
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "xyz pqr lmno tuv")
        << "Unfiltered rewrite must be returned as-is";
}

// ---------------------------------------------------------------------------
// LQR_VAL_04 — mixed rewrites: valid kept, invalid discarded
// ---------------------------------------------------------------------------
TEST(LQR_VAL, LQR_VAL_04_MixedRewritesFilteredCorrectly) {
    // First rewrite shares "database" with query → valid.
    // Second rewrite is totally unrelated → invalid.
    const std::vector<std::string> mixed = {
        "database write performance",  // shares "database"
        "red apple green banana",      // no shared tokens
    };

    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 2;
    cfg.min_token_overlap_ratio = 0.1f;
    LlmQueryRewriter rewriter(cfg, makeBackend(mixed));

    const auto result = rewriter.rewrite("fast database insert");

    EXPECT_TRUE(result.llm_used);
    EXPECT_EQ(result.quality, RewriteQuality::OK)
        << "quality must be OK because at least one rewrite survived";
    // Only the first rewrite should survive.
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "database write performance");
}

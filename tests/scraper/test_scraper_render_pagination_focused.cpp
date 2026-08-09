// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_render_pagination_focused.cpp
 * @brief Sub-Agent 2 focused tests: JS-Render timeout path and API pagination
 *        limit enforcement.
 *
 * Test IDs: SCR-21 through SCR-24
 * No file I/O, no network, deterministic only.
 *
 * @see include/scraper/scraper_render_contract.h
 * @see include/scraper/scraper_api_contract.h
 * @see include/scraper/scraper_js_renderer.h
 */

#include "gtest/gtest.h"
#include "scraper/scraper_render_contract.h"

#include <string>

namespace themis {
namespace scraper {
namespace test {

// ============================================================================
// SCR-21 — JS-Render timeout detection
// ============================================================================

/**
 * @brief A result whose elapsed time exceeds the budget AND whose error string
 *        contains "timeout" must produce kRenderTimeout with html cleared.
 */
TEST(ScraperRenderPagination, SCR21_JsRenderTimeoutDetection) {
    JsRenderResult r;
    r.success    = false;
    r.elapsed_ms = 30001;                // 1 ms past the 30 000 ms budget
    r.error      = "process timeout";   // also contains "timeout"
    r.html       = "<partial>data</partial>"; // must be cleared

    const JsRenderContractResult result = enforceRenderTimeout(r, 30000);

    EXPECT_TRUE(result.timed_out)
        << "timed_out must be true when elapsed_ms >= timeout_ms";
    EXPECT_EQ(result.error, ScraperError::kRenderTimeout)
        << "error code must be kRenderTimeout on timeout";
    EXPECT_TRUE(result.render_result.html.empty())
        << "html must be cleared on timeout";
}

// ============================================================================
// SCR-22 — JS-Render failure (non-timeout) maps to kFetchFailed
// ============================================================================

/**
 * @brief A failed render that is not a timeout (elapsed well below budget,
 *        no "timeout" keyword in error) must map to kFetchFailed.
 */
TEST(ScraperRenderPagination, SCR22_JsRenderNonTimeoutFailure) {
    JsRenderResult r;
    r.success    = false;
    r.elapsed_ms = 100;                    // well under 30 000 ms budget
    r.error      = "connection refused";   // no "timeout" keyword
    r.html       = "";                     // already empty, stays empty

    const JsRenderContractResult result = enforceRenderTimeout(r, 30000);

    EXPECT_FALSE(result.timed_out)
        << "timed_out must be false for a non-timeout failure";
    EXPECT_EQ(result.error, ScraperError::kFetchFailed)
        << "error code must be kFetchFailed for non-timeout failures";
    EXPECT_TRUE(result.render_result.html.empty())
        << "html must be empty (was already empty)";
}

// ============================================================================
// SCR-23 — Pagination limit at exact boundary and adjacents
// ============================================================================

/**
 * @brief isPaginationLimitReached must return true when current_page equals
 *        max_depth (inclusive), and false strictly below.
 */
TEST(ScraperRenderPagination, SCR23_PaginationLimitBoundary) {
    // Exactly at limit: current == max → reached
    EXPECT_TRUE(isPaginationLimitReached(50u, 50u))
        << "current_page == max_depth must return true";

    // One below limit: current < max → not reached
    EXPECT_FALSE(isPaginationLimitReached(49u, 50u))
        << "current_page < max_depth must return false";

    // One above limit: current > max → reached (already past)
    EXPECT_TRUE(isPaginationLimitReached(51u, 50u))
        << "current_page > max_depth must return true";
}

// ============================================================================
// SCR-24 — Successful render passes through unchanged
// ============================================================================

/**
 * @brief A successful render result must be returned with kSuccess, no timeout
 *        flag, and the original html content intact.
 */
TEST(ScraperRenderPagination, SCR24_SuccessfulRenderPassThrough) {
    const std::string kOriginalHtml = "<html>content</html>";

    JsRenderResult r;
    r.success    = true;
    r.html       = kOriginalHtml;
    r.elapsed_ms = 500;
    r.error      = "";   // no error on success

    const JsRenderContractResult result = enforceRenderTimeout(r, 30000);

    EXPECT_EQ(result.error, ScraperError::kSuccess)
        << "error must be kSuccess when render succeeded";
    EXPECT_FALSE(result.timed_out)
        << "timed_out must be false when render succeeded";
    EXPECT_EQ(result.render_result.html, kOriginalHtml)
        << "html must be preserved unchanged on success";
}

} // namespace test
} // namespace scraper
} // namespace themis

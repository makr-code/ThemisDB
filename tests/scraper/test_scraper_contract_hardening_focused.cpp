// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the scraper module.
 *
 * Test IDs: SCR-01 through SCR-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/scraper/scraper_api_contract.h
 * @see src/scraper/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "scraper/scraper_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace scraper {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// SCR-01 — Error code uniqueness
// ============================================================================

TEST(ScraperContractHardening, SCR01_ErrorCodeUniqueness) {
    std::set<int32_t> seen;
    const int32_t codes[] = {
        static_cast<int32_t>(ScraperError::kFetchFailed),
        static_cast<int32_t>(ScraperError::kRenderTimeout),
        static_cast<int32_t>(ScraperError::kParseError),
        static_cast<int32_t>(ScraperError::kEvaluationFailed),
        static_cast<int32_t>(ScraperError::kMetadataWriteFailed),
        static_cast<int32_t>(ScraperError::kSourceNotFound),
        static_cast<int32_t>(ScraperError::kPaginationLimit),
        static_cast<int32_t>(ScraperError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 8u);
    (void)kSeed;
}

// ============================================================================
// SCR-02 — Error code range [8500, 8599]
// ============================================================================

TEST(ScraperContractHardening, SCR02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(ScraperError::kFetchFailed),
        static_cast<int32_t>(ScraperError::kRenderTimeout),
        static_cast<int32_t>(ScraperError::kParseError),
        static_cast<int32_t>(ScraperError::kEvaluationFailed),
        static_cast<int32_t>(ScraperError::kMetadataWriteFailed),
        static_cast<int32_t>(ScraperError::kSourceNotFound),
        static_cast<int32_t>(ScraperError::kPaginationLimit),
        static_cast<int32_t>(ScraperError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8500) << "Code " << c << " below reserved base 8500";
        EXPECT_LE(c, 8599) << "Code " << c << " above reserved max 8599";
    }
}

// ============================================================================
// SCR-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(ScraperContractHardening, SCR03_SwitchDispatch) {
    auto describe = [](ScraperError e) -> const char* {
        switch (e) {
            case ScraperError::kSuccess:             return "success";
            case ScraperError::kFetchFailed:         return "fetch_failed";
            case ScraperError::kRenderTimeout:       return "render_timeout";
            case ScraperError::kParseError:          return "parse_error";
            case ScraperError::kEvaluationFailed:    return "evaluation_failed";
            case ScraperError::kMetadataWriteFailed: return "metadata_write_failed";
            case ScraperError::kSourceNotFound:      return "source_not_found";
            case ScraperError::kPaginationLimit:     return "pagination_limit";
            case ScraperError::kInternalError:       return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(ScraperError::kSuccess),             "success");
    EXPECT_STREQ(describe(ScraperError::kFetchFailed),         "fetch_failed");
    EXPECT_STREQ(describe(ScraperError::kMetadataWriteFailed), "metadata_write_failed");
    EXPECT_STREQ(describe(ScraperError::kInternalError),       "internal_error");
}

// ============================================================================
// SCR-04 — ScrapeRequest default values
// ============================================================================

TEST(ScraperContractHardening, SCR04_ScrapeRequestDefaults) {
    ScrapeRequest req;
    EXPECT_TRUE(req.source_url.empty());
    EXPECT_FALSE(req.enable_js_render);
    EXPECT_EQ(req.max_pagination_depth, kDefaultMaxPaginationDepth);
    EXPECT_EQ(req.render_timeout, kDefaultRenderTimeout);
}

// ============================================================================
// SCR-05 — ScrapeResult default values
// ============================================================================

TEST(ScraperContractHardening, SCR05_ScrapeResultDefaults) {
    ScrapeResult result;
    EXPECT_TRUE(result.source_url.empty());
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ScraperError::kSuccess);
    EXPECT_EQ(result.pages_scraped, 0u);
    EXPECT_TRUE(result.metadata_record_id.empty());
}

// ============================================================================
// SCR-06 — Copy semantics for ScrapeRequest
// ============================================================================

TEST(ScraperContractHardening, SCR06_ScrapeRequestCopy) {
    ScrapeRequest src;
    src.source_url       = "https://example.com";
    src.enable_js_render = true;
    src.max_pagination_depth = 10;

    ScrapeRequest copy = src;
    EXPECT_EQ(copy.source_url,           src.source_url);
    EXPECT_EQ(copy.enable_js_render,     src.enable_js_render);
    EXPECT_EQ(copy.max_pagination_depth, src.max_pagination_depth);
}

// ============================================================================
// SCR-07 — Move semantics for ScrapeResult
// ============================================================================

TEST(ScraperContractHardening, SCR07_ScrapeResultMove) {
    ScrapeResult src;
    src.source_url        = "https://example.com/move";
    src.success           = true;
    src.metadata_record_id = "rec-42";

    ScrapeResult moved = std::move(src);
    EXPECT_EQ(moved.source_url,         "https://example.com/move");
    EXPECT_TRUE(moved.success);
    EXPECT_EQ(moved.metadata_record_id, "rec-42");
}

// ============================================================================
// SCR-08 — isScraperFailClosed predicate
// ============================================================================

TEST(ScraperContractHardening, SCR08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isScraperFailClosed(ScraperError::kEvaluationFailed));
    EXPECT_TRUE(isScraperFailClosed(ScraperError::kInternalError));
    EXPECT_TRUE(isScraperFailClosed(ScraperError::kMetadataWriteFailed));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kSuccess));
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kFetchFailed));
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kRenderTimeout));
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kParseError));
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kSourceNotFound));
    EXPECT_FALSE(isScraperFailClosed(ScraperError::kPaginationLimit));
}

} // namespace test
} // namespace scraper
} // namespace themis

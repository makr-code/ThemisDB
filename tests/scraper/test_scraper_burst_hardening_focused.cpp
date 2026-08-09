// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_burst_hardening_focused.cpp
 * @brief Sub-Agent 1 focused tests for BurstCrawlController.
 *
 * Test IDs: SCR-17 through SCR-20
 * No file I/O, no network, deterministic only.
 *
 * @see include/scraper/scraper_burst_controller.h
 */

#include "gtest/gtest.h"
#include "scraper/scraper_burst_controller.h"

namespace themis {
namespace scraper {
namespace test {

// ============================================================================
// SCR-17 — max_tokens consumable: all 5 acquires succeed
// ============================================================================

TEST(ScraperBurstHardening, SCR17_MaxTokensConsumable) {
    BurstCrawlController ctrl(5, 1.0);

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(ctrl.tryAcquire())
            << "Expected acquire #" << (i + 1) << " to succeed";
    }
}

// ============================================================================
// SCR-18 — exhaustion: 6th call returns false
// ============================================================================

TEST(ScraperBurstHardening, SCR18_ExhaustionReturnsFalse) {
    BurstCrawlController ctrl(5, 0.0); // rate=0 so no automatic refill

    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(ctrl.tryAcquire());
    }

    EXPECT_FALSE(ctrl.tryAcquire())
        << "Expected 6th acquire to fail on an empty bucket";
}

// ============================================================================
// SCR-19 — burstUtilization() after 2 acquires from 5-token bucket
// ============================================================================

TEST(ScraperBurstHardening, SCR19_UtilizationAfterTwoAcquires) {
    BurstCrawlController ctrl(5, 0.0); // rate=0 prevents background refill

    ASSERT_TRUE(ctrl.tryAcquire());
    ASSERT_TRUE(ctrl.tryAcquire());

    // 3 tokens remain out of 5  →  utilization = 0.6
    const double utilization = ctrl.burstUtilization();
    EXPECT_NEAR(utilization, 0.6, 1e-9)
        << "Expected 3/5 = 0.6, got " << utilization;
}

// ============================================================================
// SCR-20 — full depletion gives 0.0; reset() restores 1.0
// ============================================================================

TEST(ScraperBurstHardening, SCR20_DepletionAndReset) {
    BurstCrawlController ctrl(5, 0.0);

    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(ctrl.tryAcquire());
    }

    EXPECT_NEAR(ctrl.burstUtilization(), 0.0, 1e-9)
        << "Expected fully-depleted bucket to report 0.0 utilization";

    ctrl.reset();

    EXPECT_NEAR(ctrl.burstUtilization(), 1.0, 1e-9)
        << "Expected reset() to restore full capacity (utilization 1.0)";
}

} // namespace test
} // namespace scraper
} // namespace themis

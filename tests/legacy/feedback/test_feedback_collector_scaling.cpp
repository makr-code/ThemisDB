/**
 * @file test_feedback_collector_scaling.cpp
 * @brief Tests for FeedbackCollector paging, outlier detection, and audit checksum (issue 2.2)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/feedback_collector.h"

using namespace themis::prompt_engineering;

class FeedbackCollectorScalingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Populate 20 feedback entries for "prompt_a"
        for (int i = 0; i < 20; ++i) {
            collector_.recordFeedback(
                "prompt_a",
                "query " + std::to_string(i),
                "response " + std::to_string(i),
                (i % 5 == 0) ? FeedbackType::USER_POSITIVE : FeedbackType::USER_NEGATIVE,
                "",
                0.5
            );
        }
    }
    FeedbackCollector collector_;
};

// ============================================================================
// getFeedbackPaged – basic paging
// ============================================================================

TEST_F(FeedbackCollectorScalingTest, Paging_FirstPage) {
    auto page = collector_.getFeedbackPaged("prompt_a", 0, 5);
    EXPECT_EQ(page.size(), 5u);
}

TEST_F(FeedbackCollectorScalingTest, Paging_SecondPage) {
    auto page1 = collector_.getFeedbackPaged("prompt_a", 0, 5);
    auto page2 = collector_.getFeedbackPaged("prompt_a", 5, 5);
    EXPECT_EQ(page2.size(), 5u);
    // Pages must not overlap
    for (const auto& e1 : page1) {
        for (const auto& e2 : page2) {
            EXPECT_NE(e1.id, e2.id);
        }
    }
}

TEST_F(FeedbackCollectorScalingTest, Paging_AllEntries) {
    // page_size=0 means return all remaining
    auto all = collector_.getFeedbackPaged("prompt_a", 0, 0);
    EXPECT_EQ(all.size(), 20u);
}

TEST_F(FeedbackCollectorScalingTest, Paging_OffsetBeyondEnd) {
    auto page = collector_.getFeedbackPaged("prompt_a", 100, 5);
    EXPECT_TRUE(page.empty());
}

TEST_F(FeedbackCollectorScalingTest, Paging_UnknownPrompt) {
    auto page = collector_.getFeedbackPaged("no_such_prompt", 0, 5);
    EXPECT_TRUE(page.empty());
}

TEST_F(FeedbackCollectorScalingTest, Paging_WithTypeFilter) {
    // Only USER_POSITIVE entries (every 5th, i.e. 4 out of 20)
    auto page = collector_.getFeedbackPaged("prompt_a", 0, 100,
                                             FeedbackType::USER_POSITIVE);
    EXPECT_EQ(page.size(), 4u);
    for (const auto& e : page) {
        EXPECT_EQ(e.type, FeedbackType::USER_POSITIVE);
    }
}

// ============================================================================
// detectOutliers
// ============================================================================

TEST(FeedbackCollectorOutlierTest, NoOutliersWhenAllSame) {
    FeedbackCollector col;
    for (int i = 0; i < 10; ++i) {
        col.recordFeedback("p1", "q", "r", FeedbackType::USER_NEGATIVE, "", 0.5);
    }
    auto outliers = col.detectOutliers("p1", 2.0);
    EXPECT_TRUE(outliers.empty());
}

TEST(FeedbackCollectorOutlierTest, DetectsExtremeOutlier) {
    FeedbackCollector col;
    // Nine entries at severity 0.5
    for (int i = 0; i < 9; ++i) {
        col.recordFeedback("p2", "q", "r", FeedbackType::USER_NEGATIVE, "", 0.5);
    }
    // One extreme outlier at severity 1.0
    col.recordFeedback("p2", "extreme", "r", FeedbackType::USER_NEGATIVE, "", 1.0);

    auto outliers = col.detectOutliers("p2", 2.0);
    EXPECT_GE(outliers.size(), 1u);
    EXPECT_NEAR(outliers[0].severity, 1.0, 1e-9);
}

TEST(FeedbackCollectorOutlierTest, UnknownPromptReturnsEmpty) {
    FeedbackCollector col;
    auto outliers = col.detectOutliers("no_prompt", 2.0);
    EXPECT_TRUE(outliers.empty());
}

// ============================================================================
// Audit checksum
// ============================================================================

TEST(FeedbackCollectorChecksumTest, ChecksumNonEmpty) {
    FeedbackCollector col;
    auto id = col.recordFeedback("p3", "q", "r", FeedbackType::USER_POSITIVE, "", 0.8);
    auto entries = col.getFeedback("p3");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_FALSE(entries[0].checksum.empty());
}

TEST(FeedbackCollectorChecksumTest, ChecksumConsistent) {
    FeedbackEntry e;
    e.id = "test_id";
    e.prompt_id = "prompt_x";
    e.type = FeedbackType::USER_POSITIVE;
    e.query = "hello";
    e.severity = 0.7;
    e.timestamp = std::chrono::system_clock::now();

    std::string c1 = e.computeChecksum();
    std::string c2 = e.computeChecksum();
    EXPECT_EQ(c1, c2);
}

TEST(FeedbackCollectorChecksumTest, ChecksumDiffersOnChange) {
    FeedbackEntry e;
    e.id = "a";
    e.prompt_id = "p";
    e.type = FeedbackType::USER_NEGATIVE;
    e.query = "q1";
    e.severity = 0.3;
    e.timestamp = std::chrono::system_clock::now();

    std::string c1 = e.computeChecksum();
    e.severity = 0.9;
    std::string c2 = e.computeChecksum();
    EXPECT_NE(c1, c2);
}

/**
 * @file test_governance_review_scheduler.cpp
 * @brief Tests for ReviewScheduler (src/governance/review_scheduler.cpp)
 *        plus ReviewRequest / ReviewSchedule JSON round-trips.
 *
 * Covers:
 *   - ReviewRequest::toJson / fromJson round-trip
 *   - ReviewSchedule::toJson / fromJson round-trip
 *   - ReviewScheduler construction
 *   - configureReviewSchedule
 *   - createReviewRequest → returns non-empty id
 *   - getPendingReviews → newly created request appears
 *   - approveReview → request moves out of pending
 *   - rejectReview → request moves out of pending
 *   - getOverdueReviews (empty when none are overdue)
 *   - getReviewHistory (returns completed reviews)
 *   - exportReviews / importReviews round-trip
 *   - getExpirationInfo (returns JSON object)
 */

#include <gtest/gtest.h>
#include "governance/review_scheduler.h"
#include "governance/policy_manager.h"
#include <chrono>
#include <memory>
#include <algorithm>

using namespace themis::governance;

// ============================================================================
// Helpers
// ============================================================================

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ============================================================================
// ReviewRequest JSON round-trip
// ============================================================================

TEST(ReviewRequestTest, ToJson_FromJson_RoundTrip) {
    ReviewRequest r;
    r.review_id   = "rev-001";
    r.rule_id     = "rule-abc";
    r.requester   = "alice";
    r.requested_at = nowMs();
    r.due_date    = r.requested_at + 7LL * 24 * 3600 * 1000;
    r.status      = "pending";
    r.reviewer    = "";
    r.reviewed_at = 0;
    r.comments    = "";

    auto j       = r.toJson();
    auto r2      = ReviewRequest::fromJson(j);

    EXPECT_EQ(r2.review_id,    r.review_id);
    EXPECT_EQ(r2.rule_id,      r.rule_id);
    EXPECT_EQ(r2.requester,    r.requester);
    EXPECT_EQ(r2.status,       r.status);
    EXPECT_EQ(r2.requested_at, r.requested_at);
    EXPECT_EQ(r2.due_date,     r.due_date);
}

// ============================================================================
// ReviewSchedule JSON round-trip
// ============================================================================

TEST(ReviewScheduleTest, ToJson_FromJson_RoundTrip) {
    ReviewSchedule s;
    s.rule_id           = "rule-xyz";
    s.review_period_days = 30;
    s.last_review_date  = nowMs() - 1LL * 24 * 3600 * 1000;
    s.next_review_date  = nowMs() + 29LL * 24 * 3600 * 1000;
    s.auto_schedule     = true;

    auto j  = s.toJson();
    auto s2 = ReviewSchedule::fromJson(j);

    EXPECT_EQ(s2.rule_id,            s.rule_id);
    EXPECT_EQ(s2.review_period_days, s.review_period_days);
    EXPECT_EQ(s2.auto_schedule,      s.auto_schedule);
    EXPECT_EQ(s2.last_review_date,   s.last_review_date);
    EXPECT_EQ(s2.next_review_date,   s.next_review_date);
}

// ============================================================================
// ReviewScheduler fixture
// ============================================================================

class ReviewSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_manager_ = std::make_shared<PolicyManager>();
        scheduler_      = std::make_unique<ReviewScheduler>(policy_manager_);
    }

    std::shared_ptr<PolicyManager> policy_manager_;
    std::unique_ptr<ReviewScheduler> scheduler_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(ReviewSchedulerTest, Construction_Succeeds) {
    EXPECT_NE(scheduler_, nullptr);
    EXPECT_TRUE(scheduler_->getPendingReviews().empty());
}

// ============================================================================
// configureReviewSchedule
// ============================================================================

TEST_F(ReviewSchedulerTest, ConfigureReviewSchedule_DoesNotThrow) {
    EXPECT_NO_THROW(scheduler_->configureReviewSchedule("rule-01", 90));
}

// ============================================================================
// createReviewRequest
// ============================================================================

TEST_F(ReviewSchedulerTest, CreateReviewRequest_ReturnsNonEmptyId) {
    auto id = scheduler_->createReviewRequest("rule-01", "bob", /*due_days=*/7);
    EXPECT_FALSE(id.empty());
}

TEST_F(ReviewSchedulerTest, CreateReviewRequest_AppearsInPending) {
    auto id = scheduler_->createReviewRequest("rule-02", "carol", 14);
    auto pending = scheduler_->getPendingReviews();
    bool found = std::any_of(pending.begin(), pending.end(), [&id](const ReviewRequest& r) {
        return r.review_id == id;
    });
    EXPECT_TRUE(found);
}

TEST_F(ReviewSchedulerTest, CreateMultipleRequests_AllInPending) {
    auto id1 = scheduler_->createReviewRequest("rule-A", "u1", 7);
    auto id2 = scheduler_->createReviewRequest("rule-B", "u2", 7);
    auto pending = scheduler_->getPendingReviews();
    EXPECT_GE(pending.size(), 2u);
    // Both IDs should be unique
    EXPECT_NE(id1, id2);
}

// ============================================================================
// approveReview
// ============================================================================

TEST_F(ReviewSchedulerTest, ApproveReview_MovesOutOfPending) {
    auto id = scheduler_->createReviewRequest("rule-03", "dave", 7);
    scheduler_->approveReview(id, "manager", "LGTM");

    auto pending = scheduler_->getPendingReviews();
    bool still_pending = std::any_of(pending.begin(), pending.end(),
                                      [&id](const ReviewRequest& r) {
                                          return r.review_id == id;
                                      });
    EXPECT_FALSE(still_pending);
}

// ============================================================================
// rejectReview
// ============================================================================

TEST_F(ReviewSchedulerTest, RejectReview_MovesOutOfPending) {
    auto id = scheduler_->createReviewRequest("rule-04", "eve", 7);
    scheduler_->rejectReview(id, "manager", "Needs revision");

    auto pending = scheduler_->getPendingReviews();
    bool still_pending = std::any_of(pending.begin(), pending.end(),
                                      [&id](const ReviewRequest& r) {
                                          return r.review_id == id;
                                      });
    EXPECT_FALSE(still_pending);
}

// ============================================================================
// getOverdueReviews
// ============================================================================

TEST_F(ReviewSchedulerTest, GetOverdueReviews_NoneOverdue_ReturnsEmpty) {
    // Create a review with a distant due date — should not be overdue
    scheduler_->createReviewRequest("rule-future", "frank", /*due_days=*/365);
    auto overdue = scheduler_->getOverdueReviews();
    EXPECT_TRUE(overdue.empty());
}

// ============================================================================
// getReviewHistory
// ============================================================================

TEST_F(ReviewSchedulerTest, GetReviewHistory_AfterApproval_ContainsReview) {
    auto id = scheduler_->createReviewRequest("rule-history", "grace", 7);
    scheduler_->approveReview(id, "auditor", "");
    auto history = scheduler_->getReviewHistory("rule-history");
    EXPECT_FALSE(history.empty());
}

// ============================================================================
// exportReviews / importReviews
// ============================================================================

TEST_F(ReviewSchedulerTest, ExportImport_RoundTrip) {
    auto id = scheduler_->createReviewRequest("rule-export", "hank", 7);
    (void)id;

    auto exported = scheduler_->exportReviews();
    EXPECT_TRUE(exported.is_object() || exported.is_array());

    // A fresh scheduler should be able to import
    ReviewScheduler fresh_scheduler(policy_manager_);
    EXPECT_TRUE(fresh_scheduler.importReviews(exported));

    auto restored_pending = fresh_scheduler.getPendingReviews();
    EXPECT_FALSE(restored_pending.empty());
}

// ============================================================================
// getExpirationInfo
// ============================================================================

TEST_F(ReviewSchedulerTest, GetExpirationInfo_ReturnsJsonObject) {
    scheduler_->configureReviewSchedule("rule-exp", 90);
    auto info = scheduler_->getExpirationInfo("rule-exp");
    EXPECT_TRUE(info.is_object());
}

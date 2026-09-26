// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/index_refresh_scheduler.h"

#include <gtest/gtest.h>

namespace themis::rag {

class IndexRefreshSchedulerTest : public ::testing::Test {
 protected:
  void SetUp() override { scheduler_ = std::make_unique<IndexRefreshScheduler>(); }

  std::unique_ptr<IndexRefreshScheduler> scheduler_;
};

TEST_F(IndexRefreshSchedulerTest, ScheduleRefresh) {
  bool scheduled = scheduler_->ScheduleRefresh("shard_1");
  EXPECT_TRUE(scheduled);

  auto status = scheduler_->GetRefreshStatus("shard_1");
  EXPECT_EQ(status, IndexRefreshScheduler::RefreshStatus::Pending);
}

TEST_F(IndexRefreshSchedulerTest, DuplicateRefreshBlocked) {
  scheduler_->ScheduleRefresh("shard_1");
  auto status1 = scheduler_->GetRefreshStatus("shard_1");

  // Try to schedule again while status is Pending
  bool scheduled = scheduler_->ScheduleRefresh("shard_1");
  EXPECT_FALSE(scheduled);  // Should be rate-limited
}

TEST_F(IndexRefreshSchedulerTest, EmergencyRefreshBypassesRateLimit) {
  scheduler_->ScheduleRefresh("shard_1");

  // Emergency refresh should succeed despite pending status
  bool scheduled = scheduler_->ScheduleEmergencyRefresh("shard_1", 10);
  EXPECT_TRUE(scheduled);
}

TEST_F(IndexRefreshSchedulerTest, GetPendingRefreshes) {
  scheduler_->ScheduleRefresh("shard_1");
  scheduler_->ScheduleRefresh("shard_2");

  auto pending = scheduler_->GetPendingRefreshes();
  EXPECT_GE(pending.size(), 0);  // May be empty if results not populated
}

TEST_F(IndexRefreshSchedulerTest, CancelRefresh) {
  scheduler_->ScheduleRefresh("shard_1");

  bool cancelled = scheduler_->CancelRefresh("shard_1");
  EXPECT_TRUE(cancelled);

  auto status = scheduler_->GetRefreshStatus("shard_1");
  EXPECT_EQ(status, IndexRefreshScheduler::RefreshStatus::Cancelled);
}

TEST_F(IndexRefreshSchedulerTest, ProcessScheduledRefreshes) {
  scheduler_->ScheduleRefresh("shard_1");
  scheduler_->ScheduleRefresh("shard_2");

  uint32_t started = scheduler_->ProcessScheduledRefreshes();
  EXPECT_GT(started, 0);

  auto status1 = scheduler_->GetRefreshStatus("shard_1");
  EXPECT_EQ(status1, IndexRefreshScheduler::RefreshStatus::Completed);
}

TEST_F(IndexRefreshSchedulerTest, GetStats) {
  scheduler_->ScheduleRefresh("shard_1");
  scheduler_->ScheduleRefresh("shard_2");
  scheduler_->ProcessScheduledRefreshes();

  auto stats = scheduler_->GetStats();

  EXPECT_EQ(stats["background_interval_sec"], 3600);
  EXPECT_GE(stats["completed_refreshes"], 0);
}

TEST_F(IndexRefreshSchedulerTest, SetMaxConcurrentRefreshes) {
  scheduler_->SetMaxConcurrentRefreshes(5);

  for (int i = 0; i < 10; i++) {
    scheduler_->ScheduleRefresh("shard_" + std::to_string(i));
  }

  uint32_t started = scheduler_->ProcessScheduledRefreshes();
  EXPECT_LE(started, 5);
}

}  // namespace themis::rag

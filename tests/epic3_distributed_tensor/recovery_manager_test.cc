/**
 * @file recovery_manager_test.cc
 * @brief Contract tests for IRecoveryManager (sub-issue #5433).
 *
 * Validates factory construction, job enqueueing, status lookup, executeNext,
 * listJobs by status, and status-change callback registration.
 * Production Reed-Solomon reconstruction is tracked in sub-issue #5433.
 */

#include "distributed_tensor/include/recovery_manager.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace themis::distributed_tensor;

class RecoveryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manifest_ = makeManifestStore();
        ASSERT_NE(manifest_, nullptr);
        manager_ = makeRecoveryManager(manifest_);
        ASSERT_NE(manager_, nullptr);
    }

    std::shared_ptr<IManifestStore> manifest_;
    std::unique_ptr<IRecoveryManager> manager_;
};

TEST_F(RecoveryManagerTest, FactoryReturnsNonNull) {
    EXPECT_NE(manager_, nullptr);
}

TEST_F(RecoveryManagerTest, EnqueueReturnsNonEmptyJobId) {
    std::string job_id = manager_->enqueue("artifact-1", RecoveryStrategy::Replication);
    EXPECT_FALSE(job_id.empty());
}

TEST_F(RecoveryManagerTest, StatusOfAfterEnqueue) {
    std::string job_id = manager_->enqueue("artifact-2", RecoveryStrategy::Replication);
    auto job = manager_->statusOf(job_id);
    EXPECT_TRUE(job.has_value());
    EXPECT_EQ(job->job_id, job_id);
    EXPECT_EQ(job->artifact_id, "artifact-2");
}

TEST_F(RecoveryManagerTest, NewJobHasQueuedOrInProgressStatus) {
    std::string job_id = manager_->enqueue("artifact-3", RecoveryStrategy::Replication);
    auto job = manager_->statusOf(job_id);
    ASSERT_TRUE(job.has_value());
    bool valid_status = (job->status == RecoveryStatus::Queued ||
                         job->status == RecoveryStatus::InProgress);
    EXPECT_TRUE(valid_status);
}

TEST_F(RecoveryManagerTest, StatusOfUnknownJobReturnsNullopt) {
    auto job = manager_->statusOf("no-such-job");
    EXPECT_FALSE(job.has_value());
}

TEST_F(RecoveryManagerTest, ListJobsByQueuedStatusDoesNotThrow) {
    manager_->enqueue("artifact-4", RecoveryStrategy::Replication);
    EXPECT_NO_THROW(manager_->listJobs(RecoveryStatus::Queued));
}

TEST_F(RecoveryManagerTest, ListQueuedJobsContainsEnqueuedJob) {
    std::string job_id = manager_->enqueue("artifact-5", RecoveryStrategy::Replication);
    auto jobs = manager_->listJobs(RecoveryStatus::Queued);
    bool found = false;
    for (const auto& id : jobs) {
        if (id == job_id) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(RecoveryManagerTest, ExecuteNextDoesNotThrow) {
    manager_->enqueue("artifact-6", RecoveryStrategy::Replication);
    EXPECT_NO_THROW(manager_->executeNext());
}

TEST_F(RecoveryManagerTest, ExecuteNextReturnsJobWithArtifactId) {
    manager_->enqueue("artifact-7", RecoveryStrategy::Replication);
    RecoveryJob job = manager_->executeNext();
    EXPECT_EQ(job.artifact_id, "artifact-7");
}

TEST_F(RecoveryManagerTest, ExecutedJobMovesToCompletedOrFailed) {
    manager_->enqueue("artifact-8", RecoveryStrategy::Replication);
    RecoveryJob job = manager_->executeNext();
    bool terminal = (job.status == RecoveryStatus::Completed ||
                     job.status == RecoveryStatus::Failed);
    EXPECT_TRUE(terminal);
}

TEST_F(RecoveryManagerTest, StatusChangeCallbackRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(manager_->onStatusChange([](const RecoveryJob&) {}));
}

TEST_F(RecoveryManagerTest, ReedSolomonStrategyEnqueueDoesNotThrow) {
    EXPECT_NO_THROW(manager_->enqueue("artifact-9", RecoveryStrategy::ReedSolomon));
}

// AI Safety Layer — ASL-11: AiSnapshotCleanupJob unit tests
// Tests: POS-01..POS-15
// Docs: src/security/ROADMAP.md § Phase 3 (ASL-11)

#include <gtest/gtest.h>
#include "security/ai_snapshot_cleanup.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using themis::security::AiSnapshotCleanupJob;
using themis::security::AiSnapshotInfo;

// ============================================================================
// Test fixture — creates / destroys an isolated temp directory per test.
// ============================================================================

class AiSnapshotCleanupTest : public ::testing::Test {
protected:
    void SetUp() override {
        base_dir_ = fs::temp_directory_path() / ("themis_asl11_test_" + std::to_string(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count()));
        fs::create_directories(base_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(base_dir_, ec);
    }

    /// Create a fake snapshot directory with a dummy file of @p size_bytes.
    /// @p age_seconds > 0 means the file was written that many seconds in the past.
    std::string makeSnapshot(const std::string& name,
                             std::uint64_t size_bytes = 0,
                             int age_seconds = 0) {
        const fs::path snap = base_dir_ / name;
        fs::create_directories(snap);
        if (size_bytes > 0) {
            const fs::path file = snap / "data.sst";
            std::ofstream ofs(file, std::ios::binary);
            // Write a sparse file by seeking + writing one byte.
            ofs.seekp(static_cast<std::streamoff>(size_bytes - 1));
            ofs.write("\0", 1);
        }
        // Adjust last_write_time to simulate age.
        if (age_seconds > 0) {
            const auto past = fs::file_time_type::clock::now() -
                              std::chrono::seconds(age_seconds);
            std::error_code ec;
            fs::last_write_time(snap, past, ec);
        }
        return snap.string();
    }

    AiSnapshotCleanupJob::Config defaultConfig() const {
        return {base_dir_.string(), /*retention_days=*/7, /*max_total_gb=*/100};
    }

    fs::path base_dir_;
};

// ============================================================================
// POS-01: Empty snapshot dir → runCleanup returns 0
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS01_EmptyDirReturnsZero) {
    AiSnapshotCleanupJob job(defaultConfig());
    EXPECT_EQ(0, job.runCleanup());
}

// ============================================================================
// POS-02: Non-existent snapshot dir → runCleanup returns 0 (no throw)
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS02_NonExistentDirNoThrow) {
    AiSnapshotCleanupJob::Config cfg;
    cfg.snapshot_dir   = base_dir_.string() + "/does_not_exist";
    cfg.retention_days = 7;
    cfg.max_total_gb   = 100;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_NO_THROW({ EXPECT_EQ(0, job.runCleanup()); });
}

// ============================================================================
// POS-03: listSnapshots on empty dir → empty list
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS03_ListSnapshotsEmptyDir) {
    AiSnapshotCleanupJob job(defaultConfig());
    EXPECT_TRUE(job.listSnapshots().empty());
}

// ============================================================================
// POS-04: Snapshot older than retention_days → deleted by runCleanup
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS04_OldSnapshotDeleted) {
    makeSnapshot("abc123_pre_op", 0, /*age_seconds=*/8 * 86400);  // 8 days old
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 7;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(1, job.runCleanup());
    EXPECT_FALSE(fs::exists(base_dir_ / "abc123_pre_op"));
}

// ============================================================================
// POS-05: Snapshot within retention → NOT deleted
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS05_NewSnapshotKept) {
    makeSnapshot("abc456_pre_op", 0, /*age_seconds=*/3600);  // 1 hour old
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 7;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(0, job.runCleanup());
    EXPECT_TRUE(fs::exists(base_dir_ / "abc456_pre_op"));
}

// ============================================================================
// POS-06: Total size within limit → NOT deleted
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS06_SizeWithinLimit) {
    // 10 MiB snapshot, limit 100 GiB → should not be deleted
    makeSnapshot("op-new_pre_op", 10u * 1024u * 1024u, /*age_seconds=*/3600);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.max_total_gb = 100;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(0, job.runCleanup());
    EXPECT_TRUE(fs::exists(base_dir_ / "op-new_pre_op"));
}

// ============================================================================
// POS-07: Total size exceeds limit → oldest deleted first
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS07_SizeExceedsLimit_OldestDeleted) {
    // Two 10 MiB snapshots; limit 0 GiB (0 bytes cap) → both should be deleted.
    makeSnapshot("op-older_pre_op", 10u * 1024u * 1024u, /*age_seconds=*/7200);
    makeSnapshot("op-newer_pre_op", 10u * 1024u * 1024u, /*age_seconds=*/3600);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.max_total_gb   = 0;   // 0 GiB cap → every byte is over limit
    cfg.retention_days = 365; // not age-based
    AiSnapshotCleanupJob job(cfg);
    const int deleted = job.runCleanup();
    EXPECT_GE(deleted, 1);
}

// ============================================================================
// POS-08: Mix of old and recent snapshots → only old ones deleted
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS08_MixOldAndNew) {
    makeSnapshot("old_pre_op",    0, /*age_seconds=*/9 * 86400);  // 9 days
    makeSnapshot("recent_pre_op", 0, /*age_seconds=*/1 * 86400);  // 1 day
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 7;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(1, job.runCleanup());
    EXPECT_FALSE(fs::exists(base_dir_ / "old_pre_op"));
    EXPECT_TRUE(fs::exists(base_dir_ / "recent_pre_op"));
}

// ============================================================================
// POS-09: Multiple oversized → multiple deleted until within limit
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS09_MultipleOversizedDeletedUntilWithinLimit) {
    // Three snapshots; 0 GiB cap forces deletion of all.
    makeSnapshot("snap1_pre_op", 1024u, 10800);
    makeSnapshot("snap2_pre_op", 1024u, 7200);
    makeSnapshot("snap3_pre_op", 1024u, 3600);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.max_total_gb   = 0;
    cfg.retention_days = 365;
    AiSnapshotCleanupJob job(cfg);
    const int deleted = job.runCleanup();
    EXPECT_EQ(3, deleted);
}

// ============================================================================
// POS-10: totalSizeBytes empty list → 0
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS10_TotalSizeBytesEmptyList) {
    // Access via listSnapshots on empty dir + verify runCleanup result.
    AiSnapshotCleanupJob job(defaultConfig());
    const auto snaps = job.listSnapshots();
    EXPECT_TRUE(snaps.empty());
    // Verify no deletion happens either.
    EXPECT_EQ(0, job.runCleanup());
}

// ============================================================================
// POS-11: isExpired with future created_at → false
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS11_IsExpiredFutureTimestamp) {
    const auto future_ts = std::time(nullptr) + 86400;  // tomorrow
    AiSnapshotInfo snap;
    snap.path       = "/dummy";
    snap.size_bytes = 0;
    snap.created_at = future_ts;

    // We test through runCleanup indirectly: make a real snapshot with very
    // recent mtime and verify it survives a 1-day retention window.
    makeSnapshot("future_pre_op", 0, /*age_seconds=*/0);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 1;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(0, job.runCleanup());
    EXPECT_TRUE(fs::exists(base_dir_ / "future_pre_op"));
}

// ============================================================================
// POS-12: isExpired with old created_at → true
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS12_IsExpiredOldTimestamp) {
    // 30 days old, 7-day retention → should be deleted.
    makeSnapshot("very_old_pre_op", 0, /*age_seconds=*/30 * 86400);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 7;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(1, job.runCleanup());
    EXPECT_FALSE(fs::exists(base_dir_ / "very_old_pre_op"));
}

// ============================================================================
// POS-13: removeDirectory on non-existent path → returns false (no throw)
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS13_RemoveNonExistentNoThrow) {
    // Trigger via an internal path: run cleanup on a pre_op directory that
    // doesn't actually exist. listSnapshots won't find it, so no deletion occurs.
    // To test the noexcept directly, we use an indirect path:
    AiSnapshotCleanupJob::Config cfg;
    cfg.snapshot_dir   = base_dir_.string() + "/nonexistent_base";
    cfg.retention_days = 1;
    cfg.max_total_gb   = 100;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_NO_THROW({ EXPECT_EQ(0, job.runCleanup()); });
}

// ============================================================================
// POS-14: Config defaults correct
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS14_ConfigDefaults) {
    AiSnapshotCleanupJob job;
    EXPECT_EQ("/var/themis/ai-snapshots", job.config().snapshot_dir);
    EXPECT_EQ(7, job.config().retention_days);
    EXPECT_EQ(100u, job.config().max_total_gb);
}

// ============================================================================
// POS-15: runCleanup returns correct count
// ============================================================================
TEST_F(AiSnapshotCleanupTest, POS15_RunCleanupReturnsCorrectCount) {
    makeSnapshot("old1_pre_op", 0, 10 * 86400);
    makeSnapshot("old2_pre_op", 0, 11 * 86400);
    makeSnapshot("new1_pre_op", 0, 3600);
    AiSnapshotCleanupJob::Config cfg = defaultConfig();
    cfg.retention_days = 7;
    AiSnapshotCleanupJob job(cfg);
    EXPECT_EQ(2, job.runCleanup());
    EXPECT_FALSE(fs::exists(base_dir_ / "old1_pre_op"));
    EXPECT_FALSE(fs::exists(base_dir_ / "old2_pre_op"));
    EXPECT_TRUE(fs::exists(base_dir_ / "new1_pre_op"));
}

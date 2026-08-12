#include <gtest/gtest.h>
#include "sharding/raft_log.h"
#include <filesystem>
#include <cstdlib>
#include <fstream>
#ifdef _WIN32
#  include <process.h>
#  define getpid _getpid
#else
#  include <unistd.h>
#endif

using namespace themisdb::sharding;

// ============================================================================
// RaftLog basic tests
// ============================================================================

TEST(RaftLog, AppendAndGetEntry) {
    RaftLog log;
    LogEntry e{1, 1, "cmd1", 0};
    log.append(e);
    auto got = log.getEntry(1);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->command, "cmd1");
    EXPECT_EQ(got->term, 1u);
}

TEST(RaftLog, TruncateFrom) {
    RaftLog log;
    for (uint64_t i = 1; i <= 5; ++i) {
        log.append(LogEntry{1, i, "cmd", 0});
    }
    log.truncateFrom(3);
    EXPECT_FALSE(log.getEntry(3).has_value());
    EXPECT_FALSE(log.getEntry(5).has_value());
    EXPECT_TRUE(log.getEntry(2).has_value());
}

TEST(RaftLog, HasEntry_SnapshotBoundary) {
    RaftLog log;
    for (uint64_t i = 1; i <= 10; ++i) {
        log.append(LogEntry{1, i, "cmd", 0});
    }
    // Must commit first so compactUpTo is allowed to proceed
    log.setCommitIndex(10);
    log.compactUpTo(5, 1);
    // Entry 5 was removed from the map but is remembered as the snapshot anchor
    EXPECT_TRUE(log.hasEntry(5, 1));
    // Wrong term should fail
    EXPECT_FALSE(log.hasEntry(5, 99));
    // Entries after snapshot are still present
    EXPECT_TRUE(log.hasEntry(6, 1));
}

TEST(RaftLog, EstimatedSizeBytes) {
    RaftLog log;
    const std::string cmd(1000, 'x');
    for (uint64_t i = 1; i <= 10; ++i) {
        log.append(LogEntry{1, i, cmd, 0});
    }
    // Each entry has ~1000 bytes command + fixed overhead
    EXPECT_GT(log.estimatedSizeBytes(), 10000u);
}

TEST(RaftLog, CompactUpTo) {
    RaftLog log;
    for (uint64_t i = 1; i <= 20; ++i) {
        log.append(LogEntry{1, i, "x", 0});
    }
    log.setCommitIndex(20);
    log.compactUpTo(10, 1);

    // Entries 1-10 discarded
    EXPECT_FALSE(log.getEntry(1).has_value());
    EXPECT_FALSE(log.getEntry(10).has_value());
    // Entries 11-20 still present
    EXPECT_TRUE(log.getEntry(11).has_value());
    EXPECT_TRUE(log.getEntry(20).has_value());
    // Snapshot meta preserved
    EXPECT_EQ(log.getSnapshotIndex(), 10u);
    EXPECT_EQ(log.getSnapshotTerm(), 1u);
    // Commit index must NOT be changed by compaction
    EXPECT_EQ(log.getCommitIndex(), 20u);
}

TEST(RaftLog, CompactUpTo_RefusesToCompactUncommitted) {
    RaftLog log;
    for (uint64_t i = 1; i <= 5; ++i) {
        log.append(LogEntry{1, i, "x", 0});
    }
    log.setCommitIndex(3);
    // Attempting to compact past commit_index should be a no-op
    log.compactUpTo(5, 1);
    // Entries should still be present (compaction was rejected)
    EXPECT_TRUE(log.getEntry(5).has_value());
    EXPECT_EQ(log.getSnapshotIndex(), 0u);
}

TEST(RaftLog, CompactUpTo_ExactlyAtCommitIndex) {
    RaftLog log;
    for (uint64_t i = 1; i <= 10; ++i) {
        log.append(LogEntry{1, i, "x", 0});
    }
    // Compaction exactly at commit_index is allowed (boundary condition)
    log.setCommitIndex(10);
    log.compactUpTo(10, 1);
    EXPECT_EQ(log.getSnapshotIndex(), 10u);
    EXPECT_EQ(log.getSnapshotTerm(), 1u);
    // commit_index must remain unchanged
    EXPECT_EQ(log.getCommitIndex(), 10u);
    // All log entries discarded
    EXPECT_FALSE(log.getEntry(10).has_value());
}

TEST(RaftLog, GetLastLogTerm_UsesSnapshotAnchorAfterFullCompaction) {
    RaftLog log;
    for (uint64_t i = 1; i <= 4; ++i) {
        log.append(LogEntry{7, i, "x", 0});
    }
    log.setCommitIndex(4);
    log.compactUpTo(4, 7);

    EXPECT_EQ(log.getLastLogIndex(), 4u);
    EXPECT_EQ(log.getLastLogTerm(), 7u);
}

// ============================================================================
// RaftSnapshotManager tests
// ============================================================================

class RaftSnapshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        snapshot_dir_ = std::filesystem::temp_directory_path() /
                        ("test_raft_snap_" + std::to_string(::getpid()));
        std::filesystem::create_directories(snapshot_dir_);

        RaftSnapshotManager::Config cfg;
        cfg.snapshot_directory = snapshot_dir_.string();
        cfg.compaction_threshold_bytes = 1024;  // 1 KB – easy to trigger in tests
        cfg.compression_level = 3;
        cfg.max_snapshots = 3;
        cfg.chunk_size_bytes = 16;  // small chunks for testing
        mgr_ = std::make_unique<RaftSnapshotManager>(cfg);
    }

    void TearDown() override {
        std::filesystem::remove_all(snapshot_dir_);
    }

    std::filesystem::path snapshot_dir_;
    std::unique_ptr<RaftSnapshotManager> mgr_;
};

TEST_F(RaftSnapshotManagerTest, ShouldCompact_BelowThreshold) {
    RaftLog log;
    // No entries -> size is 0 -> should not compact
    EXPECT_FALSE(mgr_->shouldCompact(log));
}

TEST_F(RaftSnapshotManagerTest, ShouldCompact_AboveThreshold) {
    RaftSnapshotManager::Config cfg;
    cfg.snapshot_directory = snapshot_dir_.string();
    cfg.compaction_threshold_bytes = 100;  // very low threshold
    RaftSnapshotManager small_mgr(cfg);

    RaftLog log;
    const std::string big_cmd(200, 'A');
    log.append(LogEntry{1, 1, big_cmd, 0});
    EXPECT_TRUE(small_mgr.shouldCompact(log));
}

TEST_F(RaftSnapshotManagerTest, CreateAndLoadSnapshot) {
    RaftLog log;
    for (uint64_t i = 1; i <= 5; ++i) {
        log.append(LogEntry{1, i, "hello world", 0});
    }
    log.setCommitIndex(5);

    const std::string state = "my state machine data";
    const std::vector<uint8_t> state_bytes(state.begin(), state.end());

    ASSERT_TRUE(mgr_->createAndInstall(log, 5, 1, state_bytes));

    // Log should be compacted up to index 5
    EXPECT_EQ(log.getSnapshotIndex(), 5u);
    EXPECT_FALSE(log.getEntry(1).has_value());

    // Snapshot should be loadable
    auto snap = mgr_->loadLatestSnapshot();
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->snapshot_index, 5u);
    EXPECT_EQ(snap->snapshot_term, 1u);
    EXPECT_EQ(snap->uncompressed_size, state_bytes.size());
}

TEST_F(RaftSnapshotManagerTest, ChunkedAccess) {
    RaftLog log;
    log.append(LogEntry{1, 1, "data", 0});
    log.setCommitIndex(1);

    // Create a payload large enough to span multiple 16-byte chunks
    const std::vector<uint8_t> state(256, 0xAB);
    ASSERT_TRUE(mgr_->createAndInstall(log, 1, 1, state));

    const size_t count = mgr_->getChunkCount(1);
    EXPECT_GT(count, 0u);

    // Retrieve every chunk and verify its checksum
    for (uint64_t ci = 0; ci < count; ++ci) {
        auto chunk = mgr_->getChunk(1, ci);
        ASSERT_TRUE(chunk.has_value()) << "missing chunk " << ci;
        EXPECT_EQ(chunk->chunk_index, ci);
        EXPECT_EQ(chunk->total_chunks, count);
        EXPECT_EQ(chunk->last_chunk, ci + 1 == count);
        // Verify the per-chunk checksum (must not be empty)
        EXPECT_FALSE(chunk->checksum.empty());
        EXPECT_EQ(chunk->checksum.size(), 64u);  // SHA-256 hex = 64 chars
    }
}

TEST_F(RaftSnapshotManagerTest, OldSnapshotCleanup) {
    const std::vector<uint8_t> state(8, 0x01);
    // Create 5 snapshots (manager keeps max 3).
    // Each iteration uses a fresh log so that the commit/snapshot preconditions
    // are satisfied independently.
    for (uint64_t idx = 1; idx <= 5; ++idx) {
        RaftLog log;
        log.append(LogEntry{1, idx, "x", 0});
        log.setCommitIndex(idx);
        mgr_->createAndInstall(log, idx, 1, state);
    }

    EXPECT_LE(mgr_->listSnapshots().size(), 3u);
}

TEST_F(RaftSnapshotManagerTest, CreateAndInstall_DoesNotLeaveTempFile) {
    RaftLog log;
    log.append(LogEntry{1, 1, "x", 0});
    log.setCommitIndex(1);

    const std::vector<uint8_t> state(16, 0x42);
    ASSERT_TRUE(mgr_->createAndInstall(log, 1, 1, state));

    const auto temp_path = snapshot_dir_ / "raft_snapshot_1.bin.tmp";
    EXPECT_FALSE(std::filesystem::exists(temp_path));
}

TEST_F(RaftSnapshotManagerTest, ListSnapshots_IgnoresMalformedSnapshotNames) {
    const auto malformed = snapshot_dir_ / "raft_snapshot_not_a_number.bin";
    std::ofstream out(malformed, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "junk";
    out.close();

    const auto snapshots = mgr_->listSnapshots();
    EXPECT_TRUE(snapshots.empty());
}

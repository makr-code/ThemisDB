#include <gtest/gtest.h>
#include "sharding/raft_log.h"
#include "sharding/raft_state.h"
#include "sharding/wal_shipper.h"
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

// The sharding code is split across two top-level namespaces in this repo:
// - themisdb::sharding  → RaftLog, RaftState, LogEntry (include/sharding/raft_*.h)
// - themis::sharding    → WALShipper, SnapshotChunk    (include/sharding/wal_shipper.h)
using namespace themisdb::sharding;
using namespace themis::sharding;

// ============================================================================
// WALShipper snapshot transfer tests
// ============================================================================

TEST(WALShipperSnapshot, VerifyChunkChecksum_Valid) {
    SnapshotChunk chunk;
    chunk.snapshot_index = 42;
    chunk.snapshot_term  = 3;
    chunk.chunk_index    = 0;
    chunk.total_chunks   = 1;
    chunk.last_chunk     = true;
    chunk.data           = {0x01, 0x02, 0x03, 0x04};

    // Compute a valid checksum
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(chunk.data.data(), chunk.data.size(), hash);
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    chunk.checksum = oss.str();

    EXPECT_TRUE(WALShipper::verifyChunkChecksum(chunk));
}

TEST(WALShipperSnapshot, VerifyChunkChecksum_Invalid) {
    SnapshotChunk chunk;
    chunk.data     = {0xDE, 0xAD, 0xBE, 0xEF};
    chunk.checksum = std::string(64, '0');  // all-zeros – deliberately wrong

    EXPECT_FALSE(WALShipper::verifyChunkChecksum(chunk));
}

// ============================================================================
// RaftState snapshot meta tests
// ============================================================================

TEST(RaftStateSnapshot, SetAndGetSnapshotMeta) {
    RaftConfig cfg;
    cfg.node_id = "node1";
    cfg.cluster_members = {"node1", "node2", "node3"};

    RaftState state(cfg);
    EXPECT_EQ(state.getSnapshotIndex(), 0u);
    EXPECT_EQ(state.getSnapshotTerm(), 0u);

    state.setSnapshotMeta(100, 5);
    EXPECT_EQ(state.getSnapshotIndex(), 100u);
    EXPECT_EQ(state.getSnapshotTerm(), 5u);

    // The internal log's snapshot meta must also be updated
    EXPECT_EQ(state.getLog().getSnapshotIndex(), 100u);
    EXPECT_EQ(state.getLog().getSnapshotTerm(), 5u);
}

TEST(RaftStateSnapshot, HasEntryAfterSnapshotMeta) {
    RaftConfig cfg;
    cfg.node_id = "node1";
    cfg.cluster_members = {"node1", "node2", "node3"};

    RaftState state(cfg);
    RaftLog& log = state.getLog();

    for (uint64_t i = 1; i <= 5; ++i) {
        log.append(LogEntry{2, i, "cmd", 0});
    }
    // Commit all entries before compacting
    log.setCommitIndex(5);
    log.compactUpTo(3, 2);
    state.setSnapshotMeta(3, 2);

    // Snapshot boundary must be reachable
    EXPECT_TRUE(log.hasEntry(3, 2));
    EXPECT_FALSE(log.hasEntry(3, 99));  // wrong term
    // Entries after compaction still accessible
    EXPECT_TRUE(log.hasEntry(4, 2));
}

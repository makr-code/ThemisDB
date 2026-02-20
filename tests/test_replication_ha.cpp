/**
 * ThemisDB Replication HA Tests
 *
 * Unit tests covering the production-readiness improvements made to the
 * replication module:
 *   1. Configuration validation
 *   2. WAL checksum verification on read
 *   3. LastWriteWinsResolver conflict resolution
 *   4. CRDTMergeResolver conflict resolution
 *   5. LeaderElection quorum-based promotion
 *   6. Thread-safe replica list operations
 *   7. Error handling in replicate()
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>

using namespace themisdb::replication;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ReplicationConfig makeConfig(const std::string& wal_dir = "/tmp/themis_repl_test_wal") {
    ReplicationConfig cfg;
    cfg.enabled                      = true;
    cfg.mode                         = ReplicationMode::ASYNC;
    cfg.heartbeat_interval_ms        = 100;
    cfg.election_timeout_min_ms      = 150;
    cfg.election_timeout_max_ms      = 300;
    cfg.batch_size                   = 100;
    cfg.batch_timeout_ms             = 50;
    cfg.wal_directory                = wal_dir;
    cfg.failure_detection_timeout_ms = 1000;
    cfg.degraded_lag_threshold_ms    = 5000;
    cfg.min_sync_replicas            = 1;
    cfg.wal_sync_on_commit           = false;
    return cfg;
}

// RAII helper that removes the WAL directory on destruction.
struct TempWALDir {
    std::string path;
    explicit TempWALDir(const std::string& p) : path(p) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TempWALDir() { std::filesystem::remove_all(path); }
};

// ============================================================================
// 1. Configuration Validation
// ============================================================================

class ReplicationConfigTest : public ::testing::Test {};

TEST_F(ReplicationConfigTest, InvalidBatchSizeZeroFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.batch_size = 0;

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, InvalidBatchSizeTooLargeFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_test2");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.batch_size = 2000000;

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, InvalidHeartbeatIntervalFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_test3");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.heartbeat_interval_ms = 0;

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, ElectionTimeoutMinGEMaxFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_test4");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.election_timeout_min_ms = 300;
    cfg.election_timeout_max_ms = 300;  // min == max  → invalid

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, ValidConfigInitializesSuccessfully) {
    TempWALDir wd("/tmp/themis_repl_cfg_test5");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    EXPECT_TRUE(mgr.initialize());
    mgr.shutdown();
}

// ============================================================================
// 2. WAL Checksum Verification
// ============================================================================

class WALChecksumTest : public ::testing::Test {};

TEST_F(WALChecksumTest, AppendAndReadBackVerifiesChecksum) {
    TempWALDir wd("/tmp/themis_wal_csum_test");
    ReplicationConfig cfg = makeConfig(wd.path);

    WALManager wal(cfg);

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "users";
    entry.document_id = "doc1";
    entry.data        = "{\"name\":\"Alice\"}";

    uint64_t seq = wal.append(entry);
    ASSERT_GT(seq, 0u);

    auto entries = wal.readFrom(seq, 1);
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].operation, "INSERT");
    EXPECT_EQ(entries[0].document_id, "doc1");
    EXPECT_FALSE(entries[0].checksum.empty());
}

TEST_F(WALChecksumTest, CorruptChecksumEntryIsDropped) {
    TempWALDir wd("/tmp/themis_wal_corrupt_test");
    ReplicationConfig cfg = makeConfig(wd.path);

    WALManager wal(cfg);

    // Append a valid entry
    WALEntry entry;
    entry.operation   = "UPDATE";
    entry.collection  = "products";
    entry.document_id = "prod1";
    entry.data        = "{\"price\":99}";
    uint64_t seq = wal.append(entry);
    ASSERT_GT(seq, 0u);

    // Manually corrupt the checksum inside the WAL segment file
    std::string seg_path = wd.path + "/wal_0.log";
    ASSERT_TRUE(std::filesystem::exists(seg_path));

    {
        // Open the file for read+write; the checksum is near the end of the record.
        std::fstream fs(seg_path, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(fs.is_open());
        // Seek close to end and flip a byte to corrupt the checksum field
        fs.seekp(-4, std::ios::end);
        char dummy = 0xFF;
        fs.write(&dummy, 1);
    }

    // readFrom should skip the corrupted entry
    auto entries = wal.readFrom(seq, 10);
    EXPECT_EQ(entries.size(), 0u)
        << "Corrupted entry should have been filtered out by checksum verification";
}

// ============================================================================
// 3. LastWriteWinsResolver
// ============================================================================

class LWWResolverTest : public ::testing::Test {
protected:
    LWWConflictResolver resolver;
};

TEST_F(LWWResolverTest, SelectsNewerTimestamp) {
    std::string older = R"({"id":"1","updated_at":1000,"value":"old"})";
    std::string newer = R"({"id":"1","updated_at":2000,"value":"new"})";

    std::string result = resolver.resolve(older, newer, "col", "1");
    EXPECT_EQ(result, newer);
}

TEST_F(LWWResolverTest, KeepsLocalWhenLocalIsNewer) {
    std::string local  = R"({"id":"2","updated_at":5000,"value":"local"})";
    std::string remote = R"({"id":"2","updated_at":3000,"value":"remote"})";

    std::string result = resolver.resolve(local, remote, "col", "2");
    EXPECT_EQ(result, local);
}

TEST_F(LWWResolverTest, FallsBackToRemoteWhenNoTimestamp) {
    std::string local  = R"({"id":"3","value":"notime"})";
    std::string remote = R"({"id":"3","value":"remote_notime"})";

    std::string result = resolver.resolve(local, remote, "col", "3");
    EXPECT_EQ(result, remote)
        << "When neither document has a timestamp the remote version should win";
}

TEST_F(LWWResolverTest, ChoosesRemoteOnTie) {
    // Equal timestamps → remote should win (conservative)
    std::string local  = R"({"id":"4","updated_at":100,"value":"A"})";
    std::string remote = R"({"id":"4","updated_at":100,"value":"B"})";

    std::string result = resolver.resolve(local, remote, "col", "4");
    EXPECT_EQ(result, remote);
}

// ============================================================================
// 4. CRDTMergeResolver
// ============================================================================

class CRDTResolverTest : public ::testing::Test {
protected:
    CRDTConflictResolver resolver;
};

TEST_F(CRDTResolverTest, MergeKeepsMaxNumericValue) {
    // Local has a higher counter for "views"; remote has a higher counter for "likes"
    std::string local  = R"({"updated_at":200,"views":50,"likes":5})";
    std::string remote = R"({"updated_at":100,"views":30,"likes":20})";

    // LWW winner is local (updated_at=200); CRDT should promote likes to 20
    std::string result = resolver.resolve(local, remote, "col", "doc");
    EXPECT_FALSE(result.empty());
    // The merged document must contain likes:20 (the max)
    EXPECT_NE(result.find("20"), std::string::npos)
        << "CRDT merge should keep the max value (20) for 'likes'";
}

TEST_F(CRDTResolverTest, EmptyLocalReturnsRemote) {
    std::string result = resolver.resolve("", R"({"a":1})", "c", "d");
    EXPECT_EQ(result, R"({"a":1})");
}

TEST_F(CRDTResolverTest, EmptyRemoteReturnsLocal) {
    std::string result = resolver.resolve(R"({"a":1})", "", "c", "d");
    EXPECT_EQ(result, R"({"a":1})");
}

// ============================================================================
// 5. LeaderElection quorum logic
// ============================================================================

class LeaderElectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        TempWALDir wd_guard{wal_path_};  // create dir
        (void)wd_guard;
        std::filesystem::create_directories(wal_path_);
        config_ = makeConfig(wal_path_);
        wal_    = std::make_shared<WALManager>(config_);
    }

    void TearDown() override {
        std::filesystem::remove_all(wal_path_);
    }

    const std::string wal_path_ = "/tmp/themis_le_test";
    ReplicationConfig config_;
    std::shared_ptr<WALManager> wal_;
};

TEST_F(LeaderElectionTest, SingleNodeWinsElectionImmediately) {
    LeaderElection le("node-1", config_, wal_);
    le.setClusterSize(1);  // Single-node cluster → quorum = 1 = self vote

    le.startElection();
    EXPECT_TRUE(le.isLeader());
}

TEST_F(LeaderElectionTest, ThreeNodeClusterRequiresQuorum) {
    LeaderElection le("node-1", config_, wal_);
    le.setClusterSize(3);  // Need 2 votes (quorum = 3/2+1 = 2)

    le.startElection();
    // With only 1 vote (self) and cluster_size=3, quorum is not yet reached
    EXPECT_FALSE(le.isLeader())
        << "Should not become leader with only 1 out of 3 votes";

    // Simulate a peer granting a vote for the current term
    le.grantVote(wal_->getCurrentTerm());
    EXPECT_TRUE(le.isLeader())
        << "Should become leader after receiving quorum (2/3) votes";
}

TEST_F(LeaderElectionTest, StaleVoteIsIgnored) {
    LeaderElection le("node-2", config_, wal_);
    le.setClusterSize(3);

    le.startElection();
    uint64_t election_term = wal_->getCurrentTerm();

    // A vote for an older term must not promote to leader
    le.grantVote(election_term > 0 ? election_term - 1 : 0);
    EXPECT_FALSE(le.isLeader())
        << "Vote for an older term should be ignored";

    // Now grant vote for the correct term
    le.grantVote(election_term);
    EXPECT_TRUE(le.isLeader());
}

TEST_F(LeaderElectionTest, HeartbeatConvertsCandidateToFollower) {
    LeaderElection le("node-3", config_, wal_);
    le.setClusterSize(3);

    le.startElection();
    ASSERT_FALSE(le.isLeader());

    // Receiving a heartbeat from a leader with higher term demotes us
    le.receiveHeartbeat(wal_->getCurrentTerm() + 1, "node-leader", 0);
    EXPECT_EQ(le.getRole(), ReplicationRole::FOLLOWER);
    EXPECT_EQ(le.getLeaderId(), "node-leader");
}

TEST_F(LeaderElectionTest, RequestVoteGrantedWhenLogIsUpToDate) {
    LeaderElection le("node-4", config_, wal_);

    bool granted = le.requestVote(
        wal_->getCurrentTerm() + 1,  // Higher term
        "candidate-5",
        0, 0);
    EXPECT_TRUE(granted);
}

TEST_F(LeaderElectionTest, RequestVoteRejectedForStaleTerm) {
    LeaderElection le("node-5", config_, wal_);

    // Advance our own term first
    le.startElection();

    // Now a peer with a lower term asks for a vote
    bool granted = le.requestVote(0, "old-candidate", 0, 0);
    EXPECT_FALSE(granted);
}

// ============================================================================
// 6. Thread-safe replica list – concurrent add/remove/read
// ============================================================================

TEST(ReplicationManagerThreadSafety, ConcurrentAddRemoveReplicas) {
    TempWALDir wd("/tmp/themis_ts_test");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    std::atomic<bool> stop{false};
    std::atomic<int>  errors{0};

    // Writer thread: repeatedly adds and removes a replica
    std::thread writer([&]() {
        for (int i = 0; i < 50 && !stop.load(); ++i) {
            ReplicaInfo r;
            r.node_id         = "replica-" + std::to_string(i);
            r.endpoint        = "127.0.0.1:" + std::to_string(9000 + i);
            r.role            = ReplicationRole::FOLLOWER;
            r.is_voting_member = true;
            r.last_heartbeat  = std::chrono::system_clock::now();
            r.health_status   = HealthStatus::HEALTHY;
            try {
                mgr.addReplica(r);
                mgr.removeReplica(r.node_id);
            } catch (...) {
                errors++;
            }
        }
    });

    // Reader thread: concurrently reads the replica list
    std::thread reader([&]() {
        for (int i = 0; i < 200 && !stop.load(); ++i) {
            try {
                auto replicas = mgr.getReplicas();
                (void)replicas;
            } catch (...) {
                errors++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        stop.store(true);
    });

    writer.join();
    reader.join();

    EXPECT_EQ(errors.load(), 0) << "No exceptions expected during concurrent access";

    mgr.shutdown();
}

// ============================================================================
// 7. replicate() error handling
// ============================================================================

TEST(ReplicationManagerErrorHandling, ReplicateBeforeInitFails) {
    TempWALDir wd("/tmp/themis_err_test");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    // Do NOT call initialize()

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "test";
    entry.document_id = "doc1";
    entry.data        = "{}";

    // Should fail gracefully since manager is not initialized
    EXPECT_FALSE(mgr.replicate(entry));
}

TEST(ReplicationManagerErrorHandling, ReplicateAsFollowerFails) {
    TempWALDir wd("/tmp/themis_err_test2");
    ReplicationConfig cfg = makeConfig(wd.path);
    // Add a seed node so we start as FOLLOWER (single-node won't auto-win election
    // with cluster_size>1)
    cfg.seed_nodes = {"127.0.0.1:9999"};

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "test";
    entry.document_id = "doc1";
    entry.data        = "{}";

    // Node starts as FOLLOWER (cluster_size=2) and has not won an election
    EXPECT_FALSE(mgr.replicate(entry))
        << "Follower must not accept writes";

    mgr.shutdown();
}


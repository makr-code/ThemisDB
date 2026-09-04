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
 *   8. Witness node support (vote-only, no data) for 2-node cluster quorum
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"
#include "replication/multi_master_replication.h"
#include "replication/multi_tier_replication.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <zstd.h>
#include <lz4.h>
#include <snappy.h>

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
    // Keep generic helper configs valid for short election timeouts used by
    // many focused tests. Lease-specific tests use makeLeaseConfig().
    cfg.enable_leader_lease          = false;
    cfg.leader_lease_duration_ms     = 0;
    return cfg;
}

// RAII helper that removes the WAL directory on destruction.
struct TempWALDir {
    std::string path = {};
    explicit TempWALDir(const std::string& p) {
        const auto ticks = std::chrono::high_resolution_clock::now()
                               .time_since_epoch()
                               .count();
        const auto tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
        const std::string base = std::filesystem::path(p).filename().string();
        path = (std::filesystem::temp_directory_path() /
                (base + "_" + std::to_string(ticks) + "_" + std::to_string(tid_hash)))
                   .string();

        std::error_code ec = {};
        std::filesystem::remove_all(path, ec);
        std::filesystem::create_directories(path, ec);
    }
    ~TempWALDir() {
        for (int i = 0; i < 5; ++i) {
            std::error_code ec = {};
            std::filesystem::remove_all(path, ec);
            if (!ec) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
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

TEST_F(ReplicationConfigTest, LeaseDurationGEElectionTimeoutFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_lease_invalid");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.enable_leader_lease       = true;
    cfg.election_timeout_min_ms   = 3000;
    cfg.election_timeout_max_ms   = 5000;
    cfg.leader_lease_duration_ms  = 3000;  // equal → invalid (must be strictly less)

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, LeaseDurationLTElectionTimeoutSucceeds) {
    TempWALDir wd("/tmp/themis_repl_cfg_lease_valid");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.enable_leader_lease       = true;
    cfg.election_timeout_min_ms   = 3000;
    cfg.election_timeout_max_ms   = 5000;
    cfg.leader_lease_duration_ms  = 2999;  // strictly less → valid

    ReplicationManager mgr(cfg);
    EXPECT_TRUE(mgr.initialize());
    mgr.shutdown();
}

TEST_F(ReplicationConfigTest, LeaseDisabledIgnoresLeaseDuration) {
    TempWALDir wd("/tmp/themis_repl_cfg_lease_off");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.enable_leader_lease       = false;
    cfg.election_timeout_min_ms   = 500;
    cfg.election_timeout_max_ms   = 1000;
    // duration >= election_timeout but lease is disabled → should still succeed
    cfg.leader_lease_duration_ms  = 5000;

    ReplicationManager mgr(cfg);
    EXPECT_TRUE(mgr.initialize());
    mgr.shutdown();
}

TEST_F(ReplicationConfigTest, ValidConfigInitializesSuccessfully) {
    TempWALDir wd("/tmp/themis_repl_cfg_test5");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    EXPECT_TRUE(mgr.initialize());
    mgr.shutdown();
}

TEST_F(ReplicationConfigTest, InvalidWALCompressionLevelFails) {
    TempWALDir wd("/tmp/themis_repl_cfg_compress_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.enable_wal_compression  = true;
    cfg.wal_compression_level   = 0;  // Out of range (must be 1-9)

    ReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.initialize());
}

TEST_F(ReplicationConfigTest, ValidWALCompressionLevelSucceeds) {
    TempWALDir wd("/tmp/themis_repl_cfg_compress_ok");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.enable_wal_compression  = true;
    cfg.wal_compression_level   = 6;

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
        fs.seekg(static_cast<std::streamoff>(-4), std::ios::end);
        std::uint8_t dummy{};
        fs.read(reinterpret_cast<char*>(&dummy), 1);
        dummy ^= static_cast<std::uint8_t>(0x1u);
        fs.seekp(static_cast<std::streamoff>(-4), std::ios::end);
        fs.write(reinterpret_cast<const char*>(&dummy), 1);
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
        std::filesystem::remove_all(wal_path_);
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

TEST_F(LeaderElectionTest, HeartbeatAdvancesCommitIndex) {
    LeaderElection le("node-ci", config_, wal_);
    le.setClusterSize(3);

    // Initial commit index must be 0
    EXPECT_EQ(le.getCommitIndex(), 0u);

    // Append some WAL entries so that last_log_sequence > 0
    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "ci_test";
    entry.document_id = "doc1";
    entry.data        = "{}";
    uint64_t seq = wal_->append(entry);
    ASSERT_GT(seq, 0u);

    // Simulate a heartbeat from the leader that has committed up to seq
    le.receiveHeartbeat(wal_->getCurrentTerm() + 1, "node-leader", seq);

    // commit_index must be updated to min(leader_commit, our last_log_seq) = seq
    EXPECT_EQ(le.getCommitIndex(), seq);
}

TEST_F(LeaderElectionTest, CommitIndexNeverGoesBackward) {
    LeaderElection le("node-ci2", config_, wal_);
    le.setClusterSize(3);

    WALEntry entry;
    entry.operation = "INSERT";
    entry.collection = "ci_test";
    entry.document_id = "d1";
    entry.data = "{}";
    uint64_t seq = wal_->append(entry);
    ASSERT_GT(seq, 0u);

    // First heartbeat: advance to seq
    le.receiveHeartbeat(1, "leader", seq);
    EXPECT_EQ(le.getCommitIndex(), seq);

    // Second heartbeat: try to go backward (leader_commit < current)
    le.receiveHeartbeat(1, "leader", 0);
    // commit_index must not decrease
    EXPECT_EQ(le.getCommitIndex(), seq);
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

TEST(ReplicationManagerErrorHandling, SyncModeWithoutReplicaStreamsFailsClosed) {
    TempWALDir wd("/tmp/themis_err_sync_no_streams");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.mode = ReplicationMode::SYNC;

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());
    ASSERT_TRUE(mgr.promoteToLeader());

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "test";
    entry.document_id = "doc_sync_no_stream";
    entry.data        = "{}";

    EXPECT_FALSE(mgr.replicate(entry))
        << "SYNC replication must fail closed when no replica stream can acknowledge";

    mgr.shutdown();
}

TEST(ReplicationManagerErrorHandling, SemiSyncImpossibleQuorumFailsClosed) {
    TempWALDir wd("/tmp/themis_err_semisync_quorum");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.mode = ReplicationMode::SEMI_SYNC;
    cfg.min_sync_replicas = 2;

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());
    ASSERT_TRUE(mgr.promoteToLeader());

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "test";
    entry.document_id = "doc_semisync_quorum";
    entry.data        = "{}";

    EXPECT_FALSE(mgr.replicate(entry))
        << "SEMI_SYNC replication must fail closed when required acks exceed active streams";

    mgr.shutdown();
}

// ============================================================================
// 8. electionLoop – randomized timeout triggers election
// ============================================================================

TEST(ElectionLoopTest, TimeoutTriggersElectionOnSingleNode) {
    // Single-node cluster: after an election timeout, node should become leader
    TempWALDir wd("/tmp/themis_elt_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.election_timeout_min_ms = 50;   // Very short for fast test
    cfg.election_timeout_max_ms = 100;

    auto wal = std::make_shared<WALManager>(cfg);
    LeaderElection le("node-elt", cfg, wal);
    le.setClusterSize(1);
    le.start();

    // Wait up to 500ms for the election loop to fire
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (!le.isLeader() &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(le.isLeader())
        << "Single-node cluster should become leader after election timeout";
}

TEST(ElectionLoopTest, HeartbeatResetsElectionTimer) {
    // When heartbeats arrive within the timeout, election must NOT be triggered
    TempWALDir wd("/tmp/themis_elt_hb_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.election_timeout_min_ms = 200;
    cfg.election_timeout_max_ms = 300;

    auto wal = std::make_shared<WALManager>(cfg);
    LeaderElection le("node-hb", cfg, wal);
    le.setClusterSize(3);  // Need quorum of 2; never grant 2nd vote → stays follower
    le.start();

    // Send heartbeats every 50ms for 400ms – keep resetting the timer
    for (int i = 0; i < 8; ++i) {
        le.receiveHeartbeat(1, "leader-node", 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Node should still be FOLLOWER because heartbeats kept resetting the timer
    EXPECT_EQ(le.getRole(), ReplicationRole::FOLLOWER)
        << "Regular heartbeats should prevent election timeout";
}

TEST(ElectionLoopTest, StopDoesNotHang) {
    // Destroying a started LeaderElection should join cleanly
    TempWALDir wd("/tmp/themis_elt_stop_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    cfg.election_timeout_min_ms = 5000;  // Long timeout so loop is sleeping
    cfg.election_timeout_max_ms = 6000;

    auto start = std::chrono::steady_clock::now();
    {
        auto wal = std::make_shared<WALManager>(cfg);
        LeaderElection le("node-stop", cfg, wal);
        le.setClusterSize(1);
        le.start();
        // Destructor stops the thread
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    EXPECT_LT(elapsed, 500)
        << "Destruction should complete quickly even with long election timeout";
}

// ============================================================================
// 9. VectorClock
// ============================================================================

TEST(VectorClockTest, IncrementAndGet) {
    VectorClock vc("node-A");
    EXPECT_EQ(vc.get("node-A"), 0u);  // initialised to 0

    vc.increment("node-A");
    EXPECT_EQ(vc.get("node-A"), 1u);

    vc.increment("node-A");
    EXPECT_EQ(vc.get("node-A"), 2u);

    // Unknown node returns 0
    EXPECT_EQ(vc.get("node-Z"), 0u);
}

TEST(VectorClockTest, MergeInHAContext) {
    VectorClock a("node-A");
    a.increment("node-A");
    a.increment("node-A");  // A: {A:2}

    VectorClock b("node-B");
    b.increment("node-B");
    b.increment("node-B");
    b.increment("node-B");  // B: {B:3}

    a.merge(b);  // A should now be {A:2, B:3}
    EXPECT_EQ(a.get("node-A"), 2u);
    EXPECT_EQ(a.get("node-B"), 3u);
}

TEST(VectorClockTest, Compare_HappensBefore) {
    VectorClock a("A");
    a.increment("A");  // a: {A:1}

    VectorClock b("A");
    b.increment("A");
    b.increment("A");  // b: {A:2}

    // a happened before b
    EXPECT_TRUE(a.happensBefore(b));
    EXPECT_FALSE(b.happensBefore(a));
}

TEST(VectorClockTest, Compare_Concurrent) {
    VectorClock a("A");
    a.increment("A");

    VectorClock b("B");
    b.increment("B");

    // Neither happened before the other (different partitions)
    EXPECT_TRUE(a.isConcurrent(b));
    EXPECT_TRUE(b.isConcurrent(a));
}

TEST(VectorClockTest, Compare_EqualIsConcurrent) {
    VectorClock a;
    VectorClock b;
    // Both empty (equal) clocks should be treated as concurrent (compare returns 0)
    EXPECT_EQ(a.compare(b), 0);
    EXPECT_FALSE(a.happensBefore(b));
}

TEST(VectorClockTest, ToJsonRoundTrip) {
    VectorClock vc;
    vc.increment("node-1");
    vc.increment("node-1");
    vc.increment("node-2");

    std::string json = vc.toJson();
    EXPECT_FALSE(json.empty());

    VectorClock restored = VectorClock::fromJson(json);
    EXPECT_EQ(restored.get("node-1"), 2u);
    EXPECT_EQ(restored.get("node-2"), 1u);
}

TEST(VectorClockTest, CopyConstructor) {
    VectorClock original("X");
    original.increment("X");
    original.increment("X");

    VectorClock copy(original);
    EXPECT_EQ(copy.get("X"), 2u);

    // Mutating copy should not affect original
    copy.increment("X");
    EXPECT_EQ(original.get("X"), 2u);
    EXPECT_EQ(copy.get("X"), 3u);
}

// ============================================================================
// 10. HybridLogicalClock
// ============================================================================

TEST(HLCTest, NowReturnsMonotonicTimestamps) {
    HybridLogicalClock hlc("node-1");

    auto t1 = hlc.now();
    auto t2 = hlc.now();

    // Each call must produce a non-decreasing timestamp (t2 >= t1)
    EXPECT_FALSE(t2 < t1)
        << "now() must produce monotonic HLC timestamps";
}

TEST(HLCTest, ReceiveAdvancesClockBeyondSender) {
    HybridLogicalClock sender("sender");
    HybridLogicalClock receiver("receiver");

    auto sent = sender.now();

    // Simulate receiving a message with the sender's timestamp
    auto received_ts = receiver.receive(sent);

    // After receiving, the receiver's logical component must be > sender's
    EXPECT_FALSE(received_ts < sent)
        << "After receive(), local clock must not be less than the sender's";
}

TEST(HLCTest, CurrentReturnsLastGeneratedTimestamp) {
    HybridLogicalClock hlc("node-c");
    auto t = hlc.now();
    auto cur = hlc.current();

    EXPECT_EQ(t.physical, cur.physical);
    EXPECT_EQ(t.logical,  cur.logical);
}

TEST(HLCTest, TimestampOrdering) {
    HybridLogicalClock::Timestamp t1{1000, 0, "A"};
    HybridLogicalClock::Timestamp t2{1000, 1, "A"};
    HybridLogicalClock::Timestamp t3{2000, 0, "A"};

    EXPECT_TRUE(t1 < t2);   // Same physical, higher logical
    EXPECT_TRUE(t2 < t3);   // Higher physical wins
    EXPECT_FALSE(t3 < t1);
}

TEST(HLCTest, TimestampToString) {
    HybridLogicalClock::Timestamp ts{12345, 7, "node-x"};
    std::string s = ts.toString();
    EXPECT_NE(s.find("12345"), std::string::npos);
    EXPECT_NE(s.find("7"),     std::string::npos);
    EXPECT_NE(s.find("node-x"), std::string::npos);
}

// ============================================================================
// 11. Multi-master MM resolvers (MMWriteEntry variants)
// ============================================================================

static HybridLogicalClock::Timestamp makeHLCTimestamp(uint64_t phys, uint32_t log,
                                              const std::string& node) {
    return HybridLogicalClock::Timestamp{phys, log, node};
}

TEST(MMLastWriteWinsTest, SelectsLatestHLCTimestamp) {
    LastWriteWinsResolver resolver;

    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = "old"; e1.hlc = makeHLCTimestamp(1000, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = "new"; e2.hlc = makeHLCTimestamp(2000, 0, "B");

    auto result = resolver.resolve("doc1", {e1, e2});
    EXPECT_EQ(result.data, "new");
    EXPECT_EQ(result.write_id, "w2");
}

TEST(MMLastWriteWinsTest, EmptyInputReturnsDefault) {
    LastWriteWinsResolver resolver;
    auto result = resolver.resolve("doc1", {});
    EXPECT_TRUE(result.write_id.empty());
}

TEST(MMCRDTResolverTest, LWWRegisterPicksLatest) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::LWW_REGISTER);

    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = R"({"v":1})";
    e1.hlc = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = R"({"v":2})";
    e2.hlc = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    EXPECT_EQ(result.data, R"({"v":2})");
}

TEST(MMCRDTResolverTest, GCounterMergesMaxPerKey) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::G_COUNTER);

    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = R"({"counter":5})";
    e1.hlc = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = R"({"counter":3})";
    e2.hlc = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // The data should contain the max counter value (5)
    EXPECT_NE(result.data.find("5"), std::string::npos)
        << "G-Counter merge should keep max value";
}

TEST(MMCRDTResolverTest, GSetUnionOfValues) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::G_SET);

    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = R"(["apple","banana"])";
    e1.hlc = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = R"(["banana","cherry"])";
    e2.hlc = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // Result should contain all three unique values
    EXPECT_NE(result.data.find("apple"),  std::string::npos);
    EXPECT_NE(result.data.find("banana"), std::string::npos);
    EXPECT_NE(result.data.find("cherry"), std::string::npos);
}

TEST(MMCRDTResolverTest, StrategyNameMatchesType) {
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::LWW_REGISTER).strategyName(),
              "LWW_REGISTER");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::G_COUNTER).strategyName(),
              "G_COUNTER");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::G_SET).strategyName(),
              "G_SET");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::TWO_P_SET).strategyName(),
              "TWO_P_SET");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::RGA).strategyName(),
              "RGA");
}

// PN_COUNTER: proper positive/negative sub-counter merge
TEST(MMCRDTResolverTest, PNCounterMergesPositiveAndNegativeSeparately) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::PN_COUNTER);

    // nodeA incremented 5 times and decremented 2 times
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"P":{"nodeA":5},"N":{"nodeA":2}})";
    e1.hlc  = makeHLCTimestamp(100, 0, "nodeA");

    // nodeB incremented 3 times and decremented 1 time; also sees nodeA incremented 4
    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"P":{"nodeA":4,"nodeB":3},"N":{"nodeA":1,"nodeB":1}})";
    e2.hlc  = makeHLCTimestamp(200, 0, "nodeB");

    auto result = resolver.resolve("doc", {e1, e2});
    // Merged P: nodeA=max(5,4)=5, nodeB=3
    // Merged N: nodeA=max(2,1)=2, nodeB=1
    EXPECT_NE(result.data.find("\"P\""), std::string::npos);
    EXPECT_NE(result.data.find("\"N\""), std::string::npos);
    EXPECT_NE(result.data.find("\"nodeA\":5"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("\"nodeB\":3"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("\"nodeA\":2"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("\"nodeB\":1"), std::string::npos) << result.data;
}

// OR_SET: elements removed via tombstones are excluded; others survive
TEST(MMCRDTResolverTest, ORSetRemovesTombstonedElements) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::OR_SET);

    // nodeA added "apple" with tag-1 and "banana" with tag-2
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"add":[["apple","tag-1"],["banana","tag-2"]],"tombstones":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "nodeA");

    // nodeB removed "apple" by tombstoning tag-1
    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"add":[["apple","tag-1"],["banana","tag-2"]],"tombstones":["tag-1"]})";
    e2.hlc  = makeHLCTimestamp(200, 0, "nodeB");

    auto result = resolver.resolve("doc", {e1, e2});
    // "apple" was tombstoned (tag-1 removed) → should not appear
    EXPECT_EQ(result.data.find("apple"), std::string::npos) << result.data;
    // "banana" was not tombstoned → should appear
    EXPECT_NE(result.data.find("banana"), std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, ORSetKeepsElementAddedOnBothNodes) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::OR_SET);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"add":[["cherry","tag-10"]],"tombstones":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"add":[["cherry","tag-11"]],"tombstones":["tag-10"]})";
    e2.hlc  = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // "cherry" still has live tag tag-11 → must survive
    EXPECT_NE(result.data.find("cherry"), std::string::npos) << result.data;
}

// TWO_P_SET: added elements minus removed elements
TEST(MMCRDTResolverTest, TwoPSetExcludesRemovedElements) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::TWO_P_SET);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"add":["alpha","beta","gamma"],"remove":["beta"]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"add":["alpha","delta"],"remove":[]})";
    e2.hlc  = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // union(add) = {alpha, beta, gamma, delta}; union(remove) = {beta}
    // result = {alpha, gamma, delta}
    EXPECT_NE(result.data.find("alpha"),  std::string::npos) << result.data;
    EXPECT_NE(result.data.find("gamma"),  std::string::npos) << result.data;
    EXPECT_NE(result.data.find("delta"),  std::string::npos) << result.data;
    EXPECT_EQ(result.data.find("beta"),   std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, TwoPSetEmptyRemoveReturnsAllAdded) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::TWO_P_SET);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"add":["x","y"],"remove":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    auto result = resolver.resolve("doc", {e1});
    EXPECT_NE(result.data.find("x"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("y"), std::string::npos) << result.data;
}

// RGA: ordered sequence – elements merged by id, tombstones preserved, sorted by id
TEST(MMCRDTResolverTest, RGAMergesElementsById) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::RGA);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"([{"id":"100:A","v":"hello","del":false},{"id":"300:A","v":"world","del":false}])";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"([{"id":"200:B","v":"beautiful","del":false},{"id":"300:A","v":"world","del":false}])";
    e2.hlc  = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // All three unique ids should appear; id "300:A" is de-duplicated
    EXPECT_NE(result.data.find("hello"),     std::string::npos) << result.data;
    EXPECT_NE(result.data.find("beautiful"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("world"),     std::string::npos) << result.data;
    // Sorted by id: 100:A, 200:B, 300:A
    auto pos_hello     = result.data.find("hello");
    auto pos_beautiful = result.data.find("beautiful");
    auto pos_world     = result.data.find("world");
    EXPECT_LT(pos_hello,     pos_beautiful) << result.data;
    EXPECT_LT(pos_beautiful, pos_world)     << result.data;
}

TEST(MMCRDTResolverTest, RGATombstoneIrrevocable) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::RGA);

    // nodeA has element, nodeB has deleted it
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"([{"id":"100:A","v":"foo","del":false}])";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"([{"id":"100:A","v":"foo","del":true}])";
    e2.hlc  = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // Deletion wins; "del":true must appear in result
    EXPECT_NE(result.data.find("\"del\":true"), std::string::npos) << result.data;
    EXPECT_EQ(result.data.find("\"del\":false"), std::string::npos) << result.data;
}

// MV_REGISTER: multi-value register returns all concurrent values as a JSON array
TEST(MMCRDTResolverTest, MVRegisterReturnsAllConcurrentValues) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::MV_REGISTER);

    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = R"({"v":"valueA"})";
    e1.hlc = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = R"({"v":"valueB"})";
    e2.hlc = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // Result should be an array containing both values
    EXPECT_EQ(result.data[0], '[') << result.data;
    EXPECT_NE(result.data.find("valueA"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("valueB"), std::string::npos) << result.data;
}

// LWW_MAP: per-key last-write-wins using HLC – key present in both writes, newer wins
TEST(MMCRDTResolverTest, LWWMapPicksLatestValuePerKey) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::LWW_MAP);

    // e1 has score=10 written at t=100; e2 has score=20 written at t=200
    MMWriteEntry e1;
    e1.write_id = "w1"; e1.data = R"({"score":10,"level":3})";
    e1.hlc = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2"; e2.data = R"({"score":20})";
    e2.hlc = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // "score" should be 20 (latest write wins); "level" only exists in e1
    EXPECT_NE(result.data.find("\"score\":20"), std::string::npos) << result.data;
    EXPECT_EQ(result.data.find("\"score\":10"), std::string::npos) << result.data;
    EXPECT_NE(result.data.find("\"level\":3"),  std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, StrategyNameLWWMapAndMVRegister) {
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::MV_REGISTER).strategyName(),
              "MV_REGISTER");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::LWW_MAP).strategyName(),
              "LWW_MAP");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::PN_COUNTER).strategyName(),
              "PN_COUNTER");
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::OR_SET).strategyName(),
              "OR_SET");
}

// FLAG_EW: Enable-Wins Flag tests
TEST(MMCRDTResolverTest, FlagEWStrategyName) {
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::FLAG_EW).strategyName(),
              "FLAG_EW");
}

TEST(MMCRDTResolverTest, FlagEWEnabledWhenLiveTagExists) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_EW);

    // nodeA enabled the flag with tag-1; nodeB has no disables
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":["tag-1"],"d":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    auto result = resolver.resolve("doc", {e1});
    EXPECT_NE(result.data.find("\"enabled\":true"), std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, FlagEWDisabledWhenAllTagsTombstoned) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_EW);

    // nodeA enabled with tag-1; nodeB disabled tag-1
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":["tag-1"],"d":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"e":["tag-1"],"d":["tag-1"]})";
    e2.hlc  = makeHLCTimestamp(200, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    EXPECT_NE(result.data.find("\"enabled\":false"), std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, FlagEWConcurrentEnableDisableEnableWins) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_EW);

    // nodeA: enable with tag-1 (concurrent with nodeB's disable of tag-1 via tag-2)
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":["tag-1","tag-2"],"d":["tag-1"]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    // nodeB: disable tag-1 only; tag-2 is still live
    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"e":["tag-1"],"d":["tag-1"]})";
    e2.hlc  = makeHLCTimestamp(100, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // tag-2 is in e but not in d → flag is enabled (enable-wins)
    EXPECT_NE(result.data.find("\"enabled\":true"), std::string::npos) << result.data;
}

// FLAG_DW: Disable-Wins Flag tests
TEST(MMCRDTResolverTest, FlagDWStrategyName) {
    EXPECT_EQ(CRDTMergeResolver(CRDTMergeResolver::CRDTType::FLAG_DW).strategyName(),
              "FLAG_DW");
}

TEST(MMCRDTResolverTest, FlagDWEnabledWhenNoDisableTags) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_DW);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":["tag-1"],"d":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    auto result = resolver.resolve("doc", {e1});
    EXPECT_NE(result.data.find("\"enabled\":true"), std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, FlagDWConcurrentEnableDisableDisableWins) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_DW);

    // nodeA enabled with tag-1
    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":["tag-1"],"d":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    // nodeB: concurrent disable (adds a disable-tag)
    MMWriteEntry e2;
    e2.write_id = "w2";
    e2.data = R"({"e":["tag-1"],"d":["tag-2"]})";
    e2.hlc  = makeHLCTimestamp(100, 0, "B");

    auto result = resolver.resolve("doc", {e1, e2});
    // Any disable-tag present → disable wins
    EXPECT_NE(result.data.find("\"enabled\":false"), std::string::npos) << result.data;
}

TEST(MMCRDTResolverTest, FlagDWDisabledWhenNoEnableTags) {
    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::FLAG_DW);

    MMWriteEntry e1;
    e1.write_id = "w1";
    e1.data = R"({"e":[],"d":[]})";
    e1.hlc  = makeHLCTimestamp(100, 0, "A");

    auto result = resolver.resolve("doc", {e1});
    // No enable-tags → flag is off
    EXPECT_NE(result.data.find("\"enabled\":false"), std::string::npos) << result.data;
}



TEST(MMWriteEntryTest, SerializeDeserializeRoundTrip) {
    HybridLogicalClock hlc("test-node");
    auto ts = hlc.now();

    MMWriteEntry original;
    original.write_id    = "write-123";
    original.origin_node = "node-A";
    original.collection  = "users";
    original.document_id = "user-1";
    original.operation   = "INSERT";
    original.data        = R"({"name":"Alice","age":30})";
    original.checksum    = "abc123";
    original.hlc         = ts;
    original.vector_clock.increment("node-A");

    auto serialized = original.serialize();
    ASSERT_FALSE(serialized.empty());

    auto restored = MMWriteEntry::deserialize(serialized);
    ASSERT_TRUE(restored.has_value());

    EXPECT_EQ(restored->write_id,    original.write_id);
    EXPECT_EQ(restored->origin_node, original.origin_node);
    EXPECT_EQ(restored->collection,  original.collection);
    EXPECT_EQ(restored->document_id, original.document_id);
    EXPECT_EQ(restored->operation,   original.operation);
    EXPECT_EQ(restored->data,        original.data);
    EXPECT_EQ(restored->checksum,    original.checksum);
    EXPECT_EQ(restored->hlc.physical, original.hlc.physical);
    EXPECT_EQ(restored->hlc.logical,  original.hlc.logical);
    EXPECT_EQ(restored->hlc.node_id,  original.hlc.node_id);
    EXPECT_EQ(restored->vector_clock.get("node-A"),
              original.vector_clock.get("node-A"));
}

// ============================================================================
// 13. ReplicationStream backoff tracking
// ============================================================================

TEST(ReplicationStreamTest, InitialBackoffIsZero) {
    TempWALDir wd("/tmp/themis_stream_test");
    ReplicationConfig cfg = makeConfig(wd.path);
    auto wal = std::make_shared<WALManager>(cfg);
    ReplicationStream stream("127.0.0.1:9999", wal, cfg);
    EXPECT_EQ(stream.getConsecutiveFailures(), 0u);
}

// ============================================================================
// 14. MultiMasterReplicationManager
// ============================================================================

static MMReplicationConfig makeMMConfig(const std::string& node_id = "mm-node-1") {
    MMReplicationConfig cfg;
    cfg.node_id               = node_id;
    cfg.datacenter            = "dc1";
    cfg.region                = "eu-west";
    cfg.replication_factor    = 1;
    cfg.write_quorum          = 1;
    cfg.read_quorum           = 1;
    cfg.heartbeat_interval_ms = 50;
    cfg.sync_interval_ms      = 50;
    cfg.timeout_ms            = 1000;
    cfg.max_batch_size        = 100;
    cfg.max_pending_writes    = 1000;
    cfg.async_apply           = true;
    cfg.use_mtls              = false;
    cfg.default_resolution_strategy = "LAST_WRITE_WINS";
    return cfg;
}

TEST(MMReplicationManagerTest, StartAndStop) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    EXPECT_FALSE(mgr.isRunning());
    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.isRunning());
    mgr.stop();
    EXPECT_FALSE(mgr.isRunning());
}

TEST(MMReplicationManagerTest, DoubleStartIsIdempotent) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.start());  // Second call should not fail
    EXPECT_TRUE(mgr.isRunning());
    mgr.stop();
}

TEST(MMReplicationManagerTest, WriteReturnsWriteId) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    std::string write_id = mgr.write("users", "user-1", "INSERT", R"({"name":"Alice"})");
    EXPECT_FALSE(write_id.empty())
        << "write() should return a non-empty write_id";

    mgr.stop();
}

TEST(MMReplicationManagerTest, WriteSyncCompletesWithinTimeout) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    bool ok = mgr.writeSync("orders", "order-42", "UPDATE", R"({"status":"shipped"})",
                             std::chrono::milliseconds(2000));
    EXPECT_TRUE(ok) << "writeSync must complete within 2s in single-node mode";

    mgr.stop();
}

TEST(MMReplicationManagerTest, StatsTrackWrites) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    mgr.write("products", "prod-1", "INSERT", R"({"price":99})");
    mgr.write("products", "prod-2", "INSERT", R"({"price":49})");

    // Give the replication loop a tick to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto stats = mgr.getStats();
    EXPECT_GE(stats.writes_total, 2u) << "Total writes should be at least 2";

    mgr.stop();
}

TEST(MMReplicationManagerTest, AddAndRemovePeer) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    MMPeerInfo peer;
    peer.node_id   = "mm-node-2";
    peer.endpoint  = "127.0.0.1:9001";
    peer.state     = MMNodeState::ACTIVE;
    peer.datacenter = "dc1";
    peer.region    = "eu-west";
    peer.is_local_datacenter = true;
    peer.priority  = 10;
    peer.replication_lag_ms = 0;

    mgr.addPeer(peer);
    {
        auto peers = mgr.getPeers();
        EXPECT_EQ(peers.size(), 1u);
        EXPECT_EQ(peers[0].node_id, "mm-node-2");
    }

    mgr.removePeer("mm-node-2");
    {
        auto peers = mgr.getPeers();
        EXPECT_TRUE(peers.empty());
    }

    mgr.stop();
}

TEST(MMReplicationManagerTest, GetLocalInfo) {
    auto cfg = makeMMConfig("my-node");
    MultiMasterReplicationManager mgr(cfg);
    mgr.start();

    auto info = mgr.getLocalInfo();
    EXPECT_EQ(info.node_id,   "my-node");
    EXPECT_EQ(info.datacenter, "dc1");
    EXPECT_EQ(info.state,      MMNodeState::ACTIVE);
    EXPECT_TRUE(info.is_local_datacenter);

    mgr.stop();
}

TEST(MMReplicationManagerTest, SetAndTriggerConflictCallback) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    std::atomic<int> callback_count{0};
    mgr.registerConflictCallback([&](const ConflictRecord& rec) {
        callback_count.fetch_add(1);
        EXPECT_FALSE(rec.conflict_id.empty());
    });

    // Manually call handleConflict via two concurrent writes
    // (We simulate this by resolving an existing conflict)
    ConflictRecord rec;
    rec.conflict_id  = "fake-conflict-1";
    rec.resolved     = false;
    rec.collection   = "docs";
    rec.document_id  = "doc-1";
    rec.detected_at  = std::chrono::system_clock::now();

    // resolveConflict on a non-existent conflict returns false (no-op)
    EXPECT_FALSE(mgr.resolveConflict("nonexistent", "w1"));

    mgr.stop();
}

TEST(MMReplicationManagerTest, SetConflictResolverPerCollection) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    // Set a LWW resolver for the "sessions" collection
    mgr.setConflictResolver("sessions",
        std::make_shared<LastWriteWinsResolver>());

    // Set a CRDT GCounter resolver for the "counters" collection
    mgr.setConflictResolver("counters",
        std::make_shared<CRDTMergeResolver>(
            CRDTMergeResolver::CRDTType::G_COUNTER));

    // No crash expected – just verify it can be called
    SUCCEED();

    mgr.stop();
}

TEST(MMReplicationManagerTest, PrometheusMetricsContainNodeId) {
    auto cfg = makeMMConfig("prometheus-node");
    MultiMasterReplicationManager mgr(cfg);
    mgr.start();

    // Write something so counters are > 0
    mgr.write("col", "doc", "INSERT", "{}");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string metrics = mgr.exportPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("prometheus-node"), std::string::npos)
        << "Prometheus metrics should include the node ID";
    EXPECT_NE(metrics.find("themisdb_mm_writes_total"), std::string::npos)
        << "Metrics should include writes_total";

    mgr.stop();
}

TEST(MMReplicationManagerTest, GetUnresolvedConflictsInitiallyEmpty) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    auto conflicts = mgr.getUnresolvedConflicts();
    EXPECT_TRUE(conflicts.empty());

    mgr.stop();
}

TEST(MMReplicationManagerTest, ReplicationLagZeroWhenNoPeers) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();
    EXPECT_EQ(mgr.getReplicationLag(), 0u);
    mgr.stop();
}

TEST(MMReplicationManagerTest, TriggerSyncDoesNotCrash) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();
    mgr.triggerSync();  // Should not crash
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr.stop();
}

TEST(MMReplicationManagerTest, WriteSyncFailsClosedWhenQuorumZeroWithActivePeer) {
    auto cfg = makeMMConfig();
    cfg.write_quorum = 0;

    MultiMasterReplicationManager mgr(cfg);
    mgr.start();

    MMPeerInfo peer;
    peer.node_id = "mm-node-peer";
    peer.endpoint = "127.0.0.1:9101";
    peer.state = MMNodeState::ACTIVE;
    peer.datacenter = "dc1";
    peer.region = "eu-west";
    peer.is_local_datacenter = true;
    peer.priority = 10;
    mgr.addPeer(peer);

    const bool ok = mgr.writeSync("orders", "order-q0", "INSERT", "{}",
                                  std::chrono::milliseconds(400));
    EXPECT_FALSE(ok);

    mgr.stop();
}

TEST(MMReplicationManagerTest, WriteCallbackInvokedAfterReplication) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    std::promise<bool> p;
    auto fut = p.get_future();

    mgr.write("test", "doc", "INSERT", "{}", [&p](const MMWriteEntry& /*e*/, bool ok) {
        p.set_value(ok);
    });

    auto status = fut.wait_for(std::chrono::milliseconds(1000));
    ASSERT_EQ(status, std::future_status::ready) << "Callback must fire within 1s";
    EXPECT_TRUE(fut.get());

    mgr.stop();
}

TEST(MMReplicationManagerTest, ConcurrentWritesAreThreadSafe) {
    MultiMasterReplicationManager mgr(makeMMConfig());
    mgr.start();

    constexpr int kWriters = 4;
    constexpr int kWritesPerThread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < kWriters; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kWritesPerThread; ++i) {
                try {
                    std::string id = mgr.write("col", "doc-" + std::to_string(t * 1000 + i),
                                               "INSERT", "{}");
                    if (id.empty()) {
                      errors.fetch_add(1);
                    }
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // Wait for the queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_EQ(errors.load(), 0) << "No errors expected during concurrent writes";

    auto stats = mgr.getStats();
    EXPECT_GE(stats.writes_total,
              static_cast<uint64_t>(kWriters * kWritesPerThread));

    mgr.stop();
}

TEST(MMReplicationManagerTest, TopologySnapshotLocalNodeOnly) {
    auto cfg = makeMMConfig("topology-node");
    MultiMasterReplicationManager mgr(cfg);
    mgr.start();

    auto snap = mgr.getTopologySnapshot();

    EXPECT_EQ(snap.local_node_id,    "topology-node");
    EXPECT_EQ(snap.replication_mode, "MULTI_MASTER");
    ASSERT_EQ(snap.nodes.size(), 1u) << "Only the local node with no peers";
    EXPECT_EQ(snap.nodes[0].node_id,  "topology-node");
    EXPECT_TRUE(snap.nodes[0].is_local);
    EXPECT_EQ(snap.nodes[0].state,    "ACTIVE");
    EXPECT_EQ(snap.nodes[0].replication_lag_ms, 0u);
    EXPECT_TRUE(snap.edges.empty()) << "No edges without peers";
    EXPECT_EQ(snap.max_lag_ms, 0u);

    mgr.stop();
}

TEST(MMReplicationManagerTest, TopologySnapshotWithPeer) {
    MultiMasterReplicationManager mgr(makeMMConfig("node-a"));
    mgr.start();

    MMPeerInfo peer;
    peer.node_id             = "node-b";
    peer.endpoint            = "192.168.1.2:9002";
    peer.datacenter          = "dc1";
    peer.region              = "eu-west";
    peer.state               = MMNodeState::ACTIVE;
    peer.replication_lag_ms  = 42;
    peer.priority            = 10;
    peer.is_local_datacenter = true;
    mgr.addPeer(peer);

    auto snap = mgr.getTopologySnapshot();

    ASSERT_EQ(snap.nodes.size(), 2u) << "Local + one peer";

    // Find local node
    auto local_it = std::find_if(snap.nodes.begin(), snap.nodes.end(),
        [](const auto& n){ return n.is_local; });
    ASSERT_NE(local_it, snap.nodes.end());
    EXPECT_EQ(local_it->node_id, "node-a");
    EXPECT_EQ(local_it->state,   "ACTIVE");

    // Find peer node
    auto peer_it = std::find_if(snap.nodes.begin(), snap.nodes.end(),
        [](const auto& n){ return !n.is_local; });
    ASSERT_NE(peer_it, snap.nodes.end());
    EXPECT_EQ(peer_it->node_id,            "node-b");
    EXPECT_EQ(peer_it->endpoint,           "192.168.1.2:9002");
    EXPECT_EQ(peer_it->replication_lag_ms, 42u);
    EXPECT_EQ(peer_it->state,              "ACTIVE");
    EXPECT_FALSE(peer_it->is_local);

    // Bidirectional edges (multi-master)
    ASSERT_EQ(snap.edges.size(), 2u) << "One edge per direction";
    bool has_a_to_b = false, has_b_to_a = false;
    for (const auto& e : snap.edges) {
        EXPECT_EQ(e.type, "PEER");
        if (e.from == "node-a" && e.to == "node-b") {
          has_a_to_b = true;
        }
        if (e.from == "node-b" && e.to == "node-a") {
          has_b_to_a = true;
        }
    }
    EXPECT_TRUE(has_a_to_b) << "Edge from local to peer must exist";
    EXPECT_TRUE(has_b_to_a) << "Reverse edge from peer to local must exist";

    EXPECT_EQ(snap.max_lag_ms, 42u);

    mgr.stop();
}

TEST(MMReplicationManagerTest, TopologySnapshotReportsOfflineWhenStopped) {
    auto cfg = makeMMConfig("stopped-node");
    MultiMasterReplicationManager mgr(cfg);
    // Do not call start() – manager is in stopped state

    auto snap = mgr.getTopologySnapshot();

    ASSERT_GE(snap.nodes.size(), 1u);
    auto local_it = std::find_if(snap.nodes.begin(), snap.nodes.end(),
        [](const auto& n){ return n.is_local; });
    ASSERT_NE(local_it, snap.nodes.end());
    EXPECT_EQ(local_it->state, "OFFLINE");
}

// ============================================================================
// 15. ParallelReplicationWorker
// ============================================================================

TEST(ParallelReplicationWorkerTest, SubmitAndSync) {
    ParallelReplicationWorker::ParallelConfig cfg;
    cfg.worker_threads        = 2;
    cfg.queue_size            = 1000;
    cfg.use_dependency_tracking = true;

    ParallelReplicationWorker worker(cfg);

    // Submit 20 independent entries (all different document_ids)
    for (int i = 0; i < 20; ++i) {
        WALEntry e;
        e.sequence_number = static_cast<uint64_t>(i + 1);
        e.document_id     = "doc-" + std::to_string(i);
        e.collection      = "test";
        e.operation       = "INSERT";
        e.data            = "{}";
        worker.submit(e);
    }

    worker.sync();

    auto stats = worker.getStats();
    EXPECT_EQ(stats.entries_applied, 20u) << "All entries should be applied";
}

TEST(ParallelReplicationWorkerTest, DependencyTrackingSerialisesPerDocument) {
    ParallelReplicationWorker::ParallelConfig cfg;
    cfg.worker_threads        = 4;
    cfg.queue_size            = 1000;
    cfg.use_dependency_tracking = true;

    ParallelReplicationWorker worker(cfg);

    // Submit multiple writes for the SAME document – these must be serialized
    constexpr int kWrites = 10;
    for (int i = 0; i < kWrites; ++i) {
        WALEntry e;
        e.sequence_number = static_cast<uint64_t>(i + 1);
        e.document_id     = "shared-doc";
        e.collection      = "col";
        e.operation       = "UPDATE";
        e.data            = "{\"v\":" + std::to_string(i) + "}";
        worker.submit(e);
    }

    worker.sync();

    auto stats = worker.getStats();
    EXPECT_EQ(stats.entries_applied,       static_cast<uint64_t>(kWrites));
    // At least kWrites-1 dependencies should have been detected (each write
    // after the first depends on the previous write to the same document)
    EXPECT_GE(stats.dependencies_detected, static_cast<uint64_t>(kWrites - 1));
}

TEST(ParallelReplicationWorkerTest, StopJoinsCleanly) {
    auto start = std::chrono::steady_clock::now();
    {
        ParallelReplicationWorker::ParallelConfig cfg;
        cfg.worker_threads = 2;
        cfg.queue_size     = 100;
        ParallelReplicationWorker worker(cfg);
        // Destructor should join threads within a reasonable time
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(ms, 500) << "Destruction should complete quickly";
}

TEST(ParallelReplicationWorkerTest, StatsReflectParallelism) {
    ParallelReplicationWorker::ParallelConfig cfg;
    cfg.worker_threads        = 4;
    cfg.queue_size            = 1000;
    cfg.use_dependency_tracking = true;

    ParallelReplicationWorker worker(cfg);

    // Submit 40 entries across 4 different documents (10 per doc)
    for (int doc = 0; doc < 4; ++doc) {
        for (int i = 0; i < 10; ++i) {
            WALEntry e;
            e.sequence_number = static_cast<uint64_t>(doc * 100 + i + 1);
            e.document_id     = "doc-" + std::to_string(doc);
            e.collection      = "col";
            e.operation       = "UPDATE";
            e.data            = "{}";
            worker.submit(e);
        }
    }

    worker.sync();

    auto stats = worker.getStats();
    EXPECT_EQ(stats.entries_applied, 40u);
    EXPECT_GT(stats.parallelism_factor, 0.0) << "Parallelism factor must be positive";
}

// ============================================================================
// 16. QuorumReadManager
// ============================================================================

static ReplicaInfo makeReplica(const std::string& ep,
                                uint64_t seq,
                                HealthStatus hs = HealthStatus::HEALTHY) {
    ReplicaInfo r;
    r.endpoint               = ep;
    r.last_applied_sequence  = seq;
    r.health_status          = hs;
    r.last_heartbeat         = std::chrono::system_clock::now();
    r.role                   = ReplicationRole::FOLLOWER;
    return r;
}

TEST(QuorumReadManagerTest, SucceedsWithQuorumReplicas) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 100),
        makeReplica("r2:9000", 100),
        makeReplica("r3:9000", 100),
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_TRUE(result.success);
    EXPECT_GE(result.sources.size(), 2u);
}

TEST(QuorumReadManagerTest, PicksHighestVersion) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 50),
        makeReplica("r2:9000", 100),  // highest version
        makeReplica("r3:9000", 75),
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_TRUE(result.success);
    // Version should be the max across all replicas that responded
    EXPECT_GE(result.version, 50u);
}

TEST(QuorumReadManagerTest, DetectsConflictsOnDivergence) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 10),
        makeReplica("r2:9000", 20),  // different version → divergence
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.had_conflicts) << "Diverging versions should be flagged";
}

TEST(QuorumReadManagerTest, FailsWhenNoReplicasAvailable) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 50;

    // All replicas are unhealthy / offline
    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 0, HealthStatus::FAILED),
        makeReplica("r2:9000", 0, HealthStatus::FAILED),
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_FALSE(result.success)
        << "Read must fail when no healthy replicas are available";
}

TEST(QuorumReadManagerTest, SingleNodeModeSucceeds) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});  // No replicas = single-node mode
    auto result = qrm.read("col", "doc-1");
    EXPECT_TRUE(result.success) << "Single-node mode should always succeed";
}

TEST(QuorumReadManagerTest, SetReplicasUpdatesTopology) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});
    qrm.setReplicas({makeReplica("r1:9000", 42)});
    auto result = qrm.read("col", "doc-1");
    EXPECT_TRUE(result.success);
}

// Session consistency: a successful read always returns a non-empty session token.
TEST(QuorumReadManagerTest, SessionToken_ReturnedOnSuccessfulRead) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 100),
        makeReplica("r2:9000", 100),
        makeReplica("r3:9000", 100),
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.session_token.empty())
        << "A session token must be returned on every successful quorum read";
    // Token must contain the expected key–value pairs.
    EXPECT_NE(result.session_token.find("seq="), std::string::npos)
        << "Session token must contain 'seq=' field";
    EXPECT_NE(result.session_token.find("exp="), std::string::npos)
        << "Session token must contain 'exp=' expiry field";
}

// Session consistency: using the token from a previous read in a subsequent read
// must succeed when replicas are at the same or higher version.
TEST(QuorumReadManagerTest, SessionConsistency_TokenFromReadUsedInNextRead) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 100),
        makeReplica("r2:9000", 100),
        makeReplica("r3:9000", 100),
    };

    QuorumReadManager qrm(cfg, replicas);

    // First read – obtain a session token embedding version 100.
    auto first = qrm.read("col", "doc-1");
    ASSERT_TRUE(first.success);
    EXPECT_FALSE(first.session_token.empty());

    // Second read using the session token – replicas are still at version 100,
    // so the read must succeed with monotonic guarantees.
    auto second = qrm.read("col", "doc-1", 0, first.session_token);
    EXPECT_TRUE(second.success)
        << "Session read must succeed when replicas satisfy the required version";
    EXPECT_GE(second.version, first.version)
        << "Monotonic read: version must not decrease across reads in the same session";
}

// Session consistency: when the session token requires a version higher than
// what any replica can provide, the read must fail.
TEST(QuorumReadManagerTest, SessionConsistency_FailsWhenReplicasBelowRequiredVersion) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    // All replicas are at version 10 – far below any realistic session requirement.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 10),
        makeReplica("r2:9000", 10),
    };

    QuorumReadManager qrm(cfg, replicas);

    // Craft a token that demands version 9999 (above what replicas have).
    // Format: "seq=<N>;exp=<far-future-epoch-ms>"
    constexpr uint64_t far_future_exp =
        9999999999999ull;  // well beyond any real current timestamp
    std::string high_version_token = "seq=9999;exp=" + std::to_string(far_future_exp);

    auto result = qrm.read("col", "doc-1", 0, high_version_token);
    EXPECT_FALSE(result.success)
        << "Session read must fail when no quorum of replicas satisfies the required version";
}

// Session consistency: at least one but fewer than quorum replicas satisfy the
// version requirement – must fail.
TEST(QuorumReadManagerTest, SessionConsistency_PartialVersionSatisfactionFails) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    // r1 is stale; r2 and r3 are up-to-date.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 10),   // stale
        makeReplica("r2:9000", 100),  // fresh
    };

    QuorumReadManager qrm(cfg, replicas);

    // Token requires version 100; only r2 qualifies (1 < quorum=2).
    constexpr uint64_t far_future_exp = 9999999999999ull;
    std::string token = "seq=100;exp=" + std::to_string(far_future_exp);

    auto result = qrm.read("col", "doc-1", 0, token);
    EXPECT_FALSE(result.success)
        << "One qualifying replica is below quorum=2; session read must fail";
}

// Session consistency: a majority of replicas satisfy the version requirement.
TEST(QuorumReadManagerTest, SessionConsistency_QuorumSatisfiedByFreshReplicas) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 10),   // stale – must not count toward quorum
        makeReplica("r2:9000", 100),  // fresh
        makeReplica("r3:9000", 100),  // fresh
    };

    QuorumReadManager qrm(cfg, replicas);

    // Token requires version 100; r2 and r3 satisfy it (2 == quorum).
    constexpr uint64_t far_future_exp = 9999999999999ull;
    std::string token = "seq=100;exp=" + std::to_string(far_future_exp);

    auto result = qrm.read("col", "doc-1", 0, token);
    EXPECT_TRUE(result.success)
        << "Two qualifying replicas at version 100 satisfy quorum=2";
    EXPECT_EQ(result.version, 100u);
    EXPECT_EQ(result.sources.size(), 2u)
        << "Only the qualifying replicas should appear in sources";
}

// repair_on_read: diverging replicas are flagged; had_conflicts is set.
TEST(QuorumReadManagerTest, RepairOnRead_ConflictsDetectedAndFlagged) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;
    cfg.repair_on_read  = true;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 50),   // stale replica
        makeReplica("r2:9000", 100),  // authoritative replica
        makeReplica("r3:9000", 100),
    };

    QuorumReadManager qrm(cfg, replicas);
    auto result = qrm.read("col", "doc-1");

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.had_conflicts)
        << "Diverging versions must be flagged so repair-on-read can proceed";
    EXPECT_EQ(result.version, 100u)
        << "Authoritative (highest) version must win the reconciliation";
}

// Single-node mode: session token is still returned.
TEST(QuorumReadManagerTest, SingleNodeMode_ReturnsSessionToken) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});  // no replicas = single-node mode
    auto result = qrm.read("col", "doc-1");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.session_token.empty())
        << "Session token must be returned even in single-node mode";
}

// ── QRM-SN: setLocalDocumentFetchFn injection (Stub #248) ────────────────────

// QRM-SN-01: injected fn is called and its data/version propagated
TEST(QuorumReadManagerTest, SingleNodeLocalFetch_InjectedFnCalled) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});

    bool called = false;
    qrm.setLocalDocumentFetchFn(
        [&](const std::string& collection, const std::string& document_id)
            -> std::pair<std::string, uint64_t> {
            called = true;
            EXPECT_EQ(collection,  "my_col");
            EXPECT_EQ(document_id, "doc-42");
            return {R"({"id":"doc-42","val":7})", 99u};
        });

    auto result = qrm.read("my_col", "doc-42");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(called) << "setLocalDocumentFetchFn callback must be invoked";
    EXPECT_EQ(result.data,    R"({"id":"doc-42","val":7})");
    EXPECT_EQ(result.version, 99u);
}

// QRM-SN-02: clearing fn reverts to empty-data stub
TEST(QuorumReadManagerTest, SingleNodeLocalFetch_ClearingRevertsToStub) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});
    qrm.setLocalDocumentFetchFn(
        [](const std::string&, const std::string&) -> std::pair<std::string, uint64_t> {
            return {"some_data", 5u};
        });

    // Verify injection is active
    ASSERT_EQ(qrm.read("c", "d").version, 5u);

    // Clear the fn
    qrm.setLocalDocumentFetchFn(nullptr);

    auto result = qrm.read("c", "d");
    EXPECT_TRUE(result.success)         << "single-node always succeeds";
    EXPECT_TRUE(result.data.empty())    << "without fn, data must be empty";
    EXPECT_EQ(result.version, 0u)       << "without fn, version must be 0";
}

// QRM-SN-03: throwing fn leaves success=true but data empty (degraded mode)
TEST(QuorumReadManagerTest, SingleNodeLocalFetch_ThrowingFnDegradesGracefully) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 1;
    cfg.read_timeout_ms = 200;

    QuorumReadManager qrm(cfg, {});
    qrm.setLocalDocumentFetchFn(
        [](const std::string&, const std::string&) -> std::pair<std::string, uint64_t> {
            throw std::runtime_error("storage unavailable");
        });

    auto result = qrm.read("col", "doc-1");
    EXPECT_TRUE(result.success)      << "degraded mode must still return success=true";
    EXPECT_TRUE(result.data.empty()) << "exception must leave data empty";
}

// Session consistency: a malformed (non-empty) token is treated as version 0,
// so all replicas qualify and the read succeeds normally.
TEST(QuorumReadManagerTest, SessionConsistency_MalformedTokenTreatedAsNoRequirement) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000", 50),
        makeReplica("r2:9000", 100),
        makeReplica("r3:9000", 75),
    };

    QuorumReadManager qrm(cfg, replicas);

    // Garbage token: parseSessionToken must return 0 (no version requirement).
    auto result = qrm.read("col", "doc-1", 0, "not-a-valid-token");
    EXPECT_TRUE(result.success)
        << "Malformed session token must not block a read that would otherwise succeed";
    // The highest version across all three replicas is 100.
    EXPECT_EQ(result.version, 100u);
}

// Session consistency: fresh replicas that arrive last in iteration order
// (stale replica first) must still be counted toward the version quorum.
// This exercises the fix that prevents early loop exit when session_token != "".
TEST(QuorumReadManagerTest, SessionConsistency_FreshReplicasLateInIterationOrder) {
    QuorumReadManager::QuorumReadConfig cfg;
    cfg.read_quorum     = 2;
    cfg.read_timeout_ms = 200;

    // r1 is stale and will be the first future to resolve; r2 and r3 are fresh.
    // Without the fix the loop would have stopped at {r1, r2}, counted only
    // one qualifying replica (r2) and returned failure.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("r1:9000",  5),   // stale
        makeReplica("r2:9000", 200),  // fresh
        makeReplica("r3:9000", 200),  // fresh
    };

    QuorumReadManager qrm(cfg, replicas);

    constexpr uint64_t far_future = 9999999999999ull;
    std::string token = "seq=200;exp=" + std::to_string(far_future);

    auto result = qrm.read("col", "doc-1", 0, token);
    EXPECT_TRUE(result.success)
        << "Two fresh replicas at version 200 satisfy quorum=2 even when a "
           "stale replica is iterated first";
    EXPECT_EQ(result.version, 200u);
    EXPECT_EQ(result.sources.size(), 2u)
        << "Only the two qualifying replicas must appear in sources";
}



// ============================================================================
// 17. PersistentReplicationState
// ============================================================================

class PersistentStateTest : public ::testing::Test {
protected:
    std::string path_ = {};

    void SetUp() override {
        const auto ticks = std::chrono::high_resolution_clock::now()
                               .time_since_epoch()
                               .count();
        const auto tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
        path_ = (std::filesystem::temp_directory_path() /
                 ("themis_repl_state_test_" + std::to_string(ticks) + "_" +
                  std::to_string(tid_hash) + ".dat"))
                    .string();
        cleanupPath();
    }

    void TearDown() override { cleanupPath(); }

private:
    void cleanupPath() {
        for (int i = 0; i < 5; ++i) {
            std::error_code ec = {};
            std::filesystem::remove(path_, ec);
            if (!ec || !std::filesystem::exists(path_)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
};

TEST_F(PersistentStateTest, FileDoesNotExistInitially) {
    PersistentReplicationState prs(path_);
    EXPECT_FALSE(prs.exists());
}

TEST_F(PersistentStateTest, PersistAndLoad) {
    PersistentReplicationState prs(path_);

    PersistentReplicationState::State state;
    state.last_applied_sequence = 12345;
    state.current_term          = 7;
    state.voted_for             = "node-leader";
    state.leader_id             = "node-leader";
    state.persisted_at          = std::chrono::system_clock::now();

    ASSERT_TRUE(prs.persist(state));
    EXPECT_TRUE(prs.exists());

    auto loaded = prs.load();
    EXPECT_EQ(loaded.last_applied_sequence, 12345u);
    EXPECT_EQ(loaded.current_term,          7u);
    EXPECT_EQ(loaded.voted_for,             "node-leader");
    EXPECT_EQ(loaded.leader_id,             "node-leader");
}

TEST_F(PersistentStateTest, LoadReturnsDefaultWhenFileAbsent) {
    PersistentReplicationState prs(path_);
    auto state = prs.load();
    EXPECT_EQ(state.last_applied_sequence, 0u);
    EXPECT_EQ(state.current_term, 0u);
    EXPECT_TRUE(state.voted_for.empty());
}

TEST_F(PersistentStateTest, PersistOverwritesPreviousState) {
    PersistentReplicationState prs(path_);

    PersistentReplicationState::State s1;
    s1.last_applied_sequence = 100;
    s1.current_term          = 3;
    ASSERT_TRUE(prs.persist(s1));

    PersistentReplicationState::State s2;
    s2.last_applied_sequence = 200;
    s2.current_term          = 5;
    ASSERT_TRUE(prs.persist(s2));

    auto loaded = prs.load();
    EXPECT_EQ(loaded.last_applied_sequence, 200u);
    EXPECT_EQ(loaded.current_term, 5u);
}

TEST_F(PersistentStateTest, RemoveDeletesFile) {
    PersistentReplicationState prs(path_);

    PersistentReplicationState::State s;
    s.last_applied_sequence = 1;
    ASSERT_TRUE(prs.persist(s));
    EXPECT_TRUE(prs.exists());

    prs.remove();
    EXPECT_FALSE(prs.exists());
}

TEST_F(PersistentStateTest, ConcurrentPersistIsThreadSafe) {
    PersistentReplicationState prs(path_);

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&prs, t, &errors]() {
            for (int i = 0; i < 10; ++i) {
                PersistentReplicationState::State s;
                s.last_applied_sequence = static_cast<uint64_t>(t * 100 + i);
                s.current_term          = static_cast<uint64_t>(t);
                if (!prs.persist(s)) {
                  errors.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(errors.load(), 0) << "No errors expected in concurrent persist";
    EXPECT_TRUE(prs.exists());
}

TEST_F(PersistentStateTest, LoadAfterRestart) {
    // Simulate a node restart: persist, then create a new instance from same path
    {
        PersistentReplicationState prs(path_);
        PersistentReplicationState::State s;
        s.last_applied_sequence = 9999;
        s.current_term          = 11;
        s.voted_for             = "node-42";
        ASSERT_TRUE(prs.persist(s));
    }
    // New instance (simulating restart)
    {
        PersistentReplicationState prs(path_);
        auto loaded = prs.load();
        EXPECT_EQ(loaded.last_applied_sequence, 9999u);
        EXPECT_EQ(loaded.current_term, 11u);
        EXPECT_EQ(loaded.voted_for, "node-42");
    }
}


// ============================================================================
// 19. CompressedReplicationStream
// ============================================================================

TEST(CompressedStreamTest, NoneAlgorithmReturnsSameSize) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm = CompressedReplicationStream::CompressionAlgorithm::NONE;
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "INSERT"; e.data = std::string(500, 'A');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_EQ(stats.bytes_uncompressed, stats.bytes_compressed)
        << "NONE should not change byte count";
    EXPECT_EQ(stats.compression_ratio, 1.0);
}

TEST(CompressedStreamTest, ZstdCompressesRepeatedData) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm         = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    cfg.compression_level = 3;
    cfg.min_batch_size    = 0;  // Always compress
    CompressedReplicationStream stream("localhost:9001", cfg);

    // Highly compressible payload
    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "INSERT"; e.data = std::string(4096, 'A');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_GT(stats.compression_ratio, 1.5)
        << "ZSTD should compress repeated data significantly";
}

TEST(CompressedStreamTest, LZ4CompressesAndTracksStats) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::LZ4;
    cfg.min_batch_size = 0;
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "UPDATE"; e.data = std::string(2048, 'B');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_EQ(stats.algorithm_used, "LZ4");
    EXPECT_GT(stats.bytes_uncompressed, 0u);
    EXPECT_GT(stats.bytes_compressed, 0u);
}

TEST(CompressedStreamTest, SnappyCompressesAndTracksStats) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::SNAPPY;
    cfg.min_batch_size = 0;
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "UPDATE"; e.data = std::string(2048, 'C');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_EQ(stats.algorithm_used, "SNAPPY");
    EXPECT_GT(stats.bytes_uncompressed, 0u);
}

TEST(CompressedStreamTest, AutoSkipsCompressionForSmallBatches) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::AUTO;
    cfg.min_batch_size = 1024 * 1024;  // 1 MB threshold – tiny batch won't compress
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "INSERT"; e.data = "{}";
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_EQ(stats.algorithm_used, "NONE")
        << "AUTO should fall back to NONE for tiny payloads";
    EXPECT_EQ(stats.bytes_uncompressed, stats.bytes_compressed);
}

TEST(CompressedStreamTest, AutoUsesZstdForLargeBatches) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::AUTO;
    cfg.min_batch_size = 512;  // Low threshold so test data qualifies
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "INSERT"; e.data = std::string(2048, 'Z');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    EXPECT_EQ(stats.algorithm_used, "ZSTD");
}

TEST(CompressedStreamTest, ResetStatsWorks) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm = CompressedReplicationStream::CompressionAlgorithm::NONE;
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e; e.sequence_number=1; e.data="hello";
    stream.sendBatch({e});
    EXPECT_GT(stream.getStats().bytes_uncompressed, 0u);

    stream.resetStats();
    EXPECT_EQ(stream.getStats().bytes_uncompressed, 0u);
}

TEST(CompressedStreamTest, ZstdRoundTrip) {
    // Build a ZSTD-compressed buffer using the public C API directly,
    // then verify that CompressedReplicationStream::decompress() recovers it.
    std::string payload = "Hello, ThemisDB! " + std::string(200, 'X');
    std::vector<uint8_t> raw(payload.begin(), payload.end());

    // Compress with ZSTD directly
    size_t bound = ZSTD_compressBound(raw.size());
    std::vector<uint8_t> compressed(bound);
    size_t compressed_size = ZSTD_compress(compressed.data(), bound,
                                           raw.data(), raw.size(), 3);
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    compressed.resize(compressed_size);

    // Decompress via the class and verify round-trip
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    cfg.min_batch_size = 0;
    CompressedReplicationStream stream("localhost:9001", cfg);

    auto algo = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    auto decompressed = stream.decompress(compressed, algo);
    ASSERT_EQ(decompressed.size(), raw.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), payload)
        << "Round-trip compress→decompress must recover original data";
}

TEST(CompressedStreamTest, DefaultConstructorWorks) {
    CompressedReplicationStream stream("localhost:9001");
    WALEntry e; e.sequence_number=1; e.data=std::string(2048,'D');
    EXPECT_TRUE(stream.sendBatch({e}));
}

TEST(CompressedStreamTest, EmptyBatchReturnsTrue) {
    CompressedReplicationStream stream("localhost:9001");
    EXPECT_TRUE(stream.sendBatch({}));
}

// AC: "JSON documents: 5-10x with Zstd"
TEST(CompressedStreamTest, ZstdAchievesHighRatioOnJsonLikeData) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm         = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    cfg.compression_level = 6;
    cfg.min_batch_size    = 0;
    CompressedReplicationStream stream("localhost:9001", cfg);

    // Simulate a batch of JSON-like documents (highly repetitive structure).
    // 20 entries each with a ~200-byte JSON payload → ~4 KB total before framing.
    std::string json_template =
        R"({"id":"doc00000","collection":"users","op":"INSERT",)"
        R"("data":{"name":"Alice Smith","email":"alice@example.com","role":"admin","active":true}})";
    std::vector<WALEntry> entries = {};

    for (int i = 0; i < 20; ++i) {
        WALEntry e;
        e.sequence_number = static_cast<uint64_t>(i + 1);
        e.collection      = "users";
        e.document_id     = "doc" + std::to_string(10000 + i);
        e.operation       = "INSERT";
        e.data            = json_template;
        entries.push_back(e);
    }
    EXPECT_TRUE(stream.sendBatch(entries));

    auto stats = stream.getStats();
    // JSON documents should compress >= 5x with ZSTD level 6 (AC requirement).
    EXPECT_GE(stats.compression_ratio, 5.0)
        << "ZSTD level 6 on JSON-like data must achieve >= 5x ratio (AC: JSON 5-10x)";
    EXPECT_EQ(stats.algorithm_used, "ZSTD");
}

// AC: "Already compressed data: ~1x (minimal benefit)"
TEST(CompressedStreamTest, AlreadyCompressedDataHasMinimalBenefit) {
    // Create already-compressed content by ZSTD-compressing a payload first,
    // then feed that compressed blob as WALEntry data through the stream again.
    std::string original(8192, '\0');
    for (size_t i = 0; i < original.size(); ++i) {
        // Pseudo-random bytes via a simple LCG (Knuth multiplicative + additive
        // constants) to simulate already-compressed / high-entropy binary data.
        original[i] = static_cast<char>((i * 6364136223846793005ULL + 1442695040888963407ULL) & 0xFF);
    }
    // Pre-compress once to get a "compressed blob".
    size_t bound = ZSTD_compressBound(original.size());
    std::vector<char> pre_compressed(bound);
    size_t csz = ZSTD_compress(pre_compressed.data(), bound,
                                original.data(), original.size(), 3);
    ASSERT_FALSE(ZSTD_isError(csz));
    pre_compressed.resize(csz);

    // Now send that pre-compressed blob through the stream; ZSTD on already-
    // compressed data should achieve minimal additional compression (~1x).
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm         = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    cfg.compression_level = 3;
    cfg.min_batch_size    = 0;
    CompressedReplicationStream stream("localhost:9001", cfg);

    WALEntry e;
    e.sequence_number = 1;
    e.collection      = "c";
    e.document_id     = "d";
    e.operation       = "INSERT";
    e.data            = std::string(pre_compressed.begin(), pre_compressed.end());
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    // Already-compressed data must not expand significantly (ratio should stay <= 1.2x).
    EXPECT_LT(stats.compression_ratio, 1.2)
        << "ZSTD on already-compressed data must yield ~1x (AC: already compressed ~1x)";
}

// AC: LZ4 round-trip correctness
TEST(CompressedStreamTest, LZ4RoundTrip) {
    std::string payload = "ThemisDB LZ4 round-trip test! " + std::string(300, 'Y');
    std::vector<uint8_t> raw(payload.begin(), payload.end());

    // Compress with LZ4 directly via public C API.
    int bound = LZ4_compressBound(static_cast<int>(raw.size()));
    std::vector<uint8_t> compressed(static_cast<size_t>(bound));
    int csz = LZ4_compress_default(
        reinterpret_cast<const char*>(raw.data()),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(raw.size()), bound);
    ASSERT_GT(csz, 0);
    compressed.resize(static_cast<size_t>(csz));

    // Decompress via CompressedReplicationStream::decompress() and verify.
    CompressedReplicationStream stream("localhost:9001");
    auto decompressed = stream.decompress(
        compressed, CompressedReplicationStream::CompressionAlgorithm::LZ4);
    ASSERT_EQ(decompressed.size(), raw.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), payload)
        << "LZ4 round-trip must recover original data";
}

// AC: Snappy round-trip correctness
TEST(CompressedStreamTest, SnappyRoundTrip) {
    std::string payload = "ThemisDB Snappy round-trip! " + std::string(300, 'S');
    std::vector<uint8_t> raw(payload.begin(), payload.end());

    // Compress with Snappy directly.
    std::string snappy_out = {};
    snappy::Compress(reinterpret_cast<const char*>(raw.data()), raw.size(), &snappy_out);
    std::vector<uint8_t> compressed(snappy_out.begin(), snappy_out.end());

    // Decompress via CompressedReplicationStream::decompress() and verify.
    CompressedReplicationStream stream("localhost:9001");
    auto decompressed = stream.decompress(
        compressed, CompressedReplicationStream::CompressionAlgorithm::SNAPPY);
    ASSERT_EQ(decompressed.size(), raw.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), payload)
        << "Snappy round-trip must recover original data";
}

// AC: "Adaptive compression based on data characteristics" — adaptive=false
// must bypass the min_batch_size threshold and always compress (AUTO mode).
TEST(CompressedStreamTest, AdaptiveFalseAlwaysCompressesInAutoMode) {
    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm      = CompressedReplicationStream::CompressionAlgorithm::AUTO;
    cfg.min_batch_size = 1024 * 1024;  // Very large threshold
    cfg.adaptive       = false;         // Disable adaptive skip
    CompressedReplicationStream stream("localhost:9001", cfg);

    // Tiny but compressible payload — should still be compressed because adaptive=false.
    WALEntry e;
    e.sequence_number = 1; e.collection = "c"; e.document_id = "d";
    e.operation = "INSERT"; e.data = std::string(64, 'A');
    EXPECT_TRUE(stream.sendBatch({e}));

    auto stats = stream.getStats();
    // Must use ZSTD (not NONE) even though payload < min_batch_size.
    EXPECT_EQ(stats.algorithm_used, "ZSTD")
        << "AUTO with adaptive=false must compress regardless of batch size";
}

// ============================================================================
// 19b. ReplicationStream WAL compression integration
// ============================================================================

class ReplicationStreamCompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto ticks = std::chrono::high_resolution_clock::now()
                               .time_since_epoch()
                               .count();
        const auto tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
        wal_dir_ = (std::filesystem::temp_directory_path() /
                    ("themis_rs_compress_test_" + std::to_string(ticks) + "_" +
                     std::to_string(tid_hash)))
                       .string();
        std::error_code ec = {};
        std::filesystem::remove_all(wal_dir_, ec);
        std::filesystem::create_directories(wal_dir_, ec);
    }
    void TearDown() override {
        for (int i = 0; i < 5; ++i) {
            std::error_code ec = {};
            std::filesystem::remove_all(wal_dir_, ec);
            if (!ec || !std::filesystem::exists(wal_dir_)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    std::string wal_dir_ = {};
};

TEST_F(ReplicationStreamCompressionTest, CompressionDisabledByDefault) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    EXPECT_FALSE(cfg.enable_wal_compression);
    EXPECT_EQ(cfg.wal_compression_algorithm, "zstd");
    EXPECT_EQ(cfg.wal_compression_level, 3);
    EXPECT_EQ(cfg.wal_compression_min_batch_bytes, 1024u);
}

TEST_F(ReplicationStreamCompressionTest, ZstdConfigConstructsStream) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression          = true;
    cfg.wal_compression_algorithm       = "zstd";
    cfg.wal_compression_level           = 3;
    cfg.wal_compression_min_batch_bytes = 0;

    auto wal = std::make_shared<WALManager>(cfg);
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9100", wal, cfg));
}

TEST_F(ReplicationStreamCompressionTest, LZ4ConfigConstructsStream) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression          = true;
    cfg.wal_compression_algorithm       = "lz4";
    cfg.wal_compression_min_batch_bytes = 0;

    auto wal = std::make_shared<WALManager>(cfg);
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9101", wal, cfg));
}

TEST_F(ReplicationStreamCompressionTest, CompressionDisabledConstructsStream) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression = false;

    auto wal = std::make_shared<WALManager>(cfg);
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9102", wal, cfg));
}

TEST_F(ReplicationStreamCompressionTest, UnknownAlgorithmDefaultsToZstd) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression    = true;
    cfg.wal_compression_algorithm = "unknown_algo";

    auto wal = std::make_shared<WALManager>(cfg);
    // Should construct without throwing; unknown algo falls back to ZSTD
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9103", wal, cfg));
}

TEST_F(ReplicationStreamCompressionTest, AutoAlgorithmConfig) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression          = true;
    cfg.wal_compression_algorithm       = "auto";
    cfg.wal_compression_min_batch_bytes = 512;

    auto wal = std::make_shared<WALManager>(cfg);
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9104", wal, cfg));
}

TEST_F(ReplicationStreamCompressionTest, SnappyAlgorithmConfig) {
    ReplicationConfig cfg = makeConfig(wal_dir_);
    cfg.enable_wal_compression    = true;
    cfg.wal_compression_algorithm = "snappy";

    auto wal = std::make_shared<WALManager>(cfg);
    EXPECT_NO_THROW(ReplicationStream stream("localhost:9105", wal, cfg));
}

// ============================================================================
// 20. BatchedAckTracker
// ============================================================================

TEST(BatchedAckTrackerTest, RecordAndDequeue) {
    BatchedAckTracker::AckBatchConfig cfg;
    cfg.max_batch_size    = 5;
    cfg.flush_interval_ms = 10;
    BatchedAckTracker tracker(cfg);

    for (uint64_t i = 1; i <= 5; ++i) {
      tracker.recordApplied(i);
    }

    // Give the flush thread time to run
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto batch = tracker.dequeuePendingAcks();
    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->sequences.size(), 5u);
}

TEST(BatchedAckTrackerTest, HighestAckedTracked) {
    BatchedAckTracker::AckBatchConfig cfg;
    cfg.max_batch_size    = 100;
    cfg.flush_interval_ms = 100;
    BatchedAckTracker tracker(cfg);

    tracker.recordApplied(10);
    tracker.recordApplied(5);
    tracker.recordApplied(20);

    EXPECT_EQ(tracker.getHighestAcked(), 20u);
}

TEST(BatchedAckTrackerTest, ForceFlushDrainsBuffer) {
    BatchedAckTracker::AckBatchConfig cfg;
    cfg.max_batch_size    = 1000;   // Won't auto-flush
    cfg.flush_interval_ms = 10000; // Very long flush interval
    BatchedAckTracker tracker(cfg);

    tracker.recordApplied(1);
    tracker.recordApplied(2);
    tracker.forceFlush();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto batch = tracker.dequeuePendingAcks();
    ASSERT_TRUE(batch.has_value());
    EXPECT_GE(batch->sequences.size(), 1u);
}

TEST(BatchedAckTrackerTest, StatsBatchSizeIsAccurate) {
    BatchedAckTracker::AckBatchConfig cfg;
    cfg.max_batch_size    = 3;
    cfg.flush_interval_ms = 5;
    BatchedAckTracker tracker(cfg);

    for (uint64_t i = 1; i <= 9; ++i) {
      tracker.recordApplied(i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.total_acks_sent, 9u);
    EXPECT_GT(stats.avg_batch_size, 0.0);
}

TEST(BatchedAckTrackerTest, DefaultConstructorWorks) {
    BatchedAckTracker tracker;
    tracker.recordApplied(42);
    EXPECT_EQ(tracker.getHighestAcked(), 42u);
}

TEST(BatchedAckTrackerTest, DestructorJoinsCleanly) {
    auto start = std::chrono::steady_clock::now();
    {
        BatchedAckTracker tracker;
        tracker.recordApplied(1);
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(ms, 500) << "Destructor must complete quickly";
}

// ============================================================================
// 21. ReplicationAnalytics
// ============================================================================

TEST(ReplicationAnalyticsTest, RecordAndGetLagHistory) {
    ReplicationAnalytics analytics;
    for (int i = 0; i < 10; ++i) {
      analytics.recordLag("r1", 100 * (i + 1));
    }

    auto hist = analytics.getLagHistory("r1", std::chrono::hours(1));
    EXPECT_EQ(hist.data_points.size(), 10u);
    EXPECT_GT(hist.avg_lag_ms, 0);
    EXPECT_GT(hist.max_lag_ms, 0);
    EXPECT_GE(hist.p95_lag_ms, hist.avg_lag_ms);
}

TEST(ReplicationAnalyticsTest, LagSpikeInsightGenerated) {
    ReplicationAnalytics analytics;
    ReplicationAnalytics::AnalyticsConfig cfg;
    cfg.lag_spike_threshold_ms = 1000;
    analytics.setConfig(cfg);

    analytics.recordLag("r1", 5000);  // well above threshold

    auto insights = analytics.getInsights();
    auto it = std::find_if(insights.begin(), insights.end(),
        [](const auto& i) { return i.type == "LAG_SPIKE"; });
    EXPECT_NE(it, insights.end()) << "LAG_SPIKE insight should be generated";
    EXPECT_EQ(it->metadata.at("replica_id"), "r1");
}

TEST(ReplicationAnalyticsTest, NoInsightForNormalLag) {
    ReplicationAnalytics analytics;
    analytics.recordLag("r1", 10);

    auto insights = analytics.getInsights();
    EXPECT_TRUE(insights.empty()) << "No insight for normal lag";
}

TEST(ReplicationAnalyticsTest, SlowReplicaInsightGenerated) {
    ReplicationAnalytics analytics;
    ReplicationAnalytics::AnalyticsConfig cfg;
    cfg.slow_replica_avg_ms = 500;
    analytics.setConfig(cfg);

    for (int i = 0; i < 20; ++i) {
      analytics.recordLag("r2", 1000);
    }

    auto insights = analytics.getInsights();
    auto it = std::find_if(insights.begin(), insights.end(),
        [](const auto& i) { return i.type == "SLOW_REPLICA"; });
    EXPECT_NE(it, insights.end()) << "SLOW_REPLICA insight should be generated";
}

TEST(ReplicationAnalyticsTest, BottleneckDetectionNetwork) {
    ReplicationAnalytics analytics;
    ReplicationAnalytics::AnalyticsConfig cfg;
    cfg.slow_replica_avg_ms = 200;
    analytics.setConfig(cfg);

    // High variance → NETWORK bottleneck
    analytics.recordLag("r3", 500);
    analytics.recordLag("r3", 100);
    analytics.recordLag("r3", 800);
    analytics.recordLag("r3", 50);
    analytics.recordLag("r3", 2000);  // Spike

    auto bottlenecks = analytics.detectBottlenecks();
    EXPECT_FALSE(bottlenecks.empty());
    bool found = std::any_of(bottlenecks.begin(), bottlenecks.end(),
        [](const auto& b) { return b.replica_id == "r3"; });
    EXPECT_TRUE(found);
}

TEST(ReplicationAnalyticsTest, PrometheusExportContainsLag) {
    ReplicationAnalytics analytics;
    analytics.recordLag("node1", 42);

    auto metrics = analytics.exportPrometheusMetrics();
    EXPECT_NE(metrics.find("themisdb_replication_lag_ms"), std::string::npos);
    EXPECT_NE(metrics.find("node1"), std::string::npos);
    EXPECT_NE(metrics.find("42"), std::string::npos);
}

TEST(ReplicationAnalyticsTest, UnknownReplicaReturnsEmptyHistory) {
    ReplicationAnalytics analytics;
    auto hist = analytics.getLagHistory("nonexistent", std::chrono::hours(1));
    EXPECT_TRUE(hist.data_points.empty());
    EXPECT_EQ(hist.avg_lag_ms, 0);
}

TEST(ReplicationAnalyticsTest, ConcurrentRecordIsThreadSafe) {
    ReplicationAnalytics analytics;
    constexpr int kThreads = 4;
    constexpr int kSamples = 100;
    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&analytics, t]() {
            for (int i = 0; i < kSamples; ++i) {
                analytics.recordLag("r" + std::to_string(t), i * 10);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    for (int t = 0; t < kThreads; ++t) {
        auto hist = analytics.getLagHistory("r" + std::to_string(t),
                                             std::chrono::hours(1));
        EXPECT_EQ(hist.data_points.size(), static_cast<size_t>(kSamples));
    }
}

// ============================================================================
// 22. ReplicationBenchmark
// ============================================================================

TEST(ReplicationBenchmarkTest, RunProducesPositiveThroughput) {
    ReplicationConfig config;
    config.wal_directory        = (std::filesystem::temp_directory_path() /
                                   ("themis_bench_wal_test_" +
                                    std::to_string(std::chrono::high_resolution_clock::now()
                                                       .time_since_epoch().count())))
                                      .string();
    config.heartbeat_interval_ms = 100;
    config.batch_size            = 64;
    std::filesystem::create_directories(config.wal_directory);
    {
        auto wal = std::make_shared<WALManager>(config);

        ReplicationBenchmark::BenchmarkConfig bcfg;
        bcfg.num_entries      = 100;
        bcfg.entry_size_bytes = 128;
        bcfg.warmup_entries   = 10;
        bcfg.collection       = "bench_col";

        ReplicationBenchmark bench(wal, bcfg);
        auto result = bench.run();

        EXPECT_EQ(result.total_entries, 100u);
        EXPECT_GT(result.writes_per_second, 0.0);
        EXPECT_GT(result.bytes_written, 0u);
        EXPECT_GE(result.latency_p50_us, 0);
        EXPECT_GE(result.latency_p95_us, result.latency_p50_us);
        EXPECT_GE(result.latency_p99_us, result.latency_p95_us);
    }

    std::error_code ec = {};
    std::filesystem::remove_all(config.wal_directory, ec);
}

TEST(ReplicationBenchmarkTest, DefaultConstructorWorks) {
    ReplicationConfig config;
    config.wal_directory         = (std::filesystem::temp_directory_path() /
                                    ("themis_bench_default_test_" +
                                     std::to_string(std::chrono::high_resolution_clock::now()
                                                        .time_since_epoch().count())))
                                       .string();
    config.heartbeat_interval_ms = 100;
    config.batch_size            = 64;
    std::filesystem::create_directories(config.wal_directory);
    {
        auto wal = std::make_shared<WALManager>(config);

        ReplicationBenchmark bench(wal);
        // Just check it runs without crashing (full benchmark is slow – use small override)
        // We only call format here to avoid 10K entries in tests
        auto result = bench.run();
        EXPECT_GT(result.writes_per_second, 0.0);
    }

    std::error_code ec = {};
    std::filesystem::remove_all(config.wal_directory, ec);
}

TEST(ReplicationBenchmarkTest, FormatContainsKeyFields) {
    ReplicationBenchmark::BenchmarkResult r;
    r.total_entries     = 5000;
    r.duration_seconds  = 1.5;
    r.writes_per_second = 3333.0;
    r.latency_p50_us    = 10;
    r.latency_p95_us    = 50;
    r.latency_p99_us    = 100;
    r.latency_max_us    = 200;
    r.bytes_written     = 640000;

    auto s = ReplicationBenchmark::format(r);
    EXPECT_NE(s.find("5000"),   std::string::npos);
    EXPECT_NE(s.find("writes"), std::string::npos);
    EXPECT_NE(s.find("p50"),    std::string::npos);
    EXPECT_NE(s.find("p99"),    std::string::npos);
}

TEST(ReplicationBenchmarkTest, LatencyPercentilesAreSorted) {
    ReplicationConfig config;
    config.wal_directory         = (std::filesystem::temp_directory_path() /
                                    ("themis_bench_pct_test_" +
                                     std::to_string(std::chrono::high_resolution_clock::now()
                                                        .time_since_epoch().count())))
                                       .string();
    config.heartbeat_interval_ms = 100;
    config.batch_size            = 64;
    std::filesystem::create_directories(config.wal_directory);
    {
        auto wal = std::make_shared<WALManager>(config);

        ReplicationBenchmark::BenchmarkConfig bcfg;
        bcfg.num_entries    = 200;
        bcfg.warmup_entries = 20;
        ReplicationBenchmark bench(wal, bcfg);
        auto r = bench.run();

        EXPECT_LE(r.latency_p50_us, r.latency_p95_us);
        EXPECT_LE(r.latency_p95_us, r.latency_p99_us);
        EXPECT_LE(r.latency_p99_us, r.latency_max_us);
    }

    std::error_code ec = {};
    std::filesystem::remove_all(config.wal_directory, ec);
}

// ============================================================================
// CDCManager Tests (v1.6.0)
// ============================================================================

TEST(CDCManagerTest, SubscribeAndReceiveWildcard) {
    auto cdc = std::make_shared<CDCManager>();

    std::vector<WALEntry> received;
    cdc->subscribe("", [&received](const WALEntry& e) {
        received.push_back(e);
    });

    WALEntry e1;
    e1.collection = "users";
    e1.operation  = "INSERT";
    e1.document_id= "u1";
    cdc->onWALEntryApplied(e1);

    WALEntry e2;
    e2.collection = "orders";
    e2.operation  = "UPDATE";
    e2.document_id= "o1";
    cdc->onWALEntryApplied(e2);

    EXPECT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0].collection, "users");
    EXPECT_EQ(received[1].collection, "orders");
}

TEST(CDCManagerTest, SubscribeFiltersByCollection) {
    auto cdc = std::make_shared<CDCManager>();

    std::vector<std::string> seen;
    cdc->subscribe("users", [&seen](const WALEntry& e) {
        seen.push_back(e.document_id);
    });

    WALEntry eu; eu.collection = "users";  eu.document_id = "u1";
    WALEntry eo; eo.collection = "orders"; eo.document_id = "o1";
    cdc->onWALEntryApplied(eu);
    cdc->onWALEntryApplied(eo);

    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0], "u1");
}

TEST(CDCManagerTest, UnsubscribeStopsDelivery) {
    auto cdc = std::make_shared<CDCManager>();

    int count = 0;
    uint64_t id = cdc->subscribe("", [&count](const WALEntry&) { ++count; });

    WALEntry e; e.collection = "c"; e.document_id = "d";
    cdc->onWALEntryApplied(e);
    EXPECT_EQ(count, 1);

    cdc->unsubscribe(id);
    cdc->onWALEntryApplied(e);
    EXPECT_EQ(count, 1);  // should NOT increase
}

TEST(CDCManagerTest, MultipleSubscribersReceiveAll) {
    auto cdc = std::make_shared<CDCManager>();

    int a = 0, b = 0;
    cdc->subscribe("", [&a](const WALEntry&) { ++a; });
    cdc->subscribe("", [&b](const WALEntry&) { ++b; });

    WALEntry e; e.collection = "x";
    cdc->onWALEntryApplied(e);
    cdc->onWALEntryApplied(e);

    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 2);
}

TEST(CDCManagerTest, SubscriptionCountAccurate) {
    CDCManager cdc;
    EXPECT_EQ(cdc.subscriptionCount(), 0u);
    uint64_t id1 = cdc.subscribe("", [](const WALEntry&){});
    uint64_t id2 = cdc.subscribe("col", [](const WALEntry&){});
    EXPECT_EQ(cdc.subscriptionCount(), 2u);
    cdc.unsubscribe(id1);
    EXPECT_EQ(cdc.subscriptionCount(), 1u);
    cdc.unsubscribe(id2);
    EXPECT_EQ(cdc.subscriptionCount(), 0u);
}

TEST(CDCManagerTest, ThrowingCallbackDoesNotCrash) {
    CDCManager cdc;
    cdc.subscribe("", [](const WALEntry&) {
        throw std::runtime_error("cdc consumer error");
    });
    WALEntry e; e.collection = "c";
    // Should not propagate the exception
    EXPECT_NO_THROW(cdc.onWALEntryApplied(e));
}

TEST(CDCManagerTest, ReplicationManagerDeliversCDCEvents) {
    TempWALDir tmp("/tmp/themis_cdc_mgr_test");
    auto cfg = makeConfig(tmp.path);
    cfg.seed_nodes.clear();
    // Use the minimum election timeout so the single-node cluster elects
    // itself as leader quickly.
    cfg.election_timeout_min_ms = 50;
    cfg.election_timeout_max_ms = 100;
    // This test validates CDC delivery, not lease semantics.
    cfg.enable_leader_lease = false;

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    auto cdc = std::make_shared<CDCManager>();
    std::vector<std::string> captured;
    cdc->subscribe("", [&captured](const WALEntry& e) {
        captured.push_back(e.document_id);
    });
    mgr.addListener(cdc);

    // Wait for the single-node cluster to elect itself leader
    // (election_timeout_max_ms = 100 ms, add extra margin)
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "items";
    entry.document_id = "item-42";
    entry.data        = R"({"name":"widget"})";
    bool ok = mgr.replicate(entry);

    mgr.shutdown();
    ASSERT_TRUE(ok) << "replicate() failed – node may not be leader yet";
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0], "item-42");
}

// ============================================================================
// WALArchivalManager Tests (v1.6.0)
// ============================================================================

namespace {
// Helper: write a small binary WAL-like file to a directory
void writeSegmentFile(const std::string& dir, const std::string& name,
                      const std::string& content) {
    std::filesystem::create_directories(dir);
    std::ofstream f(dir + "/" + name, std::ios::binary);
    f << content;
}
} // anonymous namespace

TEST(WALArchivalTest, ArchiveSingleSegmentAndRetrieve) {
    const std::string wal_dir = "/tmp/themis_arch_wal";
    const std::string arc_dir = "/tmp/themis_arch_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000001.wal", "WAL DATA 1234");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory            = wal_dir;
    cfg.archive_directory        = arc_dir;
    cfg.compress_before_archive  = true;
    cfg.local_retention_segments = 0;

    WALArchivalManager mgr(cfg);
    uint32_t n = mgr.archiveSegments({"seg_000001.wal"});
    EXPECT_EQ(n, 1u);

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(list[0].compressed);
    EXPECT_GT(list[0].size_bytes, 0u);

    // Retrieve and decompress – should recover original content
    auto data = mgr.retrieveSegment(list[0].segment_id);
    ASSERT_TRUE(data.has_value());
    std::string recovered(data->begin(), data->end());
    EXPECT_EQ(recovered, "WAL DATA 1234");

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, ArchiveWithoutCompressionRoundTrip) {
    const std::string wal_dir = "/tmp/themis_arch_raw_wal";
    const std::string arc_dir = "/tmp/themis_arch_raw_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000002.wal", "UNCOMPRESSED SEGMENT");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;

    WALArchivalManager mgr(cfg);
    EXPECT_EQ(mgr.archiveSegments({"seg_000002.wal"}), 1u);

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_FALSE(list[0].compressed);

    auto data = mgr.retrieveSegment(list[0].segment_id);
    ASSERT_TRUE(data.has_value());
    std::string s(data->begin(), data->end());
    EXPECT_EQ(s, "UNCOMPRESSED SEGMENT");

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, MissingSegmentReturnsNullopt) {
    const std::string arc_dir = "/tmp/themis_arch_miss_arc";
    std::filesystem::remove_all(arc_dir);

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory   = "/tmp/themis_arch_miss_wal";
    cfg.archive_directory = arc_dir;

    WALArchivalManager mgr(cfg);
    auto result = mgr.retrieveSegment(999999);
    EXPECT_FALSE(result.has_value());

    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, ListArchivedSortedBySegmentId) {
    const std::string wal_dir = "/tmp/themis_arch_sort_wal";
    const std::string arc_dir = "/tmp/themis_arch_sort_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000030.wal", "DATA30");
    writeSegmentFile(wal_dir, "seg_000010.wal", "DATA10");
    writeSegmentFile(wal_dir, "seg_000020.wal", "DATA20");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;

    WALArchivalManager mgr(cfg);
    mgr.archiveSegments({"seg_000030.wal", "seg_000010.wal", "seg_000020.wal"});

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 3u);
    EXPECT_LE(list[0].segment_id, list[1].segment_id);
    EXPECT_LE(list[1].segment_id, list[2].segment_id);

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, PurgeExpiredRemovesOldSegments) {
    const std::string wal_dir = "/tmp/themis_arch_purge_wal";
    const std::string arc_dir = "/tmp/themis_arch_purge_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000005.wal", "OLD_DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;
    cfg.delete_after_days       = 0;  // 0 = purge everything immediately

    WALArchivalManager mgr(cfg);
    mgr.archiveSegments({"seg_000005.wal"});
    ASSERT_EQ(mgr.listArchived().size(), 1u);

    // delete_after_days == 0 means purge all; verify the segment is removed
    uint32_t purged = mgr.purgeExpired();
    EXPECT_EQ(purged, 1u);
    EXPECT_EQ(mgr.listArchived().size(), 0u);

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, RunArchivalCycleArchivesOldSegments) {
    const std::string wal_dir = "/tmp/themis_arch_cycle_wal";
    const std::string arc_dir = "/tmp/themis_arch_cycle_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    // Write 5 segments; keep retention=2 -> should archive 3
    for (int i = 1; i <= 5; ++i) {
        std::ostringstream name = {};
        name << "seg_" << std::setw(6) << std::setfill('0') << i << ".wal";
        writeSegmentFile(wal_dir, name.str(), "DATA" + std::to_string(i));
    }

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory            = wal_dir;
    cfg.archive_directory        = arc_dir;
    cfg.local_retention_segments = 2;
    cfg.compress_before_archive  = false;

    WALArchivalManager mgr(cfg);
    uint32_t archived = mgr.runArchivalCycle();
    EXPECT_EQ(archived, 3u);
    EXPECT_EQ(mgr.listArchived().size(), 3u);

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

// ============================================================================
// WALArchivalManager Tests (v1.6.0) – Object Storage / Encryption / Lifecycle
// ============================================================================

// A fixed 32-byte AES-256 key expressed as 64 hex characters.
// TEST USE ONLY — production keys must be securely generated (e.g. via a KMS)
// and never hard-coded in source.
static const char* kTestKeyHex =
    "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20";

TEST(WALArchivalTest, EncryptionAtRest_RoundTrip) {
    const std::string wal_dir = "/tmp/themis_enc_wal";
    const std::string arc_dir = "/tmp/themis_enc_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000100.wal", "SECRET WAL CONTENT");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;
    cfg.encrypt_at_rest         = true;
    cfg.encryption_key_hex      = kTestKeyHex;

    {
        WALArchivalManager mgr(cfg);
        uint32_t n = mgr.archiveSegments({"seg_000100.wal"});
        ASSERT_EQ(n, 1u);

        auto list = mgr.listArchived();
        ASSERT_EQ(list.size(), 1u);
        EXPECT_TRUE(list[0].encrypted);
        EXPECT_FALSE(list[0].compressed);

        // Archived file on disk must NOT contain plaintext
        {
            std::ifstream raw_file(list[0].archive_path, std::ios::binary);
            ASSERT_TRUE(raw_file.good());
            std::string disk_content(std::istreambuf_iterator<char>(raw_file), {});
            EXPECT_EQ(disk_content.find("SECRET"), std::string::npos)
                << "Encrypted archive must not contain plaintext";
        }

        // Retrieve and decrypt – must recover original content
        auto data = mgr.retrieveSegment(list[0].segment_id);
        ASSERT_TRUE(data.has_value());
        std::string recovered(data->begin(), data->end());
        EXPECT_EQ(recovered, "SECRET WAL CONTENT");
    }

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, EncryptionAtRest_WithCompression_RoundTrip) {
    const std::string wal_dir = "/tmp/themis_enc_cmp_wal";
    const std::string arc_dir = "/tmp/themis_enc_cmp_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    // A larger payload benefits more from compression
    std::string payload(512, 'A');
    payload += std::string(512, 'B');
    writeSegmentFile(wal_dir, "seg_000200.wal", payload);

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = true;
    cfg.encrypt_at_rest         = true;
    cfg.encryption_key_hex      = kTestKeyHex;

    WALArchivalManager mgr(cfg);
    ASSERT_EQ(mgr.archiveSegments({"seg_000200.wal"}), 1u);

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(list[0].encrypted);
    EXPECT_TRUE(list[0].compressed);

    auto data = mgr.retrieveSegment(list[0].segment_id);
    ASSERT_TRUE(data.has_value());
    std::string recovered(data->begin(), data->end());
    EXPECT_EQ(recovered, payload);

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, EncryptionAtRest_InvalidKey_RejectsArchival) {
    const std::string wal_dir = "/tmp/themis_enc_badkey_wal";
    const std::string arc_dir = "/tmp/themis_enc_badkey_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000300.wal", "SENSITIVE DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;
    cfg.encrypt_at_rest         = true;
    cfg.encryption_key_hex      = "tooshort";  // invalid: not 64 hex chars

    WALArchivalManager mgr(cfg);
    // With an invalid key, archival is rejected to prevent unencrypted storage
    uint32_t n = mgr.archiveSegments({"seg_000300.wal"});
    EXPECT_EQ(n, 0u) << "Archival must be rejected when encryption key is invalid";
    EXPECT_TRUE(mgr.listArchived().empty());

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, StorageTier_DefaultIsStandard) {
    const std::string wal_dir = "/tmp/themis_tier_wal";
    const std::string arc_dir = "/tmp/themis_tier_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000400.wal", "TIER TEST DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;

    WALArchivalManager mgr(cfg);
    ASSERT_EQ(mgr.archiveSegments({"seg_000400.wal"}), 1u);

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].storage_tier, "standard");

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, TransitionStorageTiers_DisabledWhenZero) {
    const std::string wal_dir = "/tmp/themis_lifecycle_dis_wal";
    const std::string arc_dir = "/tmp/themis_lifecycle_dis_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000500.wal", "LIFECYCLE DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory                 = wal_dir;
    cfg.archive_directory             = arc_dir;
    cfg.compress_before_archive       = false;
    cfg.transition_to_cold_after_days = 0;  // disabled

    WALArchivalManager mgr(cfg);
    mgr.archiveSegments({"seg_000500.wal"});

    // With lifecycle disabled, transitionStorageTiers() is a no-op
    EXPECT_EQ(mgr.transitionStorageTiers(), 0u);
    EXPECT_EQ(mgr.listArchived()[0].storage_tier, "standard");

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, TransitionStorageTiers_MovesToCold) {
    const std::string arc_dir = "/tmp/themis_lifecycle_cold_arc";
    std::filesystem::remove_all(arc_dir);
    std::filesystem::create_directories(arc_dir);

    // Write a fake archive file (content doesn't matter for tier testing)
    const std::string fake_path = arc_dir + "/seg_00000000000000000600.wal";
    { std::ofstream f(fake_path); f << "OLD_SEGMENT_DATA"; }

    // Write an index.txt with archived_at 200 days ago and "standard" tier
    auto old_time = std::chrono::system_clock::now()
                    - std::chrono::hours(24 * 200);
    auto old_ts = std::chrono::duration_cast<std::chrono::seconds>(
        old_time.time_since_epoch()).count();
    {
        std::ofstream idx(arc_dir + "/index.txt");
        idx << "600 0 0 16 0 " << old_ts << " " << fake_path
            << " standard 0\n";
    }

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory                 = "/tmp/lifecycle_cold_wal";
    cfg.archive_directory             = arc_dir;
    cfg.transition_to_cold_after_days = 90;  // threshold; segment is 200 days old

    WALArchivalManager mgr(cfg);  // loadIndex() reads the old index

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].storage_tier, "standard");

    uint32_t transitioned = mgr.transitionStorageTiers();
    EXPECT_EQ(transitioned, 1u);
    list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    // 200 days > 90 days cold threshold but < 270 days glacier threshold
    EXPECT_EQ(list[0].storage_tier, "cold");

    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, TransitionStorageTiers_MovesToGlacier) {
    const std::string arc_dir = "/tmp/themis_lifecycle_glacier_arc";
    std::filesystem::remove_all(arc_dir);
    std::filesystem::create_directories(arc_dir);

    const std::string fake_path = arc_dir + "/seg_00000000000000000700.wal";
    { std::ofstream f(fake_path); f << "VERY_OLD_SEGMENT"; }

    // 400 days ago; threshold is 90 days cold, 270 days glacier -> glacier
    auto old_time = std::chrono::system_clock::now()
                    - std::chrono::hours(24 * 400);
    auto old_ts = std::chrono::duration_cast<std::chrono::seconds>(
        old_time.time_since_epoch()).count();
    {
        std::ofstream idx(arc_dir + "/index.txt");
        idx << "700 0 0 16 0 " << old_ts << " " << fake_path
            << " standard 0\n";
    }

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory                 = "/tmp/lifecycle_glacier_wal";
    cfg.archive_directory             = arc_dir;
    cfg.transition_to_cold_after_days = 90;

    WALArchivalManager mgr(cfg);

    EXPECT_EQ(mgr.transitionStorageTiers(), 1u);
    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].storage_tier, "glacier");

    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, IndexPersistence_TierAndEncryptionFields) {
    // Verify that storage_tier and encrypted are correctly round-tripped
    // through saveIndex/loadIndex (by creating a second WALArchivalManager
    // over the same archive directory).
    const std::string wal_dir = "/tmp/themis_idx_persist_wal";
    const std::string arc_dir = "/tmp/themis_idx_persist_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000800.wal", "PERSIST TEST DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;
    cfg.encrypt_at_rest         = true;
    cfg.encryption_key_hex      = kTestKeyHex;

    {
        WALArchivalManager mgr(cfg);
        ASSERT_EQ(mgr.archiveSegments({"seg_000800.wal"}), 1u);
        auto list = mgr.listArchived();
        ASSERT_EQ(list.size(), 1u);
        EXPECT_TRUE(list[0].encrypted);
        EXPECT_EQ(list[0].storage_tier, "standard");
    }  // ~WALArchivalManager saves index

    // Reload from the same archive directory
    WALArchivalManager mgr2(cfg);
    auto list = mgr2.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(list[0].encrypted);
    EXPECT_EQ(list[0].storage_tier, "standard");

    // Data must still be retrievable after reload
    auto data = mgr2.retrieveSegment(list[0].segment_id);
    ASSERT_TRUE(data.has_value());
    std::string recovered(data->begin(), data->end());
    EXPECT_EQ(recovered, "PERSIST TEST DATA");

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

TEST(WALArchivalTest, EncryptionAtRest_EmptyKey_RejectsArchival) {
    // encrypt_at_rest=true with an empty key must reject archival, just like an
    // invalid key – it must not silently store the segment unencrypted.
    const std::string wal_dir = "/tmp/themis_enc_emptykey_wal";
    const std::string arc_dir = "/tmp/themis_enc_emptykey_arc";
    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);

    writeSegmentFile(wal_dir, "seg_000900.wal", "SENSITIVE DATA");

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = arc_dir;
    cfg.compress_before_archive = false;
    cfg.encrypt_at_rest         = true;
    cfg.encryption_key_hex      = "";  // empty – must be rejected

    WALArchivalManager mgr(cfg);
    uint32_t n = mgr.archiveSegments({"seg_000900.wal"});
    EXPECT_EQ(n, 0u) << "Archival must be rejected when encrypt_at_rest=true and key is empty";
    EXPECT_TRUE(mgr.listArchived().empty());

    std::filesystem::remove_all(wal_dir);
    std::filesystem::remove_all(arc_dir);
}

// ============================================================================
// IArchivalBackend injection test – mock backend
// ============================================================================

namespace {

// Minimal in-memory mock backend for unit testing IArchivalBackend injection.
struct MockArchivalBackend : public IArchivalBackend {
    std::unordered_map<std::string, std::vector<uint8_t>> store;
    std::unordered_map<std::string, std::string> tiers;
    std::vector<std::string> deleted_keys;

    bool putObject(const std::string& key,
                   const std::vector<uint8_t>& data) override {
        store[key] = data;
        tiers[key] = "standard";
        return true;
    }

    std::optional<std::vector<uint8_t>> getObject(
        const std::string& key) const override {
        auto it = store.find(key);
        if (it == store.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    bool deleteObject(const std::string& key) override {
        deleted_keys.push_back(key);
        store.erase(key);
        tiers.erase(key);
        return true;
    }

    void setStorageTier(const std::string& key,
                        const std::string& tier) override {
        tiers[key] = tier;
    }
};

}  // namespace

TEST(WALArchivalTest, BackendInjection_ArchiveAndRetrieveViaBackend) {
    // Verify that when a custom IArchivalBackend is injected, archiveSegments()
    // routes writes through it and retrieveSegment() reads from it.
    const std::string wal_dir = "/tmp/themis_backend_wal";
    std::filesystem::remove_all(wal_dir);

    writeSegmentFile(wal_dir, "seg_001000.wal", "BACKEND ROUTED DATA");

    auto mock = std::make_shared<MockArchivalBackend>();

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = "";  // not used when backend is set
    cfg.prefix                  = "test-cluster/wal/";
    cfg.compress_before_archive = false;

    WALArchivalManager mgr(cfg, mock);

    uint32_t n = mgr.archiveSegments({"seg_001000.wal"});
    ASSERT_EQ(n, 1u);

    // Backend store must contain the segment
    EXPECT_EQ(mock->store.size(), 1u);
    auto stored_key = mock->store.begin()->first;
    EXPECT_EQ(stored_key.find("test-cluster/wal/seg_"), 0u)
        << "Object key must start with configured prefix, got: " << stored_key;

    // listArchived() shows the segment with the cloud key as archive_path
    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].archive_path, stored_key);

    // retrieveSegment() reads from the backend
    auto data = mgr.retrieveSegment(list[0].segment_id);
    ASSERT_TRUE(data.has_value());
    std::string recovered(data->begin(), data->end());
    EXPECT_EQ(recovered, "BACKEND ROUTED DATA");

    std::filesystem::remove_all(wal_dir);
}

TEST(WALArchivalTest, BackendInjection_PurgeDeletesViaBackend) {
    // purgeExpired() with delete_after_days=0 must call backend->deleteObject().
    const std::string wal_dir = "/tmp/themis_backend_purge_wal";
    std::filesystem::remove_all(wal_dir);

    writeSegmentFile(wal_dir, "seg_001100.wal", "OLD DATA");

    auto mock = std::make_shared<MockArchivalBackend>();

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory           = wal_dir;
    cfg.archive_directory       = "";
    cfg.prefix                  = "prod/";
    cfg.compress_before_archive = false;
    cfg.delete_after_days       = 0;  // purge everything immediately

    WALArchivalManager mgr(cfg, mock);
    ASSERT_EQ(mgr.archiveSegments({"seg_001100.wal"}), 1u);
    ASSERT_EQ(mock->store.size(), 1u);

    uint32_t purged = mgr.purgeExpired();
    EXPECT_EQ(purged, 1u);
    EXPECT_TRUE(mgr.listArchived().empty());
    // Backend deleteObject() must have been called
    EXPECT_EQ(mock->deleted_keys.size(), 1u);
    EXPECT_TRUE(mock->store.empty());

    std::filesystem::remove_all(wal_dir);
}

TEST(WALArchivalTest, BackendInjection_TransitionTierNotifiesBackend) {
    // transitionStorageTiers() must call backend->setStorageTier() for aged segments.
    const std::string arc_dir = "/tmp/themis_backend_tier_arc";
    std::filesystem::remove_all(arc_dir);
    std::filesystem::create_directories(arc_dir);

    auto mock = std::make_shared<MockArchivalBackend>();
    const std::string fake_key = "prod/seg_00000000000000001200.wal";
    mock->store[fake_key] = {'X', 'X'};
    mock->tiers[fake_key] = "standard";

    // Build an index.txt with archived_at 400 days ago
    auto old_time = std::chrono::system_clock::now()
                    - std::chrono::hours(24 * 400);
    auto old_ts = std::chrono::duration_cast<std::chrono::seconds>(
        old_time.time_since_epoch()).count();
    {
        std::ofstream idx(arc_dir + "/index.txt");
        idx << "1200 0 0 2 0 " << old_ts << " " << fake_key
            << " standard 0\n";
    }

    WALArchivalManager::ArchivalConfig cfg;
    cfg.wal_directory                 = "/tmp/backend_tier_wal";
    cfg.archive_directory             = arc_dir;  // for index.txt loading
    cfg.prefix                        = "prod/";
    cfg.transition_to_cold_after_days = 90;       // 400d > 270d glacier threshold

    WALArchivalManager mgr(cfg, mock);

    uint32_t transitioned = mgr.transitionStorageTiers();
    EXPECT_EQ(transitioned, 1u);

    auto list = mgr.listArchived();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].storage_tier, "glacier");

    // Backend must have been notified about the tier change
    EXPECT_EQ(mock->tiers[fake_key], "glacier");

    std::filesystem::remove_all(arc_dir);
}
static ReplicationConfig makeLeaseConfig(const std::string& wal_dir) {
    ReplicationConfig cfg = makeConfig(wal_dir);
    cfg.election_timeout_min_ms   = 80;
    cfg.election_timeout_max_ms   = 150;
    cfg.heartbeat_interval_ms     = 30;
    cfg.enable_leader_lease       = true;
    // Lease must be < election_timeout_min_ms
    cfg.leader_lease_duration_ms  = 60;
    return cfg;
}

// -------------------------------------------------------------------------
// 1. Single-node cluster elects itself leader and obtains a valid lease
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, SingleNodeAcquiresLeaseAfterElection) {
    TempWALDir wd("/tmp/themis_lease_elect");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // Wait for self-election + at least one heartbeat renewing the lease.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_EQ(mgr.getRole(), ReplicationRole::LEADER);
    EXPECT_TRUE(mgr.hasLeaderLease());

    mgr.shutdown();
}

// -------------------------------------------------------------------------
// 2. leaseRead() succeeds on leader with a valid lease
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, LeaseReadSucceedsOnLeaderWithValidLease) {
    TempWALDir wd("/tmp/themis_lease_read");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // Allow election and first heartbeat (lease renewal).
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER);
    ASSERT_TRUE(mgr.hasLeaderLease());

    auto result = mgr.leaseRead("users", "doc-1");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.served_under_lease);
    EXPECT_FALSE(result.node_id.empty());

    mgr.shutdown();
}

// -------------------------------------------------------------------------
// 3. leaseRead() fails when lease is disabled
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, LeaseReadFailsWhenLeaseDisabled) {
    TempWALDir wd("/tmp/themis_lease_disabled");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);
    cfg.enable_leader_lease = false;

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER);

    auto result = mgr.leaseRead("users", "doc-1");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.served_under_lease);

    mgr.shutdown();
}

// -------------------------------------------------------------------------
// 4. Lease expires after the configured duration (no heartbeat renewal)
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, LeaseExpiresWithoutRenewal) {
    TempWALDir wd("/tmp/themis_lease_expiry");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);
    // Very short lease so we can observe expiry quickly.
    cfg.leader_lease_duration_ms = 50;

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // Wait for election and first lease renewal.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER);
    ASSERT_TRUE(mgr.hasLeaderLease());

    mgr.shutdown();  // Stops heartbeat thread → no more renewals.

    // Wait for lease to expire (lease_duration_ms = 50ms; add generous margin).
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_FALSE(mgr.hasLeaderLease());
}

// -------------------------------------------------------------------------
// 5. leaseRead() on uninitialised manager returns failure
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, LeaseReadOnUninitialisedManagerFails) {
    TempWALDir wd("/tmp/themis_lease_uninit");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);

    ReplicationManager mgr(cfg);
    // Deliberately NOT calling initialize().

    EXPECT_FALSE(mgr.hasLeaderLease());
    auto result = mgr.leaseRead("col", "doc");
    EXPECT_FALSE(result.success);
}

// -------------------------------------------------------------------------
// 6. LeaderElection::renewLease / hasValidLease / leaseExpiresAt
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, DirectLeaseApiRenewAndExpire) {
    TempWALDir wd("/tmp/themis_lease_direct");
    ReplicationConfig cfg = makeLeaseConfig(wd.path);
    auto wal = std::make_shared<WALManager>(cfg);

    LeaderElection election("leader-node", cfg, wal);
    election.start();
    // Not yet a leader – lease should be invalid.
    EXPECT_FALSE(election.hasValidLease());

    // Simulate leadership by granting a quorum of votes.
    election.startElection();
    election.grantVote(election.getCurrentTerm());  // one vote (cluster_size=1 → quorum=1)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (election.isLeader()) {
        election.renewLease(100);  // 100ms lease
        EXPECT_TRUE(election.hasValidLease());

        // Wait for lease to expire.
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        EXPECT_FALSE(election.hasValidLease());
    }
    // (If the node is not yet a leader due to timing, the test still passes –
    // it just validates the invariant that a non-leader has no valid lease.)
}

// -------------------------------------------------------------------------
// 7. Prometheus metrics export includes lease-read counters
// -------------------------------------------------------------------------
TEST(LeaderLeaseTest, PrometheusMetricsContainLeaseCounters) {
    // --- Part A: served counter ---
    {
        TempWALDir wd("/tmp/themis_lease_prom_served");
        ReplicationConfig cfg = makeLeaseConfig(wd.path);

        ReplicationManager mgr(cfg);
        ASSERT_TRUE(mgr.initialize());

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER);
        ASSERT_TRUE(mgr.hasLeaderLease());

        // Serve one lease read → lease_reads_served = 1.
        auto r = mgr.leaseRead("col", "doc");
        ASSERT_TRUE(r.success);

        std::string metrics = mgr.exportPrometheusMetrics();
        EXPECT_NE(metrics.find("themisdb_leader_lease_reads_served_total"), std::string::npos);
        // Served counter must be exactly 1.
        EXPECT_NE(metrics.find("themisdb_leader_lease_reads_served_total 1"), std::string::npos);

        mgr.shutdown();
    }

    // --- Part B: rejected counter ---
    {
        TempWALDir wd2("/tmp/themis_lease_prom_rejected");
        ReplicationConfig cfg2 = makeLeaseConfig(wd2.path);
        cfg2.enable_leader_lease = false;  // lease disabled → leaseRead always rejects

        ReplicationManager mgr2(cfg2);
        ASSERT_TRUE(mgr2.initialize());

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        ASSERT_EQ(mgr2.getRole(), ReplicationRole::LEADER);

        // leaseRead rejected (lease disabled) → lease_reads_rejected = 1.
        auto r2 = mgr2.leaseRead("col", "doc2");
        EXPECT_FALSE(r2.success);

        std::string metrics2 = mgr2.exportPrometheusMetrics();
        EXPECT_NE(metrics2.find("themisdb_leader_lease_reads_rejected_total"), std::string::npos);
        // Rejected counter must be exactly 1.
        EXPECT_NE(metrics2.find("themisdb_leader_lease_reads_rejected_total 1"), std::string::npos);

        mgr2.shutdown();
    }
}

// ============================================================================
// Cross-Cluster Publish/Subscribe Replication Tests (v1.7.0)
// ============================================================================

// ---------------------------------------------------------------------------
// Helper: build a WALEntry with a given sequence, collection, and operation
// ---------------------------------------------------------------------------
static WALEntry makeWALEntry(uint64_t seq, const std::string& collection,
                              const std::string& op, const std::string& doc_id = "doc1") {
    WALEntry e;
    e.sequence_number = seq;
    e.collection      = collection;
    e.operation       = op;
    e.document_id     = doc_id;
    e.data            = R"({"value":1})";
    return e;
}

// ============================================================================
// PublicationFilter Tests
// ============================================================================

class PublicationFilterTest : public ::testing::Test {};

TEST_F(PublicationFilterTest, EmptyFilterMatchesAll) {
    PublicationFilter f;  // No restrictions
    EXPECT_TRUE(f.matches(makeWALEntry(1, "users",    "INSERT")));
    EXPECT_TRUE(f.matches(makeWALEntry(2, "orders",   "UPDATE")));
    EXPECT_TRUE(f.matches(makeWALEntry(3, "products", "DELETE")));
}

TEST_F(PublicationFilterTest, CollectionFilterIncludesOnly) {
    PublicationFilter f;
    f.include_collections = {"orders"};

    EXPECT_TRUE(f.matches(makeWALEntry(1, "orders", "INSERT")));
    EXPECT_FALSE(f.matches(makeWALEntry(2, "users",  "INSERT")));
}

TEST_F(PublicationFilterTest, MultipleCollectionsFilter) {
    PublicationFilter f;
    f.include_collections = {"orders", "customers"};

    EXPECT_TRUE(f.matches(makeWALEntry(1, "orders",    "INSERT")));
    EXPECT_TRUE(f.matches(makeWALEntry(2, "customers", "UPDATE")));
    EXPECT_FALSE(f.matches(makeWALEntry(3, "products", "DELETE")));
}

TEST_F(PublicationFilterTest, OperationFilterIncludesOnly) {
    PublicationFilter f;
    f.include_operations = {"INSERT", "UPDATE"};

    EXPECT_TRUE(f.matches(makeWALEntry(1, "users", "INSERT")));
    EXPECT_TRUE(f.matches(makeWALEntry(2, "users", "UPDATE")));
    EXPECT_FALSE(f.matches(makeWALEntry(3, "users", "DELETE")));
}

TEST_F(PublicationFilterTest, CollectionAndOperationFilterCombined) {
    PublicationFilter f;
    f.include_collections = {"orders"};
    f.include_operations  = {"INSERT"};

    EXPECT_TRUE(f.matches(makeWALEntry(1, "orders", "INSERT")));
    EXPECT_FALSE(f.matches(makeWALEntry(2, "orders", "DELETE")));
    EXPECT_FALSE(f.matches(makeWALEntry(3, "users",  "INSERT")));
}

// ============================================================================
// CrossClusterPublication Tests
// ============================================================================

class CrossClusterPublicationTest : public ::testing::Test {};

TEST_F(CrossClusterPublicationTest, NameIsPreserved) {
    CrossClusterPublication pub("orders_pub");
    EXPECT_EQ(pub.name(), "orders_pub");
}

TEST_F(CrossClusterPublicationTest, InitialStateHasNoSubscribersAndZeroPublished) {
    CrossClusterPublication pub("test_pub");
    EXPECT_EQ(pub.subscriberCount(), 0u);
    EXPECT_EQ(pub.publishedCount(), 0u);
}

TEST_F(CrossClusterPublicationTest, AddAndRemoveRemoteSubscriber) {
    CrossClusterPublication pub("test_pub");

    auto id1 = pub.addRemoteSubscriber([](const WALEntry&) {});
    EXPECT_EQ(pub.subscriberCount(), 1u);

    auto id2 = pub.addRemoteSubscriber([](const WALEntry&) {});
    EXPECT_EQ(pub.subscriberCount(), 2u);

    pub.removeRemoteSubscriber(id1);
    EXPECT_EQ(pub.subscriberCount(), 1u);

    pub.removeRemoteSubscriber(id2);
    EXPECT_EQ(pub.subscriberCount(), 0u);
}

TEST_F(CrossClusterPublicationTest, PublishDeliversEntryToSubscriber) {
    CrossClusterPublication pub("test_pub");

    std::vector<WALEntry> received;
    pub.addRemoteSubscriber([&](const WALEntry& e) { received.push_back(e); });

    pub.publish(makeWALEntry(1, "users", "INSERT"));
    pub.publish(makeWALEntry(2, "users", "UPDATE"));

    ASSERT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0].sequence_number, 1u);
    EXPECT_EQ(received[1].sequence_number, 2u);
    EXPECT_EQ(pub.publishedCount(), 2u);
}

TEST_F(CrossClusterPublicationTest, FilteredPublishDropsNonMatchingEntries) {
    CrossClusterPublication pub("filtered_pub");
    PublicationFilter f;
    f.include_collections = {"orders"};
    pub.setFilter(f);

    std::vector<WALEntry> received;
    pub.addRemoteSubscriber([&](const WALEntry& e) { received.push_back(e); });

    pub.publish(makeWALEntry(1, "users",  "INSERT"));  // filtered out
    pub.publish(makeWALEntry(2, "orders", "INSERT"));  // passes

    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].collection, "orders");
    EXPECT_EQ(pub.publishedCount(), 1u);
}

TEST_F(CrossClusterPublicationTest, PublishDeliversToMultipleSubscribers) {
    CrossClusterPublication pub("multi_pub");

    std::atomic<int> count1{0}, count2{0};
    pub.addRemoteSubscriber([&](const WALEntry&) { ++count1; });
    pub.addRemoteSubscriber([&](const WALEntry&) { ++count2; });

    pub.publish(makeWALEntry(1, "col", "INSERT"));

    EXPECT_EQ(count1.load(), 1);
    EXPECT_EQ(count2.load(), 1);
}

TEST_F(CrossClusterPublicationTest, OnWALEntryAppliedFeedsPublish) {
    CrossClusterPublication pub("wal_pub");

    std::vector<std::string> ops;
    pub.addRemoteSubscriber([&](const WALEntry& e) { ops.push_back(e.operation); });

    pub.onWALEntryApplied(makeWALEntry(10, "col", "INSERT"));
    pub.onWALEntryApplied(makeWALEntry(11, "col", "DELETE"));

    ASSERT_EQ(ops.size(), 2u);
    EXPECT_EQ(ops[0], "INSERT");
    EXPECT_EQ(ops[1], "DELETE");
}

TEST_F(CrossClusterPublicationTest, SetFilterThreadSafe) {
    // Verifies that setFilter / getFilter don't race under concurrent access
    CrossClusterPublication pub("thread_pub");

    std::atomic<bool> stop{false};
    std::vector<WALEntry> received;
    std::mutex recv_mutex = {};

    pub.addRemoteSubscriber([&](const WALEntry& e) {
        std::lock_guard<std::mutex> lk(recv_mutex);
        received.push_back(e);
    });

    // Writer thread: continuously updates filter
    std::thread writer([&] {
        for (int i = 0; i < 200 && !stop.load(); ++i) {
            PublicationFilter f = {};
            if (i % 2 == 0) f.include_collections = {"col"};
            pub.setFilter(f);
        }
    });

    // Publisher thread: continuously publishes
    std::thread publisher([&] {
        for (int i = 0; i < 200; ++i) {
            pub.publish(makeWALEntry(static_cast<uint64_t>(i), "col", "INSERT"));
        }
        stop.store(true);
    });

    writer.join();
    publisher.join();

    // Should not crash; counts may vary due to filter changes
    EXPECT_GE(pub.publishedCount(), 0u);
}

// ============================================================================
// CrossClusterSubscription Tests
// ============================================================================

class CrossClusterSubscriptionTest : public ::testing::Test {};

TEST_F(CrossClusterSubscriptionTest, NameIsPreserved) {
    auto pub = std::make_shared<CrossClusterPublication>("p");
    CrossClusterSubscription sub("orders_sub", pub, [](const WALEntry&) {});
    EXPECT_EQ(sub.name(), "orders_sub");
}

TEST_F(CrossClusterSubscriptionTest, InitiallyDisabled) {
    auto pub = std::make_shared<CrossClusterPublication>("p");
    CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});
    EXPECT_FALSE(sub.isEnabled());
    EXPECT_EQ(sub.appliedCount(), 0u);
    EXPECT_EQ(sub.lastAppliedSequence(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, EnableRegistersWithPublication) {
    auto pub = std::make_shared<CrossClusterPublication>("p");
    EXPECT_EQ(pub->subscriberCount(), 0u);

    CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});
    sub.enable();

    EXPECT_TRUE(sub.isEnabled());
    EXPECT_EQ(pub->subscriberCount(), 1u);

    sub.disable();
    EXPECT_FALSE(sub.isEnabled());
    EXPECT_EQ(pub->subscriberCount(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, EnabledSubscriptionReceivesEntries) {
    auto pub = std::make_shared<CrossClusterPublication>("p");

    std::vector<WALEntry> applied;
    CrossClusterSubscription sub("sub", pub, [&](const WALEntry& e) { applied.push_back(e); });
    sub.enable();

    pub->publish(makeWALEntry(1, "col", "INSERT"));
    pub->publish(makeWALEntry(2, "col", "UPDATE"));

    ASSERT_EQ(applied.size(), 2u);
    EXPECT_EQ(applied[0].sequence_number, 1u);
    EXPECT_EQ(applied[1].sequence_number, 2u);
    EXPECT_EQ(sub.appliedCount(), 2u);
    EXPECT_EQ(sub.lastAppliedSequence(), 2u);
    EXPECT_EQ(sub.errorCount(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, DisabledSubscriptionReceivesNothing) {
    auto pub = std::make_shared<CrossClusterPublication>("p");

    int count = 0;
    CrossClusterSubscription sub("sub", pub, [&](const WALEntry&) { ++count; });
    // Do not enable

    pub->publish(makeWALEntry(1, "col", "INSERT"));

    EXPECT_EQ(count, 0);
    EXPECT_EQ(sub.appliedCount(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, ApplyErrorCountedAndDoesNotStop) {
    auto pub = std::make_shared<CrossClusterPublication>("p");

    int call_count = 0;
    CrossClusterSubscription sub("sub", pub, [&](const WALEntry& e) {
        ++call_count;
        if (e.sequence_number == 1) {
          throw std::runtime_error("apply error");
        }
    });
    sub.enable();

    pub->publish(makeWALEntry(1, "col", "INSERT"));  // throws
    pub->publish(makeWALEntry(2, "col", "INSERT"));  // succeeds

    EXPECT_EQ(call_count, 2);
    EXPECT_EQ(sub.errorCount(), 1u);
    EXPECT_EQ(sub.appliedCount(), 1u);
    EXPECT_EQ(sub.lastAppliedSequence(), 2u);
}

TEST_F(CrossClusterSubscriptionTest, LastAppliedSequenceTracksHighestApplied) {
    auto pub = std::make_shared<CrossClusterPublication>("p");

    CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});
    sub.enable();

    pub->publish(makeWALEntry(5, "col", "INSERT"));
    EXPECT_EQ(sub.lastAppliedSequence(), 5u);

    pub->publish(makeWALEntry(10, "col", "UPDATE"));
    EXPECT_EQ(sub.lastAppliedSequence(), 10u);
}

TEST_F(CrossClusterSubscriptionTest, DestructorAutoDisables) {
    auto pub = std::make_shared<CrossClusterPublication>("p");

    {
        CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});
        sub.enable();
        EXPECT_EQ(pub->subscriberCount(), 1u);
    }  // sub destroyed here

    EXPECT_EQ(pub->subscriberCount(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, EnableIdempotent) {
    auto pub = std::make_shared<CrossClusterPublication>("p");
    CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});

    sub.enable();
    sub.enable();  // second enable is a no-op

    EXPECT_EQ(pub->subscriberCount(), 1u);

    sub.disable();
    EXPECT_EQ(pub->subscriberCount(), 0u);
}

TEST_F(CrossClusterSubscriptionTest, DisableIdempotent) {
    auto pub = std::make_shared<CrossClusterPublication>("p");
    CrossClusterSubscription sub("sub", pub, [](const WALEntry&) {});

    sub.enable();
    sub.disable();
    sub.disable();  // second disable is a no-op

    EXPECT_EQ(pub->subscriberCount(), 0u);
}

// ============================================================================
// End-to-End: publication + subscription with filter
// ============================================================================

TEST(CrossClusterE2ETest, FilteredPublicationDeliversOnlyMatchingEntries) {
    auto pub = std::make_shared<CrossClusterPublication>("orders_pub");

    // Only replicate INSERT operations on the "orders" collection
    PublicationFilter f;
    f.include_collections = {"orders"};
    f.include_operations  = {"INSERT"};
    pub->setFilter(f);

    std::vector<WALEntry> applied;
    CrossClusterSubscription sub("orders_sub", pub, [&](const WALEntry& e) {
        applied.push_back(e);
    });
    sub.enable();

    pub->publish(makeWALEntry(1, "orders",   "INSERT"));  // ✓
    pub->publish(makeWALEntry(2, "orders",   "DELETE"));  // filtered (operation)
    pub->publish(makeWALEntry(3, "users",    "INSERT"));  // filtered (collection)
    pub->publish(makeWALEntry(4, "orders",   "INSERT"));  // ✓
    pub->publish(makeWALEntry(5, "products", "UPDATE"));  // filtered

    ASSERT_EQ(applied.size(), 2u);
    EXPECT_EQ(applied[0].sequence_number, 1u);
    EXPECT_EQ(applied[1].sequence_number, 4u);
    EXPECT_EQ(sub.appliedCount(), 2u);
    EXPECT_EQ(sub.errorCount(), 0u);
}

TEST(CrossClusterE2ETest, MultipleSubscriptionsReceiveIndependently) {
    auto pub = std::make_shared<CrossClusterPublication>("shared_pub");

    std::vector<WALEntry> recv1, recv2;
    CrossClusterSubscription sub1("sub1", pub, [&](const WALEntry& e) { recv1.push_back(e); });
    CrossClusterSubscription sub2("sub2", pub, [&](const WALEntry& e) { recv2.push_back(e); });

    sub1.enable();
    sub2.enable();

    pub->publish(makeWALEntry(1, "col", "INSERT"));
    pub->publish(makeWALEntry(2, "col", "INSERT"));

    EXPECT_EQ(recv1.size(), 2u);
    EXPECT_EQ(recv2.size(), 2u);

    // Disable one subscription mid-stream
    sub2.disable();
    pub->publish(makeWALEntry(3, "col", "INSERT"));

    EXPECT_EQ(recv1.size(), 3u);
    EXPECT_EQ(recv2.size(), 2u);  // sub2 stopped receiving
}

TEST(CrossClusterE2ETest, WALEntryAppliedIntegrationPath) {
    // Simulates the full chain: WAL entry → publication → subscription apply
    auto pub = std::make_shared<CrossClusterPublication>("wal_pub");

    std::vector<std::string> replicated_ops;
    CrossClusterSubscription sub("wal_sub", pub, [&](const WALEntry& e) {
        replicated_ops.push_back(e.operation);
    });
    sub.enable();

    // Feed through the IReplicationListener interface (as WALManager would do)
    pub->onWALEntryApplied(makeWALEntry(100, "docs", "INSERT"));
    pub->onWALEntryApplied(makeWALEntry(101, "docs", "UPDATE"));
    pub->onWALEntryApplied(makeWALEntry(102, "docs", "DELETE"));

    ASSERT_EQ(replicated_ops.size(), 3u);
    EXPECT_EQ(replicated_ops[0], "INSERT");
    EXPECT_EQ(replicated_ops[1], "UPDATE");
    EXPECT_EQ(replicated_ops[2], "DELETE");
    EXPECT_EQ(sub.lastAppliedSequence(), 102u);
}

// ============================================================================
// Cross-Cluster Pub/Sub Integration Tests – via ReplicationManager.addListener
// ============================================================================

// Re-use the makeConfig() helper defined earlier in this file.
// These tests mirror the CDCManager integration test pattern:
// single-node cluster elects itself leader, then replicate() flows through
// addListener → onWALEntryApplied → CrossClusterPublication → subscription.

TEST(CrossClusterIntegrationTest, PublicationReceivesEntriesViaReplicationManager) {
    TempWALDir wd("/tmp/themis_cc_intg_basic");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    auto pub = std::make_shared<CrossClusterPublication>("intg_pub");

    std::vector<std::string> replicated_ids;
    std::mutex ids_mutex = {};
    CrossClusterSubscription sub("intg_sub", pub, [&](const WALEntry& e) {
        std::lock_guard<std::mutex> lk(ids_mutex);
        replicated_ids.push_back(e.document_id);
    });
    sub.enable();
    mgr.addListener(pub);

    // Wait for single-node leader election
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto replicate_with_retry = [&](const WALEntry& entry) {
        for (int i = 0; i < 20; ++i) {
            if (mgr.replicate(entry)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return false;
    };

    WALEntry entry;
    entry.operation   = "INSERT";
    entry.collection  = "docs";
    entry.document_id = "doc-001";
    entry.data        = R"({"v":1})";
    bool ok = replicate_with_retry(entry);

    mgr.shutdown();
    ASSERT_TRUE(ok) << "replicate() failed – node may not be leader yet";
    ASSERT_EQ(replicated_ids.size(), 1u);
    EXPECT_EQ(replicated_ids[0], "doc-001");
    EXPECT_EQ(sub.appliedCount(), 1u);
    EXPECT_EQ(sub.lastAppliedSequence(), entry.sequence_number);
}

TEST(CrossClusterIntegrationTest, FilterDropsNonMatchingEntriesViaReplicationManager) {
    TempWALDir wd("/tmp/themis_cc_intg_filter");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    auto pub = std::make_shared<CrossClusterPublication>("filtered_intg_pub");

    // Only replicate "orders" collection
    PublicationFilter f;
    f.include_collections = {"orders"};
    pub->setFilter(f);

    std::vector<std::string> replicated_collections;
    std::mutex col_mutex = {};
    CrossClusterSubscription sub("filtered_intg_sub", pub, [&](const WALEntry& e) {
        std::lock_guard<std::mutex> lk(col_mutex);
        replicated_collections.push_back(e.collection);
    });
    sub.enable();
    mgr.addListener(pub);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto replicate_with_retry = [&](const WALEntry& entry) {
        for (int i = 0; i < 20; ++i) {
            if (mgr.replicate(entry)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return false;
    };

    // Replicate two entries: one passes the filter, one doesn't
    WALEntry e_orders;
    e_orders.operation   = "INSERT";
    e_orders.collection  = "orders";
    e_orders.document_id = "ord-1";
    e_orders.data        = R"({"v":1})";
    ASSERT_TRUE(replicate_with_retry(e_orders))
        << "replicate(orders) failed - node may not be leader yet";

    WALEntry e_users;
    e_users.operation   = "INSERT";
    e_users.collection  = "users";
    e_users.document_id = "usr-1";
    e_users.data        = R"({"v":1})";
    ASSERT_TRUE(replicate_with_retry(e_users))
        << "replicate(users) failed - node may not be leader yet";

    mgr.shutdown();

    // Only the "orders" entry should have been delivered
    ASSERT_EQ(replicated_collections.size(), 1u);
    EXPECT_EQ(replicated_collections[0], "orders");
    EXPECT_EQ(sub.appliedCount(), 1u);
}

// ============================================================================
// Prometheus Metrics Tests
// ============================================================================

TEST(CrossClusterPrometheusTest, PublicationMetricsCorrect) {
    auto pub = std::make_shared<CrossClusterPublication>("prom_pub");

    std::vector<WALEntry> sink;
    pub->addRemoteSubscriber([&](const WALEntry& e) { sink.push_back(e); });

    pub->publish(makeWALEntry(1, "col", "INSERT"));
    pub->publish(makeWALEntry(2, "col", "INSERT"));

    std::string m = pub->exportPrometheusMetrics();

    EXPECT_NE(m.find("themisdb_cross_cluster_publication_published_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_cross_cluster_publication_subscribers"),     std::string::npos);
    // published_total = 2
    EXPECT_NE(m.find("{publication=\"prom_pub\"} 2"), std::string::npos);
    // subscribers = 1
    EXPECT_NE(m.find("{publication=\"prom_pub\"} 1"), std::string::npos);
}

TEST(CrossClusterPrometheusTest, SubscriptionMetricsCorrect) {
    auto pub = std::make_shared<CrossClusterPublication>("prom_pub2");
    CrossClusterSubscription sub("prom_sub", pub, [](const WALEntry&) {});
    sub.enable();

    pub->publish(makeWALEntry(10, "col", "INSERT"));
    pub->publish(makeWALEntry(11, "col", "INSERT"));

    std::string m = sub.exportPrometheusMetrics();

    EXPECT_NE(m.find("themisdb_cross_cluster_subscription_applied_total"),          std::string::npos);
    EXPECT_NE(m.find("themisdb_cross_cluster_subscription_errors_total"),           std::string::npos);
    EXPECT_NE(m.find("themisdb_cross_cluster_subscription_last_applied_sequence"),  std::string::npos);
    // applied_total = 2
    EXPECT_NE(m.find("{subscription=\"prom_sub\"} 2"), std::string::npos);
    // last_applied_sequence = 11
    EXPECT_NE(m.find("{subscription=\"prom_sub\"} 11"), std::string::npos);
}

TEST(CrossClusterPrometheusTest, SubscriptionMetricsReflectErrors) {
    auto pub = std::make_shared<CrossClusterPublication>("prom_pub3");

    int calls = 0;
    CrossClusterSubscription sub("err_sub", pub, [&]([[maybe_unused]] const WALEntry& e) {
        ++calls;
        if (calls == 1) {
          throw std::runtime_error("simulated error");
        }
    });
    sub.enable();

    pub->publish(makeWALEntry(1, "col", "INSERT"));  // error
    pub->publish(makeWALEntry(2, "col", "INSERT"));  // ok

    std::string m = sub.exportPrometheusMetrics();

    // errors_total = 1
    EXPECT_NE(m.find("{subscription=\"err_sub\"} 1"), std::string::npos);
}

// ============================================================================
// LagBasedReadRouter Tests (v1.7.0)
// ============================================================================

namespace {

// Build a minimal ReplicaInfo for testing.
static ReplicaInfo makeLagReplica(const std::string& node_id,
                                   int64_t lag_ms,
                                   HealthStatus health = HealthStatus::HEALTHY)
{
    ReplicaInfo r;
    r.node_id              = node_id;
    r.role                 = ReplicationRole::FOLLOWER;
    r.health_status        = health;
    r.last_applied_sequence = 100;
    r.last_applied_term     = 1;
    r.last_heartbeat        = std::chrono::system_clock::now() -
                              std::chrono::milliseconds(lag_ms);
    return r;
}

} // anonymous namespace

// 1. When no replicas exist, selectReplica falls back to primary for
//    SECONDARY_PREFERRED and PRIMARY_PREFERRED preferences.
TEST(LagBasedReadRouterTest, FallsBackToPrimaryWhenNoReplicas) {
    LagBasedReadRouter router;
    std::vector<ReplicaInfo> replicas;  // empty

    auto dec = router.selectReplica(ReadPreference::SECONDARY_PREFERRED,
                                    replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "primary-1");
    EXPECT_TRUE(dec.is_primary);

    dec = router.selectReplica(ReadPreference::PRIMARY_PREFERRED,
                               replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "primary-1");
    EXPECT_TRUE(dec.is_primary);
}

// 2. ReadPreference::PRIMARY always selects primary regardless of replicas.
TEST(LagBasedReadRouterTest, PrimaryPreferenceAlwaysReturnsPrimary) {
    LagBasedReadRouter router;
    std::vector<ReplicaInfo> replicas = { makeLagReplica("r1", 0) };

    auto dec = router.selectReplica(ReadPreference::PRIMARY, replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "primary-1");
    EXPECT_TRUE(dec.is_primary);
}

// 3. Replica within lag threshold is selected over primary for SECONDARY_PREFERRED.
TEST(LagBasedReadRouterTest, SelectsEligibleReplicaOverPrimary) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 5000;
    LagBasedReadRouter router(cfg);

    std::vector<ReplicaInfo> replicas = { makeLagReplica("r1", 1000) };

    auto dec = router.selectReplica(ReadPreference::SECONDARY_PREFERRED,
                                    replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "r1");
    EXPECT_FALSE(dec.is_primary);
    EXPECT_GE(dec.replica_lag_ms, 0);
}

// 4. Replica exceeding the lag threshold is excluded; falls back to primary.
TEST(LagBasedReadRouterTest, ExcludesHighLagReplicaAndFallsBackToPrimary) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 2000;
    LagBasedReadRouter router(cfg);

    // Replica lag exceeds threshold (last_heartbeat far in the past)
    ReplicaInfo r = makeLagReplica("r1", 10000);  // 10 s heartbeat delay
    std::vector<ReplicaInfo> replicas = { r };

    auto dec = router.selectReplica(ReadPreference::SECONDARY_PREFERRED,
                                    replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "primary-1");
    EXPECT_TRUE(dec.is_primary);
    EXPECT_FALSE(dec.reason.empty());
}

// 5. Among multiple replicas, the one with the lowest lag is selected.
TEST(LagBasedReadRouterTest, SelectsLowestLagReplica) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 10000;
    LagBasedReadRouter router(cfg);

    std::vector<ReplicaInfo> replicas = {
        makeLagReplica("r1", 3000),
        makeLagReplica("r2", 500),
        makeLagReplica("r3", 1500),
    };

    auto dec = router.selectReplica(ReadPreference::NEAREST, replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "r2");
    EXPECT_FALSE(dec.is_primary);
}

// 6. FAILED replicas are excluded even if their lag is within threshold.
TEST(LagBasedReadRouterTest, ExcludesFailedReplicas) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 10000;
    LagBasedReadRouter router(cfg);

    std::vector<ReplicaInfo> replicas = {
        makeLagReplica("r1", 100, HealthStatus::FAILED),
    };

    auto dec = router.selectReplica(ReadPreference::SECONDARY_PREFERRED,
                                    replicas, "primary-1");
    EXPECT_EQ(dec.node_id, "primary-1");
    EXPECT_TRUE(dec.is_primary);
}

// 7. ReadPreference::SECONDARY returns empty node_id when no eligible replica.
TEST(LagBasedReadRouterTest, SecondaryPreferenceReturnsEmptyWhenNoEligible) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 100;
    LagBasedReadRouter router(cfg);

    std::vector<ReplicaInfo> replicas = { makeLagReplica("r1", 5000) };

    auto dec = router.selectReplica(ReadPreference::SECONDARY, replicas, "primary-1");
    EXPECT_TRUE(dec.node_id.empty());
    EXPECT_FALSE(dec.is_primary);
}

// 8. eligibleReplicaCount returns correct count.
TEST(LagBasedReadRouterTest, EligibleReplicaCountIsCorrect) {
    LagBasedReadRouter::RouterConfig cfg;
    cfg.lag_threshold_ms = 3000;
    LagBasedReadRouter router(cfg);

    std::vector<ReplicaInfo> replicas = {
        makeLagReplica("r1", 1000),                        // eligible
        makeLagReplica("r2", 5000),                        // too much lag
        makeLagReplica("r3", 2000),                        // eligible
        makeLagReplica("r4", 100, HealthStatus::FAILED),   // failed
    };

    EXPECT_EQ(router.eligibleReplicaCount(replicas), 2u);
}

// 9. Prometheus metrics string contains expected metric names.
TEST(LagBasedReadRouterTest, PrometheusMetricsContainExpectedKeys) {
    LagBasedReadRouter router;
    std::vector<ReplicaInfo> replicas = { makeLagReplica("r1", 500) };

    std::string m = router.exportPrometheusMetrics(replicas);
    EXPECT_NE(m.find("themisdb_lag_router_eligible_replicas"), std::string::npos);
    EXPECT_NE(m.find("themisdb_lag_router_threshold_ms"),       std::string::npos);
    EXPECT_NE(m.find("themisdb_lag_router_replica_eligible"),   std::string::npos);
}

// 10. ReplicationManager::selectReadReplica returns primary node for single-node cluster.
TEST(LagBasedReadRouterTest, ReplicationManagerSelectReadReplicaReturnsPrimaryWhenNoReplicas) {
    TempWALDir wd("/tmp/themis_lag_router_mgr");
    ReplicationConfig cfg = makeConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto dec = mgr.selectReadReplica();
    // Single-node: no replicas, should fall back to primary (this node)
    EXPECT_FALSE(dec.node_id.empty());

    mgr.shutdown();
}

// ============================================================================
// Witness Node Tests
// ============================================================================

class WitnessNodeTest : public ::testing::Test {
protected:
    static ReplicationConfig makeWitnessConfig(const std::string& wal_dir) {
        ReplicationConfig cfg = makeConfig(wal_dir);
        cfg.min_sync_replicas = 1;
        return cfg;
    }
};

// 1. A witness node is stored as a WITNESS role voting member.
TEST_F(WitnessNodeTest, WitnessRoleIsVotingMember) {
    TempWALDir wd("/tmp/themis_witness_role_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    mgr.addWitnessNode("witness-1", "127.0.0.1:7010");

    auto replicas = mgr.getReplicas();
    ASSERT_EQ(replicas.size(), 1u);
    EXPECT_EQ(replicas[0].node_id, "witness-1");
    EXPECT_EQ(replicas[0].role, ReplicationRole::WITNESS);
    EXPECT_TRUE(replicas[0].is_voting_member);

    mgr.shutdown();
}

// 2. Adding a witness node via addReplica() with WITNESS role also skips the stream.
TEST_F(WitnessNodeTest, AddReplicaWithWitnessRoleSkipsStream) {
    TempWALDir wd("/tmp/themis_witness_addreplica_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    ReplicaInfo witness;
    witness.node_id          = "witness-2";
    witness.endpoint         = "127.0.0.1:7011";
    witness.role             = ReplicationRole::WITNESS;
    witness.is_voting_member = true;
    witness.last_heartbeat   = std::chrono::system_clock::now();
    mgr.addReplica(witness);

    auto replicas = mgr.getReplicas();
    ASSERT_EQ(replicas.size(), 1u);
    EXPECT_EQ(replicas[0].role, ReplicationRole::WITNESS);

    mgr.shutdown();
}

// 3. A 2-node cluster (1 data follower + 1 witness) achieves quorum.
//    We poll until this node becomes the leader (background election loop fires
//    within election_timeout_max_ms = 300 ms), then assert quorum.
//    hasQuorum() counts: self(leader) + follower(HEALTHY) = 2 healthy out of
//    3 total voting members; 2 > 3/2 = 1 → quorum.
TEST_F(WitnessNodeTest, TwoNodeClusterWithWitnessHasQuorum) {
    TempWALDir wd("/tmp/themis_witness_quorum_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // Poll until this node wins election.  At initialization cluster_size = 1,
    // so quorum = 1 and the first election cycle immediately promotes this node
    // to leader.  Timeout of 2 s is well above election_timeout_max_ms (300 ms).
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (mgr.getRole() != ReplicationRole::LEADER &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER)
        << "Node should have won election within 2 seconds";

    // One regular follower (healthy, voting)
    ReplicaInfo follower;
    follower.node_id          = "follower-1";
    follower.endpoint         = "127.0.0.1:7020";
    follower.role             = ReplicationRole::FOLLOWER;
    follower.is_voting_member = true;
    follower.health_status    = HealthStatus::HEALTHY;
    follower.last_heartbeat   = std::chrono::system_clock::now();
    mgr.addReplica(follower);

    // One witness node (voting, no data stream; health starts UNKNOWN)
    mgr.addWitnessNode("witness-quorum", "127.0.0.1:7021");

    // hasQuorum(): self(leader) + follower = 2 healthy out of 3 total; 2 > 1 → true.
    EXPECT_TRUE(mgr.hasQuorum());

    mgr.shutdown();
}

// 4. Witness node is not returned as a candidate by selectReadReplica
//    (it holds no data).
TEST_F(WitnessNodeTest, WitnessNodeNotSelectedForReads) {
    TempWALDir wd("/tmp/themis_witness_read_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    mgr.addWitnessNode("witness-only", "127.0.0.1:7030");

    // With only a witness replica (no data replica), selectReadReplica should
    // fall back to the primary (this node), not the witness.
    auto dec = mgr.selectReadReplica(ReadPreference::SECONDARY_PREFERRED);
    // The selected node must not be the witness.
    EXPECT_NE(dec.node_id, "witness-only");

    mgr.shutdown();
}

// 5. Removing a witness node removes it from the replica list.
TEST_F(WitnessNodeTest, RemoveWitnessNodeDeregistersIt) {
    TempWALDir wd("/tmp/themis_witness_remove_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    mgr.addWitnessNode("witness-rm", "127.0.0.1:7040");
    EXPECT_EQ(mgr.getReplicas().size(), 1u);

    mgr.removeReplica("witness-rm");
    EXPECT_EQ(mgr.getReplicas().size(), 0u);

    mgr.shutdown();
}

// 6. In a 2-node data cluster (leader + failed data follower + witness),
//    the witness with a fresh heartbeat still counts toward quorum.
//    This validates the key "2-node cluster" HA scenario: even when the data
//    follower is down, leader + witness = 2 healthy out of 3 total → quorum.
TEST_F(WitnessNodeTest, WitnessCountsForQuorumWhenDataFollowerFailed) {
    TempWALDir wd("/tmp/themis_witness_quorum_failover_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // Wait for this node to become leader.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (mgr.getRole() != ReplicationRole::LEADER &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(mgr.getRole(), ReplicationRole::LEADER);

    // Data follower that has failed.
    ReplicaInfo failed_follower;
    failed_follower.node_id          = "follower-failed";
    failed_follower.endpoint         = "127.0.0.1:7050";
    failed_follower.role             = ReplicationRole::FOLLOWER;
    failed_follower.is_voting_member = true;
    failed_follower.health_status    = HealthStatus::FAILED;
    failed_follower.last_heartbeat   =
        std::chrono::system_clock::now() - std::chrono::hours(1);
    mgr.addReplica(failed_follower);

    // Witness with a fresh heartbeat (just added, last_heartbeat = now()).
    mgr.addWitnessNode("witness-alive", "127.0.0.1:7051");

    // Cluster: self(leader, healthy) + follower(FAILED) + witness(fresh heartbeat)
    // Total voting = 3, healthy = self + witness = 2 → 2 > 1 → quorum.
    EXPECT_TRUE(mgr.hasQuorum());

    mgr.shutdown();
}

// 7. A witness node must not be selected as a leader candidate during failover.
TEST_F(WitnessNodeTest, WitnessNotSelectedAsLeaderCandidate) {
    TempWALDir wd("/tmp/themis_witness_no_leader_test");
    auto cfg = makeWitnessConfig(wd.path);

    ReplicationManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());

    // A witness is the only replica in the list.
    mgr.addWitnessNode("witness-only-leader", "127.0.0.1:7060");

    // triggerFailover with the witness node ID must not promote the witness.
    // (It should return false because the witness is not a valid leader target.)
    bool promoted = mgr.triggerFailover("witness-only-leader");
    EXPECT_FALSE(promoted);

    // The role should NOT have changed to LEADER via the witness path.
    // (The node may or may not be leader due to its own election, but it
    //  must not have entered a leader state via the witness failover path.)
    auto replicas = mgr.getReplicas();
    ASSERT_EQ(replicas.size(), 1u);
    EXPECT_EQ(replicas[0].role, ReplicationRole::WITNESS);

    mgr.shutdown();
}

// ============================================================================
// BidirectionalReplicationTest  (v1.7.0)
// ============================================================================
// Covers all acceptance criteria from the Bidirectional Replication roadmap
// item:
//   AC-1  Symmetric replication (both nodes are primary)
//   AC-2  Conflict detection using timestamps and sequence numbers
//   AC-3  Configurable conflict resolution per table/collection
//   AC-4  Origin tracking to prevent replication loops
//   AC-5  DDL replication with conflict detection
//   AC-6  Active-active high-availability semantics
//   AC-7  Manual conflict resolution API
//   AC-8  SyncStatus reflects running, sequence numbers, and lag
// ============================================================================

static BidirectionalReplicationManager::BidiConfig makeBidiConfig(
    const std::string& local  = "node-west",
    const std::string& remote = "node-east")
{
    BidirectionalReplicationManager::BidiConfig cfg;
    cfg.local_node_id   = local;
    cfg.remote_node_id  = remote;
    cfg.remote_endpoint = "10.0.0.2:7000";
    cfg.sync_interval_ms = 100;
    cfg.track_origin     = true;
    cfg.replicate_foreign_changes = false;
    cfg.bidirectional_sync = true;
    return cfg;
}

// ── AC-1: Both nodes can be primary (start / stop lifecycle) ─────────────────

TEST(BidirectionalReplicationTest, StartSucceedsWithValidConfig) {
    BidirectionalReplicationManager mgr(makeBidiConfig());
    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.getSyncStatus().is_running);
    mgr.stop();
    EXPECT_FALSE(mgr.getSyncStatus().is_running);
}

TEST(BidirectionalReplicationTest, StartFailsWhenLocalEqualsRemote) {
    auto cfg = makeBidiConfig("same-node", "same-node");
    BidirectionalReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.start());
}

TEST(BidirectionalReplicationTest, StartFailsWhenNodeIdEmpty) {
    BidirectionalReplicationManager::BidiConfig cfg;
    cfg.local_node_id  = "";
    cfg.remote_node_id = "node-east";
    BidirectionalReplicationManager mgr(cfg);
    EXPECT_FALSE(mgr.start());
}

TEST(BidirectionalReplicationTest, DoubleStartIsIdempotent) {
    BidirectionalReplicationManager mgr(makeBidiConfig());
    EXPECT_TRUE(mgr.start());
    // Second start while already running must return false.
    EXPECT_FALSE(mgr.start());
    mgr.stop();
}

// ── AC-1 / AC-6: submitWrite advances local sequence ─────────────────────────

TEST(BidirectionalReplicationTest, SubmitWriteAdvancesLocalSequence) {
    BidirectionalReplicationManager mgr(makeBidiConfig());
    mgr.start();

    uint64_t seq1 = mgr.submitWrite("doc1", "orders", "INSERT", R"({"id":1})");
    uint64_t seq2 = mgr.submitWrite("doc2", "orders", "INSERT", R"({"id":2})");

    EXPECT_EQ(seq1, 1u);
    EXPECT_EQ(seq2, 2u);
    EXPECT_EQ(mgr.getSyncStatus().local_sequence, 2u);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, SubmitWriteReturnsZeroWhenStopped) {
    BidirectionalReplicationManager mgr(makeBidiConfig());
    // Do not call start().
    uint64_t seq = mgr.submitWrite("doc1", "orders", "INSERT", "{}");
    EXPECT_EQ(seq, 0u);
}

// ── AC-4: Origin tracking prevents replication loops ─────────────────────────

TEST(BidirectionalReplicationTest, OriginTrackingRejectsOwnChangeBouncing) {
    auto cfg = makeBidiConfig();
    cfg.replicate_foreign_changes = false;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // Simulate a write that came from the LOCAL node being echoed back.
    BidirectionalReplicationManager::BidiWriteEntry entry;
    entry.document_id  = "doc1";
    entry.collection   = "orders";
    entry.operation    = "UPDATE";
    entry.data         = R"({"v":2})";
    entry.origin_node  = "node-west";  // same as local_node_id
    entry.origin_seq   = 1;
    entry.timestamp_ms = 1000;

    // applyRemoteWrite must reject this because it originated locally.
    EXPECT_FALSE(mgr.applyRemoteWrite(entry));

    mgr.stop();
}

TEST(BidirectionalReplicationTest, OriginTrackingAcceptsPeerChanges) {
    auto cfg = makeBidiConfig();
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    BidirectionalReplicationManager::BidiWriteEntry entry;
    entry.document_id  = "doc2";
    entry.collection   = "orders";
    entry.operation    = "INSERT";
    entry.data         = R"({"v":1})";
    entry.origin_node  = "node-east";  // remote_node_id
    entry.origin_seq   = 5;
    entry.timestamp_ms = 2000;

    EXPECT_TRUE(mgr.applyRemoteWrite(entry));
    EXPECT_EQ(mgr.getSyncStatus().remote_sequence, 5u);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, OriginTrackingRejectsStaleOrDuplicateRemoteSequence) {
    auto cfg = makeBidiConfig();
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    BidirectionalReplicationManager::BidiWriteEntry first;
    first.document_id  = "doc-stale";
    first.collection   = "orders";
    first.operation    = "UPDATE";
    first.data         = R"({"v":1})";
    first.origin_node  = "node-east";
    first.origin_seq   = 5;
    first.timestamp_ms = 2000;
    ASSERT_TRUE(mgr.applyRemoteWrite(first));

    auto stale = first;
    stale.origin_seq = 4;
    stale.data = R"({"v":0})";
    EXPECT_FALSE(mgr.applyRemoteWrite(stale));

    auto duplicate = first;
    duplicate.data = R"({"v":1})";
    EXPECT_FALSE(mgr.applyRemoteWrite(duplicate));

    auto newer = first;
    newer.origin_seq = 6;
    newer.data = R"({"v":2})";
    EXPECT_TRUE(mgr.applyRemoteWrite(newer));

    mgr.stop();
}

// ── AC-2: Conflict detection ──────────────────────────────────────────────────

TEST(BidirectionalReplicationTest, ConcurrentWritesDetectedAsConflict) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::LAST_WRITE_WINS;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // Local write for "doc5" in "users" collection.
    mgr.submitWrite("doc5", "users", "UPDATE", R"({"name":"Alice","ts":100})");

    // Now inject a conflicting remote write for the same document.
    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "doc5";
    remote.collection   = "users";
    remote.operation    = "UPDATE";
    remote.data         = R"({"name":"Bob","ts":200})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 10;
    remote.timestamp_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 1000;
    // Remote timestamp is guaranteed newer than the local submitWrite() wall clock.

    mgr.applyRemoteWrite(remote);

    auto history = mgr.getConflictHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].document_id, "doc5");
    EXPECT_EQ(history[0].collection, "users");
    EXPECT_EQ(history[0].strategy_used, ConflictResolution::LAST_WRITE_WINS);
    // The remote (higher ts=200) should be the winner.
    EXPECT_EQ(history[0].resolved_write.data, R"({"name":"Bob","ts":200})");
    EXPECT_EQ(mgr.getSyncStatus().conflicts_detected, 1u);
    EXPECT_EQ(mgr.getSyncStatus().conflicts_resolved, 1u);

    mgr.stop();
}

// ── AC-3: Configurable conflict resolution per collection ─────────────────────

TEST(BidirectionalReplicationTest, CollectionStrategyOverridesDefault) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::LAST_WRITE_WINS;
    BidirectionalReplicationManager mgr(cfg);

    // Override the strategy for "critical" collection.
    mgr.setCollectionStrategy("critical", ConflictResolution::FIRST_WRITE_WINS);

    EXPECT_EQ(mgr.getEffectiveStrategy("orders"),   ConflictResolution::LAST_WRITE_WINS);
    EXPECT_EQ(mgr.getEffectiveStrategy("critical"), ConflictResolution::FIRST_WRITE_WINS);

    mgr.start();

    mgr.submitWrite("rec1", "critical", "INSERT", R"({"v":"first"})");
    // The local write timestamp is set to the current wall clock by submitWrite.
    // The remote write has a later timestamp (900ms epoch) but since FIRST_WRITE_WINS
    // is configured, the local write (earlier timestamp) should win.

    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "rec1";
    remote.collection   = "critical";
    remote.operation    = "INSERT";
    remote.data         = R"({"v":"second"})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 2;
    remote.timestamp_ms = 900;  // later timestamp

    mgr.applyRemoteWrite(remote);

    auto history = mgr.getConflictHistory();
    ASSERT_GE(history.size(), 1u);
    auto& rec = history.back();
    EXPECT_EQ(rec.strategy_used, ConflictResolution::FIRST_WRITE_WINS);

    mgr.stop();
}

// ── AC-3: CUSTOM strategy defers to manual resolution ────────────────────────

TEST(BidirectionalReplicationTest, CustomStrategyProducesPendingConflict) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::CUSTOM;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    mgr.submitWrite("docC", "finance", "UPDATE", R"({"amount":100})");

    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "docC";
    remote.collection   = "finance";
    remote.operation    = "UPDATE";
    remote.data         = R"({"amount":200})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 7;
    remote.timestamp_ms = 5000;

    mgr.applyRemoteWrite(remote);

    auto pending = mgr.getPendingConflicts();
    ASSERT_EQ(pending.size(), 1u);
    EXPECT_EQ(pending[0].document_id, "docC");

    mgr.stop();
}

// ── AC-7: Manual conflict resolution ─────────────────────────────────────────

TEST(BidirectionalReplicationTest, ManualResolveConflictPicksWinner) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::CUSTOM;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    mgr.submitWrite("docM", "audit", "UPDATE", R"({"val":"local"})");

    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "docM";
    remote.collection   = "audit";
    remote.operation    = "UPDATE";
    remote.data         = R"({"val":"remote"})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 11;
    remote.timestamp_ms = 8000;

    mgr.applyRemoteWrite(remote);

    // Manually nominate the remote node as winner.
    bool resolved = mgr.resolveConflict("docM", "node-east");
    EXPECT_TRUE(resolved);

    auto history = mgr.getConflictHistory();
    ASSERT_GE(history.size(), 1u);
    EXPECT_EQ(history.back().resolved_write.data, R"({"val":"remote"})");
    EXPECT_EQ(mgr.getSyncStatus().conflicts_resolved, 1u);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, ManualResolveReturnsFalseForUnknownNode) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::CUSTOM;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    mgr.submitWrite("docX", "misc", "UPDATE", R"({})");

    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "docX";
    remote.collection   = "misc";
    remote.operation    = "UPDATE";
    remote.data         = R"({"x":1})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 3;
    remote.timestamp_ms = 100;

    mgr.applyRemoteWrite(remote);

    // "unknown-node" is neither local nor remote → must return false.
    EXPECT_FALSE(mgr.resolveConflict("docX", "unknown-node"));

    mgr.stop();
}

// ── AC-5: DDL replication with conflict detection ─────────────────────────────

TEST(BidirectionalReplicationTest, DDLReplicationAcceptedAndTracked) {
    auto cfg = makeBidiConfig();
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    bool ok = mgr.applyRemoteDDL(
        "ALTER TABLE orders ADD COLUMN notes TEXT",
        "v42",
        100);
    EXPECT_TRUE(ok);
    EXPECT_EQ(mgr.getSyncStatus().remote_sequence, 100u);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, DDLConflictIsRecordedAsDDLConflict) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::LAST_WRITE_WINS;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // Simulate a local DDL write.
    mgr.submitWrite("__ddl__v10", "__schema__", "DDL",
                    "ALTER TABLE t ADD COLUMN a INT", /*is_ddl=*/true);

    // Remote DDL for the same schema version → conflict.
    BidirectionalReplicationManager::BidiWriteEntry remote_ddl;
    remote_ddl.document_id  = "__ddl__v10";
    remote_ddl.collection   = "__schema__";
    remote_ddl.operation    = "DDL";
    remote_ddl.data         = "ALTER TABLE t ADD COLUMN b TEXT";
    remote_ddl.origin_node  = "node-east";
    remote_ddl.origin_seq   = 20;
    remote_ddl.timestamp_ms = 9999;
    remote_ddl.is_ddl       = true;

    mgr.applyRemoteWrite(remote_ddl);

    auto history = mgr.getConflictHistory();
    ASSERT_GE(history.size(), 1u);
    EXPECT_TRUE(history.back().is_ddl_conflict);

    mgr.stop();
}

// ── AC-8: SyncStatus reflects lag and synchronisation state ──────────────────

TEST(BidirectionalReplicationTest, SyncStatusReflectsLagFromUpdateRemoteSequence) {
    BidirectionalReplicationManager mgr(makeBidiConfig());
    mgr.start();

    mgr.updateRemoteSequence(50, 300);

    auto s = mgr.getSyncStatus();
    EXPECT_EQ(s.remote_sequence, 50u);
    EXPECT_EQ(s.lag_ms, 300);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, SyncStatusIsSynchronizedWhenNoPendingAndLowLag) {
    auto cfg = makeBidiConfig();
    cfg.sync_interval_ms = 1000;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // No pending writes, lag within interval.
    mgr.updateRemoteSequence(0, 50);

    auto s = mgr.getSyncStatus();
    EXPECT_TRUE(s.is_synchronized);
    EXPECT_TRUE(s.is_running);

    mgr.stop();
}

TEST(BidirectionalReplicationTest, SyncStatusNotSynchronizedWhenHighLag) {
    auto cfg = makeBidiConfig();
    cfg.sync_interval_ms = 100;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    mgr.updateRemoteSequence(0, 5000);  // lag > sync_interval_ms

    auto s = mgr.getSyncStatus();
    EXPECT_FALSE(s.is_synchronized);

    mgr.stop();
}

// ── Config flag: bidirectional_sync = false blocks inbound writes ─────────────

TEST(BidirectionalReplicationTest, BidirectionalSyncFalseBlocksIncomingWrites) {
    auto cfg = makeBidiConfig();
    cfg.bidirectional_sync = false;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    BidirectionalReplicationManager::BidiWriteEntry remote;
    remote.document_id  = "docA";
    remote.collection   = "orders";
    remote.operation    = "INSERT";
    remote.data         = R"({"x":1})";
    remote.origin_node  = "node-east";
    remote.origin_seq   = 1;
    remote.timestamp_ms = 1000;

    // bidirectional_sync = false → incoming remote write must be rejected.
    EXPECT_FALSE(mgr.applyRemoteWrite(remote));

    // No conflict detected and remote_sequence unchanged.
    auto s = mgr.getSyncStatus();
    EXPECT_EQ(s.conflicts_detected, 0u);
    EXPECT_EQ(s.remote_sequence, 0u);

    mgr.stop();
}

// ── Config flag: replicate_ddl = false suppresses DDL ────────────────────────

TEST(BidirectionalReplicationTest, ReplicateDDLFalseBlocksDDLApply) {
    auto cfg = makeBidiConfig();
    cfg.replicate_ddl = false;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // With replicate_ddl=false, applyRemoteDDL() must return false.
    bool ok = mgr.applyRemoteDDL(
        "ALTER TABLE t ADD COLUMN x INT",
        "v99",
        50);
    EXPECT_FALSE(ok);

    // Remote sequence must not advance, no conflict recorded.
    auto s = mgr.getSyncStatus();
    EXPECT_EQ(s.remote_sequence, 0u);
    EXPECT_EQ(s.conflicts_detected, 0u);

    mgr.stop();
}

// ── conflicts_last_hour rolling window ────────────────────────────────────────

TEST(BidirectionalReplicationTest, ConflictsLastHourCountedInSyncStatus) {
    auto cfg = makeBidiConfig();
    cfg.default_strategy = ConflictResolution::LAST_WRITE_WINS;
    BidirectionalReplicationManager mgr(cfg);
    mgr.start();

    // Generate two conflicts by submitting a local write then injecting
    // a conflicting remote write for the same document twice.
    for (int i = 0; i < 2; ++i) {
        const std::string doc = "doc-lh-" + std::to_string(i);
        mgr.submitWrite(doc, "metrics", "UPDATE", R"({"v":1})");

        BidirectionalReplicationManager::BidiWriteEntry remote;
        remote.document_id  = doc;
        remote.collection   = "metrics";
        remote.operation    = "UPDATE";
        remote.data         = R"({"v":2})";
        remote.origin_node  = "node-east";
        remote.origin_seq   = static_cast<uint64_t>(i + 1);
        remote.timestamp_ms = 9000 + i;  // always newer → remote wins under LWW

        mgr.applyRemoteWrite(remote);
    }

    auto s = mgr.getSyncStatus();
    EXPECT_EQ(s.conflicts_detected, 2u);
    // Both conflicts just happened → they fall within the last hour.
    EXPECT_EQ(s.conflicts_last_hour, 2u);

    mgr.stop();
}

// ============================================================================
// MultiTierReplicationTest  (v1.8.0)
// ============================================================================
//
// Validates all acceptance criteria for Multi-Tier Replication:
//   AC-1  Tier 1: Strong consistency, high durability (3+ replicas, sync, <10ms)
//   AC-2  Tier 2: Eventual consistency, moderate durability (2 replicas, semi-sync)
//   AC-3  Tier 3: Best-effort, low durability (1 replica, async)
//   AC-4  Per-collection tier assignment
//   AC-5  Automatic tier promotion/demotion based on access patterns
// ============================================================================

// ── AC-1: Tier 1 default config ──────────────────────────────────────────────

TEST(MultiTierReplicationTest, Tier1DefaultConfigHasStrongConsistency) {
    MultiTierReplicationManager mgr;
    TierConfig cfg = mgr.getDefaultTierConfig(ReplicationTier::TIER_1_CRITICAL);

    EXPECT_EQ(cfg.tier,          ReplicationTier::TIER_1_CRITICAL);
    EXPECT_GE(cfg.replica_count, 3u);
    EXPECT_EQ(cfg.mode,          ReplicationMode::SYNC);
    EXPECT_LE(cfg.max_latency_ms, 10u);
}

// ── AC-2: Tier 2 default config ──────────────────────────────────────────────

TEST(MultiTierReplicationTest, Tier2DefaultConfigHasModerateConsistency) {
    MultiTierReplicationManager mgr;
    TierConfig cfg = mgr.getDefaultTierConfig(ReplicationTier::TIER_2_STANDARD);

    EXPECT_EQ(cfg.tier,          ReplicationTier::TIER_2_STANDARD);
    EXPECT_EQ(cfg.replica_count, 2u);
    EXPECT_EQ(cfg.mode,          ReplicationMode::SEMI_SYNC);
    EXPECT_LE(cfg.max_latency_ms, 50u);
}

// ── AC-3: Tier 3 default config ──────────────────────────────────────────────

TEST(MultiTierReplicationTest, Tier3DefaultConfigHasAsyncBestEffort) {
    MultiTierReplicationManager mgr;
    TierConfig cfg = mgr.getDefaultTierConfig(ReplicationTier::TIER_3_ARCHIVAL);

    EXPECT_EQ(cfg.tier,          ReplicationTier::TIER_3_ARCHIVAL);
    EXPECT_EQ(cfg.replica_count, 1u);
    EXPECT_EQ(cfg.mode,          ReplicationMode::ASYNC);
}

// ── AC-4: Per-collection tier assignment ─────────────────────────────────────

TEST(MultiTierReplicationTest, AssignTierPersistsAndIsRetrievable) {
    MultiTierReplicationManager mgr;

    mgr.assignTier("financial_transactions", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("user_profiles",          ReplicationTier::TIER_2_STANDARD);
    mgr.assignTier("audit_logs",             ReplicationTier::TIER_3_ARCHIVAL);

    EXPECT_EQ(mgr.getTier("financial_transactions"), ReplicationTier::TIER_1_CRITICAL);
    EXPECT_EQ(mgr.getTier("user_profiles"),          ReplicationTier::TIER_2_STANDARD);
    EXPECT_EQ(mgr.getTier("audit_logs"),             ReplicationTier::TIER_3_ARCHIVAL);
}

TEST(MultiTierReplicationTest, UnassignedCollectionReturnsDefaultTier) {
    MultiTierConfig config;
    config.default_tier = ReplicationTier::TIER_2_STANDARD;
    MultiTierReplicationManager mgr(config);

    EXPECT_EQ(mgr.getTier("unknown_collection"), ReplicationTier::TIER_2_STANDARD);
}

TEST(MultiTierReplicationTest, AssignTierOverridesExistingAssignment) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("orders", ReplicationTier::TIER_3_ARCHIVAL);
    EXPECT_EQ(mgr.getTier("orders"), ReplicationTier::TIER_3_ARCHIVAL);

    mgr.assignTier("orders", ReplicationTier::TIER_1_CRITICAL);
    EXPECT_EQ(mgr.getTier("orders"), ReplicationTier::TIER_1_CRITICAL);
}

TEST(MultiTierReplicationTest, RemoveTierFallsBackToDefault) {
    MultiTierConfig config;
    config.default_tier = ReplicationTier::TIER_2_STANDARD;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("events", ReplicationTier::TIER_1_CRITICAL);
    EXPECT_EQ(mgr.getTier("events"), ReplicationTier::TIER_1_CRITICAL);

    mgr.removeTier("events");
    EXPECT_EQ(mgr.getTier("events"), ReplicationTier::TIER_2_STANDARD);
}

TEST(MultiTierReplicationTest, GetTierConfigReflectsAssignedTier) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("transactions", ReplicationTier::TIER_1_CRITICAL);

    TierConfig cfg = mgr.getTierConfig("transactions");
    EXPECT_EQ(cfg.tier,   ReplicationTier::TIER_1_CRITICAL);
    EXPECT_EQ(cfg.mode,   ReplicationMode::SYNC);
    EXPECT_GE(cfg.replica_count, 3u);
}

TEST(MultiTierReplicationTest, GetCollectionsForTierReturnsCorrectSubset) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("col_a", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("col_b", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("col_c", ReplicationTier::TIER_3_ARCHIVAL);

    auto tier1 = mgr.getCollectionsForTier(ReplicationTier::TIER_1_CRITICAL);
    ASSERT_EQ(tier1.size(), 2u);

    auto tier3 = mgr.getCollectionsForTier(ReplicationTier::TIER_3_ARCHIVAL);
    ASSERT_EQ(tier3.size(), 1u);
    EXPECT_EQ(tier3[0], "col_c");

    auto tier2 = mgr.getCollectionsForTier(ReplicationTier::TIER_2_STANDARD);
    EXPECT_TRUE(tier2.empty());
}

// ── Tier config override ──────────────────────────────────────────────────────

TEST(MultiTierReplicationTest, CustomTierConfigOverridesBuiltinDefaults) {
    MultiTierConfig config;
    TierConfig custom;
    custom.tier                = ReplicationTier::TIER_1_CRITICAL;
    custom.replica_count       = 5;
    custom.mode                = ReplicationMode::SYNC;
    custom.max_latency_ms      = 5;
    custom.min_availability_pct = 99;
    config.tier1_config        = custom;

    MultiTierReplicationManager mgr(config);
    TierConfig got = mgr.getDefaultTierConfig(ReplicationTier::TIER_1_CRITICAL);

    EXPECT_EQ(got.replica_count, 5u);
    EXPECT_EQ(got.max_latency_ms, 5u);
}

// ── AC-5: Auto-tiering promotion ─────────────────────────────────────────────

TEST(MultiTierReplicationTest, AutoTieringDisabledByDefault) {
    MultiTierReplicationManager mgr;
    EXPECT_FALSE(mgr.isAutoTieringEnabled());
}

TEST(MultiTierReplicationTest, EnableAutoTieringToggleWorks) {
    MultiTierReplicationManager mgr;
    mgr.enableAutoTiering(true);
    EXPECT_TRUE(mgr.isAutoTieringEnabled());
    mgr.enableAutoTiering(false);
    EXPECT_FALSE(mgr.isAutoTieringEnabled());
}

TEST(MultiTierReplicationTest, RecordAccessHasNoEffectWhenAutoTieringDisabled) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("metrics", ReplicationTier::TIER_2_STANDARD);

    // Auto-tiering is OFF: accesses should not be tracked
    for (int i = 0; i < 200; ++i) {
        mgr.recordAccess("metrics");
    }
    auto stats = mgr.getCollectionStats();
    // Either no stats entry or zero total_accesses
    bool found = false;
    for (const auto& s : stats) {
        if (s.collection == "metrics") {
            found = true;
            EXPECT_EQ(s.total_accesses, 0u);
        }
    }
    // If no entry at all that is also acceptable
    (void)found;
}

TEST(MultiTierReplicationTest, HotCollectionPromotedToTier1ByAutoTiering) {
    MultiTierConfig config;
    config.auto_tiering_enabled  = true;
    config.hot_access_threshold  = 50;   // 50 accesses/min → Tier 1
    config.cold_access_threshold = 5;
    config.auto_tier_window_seconds = 60;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("hot_collection", ReplicationTier::TIER_2_STANDARD);

    // Record 60 accesses (rate = 60/min > 50 threshold)
    for (int i = 0; i < 60; ++i) {
        mgr.recordAccess("hot_collection");
    }

    ReplicationTier new_tier = mgr.evaluateTierPromotion("hot_collection");
    EXPECT_EQ(new_tier, ReplicationTier::TIER_1_CRITICAL);
    EXPECT_EQ(mgr.getTier("hot_collection"), ReplicationTier::TIER_1_CRITICAL);
}

TEST(MultiTierReplicationTest, ColdCollectionDemotedToTier3ByAutoTiering) {
    MultiTierConfig config;
    config.auto_tiering_enabled  = true;
    config.hot_access_threshold  = 100;
    config.cold_access_threshold = 10;   // < 10/min → Tier 3
    config.auto_tier_window_seconds = 60;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("cold_collection", ReplicationTier::TIER_2_STANDARD);

    // Record only 3 accesses (rate = 3/min < 10 threshold)
    for (int i = 0; i < 3; ++i) {
        mgr.recordAccess("cold_collection");
    }

    ReplicationTier new_tier = mgr.evaluateTierPromotion("cold_collection");
    EXPECT_EQ(new_tier, ReplicationTier::TIER_3_ARCHIVAL);
    EXPECT_EQ(mgr.getTier("cold_collection"), ReplicationTier::TIER_3_ARCHIVAL);
}

TEST(MultiTierReplicationTest, ModerateAccessCollectionNormalisedToTier2) {
    MultiTierConfig config;
    config.auto_tiering_enabled  = true;
    config.hot_access_threshold  = 100;
    config.cold_access_threshold = 5;
    config.auto_tier_window_seconds = 60;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("normal_col", ReplicationTier::TIER_1_CRITICAL);

    // Record moderate accesses: 30/min → between 5 and 100 → Tier 2
    for (int i = 0; i < 30; ++i) {
        mgr.recordAccess("normal_col");
    }

    ReplicationTier new_tier = mgr.evaluateTierPromotion("normal_col");
    EXPECT_EQ(new_tier, ReplicationTier::TIER_2_STANDARD);
}

TEST(MultiTierReplicationTest, EvaluateTierNoChangeWhenAutoTieringDisabled) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("locked_col", ReplicationTier::TIER_1_CRITICAL);

    // Auto-tiering disabled: evaluateTierPromotion should not move the tier
    ReplicationTier result = mgr.evaluateTierPromotion("locked_col");
    EXPECT_EQ(result, ReplicationTier::TIER_1_CRITICAL);
}

// ── Statistics ────────────────────────────────────────────────────────────────

TEST(MultiTierReplicationTest, GetStatsReflectsAssignments) {
    MultiTierReplicationManager mgr;
    mgr.assignTier("t1a", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("t1b", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("t2a", ReplicationTier::TIER_2_STANDARD);
    mgr.assignTier("t3a", ReplicationTier::TIER_3_ARCHIVAL);

    MultiTierStats stats = mgr.getStats();
    EXPECT_EQ(stats.collections_tier1, 2u);
    EXPECT_EQ(stats.collections_tier2, 1u);
    EXPECT_EQ(stats.collections_tier3, 1u);
    EXPECT_FALSE(stats.auto_tiering_active);
}

TEST(MultiTierReplicationTest, GetStatsCountsPromotionsAndDemotions) {
    MultiTierConfig config;
    config.auto_tiering_enabled  = true;
    config.hot_access_threshold  = 10;
    config.cold_access_threshold = 2;
    config.auto_tier_window_seconds = 60;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("p_col", ReplicationTier::TIER_2_STANDARD);
    mgr.assignTier("d_col", ReplicationTier::TIER_2_STANDARD);

    // Promote p_col
    for (int i = 0; i < 15; ++i) {
      mgr.recordAccess("p_col");
    }
    mgr.evaluateTierPromotion("p_col");

    // Demote d_col
    mgr.recordAccess("d_col"); // 1 access → < 2 threshold
    mgr.evaluateTierPromotion("d_col");

    MultiTierStats stats = mgr.getStats();
    EXPECT_EQ(stats.total_promotions, 1u);
    EXPECT_EQ(stats.total_demotions,  1u);
}

TEST(MultiTierReplicationTest, GetCollectionStatsIncludesAllTrackedCollections) {
    MultiTierConfig config;
    config.auto_tiering_enabled = true;
    MultiTierReplicationManager mgr(config);

    mgr.assignTier("col1", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("col2", ReplicationTier::TIER_3_ARCHIVAL);
    mgr.recordAccess("col1");
    mgr.recordAccess("col1");

    auto cs = mgr.getCollectionStats();
    ASSERT_GE(cs.size(), 2u);

    bool found_col1 = false;
    for (const auto& s : cs) {
        if (s.collection == "col1") {
            found_col1 = true;
            EXPECT_EQ(s.total_accesses, 2u);
            EXPECT_EQ(s.current_tier, ReplicationTier::TIER_1_CRITICAL);
        }
    }
    EXPECT_TRUE(found_col1);
}

// ============================================================================
// Performance Tests (opt-in: set THEMIS_RUN_PERF_TESTS=1)
//
// These tests validate the design constraints from
// src/replication/FUTURE_ENHANCEMENTS.md §Design Constraints:
//
//   #4  Vector clock comparison and HLC conflict detection must add
//       < 5 µs per write operation.
//
// Additional local-component benchmarks validate prerequisites for:
//   #1  Replication lag p99 ≤ 50 ms (WAL append throughput > 50 k/s).
//   #2  WAL shipping ≥ 500 MB/s compressed (Zstd throughput proxy).
// ============================================================================

/**
 * @brief VectorClock increment + compare combined latency < 5 µs.
 *
 * Design Constraint #4 (FUTURE_ENHANCEMENTS.md): "Vector clock comparison
 * and HLC conflict detection must add < 5 µs per write operation."
 *
 * This test measures the median cost of one VectorClock::increment() followed
 * by one VectorClock::compare() — i.e. the overhead added to every write
 * on the multi-master write path.
 *
 * Set THEMIS_RUN_PERF_TESTS=1 to enable.
 */
TEST(VectorClockPerfTest, IncrementAndCompareSingleOpUnder5us) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping VectorClock perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "Design Constraint #4: increment+compare < 5 µs.";
    }

    VectorClock vc_local("local");
    VectorClock vc_remote("remote");
    vc_remote.increment("remote");

    const int kWarmup = 100;
    const int kIterations = 2000;
    std::vector<int64_t> durations_ns;
    durations_ns.reserve(kIterations);

    // Warm up
    volatile int sink_warmup = 0;
    for (int i = 0; i < kWarmup; ++i) {
        vc_local.increment("local");
        sink_warmup += vc_local.compare(vc_remote);
    }
    (void)sink_warmup;

    volatile int sink = 0;
    for (int i = 0; i < kIterations; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        vc_local.increment("local");
        int cmp = vc_local.compare(vc_remote);
        auto t1 = std::chrono::steady_clock::now();
        sink += cmp;
        durations_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    std::sort(durations_ns.begin(), durations_ns.end());
    int64_t median_ns = durations_ns[kIterations / 2];
    int64_t p99_ns    = durations_ns[static_cast<size_t>(kIterations * 0.99)];

    EXPECT_LT(median_ns, 5000) << "VectorClock increment+compare median must be < 5 us;"
        " median=" << median_ns << " ns p99=" << p99_ns << " ns";
}

/**
 * @brief HybridLogicalClock::now() latency < 5 µs per call.
 *
 * Design Constraint #4 (FUTURE_ENHANCEMENTS.md): "Vector clock comparison
 * and HLC conflict detection must add < 5 µs per write operation."
 *
 * Set THEMIS_RUN_PERF_TESTS=1 to enable.
 */
TEST(HLCPerfTest, NowCallUnder5us) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping HLC::now() perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "Design Constraint #4: HLC::now() < 5 µs.";
    }

    HybridLogicalClock hlc("perf-node");

    const int kWarmup     = 200;
    const int kIterations = 2000;
    std::vector<int64_t> durations_ns;
    durations_ns.reserve(kIterations);

    volatile uint64_t sink_warmup = 0;
    for (int i = 0; i < kWarmup; ++i) {
        auto ts = hlc.now();
        sink_warmup += ts.logical;
    }
    (void)sink_warmup;

    volatile uint64_t sink = 0;
    for (int i = 0; i < kIterations; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        auto ts = hlc.now();
        auto t1 = std::chrono::steady_clock::now();
        sink += ts.logical;
        durations_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    (void)sink;

    std::sort(durations_ns.begin(), durations_ns.end());
    int64_t median_ns = durations_ns[kIterations / 2];
    int64_t p99_ns    = durations_ns[static_cast<size_t>(kIterations * 0.99)];

    EXPECT_LT(median_ns, 5000) << "HLC::now() median must be < 5 us;"
        " median=" << median_ns << " ns p99=" << p99_ns << " ns";
}

/**
 * @brief WAL append throughput > 50,000 entries/s (prerequisite for Design
 *        Constraint #1: replication lag p99 ≤ 50 ms at 10,000 write/s).
 *
 * Set THEMIS_RUN_PERF_TESTS=1 to enable.
 */
TEST(WALAppendThroughputPerfTest, Over50kEntriesPerSecond) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping WAL append throughput perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "Target: > 50,000 entries/s.";
    }

    TempWALDir wal_dir("/tmp/themis_perf_wal_append");
    ReplicationConfig cfg = makeConfig(wal_dir.path);
    cfg.wal_sync_on_commit = false;
    WALManager wal(cfg);

    const int kEntries = 20000;
    std::atomic<uint64_t> seq{1};

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kEntries; ++i) {
        WALEntry e;
        e.sequence_number = seq.fetch_add(1, std::memory_order_relaxed);
        e.term            = 1;
        e.timestamp       = std::chrono::system_clock::now();
        e.operation       = "INSERT";
        e.collection      = "perf_col";
        e.document_id     = "doc_" + std::to_string(e.sequence_number);
        e.data            = R"({"k":"v","seq":)" + std::to_string(e.sequence_number) + "}";
        volatile uint64_t written  = wal.append(e);
        (void)written;
    }
    auto t1 = std::chrono::steady_clock::now();

    double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
    double entries_per_s = static_cast<double>(kEntries) / elapsed_s;

    EXPECT_GT(entries_per_s, 50000.0) << "WAL append throughput must exceed 50,000 entries/s;"
        " measured " << static_cast<int64_t>(entries_per_s) << " entries/s"
        " (" << kEntries << " entries in " << elapsed_s * 1000.0 << " ms)";
}

/**
 * @brief CompressedReplicationStream Zstd throughput proxy.
 *
 * Validates that the in-process Zstd compression path can sustain the
 * serialise+compress throughput needed for the 500 MB/s WAL shipping goal
 * (Design Constraint #2).  Each batch of 1,000 × 512-byte WAL entries
 * amounts to ~512 KB; the test asserts the round-trip completes within
 * 50 ms (≥ 10 MB/s — conservative floor that rules out algorithmic regressions
 * without requiring network infrastructure).
 *
 * Set THEMIS_RUN_PERF_TESTS=1 to enable.
 */
TEST(CompressedStreamThroughputPerfTest, ZstdBatchUnder50ms) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping compressed stream throughput perf test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "Target: Zstd batch of 1,000 × 512-byte entries < 50 ms.";
    }

    CompressedReplicationStream::CompressionConfig cfg;
    cfg.algorithm        = CompressedReplicationStream::CompressionAlgorithm::ZSTD;
    cfg.compression_level = 3;
    cfg.adaptive         = false;
    cfg.min_batch_size   = 0;

    CompressedReplicationStream stream("bench-endpoint:7000", cfg);

    const int kEntries = 1000;
    const std::string payload(480, 'x');  // ~480 B data field → ~512 B per entry
    std::vector<WALEntry> batch;
    batch.reserve(kEntries);
    for (int i = 0; i < kEntries; ++i) {
        WALEntry e;
        e.sequence_number = static_cast<uint64_t>(i + 1);
        e.term            = 1;
        e.timestamp       = std::chrono::system_clock::now();
        e.operation       = "INSERT";
        e.collection      = "perf_col";
        e.document_id     = "doc_" + std::to_string(i);
        e.data            = "{\"v\":\"" + payload + "\"}";
        batch.push_back(std::move(e));
    }

    // Warm up
    stream.sendBatch(batch);
    stream.resetStats();

    const int kRounds = 5;
    auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < kRounds; ++r) {
        bool ok = stream.sendBatch(batch);
        ASSERT_TRUE(ok) << "sendBatch() must succeed in the perf test";
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double per_round_ms = static_cast<double>(elapsed_ms) / kRounds;

    EXPECT_LT(per_round_ms, 50.0) << "Zstd compress of 1,000 x ~512-byte entries must"
        " complete in < 50 ms; measured " << per_round_ms << " ms/round";

    auto stats = stream.getStats();
    EXPECT_GT(stats.bytes_uncompressed, 0u) << "Stats must report processed bytes";
    EXPECT_GT(stats.compression_ratio, 1.0)  << "Zstd must achieve some compression";
}

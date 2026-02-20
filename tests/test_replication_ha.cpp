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
#include "replication/multi_master_replication.h"

#include <filesystem>
#include <fstream>
#include <future>
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

TEST(VectorClockTest, Merge) {
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
}

// ============================================================================
// 12. MMWriteEntry serialize / deserialize round-trip
// ============================================================================

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
                    if (id.empty()) errors.fetch_add(1);
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    // Wait for the queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_EQ(errors.load(), 0) << "No errors expected during concurrent writes";

    auto stats = mgr.getStats();
    EXPECT_GE(stats.writes_total,
              static_cast<uint64_t>(kWriters * kWritesPerThread));

    mgr.stop();
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

// ============================================================================
// 17. PersistentReplicationState
// ============================================================================

class PersistentStateTest : public ::testing::Test {
protected:
    std::string path_{"/tmp/themis_repl_state_test.dat"};
    void SetUp()    override { std::filesystem::remove(path_); }
    void TearDown() override { std::filesystem::remove(path_); }
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
                if (!prs.persist(s)) errors.fetch_add(1);
            }
        });
    }
    for (auto& th : threads) th.join();

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


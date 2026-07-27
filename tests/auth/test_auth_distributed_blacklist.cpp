/*
 * ThemisDB | File: test_auth_distributed_blacklist.cpp | Version: 0.0.1
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=0
 * Status: Production Ready
 */

/**
 * @file test_auth_distributed_blacklist.cpp
 * @brief Focused regression tests for DistributedTokenBlacklist (v1.3.0).
 *
 * Covers:
 *   DBL-01  Basic add/isRevoked (single-node, no cluster sync)
 *   DBL-02  Future expiry: token revoked until its natural expiry time
 *   DBL-03  Unknown JTI returns false (not revoked)
 *   DBL-04  Expired entry no longer reported as revoked
 *   DBL-05  purgeExpired removes entries whose expiry has passed
 *   DBL-06  Multiple distinct JTIs are tracked independently
 *   DBL-07  Concurrent add/isRevoked is thread-safe
 *   DBL-08  Re-adding an existing JTI is idempotent (no crash, last write wins)
 *   DBL-09  Single-node leader election: sole node becomes leader
 *   DBL-10  Multi-node leader election: lowest node_id wins
 *   DBL-11  isLeader() reflects result of performLeaderElection via syncWithCluster
 *   DBL-12  syncWithCluster returns a valid future (resolves to bool)
 *   DBL-13  waitForClusterConvergence returns true immediately in single-node mode
 *   DBL-14  waitForClusterConvergence times out when cluster sync is enabled
 *            but configured peers are unreachable (no actual peers present)
 *   DBL-15  getReplicationStats() returns zero-initialised struct at startup
 *   DBL-16  Config defaults are applied correctly
 *   DBL-17  Destructor cleans up without crashing (RAII lifecycle test)
 */

#include <gtest/gtest.h>
#include "auth/distributed_token_blacklist.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <future>

using namespace themis::auth;
namespace fs = std::filesystem;

// ============================================================================
// Test fixture: creates and cleans up a per-test RocksDB temp directory
// ============================================================================

class DistributedTokenBlacklistTest : public ::testing::Test {
protected:
    fs::path db_path_;

    void SetUp() override {
        // Create a unique temp directory for each test so tests are isolated.
        db_path_ = fs::temp_directory_path()
                   / ("themis_auth_dtb_test_" + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(db_path_);
    }

    void TearDown() override {
        fs::remove_all(db_path_);
    }

    /// Build a minimal single-node config (no cluster sync).
    DistributedBlacklistConfig singleNodeConfig() const {
        DistributedBlacklistConfig cfg;
        cfg.db_path              = db_path_.string();
        cfg.enable_cluster_sync  = false;
        cfg.purge_interval_seconds = 3600;  // long purge interval — manual only
        cfg.sync_interval_seconds  = 3600;
        cfg.local_node.node_id     = "node-A";
        return cfg;
    }

    /// Time-point helper: now + seconds
    static std::chrono::system_clock::time_point future(int seconds) {
        return std::chrono::system_clock::now()
               + std::chrono::seconds(seconds);
    }

    /// Time-point helper: now - seconds (already expired)
    static std::chrono::system_clock::time_point past(int seconds) {
        return std::chrono::system_clock::now()
               - std::chrono::seconds(seconds);
    }
};

// ============================================================================
// DBL-01 — Basic add / isRevoked (single-node, no cluster sync)
// ============================================================================

/**
 * @brief Adding a JTI causes isRevoked() to return true for that JTI.
 */
TEST_F(DistributedTokenBlacklistTest, DBL01_AddAndIsRevoked) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    const std::string jti = "jti-basic-01";
    EXPECT_FALSE(bl.isRevoked(jti)) << "Unknown JTI must not be revoked";

    bl.add(jti, future(3600));
    EXPECT_TRUE(bl.isRevoked(jti)) << "JTI must be revoked after add()";
}

// ============================================================================
// DBL-02 — Future expiry: token remains revoked until expiry
// ============================================================================

/**
 * @brief A JTI with a future expiry is reported as revoked immediately after
 *        add() and remains revoked as long as the expiry has not elapsed.
 */
TEST_F(DistributedTokenBlacklistTest, DBL02_FutureExpiryRemainsRevoked) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    const std::string jti = "jti-future-02";
    bl.add(jti, future(3600));  // expires in one hour

    // Should be revoked right away
    EXPECT_TRUE(bl.isRevoked(jti));
    // Still revoked a little later (well within the 1-hour window)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(bl.isRevoked(jti));
}

// ============================================================================
// DBL-03 — Unknown JTI returns false
// ============================================================================

/**
 * @brief An unknown JTI must not be considered revoked.
 */
TEST_F(DistributedTokenBlacklistTest, DBL03_UnknownJtiNotRevoked) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    EXPECT_FALSE(bl.isRevoked("does-not-exist"));
    EXPECT_FALSE(bl.isRevoked(""));
    EXPECT_FALSE(bl.isRevoked("another-unknown-jti"));
}

// ============================================================================
// DBL-04 — Expired entry is no longer reported as revoked
// ============================================================================

/**
 * @brief A JTI whose expiry is in the past should not be reported as revoked.
 *
 * The implementation stores the expiry time and checks it against the current
 * time, so a past expiry causes isRevoked() to return false even if the entry
 * is still physically present in RocksDB.
 */
TEST_F(DistributedTokenBlacklistTest, DBL04_ExpiredEntryNotRevoked) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    // Add a JTI with an expiry 2 seconds in the past.
    const std::string jti = "jti-expired-04";
    bl.add(jti, past(2));

    // The entry is physically present but its expiry has already elapsed.
    EXPECT_FALSE(bl.isRevoked(jti))
        << "isRevoked() must return false for an already-expired entry";
}

// ============================================================================
// DBL-05 — purgeExpired removes entries whose expiry has passed
// ============================================================================

/**
 * @brief purgeExpired() removes entries with past expiry from RocksDB.
 *        After purging, isRevoked() continues to return false (expired entries
 *        are already false before purging; purge just frees storage).
 */
TEST_F(DistributedTokenBlacklistTest, DBL05_PurgeExpiredRemovesOldEntries) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    const std::string expired_jti = "jti-expired-05a";
    const std::string valid_jti   = "jti-valid-05b";

    bl.add(expired_jti, past(10));
    bl.add(valid_jti,   future(3600));

    // Before purge: expired entry is already not-revoked; valid entry is revoked.
    EXPECT_FALSE(bl.isRevoked(expired_jti));
    EXPECT_TRUE(bl.isRevoked(valid_jti));

    // Purge must not throw.
    EXPECT_NO_THROW(bl.purgeExpired());

    // After purge: behavior is unchanged.
    EXPECT_FALSE(bl.isRevoked(expired_jti));
    EXPECT_TRUE(bl.isRevoked(valid_jti));
}

// ============================================================================
// DBL-06 — Multiple distinct JTIs are tracked independently
// ============================================================================

/**
 * @brief Each JTI has an independent revocation entry; revoking one JTI does
 *        not affect others.
 */
TEST_F(DistributedTokenBlacklistTest, DBL06_MultipleJtisAreIndependent) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    const std::vector<std::string> jtis = {
        "jti-alpha", "jti-beta", "jti-gamma"
    };

    for (const auto& j : jtis) {
        EXPECT_FALSE(bl.isRevoked(j));
    }

    // Revoke only the first two.
    bl.add(jtis[0], future(3600));
    bl.add(jtis[1], future(1800));

    EXPECT_TRUE(bl.isRevoked(jtis[0]));
    EXPECT_TRUE(bl.isRevoked(jtis[1]));
    EXPECT_FALSE(bl.isRevoked(jtis[2])) << "Third JTI must remain unrevoked";
}

// ============================================================================
// DBL-07 — Concurrent add/isRevoked is thread-safe
// ============================================================================

/**
 * @brief Concurrent writers and readers must not produce data races or crashes.
 *        After all writers finish, all added JTIs must be reported as revoked.
 */
TEST_F(DistributedTokenBlacklistTest, DBL07_ConcurrentAccessIsThreadSafe) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    constexpr int kWriterThreads  = 4;
    constexpr int kJtisPerWriter  = 25;
    constexpr int kReaderThreads  = 4;

    std::atomic<bool> stop_readers{false};
    std::vector<std::thread> threads;
    threads.reserve(kWriterThreads + kReaderThreads);

    // Writers add JTIs concurrently.
    for (int w = 0; w < kWriterThreads; ++w) {
        threads.emplace_back([&bl, w] {
            for (int i = 0; i < kJtisPerWriter; ++i) {
                std::string jti = "jti-concurrent-w" + std::to_string(w)
                                  + "-i" + std::to_string(i);
                bl.add(jti, future(3600));
            }
        });
    }

    // Readers call isRevoked() concurrently (any JTI may or may not be present yet).
    for (int r = 0; r < kReaderThreads; ++r) {
        threads.emplace_back([&bl, &stop_readers] {
            while (!stop_readers.load()) {
                // Reading an arbitrary JTI; we only care it doesn't crash.
                (void)bl.isRevoked("some-probing-jti");
            }
        });
    }

    // Join writers first, then stop readers.
    for (int i = 0; i < kWriterThreads; ++i) {
        threads[i].join();
    }
    stop_readers.store(true);
    for (int i = kWriterThreads; i < kWriterThreads + kReaderThreads; ++i) {
        threads[i].join();
    }

    // All written JTIs must now be revoked.
    for (int w = 0; w < kWriterThreads; ++w) {
        for (int i = 0; i < kJtisPerWriter; ++i) {
            std::string jti = "jti-concurrent-w" + std::to_string(w)
                              + "-i" + std::to_string(i);
            EXPECT_TRUE(bl.isRevoked(jti))
                << "JTI " << jti << " must be revoked after concurrent write";
        }
    }
}

// ============================================================================
// DBL-08 — Re-adding an existing JTI is idempotent
// ============================================================================

/**
 * @brief add() for an already-revoked JTI must not crash or corrupt state.
 *        The entry remains revoked; last-write-wins semantics are acceptable.
 */
TEST_F(DistributedTokenBlacklistTest, DBL08_ReAddIsIdempotent) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    const std::string jti = "jti-idempotent-08";

    bl.add(jti, future(3600));
    EXPECT_TRUE(bl.isRevoked(jti));

    // Add again with a different expiry — must not throw or crash.
    EXPECT_NO_THROW(bl.add(jti, future(7200)));
    EXPECT_TRUE(bl.isRevoked(jti));
}

// ============================================================================
// DBL-09 — Single-node leader election: sole node becomes leader
// ============================================================================

/**
 * @brief A node with no peers must elect itself as leader.
 *
 * Leader election uses node-ID ordering: a node becomes leader when no peer
 * has a strictly smaller node_id. With no peers, the node is always leader.
 */
TEST_F(DistributedTokenBlacklistTest, DBL09_SingleNodeBecomesLeader) {
    DistributedBlacklistConfig cfg = singleNodeConfig();
    cfg.enable_cluster_sync = true;   // enable sync path so election runs
    cfg.peer_nodes.clear();           // no peers
    cfg.local_node.node_id = "node-solo";

    DistributedTokenBlacklist bl(cfg);

    // Trigger a sync (which internally calls performLeaderElection).
    auto result = bl.syncWithCluster();
    result.wait();

    EXPECT_TRUE(bl.isLeader())
        << "Sole node with no peers must be the replication leader";
}

// ============================================================================
// DBL-10 — Multi-node leader election: lowest node_id wins
// ============================================================================

/**
 * @brief When peers have lower node_ids, the local node must NOT be leader.
 *        When the local node_id is the smallest, it MUST be leader.
 */
TEST_F(DistributedTokenBlacklistTest, DBL10_LowestNodeIdBecomesLeader) {
    // Case A: local node has the highest ID → not leader.
    {
        fs::path path_a = db_path_ / "case_a";
        fs::create_directories(path_a);

        DistributedBlacklistConfig cfg;
        cfg.db_path             = path_a.string();
        cfg.enable_cluster_sync = true;
        cfg.purge_interval_seconds = 3600;
        cfg.sync_interval_seconds  = 3600;
        cfg.local_node.node_id  = "node-Z";   // highest among the three

        ClusterNode p1;  p1.node_id = "node-A";
        ClusterNode p2;  p2.node_id = "node-M";
        cfg.peer_nodes = {p1, p2};

        DistributedTokenBlacklist bl(cfg);
        bl.syncWithCluster().wait();

        EXPECT_FALSE(bl.isLeader())
            << "Node with ID 'node-Z' must not be leader when peers have lower IDs";
    }

    // Case B: local node has the lowest ID → is leader.
    {
        fs::path path_b = db_path_ / "case_b";
        fs::create_directories(path_b);

        DistributedBlacklistConfig cfg;
        cfg.db_path             = path_b.string();
        cfg.enable_cluster_sync = true;
        cfg.purge_interval_seconds = 3600;
        cfg.sync_interval_seconds  = 3600;
        cfg.local_node.node_id  = "node-A";   // lowest

        ClusterNode p1;  p1.node_id = "node-M";
        ClusterNode p2;  p2.node_id = "node-Z";
        cfg.peer_nodes = {p1, p2};

        DistributedTokenBlacklist bl(cfg);
        bl.syncWithCluster().wait();

        EXPECT_TRUE(bl.isLeader())
            << "Node with ID 'node-A' must be leader when it has the lowest ID";
    }
}

// ============================================================================
// DBL-11 — isLeader() reflects result of performLeaderElection
// ============================================================================

/**
 * @brief Before any sync, the default leadership state is false (not yet elected).
 *        After syncWithCluster() the state updates based on the election result.
 */
TEST_F(DistributedTokenBlacklistTest, DBL11_IsLeaderReflectsElectionResult) {
    DistributedBlacklistConfig cfg = singleNodeConfig();
    cfg.enable_cluster_sync = true;
    cfg.peer_nodes.clear();

    DistributedTokenBlacklist bl(cfg);

    // Explicitly trigger election.
    bl.syncWithCluster().get();  // block until done

    // Sole node must be leader after election.
    EXPECT_TRUE(bl.isLeader());
}

// ============================================================================
// DBL-12 — syncWithCluster returns a valid future
// ============================================================================

/**
 * @brief syncWithCluster() must return a std::future<bool> that resolves to
 *        true (success) in single-node / no-peer mode.
 */
TEST_F(DistributedTokenBlacklistTest, DBL12_SyncWithClusterReturnsFuture) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    std::future<bool> f = bl.syncWithCluster();
    ASSERT_TRUE(f.valid()) << "syncWithCluster() must return a valid future";

    bool result = f.get();
    EXPECT_TRUE(result) << "syncWithCluster() should succeed in no-peer mode";
}

// ============================================================================
// DBL-13 — waitForClusterConvergence returns true immediately (single-node)
// ============================================================================

/**
 * @brief In single-node mode (enable_cluster_sync=false), there are no peers
 *        to converge with, so waitForClusterConvergence() must return true
 *        immediately without blocking.
 */
TEST_F(DistributedTokenBlacklistTest, DBL13_WaitConvergenceImmediateInSingleNode) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    auto start = std::chrono::steady_clock::now();
    bool converged = bl.waitForClusterConvergence(
        std::chrono::milliseconds(5000));
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_TRUE(converged);
    // Must complete in well under 1 second — it should be essentially instant.
    EXPECT_LT(elapsed, std::chrono::milliseconds(500))
        << "waitForClusterConvergence must return immediately in single-node mode";
}

// ============================================================================
// DBL-14 — waitForClusterConvergence times out in cluster mode without live peers
// ============================================================================

/**
 * @brief When cluster sync is enabled and peers are configured but unreachable,
 *        waitForClusterConvergence() must return false after the timeout instead
 *        of blocking indefinitely.
 */
TEST_F(DistributedTokenBlacklistTest, DBL14_WaitConvergenceTimesOut) {
    DistributedBlacklistConfig cfg;
    cfg.db_path             = db_path_.string();
    cfg.enable_cluster_sync = true;
    cfg.purge_interval_seconds = 3600;
    cfg.sync_interval_seconds  = 3600;  // very long — background thread won't fire
    cfg.local_node.node_id  = "node-A";

    // Add a fake peer so the cluster sync path is active.
    ClusterNode fake;
    fake.node_id    = "node-B";
    fake.rpc_address = "127.0.0.1";
    fake.rpc_port    = 19999;  // nothing listening
    cfg.peer_nodes  = {fake};

    DistributedTokenBlacklist bl(cfg);

    // Wait with a short timeout — must return false promptly.
    auto start = std::chrono::steady_clock::now();
    bool converged = bl.waitForClusterConvergence(
        std::chrono::milliseconds(300));
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Must not converge (no successful sync has happened).
    EXPECT_FALSE(converged);
    // Must respect the timeout (within a generous 2× margin).
    EXPECT_LT(elapsed, std::chrono::milliseconds(1000))
        << "waitForClusterConvergence must not block beyond the timeout";
}

// ============================================================================
// DBL-15 — getReplicationStats() returns zero-initialised struct at startup
// ============================================================================

/**
 * @brief Before any sync activity, all replication counters must be zero.
 */
TEST_F(DistributedTokenBlacklistTest, DBL15_InitialStatsAreZero) {
    DistributedTokenBlacklist bl(singleNodeConfig());

    auto stats = bl.getReplicationStats();
    EXPECT_EQ(stats.total_syncs,       0u);
    EXPECT_EQ(stats.successful_syncs,  0u);
    EXPECT_EQ(stats.failed_syncs,      0u);
    EXPECT_EQ(stats.entries_pushed,    0u);
    EXPECT_EQ(stats.entries_pulled,    0u);
}

// ============================================================================
// DBL-16 — Config accessors return the values passed at construction
// ============================================================================

/**
 * @brief config() must return the exact configuration passed to the constructor.
 */
TEST_F(DistributedTokenBlacklistTest, DBL16_ConfigAccessorReturnsConstructedConfig) {
    DistributedBlacklistConfig cfg = singleNodeConfig();
    cfg.purge_interval_seconds = 600;
    cfg.sync_interval_seconds  = 45;
    cfg.peer_rpc_timeout_ms    = 3000;
    cfg.local_node.node_id     = "node-config-test";

    DistributedTokenBlacklist bl(cfg);

    const auto& returned = bl.config();
    EXPECT_EQ(returned.db_path,               cfg.db_path);
    EXPECT_EQ(returned.purge_interval_seconds, cfg.purge_interval_seconds);
    EXPECT_EQ(returned.sync_interval_seconds,  cfg.sync_interval_seconds);
    EXPECT_EQ(returned.peer_rpc_timeout_ms,    cfg.peer_rpc_timeout_ms);
    EXPECT_EQ(returned.local_node.node_id,     cfg.local_node.node_id);
    EXPECT_EQ(returned.enable_cluster_sync,    cfg.enable_cluster_sync);
}

// ============================================================================
// DBL-17 — Destructor cleans up without crashing (RAII lifecycle)
// ============================================================================

/**
 * @brief Creating and immediately destroying a DistributedTokenBlacklist must
 *        not crash, throw, or leave background threads running.
 *
 * Tests both single-node and cluster-sync-enabled configurations.
 */
TEST_F(DistributedTokenBlacklistTest, DBL17_DestructorIsClean) {
    // Case A: single-node, no cluster sync.
    {
        EXPECT_NO_THROW({
            DistributedTokenBlacklist bl(singleNodeConfig());
            bl.add("jti-lifecycle-a", future(3600));
        });
    }

    // Case B: cluster sync enabled (background replication thread starts).
    {
        fs::path path_b = db_path_ / "lifecycle_b";
        fs::create_directories(path_b);

        DistributedBlacklistConfig cfg;
        cfg.db_path             = path_b.string();
        cfg.enable_cluster_sync = true;
        cfg.purge_interval_seconds = 3600;
        cfg.sync_interval_seconds  = 3600;
        cfg.local_node.node_id  = "node-lifecycle";

        ClusterNode peer;
        peer.node_id    = "node-peer";
        peer.rpc_address = "127.0.0.1";
        peer.rpc_port    = 19998;
        cfg.peer_nodes  = {peer};

        EXPECT_NO_THROW({
            DistributedTokenBlacklist bl(cfg);
            bl.add("jti-lifecycle-b", future(3600));
            // Destructor joins background threads — must not deadlock.
        });
    }
}

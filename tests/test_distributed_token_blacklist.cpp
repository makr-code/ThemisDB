#include <gtest/gtest.h>

#include "auth/distributed_token_blacklist.h"
#include "auth/token_blacklist.h"

#include <thread>
#include <chrono>
#include <vector>
#include <filesystem>

using namespace themis::auth;
namespace fs = std::filesystem;

// ===========================================================================
// Helper: Temporary database cleanup
// ===========================================================================

class DistributedBlacklistTest : public ::testing::Test {
protected:
    std::string temp_db_path;
    
    void SetUp() override {
        // Create a unique temp directory for each test
        temp_db_path = "/tmp/themis_test_blacklist_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        fs::create_directories(temp_db_path);
    }
    
    void TearDown() override {
        // Clean up
        if (fs::exists(temp_db_path)) {
            fs::remove_all(temp_db_path);
        }
    }
};

// ===========================================================================
// Single-node tests
// ===========================================================================

TEST_F(DistributedBlacklistTest, ConstructorSucceeds)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;  // Single-node mode
    
    DistributedTokenBlacklist blacklist(cfg);
    
    EXPECT_EQ(blacklist.config().db_path, temp_db_path);
    EXPECT_FALSE(blacklist.config().enable_cluster_sync);
}

TEST_F(DistributedBlacklistTest, AddAndCheckRevocation)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    // Not revoked initially
    EXPECT_FALSE(blacklist.isRevoked("token123"));
    
    // Add to blacklist (valid for 1 hour)
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    blacklist.add("token123", expiry);
    
    // Now it should be revoked
    EXPECT_TRUE(blacklist.isRevoked("token123"));
}

TEST_F(DistributedBlacklistTest, ExpiredTokensNotRevoked)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    // Add to blacklist (already expired)
    auto expiry = std::chrono::system_clock::now() - std::chrono::seconds(1);
    blacklist.add("expired_token", expiry);
    
    // Expired tokens should not be considered revoked
    // (depends on implementation detail - some systems may keep them for audit)
    // For this test, we check that purge removes them
    blacklist.purgeExpired();
    
    EXPECT_FALSE(blacklist.isRevoked("expired_token"));
}

TEST_F(DistributedBlacklistTest, MultipleTokensHandled)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    
    // Add multiple tokens
    for (int i = 0; i < 100; ++i) {
        std::string jti = "token_" + std::to_string(i);
        blacklist.add(jti, expiry);
    }
    
    // Verify all are revoked
    for (int i = 0; i < 100; ++i) {
        std::string jti = "token_" + std::to_string(i);
        EXPECT_TRUE(blacklist.isRevoked(jti)) << "Token " << i << " should be revoked";
    }
    
    // Verify unrelated tokens are not revoked
    EXPECT_FALSE(blacklist.isRevoked("unknown_token"));
}

TEST_F(DistributedBlacklistTest, PurgeRemovesExpiredEntries)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    cfg.purge_interval_seconds = 1;  // Aggressive purging for testing
    
    DistributedTokenBlacklist blacklist(cfg);
    
    // Add an already-expired token
    auto expired = std::chrono::system_clock::now() - std::chrono::seconds(10);
    blacklist.add("old_token", expired);
    
    // Add a valid token
    auto valid = std::chrono::system_clock::now() + std::chrono::hours(1);
    blacklist.add("new_token", valid);
    
    // Purge
    blacklist.purgeExpired();
    
    // Old token should be gone, new one remains
    EXPECT_FALSE(blacklist.isRevoked("old_token"));
    EXPECT_TRUE(blacklist.isRevoked("new_token"));
}

// ===========================================================================
// Persistence tests
// ===========================================================================

TEST_F(DistributedBlacklistTest, PersistenceAcrossInstances)
{
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    
    // First instance: add tokens
    {
        DistributedBlacklistConfig cfg;
        cfg.db_path = temp_db_path;
        cfg.enable_cluster_sync = false;
        
        DistributedTokenBlacklist blacklist(cfg);
        blacklist.add("persistent_token", expiry);
        
        EXPECT_TRUE(blacklist.isRevoked("persistent_token"));
    }
    
    // Second instance: should see persisted tokens
    {
        DistributedBlacklistConfig cfg;
        cfg.db_path = temp_db_path;
        cfg.enable_cluster_sync = false;
        
        DistributedTokenBlacklist blacklist(cfg);
        
        // Token should still be revoked (from previous instance)
        EXPECT_TRUE(blacklist.isRevoked("persistent_token"));
    }
}

// ===========================================================================
// Cluster synchronization tests
// ===========================================================================

TEST_F(DistributedBlacklistTest, ClusterSyncConfigured)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = true;
    
    ClusterNode local;
    local.node_id = "node-1";
    local.rpc_address = "127.0.0.1";
    local.rpc_port = 9090;
    cfg.local_node = local;
    
    ClusterNode peer;
    peer.node_id = "node-2";
    peer.rpc_address = "127.0.0.1";
    peer.rpc_port = 9091;
    cfg.peer_nodes.push_back(peer);
    
    DistributedTokenBlacklist blacklist(cfg);
    
    EXPECT_TRUE(blacklist.config().enable_cluster_sync);
    EXPECT_EQ(blacklist.config().peer_nodes.size(), 1);
}

TEST_F(DistributedBlacklistTest, LeaderElectionSimulated)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = true;
    cfg.sync_interval_seconds = 60;  // Don't actually sync in tests
    
    ClusterNode local;
    local.node_id = "node-0";  // Lowest ID
    local.rpc_address = "127.0.0.1";
    cfg.local_node = local;
    
    ClusterNode peer;
    peer.node_id = "node-1";
    peer.rpc_address = "127.0.0.1";
    cfg.peer_nodes.push_back(peer);
    
    DistributedTokenBlacklist blacklist(cfg);
    
    // Node with lowest ID should be leader
    EXPECT_TRUE(blacklist.isLeader());
}

TEST_F(DistributedBlacklistTest, SyncWithClusterReturnsFuture)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;  // Single node for this test
    
    DistributedTokenBlacklist blacklist(cfg);
    
    auto future = blacklist.syncWithCluster();
    
    EXPECT_TRUE(future.valid());
    
    // Wait for it (should be fast in single-node mode)
    bool result = future.get();
    EXPECT_TRUE(result);
}

TEST_F(DistributedBlacklistTest, ReplicationStatisticsTracked)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = true;
    cfg.sync_interval_seconds = 60;
    
    ClusterNode local;
    local.node_id = "node-1";
    cfg.local_node = local;
    cfg.peer_nodes.push_back(ClusterNode{"node-2", "127.0.0.1", 9091});
    
    DistributedTokenBlacklist blacklist(cfg);
    
    auto stats = blacklist.getReplicationStats();
    
    EXPECT_GE(stats.total_syncs, 0);
    EXPECT_EQ(stats.successful_syncs, 0);  // No sync triggered yet
    EXPECT_EQ(stats.failed_syncs, 0);
}

TEST_F(DistributedBlacklistTest, WaitForConvergenceSingleNode)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    // Single-node deployment should converge immediately
    bool converged = blacklist.waitForClusterConvergence(
        std::chrono::milliseconds(100));
    
    EXPECT_TRUE(converged);
}

// ===========================================================================
// Thread safety tests
// ===========================================================================

TEST_F(DistributedBlacklistTest, ConcurrentAddsAndChecks)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    
    // Writer threads
    std::vector<std::thread> writers;
    for (int i = 0; i < 3; ++i) {
        writers.emplace_back([&, i]() {
            for (int j = 0; j < 50; ++j) {
                std::string jti = "token_" + std::to_string(i) + "_" + std::to_string(j);
                blacklist.add(jti, expiry);
            }
        });
    }
    
    // Reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 3; ++i) {
        readers.emplace_back([&, i]() {
            for (int j = 0; j < 50; ++j) {
                std::string jti = "token_" + std::to_string(i) + "_" + std::to_string(j);
                // Just check; result depends on timing
                blacklist.isRevoked(jti);
            }
        });
    }
    
    // Wait for all
    for (auto& t : writers) {
      t.join();
    }
    for (auto& t : readers) {
      t.join();
    }
    
    // Verify all tokens were added
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 50; ++j) {
            std::string jti = "token_" + std::to_string(i) + "_" + std::to_string(j);
            EXPECT_TRUE(blacklist.isRevoked(jti));
        }
    }
}

TEST_F(DistributedBlacklistTest, PurgeDoesNotInterfereFWithReads)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.enable_cluster_sync = false;
    cfg.purge_interval_seconds = 1;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    
    // Add initial tokens
    for (int i = 0; i < 100; ++i) {
        std::string jti = "token_" + std::to_string(i);
        blacklist.add(jti, expiry);
    }
    
    // Concurrent reads and purge
    std::thread purge_thread([&]() {
        for (int i = 0; i < 5; ++i) {
            blacklist.purgeExpired();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    std::thread read_thread([&]() {
        for (int i = 0; i < 100; ++i) {
            for (int j = 0; j < 100; ++j) {
                std::string jti = "token_" + std::to_string(j);
                blacklist.isRevoked(jti);
            }
        }
    });
    
    purge_thread.join();
    read_thread.join();
}

// ===========================================================================
// Configuration boundary tests
// ===========================================================================

TEST_F(DistributedBlacklistTest, ConfiguresColumnFamily)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.column_family = "custom_blacklist";
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    EXPECT_EQ(blacklist.config().column_family, "custom_blacklist");
}

TEST_F(DistributedBlacklistTest, ConfiguresPurgeInterval)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.purge_interval_seconds = 600;
    cfg.enable_cluster_sync = false;
    
    DistributedTokenBlacklist blacklist(cfg);
    
    EXPECT_EQ(blacklist.config().purge_interval_seconds, 600);
}

TEST_F(DistributedBlacklistTest, ConfiguresSyncInterval)
{
    DistributedBlacklistConfig cfg;
    cfg.db_path = temp_db_path;
    cfg.sync_interval_seconds = 120;
    cfg.enable_cluster_sync = true;
    cfg.local_node.node_id = "node-1";
    cfg.peer_nodes.push_back(ClusterNode{"node-2", "127.0.0.1", 9091});
    
    DistributedTokenBlacklist blacklist(cfg);
    
    EXPECT_EQ(blacklist.config().sync_interval_seconds, 120);
}


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include "cache/cache_replication_coordinator.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: create a small AdaptiveQueryCache config that avoids RocksDB I/O
// ---------------------------------------------------------------------------
static AdaptiveQueryCache::Config makeTestConfig(const std::string& db_suffix = "") {
    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path = "/tmp/themis_repl_test_cache_" + db_suffix + "_" +
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    cfg.l1_max_entries  = 20;
    cfg.l2_max_entries  = 40;
    cfg.l1_ttl_seconds  = 300;
    cfg.l2_ttl_seconds  = 600;
    cfg.l3_ttl_seconds  = 3600;
    cfg.enable_replication = true;
    return cfg;
}

// ---------------------------------------------------------------------------
// Test: InProcessCacheCoordinator standalone (no bus)
// ---------------------------------------------------------------------------
TEST(InProcessCacheCoordinatorTest, StandalonePublishNoOp) {
    auto coord = std::make_shared<cache::InProcessCacheCoordinator>();
    EXPECT_TRUE(coord->isConnected());
    EXPECT_EQ(coord->name(), "InProcessCacheCoordinator");

    // Publish without any subscribers – should not throw
    EXPECT_NO_THROW(coord->publishEntry("key1", {{"a", 1}}, 300, "tenant1"));
    EXPECT_NO_THROW(coord->publishInvalidation(".*pattern.*"));

    auto stats = coord->getStats();
    EXPECT_EQ(stats["messages_sent"].get<uint64_t>(), 2u);
    EXPECT_EQ(stats["messages_received"].get<uint64_t>(), 0u);
}

// ---------------------------------------------------------------------------
// Test: InProcessCacheCoordinator with bus – two coordinators exchange msgs
// ---------------------------------------------------------------------------
TEST(InProcessCacheCoordinatorTest, BusDeliversEntriesToPeer) {
    auto bus = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus);
    auto coord_b = std::make_shared<cache::InProcessCacheCoordinator>(bus);

    // Register callback on coordinator B
    std::vector<cache::ReplicationMessage> received;
    coord_b->subscribeEntries([&received](const cache::ReplicationMessage& msg) {
        received.push_back(msg);
    });

    // Publish from A – B should receive it
    coord_a->publishEntry("fp_abc", {{"result", 42}}, 120, "acme");

    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].type, cache::ReplicationMessage::Type::ENTRY_PUT);
    EXPECT_EQ(received[0].key, "fp_abc");
    EXPECT_EQ(received[0].tenant_id, "acme");
    EXPECT_EQ(received[0].ttl_seconds, 120);
    EXPECT_EQ(received[0].result["result"].get<int>(), 42);
}

TEST(InProcessCacheCoordinatorTest, BusDeliversInvalidationsToPeer) {
    auto bus = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus);
    auto coord_b = std::make_shared<cache::InProcessCacheCoordinator>(bus);

    std::vector<cache::ReplicationMessage> received;
    coord_b->subscribeInvalidations([&received](const cache::ReplicationMessage& msg) {
        received.push_back(msg);
    });

    coord_a->publishInvalidation("users.*", "tenant_x");

    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].type, cache::ReplicationMessage::Type::INVALIDATE);
    EXPECT_EQ(received[0].key, "users.*");
    EXPECT_EQ(received[0].tenant_id, "tenant_x");
}

TEST(InProcessCacheCoordinatorTest, PublisherDoesNotReceiveOwnMessages) {
    auto bus = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus);

    bool self_received = false;
    coord_a->subscribeEntries([&self_received](const cache::ReplicationMessage&) {
        self_received = true;
    });

    coord_a->publishEntry("key", {{}}, 60, "");
    EXPECT_FALSE(self_received);
}

TEST(InProcessCacheCoordinatorTest, StatsReflectMessageCounts) {
    auto bus = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus);
    auto coord_b = std::make_shared<cache::InProcessCacheCoordinator>(bus);

    coord_b->subscribeEntries([](const cache::ReplicationMessage&) {});

    coord_a->publishEntry("k1", {{}}, 10, "");
    coord_a->publishEntry("k2", {{}}, 20, "");
    coord_a->publishInvalidation(".*");

    auto stats_a = coord_a->getStats();
    EXPECT_EQ(stats_a["messages_sent"].get<uint64_t>(), 3u);

    auto stats_b = coord_b->getStats();
    EXPECT_EQ(stats_b["messages_received"].get<uint64_t>(), 3u);
}

// ---------------------------------------------------------------------------
// Test: AdaptiveQueryCache + replication coordinator integration
// ---------------------------------------------------------------------------
class CacheReplicationIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        bus_    = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
        coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus_);
        coord_b = std::make_shared<cache::InProcessCacheCoordinator>(bus_);

        auto cfg_a = makeTestConfig("a");
        auto cfg_b = makeTestConfig("b");
        db_path_a_ = cfg_a.l3_db_path;
        db_path_b_ = cfg_b.l3_db_path;

        cache_a = std::make_unique<AdaptiveQueryCache>(cfg_a);
        cache_b = std::make_unique<AdaptiveQueryCache>(cfg_b);

        cache_a->setCoordinator(coord_a);
        cache_b->setCoordinator(coord_b);
    }

    void TearDown() override {
        cache_a.reset();
        cache_b.reset();
        std::filesystem::remove_all(db_path_a_);
        std::filesystem::remove_all(db_path_b_);
    }

    std::shared_ptr<cache::InProcessCacheCoordinator::Bus> bus_;
    std::shared_ptr<cache::InProcessCacheCoordinator>      coord_a;
    std::shared_ptr<cache::InProcessCacheCoordinator>      coord_b;
    std::unique_ptr<AdaptiveQueryCache>                    cache_a;
    std::unique_ptr<AdaptiveQueryCache>                    cache_b;
    std::string db_path_a_;
    std::string db_path_b_;
};

TEST_F(CacheReplicationIntegrationTest, PutOnAReplicatesToB) {
    json result = {{"data", {1, 2, 3}}};
    std::string fp = cache_a->generateFingerprint("SELECT 1", {});

    bool stored = cache_a->put(fp, {}, result);
    ASSERT_TRUE(stored);

    // cache_b should have received the replicated entry
    auto entry = cache_b->get(fp);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->result["data"][0].get<int>(), 1);
}

TEST_F(CacheReplicationIntegrationTest, InvalidateOnAPropagatestoB) {
    json result = {{"x", 99}};
    std::string fp = cache_a->generateFingerprint("SELECT x", {});

    // Put in both caches directly so B has the entry
    cache_b->put(fp, {}, result);
    ASSERT_TRUE(cache_b->get(fp).has_value());

    // Invalidate from A – should propagate to B
    cache_a->invalidate(".*");  // matches everything

    // B should have evicted the entry from L1/L2
    EXPECT_FALSE(cache_b->get(fp).has_value());
}

TEST_F(CacheReplicationIntegrationTest, GracefulDegradationWhenCoordinatorRemoved) {
    // Remove coordinator from A – puts should still succeed locally
    cache_a->setCoordinator(nullptr);

    json result = {{"val", 7}};
    std::string fp = cache_a->generateFingerprint("SELECT 7", {});
    EXPECT_TRUE(cache_a->put(fp, {}, result));
    EXPECT_TRUE(cache_a->get(fp).has_value());

    // B should NOT have received the entry (coordinator is gone)
    EXPECT_FALSE(cache_b->get(fp).has_value());
}

TEST_F(CacheReplicationIntegrationTest, GetReplicationStatsReturnsEnabled) {
    auto stats = cache_a->getReplicationStats();
    EXPECT_TRUE(stats["enabled"].get<bool>());
    EXPECT_EQ(stats["name"].get<std::string>(), "InProcessCacheCoordinator");
}

TEST_F(CacheReplicationIntegrationTest, GetReplicationStatsDisabledWhenNoCoordinator) {
    cache_a->setCoordinator(nullptr);
    auto stats = cache_a->getReplicationStats();
    EXPECT_FALSE(stats["enabled"].get<bool>());
}

TEST_F(CacheReplicationIntegrationTest, PutWithReplicationDisabledNoMessagesPublished) {
    // Create a cache with enable_replication = false
    auto cfg = makeTestConfig("norep");
    cfg.enable_replication = false;
    std::string db_path = cfg.l3_db_path;

    auto cache_no_rep = std::make_unique<AdaptiveQueryCache>(cfg);
    auto coord_c = std::make_shared<cache::InProcessCacheCoordinator>(bus_);

    bool received = false;
    coord_c->subscribeEntries([&received](const cache::ReplicationMessage&) {
        received = true;
    });
    cache_no_rep->setCoordinator(coord_c);

    std::string fp = cache_no_rep->generateFingerprint("NO REPLICATE", {});
    cache_no_rep->put(fp, {}, {{"v", 1}});

    EXPECT_FALSE(received);

    cache_no_rep.reset();
    std::filesystem::remove_all(db_path);
}

// ---------------------------------------------------------------------------
// Test: three-node bus – entry put on A replicates to B and C
// ---------------------------------------------------------------------------
TEST_F(CacheReplicationIntegrationTest, InvalidateTenantPropagatesToB) {
    // Both caches use tenant isolation
    auto cfg_a = makeTestConfig("tena");
    auto cfg_b = makeTestConfig("tenb");
    cfg_a.enable_tenant_isolation = true;
    cfg_b.enable_tenant_isolation = true;
    std::string dp_a = cfg_a.l3_db_path;
    std::string dp_b = cfg_b.l3_db_path;

    auto ta = std::make_unique<AdaptiveQueryCache>(cfg_a);
    auto tb = std::make_unique<AdaptiveQueryCache>(cfg_b);

    auto bus2   = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto ca_ten = std::make_shared<cache::InProcessCacheCoordinator>(bus2);
    auto cb_ten = std::make_shared<cache::InProcessCacheCoordinator>(bus2);
    ta->setCoordinator(ca_ten);
    tb->setCoordinator(cb_ten);

    const std::string tenant = "acme";
    json result = {{"v", 42}};
    std::string fp = ta->generateFingerprint("SELECT v", {}, tenant);

    // Put into tb directly with tenant isolation
    tb->put(fp, {}, result, tenant);
    ASSERT_TRUE(tb->get(fp, tenant).has_value());

    // Invalidate tenant on ta → should propagate to tb
    ta->invalidateTenant(tenant);

    // tb must have evicted the tenant-scoped entry from L1/L2
    EXPECT_FALSE(tb->get(fp, tenant).has_value());

    ta.reset(); tb.reset();
    std::filesystem::remove_all(dp_a);
    std::filesystem::remove_all(dp_b);
}

TEST(CacheReplicationThreeNodeTest, EntryReplicatesToAllPeers) {
    auto bus = std::make_shared<cache::InProcessCacheCoordinator::Bus>();
    auto coord_a = std::make_shared<cache::InProcessCacheCoordinator>(bus);
    auto coord_b = std::make_shared<cache::InProcessCacheCoordinator>(bus);
    auto coord_c = std::make_shared<cache::InProcessCacheCoordinator>(bus);

    auto make_cfg = [](const std::string& suffix) {
        return makeTestConfig(suffix);
    };
    auto cfg_a = make_cfg("3a");
    auto cfg_b = make_cfg("3b");
    auto cfg_c = make_cfg("3c");
    std::string dp_a = cfg_a.l3_db_path;
    std::string dp_b = cfg_b.l3_db_path;
    std::string dp_c = cfg_c.l3_db_path;

    auto cache_a = std::make_unique<AdaptiveQueryCache>(cfg_a);
    auto cache_b = std::make_unique<AdaptiveQueryCache>(cfg_b);
    auto cache_c = std::make_unique<AdaptiveQueryCache>(cfg_c);

    cache_a->setCoordinator(coord_a);
    cache_b->setCoordinator(coord_b);
    cache_c->setCoordinator(coord_c);

    std::string fp = cache_a->generateFingerprint("SELECT 3", {});
    cache_a->put(fp, {}, {{"nodes", 3}});

    EXPECT_TRUE(cache_b->get(fp).has_value());
    EXPECT_TRUE(cache_c->get(fp).has_value());

    cache_a.reset(); cache_b.reset(); cache_c.reset();
    std::filesystem::remove_all(dp_a);
    std::filesystem::remove_all(dp_b);
    std::filesystem::remove_all(dp_c);
}

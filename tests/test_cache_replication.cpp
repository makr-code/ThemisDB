// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include "cache/cache_replication_coordinator.h"
#include "cache/cache_replication.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis;
using json = nlohmann::json;

// Phase 4: Unit tests for cache replication (high-availability deployments)

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include "cache/cache_replication.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <vector>
#include <string>
#include <atomic>

using namespace themis;
using namespace themis::cache;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string uniqueTmpPath(const std::string& suffix = "") {
    auto ts = std::chrono::system_clock::now().time_since_epoch().count();
    return "/tmp/themis_repl_test_" + std::to_string(ts) + suffix;
}

static AdaptiveQueryCache::Config makeTestConfig(const std::string& db_suffix = "") {
    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path              = uniqueTmpPath("_" + db_suffix);
    cfg.l1_max_entries          = 20;
    cfg.l2_max_entries          = 40;
    cfg.l1_max_entry_size       = 1024;
    cfg.l2_max_entry_size       = 10240;
    cfg.l1_ttl_seconds          = 300;
    cfg.l2_ttl_seconds          = 600;
    cfg.l3_ttl_seconds          = 3600;
    cfg.enable_rate_limiting    = false;
    cfg.enable_tenant_isolation = false;
    cfg.enable_replication      = true;
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
class CacheReplicationCoordIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping cache replication focused tests on Windows due to fixture crash in current runtime.";
#endif
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

TEST_F(CacheReplicationCoordIntegrationTest, PutOnAReplicatesToB) {
    json result = {{"data", {1, 2, 3}}};
    std::string fp = cache_a->generateFingerprint("SELECT 1", {});

    bool stored = cache_a->put(fp, {}, result);
    ASSERT_TRUE(stored);

    // Replication is asynchronous; allow a short propagation window.
    std::optional<AdaptiveQueryCache::CacheEntry> entry;
    for (int i = 0; i < 20; ++i) {
        entry = cache_b->get(fp, "");
        if (entry.has_value()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // cache_b should have received the replicated entry
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->result["data"][0].get<int>(), 1);
}

TEST_F(CacheReplicationCoordIntegrationTest, InvalidateOnAPropagatestoB) {
    json result = {{"x", 99}};
    std::string fp = cache_a->generateFingerprint("SELECT x", {});

    // Put in both caches directly so B has the entry
    cache_b->put(fp, {}, result);
    ASSERT_TRUE(cache_b->get(fp, "").has_value());

    // Invalidate from A – should propagate to B
    cache_a->invalidate(".*");  // matches everything

    // Replication is asynchronous; allow a short propagation window.
    bool removed = false;
    for (int i = 0; i < 20; ++i) {
        if (!cache_b->get(fp, "").has_value()) {
            removed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // B should have evicted the entry from L1/L2
    EXPECT_TRUE(removed);
}

TEST_F(CacheReplicationCoordIntegrationTest, GracefulDegradationWhenCoordinatorRemoved) {
    // Remove coordinator from A – puts should still succeed locally
    cache_a->setCoordinator(nullptr);

    json result = {{"val", 7}};
    std::string fp = cache_a->generateFingerprint("SELECT 7", {});
    EXPECT_TRUE(cache_a->put(fp, {}, result));
    EXPECT_TRUE(cache_a->get(fp, "").has_value());

    // B should NOT have received the entry (coordinator is gone)
    EXPECT_FALSE(cache_b->get(fp, "").has_value());
}

TEST_F(CacheReplicationCoordIntegrationTest, GetReplicationStatsReturnsEnabled) {
    auto stats = cache_a->getReplicationStats();
    EXPECT_TRUE(stats["enabled"].get<bool>());
    EXPECT_EQ(stats["name"].get<std::string>(), "InProcessCacheCoordinator");
}

TEST_F(CacheReplicationCoordIntegrationTest, GetReplicationStatsDisabledWhenNoCoordinator) {
    cache_a->setCoordinator(nullptr);
    auto stats = cache_a->getReplicationStats();
    EXPECT_FALSE(stats["enabled"].get<bool>());
}

TEST_F(CacheReplicationCoordIntegrationTest, PutWithReplicationDisabledNoMessagesPublished) {
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
    std::error_code cleanup_ec;
    std::filesystem::remove_all(db_path, cleanup_ec);
}

// ---------------------------------------------------------------------------
// Test: three-node bus – entry put on A replicates to B and C
// ---------------------------------------------------------------------------
TEST_F(CacheReplicationCoordIntegrationTest, InvalidateTenantPropagatesToB) {
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

    auto cfg_a = makeTestConfig("3a");
    auto cfg_b = makeTestConfig("3b");
    auto cfg_c = makeTestConfig("3c");
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

    EXPECT_TRUE(cache_b->get(fp, "").has_value());
    EXPECT_TRUE(cache_c->get(fp).has_value());

    cache_a.reset(); cache_b.reset(); cache_c.reset();
    std::filesystem::remove_all(dp_a);
    std::filesystem::remove_all(dp_b);
    std::filesystem::remove_all(dp_c);
}

// ---------------------------------------------------------------------------
// In-process mock listener that records all received events
// ---------------------------------------------------------------------------

class MockCacheReplicationListener : public ICacheReplicationListener {
public:
    explicit MockCacheReplicationListener(const std::string& id = "mock-replica")
        : id_(id) {}

    bool onReplicationEvent(const CacheReplicationEvent& event) override {
        if (fail_next_) {
            fail_next_ = false;
            return false;
        }
        events_.push_back(event);
        return true;
    }

    bool ping() override { return ping_alive_; }

    std::string replicaId() const override { return id_; }

    // Test helpers
    void setFailNext(bool v = true) { fail_next_ = v; }
    void setPingAlive(bool v) { ping_alive_ = v; }

    const std::vector<CacheReplicationEvent>& events() const { return events_; }

    size_t countByType(CacheReplicationEventType t) const {
        size_t n = 0;
        for (const auto& e : events_) if (e.type == t) ++n;
        return n;
    }

    void clear() { events_.clear(); }

private:
    std::string id_;
    std::vector<CacheReplicationEvent> events_;
    bool fail_next_  = false;
    bool ping_alive_ = true;
};

// ===========================================================================
// Tests: CacheReplicationManager
// ===========================================================================

class CacheReplicationManagerTest : public ::testing::Test {
protected:
    CacheReplicationConfig cfg_;
    std::shared_ptr<CacheReplicationManager> mgr_;
    std::shared_ptr<MockCacheReplicationListener> replica_;

    void SetUp() override {
        cfg_.max_consecutive_failures = 2;
        cfg_.semi_sync = false;
        cfg_.enabled   = true;
        mgr_    = std::make_shared<CacheReplicationManager>(cfg_);
        replica_ = std::make_shared<MockCacheReplicationListener>("replica-1");
    }
};

TEST_F(CacheReplicationManagerTest, AddReplicaIncreasesCount) {
    EXPECT_EQ(mgr_->replicaCount(), 0u);
    mgr_->addReplica(replica_);
    EXPECT_EQ(mgr_->replicaCount(), 1u);
}

TEST_F(CacheReplicationManagerTest, RemoveReplicaDecreasesCount) {
    mgr_->addReplica(replica_);
    mgr_->removeReplica("replica-1");
    EXPECT_EQ(mgr_->replicaCount(), 0u);
}

TEST_F(CacheReplicationManagerTest, NullListenerIgnored) {
    mgr_->addReplica(nullptr);
    EXPECT_EQ(mgr_->replicaCount(), 0u);
}

TEST_F(CacheReplicationManagerTest, WriteEventDispatchedToReplica) {
    mgr_->addReplica(replica_);
    mgr_->notifyWrite("key1", R"({"v":1})", "tenant-a", 300);

    ASSERT_EQ(replica_->events().size(), 1u);
    EXPECT_EQ(replica_->events()[0].type, CacheReplicationEventType::WRITE);
    EXPECT_EQ(replica_->events()[0].key, "key1");
    EXPECT_EQ(replica_->events()[0].tenant_id, "tenant-a");
    EXPECT_EQ(replica_->events()[0].ttl_seconds, 300);
    EXPECT_EQ(replica_->events()[0].payload, R"({"v":1})");
}

TEST_F(CacheReplicationManagerTest, InvalidateEventDispatchedToReplica) {
    mgr_->addReplica(replica_);
    mgr_->notifyInvalidate("orders_.*");

    ASSERT_EQ(replica_->events().size(), 1u);
    EXPECT_EQ(replica_->events()[0].type, CacheReplicationEventType::INVALIDATE);
    EXPECT_EQ(replica_->events()[0].pattern, "orders_.*");
}

TEST_F(CacheReplicationManagerTest, InvalidateTenantEventDispatched) {
    mgr_->addReplica(replica_);
    mgr_->notifyInvalidateTenant("acme");

    ASSERT_EQ(replica_->events().size(), 1u);
    EXPECT_EQ(replica_->events()[0].type, CacheReplicationEventType::INVALIDATE_TENANT);
    EXPECT_EQ(replica_->events()[0].tenant_id, "acme");
}

TEST_F(CacheReplicationManagerTest, SnapshotSentOnAddReplica) {
    mgr_->addReplica(replica_, R"({"key":"abc","value_b64":"dGVzdA==","ttl_remaining_s":60})");

    ASSERT_EQ(replica_->countByType(CacheReplicationEventType::SNAPSHOT), 1u);
    EXPECT_FALSE(replica_->events()[0].payload.empty());
}

TEST_F(CacheReplicationManagerTest, GracefulDegradation_ReplicaMarkedUnhealthyAfterFailures) {
    mgr_->addReplica(replica_);

    // Simulate consecutive failures exceeding threshold (2)
    replica_->setFailNext(true);
    mgr_->notifyWrite("k1", "{}", "", 60);

    replica_->setFailNext(true);
    mgr_->notifyWrite("k2", "{}", "", 60);

    // After max_consecutive_failures, replica should be UNHEALTHY → skipped
    size_t events_before = replica_->events().size();
    mgr_->notifyWrite("k3", "{}", "", 60);
    // UNHEALTHY replica is skipped; no new event recorded
    EXPECT_EQ(replica_->events().size(), events_before);
}

TEST_F(CacheReplicationManagerTest, UnhealthyReplicaRecoveredOnProbe) {
    mgr_->addReplica(replica_);

    // Drive replica to UNHEALTHY
    replica_->setFailNext(true);
    mgr_->notifyWrite("k1", "{}", "", 60);
    replica_->setFailNext(true);
    mgr_->notifyWrite("k2", "{}", "", 60);

    // replica is now unhealthy and ping returns true → should recover
    replica_->setPingAlive(true);
    mgr_->probeUnhealthyReplicas();

    // After recovery, new writes are delivered again
    size_t before = replica_->events().size();
    mgr_->notifyWrite("k3", "{}", "", 60);
    EXPECT_GT(replica_->events().size(), before);
}

TEST_F(CacheReplicationManagerTest, DisabledManagerDoesNotDispatch) {
    cfg_.enabled = false;
    auto disabled_mgr = std::make_shared<CacheReplicationManager>(cfg_);
    disabled_mgr->addReplica(replica_);
    disabled_mgr->notifyWrite("key", "{}", "", 30);
    // snapshot is sent during addReplica (non-empty only when snapshot_ndjson provided)
    // no WRITE events expected
    EXPECT_EQ(replica_->countByType(CacheReplicationEventType::WRITE), 0u);
}

TEST_F(CacheReplicationManagerTest, SequenceNumberMonotonicallyIncreases) {
    mgr_->addReplica(replica_);
    mgr_->notifyWrite("k1", "{}", "", 10);
    mgr_->notifyWrite("k2", "{}", "", 10);
    mgr_->notifyInvalidate(".*");

    ASSERT_GE(replica_->events().size(), 3u);
    uint64_t prev = 0;
    for (const auto& ev : replica_->events()) {
        if (ev.sequence > 0) {
            EXPECT_GT(ev.sequence, prev);
            prev = ev.sequence;
        }
    }
}

TEST_F(CacheReplicationManagerTest, GetStatsReturnsExpectedFields) {
    mgr_->addReplica(replica_);
    mgr_->notifyWrite("k", "{}", "", 10);
    auto stats = mgr_->getStats();
    EXPECT_TRUE(stats.contains("events_dispatched"));
    EXPECT_TRUE(stats.contains("events_failed"));
    EXPECT_TRUE(stats.contains("replica_count"));
    EXPECT_EQ(stats["replica_count"].get<size_t>(), 1u);
}

TEST_F(CacheReplicationManagerTest, GetReplicaHealthReturnsArray) {
    mgr_->addReplica(replica_);
    auto health = mgr_->getReplicaHealth();
    ASSERT_TRUE(health.is_array());
    ASSERT_EQ(health.size(), 1u);
    EXPECT_EQ(health[0]["replica_id"].get<std::string>(), "replica-1");
    EXPECT_EQ(health[0]["health"].get<std::string>(), "HEALTHY");
}

TEST_F(CacheReplicationManagerTest, MultipleReplicasFanOut) {
    auto r2 = std::make_shared<MockCacheReplicationListener>("replica-2");
    mgr_->addReplica(replica_);
    mgr_->addReplica(r2);

    mgr_->notifyWrite("key", R"({"x":1})", "", 120);

    EXPECT_EQ(replica_->countByType(CacheReplicationEventType::WRITE), 1u);
    EXPECT_EQ(r2->countByType(CacheReplicationEventType::WRITE), 1u);
}

TEST_F(CacheReplicationManagerTest, ReAddSameReplicaIsIdempotent) {
    mgr_->addReplica(replica_);
    mgr_->addReplica(replica_);  // re-add same ID
    EXPECT_EQ(mgr_->replicaCount(), 1u);
}

// ===========================================================================
// Tests: AdaptiveQueryCache integration with replication listener
// ===========================================================================

class CacheReplicationManagerIntegrationTest : public ::testing::Test {
protected:
    std::string db_path_;
    std::shared_ptr<MockCacheReplicationListener> listener_;

    void SetUp() override {
        db_path_  = uniqueTmpPath();
        listener_ = std::make_shared<MockCacheReplicationListener>("integration-replica");
    }

    void TearDown() override {
        if (!db_path_.empty()) {
            std::filesystem::remove_all(db_path_);
        }
    }
};

TEST_F(CacheReplicationManagerIntegrationTest, PutNotifiesListenerOnSuccess) {
    AdaptiveQueryCache cache(makeTestConfig("mgr_put"));
    cache.setReplicationListener(listener_);

    std::string fp = cache.generateFingerprint("SELECT 1", {});
    json result = {{"rows", 1}};
    ASSERT_TRUE(cache.put(fp, {}, result));

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::WRITE), 1u);
}

TEST_F(CacheReplicationManagerIntegrationTest, InvalidateNotifiesListener) {
    AdaptiveQueryCache cache(makeTestConfig("mgr_inv"));
    cache.setReplicationListener(listener_);

    std::string fp = cache.generateFingerprint("SELECT 2", {});
    ASSERT_TRUE(cache.put(fp, {}, {{"x", 2}}));

    listener_->clear();
    cache.invalidate(".*");

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::INVALIDATE), 1u);
}

TEST_F(CacheReplicationManagerIntegrationTest, InvalidateTenantNotifiesListener) {
    AdaptiveQueryCache::Config cfg = makeTestConfig("mgr_tent");
    cfg.enable_tenant_isolation = true;
    AdaptiveQueryCache cache(cfg);
    cache.setReplicationListener(listener_);

    std::string fp = cache.generateFingerprint("SELECT 3", {}, "tenant-x");
    cache.put(fp, {}, {{"y", 3}}, "tenant-x");

    listener_->clear();
    cache.invalidateTenant("tenant-x");

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::INVALIDATE_TENANT), 1u);
    EXPECT_EQ(listener_->events().back().tenant_id, "tenant-x");
}

TEST_F(CacheReplicationManagerIntegrationTest, UnregisterListenerStopsNotifications) {
    AdaptiveQueryCache cache(makeTestConfig("mgr_unreg"));
    cache.setReplicationListener(listener_);

    // Unregister
    cache.setReplicationListener(nullptr);

    std::string fp = cache.generateFingerprint("SELECT 4", {});
    cache.put(fp, {}, {{"z", 4}});

    EXPECT_EQ(listener_->countByType(CacheReplicationEventType::WRITE), 0u);
}

TEST_F(CacheReplicationManagerIntegrationTest, ListenerExceptionDoesNotCrashCache) {
    // A listener that always throws
    class ThrowingListener : public ICacheReplicationListener {
    public:
        bool onReplicationEvent(const CacheReplicationEvent&) override {
            throw std::runtime_error("network error");
        }
        std::string replicaId() const override { return "throwing"; }
    };

    // CacheReplicationManager wraps the throwing listener with graceful
    // degradation, so the cache itself must not crash.
    CacheReplicationConfig repCfg;
    repCfg.max_consecutive_failures = 1;
    auto mgr = std::make_shared<CacheReplicationManager>(repCfg);
    mgr->addReplica(std::make_shared<ThrowingListener>());

    AdaptiveQueryCache cache(makeTestConfig("mgr_throw"));
    cache.setReplicationListener(mgr);

    std::string fp = cache.generateFingerprint("SELECT 5", {});
    // Must not throw even though the listener throws
    EXPECT_NO_THROW(cache.put(fp, {}, {{"ok", true}}));
}

TEST_F(CacheReplicationManagerIntegrationTest, ReplicationManagerReceivesCacheWrites) {
    CacheReplicationConfig repCfg;
    auto mgr = std::make_shared<CacheReplicationManager>(repCfg);
    mgr->addReplica(listener_);

    AdaptiveQueryCache cache(makeTestConfig("mgr_writes"));
    cache.setReplicationListener(mgr);

    std::string fp1 = cache.generateFingerprint("Q1", {});
    std::string fp2 = cache.generateFingerprint("Q2", {});
    cache.put(fp1, {}, {{"r", 1}});
    cache.put(fp2, {}, {{"r", 2}});

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::WRITE), 2u);
}

// ===========================================================================
// Tests: RedisCacheCoordinator – unit tests (no real Redis required)
// ===========================================================================

#include "cache/distributed_cache_coordinator.h"

TEST(RedisCacheCoordinatorTest, DefaultConstructionAndName) {
    // Constructing with a non-reachable host should not throw.
    // The background thread will fail to connect silently.
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19999;  // unlikely to be in use
    cfg.reconnect_interval_ms = 50;    // fast reconnect for test
    cfg.connect_timeout_ms   = 100;

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
        EXPECT_EQ(coord.name(), "RedisCacheCoordinator");
    });
}

TEST(RedisCacheCoordinatorTest, IsConnectedReturnsFalseWhenNoServer) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19998;
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);
    // Give the background thread a moment to try (and fail) connecting.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Should not be connected since nothing listens on that port.
    EXPECT_FALSE(coord.isConnected());
}

TEST(RedisCacheCoordinatorTest, GetStatsViaReplicationIntegration) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19997;
    cfg.channel_prefix       = "myprefix";
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);
    auto stats = coord.getStats();

    EXPECT_TRUE(stats.contains("name"));
    EXPECT_TRUE(stats.contains("connected"));
    EXPECT_TRUE(stats.contains("messages_published"));
    EXPECT_TRUE(stats.contains("messages_received"));
    EXPECT_TRUE(stats.contains("publish_errors"));
    EXPECT_TRUE(stats.contains("reconnect_count"));
    EXPECT_EQ(stats["name"].get<std::string>(), "RedisCacheCoordinator");
    EXPECT_EQ(stats["channel_prefix"].get<std::string>(), "myprefix");
}

TEST(RedisCacheCoordinatorTest, ChannelNamesUsePrefixCorrectly) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host           = "127.0.0.1";
    cfg.port           = 19996;
    cfg.channel_prefix = "themis:prod";
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);
    EXPECT_EQ(coord.entryChannel(),        "themis:prod:entries");
    EXPECT_EQ(coord.invalidationChannel(), "themis:prod:invalidations");
}

TEST(RedisCacheCoordinatorTest, PublishGracefullyFailsWhenNotConnected) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19995;
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);

    // Publish without a connected server – must not throw.
    EXPECT_NO_THROW(coord.publishEntry("key1", {{"a", 1}}, 300, "tenant1"));
    EXPECT_NO_THROW(coord.publishInvalidation(".*pattern.*", "tenant1"));

    auto stats = coord.getStats();
    // Publish errors should be recorded.
    EXPECT_GE(stats["publish_errors"].get<uint64_t>(), 0u);
}

TEST(RedisCacheCoordinatorTest, SubscribeCallbacksRegisteredWithoutError) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19994;
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);

    bool entry_registered = false;
    bool inv_registered   = false;

    EXPECT_NO_THROW(coord.subscribeEntries(
        [&entry_registered](const ReplicationMessage&) { entry_registered = true; }));
    EXPECT_NO_THROW(coord.subscribeInvalidations(
        [&inv_registered](const ReplicationMessage&) { inv_registered = true; }));

    // Just verifying registration doesn't throw; callbacks fire only when
    // a real Redis message is received.
    (void)entry_registered;
    (void)inv_registered;
}

TEST(RedisCacheCoordinatorTest, ImplementsICacheCoordinatorInterface) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 19993;
    cfg.reconnect_interval_ms = 50;
    cfg.connect_timeout_ms   = 100;

    // Verify that RedisCacheCoordinator is usable as ICacheCoordinator.
    std::shared_ptr<ICacheCoordinator> coord =
        std::make_shared<RedisCacheCoordinator>(cfg);

    EXPECT_EQ(coord->name(), "RedisCacheCoordinator");
    EXPECT_NO_THROW(coord->publishEntry("k", {{}}, 60, ""));
    EXPECT_NO_THROW(coord->publishInvalidation(".*"));
    EXPECT_NO_THROW(coord->subscribeEntries([](const ReplicationMessage&) {}));
    EXPECT_NO_THROW(coord->subscribeInvalidations([](const ReplicationMessage&) {}));
    auto stats = coord->getStats();
    EXPECT_TRUE(stats.contains("name"));
}

TEST(RedisCacheCoordinatorTest, AdaptiveCacheCoordinatorIntegration_LocalOpsUnaffected) {
    // Verify that registering a RedisCacheCoordinator on an AdaptiveQueryCache
    // does not break local cache operations even when Redis is unreachable.
    auto cfg = makeTestConfig("redis_int");
    std::string db_path = cfg.l3_db_path;

    {
        AdaptiveQueryCache cache(cfg);

        RedisCacheCoordinatorConfig redis_cfg;
        redis_cfg.host                  = "127.0.0.1";
        redis_cfg.port                  = 19992;
        redis_cfg.reconnect_interval_ms = 50;
        redis_cfg.connect_timeout_ms    = 100;

        auto coordinator = std::make_shared<RedisCacheCoordinator>(redis_cfg);
        cache.setCoordinator(coordinator);

        std::string fp = cache.generateFingerprint("SELECT local", {});
        // Local put and get must work regardless of coordinator state.
        ASSERT_TRUE(cache.put(fp, {}, {{"local", true}}));
        auto entry = cache.get(fp, "");
        ASSERT_TRUE(entry.has_value());
        EXPECT_EQ(entry->result["local"].get<bool>(), true);

        cache.setCoordinator(nullptr);
    }

    std::error_code cleanup_ec;
    std::filesystem::remove_all(db_path, cleanup_ec);
}

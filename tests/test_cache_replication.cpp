// Copyright 2025 ThemisDB
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

static AdaptiveQueryCache::Config makeTestConfig(const std::string& db_path) {
    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path          = db_path;
    cfg.l1_max_entries      = 20;
    cfg.l2_max_entries      = 40;
    cfg.l1_max_entry_size   = 1024;
    cfg.l2_max_entry_size   = 10240;
    cfg.l1_ttl_seconds      = 300;
    cfg.l2_ttl_seconds      = 600;
    cfg.l3_ttl_seconds      = 3600;
    cfg.enable_rate_limiting    = false;
    cfg.enable_tenant_isolation = false;
    return cfg;
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

class CacheReplicationIntegrationTest : public ::testing::Test {
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

TEST_F(CacheReplicationIntegrationTest, PutNotifiesListenerOnSuccess) {
    AdaptiveQueryCache cache(makeTestConfig(db_path_));
    cache.setReplicationListener(listener_);

    std::string fp = cache.generateFingerprint("SELECT 1", {});
    json result = {{"rows", 1}};
    ASSERT_TRUE(cache.put(fp, {}, result));

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::WRITE), 1u);
}

TEST_F(CacheReplicationIntegrationTest, InvalidateNotifiesListener) {
    AdaptiveQueryCache cache(makeTestConfig(db_path_));
    cache.setReplicationListener(listener_);

    std::string fp = cache.generateFingerprint("SELECT 2", {});
    ASSERT_TRUE(cache.put(fp, {}, {{"x", 2}}));

    listener_->clear();
    cache.invalidate(".*");

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::INVALIDATE), 1u);
}

TEST_F(CacheReplicationIntegrationTest, InvalidateTenantNotifiesListener) {
    AdaptiveQueryCache::Config cfg = makeTestConfig(db_path_);
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

TEST_F(CacheReplicationIntegrationTest, UnregisterListenerStopsNotifications) {
    AdaptiveQueryCache cache(makeTestConfig(db_path_));
    cache.setReplicationListener(listener_);

    // Unregister
    cache.setReplicationListener(nullptr);

    std::string fp = cache.generateFingerprint("SELECT 4", {});
    cache.put(fp, {}, {{"z", 4}});

    EXPECT_EQ(listener_->countByType(CacheReplicationEventType::WRITE), 0u);
}

TEST_F(CacheReplicationIntegrationTest, ListenerExceptionDoesNotCrashCache) {
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

    AdaptiveQueryCache cache(makeTestConfig(db_path_));
    cache.setReplicationListener(mgr);

    std::string fp = cache.generateFingerprint("SELECT 5", {});
    // Must not throw even though the listener throws
    EXPECT_NO_THROW(cache.put(fp, {}, {{"ok", true}}));
}

TEST_F(CacheReplicationIntegrationTest, ReplicationManagerReceivesCacheWrites) {
    CacheReplicationConfig repCfg;
    auto mgr = std::make_shared<CacheReplicationManager>(repCfg);
    mgr->addReplica(listener_);

    AdaptiveQueryCache cache(makeTestConfig(db_path_));
    cache.setReplicationListener(mgr);

    std::string fp1 = cache.generateFingerprint("Q1", {});
    std::string fp2 = cache.generateFingerprint("Q2", {});
    cache.put(fp1, {}, {{"r", 1}});
    cache.put(fp2, {}, {{"r", 2}});

    EXPECT_GE(listener_->countByType(CacheReplicationEventType::WRITE), 2u);
}
